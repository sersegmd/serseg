#!/usr/bin/env bash
set -euo pipefail
compiler=${1:?compiler path required}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/checks.ser" <<'SER'
assert is_int("0")
assert is_int("-9223372036854775808")
assert !is_int("9223372036854775808")
assert !is_int("12x")
assert !is_int("")
assert is_float("123")
assert is_float("-1.25e2")
assert !is_float("1e9999")
assert !is_float(".5")
assert !is_float("1x")
task classify(value: int) -> text {
  when value < 0 { give "negative" }
  otherwise when value == 0 { give "zero" }
  otherwise when value < 10 { give "small" }
  otherwise { give "large" }
}
show classify(-1)
show classify(0)
show classify(9)
show classify(10)
SER
for opt in -O0 -O1; do
 "$compiler" run "$tmp/checks.ser" "$opt" > "$tmp/out"
 tr -d '\r' < "$tmp/out" > "$tmp/normal"
 printf 'negative\nzero\nsmall\nlarge\n' > "$tmp/expected"; diff -u "$tmp/expected" "$tmp/normal"
 echo "PASS numeric validation and multiple conditional branches $opt"
done
for pair in '5|Next number: 6' '1.5|That is a decimal number, not an integer.' 'oops|That is not a valid number.'; do
 value=${pair%%|*}; expected=${pair#*|}
 printf '%s\n' "$value" | "$compiler" run examples/validate-number.ser > "$tmp/out"
 tr -d '\r' < "$tmp/out" > "$tmp/normal"; printf 'Enter an integer: %s\n' "$expected" > "$tmp/expected"; diff -u "$tmp/expected" "$tmp/normal"
done
echo 'PASS validation example handles valid and invalid inputs'
for code in 'show is_int(1)' 'show is_float()' 'when true { show 1 } otherwise when 3 { show 2 }' 'task f(a:int) -> int { when a > 1 { give 1 } otherwise when a == 0 { give 2 } }'; do
 printf '%s\n' "$code" > "$tmp/invalid.ser"
 if "$compiler" check "$tmp/invalid.ser" > "$tmp/out" 2> "$tmp/err"; then echo "FAIL accepted: $code"; exit 1; fi
 grep -Eq 'E111|E112|E107|E116' "$tmp/err"
done
echo 'PASS validation argument and branch return errors'
python3 - "$tmp/deep.ser" <<'PY'
import sys
with open(sys.argv[1], 'w') as f:
    f.write('when false { show 0 }')
    for _ in range(130): f.write(' otherwise when false { show 1 }')
    f.write('\n')
PY
if "$compiler" check "$tmp/deep.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL oversized conditional chain accepted'; exit 1; fi
grep -q P008 "$tmp/err"; echo 'PASS oversized conditional chain rejected safely'
