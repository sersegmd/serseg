#!/usr/bin/env bash
set -euo pipefail
compiler=${1:?compiler path required}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/ops.ser" <<'SER'
let text = "  hello world  "
show trim(text)
show contains(trim(text), "lo wo")
show starts_with(trim(text), "hello")
show ends_with(trim(text), "world")
show contains("abc", "")
show contains("", "x")
show starts_with("a", "abc")
show ends_with("a", "abc")
show slice(trim(text), 1, 5)
show slice("abc", 3, 3) == ""
show slice("مرحبا", 0, 2) == "م"
show utf8_len("مرحبا 🌍")
show utf8_slice("مرحبا 🌍", 0, 2)
assert utf8_len("") == 0
assert utf8_slice("", 0, 0) == ""
assert utf8_slice("مرحبا", 5, 5) == ""
show slice(str(1000), to_int("1"), to_int("3"))
show to_float("-12.5e+1") + 2.0
show to_float("+15") == 15.0
show to_float("0.125") * 8.0
show text
SER
cat > "$tmp/expected" <<'OUT'
hello world
true
true
true
true
false
false
false
ello
true
true
7
مر
00
-123
true
1
OUT
printf '  hello world  \n' >> "$tmp/expected"
for opt in -O0 -O1; do
 "$compiler" run "$tmp/ops.ser" "$opt" > "$tmp/out"
 tr -d '\r' < "$tmp/out" > "$tmp/normal"; diff -u "$tmp/expected" "$tmp/normal"
 echo "PASS text search, trim, byte slice and float conversions $opt"
done
for value in '' '+' '.5' '1.' '1e' 'NaN' 'inf' ' 1.0' '1.0 ' '1.2x' '1e9999' '1e-9999'; do
 printf 'show to_float("%s")\n' "$value" > "$tmp/invalid.ser"
 if "$compiler" run "$tmp/invalid.ser" > "$tmp/out" 2> "$tmp/err"; then echo "FAIL invalid float accepted: $value"; exit 1; fi
 grep -Fq 'SERSEG runtime error: to_float:' "$tmp/err"
done
echo 'PASS invalid and out-of-range float strings rejected'
for range in '-1, 1' '2, 1' '0, 4'; do
 printf 'show slice("abc", %s)\n' "$range" > "$tmp/invalid.ser"
 if "$compiler" run "$tmp/invalid.ser" > "$tmp/out" 2> "$tmp/err"; then echo "FAIL invalid slice accepted: $range"; exit 1; fi
 grep -Fq 'slice: byte range out of bounds' "$tmp/err"
done
echo 'PASS invalid byte ranges rejected'
for code in 'show to_float(true)' 'show trim(3)' 'show contains("a", 1)' 'show starts_with("a")' 'show ends_with(3, "a")' 'show slice("a", 0, true)' 'show slice("a", 0)' 'show utf8_len(3)' 'show utf8_slice("x", false, 1)'; do
 printf '%s\n' "$code" > "$tmp/invalid.ser"
 if "$compiler" check "$tmp/invalid.ser" > "$tmp/out" 2> "$tmp/err"; then echo "FAIL accepted: $code"; exit 1; fi
 grep -Eq 'E111|E112' "$tmp/err"
done
echo 'PASS text operation type and arity checks'
cat > "$tmp/stress.ser" <<'SER'
var text = ""
repeat i from 0 to 150000 {
  text = trim(" " + str(i) + " ")
  assert starts_with(text, "")
  assert ends_with(text, "")
  assert contains(text, slice(text, 0, 1))
  assert utf8_len(text) == len(text)
  assert utf8_slice(text, 0, 1) == slice(text, 0, 1)
  assert to_float(text) >= 0.0
}
show text
SER
"$compiler" run "$tmp/stress.ser" > "$tmp/out"
tr -d '\r' < "$tmp/out" > "$tmp/normal"; printf '149999\n' > "$tmp/expected"; diff -u "$tmp/expected" "$tmp/normal"
echo 'PASS text operation allocation stress'
