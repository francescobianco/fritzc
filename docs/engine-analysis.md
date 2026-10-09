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

## 6. Internal representation

All data lives in the module's own 64 KB segment (`DS = SS = CS`, stack
below `0x2704`). **[verified]**

- Board: 10×12 mailbox of **words** at `0x108`, a1 at `0x132`, ranks
  `0x14` bytes apart, border `0xFFFE`. UI square `s` (0..63, rank-major)
  maps to `0x132 + 2·(10·(s >> 3) + (s & 7))` (`0x2b63`); the table at
  `0x5F58` maps back.
- Piece codes (even): white pawn `0x10`, knight `0x12`, bishop `0x14`,
  rook `0x16`/`0x18`, queen `0x1A`, king `0x1C`/`0x1E`; black = white
  + `0x10`. The two rook and king codes share the same routines (probably
  unmoved / moved, i.e. castling rights). **[codes verified; meaning of the
  pairs is a hypothesis]**
- Parallel per-square arrays at `+0x100` (piece index ×2), `+0x200` (white
  attack words), `+0x300` (black attack words), `+0x400` (pawn pushes).
- Attack tables: each piece owns a bit; `0x42F0` holds the word mask per
  piece index. The per-piece routines (`0x45D4` table, `0x4380…0x45AA`)
  XOR the mask along the piece's moves, so the same call adds or removes
  it; sliders stop at the first non-empty square, included. `0x46b7`
  clears the tables, `0x46f8` rebuilds them from the board. Kept up to date
  incrementally during play: a rebuild from scratch gives the same bytes.
  **[verified]**
- `0x81a3`: in-check test, attack word of the other side on the king square
  (`[0x600]`).
- Legal move generation `0x72f2`: two pseudo-legal passes (`0x7408`,
  `0x79c1`) written as coroutines; for each move `0x7326` makes it
  (`0x8692`), rejects it if the own king is attacked, stores it in the lists
  `0x2218` (from), `0x22B8` (to), `0x2358` (score), `0x23F8` (flags), count
  ×2 in `[0x640]`, and unmakes it (`0x719e`). The stored score comes from
  `0x8692`. **[verified for the structure]**
- `0x8692` is not make-move but a **static exchange evaluation**: balance of
  attackers and defenders of the target (x-ray pieces included, through the
  direction tables at `0x82CC` / `0x8404` and the piece ray table `0x81C0`)
  and the values of the cheapest pieces on each side (`0x8662`); returns
  the gain estimate in AH. **[verified]**

- Hash key: two words at `0x69C`; the `0x45F4` variants of the attack
  routines first XOR the piece's random words for the square (one table per
  piece type, high words `0x9C` bytes after the low ones). **[verified]**
- Incremental evaluation: make-move removes the piece from the from-square
  (`0x632A` table) and adds it on the to-square (`0x634A`), accumulating in
  AX. The terms include piece-square tables, pawn structure (passed,
  isolated, doubled, chains, pawns attacked by bishops), knights on c3/c6
  blocking the c-pawn in Queen's Gambit structures, bishops blocked by their
  centre pawns, rooks on the seventh with the king on the eighth, file masks
  at `0x646…0x64F`, and material sets / counters at `0x67C…0x69B`.
  Promotions take the piece from CL (`0x80` queen, `0x81` rook, `0xC0`
  bishop, `0xC1` knight). **[verified]**

- Make move `0x6f81`: restores the ply's hash key, moves the piece
  (evaluation terms out / in, with the negamax flip `-ax - 0x10`, hash and
  attacks), then by CL: capture (material value `0x710E`, counters via
  `0x714E` / `0x716E`, score clamp in low-material endings when a counter
  reaches 0), en passant (victim square per ply at `0xE7A`), castling
  (rook moved with hash and attacks only, `+0x18 - 2·[0x650]`). Unmake
  `0x719e` restores board, lists, attacks and counters; hash and
  evaluation are per ply. **[verified]**

- Move order (the search depends on it): captures first, victims in the
  opponent's piece-list order (most valuable first, king excluded) and, for
  each victim, attackers from the highest piece bit down (pawns first);
  then en passant (left pawn, right pawn); then castling (short, long; only
  the square next to the king is tested for attacks, the rest is left to the
  legality test); then quiet moves scanning the empty squares in the order
  of the linked list at `0x616C` (starting at `0x174`), for each square the
  pieces able to go there from the highest bit down, pawn pushes included.
  Promotions are tried as queen, rook, bishop, knight (`0x7326` steps the
  flag bits: `inc cl`, `add cl, 0x3F`). Each legal move gets the score
  "evaluation after the move − SEE gain × 256". **[verified]**

## 7. C reconstruction and differential tests

`src/engine/` keeps the 64 KB segment as `Engine.mem` with the same offsets,
so every C routine can be checked against the original on the same memory
image: `tests/engine/diff.py` (`make engine-test`) collects states from a
game played by the original engine in the harness, runs the routine in the
emulator and in C (`libfritzengine.so` through ctypes) and requires the two
64 KB images to be identical, apart from the stack area. States come from a
game the original plays at depth 3 and from three random legal games of 60
plies (fixed seed) and three scripted games that reach castling, en
passant and promotions. Make/unmake are tested with the exact registers
and memory the original generator passes them, recorded by a hook while it
runs (`capture_calls`). `ENGINE_QUICK=1` skips the slow exhaustive
evaluation-term test.

The C engine needs the module's data tables (piece values, direction
tables, …). As with the interface artwork, they are not copied into the
repository: the C engine will load the user's `FRITZ2.BIN` image into its
segment and run its own code on it.

| Original | C | Status |
|---|---|---|
| `0x46b7`, `0x46f8`, `0x4380…0x45AA` | `eng_attacks_clear`, `eng_attacks_build`, `eng_piece_attacks` | identical on 192 states |
| `0x8692` | `eng_see` | identical: 717 444 calls on 192 states |
| `0x45F4` dispatch (hash + attacks), `0x4646`, `0x4624` | `eng_piece_toggle`, `eng_slider_update`, `eng_square_update` | identical: 30 270 calls |
| `0x632A` / `0x634A` dispatch, `0x637C…0x6A06` | `eng_eval_remove`, `eng_eval_add` | identical: 116 286 calls (AX, BP, CX, memory) |
| `0x72fe…0x7325` with `0x7408` (captures), `0x74a8` (en passant), `0x79c1` (castling + quiet), `0x7326` (legality + scoring) | `eng_generate_legal_core` | identical: 230 positions, whole segment byte for byte (lists, scores, flags, scratch) |
| `0x6f81` make move, `0x719e` unmake move | `eng_make_move`, `eng_unmake_move` | identical: 14 460 calls captured from the original move generator (807 captures, 20 castlings, 4 en passant, 32 promotions) |
| `0x29d8` hash sizing | `engine_hash_exponent` | translated (§4) |

## 8. Plan

1. Name each command from its callers in the EXE (new game, move, take back,
   search, level, hint, …) and from its handler.
2. Load `FRITZ2.BIN` into Ghidra as a raw x86-16 binary at `0x100` and map
   the board representation, move generation, evaluation and search.
3. Build a harness that runs the original module on chosen positions (in
   DOSBox via a small driver, or in a CPU emulator), to compare the C
   reimplementation command by command (IDEA.md §8.1).
