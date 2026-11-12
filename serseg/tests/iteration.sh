#!/usr/bin/env bash
set -euo pipefail
compiler=${1:?compiler path required}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/iteration.ser" <<'SER'
task find(a: int_array) -> int {
  each item in a {
    when item == 7 { give item }
  }
  give -1
}
var items = [1, 2, 3]
var sum = 0
each item in items {
  push items, 9
  when item == 2 { continue }
  sum = sum + item
}
show sum
show items.length
show find([4, 7, 9])
show find([])
each x in [1, 2] {
  each y in [10, 20] {
    when y == 20 { break }
    show x + y
  }
}
show "".length
show ("abc" + "de").length
show "é".length
SER
printf '4\n6\n7\n-1\n11\n12\n0\n5\n2\n' > "$tmp/expected"
for opt in -O0 -O1; do
  "$compiler" run "$tmp/iteration.ser" "$opt" > "$tmp/out"
  tr -d '\r' < "$tmp/out" > "$tmp/normalized"; diff -u "$tmp/expected" "$tmp/normalized"
  echo "PASS iteration, nested loops, early returns, snapshot length and text byte lengths $opt"
done
cat > "$tmp/reassign.ser" <<'SER'
var original = [1, 2, 3]
var alias = original
each x in original {
  show x
  original = [99]
  when x == 2 { alias[2] = 8 }
}
show original[0]
SER
"$compiler" run "$tmp/reassign.ser" > "$tmp/out"
tr -d '\r' < "$tmp/out" > "$tmp/normalized"; printf '1\n2\n8\n99\n' > "$tmp/expected"; diff -u "$tmp/expected" "$tmp/normalized"
echo 'PASS iterable evaluated once and mutations to later elements observed'
cat > "$tmp/stress.ser" <<'SER'
task one(a: int_array) -> int {
 each x in a { give x }
 give 0
}
repeat i from 0 to 100000 {
 each x in [i] { when x == i { break } }
 assert one([i]) == i
}
show "done"
SER
"$compiler" run "$tmp/stress.ser" > "$tmp/out"
tr -d '\r' < "$tmp/out" > "$tmp/normalized"; grep -Fxq done "$tmp/normalized"
echo 'PASS 100000 iteration breaks and early function returns'
for code in $'each x in 1 {\n show x\n}' $'each x in [1] {\n x = 2\n}' 'show true.length' $'each x [1] {\n show x\n}'; do
  printf '%s\n' "$code" > "$tmp/invalid.ser"
  if "$compiler" check "$tmp/invalid.ser" > "$tmp/out" 2> "$tmp/error"; then echo 'FAIL accepted invalid iteration'; exit 1; fi
  grep -Eq 'Error\[(E108|E123|P005)\]' "$tmp/error"
done
echo 'PASS invalid iterable, binding mutation, text length and syntax rejected'
