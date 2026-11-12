# Linux per-user installation

This installs the currently implemented compiler only, not a SERSEG 1.0 distribution. A standard library and general-purpose managed runtime are not implemented; generated programs embed a limited text and integer-array runtime. Tested on Amazon Linux 2023 x86-64; other distributions have not been validated.

Prerequisites: Bash, GNU coreutils (`install`, `mv`, `sha256sum`, `mktemp`), Make, a C++17 compiler, GCC, assembler/linker, and glibc development files. The compiler and generated executables have the dependencies described in the README. Run from the `serseg` source directory:

```sh
make
make test
./scripts/install-linux.sh
"$HOME/.local/bin/serseg" --version
```

The script installs into `$HOME/.local/lib/serseg` and links `$HOME/.local/bin/serseg`. It refuses to replace an existing installation or command. If `$HOME/.local/bin` is not on your PATH, make it available in the current shell with:

```sh
export PATH="$HOME/.local/bin:$PATH"
serseg --version
serseg new MyProject
cd MyProject
serseg run
```

Add that export to the appropriate shell startup file yourself if desired; the installer does not modify PATH or shell configuration. To use a different absolute prefix, pass it as the sole argument to both installation and uninstallation.

Uninstall with:

```sh
"$HOME/.local/lib/serseg/uninstall.sh"
```

The uninstaller verifies its receipt and installed files before removal, removes only its own verified files and matching command link, and leaves extra files and user projects untouched. It refuses removal when its installed files or command link have changed. A failure while creating the command link may leave the installation directory in place; follow the printed uninstall command. The installer does not manage dependencies or install a standard library.
