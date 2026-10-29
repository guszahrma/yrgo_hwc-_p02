# Contributing

How we work: branches and pull requests, tests, code style and CI.

## Workflow

* One feature or fix per branch and pull request. The other person reviews before merging.
* Merge with **"Create a merge commit"** and delete the branch. For stacked PRs (a PR based on another
  PR's branch), merge bottom-up and retarget the next PR to `main`.
* Keep a branch up to date with `git merge origin/main`, not rebase, once it has been pushed.
* Commit first, then merge `main` – your work is safe in a commit, and `git merge --abort` can always undo
  the merge.

## Tests

The unit tests in `tests/` use **GoogleTest** (git submodule `tests/googletest`) and run on the host with
`gcc`/`g++`. `TESTSUITE` is defined, which selects `arch/env/test/hw_platform.h`, and hardware calls go to
mocks (e.g. `tests/source/gpio/gpio_mock.c` for the ESP-IDF GPIO API). The compilers are set in
`tests/CMakeLists.txt`, so an active ESP-IDF or Zephyr environment does not change them.

```bash
git submodule update --init tests/googletest   # once, if the repository was not cloned with --recursive
cmake -S tests -B tests/build
cmake --build tests/build
```

The tests run automatically as part of the build (target `run`); a failing test fails the build.

To add a test: write `TEST(...)` cases in `tests/source/`, and add the test file and the code under test to
`TST_FILES` / `SRC_FILES` in `tests/CMakeLists.txt`.

## Code style

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

Run `tools/clang-format/fix.sh` before committing.

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

## CI

CI (`.github/workflows/ci.yml`) runs on every push and pull request:

| Job | What it does |
|---|---|
| Check code formatting (Ubuntu and Windows) | clang-format with the pinned version, via the scripts in `tools/clang-format/` |
| Build ESP32-S3 firmware on Ubuntu | ESP-IDF 6.0 build in the Docker image `espressif/idf:release-v6.0` |
| Build ESP32-S3 firmware on Windows | ESP-IDF 6.0 installed on the runner, then the same build |
| Build Zephyr apps | `tools/zephyr_setup.sh`, then `zephyr/app` and `zephyr/samples/blinky` with the Zephyr version from `west.yml` |
| Run unit tests on the host | Builds and runs the GoogleTest suite in `tests/` |

The firmware jobs upload the built images as artifacts (kept for 3 days).
