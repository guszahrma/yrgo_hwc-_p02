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
| Build Zephyr apps for the Arduino Nano ESP32 | `tools/zephyr_setup.sh`, then `zephyr/app` and `zephyr/samples/blinky` with the Zephyr version from `west.yml` |
| Build Zephyr apps for the Daisy Seed | `tools/zephyr_setup.sh`, then Zephyr's `samples/basic/blinky` for `daisy_seed`, and `zephyr/app` with MCUboot (`--sysbuild`) for `daisy_seed/stm32h750xx/ram_load` |
| Run unit tests on the host | Builds and runs the GoogleTest suite in `tests/` |
| Flash and test on the Nano ESP32 | On push only, on a self-hosted runner: flashes the Ubuntu ESP-IDF build on a Nano ESP32 and runs `tools/hil/nano_esp32_test.py` (see below) |

The firmware jobs upload the built images as artifacts (kept for 3 days).

### Hardware test on a self-hosted runner

The job *Flash and test on the Nano ESP32* runs on a PC with a Nano attached, not on GitHub's machines.
It runs only on push: a pull request from a fork would otherwise run its code on that PC. Until a runner
is online, the job waits in the queue (GitHub gives up after 24 hours); the other jobs are not affected.

Setting up a PC as the runner (WSL, once):

1. The Nano works with ESP-IDF on this PC: [ESP-IDF 6.0 in `~/esp/esp-idf`](platform_esp_idf.md) and
   [the udev rules](setup.md#fixed-port-names) (`/dev/tty_arduino_nano_esp32`).
2. On GitHub: **Settings → Actions → Runners → New self-hosted runner**, Linux x64. Run the commands it
   shows in a folder outside the repository (e.g. `~/runners/p02/actions-runner`). `config.sh` asks:
   * runner group: press Enter (`Default`; personal repositories have no other groups),
   * runner name: e.g. `p02_nano_<pc name>`,
   * labels: `nano-esp32` – the job asks for a runner with this label,
   * work folder: press Enter.
3. Start the runner with `./run.sh` (stop it with `Ctrl+C`), or as a service with
   `sudo ./svc.sh install && sudo ./svc.sh start` (needs systemd in WSL).

While the job runs:

* The Nano must be attached to WSL (`usbipd attach`) and the port free: quit `tio` first, or the job fails.
* The job flashes the board, so whatever was on it (Zephyr, a test build) is replaced.

The test can also be run by hand after flashing:

```bash
. ~/esp/esp-idf/export.sh
python tools/hil/nano_esp32_test.py --port /dev/tty_arduino_nano_esp32
```
