#!/usr/bin/env bash
set -euo pipefail
compiler=${1:?compiler path required}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/builtins.ser" <<'SER'
show len("ab" + "cd")
show len("")
show str(0 - 9223372036854775807 - 1)
show str(9223372036854775807)
show str(1.25)
show str(true) + "/" + str(false)
show to_int("+00042")
show to_int("-9223372036854775808")
show to_int("9223372036854775807")
show ("a" + "b") == "ab"
show ("a" + "b") != "ac"
let answer = input("Question? ")
show "[" + answer + "]"
SER
printf '4\n0\n-9223372036854775808\n9223372036854775807\n1.25\ntrue/false\n42\n-9223372036854775808\n9223372036854775807\ntrue\ntrue\nQuestion? [OK]\n' > "$tmp/expected"
for opt in -O0 -O1; do
  printf 'OK\r\n' | "$compiler" run "$tmp/builtins.ser" "$opt" > "$tmp/out"
  tr -d '\r' < "$tmp/out" > "$tmp/normal"; diff -u "$tmp/expected" "$tmp/normal"
  echo "PASS text builtins, extremes, prompt and comparison $opt"
done
for value in '' '+' '-' ' 12' '12 ' '1x' '9223372036854775808' '-9223372036854775809'; do
  printf 'show to_int("%s")\n' "$value" > "$tmp/invalid.ser"
  if "$compiler" run "$tmp/invalid.ser" > "$tmp/out" 2> "$tmp/err"; then echo "FAIL invalid to_int: $value"; exit 1; fi
  grep -Fq 'SERSEG runtime error: to_int:' "$tmp/err"
done
echo 'PASS invalid and overflowing decimal input rejected'
for code in 'show len(3)' 'show len()' 'show str("x")' 'show str(1, 2)' 'show to_int(true)' 'show input(1)' 'show input("a", "b")'; do
  printf '%s\n' "$code" > "$tmp/invalid.ser"
  if "$compiler" check "$tmp/invalid.ser" > "$tmp/out" 2> "$tmp/err"; then echo "FAIL accepted: $code"; exit 1; fi
  grep -Eq 'E111|E112' "$tmp/err"
done
echo 'PASS builtin arity and type errors'
printf 'Aya\n22\n' | "$compiler" run examples/text.ser > "$tmp/out"
tr -d '\r' < "$tmp/out" > "$tmp/normal"
printf 'What is your name? How old are you? Hello, Aya!\nYour age: 22\nName length in bytes: 3\n' > "$tmp/expected"
diff -u "$tmp/expected" "$tmp/normal"
echo 'PASS text example'
cat > "$tmp/stress.ser" <<'SER'
var result = ""
repeat i from 0 to 150000 {
  result = str(i)
  assert to_int(result) == i
  assert len(result) > 0
}
show result
SER
"$compiler" run "$tmp/stress.ser" > "$tmp/out"
tr -d '\r' < "$tmp/out" > "$tmp/normal"; printf '149999\n' > "$tmp/expected"; diff -u "$tmp/expected" "$tmp/normal"
echo 'PASS builtin managed text allocation stress'
