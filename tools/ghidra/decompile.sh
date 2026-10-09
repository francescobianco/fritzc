#!/bin/sh
# Prints Ghidra's decompilation of FRITZ2.UNP.EXE functions.
#
# Usage: tools/ghidra/decompile.sh SEG:OFF [SEG:OFF ...]
#
# Expects Ghidra in build/deps/ghidra_* and a JDK in build/deps/jdk (or
# JAVA_HOME). The first run imports and analyses build/re/FRITZ2.UNP.EXE
# into build/ghidra; later runs reuse that project. Names and signatures
# from tools/ghidra/symbols.txt are applied before decompiling.
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
ghidra=$(ls -d "$root"/build/deps/ghidra_* | tail -1)
[ -d "$root/build/deps/jdk" ] && export JAVA_HOME="$root/build/deps/jdk"
mkdir -p "$root/build/ghidra"

if [ -d "$root/build/ghidra/fritz.rep" ]; then
    set -- -process FRITZ2.UNP.EXE -noanalysis "$@"
else
    set -- -import "$root/build/re/FRITZ2.UNP.EXE" "$@"
fi
first=$1 second=$2; shift 2
if [ "$first" = -process ]; then
    third=$1; shift
    "$ghidra/support/analyzeHeadless" "$root/build/ghidra" fritz "$first" "$second" "$third" \
        -scriptPath "$root/tools/ghidra" \
        -postScript Annotate.java "$root/tools/ghidra/symbols.txt" \
        -postScript Decompile.java "$@" 2>&1
else
    "$ghidra/support/analyzeHeadless" "$root/build/ghidra" fritz "$first" "$second" \
        -scriptPath "$root/tools/ghidra" \
        -postScript Annotate.java "$root/tools/ghidra/symbols.txt" \
        -postScript Decompile.java "$@" 2>&1
fi | awk '
    /^INFO  Decompile\.java> / { on = 1; sub(/^INFO  Decompile\.java> /, "") }
    /^(INFO|WARN|ERROR) / && !/Decompile\.java>/ { on = 0 }
    on { sub(/ \(GhidraScript\)  $/, ""); print }'
