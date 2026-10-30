# AGENTS.md

Context for AI coding assistants working in this repository: commands, conventions, troubleshooting,
design decisions and lessons learned. People should start with the [readme](readme.md) and
[`docs/`](docs/).

## Project

C++17 drivers and a small command-line application (`logic::Logic`) for the Arduino Nano ESP32
(ESP32-S3). The same application runs on **ESP-IDF 6.0** and **Zephyr RTOS 4.4.2**: each platform has its
own factory (`factory::Esp32s3`, `factory::Zephyr`, plus `factory::Stub` for tests) and its own driver
implementations next to the interfaces in `main/`. See [docs/architecture.md](docs/architecture.md).

## Commands

All commands run from the repository root. Never mix the ESP-IDF and Zephyr environments in one shell.

```bash
# ESP-IDF (ESP-IDF 6.0 in ~/esp/esp-idf)
. ~/esp/esp-idf/export.sh
idf.py set-target esp32s3            # first time only
idf.py build
idf.py -p /dev/ttyACM0 flash monitor

# Zephyr
tools/zephyr_setup.sh                # first time only
source tools/zephyr_env.sh
west build -p always -b arduino_nano_esp32/esp32s3/procpu -s zephyr/app -d zephyr/app/build
west flash -d zephyr/app/build --esp-device /dev/ttyACM0

# Unit tests on the host (GoogleTest)
cmake -S tests -B tests/build && cmake --build tests/build

# Formatting (pinned clang-format) – run before committing
tools/clang-format/fix.sh
tools/clang-format/check.sh
```

## Conventions

* Each driver: `driver/<name>/interface.h` + `esp32s3.h/.cpp`, `zephyr.h/.cpp`, `stub.h`. Each build
  system lists its own sources (`main/CMakeLists.txt`, `zephyr/app/CMakeLists.txt`, `tests/CMakeLists.txt`).
* Classes are `final` where possible, member functions `noexcept`, overrides marked `override`, copy and
  move deleted for drivers; members are prefixed `my` (`myState`, `myValid`).
* Hardware is requested from the factory by role (`GpioId`, `AdcId`), never by pin number.
* Workflow: one feature per branch and PR, review, merge with a merge commit (see
  [docs/contributing.md](docs/contributing.md)).
* `.gitignore` ignores files without an extension; `Kconfig` and `*_defconfig` are excepted, `sdkconfig`
  is not committed.

---

## Troubleshooting

### Setup (WSL, USB, serial)

| Error | Cause / fix |
|---|---|
| `Could not open COM3` | Windows port name used in WSL. Use `/dev/ttyACM0`. |
| `Path '/dev/ttyACM0' is not readable` | The user is not in the `dialout` group. See [Setup step 2](docs/setup.md#2-packages-in-ubuntu), then run `wsl --shutdown` in PowerShell. |
| `usbipd: error: Device is not shared` | Run `usbipd bind --busid <id>` as administrator first. |
| No `/dev/ttyACM*` in WSL | Check that `usbipd attach` is running and that the board is not in Arduino mode (`2341:0070`). |
| `SerialException: device reports readiness to read but returned no data` | The board disconnected from USB (reset/re-attach). Start the monitor again. |
| `could not open port /dev/ttyACM0` right after flashing | usbipd had not re-attached the board yet when the monitor started. Wait a moment, or add `sleep 2 &&` before the monitor command. |
| `tools/...sh: Permission denied`, or `git status` shows a script as modified with no changes (`git diff --summary`: `mode change 100755 => 100644`) | The file lost its executable bit (e.g. edited from Windows). `chmod +x <file>`. |

### ESP-IDF

| Error | Cause / fix |
|---|---|
| `ESP-IDF Python virtual environment ... not found` | `install.sh` has not completed. Run `cd ~/esp/esp-idf && ./install.sh esp32s3`. |
| `libusb-1.0.so.0: cannot open shared object file` (during install) | `sudo apt install -y libusb-1.0-0`, then run `install.sh` again. |
| `Current directory '.../esp-idf' is not buildable` | You are in the ESP-IDF directory. `cd` to the repository root. Remove `~/esp/esp-idf/build`. |
| CMake uses `Unix Makefiles` instead of Ninja | `ninja-build` is missing: `sudo apt install -y ninja-build`. |
| `esp32s3_local_config.h: No such file or directory` | Copy the Wi-Fi template, see [Create the Wi-Fi config](docs/espressif_environment.md#3-create-the-wi-fi-config-once). |
| `task_wdt: Task watchdog got triggered ... IDLE0` | A task never blocks, so the idle task cannot run. See *Sleeps shorter than one tick* under ESP-IDF pitfalls. |

### Zephyr

| Error | Cause / fix |
|---|---|
| `error: the Zephyr workspace is not set up` | Run `tools/zephyr_setup.sh` first (once per clone). |
| `west: unknown command "build"` | `ZEPHYR_BASE` is not set. Run `source tools/zephyr_env.sh`. |
| `No board named 'arduino_nano_esp32' found` | Zephyr cannot find our board. Our apps: `list(APPEND BOARD_ROOT ...)` must come before `find_package(Zephyr)` in `CMakeLists.txt`. Zephyr's samples: add `-- -DBOARD_ROOT=$P02_ROOT/zephyr`. |
| `devicetree error: ... 'pinctrl-names' is marked as required` (after enabling `i2c0`/`spi2`) | Disabled nodes are not checked against their bindings – errors only show when a node is enabled. Test the board with each bus enabled. |
| `CMake was unable to find a build program corresponding to "Ninja"` | `pip install ninja` in the Zephyr venv, or `sudo apt install ninja-build`. |
| `west blobs fetch` fails with `504 Gateway Time-out` | Temporary GitHub error. Run `west blobs fetch hal_espressif` again. |
| Flashing works but the monitor shows nothing | Boot output was printed before the monitor connected (usbipd re-attached the board slowly). Increase `CONFIG_BOOT_DELAY` in the app's `prj.conf`, e.g. to `3000`. |
| Strange compiler errors when building for the ESP32-S3 | The course's `env.sh` is active in the terminal. Open a new terminal and run `source tools/zephyr_env.sh`. |
| `CMake Error: Not a file: .../cmake/pristine.cmake` | The build directory was made with another Zephyr installation. Delete it once: `rm -rf zephyr/app/build`. |

---

## ESP-IDF: design decisions and lessons learned

> Draft – to be reviewed and extended.

### Design decisions

| Decision | Choice | Why |
|---|---|---|
| ADC attenuation | Chosen from `referenceVoltage` in the constructor | The caller says which voltage range it needs; the driver picks the attenuation |
| ADC noise | Average of `AdcSamples` (5) reads in `readValue()`; `readVoltage()` uses it | Single reads are noisy; the same on Zephyr |
| NVS initialisation | `nvs_flash_init()` in the NVS driver's constructor (erase and retry if the partition is full or from a newer version) | The driver works without the Wi-Fi driver having initialised flash first |
| Pins | Arduino names (`D6`, `A6`) mapped to GPIO numbers in `driver/pin/esp32s3.h`; `PhysicalPinManager` stops two drivers from claiming the same pin | Code and wiring use the names printed on the board |

### Pitfalls and lessons learned

* **Sleeps shorter than one tick.** FreeRTOS ticks every 10 ms (`CONFIG_FREERTOS_HZ=100`). A shorter
  sleep busy-waits in ESP-IDF, so the task never blocks, the idle task cannot run, and the task watchdog
  fires. `Logic` therefore sleeps 10 ms.
* **`sleep()` takes seconds**, not milliseconds – use `std::this_thread::sleep_for(std::chrono::milliseconds(...))`.
* **ADC2 (A4–A7) cannot be used while Wi-Fi is active.** Use A0–A3 (ADC1) for analog sensors if Wi-Fi is
  needed.
* The Nano ESP32 ships with the Arduino bootloader – the first flash needs B1 to GND (see
  [Setup](docs/setup.md#5-first-time-get-the-board-out-of-arduino-mode)).

---

## Zephyr: design decisions and lessons learned

### Design decisions

| Decision | Choice | Why |
|---|---|---|
| How Zephyr gets into the project | `west.yml` in this repository, which is the west workspace (T2); Zephyr and `hal_espressif` in `deps/` | Zephyr is ~80 repositories that west fetches; git submodules cannot follow Zephyr's own manifest. The version is pinned in git, setup is one script, and CI builds the port |
| Where the Zephyr drivers live | Next to the ESP32 ones in `main/` (`driver/<name>/zephyr.h/.cpp`) | Each driver's interface, ESP32, stub and Zephyr versions stay together; each build system lists its own files |
| Board | Own out-of-tree board definition, found through `BOARD_ROOT` | The Nano ESP32 is not in Zephyr; app overlays can use Arduino pin names (`arduino_nano_header`) |
| Where pins come from | Devicetree: drivers take a `gpio_dt_spec` / `adc_dt_spec` | Pin **and** polarity/gain come from `app.overlay`; no GPIO numbers in Zephyr code |
| How the factory maps hardware | Roles (`GpioId::StatusLed`, `AdcId::TempSensor`) instead of pin numbers | Each factory decides what a role means: `factory::Esp32s3` a pin, `factory::Zephyr` a devicetree node |
| ADC attenuation | `zephyr,gain` in devicetree; `referenceVoltage` is ignored | Zephyr configures channels from devicetree, not at runtime |
| ADC noise | Average of `AdcSamples` (5) reads in `readValue()`; `readVoltage()` uses it | Single reads are noisy; the same on ESP-IDF |
| Timer | Polled `timeout()`, restarts from "now" | `Logic` stays unchanged. A `k_timer` callback would be more idiomatic but changes the interface |
| Serial | Our own command parser kept, not Zephyr's shell | `Logic` stays unchanged on both platforms |
| NVS | Settings subsystem (NVS backend), not the raw NVS API | String keys like ESP-IDF (raw NVS has numeric IDs). The namespace is the key prefix (`user_settings/blink_state`). Every write goes to flash at once, so `autoCommit`/`commit()` have no effect |
| `std::cout` in `Logic` | Kept, via `CONFIG_POSIX_API` | `Logic` stays unchanged. Costs ~225 KB flash; Zephyr logging would be an improvement |
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

* LEDs, GPIO numbers and strapping pins are listed in [docs/board.md](docs/board.md).
* `gpio0` = GPIO 0–31, `gpio1` = GPIO 32–48 (pin = GPIO − 32).
* When copying another board, change **every** board-specific value: copied SPI pinctrl landed on the
  I2C pins and nothing complained.
* **Disabled nodes are not checked against their bindings.** Errors only show when a node is enabled –
  test the board with each bus `okay`.
* **Label, node name and alias** – `p02_led: led-d6 { ... }`, `aliases { p02-led = &p02_led; }`:
  label (`_`) for `&p02_led` in devicetree, node name (`-`) for description, alias (lowercase, `-`) for
  `DT_ALIAS(p02_led)` in code. Name aliases after the **role**, not the pin.
* New nodes go inside `/ { }`; changing an existing node (`&adc1 { }`) is written at the top level.
* A node with the same name as one in the board is **merged** into it; give the app's wiring its own
  parent (`p02-leds`).
* The same property twice in a node is not an error – the last one wins.
* ADC: `adc0` = the chip's ADC1, `adc1` = ADC2. `io-channels = <&adc1 2>` takes the **channel** number
  (A6 = GPIO13 = ADC2 ch 2), not the GPIO number. `ADC_GAIN_1_4` = 12 dB (~3.1 V full scale).
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
  `std::cout` (otherwise `undefined reference to 'open'`, `read`, ...).
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
* `miniterm --exit-char 24` exits with `Ctrl+X`; the default `Ctrl+]` is awkward on a Swedish keyboard.
