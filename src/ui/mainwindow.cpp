#include "mainwindow.h"

#include "device/devicemanager.h"
#include "model/keymap.h"
#include "ui/devicepage.h"
#include "ui/lightingpage.h"
#include "ui/macrospage.h"
#include "ui/remappage.h"
#include "ui/settingspage.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

namespace f75 {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("AULA F75"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("input-keyboard")));
    resize(1040, 680);

    buildUi();

    m_connectTimer.setInterval(2000);
    connect(&m_connectTimer, &QTimer::timeout, this, &MainWindow::tryConnect);
    m_connectTimer.start();

    m_keepaliveTimer.setInterval(50);
    connect(&m_keepaliveTimer, &QTimer::timeout, this, &MainWindow::keepaliveTick);
    m_keepaliveTimer.start();

    tryConnect();
    updateStatus();
}

void MainWindow::buildUi()
{
    // Menu bar
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    auto* saveAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("document-save")),
                                           tr("&Save profile…"));
    auto* loadMenu = fileMenu->addMenu(QIcon::fromTheme(QStringLiteral("document-open")),
                                       tr("&Load profile"));
    auto* deleteAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-delete")),
                                             tr("&Delete profile…"));
    fileMenu->addSeparator();
    auto* quitAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")),
                                           tr("&Quit"));
    connect(quitAction, &QAction::triggered, this, [this] {
        m_forceQuit = true;
        qApp->quit();
    });

    auto refreshProfiles = [loadMenu] {
        loadMenu->clear();
        const QStringList names = Profile::list();
        if (names.isEmpty())
        {
            loadMenu->addAction(tr("(no profiles)"))->setEnabled(false);
            return;
        }
        for (const QString& n : names)
        {
            loadMenu->addAction(n);
        }
    };
    refreshProfiles();

    connect(saveAction, &QAction::triggered, this, [this, refreshProfiles] {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("Save profile"),
                                                   tr("Name:"), QLineEdit::Normal,
                                                   m_profile.name.isEmpty()
                                                       ? tr("My profile") : m_profile.name, &ok);
        if (ok && !name.isEmpty())
        {
            m_profile.name = name;
            m_profile.save();
            refreshProfiles();
            statusBar()->showMessage(tr("Saved profile “%1”").arg(name), 3000);
        }
    });
    connect(loadMenu, &QMenu::triggered, this, [this](QAction* a) {
        if (a->text().isEmpty())
        {
            return;
        }
        m_profile = Profile::load(a->text());
        m_pageLighting->refreshFromProfile();
        sendPerKey(true);
        statusBar()->showMessage(tr("Loaded profile “%1”").arg(a->text()), 3000);
    });
    connect(deleteAction, &QAction::triggered, this, [this, refreshProfiles] {
        const QStringList names = Profile::list();
        if (names.isEmpty())
        {
            return;
        }
        bool ok = false;
        const QString name = QInputDialog::getItem(this, tr("Delete profile"), tr("Profile:"),
                                                   names, 0, false, &ok);
        if (ok && !name.isEmpty())
        {
            Profile::remove(name);
            refreshProfiles();
        }
    });

    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, [this] {
        QMessageBox::about(this, tr("About AULA F75"),
            tr("<h3>AULA F75 for Linux</h3>"
               "<p>Native configurator for the AULA F75 keyboard, speaking the "
               "vendor HID protocol directly — no Wine, no vendor driver.</p>"
               "<p>Protocol reverse engineered from OemDrv.exe.  Lighting (solid "
               "and per-key) is verified against hardware; on-board effects, key "
               "matrix and macro handling are experimental.</p>"));
    });

    // Toolbar
    auto* tb = addToolBar(tr("Main"));
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    auto* recon = tb->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), tr("Reconnect"));
    connect(recon, &QAction::triggered, this, [this] {
        m_device.close();
        tryConnect();
    });
    tb->addSeparator();
    auto* applyTb = tb->addAction(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")), tr("Apply"));
    connect(applyTb, &QAction::triggered, this, [this] { sendPerKey(true); });

    auto* spacer = new QWidget(tb);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->addWidget(spacer);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setContentsMargins(0, 0, 12, 0);
    tb->addWidget(m_statusLabel);

    // Central: nav + stack
    auto* central = new QWidget(this);
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_nav = new QListWidget(central);
    m_nav->setFixedWidth(200);
    m_nav->setFrameShape(QFrame::NoFrame);
    m_nav->setIconSize(QSize(22, 22));
    m_nav->setStyleSheet(QStringLiteral(
        "QListWidget { background: palette(alternate-base); padding: 6px; }"
        "QListWidget::item { padding: 9px 10px; border-radius: 6px; }"
        "QListWidget::item:selected { background: palette(highlight); color: palette(highlighted-text); }"));
    layout->addWidget(m_nav);

    m_stack = new QStackedWidget(central);
    layout->addWidget(m_stack, 1);
    setCentralWidget(central);

    auto addPage = [&](const QString& title, const QString& icon, QWidget* page) {
        auto* item = new QListWidgetItem(QIcon::fromTheme(icon), title);
        m_nav->addItem(item);
        m_stack->addWidget(page);
    };

    m_pageDevice = new DevicePage(m_stack);
    m_pageLighting = new LightingPage(this, m_stack);
    m_pageRemap = new RemapPage(this, m_stack);
    m_pageMacros = new MacrosPage(this, m_stack);
    m_pageSettings = new SettingsPage(this, m_stack);

    addPage(tr("Device"), QStringLiteral("computer"), m_pageDevice);
    addPage(tr("Lighting"), QStringLiteral("video-display"), m_pageLighting);
    addPage(tr("Key Mapping"), QStringLiteral("preferences-desktop-keyboard"), m_pageRemap);
    addPage(tr("Macros"), QStringLiteral("document-edit"), m_pageMacros);
    addPage(tr("Settings"), QStringLiteral("configure"), m_pageSettings);

    connect(m_nav, &QListWidget::currentRowChanged, this, &MainWindow::onNavChanged);
    m_nav->setCurrentRow(1);

    // System tray: closing the window keeps the app (and the direct-mode
    // keepalive) running so the keyboard stays exactly as configured instead
    // of reverting to its on-board effect.  Use File > Quit to really exit.
    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        m_tray = new QSystemTrayIcon(QIcon::fromTheme(QStringLiteral("input-keyboard")), this);
        m_tray->setToolTip(tr("AULA F75"));
        auto* menu = new QMenu(this);
        menu->addAction(QIcon::fromTheme(QStringLiteral("view-restore")), tr("Show window"),
                        this, [this] { show(); raise(); activateWindow(); });
        menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), tr("Quit"),
                        this, [this] { m_forceQuit = true; qApp->quit(); });
        m_tray->setContextMenu(menu);
        connect(m_tray, &QSystemTrayIcon::activated, this,
                [this](QSystemTrayIcon::ActivationReason r) {
                    if (r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick)
                    {
                        show(); raise(); activateWindow();
                    }
                });
        m_tray->show();
    }

    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_tray && !m_forceQuit)
    {
        hide();
        m_tray->showMessage(tr("AULA F75"),
                            tr("Still running in the tray so the keyboard stays as "
                               "configured.  Use File > Quit to exit."),
                            QSystemTrayIcon::Information, 3000);
        event->ignore();
        return;
    }
    event->accept();
}

void MainWindow::onNavChanged(int row)
{
    m_stack->setCurrentIndex(row);
}

void MainWindow::tryConnect()
{
    if (m_device.isOpen())
    {
        return;
    }
    const QString path = DeviceManager::findF75();
    if (path.isEmpty())
    {
        m_summary = tr("Searching for keyboard…");
        m_statusLabel->setText(m_summary);
        return;
    }
    if (!m_device.open(path))
    {
        m_summary = tr("Found %1 but cannot open it (permissions?)").arg(path);
        m_statusLabel->setText(m_summary);
        return;
    }

    m_model = m_device.modelId();
    QString modelName;
    switch (m_model)
    {
    case ModelF75: modelName = QStringLiteral("F75"); break;
    case ModelF99: modelName = QStringLiteral("F99"); break;
    default:       modelName = QStringLiteral("AULA F75"); break;
    }

    uint16_t vid = 0, pid = 0;
    for (const DeviceInfo& d : DeviceManager::enumerate())
    {
        if (d.path == path)
        {
            vid = d.vid;
            pid = d.pid;
            break;
        }
    }

    m_pageDevice->setConnection(true, path);
    m_pageDevice->setModel(modelName, m_model);
    m_pageDevice->setId(vid, pid);
    const int st = m_device.status();
    m_pageDevice->setFirmware(m_model >= 0 ? tr("reported (model query OK)") : tr("unknown"));
    m_pageDevice->setStatus(st >= 0 ? tr("online (0x%1)").arg(st, 2, 16, QLatin1Char('0')) : tr("asleep or no reply"));

    m_summary = m_model >= 0
        ? tr("%1 connected").arg(modelName)
        : tr("Connected (board asleep)");
    m_statusLabel->setText(m_summary);
    emit connectionChanged();
    updateStatus();
}

void MainWindow::updateStatus()
{
    if (!m_device.isOpen())
    {
        m_statusLabel->setText(tr("Not connected"));
    }
    else
    {
        m_statusLabel->setText(m_summary);
    }
}

QColor MainWindow::toDeviceColor(const QColor& c, int brightnessPercent)
{
    const double f = qBound(0, brightnessPercent, 100) / 100.0;
    return QColor(int(c.red() * f), int(c.green() * f), int(c.blue() * f));
}

QList<ColorGroup> MainWindow::currentGroups() const
{
    QMap<QRgb, QByteArray> byColor;
    for (int led = 0; led < m_profile.colors.size(); ++led)
    {
        if (led == 84)
        {
            continue; // knob has no LED
        }
        const QColor c = m_profile.colors.at(led);
        if (c.alpha() == 0)
        {
            continue;
        }
        const QColor d = toDeviceColor(c, m_profile.keyBrightness);
        byColor[d.rgb()].append(static_cast<char>(led));
    }

    QList<ColorGroup> groups;
    if (byColor.isEmpty())
    {
        // Nothing lit: explicitly turn every key off.
        ColorGroup off;
        off.r = off.g = off.b = 0;
        for (const KeyDef& k : KeyMap::keys())
        {
            if (k.led != 84)
            {
                off.indices.append(static_cast<char>(k.led));
            }
        }
        groups.append(off);
        return groups;
    }

    for (auto it = byColor.constBegin(); it != byColor.constEnd(); ++it)
    {
        const QColor c = QColor::fromRgb(it.key());
        ColorGroup g;
        g.r = static_cast<uint8_t>(c.red());
        g.g = static_cast<uint8_t>(c.green());
        g.b = static_cast<uint8_t>(c.blue());
        g.indices = it.value();
        groups.append(g);
    }
    return groups;
}

void MainWindow::sendPerKey(bool force)
{
    Q_UNUSED(force);
    if (!m_device.isOpen())
    {
        statusBar()->showMessage(tr("Not connected"), 2000);
        return;
    }
    m_lastGroups = currentGroups();
    const QList<QByteArray> reports = buildChunks(OpSetRealData, LightDirect, directPayload(m_lastGroups));
    if (m_device.writeReports(reports))
    {
        m_lastFrameValid = true;
    }
}

void MainWindow::sendSolid(const QColor& c)
{
    if (!m_device.isOpen())
    {
        statusBar()->showMessage(tr("Not connected"), 2000);
        return;
    }
    const QColor d = toDeviceColor(c, m_profile.keyBrightness);
    const QList<QByteArray> reports = buildChunks(OpSetRealData, LightSolid,
                                                  solidPayload(d.red(), d.green(), d.blue()));
    m_lastFrameValid = m_device.writeReports(reports);
}

void MainWindow::sendEffect()
{
    if (!m_device.isOpen())
    {
        statusBar()->showMessage(tr("Not connected"), 2000);
        return;
    }

    // Effects are an on-board profile write (opcode 0x04).  The captured base
    // block differs between effects only in byte 10, brightness in byte 58 and
    // speed in byte 63.
    static const uint8_t hwEffect[16] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x0A,
        0x0B, 0x0C, 0x0D, 0x0F, 0x10, 0x11, 0x15, 0x00
    };
    const int idx = qBound(0, m_profile.effectId - 1, 15);
    const uint8_t brightness = static_cast<uint8_t>(qBound(0, int(m_profile.brightness), 9));
    const uint8_t speed = static_cast<uint8_t>(qBound(0, int(m_profile.speed), 4));

    QByteArray prof = baseLedProfile();
    if (prof.size() == kLedProfileSize)
    {
        prof[10] = static_cast<char>(hwEffect[idx]);
        prof[58] = static_cast<char>(brightness);
        prof[63] = static_cast<char>((speed << 4) | 0x07);
    }

    // Stop the direct-mode keepalive so the on-board effect is visible.
    m_lastFrameValid = false;

    if (m_device.writeLedProfile(prof))
    {
        statusBar()->showMessage(tr("Effect applied."), 4000);
    }
    else
    {
        statusBar()->showMessage(tr("Effect command failed: %1").arg(m_device.lastError()), 5000);
    }
}

void MainWindow::setKeepaliveActive(bool on)
{
    m_keepalive = on;
}

void MainWindow::setKeepaliveInterval(int ms)
{
    m_keepaliveTimer.setInterval(qBound(20, ms, 500));
}

void MainWindow::keepaliveTick()
{
    if (!m_keepalive || !m_device.isOpen() || !m_lastFrameValid)
    {
        return;
    }
    // Re-send the last direct frame so the board stays in direct mode.  Sent
    // without the inter-chunk delay to keep the stream tight.
    for (const QByteArray& r : buildChunks(OpSetRealData, LightDirect, directPayload(m_lastGroups)))
    {
        m_device.writeReports({ r }, 30);
    }
}

void MainWindow::onActionReset()
{
}

} // namespace f75
