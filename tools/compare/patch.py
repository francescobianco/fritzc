#!/usr/bin/env python3
"""Applica patch byte a FRITZ2.UNP.EXE per verificare ipotesi in DOSBox.

Uso: patch.py IN.EXE OUT.EXE INDIRIZZO=HEX [INDIRIZZO=HEX ...]
  INDIRIZZO: SEG:OFF (segmento relativo all'immagine, come nel disassemblato)
             oppure DS:OFF (DGROUP = 3464).
  HEX:       byte da scrivere, es. ffffffffffffffff
Ogni patch stampa i byte originali, così resta traccia nel log dell'esperimento.
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
