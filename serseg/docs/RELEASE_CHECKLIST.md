# SERSEG 1.0 acceptance checklist

- [x] Rename the current compiler, CLI, source extension, configuration template and examples to SERSEG.

Only check an item after its implementation and the relevant tests have actually passed on both supported platforms where applicable.

- [ ] Specify and implement the complete grammar, types, collections, structures, enums, modules, error handling and portable core semantics.
- [ ] Implement typed IR, verification, optimizations, and complete native Windows and Linux backends and linkers.
- [ ] Implement and stress-test automatic memory management including cyclic structures.
- [ ] Implement and test the full portable standard library and platform-specific integrations.
- [ ] Implement project configuration parsing, dependency resolution, lockfiles, local cache and package operations.
- [ ] Complete the CLI including doctor, test, fmt, clean, install and update.
- [ ] Implement LSP and a tested VS Code extension packaged as a VSIX.
- [ ] Build and test Windows and Linux installations, uninstallations and release artifacts.
- [ ] Pass comprehensive Linux and Windows CI, clean-install, standalone dependency and same-source cross-platform tests.
- [ ] Run comparable reproducible benchmarks and publish raw results and environment.
- [ ] Finish complete documentation, choose an authorized license and produce source and platform release archives.

Current implementation and actual verification: see `STATUS.md` and `VALIDATION.md`.
