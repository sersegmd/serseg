#!/usr/bin/env bash
set -euo pipefail
compiler=${1:?compiler path required}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/arrays.ser" <<'SER'
task make(n: int) -> int_array {
  give [n, n + 1, n + 2]
}
task alias(a: int_array) -> int_array {
  var local = a
  push local, 9
  give local
}
task recursive(a: int_array, n: int) -> int_array {
  when n == 0 { give a }
  give recursive(a, n - 1)
}
var numbers: int_array = make(10)
let copy = numbers
numbers[1] = numbers[0] + 20
show copy[1]
let other = alias(numbers)
show other.length
show copy[3]
show recursive(make(4), 10)[2]
numbers = [5]
show copy[0]
show numbers[0]
show [].length
SER
printf '30\n4\n9\n6\n10\n5\n0\n' > "$tmp/expected"
for opt in -O0 -O1; do
  "$compiler" run "$tmp/arrays.ser" "$opt" > "$tmp/out"
  tr -d '\r' < "$tmp/out" > "$tmp/normalized"
  diff -u "$tmp/expected" "$tmp/normalized"
  echo "PASS arrays aliases, mutation, recursion and function returns $opt"
done
cat > "$tmp/stress.ser" <<'SER'
var data = []
let alias = data
repeat i from 0 to 100000 {
  push data, i
  let temp = [i, i + 1]
  when i == 99999 { show temp[1] }
}
show alias.length
show alias[99999]
SER
"$compiler" run "$tmp/stress.ser" > "$tmp/out"
tr -d '\r' < "$tmp/out" > "$tmp/normalized"
printf '100000\n100000\n99999\n' > "$tmp/expected"; diff -u "$tmp/expected" "$tmp/normalized"
echo 'PASS array growth and 100000 temporary allocations'
for code in 'show [1][-1]' 'show [1][1]' 'show [][0]' $'var x = [1]\nx[1] = 7'; do
  printf '%s\n' "$code" > "$tmp/invalid.ser"
  if "$compiler" run "$tmp/invalid.ser" > "$tmp/out" 2> "$tmp/error"; then echo 'FAIL bounds check'; exit 1; fi
  grep -Fq 'array index out of bounds' "$tmp/error"
done
echo 'PASS negative, upper, empty and write bounds checks'
for code in 'show [1, true]' 'show [1][false]' 'show 1.length' $'let x = [1]\nx[0] = 2' $'let x = []\npush x, 1' $'var x = []\npush x, true' 'show [1]' 'show [1] == [1]'; do
  printf '%s\n' "$code" > "$tmp/invalid.ser"
  if "$compiler" check "$tmp/invalid.ser" > "$tmp/out" 2> "$tmp/error"; then echo "FAIL invalid array program accepted: $code"; exit 1; fi
  grep -Eq 'Error\[(E106|E108|E122|E123|L005)\]' "$tmp/error"
done
echo 'PASS array type and mutation errors rejected'
