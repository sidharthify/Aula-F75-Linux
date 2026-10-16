#pragma once

#include <QList>
#include <QString>
#include <cstdint>

namespace f75 {

struct DeviceInfo
{
    QString  path;              // /dev/hidrawN
    QString  name;              // HID_NAME
    uint16_t vid = 0;
    uint16_t pid = 0;
    int      interfaceNumber = -1;
    bool     vendorCollection = false; // Usage Page 0xFF02 / Report ID 0x13
    bool     known = false;            // matches a known AULA F75 id
};

class DeviceManager
{
public:
    static QList<DeviceInfo> enumerate();

    // Best vendor-interface hidraw path for a known AULA F75, or empty.
    static QString findF75();

    static bool isKnownId(uint16_t vid, uint16_t pid);

    // Parse the HID report descriptor for the vendor collection
    // (Usage Page 0xFF02 with an output report of id 0x13).
    static bool hasVendorCollection(const QString& hidrawPath);
};

} // namespace f75
