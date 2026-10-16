#pragma once

#include "f75protocol.h"

#include <QByteArray>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QString>

namespace f75 {

// A live connection to the AULA F75 vendor HID interface.
//
// Lighting commands (OpSetRealData) are fire-and-forget: the board does not
// answer them, and it drops out of "direct" mode if it stops being talked to,
// so callers should re-send the current frame periodically (see the keepalive
// timer in the UI).  Query commands (OpGet*) do return a reply.
class Device
{
public:
    Device() = default;
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    bool open(const QString& path);
    void close();
    bool isOpen() const { return m_fd >= 0; }
    QString path() const { return m_path; }

    // Low level -----------------------------------------------------------
    bool writeReports(const QList<QByteArray>& reports, int timeoutMs = 500);
    bool readReport(QByteArray& out, int timeoutMs);
    // Send a query and collect the reply chunks for the matching opcode.
    bool query(const QByteArray& request, uint8_t expectOpcode,
               QList<QByteArray>& replies, int timeoutMs = 600);

    // Send a query and reassemble the chunked reply payload.
    bool readCommand(uint8_t opcode, uint8_t type, QByteArray& payload,
                     int expectedSize = 0, int timeoutMs = 900);

    // Queries -------------------------------------------------------------
    int  modelId();                 // -1 on failure
    int  status();                  // -1 on failure, else reply byte 5
    int  firmwareVersion();         // -1 on failure

    // Lighting ------------------------------------------------------------
    bool setSolid(uint8_t r, uint8_t g, uint8_t b);
    bool setDirect(const QList<ColorGroup>& groups);

    // On-board effect.  Payload layout is reverse engineered and should be
    // treated as experimental.
    bool setEffect(uint8_t effect, uint8_t brightness, uint8_t speed,
                   uint8_t direction, bool colorful, uint8_t r, uint8_t g, uint8_t b);

    // Key matrix (remapping) ---------------------------------------------
    // layer: 0 = base, 1..3 = Fn layers.  codes: 4 bytes per key, in LED order.
    bool setMatrix(int layer, const QByteArray& codes);

    // On-board LED profile (effects, brightness, speed) ----------------
    // Writes the 128-byte block with opcode 0x04.  Build it with
    // f75::baseLedProfile() and set bytes 10/58/63.
    bool writeLedProfile(const QByteArray& profile);
    bool resetDevice();

    QString lastError() const { return m_lastError; }

private:
    bool writeRaw(const QByteArray& report, int timeoutMs);
    bool readRaw(QByteArray& out, int timeoutMs);

    int          m_fd = -1;
    QString      m_path;
    mutable QMutex m_mutex;
    QString      m_lastError;
};

} // namespace f75
