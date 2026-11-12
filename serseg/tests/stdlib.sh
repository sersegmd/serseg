#!/usr/bin/env bash
set -euo pipefail
compiler=${1:?compiler path required}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/main.ser" <<'SER'
use math
show math.min(8, 4)
show math.max(-2, 4)
show math.clamp(20, 0, 10)
show math.min_float(1.5, 2.5)
show math.max_float(1.5, 2.5)
SER
"$compiler" run "$tmp/main.ser" > "$tmp/out"; tr -d '\r' < "$tmp/out" > "$tmp/normalized"
printf '4\n4\n10\n1.5\n2.5\n' > "$tmp/expected"; diff -u "$tmp/expected" "$tmp/normalized"
echo 'PASS stdlib discovery relative to compiler and math module'
cat > "$tmp/math.ser" <<'SER'
task min(a: int, b: int) -> int { give 42 }
SER
printf 'use math\nshow math.min(1,2)\n' > "$tmp/main.ser"
"$compiler" run "$tmp/main.ser" > "$tmp/out"; tr -d '\r' < "$tmp/out" > "$tmp/normalized"; grep -Fxq 42 "$tmp/normalized"
echo 'PASS project-local modules override stdlib'
mkdir -p "$tmp/isolated/bin"
cp "$compiler" "$tmp/isolated/bin/$(basename "$compiler")"
if "$tmp/isolated/bin/$(basename "$compiler")" doctor > "$tmp/doctor" 2>&1; then echo 'FAIL doctor should indicate missing components'; exit 1; fi
grep -Fq '[missing] Math standard library module not found' "$tmp/doctor"
echo 'PASS doctor detects a missing math module'
