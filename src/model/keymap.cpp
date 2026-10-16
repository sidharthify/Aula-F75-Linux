#include "keymap.h"

#include <QHash>

namespace f75 {

// LED numbering on the F75 is column-major with six rows per column, so the
// value for a key is (column * 6) + row.  Esc is 0, F12 is 78, the arrow
// cluster is 77/82/83/89, the nav column is 85..88 and the knob (84) has no
// addressable LED.  Extracted from the driver's Dev/kb/F75KR/KB.ini.
static const QVector<KeyDef> kKeys = {
    {QStringLiteral("Esc"), 30, 30, 55, 57, 0x02, 0x1B, 0},
    {QStringLiteral("F1"), 107, 30, 132, 57, 0x02, 0x70, 12},
    {QStringLiteral("F2"), 144, 30, 169, 57, 0x02, 0x71, 18},
    {QStringLiteral("F3"), 183, 30, 208, 57, 0x02, 0x72, 24},
    {QStringLiteral("F4"), 222, 30, 247, 57, 0x02, 0x73, 30},
    {QStringLiteral("F5"), 269, 30, 294, 57, 0x02, 0x74, 36},
    {QStringLiteral("F6"), 305, 30, 330, 57, 0x02, 0x75, 42},
    {QStringLiteral("F7"), 343, 30, 368, 57, 0x02, 0x76, 48},
    {QStringLiteral("F8"), 388, 30, 413, 57, 0x02, 0x77, 54},
    {QStringLiteral("F9"), 439, 30, 464, 57, 0x02, 0x78, 60},
    {QStringLiteral("F10"), 477, 30, 502, 57, 0x02, 0x79, 66},
    {QStringLiteral("F11"), 516, 30, 541, 57, 0x02, 0x7A, 72},
    {QStringLiteral("F12"), 555, 30, 580, 57, 0x02, 0x7B, 78},
    {QStringLiteral("End"), 605, 195, 630, 222, 0x02, 0x23, 88},
    {QStringLiteral("`"), 31, 81, 56, 108, 0x02, 0xC0, 1},
    {QStringLiteral("1"), 70, 81, 95, 108, 0x02, 0x31, 7},
    {QStringLiteral("2"), 108, 81, 133, 108, 0x02, 0x32, 13},
    {QStringLiteral("3"), 147, 81, 172, 108, 0x02, 0x33, 19},
    {QStringLiteral("4"), 184, 81, 209, 108, 0x02, 0x34, 25},
    {QStringLiteral("5"), 223, 81, 248, 108, 0x02, 0x35, 31},
    {QStringLiteral("6"), 262, 81, 287, 108, 0x02, 0x36, 37},
    {QStringLiteral("7"), 300, 80, 325, 107, 0x02, 0x37, 43},
    {QStringLiteral("8"), 338, 81, 363, 108, 0x02, 0x38, 49},
    {QStringLiteral("9"), 377, 81, 402, 108, 0x02, 0x39, 55},
    {QStringLiteral("0"), 416, 81, 441, 108, 0x02, 0x30, 61},
    {QStringLiteral("-"), 453, 81, 478, 108, 0x02, 0xBD, 67},
    {QStringLiteral("="), 491, 81, 516, 108, 0x02, 0xBB, 73},
    {QStringLiteral("Backspace"), 531, 81, 591, 108, 0x02, 0x08, 79},
    {QStringLiteral("Delete"), 604, 81, 629, 108, 0x02, 0x2E, 85},
    {QStringLiteral("Tab"), 34, 120, 78, 147, 0x02, 0x09, 2},
    {QStringLiteral("Q"), 89, 119, 114, 146, 0x02, 0x51, 8},
    {QStringLiteral("W"), 127, 119, 152, 146, 0x02, 0x57, 14},
    {QStringLiteral("E"), 165, 119, 190, 146, 0x02, 0x45, 20},
    {QStringLiteral("R"), 204, 119, 229, 146, 0x02, 0x52, 26},
    {QStringLiteral("T"), 242, 119, 267, 146, 0x02, 0x54, 32},
    {QStringLiteral("Y"), 280, 119, 305, 146, 0x02, 0x59, 38},
    {QStringLiteral("U"), 318, 119, 343, 146, 0x02, 0x55, 44},
    {QStringLiteral("I"), 356, 119, 381, 146, 0x02, 0x49, 50},
    {QStringLiteral("O"), 395, 119, 420, 146, 0x02, 0x4F, 56},
    {QStringLiteral("P"), 433, 119, 458, 146, 0x02, 0x50, 62},
    {QStringLiteral("["), 471, 120, 496, 147, 0x02, 0xDB, 68},
    {QStringLiteral("]"), 509, 119, 534, 146, 0x02, 0xDD, 74},
    {QStringLiteral("\\"), 549, 119, 593, 146, 0x02, 0xDC, 80},
    {QStringLiteral("PgUp"), 605, 120, 630, 147, 0x02, 0x21, 86},
    {QStringLiteral("CapsLock"), 33, 158, 85, 185, 0x02, 0x14, 3},
    {QStringLiteral("A"), 98, 158, 123, 185, 0x02, 0x41, 9},
    {QStringLiteral("S"), 136, 158, 161, 185, 0x02, 0x53, 15},
    {QStringLiteral("D"), 174, 158, 199, 185, 0x02, 0x44, 21},
    {QStringLiteral("F"), 213, 158, 238, 185, 0x02, 0x46, 27},
    {QStringLiteral("G"), 252, 158, 277, 185, 0x02, 0x47, 33},
    {QStringLiteral("H"), 290, 158, 315, 185, 0x02, 0x48, 39},
    {QStringLiteral("J"), 327, 158, 352, 185, 0x02, 0x4A, 45},
    {QStringLiteral("K"), 366, 158, 391, 185, 0x02, 0x4B, 51},
    {QStringLiteral("L"), 405, 158, 430, 185, 0x02, 0x4C, 57},
    {QStringLiteral(";"), 443, 158, 468, 185, 0x02, 0xBA, 63},
    {QStringLiteral("'"), 481, 158, 506, 185, 0x02, 0xDE, 69},
    {QStringLiteral("Enter"), 521, 158, 591, 185, 0x02, 0x0D, 81},
    {QStringLiteral("LShift"), 33, 195, 105, 222, 0x02, 0xA0, 4},
    {QStringLiteral("Z"), 116, 195, 141, 222, 0x02, 0x5A, 10},
    {QStringLiteral("X"), 155, 195, 180, 222, 0x02, 0x58, 16},
    {QStringLiteral("C"), 193, 195, 218, 222, 0x02, 0x43, 22},
    {QStringLiteral("V"), 232, 195, 257, 222, 0x02, 0x56, 28},
    {QStringLiteral("B"), 270, 196, 295, 223, 0x02, 0x42, 34},
    {QStringLiteral("N"), 309, 195, 334, 222, 0x02, 0x4E, 40},
    {QStringLiteral("M"), 347, 195, 372, 222, 0x02, 0x4D, 46},
    {QStringLiteral(","), 384, 195, 409, 222, 0x02, 0xBC, 52},
    {QStringLiteral("."), 424, 195, 449, 222, 0x02, 0xBE, 58},
    {QStringLiteral("/"), 463, 195, 488, 222, 0x02, 0xBF, 64},
    {QStringLiteral("RShift"), 502, 195, 558, 222, 0x02, 0xA1, 70},
    {QStringLiteral("Up"), 567, 195, 592, 222, 0x02, 0x26, 82},
    {QStringLiteral("LCtrl"), 34, 234, 69, 261, 0x02, 0xA2, 5},
    {QStringLiteral("LWin"), 79, 234, 114, 261, 0x02, 0x5B, 11},
    {QStringLiteral("LAlt"), 122, 235, 157, 262, 0x02, 0xA4, 17},
    {QStringLiteral("Space"), 174, 235, 400, 262, 0x02, 0x20, 35},
    {QStringLiteral("RCtrl"), 463, 236, 497, 263, 0x02, 0xC4, 59},
    {QStringLiteral("FN"), 414, 235, 449, 262, 0x02, 0xFA, 53},
    {QStringLiteral("Left"), 529, 234, 554, 261, 0x02, 0x25, 77},
    {QStringLiteral("Down"), 568, 234, 593, 261, 0x02, 0x28, 83},
    {QStringLiteral("Right"), 605, 234, 630, 261, 0x02, 0x27, 89},
    {QStringLiteral("PgDn"), 604, 159, 629, 186, 0x02, 0x22, 87},
    {QStringLiteral("Knob"), 0, 0, 0, 0, 0x09, 0x01, 84},
};

const QVector<KeyDef>& KeyMap::keys()
{
    return kKeys;
}

QRect KeyMap::bounds()
{
    int x1 = 1 << 30, y1 = 1 << 30, x2 = 0, y2 = 0;
    for (const KeyDef& k : kKeys)
    {
        if (k.led == 84)
        {
            continue; // knob has no rectangle
        }
        x1 = qMin(x1, k.x1);
        y1 = qMin(y1, k.y1);
        x2 = qMax(x2, k.x2);
        y2 = qMax(y2, k.y2);
    }
    return QRect(x1, y1, x2 - x1, y2 - y1);
}

int KeyMap::indexForLed(int led)
{
    for (int i = 0; i < kKeys.size(); ++i)
    {
        if (kKeys[i].led == led)
        {
            return i;
        }
    }
    return -1;
}

QString KeyMap::nameForLed(int led)
{
    const int i = indexForLed(led);
    return i >= 0 ? kKeys[i].name : QString();
}

int KeyMap::ledCount()
{
    return 90;
}

// A compact USB HID keyboard-usage table for the remap "assign" picker.
static const QVector<KeyMap::Action>& buildActions()
{
    static QVector<KeyMap::Action> acts;
    if (!acts.isEmpty())
    {
        return acts;
    }

    struct Row { uint8_t usage; const char* name; };
    static const Row rows[] = {
        {0x04, "A"}, {0x05, "B"}, {0x06, "C"}, {0x07, "D"}, {0x08, "E"},
        {0x09, "F"}, {0x0A, "G"}, {0x0B, "H"}, {0x0C, "I"}, {0x0D, "J"},
        {0x0E, "K"}, {0x0F, "L"}, {0x10, "M"}, {0x11, "N"}, {0x12, "O"},
        {0x13, "P"}, {0x14, "Q"}, {0x15, "R"}, {0x16, "S"}, {0x17, "T"},
        {0x18, "U"}, {0x19, "V"}, {0x1A, "W"}, {0x1B, "X"}, {0x1C, "Y"},
        {0x1D, "Z"},
        {0x1E, "1"}, {0x1F, "2"}, {0x20, "3"}, {0x21, "4"}, {0x22, "5"},
        {0x23, "6"}, {0x24, "7"}, {0x25, "8"}, {0x26, "9"}, {0x27, "0"},
        {0x28, "Enter"}, {0x29, "Esc"}, {0x2A, "Backspace"}, {0x2B, "Tab"},
        {0x2C, "Space"}, {0x2D, "-"}, {0x2E, "="}, {0x2F, "["}, {0x30, "]"},
        {0x31, "\\"}, {0x33, ";"}, {0x34, "'"}, {0x35, "`"}, {0x36, ","},
        {0x37, "."}, {0x38, "/"}, {0x39, "CapsLock"},
        {0x3A, "F1"}, {0x3B, "F2"}, {0x3C, "F3"}, {0x3D, "F4"}, {0x3E, "F5"},
        {0x3F, "F6"}, {0x40, "F7"}, {0x41, "F8"}, {0x42, "F9"}, {0x43, "F10"},
        {0x44, "F11"}, {0x45, "F12"},
        {0x46, "PrintScreen"}, {0x48, "Pause"}, {0x49, "Insert"}, {0x4A, "Home"},
        {0x4B, "PgUp"}, {0x4C, "Delete"}, {0x4D, "End"}, {0x4E, "PgDn"},
        {0x4F, "Right"}, {0x50, "Left"}, {0x51, "Down"}, {0x52, "Up"},
        {0xE0, "LCtrl"}, {0xE1, "LShift"}, {0xE2, "LAlt"}, {0xE3, "LWin"},
        {0xE4, "RCtrl"}, {0xE5, "RShift"}, {0xE6, "RAlt"}, {0xE7, "RWin"},
    };
    for (const Row& r : rows)
    {
        acts.append({ QString::fromLatin1(r.name), 0x02, r.usage });
    }
    return acts;
}

const QVector<KeyMap::Action>& KeyMap::assignableActions()
{
    return buildActions();
}

QString KeyMap::actionName(uint8_t type, uint8_t code)
{
    for (const Action& a : assignableActions())
    {
        if (a.type == type && a.code == code)
        {
            return a.name;
        }
    }
    if (type == 0x02)
    {
        return QStringLiteral("HID 0x%1").arg(code, 2, 16, QLatin1Char('0')).toUpper();
    }
    return QStringLiteral("0x%1:0x%2").arg(type, 2, 16, QLatin1Char('0'))
                                       .arg(code, 2, 16, QLatin1Char('0')).toUpper();
}

} // namespace f75
