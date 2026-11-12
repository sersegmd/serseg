#!/usr/bin/env bash
set -euo pipefail
compiler=${1:?compiler path required}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/folder with spaces"
dir="$tmp/folder with spaces"
printf '\357\273\277use helper\r\nshow square(7)\r\n' > "$dir/main.ser"
printf '\357\273\277task square(n:int) -> int {\r\n give n*n\r\n}\r\n' > "$dir/helper.ser"
"$compiler" check "$dir/main.ser" > /dev/null
"$compiler" run "$dir/main.ser" > "$tmp/output"
tr -d '\r' < "$tmp/output" > "$tmp/normalized"
grep -Fxq '49' "$tmp/normalized"
echo 'PASS UTF-8 BOM, CRLF, imports and source path containing spaces'
printf '\357\273\277show missing\r\n' > "$dir/invalid.ser"
if "$compiler" check "$dir/invalid.ser" > "$tmp/out" 2> "$tmp/error"; then echo 'FAIL BOM diagnostic'; exit 1; fi
grep -Fq 'E102' "$tmp/error"; grep -Fq 'invalid.ser:1:6' "$tmp/error"; grep -Fq '1 | show missing' "$tmp/error"
echo 'PASS BOM diagnostic column and snippet'
printf 'show 1\n\357\273\277show 2\n' > "$dir/invalid.ser"
if "$compiler" check "$dir/invalid.ser" > "$tmp/out" 2> "$tmp/error"; then echo 'FAIL BOM accepted after start'; exit 1; fi
grep -Fq L001 "$tmp/error"; echo 'PASS BOM rejected away from start of file'
