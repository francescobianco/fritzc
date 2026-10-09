#!/bin/sh
# Compares the port with DOSBox captures of the original, in the screen
# regions that are already reconstructed.
#
# Usage: tests/gui/screens.sh [BUILD_DIR]
# Needs dos/fritz251 (original distribution), DOSBox and an X display the
# first time, to create the captures in build/captures/test (not committed:
# they are images of the original program).
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
build=${1:-$root/build/cmake}
caps=$root/build/captures/test
unpacked=$root/build/re/FRITZ2.UNP.EXE

[ -f "$unpacked" ] || python3 -I "$root/tools/inspect/unexepack.py" \
    "$root/dos/fritz251/FRITZ2.EXE" "$unpacked"

# Experiment 3: background/menu colour 14, hatching colour 4.
exp_par=$caps/exp-colours.par
mkdir -p "$caps"
[ -f "$exp_par" ] || python3 -I - "$root/dos/fritz251/FRITZ2.PAR" "$exp_par" <<'PY'
import sys
p = bytearray(open(sys.argv[1], 'rb').read())
p[0x194] = 14
p[0x19e] = 4
open(sys.argv[2], 'wb').write(p)
PY

# Experiment 4: German language file.
de_par=$caps/exp-deutsch.par
[ -f "$de_par" ] || python3 -I - "$root/dos/fritz251/FRITZ2.PAR" "$de_par" <<'PY'
import sys
p = bytearray(open(sys.argv[1], 'rb').read())
p[4:20] = b'FDEUTSCH.LAN'.ljust(16, b'\0')
open(sys.argv[2], 'wb').write(p)
PY

# Experiment 5: flipped board.
flip_par=$caps/exp-flip.par
[ -f "$flip_par" ] || python3 -I - "$root/dos/fritz251/FRITZ2.PAR" "$flip_par" <<'PY'
import sys
p = bytearray(open(sys.argv[1], 'rb').read())
p[0x1d4] = 1
open(sys.argv[2], 'wb').write(p)
PY

capture() {  # NAME PAR
    if [ ! -f "$caps/$1/fritz2_000.png" ]; then
        FRITZ_EXE=$unpacked FRITZ_FILES="FRITZ2.PAR=$2" \
            "$root/tools/compare/dosbox-run.sh" "$caps/$1" 10 ctrl+F5 >/dev/null
    fi
}

# The whole start screen is reconstructed.
REGIONS="0,0,640,480"

status=0
for case in main:$root/dos/fritz251/FRITZ2.PAR exp-colours:$exp_par exp-deutsch:$de_par \
            exp-flip:$flip_par; do
    name=${case%%:*} par=${case#*:}
    capture "$name" "$par"
    # The captures run the unpacked EXE, which leaves 101 kB free.
    "$build/fritz" --data "$root/dos/fritz251" --par "$par" --free-kb 101 \
        --dump "$caps/$name/native.bmp"
    echo "== $name"
    # shellcheck disable=SC2086
    python3 -I "$root/tools/compare/diff.py" "$caps/$name/native.bmp" \
        "$caps/$name/fritz2_000.png" $REGIONS || status=1
done
exit $status
