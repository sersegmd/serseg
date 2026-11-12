#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
serseg="${1:-$root/build/serseg}"
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) suffix=.exe; format='PE32+';; *) suffix=; format='ELF 64-bit';; esac
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
pass=0
ok(){ pass=$((pass+1)); printf 'PASS %s\n' "$1"; }
expect_output(){ local label=$1 source=$2 expected=$3; printf '%s\n' "$source" > "$tmp/test.ser"; "$serseg" run "$tmp/test.ser" > "$tmp/out" 2> "$tmp/err" || { cat "$tmp/err"; exit 1; }; printf '%s\n' "$expected" > "$tmp/expected"; tr -d '\r' < "$tmp/out" > "$tmp/normalized"; diff -u "$tmp/expected" "$tmp/normalized"; ok "$label"; }
expect_error(){ local label=$1 source=$2 error=$3; printf '%s\n' "$source" > "$tmp/test.ser"; if "$serseg" check "$tmp/test.ser" > "$tmp/out" 2> "$tmp/err"; then echo "FAIL $label: accepted invalid program"; exit 1; fi; grep -Fq "$error" "$tmp/err" || { cat "$tmp/err"; exit 1; }; ok "$label"; }
printf 'show 2 + 3 * 4\nshow 9223372036854775807 + 1\nshow true && false\n' > "$tmp/optimized.ser"
"$serseg" run "$tmp/optimized.ser" -O0 > "$tmp/out0"
"$serseg" run "$tmp/optimized.ser" -O1 > "$tmp/out1"
diff -u "$tmp/out0" "$tmp/out1"; ok 'O1 preserves integer and boolean output'
"$serseg" build "$tmp/optimized.ser" -O1 -o "$tmp/optimized$suffix" > /dev/null
"$tmp/optimized$suffix" > "$tmp/out1"; diff -u "$tmp/out0" "$tmp/out1"; ok 'O1 native build executes'
printf 'show 1 / 0\n' > "$tmp/optimized.ser"
if "$serseg" run "$tmp/optimized.ser" -O1 > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL O1 division error'; exit 1; fi
grep -Fq 'invalid integer division' "$tmp/err"; ok 'O1 preserves runtime traps'
if "$serseg" run "$tmp/optimized.ser" -O2 > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL unsupported optimization accepted'; exit 1; fi
grep -Fq 'Unknown option' "$tmp/err"; ok 'unsupported optimization rejected'
expect_output 'leading zero decimal literals' $'show 008\nshow 00012' $'8\n12'
expect_output 'precedence and parentheses' $'show 2 + 3 * 4\nshow (2 + 3) * 4\nshow 20 / 3\nshow 20 % 3' $'14\n20\n6\n2'
expect_output 'signed arithmetic and wrapping' $'show -7 / 3\nshow -7 % 3\nshow 9223372036854775807 + 1' $'-2\n-1\n-9223372036854775808'
expect_output 'integer bitwise operations' $'show 5 & 3\nshow 5 | 2\nshow 5 ^ 3\nshow ~0\nshow -8 >> 2\nshow 1 << 63' $'1\n7\n6\n-1\n-2\n-9223372036854775808'
expect_output 'bitwise precedence' $'show 1 | 2 ^ 3 & 6\nshow 3 + 1 << 2\nshow 8 >> 1 + 1\nshow (7 & 3) == 3' $'1\n16\n2\ntrue'
expect_error 'bitwise types' 'show true & 3' 'E106'
expect_error 'bitwise unary type' 'show ~true' 'E106'
expect_output 'float arithmetic and literals' $'show 1.5 + 2.25\nshow 1e2 - 3.5\nshow 1.5 * 2.0\nshow 1.5 / 2.0\nshow -0.0' $'3.75\n96.5\n3\n0.75\n-0'
expect_output 'float comparisons and NaN' $'let n = 0.0 / 0.0\nshow 1.0 < 2.0\nshow 1.0 >= 1.0\nshow n == n\nshow n != n\nshow n < 1.0\nshow n <= 1.0\nshow n > 1.0\nshow n >= 1.0' $'true\ntrue\nfalse\ntrue\nfalse\nfalse\nfalse\nfalse'
expect_output 'typed float function' $'task avg(a:float,b:float) -> float {\n give (a + b) / 2.0\n}\nlet x: float = avg(1.5, 2.25)\nshow x' '1.875'
expect_error 'mixed numeric types rejected' 'show 1 + 1.0' 'E106'
expect_error 'float remainder rejected' 'show 1.0 % 2.0' 'E106'
expect_error 'float overflow literal' 'show 1e9999' 'E120'
expect_error 'malformed float exponent' 'show 1e+' 'L005'
expect_error 'missing fractional digit' 'show 1.' 'L005'
expect_output 'comparisons and booleans' $'show 3 < 4\nshow 3 >= 4\nshow true == !false\nshow false != true' $'true\nfalse\ntrue\ntrue'
expect_output 'passing assertion' $'assert 1 + 1 == 2\nshow 7' '7'
expect_error 'assertion type' 'assert 1' 'E107'
expect_output 'short circuit' $'show false && (1 / 0 == 0)\nshow true || (1 / 0 == 0)' $'false\ntrue'
expect_output 'text and escapes' $'let name: text = "Hello"\nshow name\nshow "line\\nnext"\nshow "100%"' $'Hello\nline\nnext\n100%'
expect_output 'mutation and inferred types' $'let mut x: int = 10\nx = x + 3\nlet flag = true\nshow x\nshow flag' $'13\ntrue'
expect_output 'branches and nested lexical scopes' $'let x = 3\nwhen x > 1 {\n let x = 7\n when x == 7 {\n  show x\n }\n} otherwise {\n show 0\n}\nshow x' $'7\n3'
expect_output 'else on new line' $'when false {\n show 0\n}\notherwise {\n show 1\n}' '1'
expect_output 'ranges nested and end-exclusive' $'let mut sum = 0\nrepeat i from 1 to 4 {\n repeat j from 0 to 2 {\n  sum = sum + i + j\n }\n}\nshow sum\nrepeat i from 2 to 2 {\n show i\n}' '15'
expect_output 'loop bounds evaluated once' $'let mut end = 3\nrepeat i from 0 to end {\n end = 0\n show i\n}' $'0\n1\n2'
expect_output 'comments' $'# comment\nshow 42 # tail' '42'
expect_output 'loop stress' $'let mut sum = 0\nrepeat i from 0 to 1000000 {\n sum = sum + i\n}\nshow sum' '499999500000'
expect_error 'undefined variable' 'show missing' 'E102'
expect_error 'immutable assignment' $'let x = 1\nx = 2' 'E108'
expect_error 'type mismatch' 'let x: bool = 1' 'E107'
expect_error 'invalid binary operands' 'show true + 1' 'E106'
expect_error 'condition type' 'when 1 { show 1 }' 'E107'
expect_error 'duplicate declaration' $'let x = 1\nlet x = 2' 'E104'
expect_error 'block scope' $'when true {\n let x = 1\n}\nshow x' 'E102'
expect_error 'immutable loop variable' $'repeat i from 0 to 2 {\n i = 9\n}' 'E108'
expect_error 'bad token' 'show @' 'L001'
expect_error 'bad escape' 'show "bad\q"' 'L003'
expect_error 'unterminated string' 'show "bad' 'L002'
expect_error 'missing brace' $'when true {\n show 1' 'P003'
expect_error 'integer literal overflow' 'show 9223372036854775808' 'E105'
expect_error 'missing function return type' 'task add(a: int) { }' 'P001'
expect_output 'recursive tasks' $'task fact(n: int) -> int {\n when n <= 1 {\n  give 1\n } otherwise {\n  give n * fact(n - 1)\n }\n}\nshow fact(10)' '3628800'
expect_output 'forward calls and multiple arguments' $'show sum(1, 2, 3, 4, 5, 6, 7, 8)\ntask sum(a:int,b:int,c:int,d:int,e:int,f:int,g:int,h:int) -> int {\n give a+b+c+d+e+f+g+h\n}' '36'
expect_output 'nested calls and stack alignment' $'task id(n:int) -> int {\n show n\n give n\n}\ntask add(a:int,b:int) -> int {\n give a+b\n}\nshow 100 + add(id(1), id(2))' $'1\n2\n103'
expect_output 'text and bool return' $'task greeting() -> text {\n give "hello"\n}\ntask yes() -> bool {\n give true\n}\nshow greeting()\nshow yes()' $'hello\ntrue'
expect_output 'break and continue' $'var sum = 0\nrepeat i from 0 to 10 {\n when i == 3 {\n  continue\n }\n when i == 6 {\n  break\n }\n sum = sum + i\n}\nshow sum' '12'
expect_output 'nested loop break targets inner loop' $'repeat i from 0 to 2 {\n repeat j from 0 to 3 {\n  when j == 1 {\n   break\n  }\n  show i\n }\n}' $'0\n1'
expect_output 'function call short circuit' $'task fail() -> bool {\n give 1 / 0 == 1\n}\nshow false && fail()' 'false'
expect_output 'while, break and continue' $'var x = 0\nvar sum = 0\nwhile x < 10 {\n x = x + 1\n when x == 3 {\n  continue\n }\n when x == 6 {\n  break\n }\n sum = sum + x\n}\nshow sum' '12'
expect_output 'const literal expressions' $'const MAX_SIZE = 100 + 2 * 3\nconst FLAG = !false\nshow MAX_SIZE\nshow FLAG' $'106\ntrue'
expect_error 'const rejects variables' $'let x = 1\nconst Y = x' 'E119'
expect_error 'const rejects side effects' $'task x() -> int {\n give 1\n}\nconst Y = x()' 'E119'
expect_error 'while condition type' $'while 1 {\n break\n}' 'E107'
expect_error 'unknown function' 'show missing(1)' 'E110'
expect_error 'function arity' $'task x(a:int) -> int {\n give a\n}\nshow x()' 'E111'
expect_error 'function argument type' $'task x(a:int) -> int {\n give a\n}\nshow x(true)' 'E112'
expect_error 'function return mismatch' $'task x() -> int {\n give false\n}' 'E107'
expect_error 'missing return path' $'task x(a:bool) -> int {\n when a {\n  give 1\n }\n}' 'E116'
expect_error 'task parameter immutable' $'task x(a:int) -> int {\n a = 1\n give a\n}' 'E108'
expect_error 'task cannot capture main scope' $'let x = 1\ntask f() -> int {\n give x\n}' 'E102'
expect_error 'nested task rejected' $'when true {\n task x() -> int {\n  give 0\n }\n}' 'E115'
expect_error 'give outside task' 'give 1' 'E114'
expect_error 'break outside loop' 'break' 'E113'
expect_error 'continue outside loop' 'continue' 'E113'
expect_error 'duplicate tasks' $'task x() -> int {\n give 0\n}\ntask x() -> int {\n give 1\n}' 'E104'
expect_error 'invalid type' 'let x: mystery = 1' 'E109'
printf 'show 99\nshow 1 + (2 / 0)\n' > "$tmp/test.ser"
if "$serseg" run "$tmp/test.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL division by zero'; exit 1; fi
grep -Fq 'invalid integer division' "$tmp/err"; tr -d '\r' < "$tmp/out" > "$tmp/normalized"; grep -Fxq '99' "$tmp/normalized"; ok 'nested division by zero preserves earlier output'
printf 'show 17\nassert false\n' > "$tmp/test.ser"
if "$serseg" run "$tmp/test.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL assertion'; exit 1; fi
grep -Fq 'assertion failed' "$tmp/err"; tr -d '\r' < "$tmp/out" > "$tmp/normalized"; grep -Fxq '17' "$tmp/normalized"; ok 'failed assertion returns error and preserves output'
printf 'show 42\nshow 1 << 64\n' > "$tmp/test.ser"
if "$serseg" run "$tmp/test.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL shift too large'; exit 1; fi
grep -Fq 'invalid shift count' "$tmp/err"; tr -d '\r' < "$tmp/out" > "$tmp/normalized"; grep -Fxq '42' "$tmp/normalized"; ok 'shift too large runtime error'
printf 'show 1 >> -1\n' > "$tmp/test.ser"
if "$serseg" run "$tmp/test.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL negative shift'; exit 1; fi
grep -Fq 'invalid shift count' "$tmp/err"; ok 'negative shift runtime error'
printf 'show (0 - 9223372036854775807 - 1) / -1\n' > "$tmp/test.ser"
if "$serseg" run "$tmp/test.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL division overflow'; exit 1; fi
grep -Fq 'invalid integer division' "$tmp/err"; ok 'division overflow runtime error'
"$serseg" check "$root/examples/hello.ser" >/dev/null; ok 'check example'
"$serseg" build "$root/examples/hello.ser" -o "$tmp/hello$suffix" >/dev/null
file "$tmp/hello$suffix" | grep -Fq "$format"; ok 'native executable format'
(cd / && "$tmp/hello$suffix" > "$tmp/out")
printf 'SERSEG\nSum is correct\n10\n' > "$tmp/expected"; tr -d '\r' < "$tmp/out" > "$tmp/normalized"; diff -u "$tmp/expected" "$tmp/normalized"; ok 'independent execution away from compiler'
sha256sum "$tmp/hello$suffix" > "$tmp/hash"
if "$serseg" build "$root/examples/hello.ser" -o "$tmp/hello$suffix" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL overwrite protection'; exit 1; fi
sha256sum -c "$tmp/hash" >/dev/null; ok 'existing output protected'
cp "$root/examples/hello.ser" "$tmp/bad.serseg"; if "$serseg" check "$tmp/bad.serseg" > /dev/null 2>&1; then echo 'FAIL extension restriction'; exit 1; fi; ok 'source extension restriction'
"$serseg" --version | grep -q 'SERSEG 0.2.0'; "$serseg" help | grep -q 'serseg check'; ok 'version and help'
if "$serseg" install > /dev/null 2>&1; then echo 'FAIL unknown command'; exit 1; fi; ok 'unimplemented commands rejected'
mkdir -p "$tmp/modules"
printf 'use math\nuse math\nshow square(9)\n' > "$tmp/modules/main.ser"
printf 'use helper\ntask square(n:int) -> int {\n give multiply(n,n)\n}\n' > "$tmp/modules/math.ser"
printf 'task multiply(a:int,b:int) -> int {\n give a*b\n}\n' > "$tmp/modules/helper.ser"
"$serseg" check "$tmp/modules/main.ser" > /dev/null
"$serseg" run "$tmp/modules/main.ser" > "$tmp/out"
tr -d '\r' < "$tmp/out" > "$tmp/normalized"; grep -Fxq '81' "$tmp/normalized"; ok 'recursive modules and deduplicated imports'
printf 'use absent\n' > "$tmp/modules/main.ser"
if "$serseg" check "$tmp/modules/main.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL missing module'; exit 1; fi
grep -Fq M101 "$tmp/err"; grep -Fq 'main.ser:1:1' "$tmp/err"; ok 'missing module diagnostic'
printf 'use math\n' > "$tmp/modules/main.ser"; printf 'use main\n' > "$tmp/modules/math.ser"
if "$serseg" check "$tmp/modules/main.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL circular import'; exit 1; fi
grep -Fq M102 "$tmp/err"; grep -Fq 'math.ser:1:1' "$tmp/err"; ok 'circular module detection'
printf 'use math\n' > "$tmp/modules/main.ser"; printf 'show 1\n' > "$tmp/modules/math.ser"
if "$serseg" check "$tmp/modules/main.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL module top level'; exit 1; fi
grep -Fq M103 "$tmp/err"; grep -Fq 'math.ser:1:1' "$tmp/err"; ok 'module rejects top-level executable statements'
printf 'task broken() -> int {\n give missing\n}\n' > "$tmp/modules/math.ser"
if "$serseg" check "$tmp/modules/main.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL imported semantic error'; exit 1; fi
grep -Fq E102 "$tmp/err"; grep -Fq 'math.ser:2:' "$tmp/err"; grep -Fq 'give missing' "$tmp/err"; ok 'imported semantic error has correct source'
printf 'task broken() -> int {\n show @\n give 1\n}\n' > "$tmp/modules/math.ser"
if "$serseg" check "$tmp/modules/main.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL imported lexer error'; exit 1; fi
grep -Fq L001 "$tmp/err"; grep -Fq 'math.ser:2:' "$tmp/err"; ok 'imported lexer error has correct source'
printf 'use math\nuse helper\n' > "$tmp/modules/main.ser"
printf 'task same() -> int {\n give 1\n}\n' > "$tmp/modules/math.ser"
printf 'task same() -> int {\n give 2\n}\n' > "$tmp/modules/helper.ser"
if "$serseg" check "$tmp/modules/main.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL duplicate imported task'; exit 1; fi
grep -Fq E104 "$tmp/err"; ok 'duplicate imported tasks rejected'
printf 'when true {\n use math\n}\n' > "$tmp/modules/main.ser"
if "$serseg" check "$tmp/modules/main.ser" > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL nested import'; exit 1; fi
grep -Fq E121 "$tmp/err"; ok 'imports restricted to top level'
if "$serseg" doctor > "$tmp/out" 2> "$tmp/err"; then echo 'FAIL doctor should report missing components'; exit 1; fi
grep -Fq '[ok] Math standard library module:' "$tmp/out"; grep -Fq '[missing] Other standard library modules and general-purpose managed objects' "$tmp/out"; grep -Fq '[ok] GCC found:' "$tmp/out"; ok 'doctor reports present and missing components'
(cd "$tmp" && "$serseg" new MyProject > /dev/null)
[ -f "$tmp/MyProject/serseg.toml" ] && [ -f "$tmp/MyProject/README.md" ] && [ -f "$tmp/MyProject/src/main.ser" ]
"$serseg" run "$tmp/MyProject/src/main.ser" > "$tmp/out"
tr -d '\r' < "$tmp/out" > "$tmp/normalized"; grep -Fxq 'Hello from SERSEG!' "$tmp/normalized"; ok 'project scaffold builds and runs'
if (cd "$tmp" && "$serseg" new MyProject > /dev/null 2>&1); then echo 'FAIL existing project protection'; exit 1; fi
[ -f "$tmp/MyProject/src/main.ser" ]; ok 'existing project protected'
(cd "$tmp/MyProject" && "$serseg" run > "$tmp/out")
tr -d '\r' < "$tmp/out" > "$tmp/normalized"; grep -Fxq 'Hello from SERSEG!' "$tmp/normalized"; ok 'default project entry point'

mkdir -p "$tmp/suite/tests/nested"
printf 'assert 1 + 1 == 2\n' > "$tmp/suite/tests/nested/pass.ser"
(cd "$tmp/suite" && "$serseg" test > "$tmp/out" 2> "$tmp/err")
grep -Fq '1 passed; 0 failed' "$tmp/out"; ok 'test runner discovers nested passing program'
printf 'assert false\n' > "$tmp/suite/tests/fail.ser"
if (cd "$tmp/suite" && "$serseg" test > "$tmp/out" 2> "$tmp/err"); then echo 'FAIL test runner did not fail'; exit 1; fi
grep -Fq '1 passed; 1 failed' "$tmp/out"; grep -Fq 'assertion failed' "$tmp/err"; ok 'test runner reports failing assertion'
printf 'show missing\n' > "$tmp/suite/tests/invalid.ser"
if (cd "$tmp/suite" && "$serseg" test > "$tmp/out" 2> "$tmp/err"); then echo 'FAIL test runner accepted invalid program'; exit 1; fi
grep -Fq '1 passed; 2 failed' "$tmp/out"; grep -Fq 'E102' "$tmp/err"; ok 'test runner reports compilation failure'
mkdir -p "$tmp/empty/tests"
if (cd "$tmp/empty" && "$serseg" test > "$tmp/out" 2> "$tmp/err"); then echo 'FAIL empty test suite'; exit 1; fi
grep -Fq 'No .ser test files' "$tmp/err"; ok 'test runner rejects empty suite'
{ printf 'show 1'; for ((i=0;i<260;i++)); do printf ' + 1'; done; printf '\n'; } > "$tmp/test.ser"
if "$serseg" check "$tmp/test.ser" > /dev/null 2> "$tmp/err"; then echo 'FAIL expression limit'; exit 1; fi
grep -Fq P007 "$tmp/err"; ok 'expression complexity limit'
{ for ((i=0;i<130;i++)); do printf 'when true {\n'; done; printf 'show 1\n'; for ((i=0;i<130;i++)); do printf '}\n'; done; } > "$tmp/test.ser"
if "$serseg" check "$tmp/test.ser" > /dev/null 2> "$tmp/err"; then echo 'FAIL block nesting limit'; exit 1; fi
grep -Fq P008 "$tmp/err"; ok 'block nesting limit'
{ for ((i=0;i<401;i++)); do printf 'let n%s = 1\n' "$i"; done; } > "$tmp/test.ser"
if "$serseg" check "$tmp/test.ser" > /dev/null 2> "$tmp/err"; then echo 'FAIL frame slot limit'; exit 1; fi
grep -Fq E117 "$tmp/err"; ok 'local frame size limit'
printf '\n%d tests passed\n' "$pass"
