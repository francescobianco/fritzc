#!/bin/sh
# Prints Ghidra's decompilation of FRITZ2.BIN (engine module) functions.
#
# Usage: tools/ghidra/decompile-engine.sh OFF [OFF ...]
#   OFF: address in the module's 0x100-based space (e.g. 72f2).
# The module is imported as a raw x86-16 real-mode binary at 1000:0100.
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
ghidra=$(ls -d "$root"/build/deps/ghidra_* | tail -1)
[ -d "$root/build/deps/jdk" ] && export JAVA_HOME="$root/build/deps/jdk"
mkdir -p "$root/build/ghidra"
args=""
for off in "$@"; do args="$args @1000:$off"; done

if [ -d "$root/build/ghidra/engine.rep" ]; then
    set -- -process FRITZ2.BIN -noanalysis
else
    set -- -import "$root/dos/fritz251/FRITZ2.BIN" -loader BinaryLoader \
        -loader-baseAddr 1000:0100 -processor "x86:LE:16:Real Mode"
fi
# shellcheck disable=SC2086
"$ghidra/support/analyzeHeadless" "$root/build/ghidra" engine "$@" \
    -scriptPath "$root/tools/ghidra" \
    -postScript Annotate.java "$root/tools/ghidra/engine-symbols.txt" \
    -postScript Decompile.java $args 2>&1 | awk '
    /^INFO  Decompile\.java> / { on = 1; sub(/^INFO  Decompile\.java> /, "") }
    /^(INFO|WARN|ERROR) / && !/Decompile\.java>/ { on = 0 }
    on { sub(/ \(GhidraScript\)  $/, ""); print }'
