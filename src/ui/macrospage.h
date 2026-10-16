#pragma once

#include <QWidget>

class QListWidget;

namespace f75 {

class MainWindow;

// On-board macro editor.
//
// The driver's macro upload path (opcode 0x03, 0x200-byte records) is not yet
// fully reverse engineered, so this page currently manages macro definitions
// stored on the host and documents the remaining work.  It is included so the
// feature is visible and clearly scoped.
class MacrosPage : public QWidget
{
    Q_OBJECT
public:
    explicit MacrosPage(MainWindow* mw, QWidget* parent = nullptr);

private slots:
    void addMacro();

private:
    MainWindow*  m_mw = nullptr;
    QListWidget* m_list = nullptr;
};

} // namespace f75
