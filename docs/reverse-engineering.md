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
- **Compiler:** Microsoft C (`MS Run-Time Library - Copyright (c) 1988,
  Microsoft Corp`, MSC runtime `R6xxx` messages). No DOS extender.
  **[verified]**
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
- **Patterns** `DS:7023` → eight 8×8 masks at `DS:7033…706A`; number 1
  (`88 22…`) is the desktop background. **[verified by patch]**

## 7. Experiment log

| # | Hypothesis | Patch | Result |
|---|---|---|---|
| 1 | The unpacked EXE is equivalent to the original | none (`FRITZ_EXE=FRITZ2.UNP.EXE`) | Confirmed: identical screen except the memory indicator |
| 2 | `DS:703B` is the desktop pattern | `DS:703b=ff00ff00ff00ff00` | Confirmed: the background turns into horizontal blue and white stripes |

## 8. Reconstructed functions

| Original | C | Status |
|---|---|---|
| `293b:aa5f`, `293b:aa75`, `293b:aa8b` | `gfx_scale_x/y`, `gfx_set_scale` | complete |
| `293b:9e44` (VGA driver only) | `gfx_fill_pattern` | complete for VGA |
| `0000:2512` up to `0000:259d` | `desktop_draw` | partial: colours `[AD6E]`, `[0F4E]`, `[B0C6]` taken from observation, origin to be reconstructed |

Check: `fritz --dump` matches the DOSBox screenshot pixel for pixel
wherever the desktop is visible.

## 9. Tools

| Tool | Use |
|---|---|
| `tools/inspect/unexepack.py IN OUT` | Unpacks an EXEPACK executable |
| `tools/inspect/xkeys.py KEYS…` | Sends keys via XTest to the focused window (DOSBox) |
| `tools/inspect/dis.py EXE SEG:OFF [N]` | Disassembles N instructions at an address |
| `tools/compare/patch.py IN OUT SEG:OFF=HEX…` | Patches the unpacked EXE (for experiments) |
| `tools/compare/dosbox-run.sh OUTDIR SECS KEYS…` | Runs Fritz on a temporary copy and collects screenshots (`ctrl+F5`); `FRITZ_EXE` replaces the executable |
| `tools/ghidra/decompile.sh SEG:OFF…` | Imports the unpacked EXE into Ghidra (headless) and prints decompiled functions |

Example, capturing the About box:

```sh
tools/compare/dosbox-run.sh build/captures 15 F1 Up Return sleep:1.5 ctrl+F5
```

Note: under Wayland DOSBox runs through XWayland. XTest works as long as
the DOSBox window has focus.

## 10. Next steps

1. Continue `0000:2512` from `call 0000:05f3`: frames, panels, menu bar.
2. Find where the colours `[AD6E]`, `[0F4E]`, `[B0C6]` come from
   (probably `FRITZ2.PAR` or a default table).
3. Annotate the Ghidra project with the names of the functions identified so far.
4. Check whether `FRITZ2.BIN` is the engine: trace how `FRITZ2.EXE` loads it
   and its entry point (`JMP 0x2604`).
5. Decode `FRITZ2.IMG` (logo) and locate the piece bitmaps and the 8×14
   fonts in the EXE (they are visible in the uncompressed data).
6. Catalogue all menus (F1–F6) with `dosbox-run.sh` (IDEA.md §6.1).
