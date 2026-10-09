#!/usr/bin/env python3
"""Recursive-descent disassembly listing of FRITZ2.BIN (0x100-based).

Usage: listing.py FRITZ2.BIN OUT.lst [EXTRA_ENTRY ...]
Starts from the entry point, the command handlers (jump table at 0x2747)
and any extra entries (hex), follows direct jumps and calls, and writes
the reachable code in address order with labels (sub_XXXX / loc_XXXX).
Indirect calls and jumps are listed but not followed: pass their targets
as extra entries once known.
"""
import struct
import sys

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

ORG = 0x100
STOP = {'ret', 'retf', 'iret', 'jmp', 'ljmp'}


def main():
    code = open(sys.argv[1], 'rb').read()
    end = ORG + len(code)
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    table = struct.unpack_from('<19H', code, 0x2747 - ORG)
    entries = [0x2704, *table] + [int(a, 16) for a in sys.argv[3:]]
    subs = set(entries)
    labels = set()
    seen = {}
    work = list(entries)
    while work:
        addr = work.pop()
        while ORG <= addr < end and addr not in seen:
            insn = next(md.disasm(code[addr - ORG:addr - ORG + 16], addr), None)
            if insn is None:
                break
            seen[addr] = insn
            target = None
            if insn.op_str.startswith('0x') and (insn.mnemonic == 'call' or insn.mnemonic.startswith('j')):
                target = int(insn.op_str, 16)
                if insn.mnemonic == 'call':
                    subs.add(target)
                else:
                    labels.add(target)
                work.append(target)
            if insn.mnemonic in STOP:
                break
            addr += insn.size
    with open(sys.argv[2], 'w') as out:
        prev_end = None
        for addr in sorted(seen):
            insn = seen[addr]
            if prev_end is not None and addr != prev_end:
                out.write('    ...\n')
            if addr in subs:
                out.write(f'\nsub_{addr:04x}:\n')
            elif addr in labels:
                out.write(f'loc_{addr:04x}:\n')
            op = insn.op_str
            if op.startswith('0x') and (insn.mnemonic == 'call' or insn.mnemonic.startswith('j')):
                t = int(op, 16)
                op = f'sub_{t:04x}' if t in subs else f'loc_{t:04x}'
            out.write(f'    {addr:04x}  {insn.mnemonic:<6} {op}\n')
            prev_end = addr + insn.size
    print(f'{len(seen)} instructions, {len(subs)} subroutines')


if __name__ == '__main__':
    main()
