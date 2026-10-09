#!/bin/sh
# Runs Fritz 2.51 in DOSBox on a temporary copy of the distribution,
# optionally sends a key sequence, and collects the screenshots.
#
# Usage: tools/compare/dosbox-run.sh OUTDIR [SECONDS] [KEYS...]
#   Keys use the tools/inspect/xkeys.py syntax; "ctrl+F5" saves a native
#   PNG screenshot (640x480, palettized) into OUTDIR.
# Example: tools/compare/dosbox-run.sh out 15 F1 Up Return sleep:1 ctrl+F5
#
# FRITZ_EXE=path/FRITZ2.EXE replaces the executable in the working copy
# (e.g. the unpacked build or one patched with tools/compare/patch.py).
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
out=$(mkdir -p "$1" && cd "$1" && pwd); shift
secs=${1:-15}; [ $# -gt 0 ] && shift

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/c"
cp -r "$root/dos/fritz251" "$work/c/FRITZ"
[ -n "${FRITZ_EXE:-}" ] && cp "$FRITZ_EXE" "$work/c/FRITZ/FRITZ2.EXE"
{
    cat "$root/reference/dosbox.conf"
    printf '\n[dosbox]\ncaptures=%s\n' "$out"
    printf '[autoexec]\nmount c %s\nc:\ncd FRITZ\nFRITZ2.EXE\n' "$work/c"
} > "$work/dosbox.conf"

timeout "$secs" dosbox -conf "$work/dosbox.conf" -noconsole >/dev/null 2>&1 &
pid=$!
sleep 7   # wait for the program to load
[ $# -gt 0 ] && python3 -I "$root/tools/inspect/xkeys.py" "$@"
wait $pid || true
ls "$out"
