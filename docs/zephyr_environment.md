# Zephyr environment

P02 on **Zephyr RTOS v4.4.2**: a port of the ESP-IDF build. The Zephyr drivers are in `main/` next to the
ESP-IDF ones (`driver/<name>/zephyr.h/.cpp`), and `zephyr/app/src/main.cpp` runs the unchanged
`logic::Logic` with `factory::Zephyr`. Wi-Fi and MQTT are not ported yet. Common setup (WSL, usbipd,
cloning) is in [Setup](setup.md).

## Setup

### 1. Install Zephyr (once)

The Zephyr version is pinned in [`west.yml`](../west.yml) (the west manifest), and the repository itself is
the west workspace: Zephyr and the modules it needs are fetched into `deps/` inside the repository
(ignored by git). See [Design decisions](#design-decisions) for why Zephyr is not a git submodule.

From the **repository root** (~800 MB, a few minutes):

```bash
tools/zephyr_setup.sh
```

The script creates `.venv/` (west and Zephyr's Python tools), `.west/config` and `deps/` (Zephyr and
`hal_espressif`), installs the Zephyr SDK with the ESP32-S3 compiler in `~/zephyr-sdk-<version>` if it is
not there yet, and fetches Espressif's binary blobs. Run it again after `west.yml` has changed (e.g. a new
Zephyr version) – it only updates what is needed.

### 2. Activate the environment (every new terminal)

```bash
source tools/zephyr_env.sh
```

It activates `.venv`, sets `ZEPHYR_BASE` to `deps/zephyr` and exports `P02_ROOT` (the repository root),
which commands and scripts can use instead of a hard-coded path.

Optionally add an alias. Run this from the repository root: `$(pwd)` is replaced with the repository's
path when the line is written to `~/.bashrc`, so the alias works from any directory:

```bash
echo "alias get_zephyr='source $(pwd)/tools/zephyr_env.sh'" >> ~/.bashrc
```

Do not mix it with the ESP-IDF environment in the same terminal (see [Setup](setup.md#next-set-up-a-platform)).

## Building and flashing

### P02 (`zephyr/app/`)

From the **repository root** (attach the board first, see [Setup](setup.md#4-attach-the-board-to-wsl-powershell-as-administrator)):

```bash
source tools/zephyr_env.sh
west build -p always -b arduino_nano_esp32/esp32s3/procpu -s zephyr/app -d zephyr/app/build
west flash -d zephyr/app/build --esp-device /dev/ttyACM0 && python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
```

`-s` is the app and `-d` its build directory. Always give `-d`: without it, west builds in `./build`, which
belongs to ESP-IDF. Inside `zephyr/app` the short form `west build -p always -b arduino_nano_esp32/esp32s3/procpu .`
and `west flash --esp-device /dev/ttyACM0` do the same.

Exit the monitor with `Ctrl+X` (`--exit-char 24`; the default `Ctrl+]` is awkward on a Swedish keyboard).

Then use the [serial commands](usage.md#serial-commands).

### Our samples (`zephyr/samples/`)

Our apps find the board through `list(APPEND BOARD_ROOT ...)` in their own `CMakeLists.txt` and set
`CONFIG_BOOT_DELAY` in `prj.conf`, so no extra flags are needed:

```bash
west build -p always -b arduino_nano_esp32/esp32s3/procpu -s zephyr/samples/blinky -d zephyr/samples/blinky/build
west flash -d zephyr/samples/blinky/build --esp-device /dev/ttyACM0 && python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
```

Expected: the yellow LED blinks and the RGB LED cycles red → green → blue, with one line per second:

```
Yellow: OFF  RGB: green
Yellow: ON   RGB: blue
```

### Zephyr's samples

Zephyr's own samples don't know about our board, so `BOARD_ROOT` is passed on the command line.
Example with `hello_world`:

```bash
west build -p always -b arduino_nano_esp32/esp32s3/procpu -s $ZEPHYR_BASE/samples/hello_world -d hello_world_build -- \
  -DBOARD_ROOT=$P02_ROOT/zephyr \
  -DCONFIG_BOOT_DELAY=1000
west flash -d hello_world_build --esp-device /dev/ttyACM0 && python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
```

* `-p always` – pristine build (start from scratch)
* `BOARD_ROOT` – the directory that contains `boards/`, i.e. the repository's `zephyr/` directory
* `CONFIG_BOOT_DELAY=1000` – waits 1 s before booting, see below

Expected output:

```
*** Booting Zephyr OS build v4.4.2 (delayed boot 1000ms) ***
Hello World! arduino_nano_esp32/esp32s3/procpu
```

**Why `BOOT_DELAY`?** The console runs over the ESP32-S3's built-in USB. Every reset disconnects USB, and
usbipd has to re-attach the board to WSL before the monitor can open `/dev/ttyACM0`. The 1 s boot delay
keeps the boot output until the monitor is open. If the boot output is missing on your computer, see
[Troubleshooting](#troubleshooting).

### Useful files and commands

| File / command | Contents |
|---|---|
| `west build -d zephyr/app/build -t menuconfig` | Interactive Kconfig browser (`/` to search, `?` for help) |
| `zephyr/app/build/zephyr/.config` | All active Kconfig options |
| `zephyr/app/build/zephyr/zephyr.dts` | The final devicetree (board + overlays), with the source file of every line |
| `zephyr/app/build/zephyr/zephyr.bin`, `zephyr.elf` | Firmware, ELF with symbols |

## Troubleshooting

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

## Design decisions and lessons learned

### Overview

| Driver | Zephyr API | Configured from |
|---|---|---|
| `timer::Zephyr` | `k_uptime_get()`, polled | – |
| `gpio::Zephyr` | `gpio_dt_spec`, `gpio_pin_*_dt` | `app.overlay`: alias `p02-led` (D6) |
| `adc::Zephyr` (+ `Tmp36`) | `adc_dt_spec`, `adc_read_dt`, average of 5 samples | `app.overlay`: `zephyr,user` `io-channels` (A6 = ADC2 ch 2) |
| `serial::Zephyr` | `uart_poll_in/out` on the console UART (USB) | `zephyr,console` |
| `nvs::Zephyr` | Settings subsystem, NVS backend | `storage_partition` |

Where things are:

| Path | Contents |
|---|---|
| `main/include/driver/<name>/zephyr.h`, `main/source/driver/<name>/zephyr.cpp` | The drivers, next to the ESP32 and stub versions |
| `main/source/driver/factory/zephyr.cpp` | Maps the roles (`GpioId::StatusLed`, ...) to devicetree |
| `zephyr/app/` | The app: `CMakeLists.txt` (lists the sources from `main/`), `prj.conf`, `app.overlay`, `main.cpp` |
| `zephyr/boards/arduino/nano_esp32/` | Our board definition (Zephyr has none for this board), see [Board](board.md) |
| `zephyr/samples/blinky/` | First exercise: LEDs from devicetree |
| `west.yml`, `tools/zephyr_setup.sh`, `tools/zephyr_env.sh` | The pinned Zephyr workspace |

#### Known issues

* **Wi-Fi and MQTT** are not ported. The ESP-IDF app does not use them either (`main.cpp` only runs
  `Logic`). ADC2 cannot be used while Wi-Fi is on, so the TMP36 must move from A6 to A0–A3 first.

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

* LEDs, GPIO numbers and strapping pins are listed in [Board](board.md).
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
  `sleep` before the monitor (see [Setup → Troubleshooting](setup.md#troubleshooting)).
* `miniterm --exit-char 24` exits with `Ctrl+X`; the default `Ctrl+]` is awkward on a Swedish keyboard.
