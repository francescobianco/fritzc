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

capture() {  # NAME PAR
    if [ ! -f "$caps/$1/fritz2_000.png" ]; then
        FRITZ_EXE=$unpacked FRITZ_FILES="FRITZ2.PAR=$2" \
            "$root/tools/compare/dosbox-run.sh" "$caps/$1" 10 ctrl+F5 >/dev/null
    fi
}

# Regions reconstructed so far: desktop pattern, outer frame, message box.
REGIONS="5,20,11,380 612,20,635,380 5,20,635,35 0,15,640,20 0,472,640,480
0,15,5,480 635,15,640,480 5,380,635,472"

status=0
for case in main:$root/dos/fritz251/FRITZ2.PAR exp-colours:$exp_par; do
    name=${case%%:*} par=${case#*:}
    capture "$name" "$par"
    "$build/fritz" --par "$par" --dump "$caps/$name/native.bmp"
    echo "== $name"
    # shellcheck disable=SC2086
    python3 -I "$root/tools/compare/diff.py" "$caps/$name/native.bmp" \
        "$caps/$name/fritz2_000.png" $REGIONS || status=1
done
exit $status
