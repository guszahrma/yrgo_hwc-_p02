#!/usr/bin/env bash
# Runs clang-format at the version pinned in version.txt, installing it into .venv if needed.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
version="$(tr -d '[:space:]' < "$root/tools/clang-format/version.txt")"
venv="$root/.venv"
bin="$venv/bin/clang-format"

if [[ ! -x "$bin" ]] || [[ ! "$("$bin" --version)" =~ "version $version"([^0-9.]|$) ]]; then
    echo "Installing clang-format $version into $venv" >&2
    [[ -x "$venv/bin/python" ]] || python3 -m venv "$venv"
    "$venv/bin/python" -m pip install --quiet --disable-pip-version-check "clang-format==$version"
fi

exec "$bin" "$@"
