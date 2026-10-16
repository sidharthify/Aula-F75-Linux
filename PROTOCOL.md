# AULA F75 HID protocol

Recovered from the Windows driver `OemDrv.exe`
(`Program Files (x86)/AULA/F75`, MFC/GDI+ application) and the keyboard's HID
report descriptors. Everything here was read out of the shipped binary; entries
marked **(verified)** were additionally confirmed by driving the physical
keyboard over `/dev/hidraw`.

## Transport

The config interface is a vendor HID collection:

- Usage Page `0xFF02`, Usage `0x0002`
- Report ID `0x13`
- 19 data bytes → 20 byte reports
- present on interface 1 of the Compx `3554:fa09` 2.4 GHz receiver, and on the
  Sinowealth `258a:010c` wired device

Every host→device report (and device→host reply) is framed as:

```
offset  size  meaning
  0      1    report id (0x13)
  1      1    opcode
  2      1    chunk count    (number of reports in the transaction)
  3      1    chunk index    (0-based)
  4      1    (type << 4) | payload length   (length ≤ 14)
  5..18  14   payload
 19      1    CRC = sum of bytes 0..18, mod 256
```

Windows uses overlapped `WriteFile`/`ReadFile` against the HID device; on Linux
that is a plain `write()`/`read()` on `/dev/hidrawN`.

## Opcodes

| Opcode | Direction | Name | Notes |
|-------:|-----------|------|-------|
| `0x05` | get | model | reply byte 10 = model id (**verified** via OpenRGB) |
| `0x07` | get | status | dongle/online status |
| `0x41` | get | matrix | key matrix for a layer |
| `0x42` | get | game | |
| `0x43` | get | macro | |
| `0x44` | get | LED | |
| `0x49` | get | LED RGB table | |
| `0x4a` | get | on-board config | |
| `0x01` | set | matrix | 4 bytes per key, `layer` in `type` |
| `0x02` | set | game | |
| `0x03` | set | macro | 0x200-byte records |
| `0x04` | set | LED | |
| `0x06` | set | reset device | |
| `0x09` | set | LED RGB table | |
| `0x0a` | set | on-board config | |
| `0x0b` | set | screen param | |
| `0x88` | set | **real data (lighting)** | `type`: see below |
| `0x8e` | set | effect | payload not confirmed |

Model ids: `0xA4` = AULA F99, `0xCD` = AULA F75.

## Lighting — opcode `0x88`

The low nibble of byte 4 selects the flavour:

- `type = 1` — **per-key**, payload split into 14-byte chunks. Raw payload is a
  sequence of colour groups:

  ```
  RR GG BB count idx[count] …
  ```

  (**verified** — this is what OpenRGB and the included probe use.)

- `type = 2` — **solid**, a 3-byte payload `RR GG BB`
  (**verified**). Byte 4 is then `0x23`.

### Example: solid red

```
13 88 01 00 23 ff 00 00 00 00 00 00 00 00 00 00 00 00 00 CRC
```

### Example: model query

```
13 05 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 19
```

## LED map

The F75 numbers its LEDs **column-major, six rows per column**, so a key's LED
value is `column * 6 + row`:

| Key | LED | Key | LED |
|-----|----:|-----|----:|
| Esc | 0 | F12 | 78 |
| ` | 1 | Backspace | 79 |
| Tab | 2 | \ | 80 |
| CapsLock | 3 | Enter | 81 |
| LShift | 4 | ↑ (Up) | 82 |
| LCtrl | 5 | ↓ (Down) | 83 |
| … | | → (Right) | 89 |
| ← (Left) | 77 | knob | — (no LED) |

The navigation column is Delete = 85, PgUp = 86, PgDn = 87, End = 88. Values
84 and 90–95 are unused. The complete, per-key table is in
`src/model/keymap.cpp` (extracted from the driver's `Dev/kb/F75KR/KB.ini`).

## Direct-mode keepalive

While the board is in direct colour mode it must be fed continuously. If it
goes ~a few hundred milliseconds without a packet it returns to its on-board
effect. The application therefore re-sends the current frame every 50 ms.

## Unresolved

- On-board effect payload — the driver stores lighting/effect configuration in a
  128-byte "LED profile" block read with opcode `0x44` and written with opcode
  `0x04` (the driver verifies an `A5 5A` magic near the end).  The block layout
  (which bytes select the effect, brightness, speed and direction) has not been
  decoded yet, so the application's Effects tab is not functional.  Use
  `tools/f75cfg` to read/diff/write the block and help isolate the fields:

      gcc -O2 -o f75cfg tools/f75cfg.c
      ./f75cfg read  /dev/hidraw4
      ./f75cfg diff  /dev/hidraw4     # change the on-board effect, then Enter
      ./f75cfg write <256 hex chars>  # write a profile back

- Macro record format (`0x03`) — the driver writes 0x200-byte records with a
  name and event list; not yet decoded.
- Key matrix index ↔ LED index mapping — assumed 1:1 in `RemapPage` and marked
  experimental.

## Tooling

`tools/f75cfg.c` speaks opcodes `0x05`, `0x44` and `0x04` directly and is the
intended way to finish the effect decode.

