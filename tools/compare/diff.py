#!/usr/bin/env python3
"""Compares a port frame with a DOSBox capture, pixel by pixel (palette indices).

Usage: diff.py NATIVE.bmp CAPTURE.png [x0,y0,x1,y1 ...]
Without regions the whole 640x480 screen is compared. Exit status 1 if any
compared pixel differs.
"""
import sys

from PIL import Image


def main() -> None:
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    a = Image.open(sys.argv[1])
    b = Image.open(sys.argv[2])
    if a.mode != 'P' or b.mode != 'P':
        sys.exit('both images must be palettized')
    pa, pb = a.load(), b.load()
    regions = [tuple(map(int, r.split(','))) for r in sys.argv[3:]] or [(0, 0, *a.size)]
    failed = False
    for x0, y0, x1, y1 in regions:
        bad = [(x, y) for y in range(y0, y1) for x in range(x0, x1) if pa[x, y] != pb[x, y]]
        print(f'{x0},{y0},{x1},{y1}: {len(bad)} of {(x1 - x0) * (y1 - y0)} differ'
              + (f', first at {bad[0]}' if bad else ''))
        failed |= bool(bad)
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
