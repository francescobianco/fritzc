# Reverse engineering notes

Analysis of the distribution in `dos/fritz251/` (not part of the repository,
see `.gitignore` and IDEA.md §14). SHA-256 hashes of every file are in
`reference/fritz251.sha256`.

Legend: **[verified]** = confirmed by direct evidence;
**[hypothesis]** = still to be confirmed.

Addresses are `SEG:OFF` in the unpacked image `build/re/FRITZ2.UNP.EXE`
(segments relative to the load address; DGROUP = `3464`). Ghidra loads the
image at segment `1000`, so `293b:9e44` appears there as `393b:9e44`.

## 1. Version identification

The distribution is **ChessBase Fritz 2.51** for MS-DOS. **[verified]**

Evidence:

1. **"About Fritz" box at runtime.** Running `FRITZ2.EXE` in DOSBox,
   `Fritz=F1 → About Fritz` shows `ChessBase Fritz 2.51` and the credits
   *Mathias Feist, Frans Morsch and Matthias Wüllenweber*.
2. **String in the binary.** In the unpacked executable, at image offset
   `0x3e0e9`, there is the length-prefixed string (Pascal style, `0x14` =
   20 bytes) `ChessBase Fritz 2.51`, followed by the authors' names. It is
   the string read by the About box, not a label added afterwards.
3. **Consistent distribution.** All program files share the timestamp
   `1993-08-01 05:34:56`, typical of a release mastered in one go.
   `FRITZ2.DAT` is the exception (`1993-08-01 11:45:06`, same day). The books
   in `BOOKS/` date from 23–25 Nov 1992, so they predate the program and were
   probably shared with earlier ChessBase products.
4. **File names.** The `FRITZ2.*` prefix stands for the product "Fritz 2";
   2.51 is the revision. The directory name `fritz251` is therefore
   consistent. The program does not depend on the directory name: it reads
   its files from the current directory.

The only file modified after release is **`FRITZ2.PAR`** (timestamp
`2001-02-17`). It holds the user settings, which Fritz rewrites. It contains
paths such as `C:\TEMP` and `C:FRITZ2.CBF`, so this copy comes from a used
installation, not from the original floppies. With this file present the
program skips the language and graphics card prompts. To reproduce a clean
first start, remove it (from the working copy, never from the original).

No public list of reference hashes was found to compare against a known
copy. The local hashes are recorded anyway.

## 2. File inventory

| File | Size | Purpose |
|---|---:|---|
| `FRITZ2.EXE` | 244 858 | Main executable (MZ, EXEPACK-compressed) **[verified]** |
| `FRITZ2.BIN` | 35 877 | Binary module loaded at runtime. Starts with `JMP` and sets up its own stack. Possibly the chess engine, separate from the UI **[hypothesis]** |
| `FRITZ2.DAT` | 888 406 | Unsigned 8-bit audio samples (around `0x80`), blocks tagged `SOL` **[hypothesis: sounds/speech]** |
| `FRITZ2.IMG` | 1 512 | Compressed bitmap, probably the "fritz 2" logo **[hypothesis]** |
| `FRITZ2.PAR` | 759 | User settings (language, paths, database) **[verified]** |
| `FENGLISH.LAN`, `FDEUTSCH.LAN` | 10 445 / 11 334 | UI strings (menus, messages), English and German **[verified]** |
| `FENGLISH.HLP`, `FDEUTSCH.HLP` | 8 871 / 9 006 | Online help texts **[verified]** |
| `*.CBF`, `*.CBI`, `*.CKO`, `*.CPO`, `*.CPA` | — | ChessBase-format databases (games, index, keys, positions). Sets: `FRITZ2`, `C45_C47`, `TAKTIK` **[hypothesis on each extension's role]** |
| `FRITZ2.FBK`, `BOOKS/*.FBK` | — | Fritz opening books. `FRITZ2.FBK` is identical to `BOOKS/A00-E99.FBK` **[verified: same hash]** |
| `BOOKS/A00-E99.CKB`, `.CPB` | — | Opening book in ChessBase format **[hypothesis]** |

Other file names referenced by the executable: `MAINBASE.CBF`, `EXPORT.CBF`,
`EXPORT.DOC`, `IMPORT.CKO`, `DIAGRAMM.DOC`, plus the patterns `*.CFG`,
`*.VFG`, `*.EFG`, `*.HFG` (probably colour settings for VGA, EGA and
Hercules).

## 3. Executable format

- **Format:** MZ (DOS real mode, 16-bit), compressed with **Microsoft
  EXEPACK**. The header has zero relocations and the unpacking stub sits at
  the end, with the message `Packed file is corrupt`. **[verified]**
- **Unpacking:** `tools/inspect/unexepack.py` produces a standard 254 880-byte
  MZ with 2 468 relocations, all pointing to valid targets. Original entry
  point `0000:8C0C`, stack `3F7D:4000`. **[verified]**
- **Languages and toolchain.** The program is made of three parts:
  - **UI and program logic: C, Microsoft C.** Runtime banner `MS Run-Time
    Library - Copyright (c) 1988, Microsoft Corp`, `<<NMSG>>`/`R6xxx` runtime
    messages, MSC startup code at the entry point (DOS version check,
    DGROUP setup). The 1988 banner points to MSC 5.1 or QuickC 2.x
    (MSC 6.0 says 1990). Segments `0000`, `0a1f`, `19e7` show compiler-made
    `push bp / mov bp,sp` frames and far cdecl calls (caller pops).
    No DOS extender. **[verified; exact compiler version open]**
  - **Graphics library: hand-written assembly** (segment `293b`, Pascal
    convention, per-card drivers, see §6). No third-party framework (no
    Borland BGI, no MSC `graph.lib`). Probably ChessBase in-house code shared
    with the ChessBase DOS program: the EXE contains `ChessBase 2.4`,
    "Type 'EXIT' to return to ChessBase" and uses the ChessBase database
    formats. **[verified for the code style; origin is a hypothesis]**
  - **`FRITZ2.BIN`: assembly.** No compiler frames at all (zero
    `push bp/mov bp,sp`, 326 near `ret`), and at `0x262f` a dispatcher on a
    command code (0–0x12) through a 19-entry jump table: the profile of an
    engine written in assembler and driven by numbered commands.
    **[hypothesis: to be confirmed by tracing how the EXE calls it]**
  - Unexplained: the data contains `p:nulf.PAS` preceded by `BW`.
- **Segments referenced by relocations:** `0000`, `0A1F`, `19E7`, `293B`,
  `344D`, `3464`. Multiple code segments (medium or large model).
  **[verified for the segments; model to be confirmed]**
- **Graphics:** the program's own library in segment `293b` (see §6), with
  drivers for VGA, EGA, CGA and Hercules (direct CRTC programming on ports
  `3B4h/3B8h/3BFh`). Command-line switches include `/X`, `/EMS`, `/VGA`, …
  **[verified]**

## 4. Runtime environment and video mode

- Runs in DOSBox 0.74-3 (`machine=svga_s3`, `cycles=20000`).
  Configuration: `reference/dosbox.conf`. **[verified]**
- Supported graphics cards: VGA, EGA, Hercules, CGA.
- With VGA the mode is **640×480, 16 colours** (standard EGA/VGA palette,
  VGA mode 12h). The main screen uses only 4 colours: black, white, dark blue
  and light grey. Pixels are square, so no aspect correction is needed.
  **[verified with native DOSBox screenshots]**

## 5. Method

The port is built by **translating the binary's code and data into C**,
not by imitating screenshots. Every C function cites the `SEG:OFF` address
of the original routine.

DOSBox has two roles:

1. **Patch experiments.** Modify `FRITZ2.UNP.EXE` with
   `tools/compare/patch.py`, run it with `FRITZ_EXE=… dosbox-run.sh`, and
   check whether the observed effect confirms the hypothesis. Every
   experiment is recorded in the log (§7).
2. **Test reference.** Native screenshots are compared with the frame
   produced by the port (`fritz --dump`).

The unpacked EXE behaves like the original: the start screen is identical
except for the free-memory indicator ("105 kB" vs "101 kB"), which depends
on the file size.

## 6. Graphics library (segment `293b`)

The program's own library, not Microsoft C's. Far functions with Pascal
calling convention (`retf N`); per-card drivers dispatched through the tables
at `DS:6FA1…6FDD`, indexed by `[DS:6F58]`. **[verified]**

- **720×348 logical coordinates.** The UI works in Hercules geometry and
  converts with `293b:aa5f` (x) and `293b:aa75` (y):
  `x' = (x·[6F48] + [6F4C]) / [6F4A]` and `y' = (y·[6F50] + [6F54]) / [6F52]`.
  The addition does not propagate the carry into the high word. The
  parameters are set by `293b:aa8b`, called from `0000:814b`. **[verified]**
- **Per-card parameters.** `0000:7eaf` copies them from `DS:7478`.
  For VGA (`DS:7478`) they are `16/18 +9` and `48/35 +17`, so
  720×348 → 640×477: screen rows 477–479 stay black. For EGA (`DS:74AE`)
  the scale is `16/18 +9` and `1/1`, i.e. 640×348. **[verified]**
- **`293b:9e44` FillPattern(x, y, w, h, colour, pattern, rop).** Device
  coordinates. The row mask is `pattern[y & 7]` and bit 7 is the pixel with
  `x & 7 == 0`. Zero bits leave the pixel unchanged. `rop` is the VGA
  Function Select value (`0` copy, `08h` AND, `10h` OR, `18h` XOR). The VGA
  driver is `293b:9ec4` → `293b:9b2c` (write mode 0 with set/reset).
  Translated in `src/gui/gfx.c`. **[verified]**
- **Lines and rectangles.** `293b:9ab2` HLine(x, y, w, colour) and
  `293b:9cd4` VLine(x, y, h, colour) always copy (rop 0).
  `293b:95db` DrawRect(x, y, w, h, colour, rop) draws left, top, right, bottom
  in this order, so with XOR the corners are inverted twice. `293b:aad3`
  scales line thickness: `(v·[6F56] + 1) / [6F4E]` unless the two are
  equal (VGA: 1 and 1). **[verified]**
- **Driver selection.** The ninth `SetScale` argument goes to `[6F58]`
  (6 is mapped to 0). VGA and EGA use driver 0 of each 3-entry dispatch
  table (`DS:6FA1` address, `DS:6FB9` pattern fill, `DS:6FBF` row,
  `DS:6FC5` column, …). **[verified]**
- **Screen layout.** `0000:7eaf` (VGA) also copies per-card layout values:
  `DS:0B8E…0B9B` ← `DS:748A` (3, 4, 5, board x 16, board y 40,
  square 40×40), `DS:0B9C…` ← `DS:7498`, `DS:0BAA…` ← `DS:73CC`
  (menu bar height 14), `DS:0BB0…` ← `DS:74A6`. Translated in
  `src/gui/layout.c`. **[verified]**
- **Text.** `293b:9fb1` DrawTextP(x, y, font, pstring, colour, mode)
  (length-prefixed string) and `293b:a044` DrawText(…) (zero-terminated).
  Fixed 8-pixel advance. VGA glyph driver `293b:a0ce`: each glyph row is
  shifted by `x & 7` and used as the bit mask, so zero bits are
  transparent. `mode` (`[715A]`): bit 0 bold (`b | b>>1`), bit 1 underline
  (last row full), bit 3 halftone (`& 0xAA/0x55` by row parity), bit 2
  halftone every third row, bit 4 inverse. **[verified for mode 0]**
- **Fonts.** Table `DS:714F` → 4-byte descriptors (data offset in segment
  `293b`, rows, bytes per glyph): font 0 8×14 at `293b:2D6A`, font 1 8×8 at
  `293b:3B6A`, font 2 8×14 at `293b:436A`, "font" 3 at `293b:516A` (14-byte
  graphic tiles: panel arrows and more, probably the pieces). Code page 437
  with ChessBase figurines at `0xB0…`. The port reads them from the user's
  `FRITZ2.EXE` (`src/assets/exe.c` unpacks it in memory). **[verified]**
- **Patterns** `DS:7023` → eight 8×8 masks at `DS:7033…706A`; number 1
  (`88 22…`) is the desktop background. **[verified by patch]**

## 7. Settings (`FRITZ2.PAR`)

Loaded by `19e7:4294` (`LoadSettings(reload)`). With `reload == 0` it first
sets the defaults, then opens the file (`19e7:3ee4` builds the path) and
reads a `0x212`-byte record with `393b:20a3`. The record is applied only if
`word[0] == 200` and `word[2]` equals the id of the current graphics card
descriptor. **[verified]**

Graphics card descriptors: 8 bytes each at `DS:0E10`, selected by
`0a1f:c4e0` into `[AFAE]`: `(flags, id, ?, default colour)`. VGA is
`DS:0E28 = (1, 18, 3, 15)`; `FRITZ2.PAR` has id 18. **[verified]**

Known record fields (offsets in the file):

| Offset | Size | Destination | Meaning |
|---|---:|---|---|
| `0x000` | 2 | — | magic `200` |
| `0x002` | 2 | — | card descriptor id |
| `0x004` | 16 | `DS:AD5D` | language file name (`FENGLISH.LAN`) |
| `0x190` | 2 | `[AFB8]` | ? |
| `0x192` | 2 | `[B0C2]` | ? |
| `0x194` | 2 | `[AD6E]` | background / menu bar colour (default: descriptor colour) |
| `0x196` | 2 | `[B0C6]` | colour (default 0) |
| `0x198` | 2 | `[B112]` | board frame, second ring (default: descriptor colour) |
| `0x19A` | 2 | `[AC30]` | board frame (default 0) |
| `0x19C` | 56 | `DS:0F4C…0F83` | colour table; entry 1 (`[0F4E]`) is the desktop hatching and the panel shadows (default `DS:0F4C`: 0..15, 10, 11, 12, 14, 15, 15, 0, 15, 0, 15, 0, 15) |

Translated in `src/game/settings.c`.

## 8. Language files (`*.LAN`)

Read block by block by `19e7:b0c4`. Header: word directory offset (`0x42`),
word `0x4D2`, word block count (36). Directory: one dword file offset per
block. Block: word header size (10), word `0x4D2`, word block number, word
string count, word data size, then one word offset per string (relative to
the string data) and the zero-terminated strings, copied into a 12 KB pool
at `DS:7AD8`. `19e7:2354` loads block `0x1A` (106 strings) into the pointer
table `DS:77AE`, plus blocks `0x17`, `0x18`, `0x19`. String `0x1A/0x1C` is
the menu bar. **[verified]** Translated in `src/game/language.c`.

UI text helpers: `0a1f:67c3(n, colour, y, column)` takes message `n`
(`19e7:22ac`, copied to a Pascal string by `293b:1dee`) and calls
`0a1f:65bb`, which clears the area with `[AD6E]` (`0a1f:64b5`: 8 px per
character, `[0BAC]` rows) and draws the text in `[B0C6]` at
`x = 8·column`, `y = sy(y)`, inverted when `colour == [B0C6]`.
Translated in `src/gui/ui_text.c`. **[verified]**

## 9. Experiment log

| # | Hypothesis | Patch | Result |
|---|---|---|---|
| 1 | The unpacked EXE is equivalent to the original | none (`FRITZ_EXE=FRITZ2.UNP.EXE`) | Confirmed: identical screen except the memory indicator |
| 2 | `DS:703B` is the desktop pattern | `DS:703b=ff00ff00ff00ff00` | Confirmed: the background turns into horizontal blue and white stripes |
| 3 | `FRITZ2.PAR` offsets `0x194` and `0x19E` hold `[AD6E]` and `[0F4E]` | PAR: `0x194=14`, `0x19e=4` (`FRITZ_FILES`) | Confirmed: yellow background and menu bar, red hatching and panel shadows; the port with the same PAR matches the capture in every comparable zone |
| 4 | `FRITZ2.PAR` offset `0x004` names the language file | PAR: `0x004="FDEUTSCH.LAN"` | Confirmed: German menu bar; the port matches it pixel for pixel |

## 10. Reconstructed functions

| Original | C | Status |
|---|---|---|
| `293b:aa5f`, `293b:aa75`, `293b:aa8b` | `gfx_scale_x/y`, `gfx_set_scale` | complete |
| `293b:9e44` (VGA driver only) | `gfx_fill_pattern` | complete for VGA |
| `293b:9ab2`, `293b:9cd4`, `293b:95db`, `293b:aad3` | `gfx_hline`, `gfx_vline`, `gfx_draw_rect`, `gfx_scale_line` | complete for VGA |
| `0000:2512` up to the call of `0000:05f3` | `desktop_draw` | complete; the calls after `05f3` (board, panels, menu) are missing |
| `0000:05f3` up to `0000:0820` | `desktop_frame` | outer frame and message box; board coordinates (`0000:0413`) and box text missing |
| `293b:9fb1`, `293b:a044`, `293b:a0ce`, fonts `DS:714F` | `gfx_draw_text_p`, `gfx_draw_text`, `fonts_load` | complete for VGA |
| `19e7:b0c4`, `19e7:2354` | `language_load`, `language_string` | reading; the in-memory pool is not emulated |
| `0a1f:67c3`, `0a1f:65bb`, `0a1f:64b5` | `ui_message`, `ui_text_line` | complete |
| `0000:2037` | `arrow_buttons` (desktop.c) | complete; drawn only with a mouse (`[AFB2]`, set by `0000:7d99` via int 33h) |
| `0a1f:7d72` | `board_frame_draw` | complete |
| `0000:2512` final menu bar and line (`0a1f:67c3(0x1C…)`, `293b:9ab2`) | `desktop_draw` | complete |
| `19e7:4294` (colours and language only) | `settings_defaults`, `settings_load_par` | partial |

Check (`tools/compare/diff.py`): `fritz --par … --dump` matches the DOSBox
screenshot pixel for pixel wherever the desktop is visible, with the
original PAR and with the modified one of experiment 3, including the
outer frame and the message box. `tests/gui/screens.sh` runs these checks.

## 11. Tools

| Tool | Use |
|---|---|
| `tools/inspect/unexepack.py IN OUT` | Unpacks an EXEPACK executable |
| `tools/inspect/xkeys.py KEYS…` | Sends keys via XTest to the focused window (DOSBox) |
| `tools/inspect/dis.py EXE SEG:OFF [N]` | Disassembles N instructions at an address |
| `tools/compare/patch.py IN OUT SEG:OFF=HEX…` | Patches the unpacked EXE (for experiments) |
| `tools/compare/dosbox-run.sh OUTDIR SECS KEYS…` | Runs Fritz on a temporary copy and collects screenshots (`ctrl+F5`); `FRITZ_EXE` replaces the executable, `FRITZ_FILES="NAME=path …"` other files |
| `tools/compare/diff.py NATIVE.bmp CAPTURE.png [REGIONS…]` | Pixel comparison between the port and a capture |
| `tools/ghidra/decompile.sh SEG:OFF…` | Imports the unpacked EXE into Ghidra (headless), applies `tools/ghidra/symbols.txt` and prints decompiled functions |
| `tests/gui/screens.sh` | Regression test: port vs DOSBox captures in the reconstructed regions |

Example, capturing the About box:

```sh
tools/compare/dosbox-run.sh build/captures 15 F1 Up Return sleep:1.5 ctrl+F5
```

Note: under Wayland DOSBox runs through XWayland. XTest works as long as
the DOSBox window has focus.

## 12. Next steps

1. Squares and pieces: `0a1f:78ac` → `0a1f:74ac` (empty square) and
   `0a1f:7632` (piece); then the remaining panels of `0000:2512`.
2. Decode the remaining `FRITZ2.PAR` fields.
3. Annotate the Ghidra project with the names of the functions identified so far.
4. Check whether `FRITZ2.BIN` is the engine: trace how `FRITZ2.EXE` loads it
   and its entry point (`JMP 0x2604`).
5. Decode `FRITZ2.IMG` (logo) and locate the piece bitmaps and the 8×14
   fonts in the EXE (they are visible in the uncompressed data).
6. Catalogue all menus (F1–F6) with `dosbox-run.sh` (IDEA.md §6.1).
