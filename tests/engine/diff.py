#!/usr/bin/env python3
"""Differential tests: C engine routines vs the original FRITZ2.BIN ones.

Usage: diff.py FRITZ2.BIN libfritzengine.so

States are collected by letting the original engine play a game in the
emulator; on each state a routine runs twice, in the emulator and in C, on
the same 64 KB image, and the resulting images must be identical.
"""
import ctypes
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'tools', 'engine'))
import harness  # noqa: E402

# The stub written by call_near and the module's stack area.
IGNORE = {harness.HALT_OFFSET, *harness.STACK_SCRATCH}


class CEngine:
    def __init__(self, path):
        self.lib = ctypes.CDLL(path)
        self.buf = (ctypes.c_uint8 * 0x10000)()

    def load(self, image):
        ctypes.memmove(self.buf, bytes(image), 0x10000)

    def image(self):
        return bytearray(self.buf)

    def __getattr__(self, name):
        fn = getattr(self.lib, name)
        return lambda *args: fn(self.buf, *[ctypes.c_uint16(a) for a in args])


def collect_states(path, plies=16, depth=3):
    eng = harness.Engine(path)
    eng.command(0xFF)
    eng.command(0x00)
    eng.poke(0x9D, [1])
    eng.poke(0x9E, [depth])
    states = [harness.snapshot(eng)]
    for _ in range(plies):
        eng.command(0x04)
        eng.command(0x0A)
        states.append(harness.snapshot(eng))
    return eng, states


def collect_random_states(path, games=3, plies=60, seed=1993):
    """Random legal games: command 0x0E gives the i-th legal move (from and
    to at p+0x82, p+0x83), command 0x02 plays it. Command 2 expects the
    promotion code the EXE derives from table DS:7198 (queen 0x80), not the
    flags 0x0E leaves at p+0x84."""
    import random
    rng = random.Random(seed)
    eng = harness.Engine(path)
    eng.command(0xFF)
    states = []
    for _ in range(games):
        eng.command(0x00)
        for _ in range(plies):
            count = eng.command(0x0F)
            if count == 0 or eng.command(0x0B) != 0:
                break
            eng.command(0x0E, bytes([rng.randrange(count)]))
            frm, to = eng.peek(0x82, 2)
            piece = eng.peek(0x132 + 2 * (10 * (frm >> 3) + (frm & 7)))[0]
            promo = 0x80 if piece in (0x10, 0x20) and to >> 3 in (0, 7) else 0
            if eng.command(0x02, bytes([0, frm, to, promo])) != 0:
                break
            states.append(harness.snapshot(eng))
    return states


def compare(a, b):
    return [i for i in range(0x10000) if a[i] != b[i] and i not in IGNORE]


def board_squares():
    for rank in range(8):
        for file in range(8):
            yield 0x132 + rank * 0x14 + file * 2


def test_see(eng, c, state):
    """0x8692 on every (piece square, target square) pair, both sides."""
    harness.restore(eng, state)
    c.load(state)
    c.lib.eng_see.restype = ctypes.c_uint16
    bad = []
    calls = 0
    for si in board_squares():
        if state[si] == 0 or state[si + 1] & 0x80:
            continue
        for di in board_squares():
            if di == si:
                continue
            for dl in (0x10, 0x00):
                r = harness.call_near(eng, 0x8692, {'si': si, 'di': di, 'dx': dl, 'cx': 0, 'bp': 0})
                mine = c.lib.eng_see(c.buf, ctypes.c_uint16(si), ctypes.c_uint16(di),
                                     ctypes.c_uint16(dl))
                calls += 1
                if r['ax'] != mine:
                    bad.append((si, di, dl, r['ax'], mine))
    return calls, bad


def run(path_bin, path_lib):
    eng, states = collect_states(path_bin)
    states += collect_random_states(path_bin)
    c = CEngine(path_lib)
    failures = 0
    for n, state in enumerate(states):
        # 0x46b7 alone
        harness.restore(eng, state)
        harness.call_near(eng, 0x46B7)
        c.load(state)
        c.eng_attacks_clear()
        diff = compare(harness.snapshot(eng), c.image())
        # 0x46b7 then 0x46f8 (rebuild from scratch)
        harness.restore(eng, state)
        harness.call_near(eng, 0x46B7)
        harness.call_near(eng, 0x46F8)
        c.load(state)
        c.eng_attacks_clear()
        c.eng_attacks_build()
        diff2 = compare(harness.snapshot(eng), c.image())
        # rebuilt tables must equal the incrementally maintained ones
        diff3 = compare(state, harness.snapshot(eng))
        ok = not diff and not diff2
        failures += not ok
        print(f'state {n:2d}: clear {len(diff)} diffs, build {len(diff2)} diffs, '
              f'rebuild vs incremental {len(diff3)} diffs' + ('' if ok else '  FAIL'))
        if diff2:
            print('   first:', [hex(i) for i in diff2[:8]])
        calls, bad = test_see(eng, c, state)
        failures += bool(bad)
        print(f'          see: {calls} calls, {len(bad)} mismatches'
              + ''.join(f'\n            si={a:#x} di={b:#x} dl={d:#x} orig={o:#06x} c={m:#06x}'
                        for a, b, d, o, m in bad[:5]))
    return failures


if __name__ == '__main__':
    sys.exit(1 if run(sys.argv[1], sys.argv[2]) else 0)
