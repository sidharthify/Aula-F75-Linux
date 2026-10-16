#include "f75device.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QMap>
#include <QThread>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>

namespace f75 {

Device::~Device()
{
    close();
}

bool Device::open(const QString& path)
{
    close();
    m_fd = ::open(path.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);
    if (m_fd < 0)
    {
        m_lastError = QStringLiteral("open(%1): %2").arg(path, QString::fromLocal8Bit(strerror(errno)));
        return false;
    }
    m_path = path;
    m_lastError.clear();
    return true;
}

void Device::close()
{
    if (m_fd >= 0)
    {
        ::close(m_fd);
        m_fd = -1;
    }
    m_path.clear();
}

bool Device::writeRaw(const QByteArray& report, int timeoutMs)
{
    if (m_fd < 0)
    {
        return false;
    }

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs)
    {
        ssize_t n = ::write(m_fd, report.constData(), report.size());
        if (n == report.size())
        {
            return true;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
        {
            QThread::msleep(2);
            continue;
        }
        m_lastError = QStringLiteral("write: %1").arg(QString::fromLocal8Bit(strerror(errno)));
        return false;
    }
    m_lastError = QStringLiteral("write timeout");
    return false;
}

bool Device::readRaw(QByteArray& out, int timeoutMs)
{
    if (m_fd < 0)
    {
        return false;
    }
    struct pollfd pf;
    pf.fd = m_fd;
    pf.events = POLLIN;
    pf.revents = 0;
    int pr = ::poll(&pf, 1, timeoutMs);
    if (pr <= 0)
    {
        return false;
    }

    char buf[kReportSize];
    ssize_t n = ::read(m_fd, buf, sizeof(buf));
    if (n <= 0)
    {
        return false;
    }
    out = QByteArray(buf, static_cast<int>(n));
    return true;
}

bool Device::writeReports(const QList<QByteArray>& reports, int timeoutMs)
{
    QMutexLocker lock(&m_mutex);
    for (const QByteArray& r : reports)
    {
        if (!writeRaw(r, timeoutMs))
        {
            return false;
        }
        // Small gap between chunks, as the reference driver does.
        QThread::msleep(4);
    }
    return true;
}

bool Device::readReport(QByteArray& out, int timeoutMs)
{
    QMutexLocker lock(&m_mutex);
    return readRaw(out, timeoutMs);
}

bool Device::query(const QByteArray& request, uint8_t expectOpcode,
                   QList<QByteArray>& replies, int timeoutMs)
{
    replies.clear();
    if (!writeReports({ request }))
    {
        return false;
    }
    QElapsedTimer timer;
    timer.start();
    int lastIndex = -1;
    while (timer.elapsed() < timeoutMs)
    {
        QByteArray r;
        if (!readReport(r, 60))
        {
            continue;
        }
        if (r.size() < kReportSize)
        {
            continue;
        }
        const uint8_t* p = reinterpret_cast<const uint8_t*>(r.constData());
        if (p[0] != kReportId || (p[1] & 0x7F) != (expectOpcode & 0x7F))
        {
            continue;
        }
        const int total = p[2] & 0x7F;
        const int index = p[3] & 0x7F;
        replies.append(r);
        lastIndex = index;
        if (total <= 0 || index + 1 >= total)
        {
            return true;
        }
    }
    return !replies.isEmpty();
}

bool Device::readCommand(uint8_t opcode, uint8_t type, QByteArray& payload,
                         int expectedSize, int timeoutMs)
{
    uint8_t p[kReportSize];
    memset(p, 0, sizeof(p));
    p[0] = kReportId;
    p[1] = opcode;
    p[2] = 0x01;
    p[3] = 0x00;
    p[4] = static_cast<uint8_t>(type << 4);
    p[19] = calculateCrc(p, kReportSize);

    QList<QByteArray> replies;
    if (!query(QByteArray(reinterpret_cast<char*>(p), kReportSize), opcode, replies, timeoutMs))
    {
        return false;
    }

    QMap<int, QByteArray> chunks;
    int maxIndex = 0;
    for (const QByteArray& r : replies)
    {
        if (r.size() < kReportSize)
        {
            continue;
        }
        const uint8_t* q = reinterpret_cast<const uint8_t*>(r.constData());
        const int idx = q[3] & 0x7F;
        const int len = q[4] & 0x0F;
        chunks.insert(idx, QByteArray(r.constData() + 5, qMin(len, kPayloadPerChunk)));
        maxIndex = qMax(maxIndex, idx);
    }

    payload.clear();
    for (int i = 0; i <= maxIndex; ++i)
    {
        if (chunks.contains(i))
        {
            payload.append(chunks.value(i));
        }
    }
    if (expectedSize > 0 && payload.size() < expectedSize)
    {
        payload.resize(expectedSize);
    }
    return !payload.isEmpty();
}

int Device::modelId()
{
    uint8_t p[kReportSize];
    memset(p, 0, sizeof(p));
    p[0] = kReportId;
    p[1] = OpGetModel;
    p[2] = 0x01;
    p[19] = calculateCrc(p, kReportSize);

    QList<QByteArray> replies;
    if (!query(QByteArray(reinterpret_cast<char*>(p), kReportSize), OpGetModel, replies))
    {
        return -1;
    }
    // The model id sits at byte 10 of the reply report.
    const QByteArray& r = replies.first();
    if (r.size() >= 11)
    {
        return static_cast<uint8_t>(r.at(10));
    }
    return -1;
}

int Device::status()
{
    uint8_t p[kReportSize];
    memset(p, 0, sizeof(p));
    p[0] = kReportId;
    p[1] = OpGetStatus;
    p[2] = 0x01;
    p[19] = calculateCrc(p, kReportSize);

    QList<QByteArray> replies;
    if (!query(QByteArray(reinterpret_cast<char*>(p), kReportSize), OpGetStatus, replies))
    {
        return -1;
    }
    const QByteArray& r = replies.first();
    return r.size() >= 6 ? static_cast<uint8_t>(r.at(5)) : 0;
}

int Device::firmwareVersion()
{
    // Firmware version is read from the HID attributes on Windows; over
    // hidraw the closest equivalent the driver exposes is the status query.
    return status();
}

bool Device::setSolid(uint8_t r, uint8_t g, uint8_t b)
{
    QByteArray payload = solidPayload(r, g, b);
    return writeReports(buildChunks(OpSetRealData, LightSolid, payload));
}

bool Device::setDirect(const QList<ColorGroup>& groups)
{
    QByteArray payload = directPayload(groups);
    if (payload.isEmpty())
    {
        // Nothing to light: send an empty group to clear the board.
        payload = directPayload({ ColorGroup{ 0, 0, 0, QByteArray(1, '\0') } });
    }
    if (payload.size() > kMaxPayload)
    {
        m_lastError = QStringLiteral("direct payload too large (%1 bytes)").arg(payload.size());
        return false;
    }
    return writeReports(buildChunks(OpSetRealData, LightDirect, payload));
}

bool Device::setEffect(uint8_t effect, uint8_t brightness, uint8_t speed,
                       uint8_t direction, bool colorful, uint8_t r, uint8_t g, uint8_t b)
{
    QByteArray payload(8, '\0');
    payload[0] = static_cast<char>(effect);
    payload[1] = static_cast<char>(brightness); // 0..4 (LightHW)
    payload[2] = static_cast<char>(speed);      // 0..4 (SpeedHW)
    payload[3] = static_cast<char>(direction);  // 0..1
    payload[4] = static_cast<char>(colorful ? 1 : 0);
    payload[5] = static_cast<char>(r);
    payload[6] = static_cast<char>(g);
    payload[7] = static_cast<char>(b);
    return writeReports(buildChunks(OpSetEffect, 0, payload));
}

bool Device::setMatrix(int layer, const QByteArray& codes)
{
    if (layer < 0 || layer > 3)
    {
        return false;
    }
    return writeReports(buildChunks(OpSetMatrix, static_cast<uint8_t>(layer), codes));
}

bool Device::resetDevice()
{
    uint8_t p[kReportSize];
    memset(p, 0, sizeof(p));
    p[0] = kReportId;
    p[1] = OpResetDevice;
    p[2] = 0x01;
    p[19] = calculateCrc(p, kReportSize);
    return writeReports({ QByteArray(reinterpret_cast<char*>(p), kReportSize) });
}

} // namespace f75
