# Contributing

Tests, code style and CI.

## Tests

The test suite in `tests/` is built with plain `g++` and runs the drivers against mocked hardware
(`TESTSUITE` is defined, which selects `arch/env/test/hw_platform.h`). Do **not** run it in a terminal where
`export.sh` is active:

```bash
cmake -S tests -B tests/build
cmake --build tests/build
```

The tests run automatically as part of the build (target `run`).

## Code style and CI

The code is formatted with `clang-format` according to `.clang-format`. Everyone, including CI, uses the
version in `tools/clang-format/version.txt`, since different versions format the code differently.

```bash
# Linux / WSL
tools/clang-format/check.sh # check
tools/clang-format/fix.sh   # format
```

On Windows, the equivalent scripts are `tools/clang-format/check.ps1` and `tools/clang-format/fix.ps1`.

The scripts install the pinned version into `.venv/` in the repo root the first time they run (and again if the
version changes), so no system-wide clang-format is needed. To run it on single files, use `tools/clang-format/run.sh`
(`tools/clang-format/run.ps1` on Windows) the same way as `clang-format`.

To make VS Code format with the same version, add this to `.vscode/settings.json` (run one of the scripts first so
that `.venv/` exists; on Windows the path is `${workspaceFolder}/.venv/Scripts/clang-format.exe`):

```json
"C_Cpp.clang_format_path": "${workspaceFolder}/.venv/bin/clang-format"
```

Commits that only reformat code are listed in `.git-blame-ignore-revs`, so `git blame` shows who last changed
the code rather than who reformatted it. GitHub uses the file automatically; locally, run this once per clone:

```bash
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

CI (`.github/workflows/ci.yml`) runs on every push and pull request:
* format check with clang-format on Ubuntu and Windows (via the scripts in `tools/clang-format/`)
* build with ESP-IDF 6.0 on Ubuntu (Docker image `espressif/idf:release-v6.0`) and Windows
* build of the Zephyr apps (`zephyr/app`, `zephyr/samples/blinky`) with the Zephyr version from `west.yml`

Git commands and SSH setup are described in [`git_commands.md`](git_commands.md).
