# AGENTS.md

Context for AI coding assistants working in this repository: commands, conventions, troubleshooting,
design decisions and lessons learned. People should start with the [readme](readme.md) and
[`docs/`](docs/).

## Project

C++17 drivers and a small command-line application (`logic::Logic`) for the Arduino Nano ESP32
(ESP32-S3). The same application runs on **ESP-IDF 6.0** and **Zephyr RTOS 4.4.2**: each platform has its
own factory (`factory::Esp32s3`, `factory::Zephyr`, plus `factory::Stub` for tests) and its own driver
implementations next to the interfaces in `main/`. See [docs/architecture.md](docs/architecture.md).

## Glossary

| Term | Meaning |
|---|---|
| P02 | The application: the entry point (`main/source/main.cpp` on ESP-IDF, `zephyr/app/src/main.cpp` on Zephyr) creates the platform's factory and runs `logic::Logic`. Board definitions, MCUboot and Zephyr's samples are not P02 |
| Board | The hardware (e.g. Arduino Nano ESP32, Daisy Seed): chip, memory, pins, LEDs, buttons |
| Board definition | Zephyr's description of a board (`zephyr/boards/<vendor>/<board>/`): devicetree, Kconfig, flash runner, and on the Daisy MCUboot. Used by any Zephyr app on the board, not only P02, so it is documented with the board |
| Board document | One per board, in the board definition directory (`zephyr/boards/<vendor>/<board>/doc/index.md`, like Zephyr's own boards, so it is copied with the board): the board and its board definition. Can be copied into another project without removing anything – nothing about P02, and nothing outside the board definition and Zephyr: no repository scripts, no `$P02_ROOT`, no links to other docs. It says *what* the board needs (toolchain, modules, udev rules, `<board root>` in commands); the P02 board document says how this repository provides it. Design decisions about the board definition are summarised in it (one line each); the full table, lessons and verification notes are in the board definition's own `AGENTS.md`, which is copied with it |
| P02 board document | One per board, `docs/p02_<board>.md`: what is true only for P02 on that board (wiring, app overlay, P02's memory use; building and flashing P02 only where it differs from the platform guides – the Nano's is in `platform_esp_idf.md` and `platform_zephyr.md`, the Daisy's and the Nucleo-G0B1RE's in their P02 board documents). Decisions about P02 on a board stay in AGENTS.md |
| Board target | The full name given to `west build -b`: `arduino_nano_esp32/esp32s3/procpu`, `daisy_seed`, `daisy_seed/stm32h750xx/ram_load`, `nucleo_g0b1re`. One board definition can have several. Zephyr completes a short name with the SoC: `-b daisy_seed` gives `CONFIG_BOARD_TARGET="daisy_seed/stm32h750xx"` |
| Platform | What P02 runs on: ESP-IDF or Zephyr, each with its own factory. Not the *environment*, which is the shell setup for one platform (`export.sh`, `tools/zephyr_env.sh`) |
| App overlay | `zephyr/app/boards/<board target, / → _>.overlay`: P02's wiring on one board target (`p02-led`, `zephyr,user`). Not part of the board definition |
| `docs/boards.md` | What is true for P02 on every board: what a board needs, the supported boards |

## Commands

All commands run from the repository root. Never mix the ESP-IDF and Zephyr environments in one shell.

```bash
# ESP-IDF (ESP-IDF 6.0 in ~/esp/esp-idf)
. ~/esp/esp-idf/export.sh
idf.py set-target esp32s3            # first time only
idf.py build
idf.py -p /dev/tty_arduino_nano_esp32 flash monitor

# Zephyr
tools/zephyr_setup.sh                # first time only
source tools/zephyr_env.sh
west build -p always --sysbuild -b arduino_nano_esp32/esp32s3/procpu -s zephyr/app -d zephyr/app/build/arduino_nano_esp32 -- -DBOARD_ROOT=$P02_ROOT/zephyr
west flash -d zephyr/app/build/arduino_nano_esp32 --esp-device /dev/tty_arduino_nano_esp32 && tio /dev/tty_arduino_nano_esp32

# Unit tests on the host (GoogleTest). Building also runs them (custom target "run" in
# tests/CMakeLists.txt); no CTest. Compiled with -Wall -Werror, so a warning fails the build.
cmake -S tests -B tests/build && cmake --build tests/build

# Formatting (pinned clang-format) – run before committing
tools/clang-format/fix.sh
tools/clang-format/check.sh
```

## Conventions

* Each driver: headers in `main/include/driver/<name>/` (`interface.h`, `esp32s3.h`, `zephyr.h`, `stub.h`),
  sources in `main/source/driver/<name>/` (`esp32s3.cpp`, `zephyr.cpp`). `communication/` and `ml/` use the
  same interface pattern. Each build system lists its own sources (`main/CMakeLists.txt`,
  `zephyr/app/CMakeLists.txt`, `tests/CMakeLists.txt`).
* Adding a driver: interface, ESP-IDF and Zephyr implementations, stub, a `create...()` method in
  `factory::Interface` and all three factories, the `.cpp` files in the build files above, and a test. See
  [docs/architecture.md](docs/architecture.md#writing-a-driver).
* Classes are `final` where possible, member functions `noexcept`, overrides marked `override`, copy and
  move deleted for drivers; members are prefixed `my` (`myState`, `myValid`).
* Hardware is requested from the factory by role (`GpioId`, `AdcId`), never by pin number.
* Line endings: text for the serial driver's `print()` ends lines with `\r\n` – the drivers send bytes as
  they are, on both platforms (raw bytes also suit binary protocols such as mcumgr). `printf`/`printk` keep
  `\n`: ESP-IDF's stdout (`CONFIG_LIBC_STDOUT_LINE_ENDING_CRLF`) and Zephyr's console add the `\r`
  themselves, so `\r\n` there gives `\r\r\n`.
* Workflow: one feature per branch and PR, review, merge with a merge commit (see
  [docs/contributing.md](docs/contributing.md)).
* `docs/`, `readme.md` and board documents are for people: what to do and a short why. Mechanism, evidence
  ("verified on the board"), exact error bytes and history go in an `AGENTS.md` (the root one, or the board
  definition's own).
* `.gitignore` ignores files without an extension; `Kconfig` and `*_defconfig` are excepted, `sdkconfig`
  is not committed.

## Do not touch

* Do not edit generated or fetched files: `deps/`, `.west/`, `.venv/`, `managed_components/`, `build/`,
  `zephyr/*/build/`, `tests/googletest/` (submodule).
* Never commit `main/include/communication/wifi/esp32s3_local_config.h` (Wi-Fi credentials) or `sdkconfig`.

---

## Troubleshooting

### Setup (WSL, USB, serial)

| Error | Cause / fix |
|---|---|
| `Could not open COM3` | Windows port name used in WSL. Use `/dev/tty_arduino_nano_esp32` (see [fixed port names](docs/setup.md#fixed-port-names)). |
| `Path '/dev/ttyACM0' is not readable` | The user is not in the `dialout` group. See [Setup step 2](docs/setup.md#2-packages-in-ubuntu), then run `wsl --shutdown` in PowerShell. |
| `usbipd: error: Device is not shared` | Run `usbipd bind --busid <id>` as administrator first. |
| No `/dev/ttyACM*` in WSL | Check that `usbipd attach` is running and that the board is not in Arduino mode (`2341:0070`). |
| `/dev/ttyACM*` exists, but not `/dev/tty_arduino_nano_esp32` / `/dev/tty_daisy_seed` | The udev rules are not installed: see [fixed port names](docs/setup.md#fixed-port-names). On the Daisy, also software built before the USB product name `Daisy Seed`: flash MCUboot and P02 again. |
| `smpmgr`, `esptool` or `tio`: port busy / `multiple access on port` | Another program has the port open, often `tio` in another terminal. Quit it (`Ctrl+T`, `q`); `fuser -v /dev/ttyACM*` shows who. |
| `SerialException: device reports readiness to read but returned no data` | The board disconnected from USB (reset/re-attach). Start the monitor again. |
| `idf.py monitor` cannot open the port right after flashing | usbipd had not re-attached the board yet when the monitor started. Wait a moment and run `idf.py -p /dev/tty_arduino_nano_esp32 monitor`. (`tio` waits by itself.) |
| `tools/...sh: Permission denied`, or `git status` shows a script as modified with no changes (`git diff --summary`: `mode change 100755 => 100644`) | The file lost its executable bit (e.g. edited from Windows). `chmod +x <file>`. |

### ESP-IDF

| Error | Cause / fix |
|---|---|
| `ESP-IDF Python virtual environment ... not found` | `install.sh` has not completed. Run `cd ~/esp/esp-idf && ./install.sh esp32s3`. |
| `libusb-1.0.so.0: cannot open shared object file` (during install) | `sudo apt install -y libusb-1.0-0`, then run `install.sh` again. |
| `Current directory '.../esp-idf' is not buildable` | You are in the ESP-IDF directory. `cd` to the repository root. Remove `~/esp/esp-idf/build`. |
| CMake uses `Unix Makefiles` instead of Ninja | `ninja-build` is missing: `sudo apt install -y ninja-build`. |
| `esp32s3_local_config.h: No such file or directory` | Copy the Wi-Fi template, see [Create the Wi-Fi config](docs/platform_esp_idf.md#3-create-the-wi-fi-config-once). |
| `task_wdt: Task watchdog got triggered ... IDLE0` | A task never blocks, so the idle task cannot run. See *Sleeps shorter than one tick* under ESP-IDF pitfalls. |
| `task_wdt: Task watchdog got triggered ... - main (CPU 0)` | `Logic`'s loop stopped kicking the watchdog for 3 s: the `hang` command (expected), or something in the loop blocks. |

### Zephyr

| Error | Cause / fix |
|---|---|
| `error: the Zephyr workspace is not set up` | Run `tools/zephyr_setup.sh` first (once per clone). |
| `west: unknown command "build"` | `ZEPHYR_BASE` is not set. Run `source tools/zephyr_env.sh`. |
| `No board named 'arduino_nano_esp32' found` | Zephyr cannot find our board. Our apps: `list(APPEND BOARD_ROOT ...)` must come before `find_package(Zephyr)` in `CMakeLists.txt`. Zephyr's samples, and any build with `--sysbuild` (every P02 build, since P02 boots through MCUboot): add `-- -DBOARD_ROOT=$P02_ROOT/zephyr` – sysbuild looks up the board before the app's `CMakeLists.txt` runs. Not for the Nucleo, which is in Zephyr. |
| `devicetree error: ... 'pinctrl-names' is marked as required` (after enabling `i2c0`/`spi2`) | Disabled nodes are not checked against their bindings – errors only show when a node is enabled. Test the board with each bus enabled. |
| `CMake was unable to find a build program corresponding to "Ninja"` | `pip install ninja` in the Zephyr venv, or `sudo apt install ninja-build`. |
| `west blobs fetch` fails with `504 Gateway Time-out` | Temporary GitHub error. Run `west blobs fetch hal_espressif` again. |
| Flashing works but the monitor shows nothing | Boot output was printed before the monitor connected (usbipd re-attached the board slowly). Increase `CONFIG_BOOT_DELAY` in the app's `prj.conf`, e.g. to `3000`. |
| Strange compiler errors when building for the ESP32-S3 | The course's `env.sh` is active in the terminal. Open a new terminal and run `source tools/zephyr_env.sh`. |
| Nano: the monitor never shows `*** Booting MCUboot ***` | Expected: MCUboot prints before usbipd has attached the board again. If P02 starts, MCUboot started it. |
| `region 'FLASH' overflowed` when building P02 with `--sysbuild` | P02 no longer fits in `slot0_partition` (200 KB on the Nucleo). Check what takes the space before changing partitions: `ninja -C zephyr/app/build/<board>/app rom_report` (with sysbuild, `rom_report` is in the app's own build directory). |
| `warning: MCUBOOT_UPDATE_FOOTER_SIZE ... was assigned the value '0x800' but got the value ''` (building with `--sysbuild`) | Harmless. MCUboot's sysbuild code always passes the footer size to the app, but only mcumgr's image management uses it, and P02 does not enable it yet (`IMG_MANAGER=n`). Goes away when mcumgr is added. |
| `west flash`: `libusb_open() failed with LIBUSB_ERROR_ACCESS` (OpenOCD, Nucleo) | The ST-Link's USB device belongs to root: install `tools/udev/99-p02.rules` again (it gives `dialout` access to `0483:374b`). |
| `CMake Error: Not a file: .../cmake/pristine.cmake` | The build directory was made with another Zephyr installation. Delete it once: `rm -rf zephyr/app/build`. |

---

## ESP-IDF: design decisions and lessons learned

### Design decisions

| Decision | Choice | Why |
|---|---|---|
| ADC attenuation | Chosen from `referenceVoltage` in the constructor | The caller says which voltage range it needs; the driver picks the attenuation |
| ADC noise | Average of `AdcSamples` (5) reads in `readValue()`; `readVoltage()` uses it | Single reads are noisy. `adc::Zephyr` does the same |
| NVS initialisation | `nvs_flash_init()` in the NVS driver's constructor (erase and retry if the partition is full or from a newer version) | The driver works without the Wi-Fi driver having initialised flash first |
| Pins | Arduino names (`D6`, `A6`) mapped to GPIO numbers in `driver/pin/esp32s3.h`; `PhysicalPinManager` stops two drivers from claiming the same pin | Code and wiring use the names printed on the board |
| Watchdog | The task watchdog (TWDT): `esp_task_wdt_reconfigure()` (ESP-IDF has usually started it; `esp_task_wdt_init()` if not) with `trigger_panic = true` and both idle tasks still watched; the task that creates the driver is added with `esp_task_wdt_add(nullptr)` | `trigger_panic` restarts instead of only logging. The TWDT watches tasks, which fits `Logic`'s loop; the interrupt and RTC watchdogs are for lower-level hangs |
| Board in `status` | Hard-coded in `factory::Esp32s3`: board `arduino_nano_esp32`, board target `arduino_nano_esp32/esp32s3/procpu` (Zephyr's name for the same board) | ESP-IDF knows only the chip (`CONFIG_IDF_TARGET`), not the board; the factory already maps the Nano's pins, so it is the Nano |
| Testing the watchdog | The `hang` command, which sets a flag (`myHung`) that stops the kicks; an invalid `period` is only rejected | A user error is not a reason to restart. A flag rather than an endless loop in `handleHang()`: the rest of the program keeps running until the restart, so `hang` can be tested on the host |

### Pitfalls and lessons learned

* **Sleeps shorter than one tick.** FreeRTOS ticks every 10 ms (`CONFIG_FREERTOS_HZ=100`). A shorter
  sleep busy-waits in ESP-IDF, so the task never blocks, the idle task cannot run, and the task watchdog
  fires. `Logic` therefore sleeps 10 ms.
* **`sleep()` takes seconds**, not milliseconds – use `std::this_thread::sleep_for(std::chrono::milliseconds(...))`.
* **ADC2 (A4–A7) cannot be used while Wi-Fi is active.** Use A0–A3 (ADC1) for analog sensors if Wi-Fi is
  needed.
* The Nano ESP32 ships with the Arduino bootloader – the first flash needs B1 to GND (see
  [the board document](zephyr/boards/arduino/nano_esp32/doc/index.md#first-flash-leave-arduino-mode)).

---

## Zephyr: design decisions and lessons learned

### Design decisions

| Decision | Choice | Why |
|---|---|---|
| How Zephyr gets into the project | `west.yml` in this repository, which is the west workspace (T2); Zephyr and `hal_espressif` in `deps/` | Zephyr is ~80 repositories that west fetches; git submodules cannot follow Zephyr's own manifest. The version is pinned in git, setup is one script, and CI builds the port |
| Where the Zephyr drivers live | Next to the ESP32 ones in `main/` (`driver/<name>/zephyr.h/.cpp`) | Each driver's interface, ESP32, stub and Zephyr versions stay together; each build system lists its own files |
| Board | Own out-of-tree board definition, found through `BOARD_ROOT` | The Nano ESP32 is not in Zephyr; app overlays can use Arduino pin names (`arduino_nano_header`) |
| Where pins come from | Devicetree: drivers take a `gpio_dt_spec` / `adc_dt_spec` | Pin **and** polarity/gain come from the board's overlay in `zephyr/app/boards/`; no GPIO numbers in Zephyr code |
| How the factory maps hardware | Roles (`GpioId::StatusLed`, `AdcId::TempSensor`) instead of pin numbers | Each factory decides what a role means: `factory::Esp32s3` a pin, `factory::Zephyr` a devicetree node |
| ADC attenuation | `zephyr,gain` in devicetree; `referenceVoltage` is ignored | Zephyr configures channels from devicetree, not at runtime |
| Timer | Polled `timeout()`, restarts from "now" | `Logic` stays unchanged. A `k_timer` callback would be more idiomatic but changes the interface |
| Serial | Our own command parser kept, not Zephyr's shell | `Logic` stays unchanged on both platforms |
| Watchdog | The board's alias `watchdog0` – `wdt0` (Timer Group 0) on the Nano, the IWDG on the Daisy Seed – with `WDT_FLAG_RESET_SOC`, `window.min = 0`; reset cause from `hwinfo_get_reset_cause()` (`CONFIG_HWINFO`) | Zephyr's ESP32 driver uses two stages with the same time (interrupt, then reset), so the Nano restarts after about **twice** the timeout (~6 s instead of 3 s). `window.max = timeoutMs / 2` would make it match ESP-IDF. The Daisy's IWDG is not tested yet (see the Daisy's open items) |
| Board in `status` | `factory::Zephyr` returns `CONFIG_BOARD` and `CONFIG_BOARD_TARGET`; `Logic` prints them with the platform (three methods on `factory::Interface`, not a struct) | Each new board gets the right name without code changes. The board target shows the build variant (Daisy: `ram_load` or not); the platform tells ESP-IDF and Zephyr apart on the Nano |
| NVS | Settings subsystem (NVS backend), not the raw NVS API | String keys like ESP-IDF (raw NVS has numeric IDs). The namespace is the key prefix (`user_settings/blink_state`). Every write goes to flash at once, so `autoCommit`/`commit()` have no effect |
| Output from `Logic` | `mySerialDriver->print()`, with `std::snprintf` for lines with numbers; no `std::cout` | `<iostream>` (streams and locales) cost most of P02's flash. Removing it: Nucleo ~206 → ~48 KB, Nano (Zephyr) ~432 → ~141 KB, Nano (ESP-IDF) ~402 → ~231 KB, Daisy ~258 → ~133 KB. Zephyr logging would be a further improvement |
| Bootloader | MCUboot on every board (`zephyr/app/sysbuild.conf`, build with `--sysbuild`), no signature yet (`BOOT_SIGNATURE_TYPE_NONE`), each board's default mode: Nucleo swap using offset, Nano overwrite only (Zephyr's default for Espressif chips), Daisy RAM load (its board's `Kconfig.sysbuild`) | The same ground on all boards before the update demo (Fas 1). The mode is not set in `sysbuild.conf`, as that would override the Daisy's RAM load; rollback needs a swap mode (or RAM load with revert), chosen per board later. `NONE` because sysbuild's default, RSA, needs the mbedTLS module (not in `west.yml`); ECDSA P-256 with MCUboot's bundled TinyCrypt works without it. Not on ESP-IDF, which has its own bootloader |
| Build directories | P02 in `zephyr/app/build/<board>` (`arduino_nano_esp32`, `daisy_seed`, `nucleo_g0b1re`); Zephyr's samples in `<sample>_build` at the root (`hello_world_build`); CI one directory per build in `build-zephyr/` | A build for one board does not overwrite another's. Before MCUboot, P02 built in `zephyr/app/build` and the Daisy in `zephyr/app/daisy_seed_build`; delete both once (`rm -rf zephyr/app/build zephyr/app/daisy_seed_build`) |
| ESP-IDF and Zephyr in one repository | Yes | The shared logic is built for both in CI; reorganise (`common/`, `esp-idf/`, `zephyr/`) if the ESP-IDF build is dropped |

### Pitfalls and lessons learned

#### West workspace

* `west init -l` makes the *parent* of the repository the workspace (`~`), which collides with other
  workspaces. `tools/zephyr_setup.sh` writes `.west/config` with `path = .` instead.
* `name-allowlist` fetches only `hal_espressif` (~800 MB instead of several GB).
* `path-prefix: deps` also applies to the zephyr project itself: declare it as `path: zephyr`
  (→ `deps/zephyr`); `path: deps/zephyr` ends up in `deps/deps/zephyr`.
* The SDK is per user (`~/zephyr-sdk-<version>`), not per workspace.
* A build directory remembers the Zephyr it was made with. After changing workspaces, delete it.
* IDE extensions that "set up Zephyr" may create their own workspace and rewrite `.west/config`. Point
  them at the existing workspace, or check `.west/config` afterwards.

#### Board and devicetree

* The Nano's LEDs, GPIO numbers, strapping pins, `gpio0`/`gpio1` and ADC numbering are in its board
  definition: [`doc/index.md`](zephyr/boards/arduino/nano_esp32/doc/index.md) and [`AGENTS.md`](zephyr/boards/arduino/nano_esp32/AGENTS.md).
* When copying another board, change **every** board-specific value: copied SPI pinctrl landed on the
  I2C pins and nothing complained.
* **Disabled nodes are not checked against their bindings.** Errors only show when a node is enabled –
  test the board with each bus `okay`.
* **Label, node name and alias** – `p02_led: led-d6 { ... }`, `aliases { p02-led = &p02_led; }`:
  label (`_`) for `&p02_led` in devicetree, node name (`-`) for description, alias (lowercase, `-`) for
  `DT_ALIAS(p02_led)` in code. Name aliases after the **role**, not the pin.
* **App overlays** in `zephyr/app/boards/` are named after the full target, `/` → `_`
  (`arduino_nano_esp32_procpu.overlay`), or the same without the SoC (`<board>_<variant>.overlay`) –
  **not** the bare board name: `arduino_nano_esp32.overlay` is silently ignored. A board overlay replaces
  `app.overlay`. Check the `Found devicetree overlay:` line in the build output.
* **Arduino R3 header indexes start with A0–A5**: `<&arduino_header 6 ...>` is **D0**, not D6. Use the
  names from `arduino-header-r3.h` (`ARDUINO_HEADER_R3_D6` = 12) and check the index in
  `build/zephyr/zephyr.dts`. Caught on the Nucleo-G0B1RE before flashing. The Nano's overlay writes
  `<&arduino_nano_header 6 ...>`; whether its header starts at D0 has not been checked.
* New nodes go inside `/ { }`; changing an existing node (`&adc1 { }`) is written at the top level.
* A node with the same name as one in the board is **merged** into it; give the app's wiring its own
  parent (`p02-leds`).
* The same property twice in a node is not an error – the last one wins.
* Read the **first** line of a devicetree error; a missing `;` is reported on the line after.

#### Kconfig and CMake

* No spaces around `=` in `prj.conf`, and end the file with a newline.
* Board defconfig = what every app needs; `prj.conf` = what this app uses.
* `CONFIG_SETTINGS_NVS` does not need to be set: the backend defaults to NVS when `CONFIG_NVS` is on.
* `BOARD_ROOT` and `EXTRA_DTC_OVERLAY_FILE` must be set **before** `find_package(Zephyr)`.
* Every `.cpp` must be in `target_sources` – a missing one is a **linker** error
  (`undefined reference to ...`), not a compiler error.
* `.gitignore` ignores files without an extension – Zephyr's `Kconfig` and `*_defconfig` are excepted.
  Build from a fresh clone before opening a PR.

#### C++ on Zephyr

* `CONFIG_CPP`, `CONFIG_STD_CPP17`, `CONFIG_REQUIRES_FULL_LIBCPP`, and `CONFIG_POSIX_API` for
  `std::this_thread::sleep_for` (otherwise `undefined reference to 'usleep'`, `sleep`, `gettimeofday`).
* No `std::cout` (`<iostream>`): it costs ~160 KB of flash. Print through the serial driver, with
  `std::snprintf` for numbers; a `std::string_view` needs `%.*s`, since it is not null-terminated.
* `sleep()` takes **seconds**. Use `std::this_thread::sleep_for(std::chrono::milliseconds(...))`
  (`<thread>`, `<chrono>`); `usleep` gives header conflicts between Zephyr's POSIX headers and picolibc.
* `printk` formats must match the type (`%lld` + cast for `int64_t`); no `%f` by default.
* A C callback can be a lambda **without capture** (`[]`); pass data through the `void* param` argument.
* Keep signed return values signed (`ssize_t`); `const size_t n{ssize_t_value}` is a narrowing error
  with `{}` – which is the point of `{}`.

#### Drivers

* Zephyr calls return negative error codes. Store them in an `int` before converting to `bool`, or an
  error becomes `true`.
* ADC: `buffer_size` in `adc_sequence` is in **bytes**; `adc_raw_to_millivolts_dt` needs an `int32_t`.
* NVS: Settings stores raw bytes, so `set<T>`/`get<T>` are `&value, sizeof(T)` with
  `static_assert(std::is_integral_v<T>)`. `eraseAll()` collects the keys with
  `settings_load_subtree_direct()` first and erases them afterwards.
* P02 only saves on the `store` command – a changed `period` is not kept after a reset by itself.

#### USB console (usbipd/WSL)

* Every reset re-enumerates USB, and usbipd has to re-attach the board to WSL. `CONFIG_BOOT_DELAY=1000`
  keeps the boot output, and the monitor is started right after `west flash`. With a slower usbipd, add a
  `sleep` before the monitor (see [Troubleshooting → Setup](#setup-wsl-usb-serial)).
* The monitor is `tio`, not `miniterm`: `tio` waits for the port and reconnects after every reset. `miniterm`
  crashed when the board left USB (RESET pressed), and as it still held `/dev/ttyACM0`, the board came back as
  `/dev/ttyACM1`. The fixed names from `tools/udev/99-p02.rules` (`/dev/tty_daisy_seed`,
  `/dev/tty_arduino_nano_esp32`) do not change. Exit `tio` with `Ctrl+T`, `q`; quit it before `west flash` or
  `smpmgr`, which need the port to themselves.
* **The first command after opening the monitor had junk in front of it** ("unknown command"
  with e.g. `Zephyr` in it), seen with `tio` and `miniterm`. Cause: Linux gives a tty `ECHO` on by default.
  On quit, `tio` restores the settings it found at open (`ECHO` on) and then closes the port; in between,
  the output P02 keeps printing (timer debug lines) is sent straight back. P02 takes it as input, and the part
  after the last line break stays in `serial::Zephyr::readLine()`'s buffer, in front of the next command.
  Shown on the Daisy with a test that restores the settings like `tio` before closing: with `ECHO` restored,
  `help` was read as `Zephyr timer timZephyr timer timhelp`; with `-echo`, as `help`. Opening the port was not
  the problem (a clean `help` even with `ECHO` on). The same echo is behind smpmgr's
  `Frame could not be parsed` when P02 runs. Fixed twice: `tools/udev/99-p02.rules` sets `-echo` on both
  ports when the board appears (Linux keeps it until the board disconnects); this leaves one echo when the
  rule's own `stty` opens the port, seen as junk in the first command after a power cycle. So
  `serial::Zephyr::readLine()` also drops its input when DTR goes high, i.e. a monitor opens the port
  (`CONFIG_UART_LINE_CTRL`; CDC ACM only, the Nano's USB-Serial/JTAG reports no DTR). Verified on the Daisy
  with `tio`: clean first commands after a restart, and with `ECHO` on and no rule. The Nano still shows
  `!help` when `tio` is already waiting while the board attaches: udev creates the symlink before the rule's
  `stty` runs, so `tio` can open the port with `ECHO` still on, and a `!` the Nano sends at that point (source
  unknown; P02 does not print it) comes back. Opening `tio` after the attach is complete avoids it; the Nano
  cannot drop the input itself (no DTR).

---

## Daisy Seed (work in progress): P02 on the Daisy

The board definition is in `zephyr/boards/electrosmith/daisy_seed/`, with its own board document
(`doc/index.md`) and its own [`AGENTS.md`](zephyr/boards/electrosmith/daisy_seed/AGENTS.md): design decisions
about the board definition (targets, MCUboot, recovery mode, QSPI, USB), the bring-up and its lessons.
Read that file before changing anything in the board directory. P02 on the Daisy for people:
[docs/p02_daisy_seed.md](docs/p02_daisy_seed.md).

| Decision | Choice | Why |
|---|---|---|
| Where P02 runs | MCUboot in the 128 KB internal flash; P02 stored in the 8 MB QSPI flash | P02 is ~133 KB (signed image); the STM32H750 has 128 KB internal flash |
| RAM load, not XIP | P02 is built for `daisy_seed/stm32h750xx/ram_load` and uses ~153 KB of the 512 KB AXI SRAM (code, data and stack) | `store` writes NVS in QSPI, which a program running from QSPI cannot do (details in the board definition's `AGENTS.md`) |
| Main stack | `CONFIG_MAIN_STACK_SIZE=4096` in `zephyr/app/prj.conf` | Zephyr's default 1 KB overflowed on `store` (Settings → NVS → flash driver in the main thread); with the MPU stack guard the app halted. The Nano's board already used 4 KB, so it never showed there |
| App wiring | One overlay per board in `zephyr/app/boards/` providing `p02-led` and `zephyr,user`: `arduino_nano_esp32_procpu.overlay`, `daisy_seed_ram_load.overlay` | `factory::Zephyr` uses only those names; `boards/<board>.overlay` replaces `app.overlay`. See *App overlays* under Board and devicetree for the file name. |
| Wiring comments | Give all three names: Daisy name, physical pin, STM32 name (`/* D15 = pin 22 = PC0 */`) | Only the physical pin number matches the board's print; the STM32 name is what the datasheet and errors use |
| CI | A separate job per board: the Nano's `zephyr-build` stays, a new Daisy job builds Zephyr's blinky on `daisy_seed` and P02 with MCUboot (`--sysbuild`) on `daisy_seed/stm32h750xx/ram_load` | Each job is its own check in the PR, so it shows directly whether Zephyr works for the Nano and/or the Daisy. Cost: the workspace setup runs in both jobs (in parallel) – runner time and energy, judged worth it. Later: per-board setup, so each job downloads only its own toolchain |
| Role mapping | External parts, like the Nano: `StatusLed` = LED on D26 = pin 33 = PD11 (330 Ω to GND, active high); `TempSensor` = potentiometer simulating a TMP36 on A3/D18 = pin 25 = PA7 (ADC1 ch 7), fed from 3V3 pin 38 / GND pin 40 | Pins 16, 20–24 are used by another project on the same breadboard. D1–D14 and D29–D30 are kept free for the Daisy's usual buses (SD card, SPI, I2C, UART, USB HS). Verified on the board: pin 33 = D26 (blinky on the external LED through `daisy_seed_header`) and pin 25 = A3 (Zephyr's `adc_dt` sample with this overlay follows the pot, ~0–3300 mV). Breadboard tip: on the side with pins 21–40, the row number counts the other way, so one row off is an easy mistake |
| Modules | `hal_stm32`, `cmsis_6` (Cortex-M; `cmsis` is for Cortex-A/R), `mcuboot` and `zcbor` (MCUboot's serial recovery) in `west.yml`; `arm-zephyr-eabi` in `tools/zephyr_setup.sh` | Needed to build for the STM32H750 |

Open items: **P02 sometimes starts with its USB off** after a software reset (`smpmgr os reset`, the end of
`west flash`), never seen after power-on or RESET. P02 runs, but the PC sees no device on the port (UsbTreeView:
port empty). A probe on the board showed `usbd_init()` failing with `-EIO` (likely `HAL_PCD_Init`, the USB
core reset timing out) and a retry 1 s later failing too; RESET recovers. Not reproducible on demand (a few times
in an afternoon). Next step if it returns: save the USB/RCC/PWR registers in `__noinit` RAM on failure and print
them after RESET, and try a retry after an RCC reset of OTG FS. Seen with `board.c` in place; whether it also
happens without it is not yet known. Other open items: a `zephyr/module.yml` with `board_root: zephyr`, so builds find our boards without `-DBOARD_ROOT` (needed today even for `zephyr/app` with `--sysbuild`); P02 on `native_sim` (emulated LED, ADC and flash; its own branch); **the watchdog on the Daisy**
(`watchdog0` = IWDG, added with #77, not tested with P02 yet): check that `hang` restarts the board, and
whether `Last reset was caused by the watchdog` is printed – `mcuboot_hooks.c` clears the reset flags
(`RCC->RSR`) before P02 reads them, so `causedLastReset()` is expected to be false there.

---

## Nucleo-G0B1RE (work in progress): P02 on the Nucleo

The ST Nucleo-G0B1RE (STM32G0B1RE, Cortex-M0+, 512 KB flash, 144 KB RAM, on-board ST-Link/V2-1 `0483:374b`)
is in Zephyr (`deps/zephyr/boards/st/nucleo_g0b1re/`), so there is no board definition or board document
here. P02 on the Nucleo for people: [docs/p02_nucleo_g0b1re.md](docs/p02_nucleo_g0b1re.md).

| Decision | Choice | Why |
|---|---|---|
| Board definition | Zephyr's own `nucleo_g0b1re` | It already has a console (USART2 via the ST-Link), `watchdog0` = IWDG, `adc1` with PA0/PA1 and a 64 KB `storage_partition` |
| MCUboot | MCUboot (~26 KB) in the board's 48 KB `boot_partition`, P02 (~48 KB) linked to the 200 KB `slot0_partition` at 0xC000 | The board's partitions are already laid out for MCUboot. P02 only fits since `std::cout` is gone (206 KB before). Default mode swap using offset, with the board's equal slots: fine for booting, not tried for an update yet |
| Flash runner | `--runner openocd` | The board's first runner is STM32CubeProgrammer (not installed); OpenOCD comes with the Zephyr SDK |
| Role mapping | `StatusLed` = external LED on D6 = PB14 (330 Ω to GND, active high); `TempSensor` = TMP36 on A0 = PA0 (ADC1 ch 0), Vs from 3V3 | D6 as on the Nano, so the wiring is the same. The on-board LD4 (PA5 = D13) was tried first and dropped in favour of the external LED. A0 because the board already has PA0 in `adc1`'s pinctrl |
| udev | `0483:374b` → `/dev/tty_nucleo_g0b1re` (`-echo`), and `MODE=0660 GROUP=dialout` on the USB device | OpenOCD failed with `LIBUSB_ERROR_ACCESS` without it. The rule matches any ST-Link/V2-1 (most Nucleo-64 boards) |

Open items: `hang` → IWDG restart and the reset cause with MCUboot; the wiring picture the user drew shows an MB1814C (USB-C, STLINK-V3), while
the board here reports an ST-Link/V2-1 – check which board revision it is.
