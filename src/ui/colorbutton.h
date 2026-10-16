#pragma once

#include <QColor>
#include <QPushButton>

namespace f75 {

// A push button that shows a colour swatch and opens a colour dialog.
class ColorButton : public QPushButton
{
    Q_OBJECT
public:
    explicit ColorButton(const QColor& color = QColor(255, 0, 0), QWidget* parent = nullptr);

    QColor color() const { return m_color; }
    void setColor(const QColor& c);

signals:
    void colorChanged(const QColor& c);

private slots:
    void pick();

private:
    void refresh();
    QColor m_color;
};

} // namespace f75
