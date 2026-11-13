# Contributing

How we work: branches and pull requests, tests, code style and CI.

## Workflow

* One feature or fix per branch and pull request. The other person reviews before merging.
* Merge with **"Create a merge commit"** and delete the branch. For stacked PRs (a PR based on another
  PR's branch), merge bottom-up and retarget the next PR to `main`.
* Keep a branch up to date with `git merge origin/main`, not rebase, once it has been pushed.
* Commit first, then merge `main` – your work is safe in a commit, and `git merge --abort` can always undo
  the merge.
* A new test on a board or a new measurement (an image size, a time) gets a row in
  [`report/material.md`](../report/material.md): board target, how, result, and the commit of the code that
  was tested.
* Before approving a pull request, check that `main` has been merged into the branch and pushed, and that
  CI passed. CI runs on push only, so it tests the branch, not its merge with `main`.

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

CI (`.github/workflows/ci.yml`) runs on every push, not on pull requests: a pull request comes from a pushed
branch and shows that push's checks.

| Job | What it does |
|---|---|
| Check code formatting (Ubuntu and Windows) | clang-format with the pinned version, via the scripts in `tools/clang-format/` |
| Build ESP32-S3 firmware on Ubuntu | ESP-IDF 6.0 build in the Docker image `espressif/idf:release-v6.0` |
| Build ESP32-S3 firmware on Windows | ESP-IDF 6.0 installed on the runner, then the same build |
| Build Zephyr apps for the Arduino Nano ESP32 | `tools/zephyr_setup.sh`, then `zephyr/app` with MCUboot (`--sysbuild`) and `zephyr/samples/blinky` with the Zephyr version from `west.yml` |
| Build Zephyr apps for the Daisy Seed | `tools/zephyr_setup.sh`, then Zephyr's `samples/basic/blinky` for `daisy_seed`, and `zephyr/app` with MCUboot (`--sysbuild`) for `daisy_seed/stm32h750xx/ram_load` |
| Build Zephyr apps for the Nucleo-G0B1RE | `tools/zephyr_setup.sh`, then Zephyr's `samples/basic/blinky` and `zephyr/app` with MCUboot (`--sysbuild`) for `nucleo_g0b1re` |
| Run unit tests on the host | Builds and runs the GoogleTest suite in `tests/` |
| Flash and test ESP-IDF on the Nano ESP32 | On a self-hosted runner: flashes the Ubuntu ESP-IDF build on a Nano ESP32 and runs `tools/hil/p02_test.py` (see below) |
| Flash and test Zephyr on the Nano ESP32 | The same for the Zephyr build (MCUboot and P02), on the same Nano and runner |
| Flash and test on the Nucleo-G0B1RE | On a self-hosted runner: flashes the Zephyr build (MCUboot and P02) with OpenOCD and runs `tools/hil/p02_test.py` |

The firmware jobs upload the built images as artifacts (kept for 3 days).

### Hardware test on a self-hosted runner

The *Flash and test* jobs run on a PC with the board attached, not on GitHub's machines. GitHub builds the
firmware; the PC only downloads it, flashes the board and talks to P02 over the serial port. Like all of CI
they run only on push, so a pull request from a fork never runs its code on that PC. Until a runner is
online, a job waits in the queue (GitHub gives up after 24 hours); the other jobs are not affected.

Each board has its own runner, with the board's label, so the boards are tested at the same time. A board
tested on both platforms (the Nano) has one job per platform; its runner runs them one after the other,
each flashing its own platform first:

| Board | Label | Port | Needs on the PC |
|---|---|---|---|
| Arduino Nano ESP32 | `arduino_nano_esp32` | `/dev/tty_arduino_nano_esp32` | [ESP-IDF 6.0 in `~/esp/esp-idf`](platform_esp_idf.md) of the runner's user |
| Nucleo-G0B1RE | `nucleo_g0b1re` | `/dev/tty_nucleo_g0b1re` | `sudo apt install openocd python3-serial` |

Setting up a PC as the runner (WSL, once). The runner runs whatever a workflow asks for, so it gets its own
Linux user, which cannot read your files:

1. [The udev rules](setup.md#fixed-port-names) (fixed port names, and access to the ST-Link) and the
   packages in the table.
2. A user for the runners, in `dialout` for the boards' ports and without `sudo`:

   ```bash
   sudo adduser --disabled-password --gecos "" ghrunner
   sudo usermod -aG dialout ghrunner
   ```

   Check that it cannot read your home folder (`sudo -u ghrunner ls ~` gives `Permission denied`; if not,
   `chmod 750 ~`) or the Windows drive (`sudo -u ghrunner ls /mnt/c/Users`). For the Windows drive, add
   this to `/etc/wsl.conf`, then run `wsl --shutdown` in PowerShell:

   ```ini
   [automount]
   options = "uid=1000,gid=1000,umask=077"
   ```

3. On GitHub: **Settings → Actions → Runners → New self-hosted runner**, Linux x64. As `ghrunner`
   (`sudo -iu ghrunner`), download the runner as GitHub shows, unpack it into one folder per board
   (`~/runners/nucleo_g0b1re`) and run `config.sh` there:
   * runner group: press Enter (`Default`; personal repositories have no other groups),
   * runner name: `p02_<board>_<pc name>`, e.g. `p02_nucleo_g0b1re_QR-PF2KTLE6`,
   * labels: the board's label from the table – the job asks for a runner with this label,
   * work folder: press Enter.
4. Back as yourself, start it as a service (needs systemd in WSL), which also starts it with WSL:

   ```bash
   sudo bash -c 'cd /home/ghrunner/runners/nucleo_g0b1re && ./svc.sh install ghrunner && ./svc.sh start'
   ```

5. Start WSL and attach the boards at logon. Windows stops WSL, and the runners with it, soon after the
   last WSL window closes, and the boards are only in WSL while `usbipd attach` runs. Share each board
   once (`usbipd bind --busid <id>`, as administrator), then copy
   [`tools/hil/p02-runners.ps1`](../tools/hil/p02-runners.ps1) to e.g.
   `C:\Users\<you>\Scripts\autostart\p02-runners.ps1` and keep only the boards this PC has. The Nucleo
   and the Nano are attached by USB ID, so any USB port works. The Daisy is attached by its USB port (bus
   ID from `usbipd list`), since it changes its USB ID in MCUboot and DFU mode: keep it in that port, or
   change the bus ID in the script.

   Run it at logon with a task (PowerShell, not as administrator):

   ```powershell
   $action   = New-ScheduledTaskAction -Execute "conhost.exe" -Argument "--headless powershell.exe -NoProfile -ExecutionPolicy Bypass -File $env:USERPROFILE\Scripts\autostart\p02-runners.ps1"
   $trigger  = New-ScheduledTaskTrigger -AtLogOn -User $env:USERNAME
   $settings = New-ScheduledTaskSettingsSet -ExecutionTimeLimit (New-TimeSpan -Seconds 0) -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries
   Register-ScheduledTask -TaskName "P02 runners" -Action $action -Trigger $trigger -Settings $settings
   ```

   The runners only work while the PC is awake and you are logged in; jobs queued in the meantime run
   afterwards.
6. Keep each runner online only while its board is attached, so that a job never lands on a PC without
   the board (e.g. the Nano's runner at home and at work). Every 30 s, a timer starts a runner whose port
   is there and stops one whose port has been gone for 2 minutes, unless a job is running – a board
   leaves USB for a moment each time a job flashes it. A stopped runner shows as offline, and its jobs
   wait for a runner that has the board. Install it as yourself, and remove the lines for boards this PC
   does not have from `/etc/p02-runner-guard.conf`:

   ```bash
   sudo install -m 755 tools/hil/runner_guard/runner_guard.sh /usr/local/sbin/p02-runner-guard
   sudo install -m 644 tools/hil/runner_guard/p02-runner-guard.conf /etc/
   sudo install -m 644 tools/hil/runner_guard/p02-runner-guard.{service,timer} /etc/systemd/system/
   sudo systemctl daemon-reload && sudo systemctl enable --now p02-runner-guard.timer
   ```

   `journalctl -u p02-runner-guard` shows when it started or stopped a runner.

While a job runs:

* The board must be attached to WSL (`usbipd attach`) and its port free: quit `tio` first, or the job fails.
* The job flashes the board, so whatever was on it (another platform, a test build) is replaced.

The test can also be run by hand after flashing:

```bash
. ~/esp/esp-idf/export.sh
python tools/hil/p02_test.py --port /dev/tty_arduino_nano_esp32
python3 tools/hil/p02_test.py --port /dev/tty_nucleo_g0b1re --board nucleo_g0b1re --platform Zephyr
```
