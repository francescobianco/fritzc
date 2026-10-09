#!/usr/bin/env python3
"""Unpacker for DOS executables compressed with Microsoft EXEPACK.

Usage: unexepack.py INPUT.EXE OUTPUT.EXE

Rebuilds a standard MZ executable with the original relocation table,
so it can be loaded into Ghidra or disassembled.
"""
import struct
import sys


def unpack(data: bytes) -> bytes:
    (magic, last, pages, nrel, hdrpar, minalloc, maxalloc, ss, sp, csum,
     ip, cs, lfarlc, ovl) = struct.unpack_from('<14H', data, 0)
    if magic != 0x5A4D:
        raise ValueError('not an MZ file')
    size = pages * 512 - ((512 - last) if last else 0)
    image = data[hdrpar * 16:size]

    # EXEPACK header at the start of the CS segment (16- or 18-byte variant).
    hdr_off = cs * 16
    real_ip, real_cs, _mem, exepack_size, real_sp, real_ss, dest_len = \
        struct.unpack_from('<7H', image, hdr_off)
    skip_len = 1
    if struct.unpack_from('<H', image, hdr_off + 14)[0] != 0x4252:  # "RB"
        skip_len = struct.unpack_from('<H', image, hdr_off + 14)[0]
        if struct.unpack_from('<H', image, hdr_off + 16)[0] != 0x4252:
            raise ValueError('EXEPACK "RB" signature not found')
    hdr_len = 16 if skip_len == 1 else 18

    compressed = bytearray(image[:hdr_off - (skip_len - 1) * 16])
    out = bytearray(dest_len * 16)

    # Decompress backwards.
    src = len(compressed) - 1
    while src >= 0 and compressed[src] == 0xFF:
        src -= 1
    dst = len(out) - 1
    while True:
        cmd = compressed[src]
        count = compressed[src - 1] << 8 | compressed[src - 2]
        src -= 3
        op = cmd & 0xFE
        if op == 0xB0:
            fill = compressed[src]
            src -= 1
            for _ in range(count):
                out[dst] = fill
                dst -= 1
        elif op == 0xB2:
            for _ in range(count):
                out[dst] = compressed[src]
                dst -= 1
                src -= 1
        else:
            raise ValueError(f'unknown EXEPACK command {cmd:#x}')
        if cmd & 1:
            break
    # Bytes before the first compressed block are left as they are.
    out[:src + 1] = compressed[:src + 1]

    # Packed relocation table: 16 groups (one per 0x1000 paragraphs).
    stub = hdr_off + hdr_len
    msg = image.find(b'Packed file is corrupt', stub)
    if msg < 0:
        raise ValueError('EXEPACK stub not recognised')
    p = msg + len(b'Packed file is corrupt')
    relocs = []
    for seg in range(16):
        n = struct.unpack_from('<H', image, p)[0]
        p += 2
        for _ in range(n):
            off = struct.unpack_from('<H', image, p)[0]
            p += 2
            relocs.append((off, seg * 0x1000))

    hdr_size = 0x1C + 4 * len(relocs)
    hdr_paras = (hdr_size + 15) // 16
    total = hdr_paras * 16 + len(out)
    h = bytearray(hdr_paras * 16)
    struct.pack_into('<14H', h, 0, 0x5A4D, total % 512, (total + 511) // 512,
                     len(relocs), hdr_paras,
                     max(0, minalloc + (len(image) // 16) - dest_len), 0xFFFF,
                     real_ss, real_sp, 0, real_ip, real_cs, 0x1C, 0)
    for i, (off, seg) in enumerate(relocs):
        struct.pack_into('<HH', h, 0x1C + 4 * i, off, seg)
    return bytes(h) + bytes(out)


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    data = open(sys.argv[1], 'rb').read()
    out = unpack(data)
    open(sys.argv[2], 'wb').write(out)
    print(f'{sys.argv[1]}: {len(data)} -> {len(out)} bytes')


if __name__ == '__main__':
    main()
