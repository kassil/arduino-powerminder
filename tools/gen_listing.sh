#!/bin/sh
# Generate a human-readable assembly listing (.lss) and symbol dump (.sym) for
# an ELF, to aid size analysis and low-level debugging of AVR firmware.
#
# Usage: gen_listing.sh <objdump-exe> <elf-file> <lss-out> <sym-out>
set -eu

usage() {
    cat >&2 <<EOF
Usage: $0 <objdump-exe> <elf-file> <lss-out> <sym-out>

Generate an assembly listing (.lss) and symbol dump (.sym) from an ELF.

  <objdump-exe>  objdump to use (e.g. avr-objdump)
  <elf-file>     input ELF file
  <lss-out>      output path for the assembly listing
  <sym-out>      output path for the symbol dump

This script is normally invoked automatically by the CMake build.  To run it
by hand, pass all four arguments, e.g.:
  $0 avr-objdump build/ArduinoProject.elf build/ArduinoProject.lss build/ArduinoProject.sym
EOF
    exit 2
}

if [ "$#" -ne 4 ]; then
    echo "$0: error: expected 4 arguments, got $#" >&2
    usage
fi

OBJDUMP=$1
ELF=$2
LSS=$3
SYM=$4

if [ ! -f "$ELF" ]; then
    echo "$0: error: ELF file not found: $ELF" >&2
    exit 1
fi

mkdir -p "$(dirname "$LSS")" "$(dirname "$SYM")"

# Interleaved C/C++ source + disassembly, with C++ names demangled (-C).
"$OBJDUMP" -d -S -C "$ELF" > "$LSS"

# Prefer nm for the symbol dump: it can sort and print sizes, which is far more
# useful than 'objdump -t' for hunting down flash/RAM bloat and it omits the
# .debug_* clutter.  Derive the matching nm from the objdump path
# (e.g. avr-objdump -> avr-nm); fall back to objdump if nm is unavailable.
NM=$(printf '%s' "$OBJDUMP" | sed 's/objdump$/nm/')
if command -v "$NM" >/dev/null 2>&1; then
    {
        echo "== Symbols by address =="
        "$NM" -C -n --print-size "$ELF"
        echo
        echo "== Symbols by size (largest first) =="
        "$NM" -C --print-size --size-sort --reverse-sort "$ELF"
    } > "$SYM"
else
    "$OBJDUMP" -t -C "$ELF" > "$SYM"
fi
