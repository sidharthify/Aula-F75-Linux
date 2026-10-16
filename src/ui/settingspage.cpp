#include "settingspage.h"

#include "mainwindow.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace f75 {

SettingsPage::SettingsPage(MainWindow* mw, QWidget* parent)
    : QWidget(parent)
    , m_mw(mw)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(tr("Settings"));
    QFont f = title->font();
    f.setPointSizeF(f.pointSizeF() * 1.4);
    f.setBold(true);
    title->setFont(f);
    root->addWidget(title);

    auto* directBox = new QGroupBox(tr("Direct-mode keepalive"), this);
    auto* dl = new QFormLayout(directBox);
    dl->setHorizontalSpacing(24);

    m_keepalive = new QCheckBox(tr("Keep the board in direct mode"), directBox);
    m_keepalive->setChecked(true);
    m_keepalive->setToolTip(tr("The F75 leaves direct colour mode when it stops "
                               "receiving packets, so the current frame is re-sent "
                               "continuously while enabled."));
    dl->addRow(QString(), m_keepalive);

    m_interval = new QSpinBox(directBox);
    m_interval->setRange(20, 500);
    m_interval->setSingleStep(10);
    m_interval->setValue(50);
    m_interval->setSuffix(tr(" ms"));
    dl->addRow(tr("Re-send interval:"), m_interval);

    connect(m_keepalive, &QCheckBox::toggled, this, [this](bool on) {
        m_mw->setKeepaliveActive(on);
    });
    connect(m_interval, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
        m_mw->setKeepaliveInterval(v);
    });

    root->addWidget(directBox);

    auto* deviceBox = new QGroupBox(tr("Device"), this);
    auto* vl = new QVBoxLayout(deviceBox);

    auto* resetBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-undo")),
                                     tr("Reset keyboard to factory settings"), deviceBox);
    resetBtn->setToolTip(tr("Sends the driver's reset opcode (0x06)."));
    connect(resetBtn, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, tr("Reset keyboard"),
                                  tr("Reset the keyboard to its factory settings?"))
            == QMessageBox::Yes)
        {
            if (m_mw->isConnected() && m_mw->device().resetDevice())
            {
                QMessageBox::information(this, tr("Reset keyboard"), tr("Reset command sent."));
            }
            else
            {
                QMessageBox::warning(this, tr("Reset keyboard"), tr("Not connected."));
            }
        }
    });
    vl->addWidget(resetBtn);

    auto* info = new QLabel(tr(
        "Permissions are granted by the udev rule installed with the package "
        "(hidraw for 3554:fa09 and 258a:010c, group <tt>input</tt>).  Add your "
        "user to the <tt>input</tt> group if the device cannot be opened."));
    info->setWordWrap(true);
    info->setEnabled(false);
    vl->addWidget(info);

    root->addWidget(deviceBox);
    root->addStretch(1);
}

} // namespace f75
