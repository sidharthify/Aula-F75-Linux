# tools

Helper utilities used while reverse engineering the protocol.

- `f75cfg.c` — read/diff/write the AULA F75 on-board 128-byte LED profile
  (opcodes `0x44` / `0x04`).  Used to isolate the on-board effect fields.

      gcc -O2 -o f75cfg tools/f75cfg.c
      ./f75cfg read  /dev/hidraw4
      ./f75cfg diff  /dev/hidraw4
      ./f75cfg write <256 hex chars> /dev/hidraw4
