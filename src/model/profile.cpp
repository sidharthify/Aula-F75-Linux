#include "profile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace f75 {

QString Profile::directory()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString dir = base + QStringLiteral("/aula-f75/profiles");
    QDir().mkpath(dir);
    return dir;
}

static QString pathFor(const QString& name)
{
    QString safe = name;
    safe.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._ -]")), QStringLiteral("_"));
    return Profile::directory() + QLatin1Char('/') + safe + QStringLiteral(".json");
}

Profile Profile::load(const QString& name)
{
    Profile p;
    p.name = name;
    QFile f(pathFor(name));
    if (!f.open(QIODevice::ReadOnly))
    {
        return p;
    }
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();

    p.mode       = static_cast<Mode>(o.value(QStringLiteral("mode")).toInt(int(Mode::PerKey)));
    p.effectId   = o.value(QStringLiteral("effectId")).toInt(1);
    p.brightness = static_cast<uint8_t>(o.value(QStringLiteral("brightness")).toInt(4));
    p.speed      = static_cast<uint8_t>(o.value(QStringLiteral("speed")).toInt(2));
    p.direction  = static_cast<uint8_t>(o.value(QStringLiteral("direction")).toInt(0));
    p.colorful   = o.value(QStringLiteral("colorful")).toBool(true);
    p.effectColor = QColor(o.value(QStringLiteral("effectColor")).toString(QStringLiteral("#ff0000")));
    p.keyBrightness = o.value(QStringLiteral("keyBrightness")).toInt(100);

    const QJsonArray arr = o.value(QStringLiteral("colors")).toArray();
    for (int i = 0; i < p.colors.size() && i < arr.size(); ++i)
    {
        p.colors[i] = QColor(arr.at(i).toString());
    }
    return p;
}

bool Profile::save() const
{
    QJsonObject o;
    o.insert(QStringLiteral("mode"), int(mode));
    o.insert(QStringLiteral("effectId"), effectId);
    o.insert(QStringLiteral("brightness"), int(brightness));
    o.insert(QStringLiteral("speed"), int(speed));
    o.insert(QStringLiteral("direction"), int(direction));
    o.insert(QStringLiteral("colorful"), colorful);
    o.insert(QStringLiteral("effectColor"), effectColor.name(QColor::HexRgb));
    o.insert(QStringLiteral("keyBrightness"), keyBrightness);

    QJsonArray arr;
    for (const QColor& c : colors)
    {
        arr.append(c.alpha() == 0 ? QStringLiteral("#00000000") : c.name(QColor::HexArgb));
    }
    o.insert(QStringLiteral("colors"), arr);

    QFile f(pathFor(name));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return true;
}

QStringList Profile::list()
{
    QStringList out;
    const QDir dir(directory());
    for (const QFileInfo& fi : dir.entryInfoList(QStringList() << QStringLiteral("*.json"),
                                                 QDir::Files, QDir::Name))
    {
        out.append(fi.completeBaseName());
    }
    return out;
}

bool Profile::remove(const QString& name)
{
    return QFile::remove(pathFor(name));
}

} // namespace f75
