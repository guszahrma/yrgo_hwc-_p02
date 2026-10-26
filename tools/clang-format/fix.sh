#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/../.."

mapfile -t files < <(git ls-files -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx')

if [[ ${#files[@]} -eq 0 ]]; then
    echo 'No tracked C/C++ files found.' >&2
    exit 1
fi

tools/clang-format/run.sh -i --style=file -- "${files[@]}"
