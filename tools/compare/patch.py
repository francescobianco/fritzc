#!/usr/bin/env python3
"""Patches bytes in FRITZ2.UNP.EXE to test hypotheses in DOSBox.

Usage: patch.py IN.EXE OUT.EXE ADDRESS=HEX [ADDRESS=HEX ...]
  ADDRESS: SEG:OFF (image-relative segment, as in the disassembly)
           or DS:OFF (DGROUP = 3464).
  HEX:     bytes to write, e.g. ffffffffffffffff
Each patch prints the original bytes, so the experiment log keeps a record.
"""
import struct
import sys

DGROUP = 0x3464


def main() -> None:
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    data = bytearray(open(sys.argv[1], 'rb').read())
    hdr = struct.unpack_from('<H', data, 8)[0] * 16
    for spec in sys.argv[3:]:
        addr, value = spec.split('=')
        seg, off = addr.split(':')
        seg = DGROUP if seg.upper() == 'DS' else int(seg, 16)
        pos = hdr + seg * 16 + int(off, 16)
        new = bytes.fromhex(value)
        old = bytes(data[pos:pos + len(new)])
        data[pos:pos + len(new)] = new
        print(f'{addr}: {old.hex()} -> {new.hex()}')
    open(sys.argv[2], 'wb').write(data)


if __name__ == '__main__':
    main()
