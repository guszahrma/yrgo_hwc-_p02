#!/usr/bin/env bash
# Checks that modules/fuz points to a commit on the module's main branch (see AGENTS.md: Do not
# touch). Clones only the module's main (bare, without file contents) and checks with
# git merge-base --is-ancestor that the commit is on it.
#
#   tools/check_fuz_submodule.sh          fails if it is not (pull requests to main)
#   tools/check_fuz_submodule.sh --warn   only warns (every push, while a change spans both
#                                         repositories)
set -euo pipefail

level=error
[[ "${1:-}" == "--warn" ]] && level=warning

root="$(git rev-parse --show-toplevel)"
pin="$(git -C "$root" ls-tree HEAD modules/fuz | awk '{print $3}')"
url="$(git -C "$root" config -f .gitmodules submodule.modules/fuz.url)"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

git clone --quiet --bare --filter=blob:none --single-branch --branch main "$url" "$tmp/fuz"
if git -C "$tmp/fuz" merge-base --is-ancestor "$pin" main 2>/dev/null; then
    echo "modules/fuz at $pin, on the module's main"
    exit 0
fi

echo "::$level file=.gitmodules::modules/fuz points to $pin, which is not on the module's main." \
    "Merge the module's pull request first, then move the submodule to the merged commit."
[[ "$level" == "warning" ]]
