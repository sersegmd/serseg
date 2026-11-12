#!/usr/bin/env bash
# Verify owned files before removal and leave all unrecognized files untouched.
set -euo pipefail
prefix=${1:-"$HOME/.local"}
case "$prefix" in /*) ;; *) echo 'Error: prefix must be an absolute path' >&2; exit 1;; esac
[ "$prefix" != / ] || { echo 'Error: refusing prefix /' >&2; exit 1; }
lib="$prefix/lib/serseg"; link="$prefix/bin/serseg"
[ -d "$lib" ] && [ ! -L "$lib" ] && [ -f "$lib/receipt" ] && [ ! -L "$lib/receipt" ] || { echo 'Error: installation receipt not found or unsafe' >&2; exit 1; }
mapfile -t expected < "$lib/receipt"
[ "${#expected[@]}" -eq 3 ] || { echo 'Error: invalid receipt; no files removed' >&2; exit 1; }
files=(bin/serseg uninstall.sh README.md)
for i in 0 1 2; do
  f="$lib/${files[$i]}"
  [[ ${expected[$i]} =~ ^[a-f0-9]{64}$ ]] && [ -f "$f" ] && [ ! -L "$f" ] && [ "$(sha256sum "$f" | cut -d ' ' -f 1)" = "${expected[$i]}" ] || { echo "Error: installed file changed or missing: $f; no files removed" >&2; exit 1; }
done
if [ -L "$link" ]; then
  [ "$(readlink "$link")" = '../lib/serseg/bin/serseg' ] || { echo 'Error: command link changed; no files removed' >&2; exit 1; }
elif [ -e "$link" ]; then echo 'Error: command path is not the installed link; no files removed' >&2; exit 1
fi
[ ! -L "$lib/bin" ] || { echo 'Error: installation bin directory is a symlink; no files removed' >&2; exit 1; }
[ ! -L "$link" ] || rm -- "$link"
rm -- "$lib/bin/serseg" "$lib/uninstall.sh" "$lib/README.md" "$lib/receipt"
rmdir -- "$lib/bin" 2>/dev/null || true
if ! rmdir -- "$lib" 2>/dev/null; then echo "Preserved additional files in $lib"; fi
printf 'Removed installer-owned SERSEG files. User projects were not touched.\n'
