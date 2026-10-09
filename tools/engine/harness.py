#!/usr/bin/env python3
"""Runs the original FRITZ2.BIN engine module in a 16-bit CPU emulator.

The module is called exactly as FRITZ2.EXE does (see docs/engine-analysis.md):
the 64 KB block p = SEG:0000 holds the image at 0x100, the command code at
p+0x80 and the arguments from p+0x82; the entry point is the JMP at 0x100.
The few services the module needs are emulated here:

  int 21h 48h/49h  DOS memory allocation (bump allocator)
  int 15h 88h      extended memory size (0 by default)
  0000:046C        BIOS tick counter, set by the caller (deterministic)
  p[0x86], p[0x8A] callbacks into the EXE, replaced by RETF stubs

Requires unicorn (make engine-venv). Usage as a module:

    eng = Engine('dos/fritz251/FRITZ2.BIN')
    eng.command(0xFF)          # init
    eng.command(0x00)          # new game
    print(eng.command(0x0F))   # legal move count (AL)
"""
import struct
import sys

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CS,
                               UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_FLAGS,
                               UC_X86_REG_IP, UC_X86_REG_SP, UC_X86_REG_SS)

SEG = 0x1000            # engine block segment (p)
STUB_SEG = 0x0050       # RETF / HLT stubs
STACK_SEG = 0x9E00       # above the heap, clear of the engine block
HEAP_START = 0x2000     # first paragraph handed out by int 21h/48h
HEAP_END = 0x9E00       # conventional memory top (stack above)
CF = 1


class Engine:
    def __init__(self, path, largest_paragraphs=None, extended_kb=0):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_16)
        self.uc.mem_map(0, 0x100000)
        image = open(path, 'rb').read()
        self.uc.mem_write(SEG * 16 + 0x100, image)
        # IVT: every vector points to an IRET stub; interrupts are hooked anyway.
        self.uc.mem_write(STUB_SEG * 16, bytes([0xCB, 0xF4, 0xCF]))  # retf, hlt, iret
        for v in range(256):
            self.uc.mem_write(v * 4, struct.pack('<HH', 2, STUB_SEG))
        self.extended_kb = extended_kb
        self.heap = HEAP_START
        self.largest = largest_paragraphs
        self.ticks = 0
        self.callbacks = 0
        self.uc.hook_add(UC_HOOK_INTR, self._interrupt)
        self.uc.hook_add(UC_HOOK_CODE, self._callback, begin=STUB_SEG * 16,
                         end=STUB_SEG * 16)
        # Callbacks into the EXE (p[0x86], p[0x8A]) -> RETF stub.
        self.poke16(0x86, 0)
        self.poke16(0x88, STUB_SEG)
        self.poke16(0x8A, 0)
        self.poke16(0x8C, STUB_SEG)

    # --- memory helpers (offsets in the engine block) ---
    def peek(self, off, n=1):
        return bytes(self.uc.mem_read(SEG * 16 + off, n))

    def peek16(self, off):
        return struct.unpack('<H', self.peek(off, 2))[0]

    def poke(self, off, data):
        self.uc.mem_write(SEG * 16 + off, bytes(data))

    def poke16(self, off, value):
        self.poke(off, struct.pack('<H', value & 0xFFFF))

    def set_ticks(self, ticks):
        self.uc.mem_write(0x46C, struct.pack('<I', ticks & 0xFFFFFFFF))

    # --- emulated services ---
    def _callback(self, uc, address, size, user):
        self.callbacks += 1

    def _set_cf(self, on):
        flags = self.uc.reg_read(UC_X86_REG_FLAGS)
        self.uc.reg_write(UC_X86_REG_FLAGS, flags | CF if on else flags & ~CF)

    def _interrupt(self, uc, intno, user):
        ax = uc.reg_read(UC_X86_REG_AX)
        ah = ax >> 8
        if intno == 0x21 and ah == 0x48:
            want = uc.reg_read(UC_X86_REG_BX)
            free = (self.largest if self.largest is not None else HEAP_END - self.heap)
            if want > free or self.heap + want > HEAP_END:
                uc.reg_write(UC_X86_REG_BX, free)
                uc.reg_write(UC_X86_REG_AX, 8)
                self._iret(carry=True)
            else:
                uc.reg_write(UC_X86_REG_AX, self.heap)
                self.heap += want
                self._iret(carry=False)
        elif intno == 0x21 and ah == 0x49:
            self._iret(carry=False)
        elif intno == 0x15 and ah == 0x88:
            uc.reg_write(UC_X86_REG_AX, self.extended_kb)
            self._iret(carry=False)
        else:
            raise RuntimeError(f'unhandled int {intno:#x} ax={ax:#06x}')

    def _iret(self, carry):
        # The hook runs instead of the interrupt: emulate IRET's effect on CF
        # by editing the caller's FLAGS (unicorn does not push for us).
        self._set_cf(carry)

    # --- calling the module ---
    def command(self, code, args=b'', max_instructions=200_000_000):
        """Sends a command; returns AL as stored by the module at p+0x81."""
        self.poke(0x80, bytes([code & 0xFF]))
        if args:
            self.poke(0x81, args)
        # Far call from STUB_SEG:0001 (HLT after return).
        sp = 0xFFF0
        self.uc.mem_write(STACK_SEG * 16 + sp - 4, struct.pack('<HH', 1, STUB_SEG))
        self.uc.reg_write(UC_X86_REG_SS, STACK_SEG)
        self.uc.reg_write(UC_X86_REG_SP, sp - 4)
        self.uc.reg_write(UC_X86_REG_DS, 0x0000)
        self.uc.reg_write(UC_X86_REG_ES, 0x0000)
        self.uc.reg_write(UC_X86_REG_CS, SEG)
        self.uc.reg_write(UC_X86_REG_IP, 0x100)
        start = SEG * 16 + 0x100
        try:
            self.uc.emu_start(start, STUB_SEG * 16 + 1, count=max_instructions)
        except UcError as e:
            ip = self.uc.reg_read(UC_X86_REG_IP)
            cs = self.uc.reg_read(UC_X86_REG_CS)
            raise RuntimeError(f'emulation stopped at {cs:04x}:{ip:04x}: {e}') from e
        return self.peek(0x81)[0]


def square(name):
    """'e2' -> UI square index used by command 2 (8*rank + file - 9)."""
    return 8 * int(name[1]) + (ord(name[0]) - ord('a') + 1) - 9


def square_name(index):
    return 'abcdefgh'[index % 8] + str(index // 8 + 1)


def main():
    """Demo: the original engine plays a few moves against itself."""
    path = sys.argv[1] if len(sys.argv) > 1 else 'dos/fritz251/FRITZ2.BIN'
    depth = int(sys.argv[2]) if len(sys.argv) > 2 else 4
    plies = int(sys.argv[3]) if len(sys.argv) > 3 else 10
    eng = Engine(path)
    eng.command(0xFF)                   # init
    eng.command(0x00)                   # new game
    eng.poke(0x9D, [1])                 # fixed depth, as 293b:affb does
    eng.poke(0x9E, [depth])
    print(f'legal moves at start: {eng.command(0x0F)}')
    for ply in range(plies):
        eng.command(0x04)               # search
        frm, to = eng.peek(0x82, 2)
        eng.command(0x0A)               # play the move found
        status = eng.command(0x0B)
        print(f'{ply // 2 + 1}{"." if ply % 2 == 0 else "..."} '
              f'{square_name(frm)}{square_name(to)}  legal={eng.command(0x0F)} '
              f'status={status}')
        if status:
            break


if __name__ == '__main__':
    main()
