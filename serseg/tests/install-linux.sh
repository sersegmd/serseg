#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
prefix="$tmp/user prefix"
"$root/scripts/install-linux.sh" "$prefix" > /dev/null
"$prefix/bin/serseg" --version | grep -q 'SERSEG 0.2.0'
"$prefix/bin/serseg" run "$root/examples/modules/main.ser" | grep -qx 81
echo 'PASS Linux per-user install and native execution'
if "$root/scripts/install-linux.sh" "$prefix" > /dev/null 2>&1; then echo 'FAIL overwrite check'; exit 1; fi
echo 'PASS Linux installer refuses existing installation'
mkdir -p "$prefix/lib/serseg/user-data"; echo keep > "$prefix/lib/serseg/user-data/keep"
"$prefix/lib/serseg/uninstall.sh" "$prefix" > /dev/null
[ ! -e "$prefix/bin/serseg" ] && [ -f "$prefix/lib/serseg/user-data/keep" ]
echo 'PASS Linux uninstall preserves unrelated files'
rm "$prefix/lib/serseg/user-data/keep"; rmdir "$prefix/lib/serseg/user-data" "$prefix/lib/serseg"
"$root/scripts/install-linux.sh" "$prefix" > /dev/null
printf 'changed\n' >> "$prefix/lib/serseg/README.md"
if "$prefix/lib/serseg/uninstall.sh" "$prefix" > /dev/null 2>&1; then echo 'FAIL modified file protection'; exit 1; fi
[ -L "$prefix/bin/serseg" ] && [ -x "$prefix/lib/serseg/bin/serseg" ] && grep -q changed "$prefix/lib/serseg/README.md"
echo 'PASS Linux uninstall refuses modified files'
