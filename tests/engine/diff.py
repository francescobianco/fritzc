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
    """The C Engine struct: a window pointer, the segment and 1 MB of RAM.
    Images are full 1 MB memory snapshots of the emulator."""

    RAM_OFFSET = ctypes.sizeof(ctypes.c_void_p) * 2   # after mem and segment (padded)

    def __init__(self, path):
        self.lib = ctypes.CDLL(path)
        self.lib.eng_size.restype = ctypes.c_ulong
        self.buf = (ctypes.c_uint8 * self.lib.eng_size())()
        self.lib.eng_bind(self.buf, ctypes.c_uint16(harness.SEG))
        self.ram_offset = len(self.buf) - 0x100000

    def load(self, image):
        ctypes.memmove(ctypes.addressof(self.buf) + self.ram_offset, bytes(image), 0x100000)

    def image(self):
        return bytearray(self.buf)[self.ram_offset:]

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


# Scripted games that reach the special moves (castling both sides, en
# passant both sides, promotions with and without capture).
SCRIPTS = [
    'e2e4 e7e5 g1f3 b8c6 f1c4 f8c5 d2d3 g8f6 c1e3 d7d6 b1c3 c8e6 d1d2 d8d7',
    'e2e4 a7a6 e4e5 d7d5 a2a3 h7h5 g2g4 h5g4 h2h4 g4h3',
    'h2h4 g7g5 h4g5 f7f6 g5f6 a7a6 f6e7 a6a5 g2g4 a5a4 g4g5 a4a3 g5g6 a3b2',
]


def collect_scripted_states(path):
    eng = harness.Engine(path)
    eng.command(0xFF)
    states = []
    for script in SCRIPTS:
        eng.command(0x00)
        for move in script.split():
            frm, to = harness.square(move[:2]), harness.square(move[2:])
            if eng.command(0x02, bytes([0, frm, to, 0])) != 0:
                raise RuntimeError(f'illegal scripted move {move}')
            states.append(harness.snapshot(eng))
    return states


def compare(a, b):
    """Differences between two 1 MB images, as offsets in the engine segment
    when inside it, else as linear addresses + 0x100000."""
    if a == b:
        return []
    base = harness.SEG * 16
    out = []
    for i in range(0x100000):
        if a[i] != b[i]:
            if base <= i < base + 0x10000:
                if i - base not in IGNORE:
                    out.append(i - base)
            elif not harness.STACK_SEG * 16 <= i < harness.STACK_SEG * 16 + 0x10000:
                out.append(i + 0x100000)
    return out


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
        if byte(state, si) == 0 or byte(state, si + 1) & 0x80:
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


BASE = harness.SEG * 16


def byte(image, off):
    """Byte at offset `off` of the engine segment in a 1 MB image."""
    return image[BASE + off]


def word(image, off):
    return image[BASE + off] | image[BASE + off + 1] << 8


def test_square_updates(eng, c, state):
    """0x45f4 dispatch (hash + attacks) on every piece, 0x4646 and 0x4624 on
    every square; each on a fresh copy of the state, whole image compared."""
    bad = []
    calls = 0
    code_table = {code: word(state, 0x45f4 + code) for code in range(0x10, 0x30, 2)}
    for di in board_squares():
        cases = [('slider_update', 0x4646, 'eng_slider_update', (di,)),
                 ('square_update', 0x4624, 'eng_square_update', (di,))]
        code = word(state, di)
        if 0x10 <= code < 0x30:
            mask = word(state, word(state, di + 0x100) + 0x42f0)
            cases.append(('piece_toggle', code_table[code], 'eng_piece_toggle', (code, di, mask)))
        for name, addr, fn, args in cases:
            harness.restore(eng, state)
            regs = {'di': di}
            if name == 'piece_toggle':
                regs.update(bp=code, dx=args[2])
            harness.call_near(eng, addr, regs)
            c.load(state)
            getattr(c, fn)(*args)
            d = compare(harness.snapshot(eng), c.image())
            calls += 1
            if d:
                bad.append((name, di, [hex(i) for i in d[:4]]))
    return calls, bad


class Regs(ctypes.Structure):
    _fields_ = [(n, ctypes.c_uint16) for n in ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp')]


def test_eval_terms(eng, c, state, rng):
    """0x634a / 0x632a dispatch (eval_add / eval_remove) with varied inputs."""
    bad = []
    calls = 0
    add_table = {code: word(state, 0x634a + code) for code in range(0x10, 0x30, 2)}
    rem_table = {code: word(state, 0x632a + code) for code in range(0x10, 0x30, 2)}
    cases = []
    for di in board_squares():
        code = word(state, di)
        if 0x10 <= code < 0x30:
            cases.append(('remove', code, di, di, 0x30 if rng.random() < 0.5 else 0))
    empties = [di for di in board_squares() if word(state, di) == 0]
    for code in range(0x10, 0x30, 2):
        for di in rng.sample(empties, min(6, len(empties))):
            si = di - 0x28 if code == 0x10 else di + 0x28 if code == 0x20 else di
            for cl in (0x00, 0x30, 0x01, 0x40, 0x41, 0x80):
                cases.append(('add', code, di, si, cl))
    for kind, code, di, si, cl in cases:
        ax = rng.randrange(0x10000)
        dx = rng.randrange(0x10000)
        harness.restore(eng, state)
        table = add_table if kind == 'add' else rem_table
        r = harness.call_near(eng, table[code], {'ax': ax, 'bp': code, 'di': di, 'si': si,
                                                  'cx': cl, 'dx': dx, 'bx': 0})
        c.load(state)
        regs = Regs(ax=ax, bx=0, cx=cl, dx=dx, si=si, di=di, bp=code)
        getattr(c.lib, 'eng_eval_' + kind)(c.buf, ctypes.byref(regs))
        calls += 1
        mem = compare(harness.snapshot(eng), c.image())
        if (r['ax'], r['bp'], r['cx']) != (regs.ax, regs.bp, regs.cx) or mem:
            bad.append((kind, hex(code), hex(di), hex(si), hex(cl),
                        f"orig ax={r['ax']:#06x} bp={r['bp']:#x} cx={r['cx']:#x}",
                        f'c ax={regs.ax:#06x} bp={regs.bp:#x} cx={regs.cx:#x}',
                        [hex(i) for i in mem[:3]]))
    return calls, bad


def capture_calls(eng, state, addresses, command=0x0F, limit=400):
    """Runs `command` on `state` in the emulator and records (address,
    registers, memory image) at every entry of the given routines."""
    from unicorn import UC_HOOK_CODE
    ids = harness._reg_ids()
    seen = []

    def hook(uc, address, size, user):
        if len(seen) < limit:
            regs = {n: uc.reg_read(r) for n, r in ids.items()}
            seen.append((address - harness.SEG * 16, regs, harness.snapshot(eng)))

    handles = [eng.uc.hook_add(UC_HOOK_CODE, hook, begin=harness.SEG * 16 + a,
                               end=harness.SEG * 16 + a) for a in addresses]
    harness.restore(eng, state)
    eng.command(command)
    for h in handles:
        eng.uc.hook_del(h)
    return seen


MOVE_KINDS = {}


def move_kind(cl):
    if cl & 0x80 and cl & 0xc1 not in (0, 0x40, 0x41):
        return 'promotion'
    if cl & 0x3e:
        return 'capture'
    if (cl & 0xc1) == 0x40:
        return 'en passant'
    if (cl & 0xc1) == 0x41:
        return 'double push'
    if (cl & 0xc1) == 0x01:
        return 'castling'
    return 'quiet'


def test_make_unmake(eng, c, state):
    """0x6f81 / 0x719e with the registers the move generator really uses."""
    bad = []
    calls = capture_calls(eng, state, (0x6F81, 0x719E))
    for address, regs, image in calls:
        if address == 0x719E:
            flags = byte(image, regs['bp'] + 0x1012)
            MOVE_KINDS[move_kind(flags)] = MOVE_KINDS.get(move_kind(flags), 0) + 1
        harness.restore(eng, image)
        keep = {k: regs[k] for k in ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp')}
        r = harness.call_near(eng, address, keep)
        c.load(image)
        cr = Regs(**keep)
        fn = 'eng_make_move' if address == 0x6F81 else 'eng_unmake_move'
        getattr(c.lib, fn)(c.buf, ctypes.byref(cr))
        mem = compare(harness.snapshot(eng), c.image())
        outs = ('ax', 'cx') if address == 0x6F81 else ('cx', 'dx')
        if any(r[o] != getattr(cr, o) for o in outs) or mem:
            bad.append((fn, {k: hex(v) for k, v in keep.items()},
                        {o: (hex(r[o]), hex(getattr(cr, o))) for o in outs},
                        [hex(i) for i in mem[:4]]))
    return len(calls), bad


def test_whole(eng, c, state, address, function, command=0x0F):
    """A routine from the real state at its entry during `command`: every
    byte of the segment must match afterwards."""
    calls = capture_calls(eng, state, (address,), command=command, limit=1)
    bad = []
    for _, regs, image in calls:
        harness.restore(eng, image)
        keep = {k: regs[k] for k in ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp')}
        r = harness.call_near(eng, address, keep)
        c.load(image)
        cr = Regs(**keep)
        getattr(c.lib, function)(c.buf, ctypes.byref(cr))
        mem = compare(harness.snapshot(eng), c.image())
        if mem or (r['dx'] & 0xff00) != (cr.dx & 0xff00):
            bad.append((function, f"dh orig={r['dx'] >> 8:#x} c={cr.dx >> 8:#x}",
                        [hex(i) for i in mem[:6]]))
    return len(calls), bad


def test_generate_core(eng, c, state):
    """0x72fe..0x7325 from the real state at 0x72fe during command 0x0F:
    the move lists and every byte of the segment must match."""
    calls = capture_calls(eng, state, (0x72FE,), limit=1)
    bad = []
    for address, regs, image in calls:
        harness.restore(eng, image)
        keep = {k: regs[k] for k in ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp')}
        harness.call_near(eng, address, keep)
        c.load(image)
        cr = Regs(**keep)
        c.lib.eng_generate_legal_core(c.buf, ctypes.byref(cr))
        orig = harness.snapshot(eng)
        mem = compare(orig, c.image())
        if mem:
            bad.append((f'moves orig={word(orig, 0x640) // 2} c={word(c.image(), 0x640) // 2}',
                        [hex(i) for i in mem[:6]]))
    return len(calls), bad


def run(path_bin, path_lib):
    eng, states = collect_states(path_bin)
    states += collect_random_states(path_bin)
    states += collect_scripted_states(path_bin)
    c = CEngine(path_lib)
    import random
    rng = random.Random(7)
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
        for addr, fn in ((0x6A0B, 'eng_eval_setup'), (0x72F2, 'eng_generate_legal')):
            n6, bad6 = test_whole(eng, c, state, addr, fn)
            failures += bool(bad6)
            print(f'          {fn}: {n6} runs, {len(bad6)} mismatches'
                  + ''.join(f'\n            {b}' for b in bad6[:2]))
        calls5, bad5 = test_generate_core(eng, c, state)
        failures += bool(bad5)
        print(f'          generate: {calls5} runs, {len(bad5)} mismatches'
              + ''.join(f'\n            {b}' for b in bad5[:3]))
        calls4, bad4 = test_make_unmake(eng, c, state)
        failures += bool(bad4)
        print(f'          make/unmake: {calls4} calls, {len(bad4)} mismatches'
              + ''.join(f'\n            {b}' for b in bad4[:3]))
        if os.environ.get('ENGINE_QUICK'):
            continue
        calls3, bad3 = test_eval_terms(eng, c, state, rng)
        failures += bool(bad3)
        print(f'          eval terms: {calls3} calls, {len(bad3)} mismatches'
              + ''.join(f'\n            {b}' for b in bad3[:5]))
        calls2, bad2 = test_square_updates(eng, c, state)
        failures += bool(bad2)
        print(f'          square updates: {calls2} calls, {len(bad2)} mismatches'
              + ''.join(f'\n            {b}' for b in bad2[:5]))
        calls, bad = test_see(eng, c, state)
        failures += bool(bad)
        print(f'          see: {calls} calls, {len(bad)} mismatches'
              + ''.join(f'\n            si={a:#x} di={b:#x} dl={d:#x} orig={o:#06x} c={m:#06x}'
                        for a, b, d, o, m in bad[:5]))
    print('make/unmake move kinds:', MOVE_KINDS)
    return failures


if __name__ == '__main__':
    sys.exit(1 if run(sys.argv[1], sys.argv[2]) else 0)
