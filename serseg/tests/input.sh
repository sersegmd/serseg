#!/usr/bin/env bash
set -euo pipefail
compiler=${1:?compiler path required}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/input.ser" <<'SER'
let name = input()
let second = input()
show "Hello " + name
show second.length
show input().length
SER
printf 'SERSEG\r\n\n' | "$compiler" run "$tmp/input.ser" > "$tmp/out"
tr -d '\r' < "$tmp/out" > "$tmp/normalized"; printf 'Hello SERSEG\n0\n0\n' > "$tmp/expected"; diff -u "$tmp/expected" "$tmp/normalized"
echo 'PASS input lines, CRLF, empty line and EOF'
cat > "$tmp/long.ser" <<'SER'
let text = input()
show text.length
show text == text
SER
python3 -c 'print("x" * 100000)' | "$compiler" run "$tmp/long.ser" -O1 > "$tmp/out"
tr -d '\r' < "$tmp/out" > "$tmp/normalized"; printf '100000\ntrue\n' > "$tmp/expected"; diff -u "$tmp/expected" "$tmp/normalized"
echo 'PASS long input buffer growth and text lifetime'
if printf 'bad\0input\n' | "$compiler" run "$tmp/long.ser" > "$tmp/out" 2> "$tmp/error"; then echo 'FAIL NUL accepted'; exit 1; fi
grep -Fq 'input contains NUL byte' "$tmp/error"; echo 'PASS embedded NUL input rejected'
printf 'show input(1)\n' > "$tmp/invalid.ser"
if "$compiler" check "$tmp/invalid.ser" > "$tmp/out" 2> "$tmp/error"; then echo 'FAIL invalid input arguments accepted'; exit 1; fi
grep -Fq E111 "$tmp/error"; echo 'PASS input arguments rejected'
