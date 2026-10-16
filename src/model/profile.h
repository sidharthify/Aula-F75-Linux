#pragma once

#include <QColor>
#include <QString>
#include <QVector>

namespace f75 {

// An application-side lighting profile.  The board itself only holds the live
// frame, so profiles are stored as JSON under
// $XDG_CONFIG_HOME/aula-f75/profiles.
struct Profile
{
    QString name;
    enum Mode { PerKey, Solid, Effect } mode = PerKey;

    // Per-key / solid colour state, indexed by LED value 0..89.  A colour with
    // alpha 0 means "off".
    QVector<QColor> colors = QVector<QColor>(90, QColor(0, 0, 0, 0));

    // Global brightness scaling (percent) applied to per-key / solid colours.
    int keyBrightness = 100;

    // Effect state (reverse engineered; experimental).
    int     effectId   = 1;
    uint8_t brightness = 4;
    uint8_t speed      = 2;
    uint8_t direction  = 0;
    bool    colorful   = true;
    QColor  effectColor = QColor(255, 0, 0);

    static Profile load(const QString& name);
    bool save() const;
    static QStringList list();
    static bool remove(const QString& name);
    static QString directory();
};

} // namespace f75
