# Engine analysis (`FRITZ2.BIN`)

Legend as in `reverse-engineering.md`: **[verified]** / **[hypothesis]**.

## 1. The module

`FRITZ2.BIN` (35 877 bytes) is a separate binary module, written in
assembly: no compiler stack frames, near `ret` everywhere. It is a
`.COM`-style image linked at offset `0x100` (addresses below are in that
space; file offset = address − `0x100`). **[verified]**

- Offset `0x100`: `JMP 0x2704`, the entry point.
- `0x2704`: switches to the module's own stack (`SS = CS`, `SP = 0x2704`),
  saves the caller's registers, hooks a vector at `[0x100]`, then
  dispatches the command at `[0x80]`. **[verified]**
- Dispatcher at `0x262F + 0x100`: `bl = [0x80] + 1`, commands above `0x12`
  return `0xFF`; jump table at `0x2747`. **[verified]**

## 2. Interface on the FRITZ2.EXE side (segment `293b`)

- `293b:ae86 SetEngine(far p)`: `[7190] = p` (the module's 64 KB block,
  PSP-like header first), `[7194] = p + 0x103 + word[p + 0x101]`, i.e. the
  target of the `JMP` at `0x100`. **[verified]**
- `293b:aea8 CallEngine`: saves `bp, si, di, ds` and calls `[7194]` far.
  **[verified]**
- A command writes its code to `p + 0x80` and its arguments from
  `p + 0x82`. Results are read back from the same block. **[verified for
  command 2]**
- Loader `19e7:38c6`: opens `FRITZ2.BIN`, allocates `(size + 0x10F) / 16`
  paragraphs, reads the file at `block:0x100`, calls `SetEngine`, clears
  `p[0xF0…0xFC]`, installs the `int 15h` handler (`344d:0075`) and sends
  command `0xFF` (init). If `p[0x81] != 0` it shows message `0x48` and gives
  up. Then it opens the opening book. **[verified]**
- Segment `344d` (0x170 bytes) is not the engine: it is an `int 15h`
  handler for extended memory block moves (A20 through the keyboard
  controller, 32-bit protected-mode copy); `344d:0115` restores the vector.
  **[verified]**
- `p[0xFE]`: hash size exponent; the book line shows `64 << p[0xFE]` KB
  ("128" at start). **[verified]**

## 3. Commands

| Code | EXE wrapper | Calls in EXE | Engine handler | Meaning |
|---:|---|---:|---|---|
| `0x00` | `293b:afb7` | 1 | `0x2775` | new game (called by `0000:166a`) **[verified in the harness]** |
| `0x01` | `293b:afc8` | 1 | `0x277e` | ? |
| `0x02` | `293b:aeb5` | 13 | `0x27e8` | make a move: `p[0x82]` from, `p[0x83]` to (`8·rank + file − 9`), `p[0x84]` promotion via `DS:7198` **[verified for the encoding]** |
| `0x03` | `293b:afea` | 3 | `0x2812` | ? |
| `0x04` | `293b:af13` | 3 | `0x2825` | search (calls back into the EXE while thinking); the move found is left at `p[0x82]`, `p[0x83]` **[verified]** |
| `0x05` | `293b:af24` | 1 | `0x283e` | ? |
| `0x06` | `293b:affb` (shared) | 1 | `0x2865` | apply the time control: `293b:affb` sets `p[0x9D]` (0 time, 1 fixed depth `p[0x9E]`, 2 …) and `p[0x90]`, `p[0x94]` |
| `0x07` | `293b:afd9` | 3 | `0x286b` | ? |
| `0x08` | `293b:af35` | 1 | `0x2871` | ? |
| `0x09` | `293b:af46` | 1 | `0x2891` | ? |
| `0x0A` | `293b:af57` | 2 | `0x28a3` | play the move found by the search **[verified]** |
| `0x0B` | `293b:af68` | 3 | `0x28a9` | game status: 0 on, 1 repetition, 2 fifty-move rule (200 plies), 3 mate, 4 stalemate **[verified for 0 and 3]** |
| `0x0C` | `293b:af79` | 4 | `0x28e5` | ? |
| `0x0D` | `293b:af02` | 2 | `0x2906` | play a prepared move (`[0x50…0x54]`, requires `p[0x9C] & 4`); chosen instead of `0x04` by `19e7:7a24` **[hypothesis: book move]** |
| `0x0E` | `293b:b070` | 1 | `0x2940` | i-th legal move (`p[0x81]`) with its score in `p[0x64]` |
| `0x0F` | `293b:b08f` | 1 | `0x2991` | number of legal moves **[verified]** |
| `0x10` | `293b:b0a0` | 2 | `0x29a2` | ? |
| `0x11` | `293b:af98` | 2 | `0x29c1` | ? |
| `0xFF` | `293b:b0bf` | 1 | `0x29d8` | init: callbacks into the EXE at `p[0x86]` (`293b:b110`) and `p[0x8A]` (`293b:b105`), `p[0xFA]` = `[B0FA]`; sizes and allocates the hash table (§4) **[verified]** |

Command 2 returns 1 for an illegal move. **[verified]**

The square encoding of command 2 (`8·rank + file − 9`, rank-major) differs
from the UI board array (file-major): the engine has its own board
representation. **[verified for the encoding]**

## 4. Hash table sizing (command `0xFF`)

- `[B0FA]` = 1 only with the `/X` command-line switch (`19e7:3b28`), and is
  reset when the CPU check `293b:ad79` fails or an XMS driver answers
  `int 2Fh/4300h` (`293b:ae77`).
- With `p[0xFA]` set and a 286+ (`0x2ddc` ≥ 2): `n = (int 15h/88h KB) / 64`,
  `p[0xFE] = floor(log2 n)`, hash in extended memory.
- Otherwise: `b` = largest free DOS block (paragraphs). `b <= 0x1000` →
  refuse (`p[0x81] = 0xFF`, every command disabled). Else `b - 0x1800` if
  `b > 0x2800`, otherwise `0x1000`; `n = b >> 12`;
  `p[0xFE] = floor(log2 n)`; allocates `1 << (p[0xFE] + 12)` paragraphs.
- The reference DOSBox run shows 128 KB, so `p[0xFE] = 1` and the largest
  block was `0x3800…0x57FF` paragraphs. The port uses `0x4000`.
- Translated in `src/engine/engine_memory.c`. **[verified]**

## 5. Emulation harness

`tools/engine/harness.py` runs the original module in Unicorn (x86-16),
exactly as the EXE calls it, with the three services and the callbacks
emulated (`make engine-demo`). The tick counter `0000:046C` is set by the
harness, so runs are deterministic. Checked so far: 20 legal moves at the
start; 20, 19, 30, 0 legal moves and status 3 along f3 e5 g4 Qh4#; illegal
moves rejected; at depth 4 the engine plays 1.d4 d5 2.e3 e6 3.Nf3 Nf6.

The module is not a child process: the EXE loads it into a memory block,
calls it with a far call per command, and the module calls the EXE back
during long searches so that the screen and the keyboard stay alive.

## 6. Plan

1. Name each command from its callers in the EXE (new game, move, take back,
   search, level, hint, …) and from its handler.
2. Load `FRITZ2.BIN` into Ghidra as a raw x86-16 binary at `0x100` and map
   the board representation, move generation, evaluation and search.
3. Build a harness that runs the original module on chosen positions (in
   DOSBox via a small driver, or in a CPU emulator), to compare the C
   reimplementation command by command (IDEA.md §8.1).
