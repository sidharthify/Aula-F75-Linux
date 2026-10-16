#include "keyboardview.h"
#include "model/keymap.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

namespace f75 {

KeyboardView::KeyboardView(QWidget* parent)
    : QWidget(parent)
    , m_colors(KeyMap::ledCount(), QColor(0, 0, 0, 0))
{
    setMouseTracking(true);
    setMinimumHeight(300);
}

void KeyboardView::setMode(Mode mode)
{
    m_mode = mode;
    update();
}

void KeyboardView::setColors(const QVector<QColor>& colors)
{
    m_colors = colors;
    while (m_colors.size() < KeyMap::ledCount())
    {
        m_colors.append(QColor(0, 0, 0, 0));
    }
    update();
}

void KeyboardView::setColor(int led, const QColor& c)
{
    if (led < 0 || led >= m_colors.size())
    {
        return;
    }
    m_colors[led] = c;
    update();
}

void KeyboardView::setSelectedLed(int led)
{
    m_selected = led;
    update();
}

void KeyboardView::setLabels(const QHash<int, QString>& labels)
{
    m_labels = labels;
    update();
}

void KeyboardView::setShowLabels(bool on)
{
    m_showLabels = on;
    update();
}

static QRect computeLayout(const QRect& bounds, const QSize& widget, double& scaleOut)
{
    const double sx = double(widget.width() - 24) / bounds.width();
    const double sy = double(widget.height() - 24) / bounds.height();
    const double s = qMin(sx, sy);
    scaleOut = s;
    const int w = int(bounds.width() * s);
    const int h = int(bounds.height() * s);
    const int x = (widget.width() - w) / 2;
    const int y = (widget.height() - h) / 2;
    return QRect(x, y, w, h);
}

void KeyboardView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect bounds = KeyMap::bounds();
    double scale = 1.0;
    const QRect target = computeLayout(bounds, size(), scale);

    m_rects.clear();
    for (const KeyDef& k : KeyMap::keys())
    {
        if (k.led == 84 || k.x2 <= k.x1)
        {
            continue;
        }
        QRect r(target.x() + int((k.x1 - bounds.x()) * scale),
                target.y() + int((k.y1 - bounds.y()) * scale),
                int((k.x2 - k.x1) * scale),
                int((k.y2 - k.y1) * scale));
        r.adjust(1, 1, -1, -1);
        m_rects.insert(k.led, r);
    }

    QFont f = p.font();
    f.setPointSizeF(qMax(6.0, scale * 10.0));
    p.setFont(f);

    for (auto it = m_rects.constBegin(); it != m_rects.constEnd(); ++it)
    {
        const int led = it.key();
        const QRect r = it.value();
        QColor c = (led < m_colors.size()) ? m_colors[led] : QColor(0, 0, 0, 0);

        QColor fill = c.alpha() == 0 ? QColor(48, 48, 48) : c;
        p.setBrush(fill);
        p.setPen(QPen(led == m_selected ? QColor(61, 174, 233) : QColor(24, 24, 24),
                      led == m_selected ? 2.0 : 1.0));
        p.drawRoundedRect(r, 4, 4);

        // Text
        const bool dark = fill.lightnessF() < 0.5;
        p.setPen(dark ? QColor(220, 220, 220) : QColor(20, 20, 20));

        QString label;
        if (m_showLabels && m_labels.contains(led))
        {
            label = m_labels.value(led);
        }
        else if (scale > 0.62)
        {
            label = KeyMap::nameForLed(led);
            if (label.size() > 4)
            {
                label = label.left(4);
            }
        }
        if (!label.isEmpty())
        {
            p.drawText(r, Qt::AlignCenter, label);
        }
    }
}

int KeyboardView::ledAt(const QPoint& pos) const
{
    for (auto it = m_rects.constBegin(); it != m_rects.constEnd(); ++it)
    {
        if (it.value().contains(pos))
        {
            return it.key();
        }
    }
    return -1;
}

void KeyboardView::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton)
    {
        return;
    }
    const int led = ledAt(e->pos());
    if (led < 0)
    {
        return;
    }
    m_dragging = true;
    m_selected = led;
    emit keyClicked(led);
    if (m_mode == Paint)
    {
        setColor(led, m_current);
        emit keyPainted(led, m_current);
    }
    update();
}

void KeyboardView::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_dragging || m_mode != Paint)
    {
        return;
    }
    const int led = ledAt(e->pos());
    if (led < 0)
    {
        return;
    }
    setColor(led, m_current);
    emit keyPainted(led, m_current);
}

void KeyboardView::mouseReleaseEvent(QMouseEvent*)
{
    m_dragging = false;
}

} // namespace f75
