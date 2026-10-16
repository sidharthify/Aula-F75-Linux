#pragma once

#include "device/f75device.h"
#include "model/profile.h"

#include <QMainWindow>
#include <QTimer>

class QListWidget;
class QStackedWidget;
class QLabel;

namespace f75 {
class DevicePage;
class LightingPage;
class RemapPage;
class MacrosPage;
class SettingsPage;
}

namespace f75 {

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    Profile& profile() { return m_profile; }
    Device&  device() { return m_device; }
    bool     isConnected() const { return m_device.isOpen(); }
    QString  deviceSummary() const { return m_summary; }

    // Lighting senders (called by the pages).
    void sendPerKey(bool force = false);
    void sendSolid(const QColor& c);
    void sendEffect();

    void setKeepaliveActive(bool on);
    void setKeepaliveInterval(int ms);

    static QColor toDeviceColor(const QColor& c, int brightnessPercent);

signals:
    void connectionChanged();

private slots:
    void onNavChanged(int row);
    void tryConnect();
    void keepaliveTick();
    void onActionReset();

private:
    void buildUi();
    void updateStatus();
    QList<f75::ColorGroup> currentGroups() const;

    Device  m_device;
    Profile m_profile;

    QString m_summary = tr("Searching for keyboard…");
    int     m_model = -1;

    QTimer  m_connectTimer;
    QTimer  m_keepaliveTimer;
    bool    m_keepalive = true;
    bool    m_lastFrameValid = false;
    QList<f75::ColorGroup> m_lastGroups;

    QListWidget*    m_nav = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLabel*         m_statusLabel = nullptr;

    DevicePage*     m_pageDevice = nullptr;
    LightingPage*   m_pageLighting = nullptr;
    RemapPage*      m_pageRemap = nullptr;
    MacrosPage*     m_pageMacros = nullptr;
    SettingsPage*   m_pageSettings = nullptr;
};

} // namespace f75
