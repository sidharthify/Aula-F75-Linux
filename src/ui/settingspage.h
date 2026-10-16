#pragma once

#include <QWidget>

class QCheckBox;
class QSpinBox;

namespace f75 {

class MainWindow;

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(MainWindow* mw, QWidget* parent = nullptr);

private:
    MainWindow* m_mw = nullptr;
    QCheckBox*  m_keepalive = nullptr;
    QSpinBox*   m_interval = nullptr;
};

} // namespace f75
