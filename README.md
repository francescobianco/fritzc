# Fritz 2.5 Reborn

A native C11 + SDL3 port of ChessBase Fritz 2.51 for MS-DOS, rebuilt by
reverse engineering the original program. Vision and plan: [IDEA.md](IDEA.md).
Analysis status: [docs/reverse-engineering.md](docs/reverse-engineering.md).

## Building

```sh
cmake -S . -B build/cmake          # downloads and builds SDL3 if not installed
cmake --build build/cmake
build/cmake/fritz [--data dos/fritz251] [--par FILE] [--scale N] [--reference capture.bmp] [--dump out.bmp]
```

To use an existing SDL3 build: `-DCMAKE_PREFIX_PATH=/path/to/prefix`.

## Original distribution

Put the original files in `dos/fritz251/` (excluded from the repository).
The port reads fonts, graphics and texts from them at runtime; no original
asset is stored in this repository.
The expected hashes are in `reference/fritz251.sha256`:

```sh
(cd dos/fritz251 && sha256sum -c ../../reference/fritz251.sha256)
python3 tools/inspect/unexepack.py dos/fritz251/FRITZ2.EXE build/re/FRITZ2.UNP.EXE
```

## Comparing with the original

```sh
tools/compare/dosbox-run.sh build/captures/main 10 ctrl+F5
python3 -c "from PIL import Image; Image.open('build/captures/main/fritz2_000.png').save('build/captures/main/main.bmp')"
build/cmake/fritz --reference build/captures/main/main.bmp   # Tab = original, D = differences
build/cmake/fritz --dump build/native.bmp                     # native frame, no window
```

## Tests

```sh
tests/gui/screens.sh    # needs dos/fritz251 and DOSBox the first time
```

## Patch experiments

```sh
python3 tools/compare/patch.py build/re/FRITZ2.UNP.EXE build/re/exp.exe DS:703b=ff00ff00ff00ff00
FRITZ_EXE=build/re/exp.exe tools/compare/dosbox-run.sh build/captures/exp 10 ctrl+F5
```

## Ghidra

```sh
tools/ghidra/decompile.sh 293b:9e44 0000:2512   # imports on first run
```

Ghidra and a JDK are expected in `build/deps/` (see the script header).
