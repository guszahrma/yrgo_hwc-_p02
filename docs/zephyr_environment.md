# Zephyr environment

The application also runs on **Zephyr RTOS v4.4.2**. Thanks to the factory pattern, `logic::Logic` does
not change – Zephyr implementations of the drivers live next to the ESP32-S3 implementations, together
with a `factory::Zephyr`. Wi-Fi and MQTT are not ported yet.

## Setup

The Zephyr version is pinned in [`west.yml`](../west.yml) (the west manifest), and the repository itself is
the west workspace: Zephyr and the modules it needs are fetched into `deps/` inside the repository
(ignored by git). See [Design decisions](#design-decisions) for why Zephyr is not a git submodule.

All commands in this section run from the **repository root**, wherever you cloned it.

Set up once (needs the packages from [step 2](setup.md#2-packages-in-ubuntu) in Setup, ~800 MB, a few minutes):

```bash
tools/zephyr_setup.sh
```

The script creates `.venv/` (west and Zephyr's Python tools), `.west/config` and `deps/` (Zephyr and
`hal_espressif`), installs the Zephyr SDK with the ESP32-S3 compiler in `~/zephyr-sdk-<version>` if it is
not there yet, and fetches Espressif's binary blobs. Run it again after `west.yml` has changed (e.g. a new
Zephyr version) – it only updates what is needed.

Then, in **every new terminal**:

```bash
source tools/zephyr_env.sh
```

It activates `.venv`, sets `ZEPHYR_BASE` to `deps/zephyr` and exports `P02_ROOT` (the repository
root), which commands and scripts can use instead of a hard-coded path.

Optionally add an alias. Run this from the repository root: `$(pwd)` is replaced with the repository's
path when the line is written to `~/.bashrc`, so the alias works from any directory:

```bash
echo "alias get_zephyr='source $(pwd)/tools/zephyr_env.sh'" >> ~/.bashrc
```

> **Never mix environments in one terminal.** Use a fresh terminal for each of:
>
> | Environment | Activate with | Used for |
> |---|---|---|
> | ESP-IDF | `get_idf` | This project as it is today |
> | Zephyr (board) | `source tools/zephyr_env.sh` | The Zephyr port, flashing the board |
> | Zephyr course | `source ~/zephyr/env.sh` | Course exercises on `native_sim` |
>
> The course's `env.sh` sets `ZEPHYR_TOOLCHAIN_VARIANT=host` (the PC's own GCC), which breaks builds for
> the ESP32-S3.

## Building and flashing

### P02 on Zephyr (`zephyr/app/`)

`zephyr/app` runs the same `logic::Logic` as the ESP-IDF build, with `driver::factory::Zephyr` instead
of `factory::Esp32s3`. Start in the repository root, then build from the **app directory** (never from
the repository root – the root `build/` belongs to ESP-IDF):

```bash
source tools/zephyr_env.sh
cd zephyr/app
west build -p always -b arduino_nano_esp32/esp32s3/procpu .
west flash --esp-device /dev/ttyACM0 && python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
```

Exit the monitor with `Ctrl+X` (`--exit-char 24`; the default `Ctrl+]` is awkward on a Swedish
keyboard).

The [serial commands](usage.md#serial-commands) work over the USB port as on ESP-IDF. Wiring: LED on D6, TMP36
on A6 (`zephyr/app/app.overlay`). `store` saves the blink state and period with `nvs::Zephyr` (Zephyr's
Settings subsystem, in the board's `storage_partition`), so they are loaded again after a reset or a new
`west flash`. Not ported yet: Wi-Fi and MQTT.

### Our own samples (`zephyr/samples/`)

Our apps find the board through `list(APPEND BOARD_ROOT ...)` in their own `CMakeLists.txt` and set
`CONFIG_BOOT_DELAY` in `prj.conf`, so no extra flags are needed. From the repository root:

```bash
source tools/zephyr_env.sh
cd zephyr/samples/blinky
west build -p always -b arduino_nano_esp32/esp32s3/procpu .
west flash --esp-device /dev/ttyACM0 && python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
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
source tools/zephyr_env.sh
west build -p always -b arduino_nano_esp32/esp32s3/procpu $ZEPHYR_BASE/samples/hello_world -- \
  -DBOARD_ROOT=$P02_ROOT/zephyr \
  -DCONFIG_BOOT_DELAY=1000
```

* `-p always` – pristine build (start from scratch)
* `BOARD_ROOT` – the directory that contains `boards/`, i.e. the repository's `zephyr/` directory
* `CONFIG_BOOT_DELAY=1000` – waits 1 s before booting, see below

Flash (attach the board with usbipd first, see [Setup](setup.md#attaching-the-board-to-wsl)):

```bash
esptool --chip esp32s3 -p /dev/ttyACM0 erase-flash   # once, when switching from ESP-IDF
west flash --esp-device /dev/ttyACM0 && python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
```

Exit the monitor with `Ctrl+X`. Expected output:

```
*** Booting Zephyr OS build v4.4.2 (delayed boot 1000ms) ***
Hello World! arduino_nano_esp32/esp32s3/procpu
```

**Why `BOOT_DELAY`?** The console runs over the ESP32-S3's built-in USB. Every reset disconnects USB, and
usbipd has to re-attach the board to WSL before the monitor can open `/dev/ttyACM0`. The 1 s boot delay
keeps the boot output until the monitor is open. If the port or the boot output is missing on your
computer, see [Troubleshooting](#troubleshooting).

Useful files after a build:

| File | Contents |
|---|---|
| `build/zephyr/zephyr.dts` | The final devicetree (board + overlays), with the source file of every line |
| `build/zephyr/.config` | All active Kconfig options |
| `west build -t menuconfig` | Interactive Kconfig browser (`/` to search, `?` for help) |

## Troubleshooting

| Error | Cause / fix |
|---|---|
| `No board named 'arduino_nano_esp32' found` | Zephyr cannot find our board. Our apps: `list(APPEND BOARD_ROOT ...)` must come before `find_package(Zephyr)` in `CMakeLists.txt`. Zephyr's samples: add `-- -DBOARD_ROOT=$P02_ROOT/zephyr`. |
| `devicetree error: ... 'pinctrl-names' is marked as required` (after enabling `i2c0`/`spi2`) | Disabled nodes are not checked against their bindings – errors only show when a node is enabled. Test the board with each bus enabled. |
| `CMake was unable to find a build program corresponding to "Ninja"` | `pip install ninja` in the Zephyr venv, or `sudo apt install ninja-build`. |
| `west blobs fetch` fails with `504 Gateway Time-out` | Temporary GitHub error. Run `west blobs fetch hal_espressif` again. |
| Flashing works but the monitor shows nothing | Boot output was printed before the monitor connected (usbipd re-attached the board slowly). Increase `CONFIG_BOOT_DELAY` in the app's `prj.conf`, e.g. to `3000`. |
| `SerialException: device reports readiness to read but returned no data` | The board disconnected from USB (reset/re-attach). Start the monitor again. |
| `could not open port /dev/ttyACM0` (or the `SerialException` above) right after `west flash` | usbipd had not re-attached the board yet when the monitor started. Wait before opening the monitor: `west flash --esp-device /dev/ttyACM0 && sleep 2 && python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200` (increase the `sleep` if needed). |
| Strange compiler errors when building for the ESP32-S3 | The course's `env.sh` is active in the terminal. Open a new terminal and run `source tools/zephyr_env.sh`. |
| `error: the Zephyr workspace is not set up` | Run `tools/zephyr_setup.sh` first (once per clone). |
| `tools/zephyr_setup.sh: Permission denied`, or `git status` shows the script as modified with no changes (`git diff --summary`: `mode change 100755 => 100644`) | The file lost its executable bit (e.g. edited or created from Windows). `chmod +x tools/zephyr_setup.sh`. |
| `CMake Error: Not a file: .../zephyrproject/zephyr/cmake/pristine.cmake` | The app's `build/` was made with the old `~/zephyrproject` workspace, and `-p always` cleans it with the Zephyr it was built with. Delete it once: `rm -rf build`. |
| The build prints `Zephyr version: ... (~/zephyrproject/zephyr)` | An old `get_zephyr` alias from before `west.yml` is active. Remove it with `sed -i '/alias get_zephyr=/d' ~/.bashrc`, add the new one (see [Setup](#setup)) and open a new terminal. The old `~/zephyrproject` can then be deleted; keep `~/zephyr-sdk-<version>` (the compiler). |
| `west: unknown command "build"` | `ZEPHYR_BASE` is not set. Run `source tools/zephyr_env.sh`. |

## Design decisions and lessons learned

### Overview

The unchanged application (`logic::Logic`) runs on Zephyr RTOS v4.4.2 on the Arduino Nano ESP32. Only
the factory and the drivers differ from the ESP-IDF build:

```
zephyr/app/src/main.cpp ──► logic::Logic ──► factory::Zephyr  ──► driver::*::Zephyr  ──► Zephyr APIs
main/source/main.cpp    ──► logic::Logic ──► factory::Esp32s3 ──► driver::*::Esp32s3 ──► ESP-IDF
```

| Driver | Zephyr API | Configured from |
|---|---|---|
| `timer::Zephyr` | `k_uptime_get()`, polled | – |
| `gpio::Zephyr` | `gpio_dt_spec`, `gpio_pin_*_dt` | `app.overlay`: alias `p02-led` (D6) |
| `adc::Zephyr` (+ unchanged `Tmp36`) | `adc_dt_spec`, `adc_read_dt` | `app.overlay`: `zephyr,user` `io-channels` (A6 = ADC2 ch 2) |
| `serial::Zephyr` | `uart_poll_in/out` on the console UART (USB) | `zephyr,console` |
| `nvs::Zephyr` | Settings subsystem, NVS backend | `storage_partition` |

Where things are:

| Path | Contents |
|---|---|
| `main/include/driver/<name>/zephyr.h`, `main/source/driver/<name>/zephyr.cpp` | The drivers, next to the ESP32 and stub versions |
| `main/source/driver/factory/zephyr.cpp` | Maps the roles (`GpioId::StatusLed`, ...) to devicetree |
| `zephyr/app/` | The app: `CMakeLists.txt` (lists the sources from `main/`), `prj.conf`, `app.overlay`, `main.cpp` |
| `zephyr/boards/arduino/nano_esp32/` | Our board definition (Zephyr has none for this board) |
| `zephyr/samples/blinky/` | First exercise: LEDs from devicetree |
| `west.yml`, `tools/zephyr_setup.sh`, `tools/zephyr_env.sh` | The pinned Zephyr workspace |

#### Not ported / known issues

* **Wi-Fi and MQTT** are not ported. The ESP-IDF app does not use them either (`main.cpp` only runs
  `Logic`). Note: ADC2 cannot be used while Wi-Fi is on, so the TMP36 must move from A6 to A0-A3 first.
* `Logic`'s destructor resets the ADC before the temperature sensor that refers to it. Harmless today
  (`run()` never returns), but wrong.
* `gpio::Interface` says "high/low"; Zephyr's `gpio_pin_set_dt` means **active/inactive**. The two
  platforms differ for an active-low pin.
* The ESP32 ADC driver ignores the return values of `adc_oneshot_*`/`adc_cali_*`, so its errors are
  invisible.

---

### Design decisions

| Decision | Choice | Why |
|---|---|---|
| How Zephyr gets into the project | `west.yml` in this repository, which is the west workspace (T2); Zephyr and `hal_espressif` in `deps/` | Zephyr is ~80 repositories that west fetches; git submodules cannot follow Zephyr's own manifest. The version is pinned in git, setup is one script, and CI builds the port |
| Where the Zephyr drivers live | Next to the ESP32 ones in `main/` (`driver/<name>/zephyr.h/.cpp`) | Each driver's interface, ESP32, stub and Zephyr versions stay together; each build system lists its own files |
| Board | Own out-of-tree board definition, found through `BOARD_ROOT` | The Nano ESP32 is not in Zephyr; app overlays can use Arduino pin names (`arduino_nano_header`) |
| Where pins come from | Devicetree: drivers take a `gpio_dt_spec` / `adc_dt_spec` | Pin **and** polarity/gain come from `app.overlay`; no GPIO numbers in Zephyr code |
| How the factory maps hardware | Roles (`GpioId::StatusLed`, `AdcId::TempSensor`) instead of pin numbers (#60) | Each factory decides what a role means: `factory::Esp32s3` a pin, `factory::Zephyr` a devicetree node. The roles stay in `driver::factory` |
| ADC attenuation | `zephyr,gain` in devicetree; `referenceVoltage` is ignored | Zephyr configures channels from devicetree, not at runtime |
| Timer | Polled `timeout()`, restarts from "now" | `Logic` stays unchanged. A `k_timer` callback would be more idiomatic but changes the interface |
| Serial | Our own command parser kept, not Zephyr's shell | `Logic` stays unchanged on both platforms |
| NVS | Settings subsystem (NVS backend), not the raw NVS API | String keys like ESP-IDF (raw NVS has numeric IDs). The namespace is the key prefix (`user_settings/blink_state`). Every write goes to flash at once, so `autoCommit`/`commit()` have no effect |
| `std::cout` in `Logic` | Kept, via `CONFIG_POSIX_API` | `Logic` stays unchanged. Costs ~225 KB flash; Zephyr logging would be an improvement |
| ESP-IDF and Zephyr in one repository | Yes | The shared logic is built for both in CI; reorganise (`common/`, `esp-idf/`, `zephyr/`) if the ESP-IDF build is dropped |

---

### Pitfalls and lessons learned

#### West workspace

* `west init -l` makes the *parent* of the repository the workspace (`~`), which collides with other
  workspaces. `tools/zephyr_setup.sh` writes `.west/config` with `path = .` instead.
* `name-allowlist` fetches only `hal_espressif` (~800 MB instead of several GB).
* `path-prefix: deps` also applies to the zephyr project itself: declare it as `path: zephyr`
  (→ `deps/zephyr`); `path: deps/zephyr` ends up in `deps/deps/zephyr`.
* The SDK is per user (`~/zephyr-sdk-<version>`), not per workspace.
* An old `build/` remembers the Zephyr it was made with. After moving workspaces, `rm -rf build`.

#### Board and devicetree

* LEDs on the board (verified):

  | Alias | LED | GPIO | Polarity |
  |---|---|---|---|
  | `led0` | Yellow (D13) | GPIO48 | Active high |
  | `led1` / `led2` / `led3` | RGB green / blue / red | GPIO0 / GPIO45 / GPIO46 | Active low (common anode) |

  The schematic does not say which RGB pin is which colour; found by testing, matches Arduino's
  `pins_arduino.h` for the Nano ESP32. GPIO0 and GPIO46 are strapping pins.
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
* Build from the app directory; the root `build/` belongs to ESP-IDF.

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

* `override` catches signatures that do not match the interface.
* Zephyr calls return negative error codes. Store them in an `int` before converting to `bool`, or an
  error becomes `true`.
* Update driver state (`myState`) only after the call succeeded.
* A `noexcept` constructor cannot report failure – a `myReady`/`myValid` flag records it and the other
  functions check it.
* ADC: `buffer_size` in `adc_sequence` is in **bytes**; `adc_raw_to_millivolts_dt` needs an `int32_t`.
* NVS: Settings stores raw bytes, so `set<T>`/`get<T>` are `&value, sizeof(T)` with
  `static_assert(std::is_integral_v<T>)`. `eraseAll()` collects the keys with
  `settings_load_subtree_direct()` first and erases them afterwards.
* P02 only saves on the `store` command – a changed `period` is not kept after a reset by itself.
* Remove what the copied ESP32 code needed but Zephyr does not – unused code gives warnings that hide
  real ones.

#### USB console (usbipd/WSL)

* Every reset re-enumerates USB, and usbipd has to re-attach the board to WSL. `CONFIG_BOOT_DELAY=1000`
  keeps the boot output, and the monitor is started right after `west flash` without a `sleep`. We first
  used a 10 s delay and `sleep 9` – estimated, not measured; tested on both our computers, neither is
  needed. With a slower usbipd, add a `sleep` before the monitor (see the readme's troubleshooting).
* `miniterm --exit-char 24` exits with `Ctrl+X`; the default `Ctrl+]` is awkward on a Swedish keyboard.
* `SerialException: device reports readiness to read but returned no data` = the board reset or was
  flashed. Start the monitor again.

#### Git

* `.gitignore` ignores files without an extension – Zephyr's `Kconfig` and `*_defconfig` are excepted.
  Build from a fresh clone before opening a PR.
* Stacked PRs: merge bottom-up with **"Create a merge commit"**, then retarget the next PR to `main`.
* Run `tools/clang-format/fix.sh` before committing; CI uses the pinned version in
  `tools/clang-format/version.txt`.
* A file created from Windows can lose its executable bit (`git diff --summary` shows
  `mode change 100755 => 100644`); fix with `chmod +x`.
