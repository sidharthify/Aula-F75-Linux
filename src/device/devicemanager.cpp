#include "devicemanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace f75 {

bool DeviceManager::isKnownId(uint16_t vid, uint16_t pid)
{
    return (vid == 0x3554 && pid == 0xFA09) || // Compx 2.4G wireless receiver
           (vid == 0x258A && pid == 0x010C);   // Sinowealth wired
}

bool DeviceManager::hasVendorCollection(const QString& hidrawPath)
{
    const QString descPath = QStringLiteral("/sys/class/hidraw/%1/device/report_descriptor")
                                 .arg(QFileInfo(hidrawPath).fileName());
    QFile f(descPath);
    if (!f.open(QIODevice::ReadOnly))
    {
        return false;
    }
    const QByteArray d = f.read(4096);

    // Look for: Usage Page 0xFF02 (06 02 FF) ... Collection/Report ID 0x13 with
    // an Output item (0x91).  A simple substring scan is enough in practice.
    const QByteArray usagePage = QByteArray::fromHex("0602ff");
    if (!d.contains(usagePage))
    {
        return false;
    }
    return d.contains(QByteArray::fromHex("8513"));
}

QList<DeviceInfo> DeviceManager::enumerate()
{
    QList<DeviceInfo> out;
    const QDir dir(QStringLiteral("/sys/class/hidraw"));
    const QStringList entries = dir.entryList(QStringList() << QStringLiteral("hidraw*"),
                                              QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    static const QRegularExpression reId(QStringLiteral("HID_ID=0003:([0-9A-Fa-f]{8}):([0-9A-Fa-f]{8})"));
    static const QRegularExpression reName(QStringLiteral("HID_NAME=(.*)"));
    static const QRegularExpression reIf(QStringLiteral(":(\\d+)\\.(\\d+)$"));

    for (const QString& e : entries)
    {
        DeviceInfo info;
        info.path = QStringLiteral("/dev/%1").arg(e);

        QFile uf(QStringLiteral("/sys/class/hidraw/%1/device/uevent").arg(e));
        if (uf.open(QIODevice::ReadOnly))
        {
            const QString u = QString::fromLocal8Bit(uf.readAll());
            auto m = reId.match(u);
            if (m.hasMatch())
            {
                info.vid = static_cast<uint16_t>(m.captured(1).toUInt(nullptr, 16));
                info.pid = static_cast<uint16_t>(m.captured(2).toUInt(nullptr, 16));
            }
            auto mn = reName.match(u);
            if (mn.hasMatch())
            {
                info.name = mn.captured(1).trimmed();
            }
        }

        // Resolve the interface number from the sysfs path.
        const QString real = QFileInfo(QStringLiteral("/sys/class/hidraw/%1/device").arg(e)).symLinkTarget();
        auto mi = reIf.match(real);
        if (mi.hasMatch())
        {
            info.interfaceNumber = mi.captured(2).toInt();
        }

        info.known = isKnownId(info.vid, info.pid);
        info.vendorCollection = info.known && hasVendorCollection(info.path);
        out.append(info);
    }
    return out;
}

QString DeviceManager::findF75()
{
    QList<DeviceInfo> all = enumerate();

    // Prefer a known id whose descriptor exposes the vendor collection.
    for (const DeviceInfo& d : all)
    {
        if (d.known && d.vendorCollection)
        {
            return d.path;
        }
    }
    // Fall back to a known id on interface 1.
    for (const DeviceInfo& d : all)
    {
        if (d.known && d.interfaceNumber == 1)
        {
            return d.path;
        }
    }
    // Last resort: any known id.
    for (const DeviceInfo& d : all)
    {
        if (d.known)
        {
            return d.path;
        }
    }
    return QString();
}

} // namespace f75
