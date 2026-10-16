#include "colorbutton.h"

#include <QColorDialog>

namespace f75 {

ColorButton::ColorButton(const QColor& color, QWidget* parent)
    : QPushButton(parent)
    , m_color(color)
{
    setMinimumWidth(96);
    connect(this, &QPushButton::clicked, this, &ColorButton::pick);
    refresh();
}

void ColorButton::setColor(const QColor& c)
{
    m_color = c;
    refresh();
    emit colorChanged(m_color);
}

void ColorButton::pick()
{
    const QColor c = QColorDialog::getColor(m_color, this, tr("Select colour"));
    if (c.isValid())
    {
        setColor(c);
    }
}

void ColorButton::refresh()
{
    const QString hex = m_color.name(QColor::HexRgb);
    const bool dark = m_color.lightnessF() < 0.5;
    setText(hex.toUpper());
    setStyleSheet(QStringLiteral(
                      "QPushButton { background-color: %1; color: %2; border: 1px solid palette(mid); "
                      "border-radius: 4px; padding: 6px 12px; font-weight: bold; }")
                      .arg(hex, dark ? QStringLiteral("#ffffff") : QStringLiteral("#000000")));
}

} // namespace f75
