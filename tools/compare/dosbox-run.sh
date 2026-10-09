#!/bin/sh
# Avvia Fritz 2.51 in DOSBox su una copia temporanea della distribuzione,
# invia opzionalmente una sequenza di tasti e raccoglie gli screenshot.
#
# Uso: tools/compare/dosbox-run.sh OUTDIR [SECONDI] [TASTI...]
#   I tasti usano la sintassi di tools/inspect/xkeys.py; "ctrl+F5" salva
#   uno screenshot PNG nativo (640x480, palette) in OUTDIR.
# Esempio: tools/compare/dosbox-run.sh out 15 F1 Up Return sleep:1 ctrl+F5
#
# FRITZ_EXE=percorso/FRITZ2.EXE sostituisce l'eseguibile nella copia di lavoro
# (es. una versione decompressa o modificata con tools/compare/patch.py).
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
sleep 7   # attesa caricamento del programma
[ $# -gt 0 ] && python3 -I "$root/tools/inspect/xkeys.py" "$@"
wait $pid || true
ls "$out"
