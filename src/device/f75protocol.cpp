#include "f75protocol.h"

namespace f75 {

uint8_t calculateCrc(const uint8_t* report, int len)
{
    uint8_t crc = 0;
    for (int i = 0; i < len - 1; ++i)
    {
        crc += report[i];
    }
    return crc;
}

QByteArray buildPacket(uint8_t opcode, uint8_t type, int chunkCount, int chunkIndex,
                       const uint8_t* payload, int payloadLen)
{
    if (payloadLen > kPayloadPerChunk)
    {
        payloadLen = kPayloadPerChunk;
    }

    QByteArray pkt(kReportSize, '\0');
    uint8_t* p = reinterpret_cast<uint8_t*>(pkt.data());

    p[0] = kReportId;
    p[1] = opcode;
    p[2] = static_cast<uint8_t>(chunkCount);
    p[3] = static_cast<uint8_t>(chunkIndex);
    p[4] = static_cast<uint8_t>((type << 4) | (payloadLen & 0x0F));

    if (payload != nullptr && payloadLen > 0)
    {
        for (int i = 0; i < payloadLen; ++i)
        {
            p[5 + i] = payload[i];
        }
    }

    p[19] = calculateCrc(p, kReportSize);
    return pkt;
}

QList<QByteArray> buildChunks(uint8_t opcode, uint8_t type, const QByteArray& payload)
{
    QList<QByteArray> out;
    const int len = payload.size();
    const int chunks = len == 0 ? 1 : (len + kPayloadPerChunk - 1) / kPayloadPerChunk;

    for (int c = 0; c < chunks; ++c)
    {
        const int off = c * kPayloadPerChunk;
        int n = len - off;
        if (n > kPayloadPerChunk)
        {
            n = kPayloadPerChunk;
        }
        if (n < 0)
        {
            n = 0;
        }
        const uint8_t* data = n > 0
            ? reinterpret_cast<const uint8_t*>(payload.constData()) + off
            : nullptr;
        out.append(buildPacket(opcode, type, chunks, c, data, n));
    }
    return out;
}

QByteArray solidPayload(uint8_t r, uint8_t g, uint8_t b)
{
    QByteArray p(3, '\0');
    p[0] = static_cast<char>(r);
    p[1] = static_cast<char>(g);
    p[2] = static_cast<char>(b);
    return p;
}

QByteArray directPayload(const QList<ColorGroup>& groups)
{
    QByteArray payload;
    for (const ColorGroup& g : groups)
    {
        if (g.indices.isEmpty())
        {
            continue;
        }
        payload.append(static_cast<char>(g.r));
        payload.append(static_cast<char>(g.g));
        payload.append(static_cast<char>(g.b));
        payload.append(static_cast<char>(g.indices.size()));
        payload.append(g.indices);
    }
    return payload;
}

} // namespace f75
