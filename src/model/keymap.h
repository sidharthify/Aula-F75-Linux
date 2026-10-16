#pragma once

#include <QRect>
#include <QString>
#include <QVector>
#include <cstdint>

namespace f75 {

struct KeyDef
{
    QString name;
    int     x1, y1, x2, y2; // layout rectangle in driver units
    uint8_t type;           // 0x02 keyboard, 0x09 media/wheel
    uint8_t code;           // HID usage
    int     led;            // LED index (value = column*6 + row), 84 = wheel
};

class KeyMap
{
public:
    static const QVector<KeyDef>& keys();
    static QRect bounds();

    static int     indexForLed(int led);
    static QString nameForLed(int led);
    static int     ledCount(); // number of addressable positions (0..90)

    // A physical key that can be assigned to a remapped key.
    struct Action
    {
        QString name;
        uint8_t type;
        uint8_t code;
    };
    static const QVector<Action>& assignableActions();
    static QString actionName(uint8_t type, uint8_t code);
};

} // namespace f75
