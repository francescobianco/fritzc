#!/usr/bin/env python3
"""Decompressore per eseguibili DOS compressi con Microsoft EXEPACK.

Uso: unexepack.py INPUT.EXE OUTPUT.EXE

Ricostruisce un eseguibile MZ standard con la tabella di rilocazione
originale, così da poterlo caricare in Ghidra o disassemblare.
"""
import struct
import sys


def unpack(data: bytes) -> bytes:
    (magic, last, pages, nrel, hdrpar, minalloc, maxalloc, ss, sp, csum,
     ip, cs, lfarlc, ovl) = struct.unpack_from('<14H', data, 0)
    if magic != 0x5A4D:
        raise ValueError('non è un file MZ')
    size = pages * 512 - ((512 - last) if last else 0)
    image = data[hdrpar * 16:size]

    # Header EXEPACK all'inizio del segmento CS (variante a 16 o 18 byte).
    hdr_off = cs * 16
    real_ip, real_cs, _mem, exepack_size, real_sp, real_ss, dest_len = \
        struct.unpack_from('<7H', image, hdr_off)
    skip_len = 1
    if struct.unpack_from('<H', image, hdr_off + 14)[0] != 0x4252:  # "RB"
        skip_len = struct.unpack_from('<H', image, hdr_off + 14)[0]
        if struct.unpack_from('<H', image, hdr_off + 16)[0] != 0x4252:
            raise ValueError('firma EXEPACK "RB" non trovata')
    hdr_len = 16 if skip_len == 1 else 18

    compressed = bytearray(image[:hdr_off - (skip_len - 1) * 16])
    out = bytearray(dest_len * 16)

    # Decompressione all'indietro.
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
            raise ValueError(f'comando EXEPACK sconosciuto {cmd:#x}')
        if cmd & 1:
            break
    # I byte che precedono il primo blocco compresso restano invariati.
    out[:src + 1] = compressed[:src + 1]

    # Tabella di rilocazione compressa: 16 gruppi (uno per 0x1000 paragrafi).
    stub = hdr_off + hdr_len
    msg = image.find(b'Packed file is corrupt', stub)
    if msg < 0:
        raise ValueError('stub EXEPACK non riconosciuto')
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
    print(f'{sys.argv[1]}: {len(data)} -> {len(out)} byte')


if __name__ == '__main__':
    main()
