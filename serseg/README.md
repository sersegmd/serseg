# SERSEG

**SERSEG 1.0 is not complete.** This source tree implements only a subset and is not suitable for production. The Linux x86-64 subset has been tested; Windows x86-64 code paths have been cross-compiled to PE on Linux, but have not been executed or tested on Windows; the Windows CI has not run. No cross-platform compatibility claim is made. See [status](docs/STATUS.md), [validation](docs/VALIDATION.md), [Linux installation](docs/INSTALL_LINUX.md), [Windows status in Arabic](docs/WINDOWS_AR.md), and [CI](docs/CI.md).

## Build and test on Linux x86-64

Prerequisites: GNU Make, a C++17 compiler, GCC, GNU assembler/linker, and glibc development files. Tests also require Python 3, Bash, `file`, `sha256sum`, and standard Unix tools. Python is only used for tests, not by the compiler or compiled programs.

```sh
make
make test
./build/serseg --version
./build/serseg check examples/hello.ser
./build/serseg run examples/functions.ser
./build/serseg build examples/hello.ser -o hello
./hello
```

Available commands: `--version`, `version`, `help`, `doctor`, `new NAME`, `test`, `check FILE.ser`, `run [FILE.ser] [-O0|-O1]`, and `build FILE.ser [-o OUTPUT] [-O0|-O1]`. `new` creates a working starter source, README, and a metadata-only `serseg.toml`; **this compiler does not read the configuration file**. Names contain ASCII letters, digits, underscores or hyphens and must begin with a letter or underscore. Existing directories and build outputs are never intentionally overwritten. An interrupted or failed `new` may leave a partially created directory.

`doctor` detects the compiler, GCC, PATH availability and VS Code CLI on a best-effort basis; it exits with status 1 because the standard library is limited and the general-purpose managed runtime is missing. It does not verify linker integrity or extension installation.

`serseg test` recursively discovers `.ser` files under `tests/` relative to the current directory, compiles and runs each as a separate program, reports each path and its exit status, and returns nonzero on failures or if no tests are found. `assert <bool expression>` exits the program with an error if false; it is useful in test files. For example, from `examples/`, run `../build/serseg test`. This is a basic test runner, not a full unit-testing framework.

`run` defaults to `src/main.ser` when no file is provided. It uses a temporary directory and propagates the native program's exit status. `check` parses and type-checks without assembling or linking. `.ser` is the only accepted source extension. Compilation invokes `gcc` from PATH. There is no target selection. `-O0` (the default) does not perform compiler constant folding; `-O1` folds pure literal integer arithmetic, bitwise operations, comparisons, and boolean operations in the typed AST, and compiles the embedded C runtime with GCC `-O1` (the SERSEG assembly is not optimized by GCC). Floats, division, remainder, and shifts are not folded, preserving runtime checks. `-O2`, `-O3`, `-Os`, LTO, and other requested optimizations are not supported.

The generated Linux executable is a native ELF, dynamically linked against the host glibc and ELF interpreter. It needs neither the compiler nor Python or LLVM at runtime, but is **not dependency-free**. The compiler itself dynamically links glibc, libstdc++, libgcc_s, libm and the system loader. The binary was tested on Amazon Linux 2023 x86-64; compatibility elsewhere has not been established.

## Text builtins

`len(text)` returns byte length, `str(value)` converts `int`, `float`, or `bool` to text, and `input()` or `input(prompt)` reads one line. The prompt is written and flushed without a newline. `to_int(text)` accepts only an optional leading `+` or `-` followed by ASCII decimal digits, with no whitespace; invalid input or signed 64-bit overflow terminates with a runtime error. `str(float)` uses 17 significant digits. `to_float(text)` accepts decimal digits with optional sign, fractional part and exponent (for example `-1.25e2`), rejecting malformed or out-of-range input. `is_int(text)` and `is_float(text)` validate the same formats without terminating, so programs can handle invalid input. Conversions report range errors, including underflow when the C library reports it. See `examples/text.ser` and `examples/validate-number.ser`.

`contains(text, part)`, `starts_with(text, prefix)` and `ends_with(text, suffix)` compare bytes. `trim(text)` removes ASCII whitespace at the edges. `slice(text, start, end)` uses zero-based byte offsets and an exclusive end; invalid ranges terminate and cutting through UTF-8 bytes may produce invalid text. `utf8_len(text)` counts Unicode code points, and `utf8_slice(text, start, end)` uses code-point offsets with an exclusive end; both validate the entire text as UTF-8 and terminate on invalid sequences. Code points are not necessarily user-perceived grapheme clusters; normalization is not performed. Runtime-created text uses reference counting, not a garbage collector; see [memory model](docs/MEMORY.md).

## Windows build (not yet validated on Windows)

In an MSYS2 UCRT64 shell, install `make`, `file`, `python`, `mingw-w64-ucrt-x86_64-gcc`, and `mingw-w64-ucrt-x86_64-python`; then run `make test` from the source directory. The compiler invokes GCC from PATH. The Windows process and integration tests require native Windows Python. This setup has not yet been exercised on Windows. A per-user installation script and uninstaller are provided but untested on Windows; see the Windows guide. See [Windows status](docs/WINDOWS_AR.md).

Windows code now uses UTF-16 process creation and the native temporary directory, accepts a leading UTF-8 BOM and CRLF, appends `.exe` to an extensionless explicit output name, and requests an 8 MiB PE stack reserve. Generated programs request UTF-8 output for a console and restore its previous output code page at exit; console code pages are shared, so concurrent console programs can be affected while it runs. Rendering also depends on the terminal and font. These Windows behaviors require actual Windows validation.

## Implemented language subset

Statements are separated by newlines; braces enclose blocks. `#` begins a comment outside a string. Identifiers are ASCII letters, digits and underscores, with a letter or underscore first. Identifiers and keywords are case-sensitive.

```text
task factorial(n: int) -> int {
    when n <= 1 {
        give 1
    } otherwise {
        give n * factorial(n - 1)
    }
}

const LIMIT = 5
var total = 0
repeat i from 0 to LIMIT {
    total = total + factorial(i)
}
show total
```

- `let` creates an immutable binding; `var` or `let mut` permits reassignment. `const` requires a literal-only expression, excluding division, remainder, and shifts; it is immutable. The compiler does not promise compile-time evaluation. Bindings have block scope, and nested shadowing is allowed.
- Implemented types: `int` (signed 64-bit), `float` (IEEE-754 binary64), `bool`, immutable `text` values, and growable `int_array`, `float_array`, `bool_array`, and `text_array` values. Initializers are required; annotations are optional. Values cannot change type.
- Integer operators `+ - * / %` and unary `-` are supported. Addition, subtraction, multiplication and negation wrap modulo 2^64; signed comparisons interpret bits as two's complement. Division truncates toward zero. Division or remainder by zero, or signed minimum divided by -1, terminates with a runtime error. Positive integer literals must fit in signed 64-bit range; express the minimum value as `0 - 9223372036854775807 - 1`. Leading zero literals are decimal.
- Floating-point literals include decimal fractions such as `2.5` and scientific notation such as `1e3`; a decimal point requires digits on both sides. Literal conversion must be in range. Float operands support `+ - * /`, unary `-` and comparisons; operands must have matching types, with no implicit numeric conversions. Division follows IEEE-754 behavior, including infinities and NaN. NaN is unequal to every value and all ordered comparisons with NaN are false. `show` uses 17 significant digits.
- Integer bitwise operators `&`, `|`, `^`, `~`, `<<`, `>>` operate on signed 64-bit values. Shifts require a count from 0 through 63; out-of-range counts terminate with a runtime error. Right shift is arithmetic; left shift wraps bits.
- Comparisons `< <= > >=` accept integers; `== !=` accept matching int, float, bool or text values; array equality is not supported. Boolean `!`, `&&`, `||` short-circuit. Parentheses are supported. From low to high precedence: `||`, `&&`, `|`, `^`, `&`, `== !=`, `< <= > >=`, `<< >>`, `+ -`, `* / %`, unary `! - ~`. Binary operators associate left-to-right.
- `when` accepts a bool condition and optional `otherwise`; multiple branches can use `otherwise when`. `repeat name from start to end` creates an immutable integer loop variable and an ascending, end-exclusive range. Both bounds are evaluated once. `while condition { ... }`, nested loops, `break` and `continue` are supported. `each item in array { ... }` iterates an array: the array expression and its length are evaluated once; mutations to existing elements are visible, but elements appended later are not visited. The loop variable is immutable and scoped to the loop.
- Top-level `task name(a: int) -> int { ... }` declarations support typed immutable parameters, explicit int/float/bool/text and array return types, forward calls, recursion, and `give` returns. Each control-flow path must return; return analysis is conservative. Tasks do not capture top-level variables. Function and variable names occupy separate namespaces. No overloading, void tasks, closures, default parameters or generics are implemented.
- File-based imports: a top-level `use math` loads `math.ser` from the importing file's directory. Imported files may declare tasks and other top-level imports, but no executable top-level statements. Imported tasks share one global namespace with the entry file, so duplicate task names are errors. Files are loaded once; cycles and missing modules are errors. A direct import also permits calls such as `math.min(1, 2)`; the task must belong to that directly imported file. Tasks still share one global namespace; there is no visibility control, separate compilation or package resolution. The `stdlib/math.ser` module supplies basic min, max and clamp helpers; local modules take precedence. See `examples/modules/`.
- Arrays of int, float, bool or text support `[1, 2]`, `[]`, `.length`, indexing, `push array, value`, and indexed assignment through a `var` binding. Arrays are shared references: an immutable `let` binding cannot mutate directly, but sees mutations made through a mutable alias. Indices are signed integers and are checked at runtime; invalid indices terminate with an error. Array literals must be homogeneous. An unannotated empty literal defaults to `int_array`; annotate other empty arrays, e.g. `var names: text_array = []`. Nested arrays are not supported.
- `show` prints int, float, bool, or text followed by a newline. `input()` reads a line from standard input and returns text; it strips LF and an immediately preceding CR. EOF returns empty text, indistinguishable from an empty line; input errors and NUL bytes terminate the program with an error. `text.length` returns the number of bytes, not Unicode characters. Text literals support `\n`, `\r`, `\t`, `\\`, and `\"`. `text + text` concatenates into an immutable, reference-counted byte string; values can be assigned, passed and returned. Literal data is static. See [memory model](docs/MEMORY.md). Text input and literals preserve bytes without automatic Unicode validation; the `utf8_len` and `utf8_slice` operations validate UTF-8. Unicode identifiers are not implemented. Diagnostic positions count bytes. Embedded NUL is rejected.
- Current implementation limits: 256 AST nodes per expression, 128 nested blocks, 400 local stack slots per task or main, and 128 task parameters. Recursive execution is limited by the process stack.

Nested arrays, slicing, maps, sets, string interpolation, isolated module namespaces and visibility, user-defined types, most standard library APIs, garbage collection and most other requested features do not exist yet. Unsupported syntax is an error; there is no interpreter or Python translation fallback.

## Architecture

The C++17 bootstrap reads `.ser`, loads the file-based import graph, tokenizes, builds an AST, resolves scoped names and signatures, checks types, optionally folds constant expressions, then emits x86-64 GNU assembly directly and invokes GCC to assemble and link. Linux code generation uses the System V ABI for C calls. The Windows ABI/PE-oriented path and Windows process-handling code have been cross-compiled, but are **unverified on Windows** and may require fixes. The compiler has no separate IR, general optimization pipeline or garbage collector. Its limited embedded runtime reference-counts heap-allocated concatenated text and typed arrays, but does not manage general object graphs. Function calls use a compiler-internal stack convention. The compiler is not self-hosted.

## Performance measurements

`python3 benchmarks/iteration.py build/serseg` runs one narrow optimization-level comparison. It validates output and records the environment, binary sizes, single-build times, and repeated wall times; see `benchmarks/README.md`. It is not a comparison against Python or evidence of general performance.

## VS Code syntax support

`editors/vscode` contains a syntax grammar, language configuration and snippets. On Windows, `scripts/install-vscode.ps1` copies the extension to the current user's VS Code extensions directory and refuses to overwrite an existing copy. Restart VS Code afterward. The extension invokes `serseg check` on open/save and provides build and run commands. It displays the first diagnostic; columns for non-ASCII text may be approximate. It does not provide completion beyond snippets, formatting, navigation or an LSP. A VSIX package can be built using `vsce package` from that directory. Installation and display in VS Code have not been tested.

## دليل اللغة بالعربية

راجع [دليل SERSEG بالعربية](SERSEG_GUIDE_AR.md) لقواعد اللغة وأمثلة الاستخدام والقيود الحالية.
