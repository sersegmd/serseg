# Change log

## Unreleased

- Added text search, ASCII trim, byte slicing, validated UTF-8 code-point length and slicing, numeric validation and decimal float conversion.
- Added chained `otherwise when` conditions.

- Added `len(text)`, `str(int|float|bool)`, `to_int(text)`, and optional input prompts; runtime-created text continues to use reference counting.
- Added a Windows per-user install/uninstall script and a VS Code syntax/snippets extension with an installation script; these have not been tested on Windows or in VS Code.

- Hardened Windows process creation and Unicode path handling, added UTF-8 BOM and CRLF regression coverage, automatic `.exe` output suffix, an 8 MiB PE stack reserve, and console UTF-8 setup. Windows execution and CI remain unverified.

- Added `each` iteration for integer arrays, byte length for text, and line input.
- Compiled the embedded C runtime with GCC `-O1` under the SERSEG `-O1` option; added a reproducible narrow iteration benchmark.

- Added growable integer arrays with indexed mutation, bounds checks, shared aliases, and reference-counted lifetimes.
- Added text equality.
- Added immutable text concatenation with a reference-counted text runtime embedded in generated executables.
- Added optional `-O1` constant folding for supported integer and boolean literal expressions; `-O0` remains the default.
- Renamed the language, executable, source extension, and project configuration to SERSEG.
- Added a per-user Linux compiler installer and safe uninstaller.
- Added limited file imports with cycle detection, imported-source diagnostics, and a default `serseg run` entry point.

- Added binary64 floating-point operations, integer bitwise operations with checked shift counts, assertions, a test-file runner, and partial doctor diagnostics.

## Experimental bootstrap 0.2.0

- Added typed top-level tasks, calls, recursion, `var`, limited `const`, while loops, break/continue, complexity limits, and safe project scaffolding.
- Extended the native assembly generator with an unverified Windows code path and added a cross-platform CI workflow, which has not run.
- Expanded Linux tests. This is not SERSEG 1.0; see `docs/STATUS.md` and `docs/VALIDATION.md`.
