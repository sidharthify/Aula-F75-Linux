#pragma once

#include <QColor>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QSlider;
class QTabWidget;

namespace f75 {

class MainWindow;
class KeyboardView;
class ColorButton;

// Lighting control: solid colour, per-key editor and on-board effects.
class LightingPage : public QWidget
{
    Q_OBJECT
public:
    explicit LightingPage(MainWindow* mw, QWidget* parent = nullptr);

    void refreshFromProfile();

private slots:
    void onSolidChanged();
    void onPerKeyPainted(int led, const QColor& c);
    void applySolid();
    void applyPerKey();
    void applyEffect();
    void presetAll();
    void presetOff();
    void presetWasd();
    void presetArrows();
    void presetRows();
    void presetRandom();
    void syncView();

private:
    MainWindow*   m_mw = nullptr;
    QTabWidget*   m_tabs = nullptr;

    ColorButton*  m_solidColor = nullptr;

    KeyboardView* m_view = nullptr;
    ColorButton*  m_paintColor = nullptr;
    QSlider*      m_brightness = nullptr;
    QCheckBox*    m_live = nullptr;

    QComboBox*    m_effect = nullptr;
    ColorButton*  m_effectColor = nullptr;
    QSlider*      m_effectBrightness = nullptr;
    QSlider*      m_effectSpeed = nullptr;
    QComboBox*    m_direction = nullptr;
    QCheckBox*    m_colorful = nullptr;
};

} // namespace f75
