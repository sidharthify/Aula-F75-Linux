#pragma once

#include <QColor>
#include <QHash>
#include <QVector>
#include <QWidget>

namespace f75 {

// Paints the physical F75 layout and lets the user click / drag keys.  Used
// both for the per-key lighting editor and the key-mapping page.
class KeyboardView : public QWidget
{
    Q_OBJECT
public:
    enum Mode { Paint, Select };

    explicit KeyboardView(QWidget* parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

    void setColors(const QVector<QColor>& colors); // index = LED value
    const QVector<QColor>& colors() const { return m_colors; }
    void setColor(int led, const QColor& c);

    void setCurrentColor(const QColor& c) { m_current = c; }
    QColor currentColor() const { return m_current; }

    void setSelectedLed(int led);
    int  selectedLed() const { return m_selected; }

    void setLabels(const QHash<int, QString>& labels); // led -> overlay text
    void setShowLabels(bool on);

signals:
    void keyClicked(int led);
    void keyPainted(int led, const QColor& c);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    QSize minimumSizeHint() const override { return QSize(820, 300); }

private:
    int  ledAt(const QPoint& pos) const;
    QHash<int, QRect> m_rects; // led -> widget rect

    Mode m_mode = Paint;
    QVector<QColor> m_colors;
    QColor m_current = QColor(255, 0, 0);
    int m_selected = -1;
    QHash<int, QString> m_labels;
    bool m_showLabels = false;
    bool m_dragging = false;
};

} // namespace f75
