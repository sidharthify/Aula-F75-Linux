#pragma once

// AULA F75 HID wire protocol.
//
// Reverse engineered from the Windows driver "OemDrv.exe"
// (Program Files (x86)/AULA/F75) and cross-checked against the device on
// /dev/hidraw.  The board exposes a vendor HID collection with
// Usage Page 0xFF02 / Usage 0x0002, Report ID 0x13 and 19 data bytes.
//
// Every host->device report is 20 bytes:
//
//     [0] 0x13                 report id
//     [1] opcode
//     [2] chunk count          (number of reports in this transaction)
//     [3] chunk index          (0-based)
//     [4] (type << 4) | length (length of the payload in this report, max 14)
//     [5..18] payload
//     [19] CRC                 (8-bit sum of bytes 0..18)
//
// Device->host replies use the same framing.

#include <QByteArray>
#include <QList>
#include <cstdint>

namespace f75 {

constexpr int     kReportSize    = 20;
constexpr uint8_t kReportId      = 0x13;
constexpr int     kPayloadPerChunk = 14;
constexpr int     kMaxPayload    = kPayloadPerChunk * 0x7F; // 7-bit chunk count

// Opcodes, recovered from the CDev916KB device class in OemDrv.exe.
enum Opcode : uint8_t
{
    OpGetModel       = 0x05, // -> model id (reply byte 10)
    OpGetStatus      = 0x07, // -> online / dongle status
    OpGetMatrix      = 0x41,
    OpGetGame        = 0x42,
    OpGetMacro       = 0x43,
    OpGetLed         = 0x44,
    OpGetLedRgbTab   = 0x49,
    OpGetOnBoard     = 0x4A,

    OpSetMatrix      = 0x01,
    OpSetGame        = 0x02,
    OpWriteMacro     = 0x03,
    OpSetLed         = 0x04,
    OpResetDevice    = 0x06,
    OpSetLedRgbTab   = 0x09,
    OpSetOnBoard     = 0x0A,
    OpSetScreenParam = 0x0B,
    OpSetRealData    = 0x88, // direct colour (per-LED or solid)
    OpSetEffect      = 0x8E, // on-board effect configuration
};

// The low nibble of byte 4 ("type") for OpSetRealData.
enum LightingType : uint8_t
{
    LightDirect = 0x1, // per colour-group colour table, 14 byte chunks
    LightSolid  = 0x2, // a single colour, 3 byte payload
};

// Known model ids returned by OpGetModel.
enum ModelId : uint8_t
{
    ModelF99 = 0xA4,
    ModelF75 = 0xCD,
};

uint8_t calculateCrc(const uint8_t* report, int len);

// One already framed 20 byte report (CRC filled in).
QByteArray buildPacket(uint8_t opcode, uint8_t type, int chunkCount, int chunkIndex,
                       const uint8_t* payload, int payloadLen);

// Split a payload into framed 20 byte reports.
QList<QByteArray> buildChunks(uint8_t opcode, uint8_t type, const QByteArray& payload);

// Build the payload for a solid colour request (type LightSolid).
QByteArray solidPayload(uint8_t r, uint8_t g, uint8_t b);

struct ColorGroup
{
    uint8_t r = 0, g = 0, b = 0;
    QByteArray indices; // LED indices that share this colour
};

// Build the per-LED colour-group payload (type LightDirect).
QByteArray directPayload(const QList<ColorGroup>& groups);

// The F75's 128-byte on-board "LED profile" block, captured from the Windows
// driver.  Byte [10] selects the effect, [58] is brightness (0..9) and [63] is
// speed encoded as (speed << 4) | 0x07 (speed 0..4).  It is read with opcode
// 0x44 and written with opcode 0x04.
QByteArray baseLedProfile();
constexpr int kLedProfileSize = 128;
constexpr int kOffEffectId     = 0x00;

} // namespace f75
