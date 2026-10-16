#include "remappage.h"

#include "keyboardview.h"
#include "mainwindow.h"
#include "model/keymap.h"
#include "model/profile.h"

#include <QDir>
#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>

namespace f75 {

static const int kMatrixBytes = 128 * 4; // MatrixLen (128) * 4 bytes per key

RemapPage::RemapPage(MainWindow* mw, QWidget* parent)
    : QWidget(parent)
    , m_mw(mw)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* note = new QLabel(tr(
        "<b>Read-only in this build.</b>  The key map is stored on the keyboard "
        "as a matrix of 4-byte hardware codes.  Reading and backing it up is "
        "safe; writing it is disabled because the block layout has not been "
        "verified on hardware and a wrong write can require a factory reset."));
    note->setWordWrap(true);
    root->addWidget(note);

    m_view = new KeyboardView(this);
    m_view->setMode(KeyboardView::Select);
    m_view->setColors(QVector<QColor>(KeyMap::ledCount(), QColor(52, 52, 52)));
    root->addWidget(m_view, 1);

    auto* row = new QHBoxLayout();

    auto* readBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")),
                                    tr("Read from keyboard"), this);
    connect(readBtn, &QPushButton::clicked, this, &RemapPage::readMatrix);
    row->addWidget(readBtn);

    m_writeBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("document-save")),
                                 tr("Write to keyboard"), this);
    m_writeBtn->setEnabled(false);
    connect(m_writeBtn, &QPushButton::clicked, this, &RemapPage::writeMatrix);
    row->addWidget(m_writeBtn);

    auto* backupBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("document-save-as")),
                                      tr("Back up…"), this);
    connect(backupBtn, &QPushButton::clicked, this, &RemapPage::backup);
    row->addWidget(backupBtn);

    auto* restoreBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("document-revert")),
                                       tr("Restore backup"), this);
    connect(restoreBtn, &QPushButton::clicked, this, &RemapPage::restore);
    row->addWidget(restoreBtn);

    row->addStretch(1);
    root->addLayout(row);

    auto* swapBox = new QGroupBox(tr("Swap two keys"), this);
    auto* sl = new QHBoxLayout(swapBox);
    m_sourceLabel = new QLabel(tr("Source: —"), swapBox);
    m_targetLabel = new QLabel(tr("Target: —"), swapBox);
    auto* swapBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("object-flip-horizontal")),
                                    tr("Swap"), swapBox);
    auto* resetBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-undo")),
                                     tr("Reset selection"), swapBox);
    connect(swapBtn, &QPushButton::clicked, this, &RemapPage::swapKeys);
    connect(resetBtn, &QPushButton::clicked, this, &RemapPage::resetMatrix);
    sl->addWidget(m_sourceLabel);
    sl->addWidget(m_targetLabel);
    sl->addStretch(1);
    sl->addWidget(swapBtn);
    sl->addWidget(resetBtn);
    root->addWidget(swapBox);

    m_status = new QLabel(tr("Not read yet."), this);
    m_status->setEnabled(false);
    root->addWidget(m_status);

    connect(m_view, &KeyboardView::keyClicked, this, &RemapPage::onKeyClicked);
}

void RemapPage::setStatus(const QString& s)
{
    m_status->setText(s);
}

void RemapPage::updateView()
{
    QVector<QColor> cols(KeyMap::ledCount(), QColor(52, 52, 52));
    if (m_source >= 0 && m_source < cols.size())
    {
        cols[m_source] = QColor(61, 174, 233);
    }
    if (m_target >= 0 && m_target < cols.size())
    {
        cols[m_target] = QColor(246, 116, 0);
    }
    m_view->setColors(cols);
    m_sourceLabel->setText(m_source >= 0
        ? tr("Source: %1").arg(KeyMap::nameForLed(m_source))
        : tr("Source: —"));
    m_targetLabel->setText(m_target >= 0
        ? tr("Target: %1").arg(KeyMap::nameForLed(m_target))
        : tr("Target: —"));
}

void RemapPage::onKeyClicked(int led)
{
    if (m_source < 0)
    {
        m_source = led;
    }
    else if (m_target < 0)
    {
        m_target = led;
    }
    else
    {
        m_source = led;
        m_target = -1;
    }
    updateView();
}

void RemapPage::readMatrix()
{
    if (!m_mw->isConnected())
    {
        setStatus(tr("Not connected."));
        return;
    }
    QByteArray payload;
    if (!m_mw->device().readCommand(OpGetMatrix, 0, payload, kMatrixBytes))
    {
        setStatus(tr("No reply from the keyboard (it may be asleep)."));
        return;
    }
    m_matrix = payload.left(kMatrixBytes);
    if (!m_haveBackup)
    {
        backup();
        m_haveBackup = true;
    }
    m_writeBtn->setEnabled(true);
    setStatus(tr("Read %1 bytes.  Select two keys and press Swap.").arg(m_matrix.size()));
}

void RemapPage::writeMatrix()
{
    // Writing the key matrix has not been verified against hardware and an
    // incorrect block can leave the keyboard needing a factory reset, so it is
    // disabled.  Read/backup remain available for inspection.
    QMessageBox::information(this, tr("Write matrix"),
        tr("Writing the key matrix is disabled in this build for safety.\n\n"
           "The block layout has not been verified on hardware yet, and an "
           "incorrect write can require resetting the keyboard."));
}

static QString backupPath()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString dir = base + QStringLiteral("/aula-f75");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/matrix-layer0.bin");
}

void RemapPage::backup()
{
    if (m_matrix.isEmpty())
    {
        return;
    }
    QFile f(backupPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        f.write(m_matrix);
        m_haveBackup = true;
        setStatus(tr("Backed up to %1").arg(backupPath()));
    }
}

void RemapPage::restore()
{
    QFile f(backupPath());
    if (!f.open(QIODevice::ReadOnly))
    {
        setStatus(tr("No backup found."));
        return;
    }
    m_matrix = f.readAll();
    m_haveBackup = true;
    m_writeBtn->setEnabled(true);
    setStatus(tr("Loaded %1 bytes from backup.").arg(m_matrix.size()));
}

void RemapPage::swapKeys()
{
    if (m_matrix.isEmpty() || m_source < 0 || m_target < 0 || m_source == m_target)
    {
        setStatus(tr("Read the matrix and select two different keys first."));
        return;
    }
    const int a = m_source * 4;
    const int b = m_target * 4;
    if (a + 4 > m_matrix.size() || b + 4 > m_matrix.size())
    {
        setStatus(tr("Key index outside the matrix."));
        return;
    }
    const QByteArray ca = m_matrix.mid(a, 4);
    const QByteArray cb = m_matrix.mid(b, 4);
    m_matrix.replace(a, 4, cb);
    m_matrix.replace(b, 4, ca);
    setStatus(tr("Swapped %1 and %2.  Press Write to apply.")
                  .arg(KeyMap::nameForLed(m_source), KeyMap::nameForLed(m_target)));
}

void RemapPage::resetMatrix()
{
    m_source = -1;
    m_target = -1;
    updateView();
}

} // namespace f75
