#!/usr/bin/env bash
set -euo pipefail
compiler=${1:?compiler path required}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/strings.ser" <<'SER'
task join(a: text, b: text) -> text {
    let copy = a
    give copy + b
}
task nested(n: int, value: text) -> text {
    when n == 0 { give value }
    give nested(n - 1, value + "x")
}
var first = "hi" + "!"
let alias = first
first = first
first = join(first, first)
show alias
show first
show join(join("a", "b"), join("c", "d"))
show nested(8, "z")
show "" + "a" + ""
show ("a" + "b") == "ab"
show first != alias
show "ab" != "ab"
SER
printf 'hi!\nhi!hi!\nabcd\nzxxxxxxxx\na\ntrue\ntrue\nfalse\n' > "$tmp/expected"
for opt in -O0 -O1; do
  "$compiler" run "$tmp/strings.ser" "$opt" > "$tmp/output"
  tr -d '\r' < "$tmp/output" > "$tmp/normalized"
  diff -u "$tmp/expected" "$tmp/normalized"
  echo "PASS managed text aliases, recursion, return and nested calls $opt"
done
cat > "$tmp/stress.ser" <<'SER'
var current = "seed"
repeat i from 0 to 150000 {
    let local = current + "!"
    current = "again" + "!"
    when i == 0 { show local }
}
show current
SER
"$compiler" run "$tmp/stress.ser" > "$tmp/output"
tr -d '\r' < "$tmp/output" > "$tmp/normalized"
printf 'seed!\nagain!\n' > "$tmp/expected"; diff -u "$tmp/expected" "$tmp/normalized"
echo 'PASS 150000 iterations of managed text allocations and reassignment'
printf 'show "a" + 1\n' > "$tmp/invalid.ser"
if "$compiler" check "$tmp/invalid.ser" > "$tmp/output" 2> "$tmp/error"; then echo 'FAIL mixed text arithmetic accepted'; exit 1; fi
grep -Fq E106 "$tmp/error"
echo 'PASS mixed text arithmetic rejected'
