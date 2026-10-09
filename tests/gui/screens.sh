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

# Regions reconstructed so far: menu bar, desktop pattern, outer frame,
# message box, arrow buttons, board frame, squares and pieces (except h3,
# where the original shows the square cursor and the mouse pointer).
REGIONS="0,0,640,15 5,20,11,380 612,20,635,380 5,20,635,35 0,15,640,20 0,472,640,480
0,15,5,480 635,15,640,480 5,380,635,472 354,181,496,229 10,34,16,366 336,34,342,366 10,34,342,40 10,360,342,366
16,40,336,240 16,240,296,360 296,280,336,360"

status=0
for case in main:$root/dos/fritz251/FRITZ2.PAR exp-colours:$exp_par exp-deutsch:$de_par \
            exp-flip:$flip_par; do
    name=${case%%:*} par=${case#*:}
    capture "$name" "$par"
    "$build/fritz" --data "$root/dos/fritz251" --par "$par" --dump "$caps/$name/native.bmp"
    echo "== $name"
    # shellcheck disable=SC2086
    python3 -I "$root/tools/compare/diff.py" "$caps/$name/native.bmp" \
        "$caps/$name/fritz2_000.png" $REGIONS || status=1
done
exit $status
