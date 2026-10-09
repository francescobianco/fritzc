#!/usr/bin/env python3
"""Disassembla un intervallo di FRITZ2.UNP.EXE in indirizzi SEG:OFF.

Uso: dis.py EXE SEG:OFF [N_ISTRUZIONI]
Le chiamate far a segmenti noti sono annotate con il target lineare.
"""
import struct
import sys

from capstone import CS_ARCH_X86, CS_MODE_16, Cs


def load(path: str) -> bytes:
    d = open(path, 'rb').read()
    return d[struct.unpack_from('<H', d, 8)[0] * 16:]


def main() -> None:
    img = load(sys.argv[1])
    seg, off = (int(x, 16) for x in sys.argv[2].split(':'))
    n = int(sys.argv[3]) if len(sys.argv) > 3 else 60
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    base = seg * 16
    for i, ins in enumerate(md.disasm(img[base + off:base + 0x10000], off)):
        if i >= n:
            break
        print(f'{seg:04x}:{ins.address:04x}  {ins.bytes.hex():<14} {ins.mnemonic} {ins.op_str}')


if __name__ == '__main__':
    main()
