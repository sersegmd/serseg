#!/usr/bin/env bash
# Install only files owned by this script; never overwrite an existing installation.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
prefix=${1:-"$HOME/.local"}
case "$prefix" in /*) ;; *) echo 'Error: prefix must be an absolute path' >&2; exit 1;; esac
[ "$prefix" != / ] || { echo 'Error: refusing to install under /' >&2; exit 1; }
[ -f "$root/build/serseg" ] && [ -x "$root/build/serseg" ] || { echo 'Error: run make before installing' >&2; exit 1; }
mkdir -p "$prefix/bin" "$prefix/lib"
lib="$prefix/lib/serseg"; link="$prefix/bin/serseg"
if [ -e "$lib" ] || [ -L "$lib" ] || [ -e "$link" ] || [ -L "$link" ]; then
  echo 'Error: SERSEG installation path already exists; nothing was overwritten' >&2; exit 1
fi
stage=$(mktemp -d "$prefix/lib/.serseg-install.XXXXXXXX")
trap 'if [ -n "${stage:-}" ] && [ -d "$stage" ]; then rm -rf -- "$stage"; fi' EXIT
mkdir "$stage/bin"
install -m 755 "$root/build/serseg" "$stage/bin/serseg"
install -m 755 "$root/scripts/uninstall-linux.sh" "$stage/uninstall.sh"
install -m 644 "$root/README.md" "$stage/README.md"
for f in bin/serseg uninstall.sh README.md; do sha256sum "$stage/$f" | cut -d ' ' -f 1; done > "$stage/receipt"
# GNU mv -n avoids replacing an installation created concurrently.
mv -T -n -- "$stage" "$lib"
if [ -e "$stage" ]; then echo 'Error: installation path appeared concurrently' >&2; exit 1; fi
stage=''
if ! ln -s '../lib/serseg/bin/serseg' "$link"; then
  echo "Error: could not create command link; installed files remain at $lib" >&2
  echo "To remove them: $lib/uninstall.sh '$prefix'" >&2; exit 1
fi
printf 'Installed compiler to %s\n' "$lib"
printf 'Command: %s\n' "$link"
printf 'Add %s/bin to PATH if it is not already present.\n' "$prefix"
printf 'No standard library or general-purpose managed runtime is installed. Generated programs embed a limited text and integer-array runtime.\n'
