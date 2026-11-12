#!/usr/bin/env bash
set -euo pipefail
compiler=${1:?compiler path required}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/main.ser" <<'SER'
use math
show math.square(7)
show square(8)
SER
cat > "$tmp/math.ser" <<'SER'
use helper
task square(n: int) -> int { give helper.multiply(n, n) }
SER
cat > "$tmp/helper.ser" <<'SER'
task multiply(a: int, b: int) -> int { give a * b }
SER
for opt in -O0 -O1; do
 "$compiler" run "$tmp/main.ser" "$opt" > "$tmp/out"
 tr -d '\r' < "$tmp/out" > "$tmp/normal"; printf '49\n64\n' > "$tmp/expected"; diff -u "$tmp/expected" "$tmp/normal"
 echo "PASS qualified and existing unqualified imports $opt"
done
printf 'show math.square(2)\n' > "$tmp/main.ser"
if "$compiler" check "$tmp/main.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL unimported module'; exit 1; fi
grep -Fq M105 "$tmp/err"
printf 'use math\nshow math.absent(1)\n' > "$tmp/main.ser"
if "$compiler" check "$tmp/main.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL nonexistent export'; exit 1; fi
grep -Fq M106 "$tmp/err"
printf 'use math\nshow math.multiply(1,2)\n' > "$tmp/main.ser"
if "$compiler" check "$tmp/main.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL transitive export'; exit 1; fi
grep -Fq M106 "$tmp/err"
echo 'PASS unknown and non-exported module names rejected'
