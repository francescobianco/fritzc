# Fritz 2.5 Reborn

Porting nativo in C11 + SDL3 di ChessBase Fritz 2.51 per MS-DOS, ottenuto con
il reverse engineering del programma originale. Visione e piano: [IDEA.md](IDEA.md).
Stato dell'analisi: [docs/reverse-engineering.md](docs/reverse-engineering.md).

## Compilazione

```sh
cmake -S . -B build/cmake          # scarica e compila SDL3 se non è installata
cmake --build build/cmake
build/cmake/fritz [--scale N] [--reference cattura.bmp]
```

Per usare una SDL3 già compilata: `-DCMAKE_PREFIX_PATH=/percorso/prefix`.

## Distribuzione originale

I file originali vanno messi in `dos/fritz251/` (esclusa dal repository).
Gli hash attesi sono in `reference/fritz251.sha256`:

```sh
(cd dos/fritz251 && sha256sum -c ../../reference/fritz251.sha256)
```

## Confronto con l'originale

```sh
tools/compare/dosbox-run.sh build/captures/main 10 ctrl+F5
python3 -c "from PIL import Image; Image.open('build/captures/main/fritz2_000.png').save('build/captures/main/main.bmp')"
build/cmake/fritz --reference build/captures/main/main.bmp   # Tab = originale, D = differenze
build/cmake/fritz --dump build/native.bmp                     # frame nativo, senza finestra
```

## Esperimenti con patch

```sh
python3 tools/compare/patch.py build/re/FRITZ2.UNP.EXE build/re/exp.exe DS:703b=ff00ff00ff00ff00
FRITZ_EXE=build/re/exp.exe tools/compare/dosbox-run.sh build/captures/exp 10 ctrl+F5
```
