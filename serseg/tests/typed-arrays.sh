#!/usr/bin/env bash
set -euo pipefail
compiler=${1:?compiler path required}
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/test.ser" <<'SER'
task first(values: text_array) -> text {
 each word in values { give word }
 give "none"
}
task empty() -> text_array { give [] }
var words: text_array = []
push words, "Al" + "i"
push words, "hello"
let shared = words
words[1] = words[0] + "!"
show shared[1]
show first(words)
show first(empty())
show first([])
var floats: float_array = [1.5, 2.5]
push floats, 3.5
floats[0] = floats[0] + 1.0
each f in floats { show f }
var flags: bool_array = [true, false]
push flags, true
flags[1] = true
var n = 0
each flag in flags { when flag { n = n + 1 } }
show n
SER
printf 'Ali!\nAli\nnone\nnone\n2.5\n2.5\n3.5\n3\n' > "$tmp/expected"
for opt in -O0 -O1; do
 "$compiler" run "$tmp/test.ser" "$opt" > "$tmp/out"; tr -d '\r' < "$tmp/out" > "$tmp/normalized"; diff -u "$tmp/expected" "$tmp/normalized"
 echo "PASS typed arrays, inference, aliases, loops, calls and returns $opt"
done
for code in 'show [1, true]' $'var a: text_array = []\npush a, 1' $'var a = [true]\na[0] = 1' 'show [[1]]' $'let a: float_array = []\nshow a[0]'; do
 printf '%s\n' "$code" > "$tmp/error.ser"
 if "$compiler" run "$tmp/error.ser" > "$tmp/out" 2> "$tmp/err"; then echo "FAIL: accepted $code"; exit 1; fi
 grep -Eq 'E122|E123|array index out of bounds' "$tmp/err"
done
echo 'PASS mixed types, invalid mutation and bounds rejected'
cat > "$tmp/stress.ser" <<'SER'
task one(a: text_array) -> text {
 each x in a { give x }
 give ""
}
repeat i from 0 to 100000 {
 var a = [str(i), "x" + "y"]
 let b = a
 a[1] = a[0] + "!"
 assert one(b) == str(i)
 each s in a { assert len(s) > 0 }
}
show "ok"
SER
"$compiler" run "$tmp/stress.ser" > "$tmp/out"; grep -Fq ok "$tmp/out"
echo 'PASS 100000 text array allocation and ownership cycles'
