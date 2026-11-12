# Status

## Implemented and tested on Linux x86-64

- Existing lexer, parser, AST, scoped name resolution, type checker and native assembly backend, extended with typed tasks, calls, recursion, `var`, limited `const`, `while`, `break`, `continue` integer bitwise operations, and binary64 floating-point operations, and a limited real file-based import graph with cycle detection.
- `version`, `help`, `new`, `check`, `run` and `build` commands, partial `doctor` diagnostics, `assert` and a file-based `test` runner. `new` creates a starter project; its `serseg.toml` is metadata only and is not read.
- `each` iteration over int, float, bool and text arrays, byte length for text, line input via `input()` or `input(text)`, and text conversions, validation, search, byte and UTF-8 code-point slicing; chained `otherwise when` branches.
- `-O0` and a limited `-O1` typed-AST constant-folding pass for integer and boolean literals plus GCC `-O1` for the embedded C runtime; arithmetic wraps consistently with the unoptimized backend.
- Immutable text concatenation and equality, growable int, float, bool and text arrays, and automatic reference-counted lifetime management for those values via a limited embedded runtime; see `MEMORY.md`.
- A limited per-user Linux compiler installer and uninstaller; its general-purpose managed runtime is not implemented.
- Narrow automated tests described in `VALIDATION.md`. Native output is Linux ELF dynamically linked against glibc and the system loader.

## Implemented but unverified

- Basic math standard-library module and module-qualified calls; tasks still share one namespace.
- VS Code diagnostics and build/run integration were added and statically checked but have not been tested in VS Code.
- Windows-specific process/temp handling and x86-64 ABI/PE-oriented assembly, Windows PowerShell installation/uninstallation scripts, and VS Code syntax/snippet extension and installation script. The compiler and process-test source have been cross-compiled to Windows PE on Linux, and a Windows backend example has been cross-linked; none has been run or validated on Windows, and must not be treated as supported releases. The GitHub Actions workflow includes Linux and Windows jobs but has not run.

## Not implemented

- LLVM/portable IR, IR verification, other requested optimization passes and levels, LTO, supported end-user cross-compilation, and proven Windows executable generation.
- Most language features: chars, bytes, nested arrays, slicing, general collections, optional values, structures, enums, module namespaces and visibility, error types, generics, closures, and concurrency.
- General object runtime, cycle handling, general memory-safety guarantees, most standard library modules, self-hosting.
- Project config parsing, package manager, dependency resolution/cache/lockfile, `fmt`, `clean`, `install`, and `update` CLI commands; full doctor integrity checks and a richer test framework.
- VS Code LSP and UI-validated VSIX installation, a validated Windows installer/uninstaller, complete Linux distribution, a validated Windows release, executed Windows tests, cross-platform compatibility tests, and comparative benchmarks.
- Full docs and all mandatory clean-install checks. No 1.0 release artifacts exist.

No third-party code is vendored. No project license has been selected; obtain appropriate permission before redistribution. A Windows implementation must be built and tested on an actual Windows environment before any support claim. See `CI.md` for the workflow and current access blocker.
