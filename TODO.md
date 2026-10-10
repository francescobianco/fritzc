# TODO — handoff

State at the end of the session of 2026-10-10. Read with
`docs/reverse-engineering.md` (UI) and `docs/engine-analysis.md` (engine).

## Where we are

### User interface (done for the start screen)
- `make start` opens the port; `make test` compares it with DOSBox captures.
- The start screen is **pixel-identical** to the original (307 200 pixels,
  4 configurations: original PAR, changed colours, German, flipped board).
- The square cursor follows the mouse (not yet compared with DOSBox).
- Machine-dependent values: free memory "105 kB" (`--free-kb`), hash
  size "128" (computed from a fixed DOS block size, see
  `src/engine/engine_memory.c`).

### Engine (FRITZ2.BIN)
- The module is loaded by the EXE into a memory block and called per command
  (code at `p+0x80`, arguments from `p+0x82`); 19 commands mapped.
- `tools/engine/harness.py` runs the **original** module in Unicorn;
  `make engine-demo` shows it playing.
- Translated to C and **verified identical** (`tests/engine/diff.py`, make
  `engine-test`): attack tables, SEE, hash key updates, incremental
  evaluation terms, make/unmake, evaluation set-up (piece-square tables),
  complete legal move generation (`0x72f2`).

## Work in progress (uncommitted before this handoff, now committed as WIP)

The search uses other segments through ES (hash table, opening book), so
the C model was changed from "engine segment only" to **1 MB of conventional
memory with the engine segment as a window**:
- `include/engine.h`: `Engine { uint8_t *mem; uint16_t segment; uint8_t
  ram[0x100000]; }`, `eng_bind()`, `eng_size()`, `eng_far_rd8/16`,
  `eng_far_wr8/16`; `src/engine/engine.c`.
- `tools/engine/harness.py`: `snapshot()` / `restore()` now work on 1 MB
  images limited to the "live ranges" (engine segment, heap blocks, BIOS
  data).
- `tests/engine/diff.py`: `CEngine` uses the new struct; `byte()` / `word()`
  read engine-segment offsets inside 1 MB images; `compare()` masks the C
  image to the live ranges.

**Not yet verified after this change**: the differential test became very
slow (no state processed after more than 10 minutes). First thing to do:
1. Profile `tests/engine/diff.py` (probably 1 MB copies per call: restore,
   CEngine.load/image, mask in compare, snapshots in `capture_calls`).
   Ideas: restore/compare only the engine segment unless a test needs the
   hash; keep the heap small in the harness (pass `largest_paragraphs` so
   the hash gets 64–128 KB); copy into the ctypes buffer with one memmove
   per live range.
2. Rerun `make engine-test QUICK=1`: everything must still be 0 mismatches
   (it was before the refactor, commit `c6783f4`).

## Next steps (engine search)

Call tree from command 0x04 (`0x2825` → `0x31d5`), sizes in instructions:
- `31d5` driver: `3040` timer init, `3c96` opening book probe (uses ES and
  the BIOS tick at `3d66` to choose: keep the harness tick fixed),
  `3baf`/`3bf9`/`3c20` play a move and update the game history (read).
- `3251` iterative deepening → `3314` root → `33ee` alpha-beta node (206)
  → `3695` move picker / quiescence (302) with generators `7af9` (428),
  `786d`, `7917`, `84d8`, `85b3`, `80dd`, `8133`, …
- Hash table: clear `88fd`, probe `8948`, store `8a60`, `8a98` (segment in
  `[0xF8]` and per ply `[bp+0x1742]`).
- Callbacks into the EXE during search (`lcall [0x86]`, `[0x8A]`) and the
  stop flags `p[0x9C]`.

Method that works: translate from the leaves up, keep the same offsets and
register outputs, record real calls with `capture_calls()` during the command
(now `command=0x04` with fixed depth: `p[0x9D]=1`, `p[0x9E]=depth`), replay
in emulator and C, require identical memory.

Then: wire the C engine into the UI (load the user's FRITZ2.BIN image into
`Engine.ram` like the EXE does and run the C routines on it), and compare
whole games at fixed depth with the original.

## Smaller open points
- K+B+N vs K branch of the evaluation set-up: translated, not exercised —
  add a scripted position.
- Menus F1–F6 and keyboard handling of the main loop `0000:7a39`.
- Square cursor tracking: compare with DOSBox by moving the mouse via XTest.
- The first commit message (`eb144ad`) is still in Italian; rewriting it
  would need a force push on `main`.

## Useful commands
```sh
make start                 # port window
make test                  # UI pixel regression (DOSBox captures)
make engine-demo           # original engine plays itself
make engine-test QUICK=1   # C engine vs original (fast mode)
tools/ghidra/decompile.sh SEG:OFF          # EXE functions
tools/ghidra/decompile-engine.sh OFF       # FRITZ2.BIN functions
python3 tools/engine/listing.py dos/fritz251/FRITZ2.BIN build/engine.lst ENTRIES...
```
Local dependencies live in `build/` (SDL3, Ghidra, JDK, Python venv): not in
git; see README and the Makefile.
