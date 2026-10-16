#include "macrospage.h"

#include "mainwindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace f75 {

MacrosPage::MacrosPage(MainWindow* mw, QWidget* parent)
    : QWidget(parent)
    , m_mw(mw)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(14);

    auto* title = new QLabel(tr("Macros"));
    QFont f = title->font();
    f.setPointSizeF(f.pointSizeF() * 1.4);
    f.setBold(true);
    title->setFont(f);
    root->addWidget(title);

    auto* note = new QLabel(tr(
        "On-board macro upload is <b>not implemented yet</b>.  The Windows "
        "driver writes macros as 0x200-byte records (opcode 0x03) in a format "
        "that is still being reverse engineered, so this build does not claim "
        "to program them.  Macro definitions you create here are kept locally "
        "and can be applied once the on-board format is completed."));
    note->setWordWrap(true);
    root->addWidget(note);

    m_list = new QListWidget(this);
    root->addWidget(m_list, 1);

    auto* row = new QHBoxLayout();
    auto* add = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), tr("New macro"), this);
    auto* del = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), tr("Delete"), this);
    connect(add, &QPushButton::clicked, this, &MacrosPage::addMacro);
    connect(del, &QPushButton::clicked, this, [this] { delete m_list->takeItem(m_list->currentRow()); });
    row->addWidget(add);
    row->addWidget(del);
    row->addStretch(1);
    root->addLayout(row);
}

void MacrosPage::addMacro()
{
    m_list->addItem(tr("Macro %1").arg(m_list->count() + 1));
}

} // namespace f75
