# Zephyr port – design and lessons learned

For whoever maintains or extends the Zephyr port of P02: **why** the code looks the way it does, and the
pitfalls that cost us time. How to set up, build and flash is in the [readme](../readme.md#zephyr-port).

---

## Overview

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

### Not ported / known issues

* **Wi-Fi and MQTT** are not ported. The ESP-IDF app does not use them either (`main.cpp` only runs
  `Logic`). Note: ADC2 cannot be used while Wi-Fi is on, so the TMP36 must move from A6 to A0-A3 first.
* `Logic`'s destructor resets the ADC before the temperature sensor that refers to it. Harmless today
  (`run()` never returns), but wrong.
* `gpio::Interface` says "high/low"; Zephyr's `gpio_pin_set_dt` means **active/inactive**. The two
  platforms differ for an active-low pin.
* The ESP32 ADC driver ignores the return values of `adc_oneshot_*`/`adc_cali_*`, so its errors are
  invisible.

---

## Design decisions

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

## Pitfalls and lessons learned

### West workspace

* `west init -l` makes the *parent* of the repository the workspace (`~`), which collides with other
  workspaces. `tools/zephyr_setup.sh` writes `.west/config` with `path = .` instead.
* `name-allowlist` fetches only `hal_espressif` (~800 MB instead of several GB).
* `path-prefix: deps` also applies to the zephyr project itself: declare it as `path: zephyr`
  (→ `deps/zephyr`); `path: deps/zephyr` ends up in `deps/deps/zephyr`.
* The SDK is per user (`~/zephyr-sdk-<version>`), not per workspace.
* An old `build/` remembers the Zephyr it was made with. After moving workspaces, `rm -rf build`.

### Board and devicetree

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

### Kconfig and CMake

* No spaces around `=` in `prj.conf`, and end the file with a newline.
* Board defconfig = what every app needs; `prj.conf` = what this app uses.
* `CONFIG_SETTINGS_NVS` does not need to be set: the backend defaults to NVS when `CONFIG_NVS` is on.
* `BOARD_ROOT` and `EXTRA_DTC_OVERLAY_FILE` must be set **before** `find_package(Zephyr)`.
* Every `.cpp` must be in `target_sources` – a missing one is a **linker** error
  (`undefined reference to ...`), not a compiler error.
* Build from the app directory; the root `build/` belongs to ESP-IDF.

### C++ on Zephyr

* `CONFIG_CPP`, `CONFIG_STD_CPP17`, `CONFIG_REQUIRES_FULL_LIBCPP`, and `CONFIG_POSIX_API` for
  `std::cout` (otherwise `undefined reference to 'open'`, `read`, ...).
* `sleep()` takes **seconds**. Use `std::this_thread::sleep_for(std::chrono::milliseconds(...))`
  (`<thread>`, `<chrono>`); `usleep` gives header conflicts between Zephyr's POSIX headers and picolibc.
* `printk` formats must match the type (`%lld` + cast for `int64_t`); no `%f` by default.
* A C callback can be a lambda **without capture** (`[]`); pass data through the `void* param` argument.
* Keep signed return values signed (`ssize_t`); `const size_t n{ssize_t_value}` is a narrowing error
  with `{}` – which is the point of `{}`.

### Drivers

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

### USB console (usbipd/WSL)

* Every reset re-enumerates USB; usbipd needs ~8 s to re-attach. `CONFIG_BOOT_DELAY=10000` keeps the
  boot output.
* `SerialException: device reports readiness to read but returned no data` = the board reset or was
  flashed. Start the monitor again.

### Git

* `.gitignore` ignores files without an extension – Zephyr's `Kconfig` and `*_defconfig` are excepted.
  Build from a fresh clone before opening a PR.
* Stacked PRs: merge bottom-up with **"Create a merge commit"**, then retarget the next PR to `main`.
* Run `tools/clang-format/fix.sh` before committing; CI uses the pinned version in
  `tools/clang-format/version.txt`.
* A file created from Windows can lose its executable bit (`git diff --summary` shows
  `mode change 100755 => 100644`); fix with `chmod +x`.
