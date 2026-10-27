# Zephyr port

Plan, division of work and notes for porting P02 from ESP-IDF to Zephyr RTOS (v4.4.2).
Setup, build and flash instructions are in the [readme](../readme.md#zephyr-port).

---

## Goal

Run the existing application (`logic::Logic`) on Zephyr without changing it, by adding Zephyr
implementations of the driver interfaces and a `factory::Zephyr` – the same way `factory::Esp32s3` and
`factory::Stub` exist today.

---

## How Zephyr gets into the project (west, not submodules)

Zephyr is not a single repository: it is Zephyr plus ~80 modules (HALs such as `hal_espressif`,
libraries such as `mbedtls`). **west** fetches them at pinned versions from a manifest (`west.yml`) and
tells the build where they are. This is why Zephyr is not added as a git submodule.

| Topology | Manifest repository | Example |
|---|---|---|
| T1 | Zephyr itself | `~/zephyrproject` (what we used at first) |
| T2 ("star") | The application | This repository, the course repository `~/zephyr` |
| T3 | A separate manifest repository | Large organisations with many applications |

**This repository is a T2 workspace** (done in step 9):

```
yrgo_hwc-_p02/
├── west.yml       ← Zephyr v4.4.2 + name-allowlist: [hal_espressif]
├── .west/config   ← written by tools/zephyr_setup.sh, not committed
├── .venv/         ← west and Zephyr's Python tools, not committed
├── deps/          ← Zephyr and hal_espressif, fetched by `west update`, not committed
└── zephyr/        ← our Zephyr code
```

Anyone (or CI) gets the exact same Zephyr with `tools/zephyr_setup.sh`, then
`source tools/zephyr_env.sh` in each terminal.

Things we learned setting it up:

* **`west init -l` was not used.** It makes the *parent* of the manifest repository the workspace, i.e.
  `~` – which would collide with the course workspace in `~/zephyr`. Instead the script writes
  `.west/config` with `path = .`, so the repository itself is the workspace.
* **`name-allowlist`** fetches only `hal_espressif` instead of all ~80 modules (~800 MB instead of
  several GB).
* **`path-prefix: deps` also applies to the zephyr project itself**, so it is declared with
  `path: zephyr` and ends up in `deps/zephyr`. With `path: deps/zephyr` it landed in
  `deps/deps/zephyr`.
* The SDK (compiler) is per user (`~/zephyr-sdk-<version>`), not per workspace; the script only
  installs it if the ESP32-S3 toolchain is missing.

---

## Directory structure

```
zephyr/
├── boards/
│   └── arduino/nano_esp32/                   ← our board definition: arduino_nano_esp32 (done)
│       ├── arduino_nano_esp32_procpu.dts        the board: console, LEDs, aliases, peripherals
│       ├── arduino_nano_esp32-pinctrl.dtsi      UART/I2C/SPI pins
│       ├── arduino_nano_connector.dtsi          Nano pin header D0-D13, A0-A7
│       └── board.yml, Kconfig*, board.cmake, *_defconfig, *.yaml
├── samples/                                  ← our own learning exercises
│   └── blinky/                                  (done)
│       ├── CMakeLists.txt                       ← finds the board with BOARD_ROOT
│       ├── prj.conf
│       └── src/main.c
└── app/                                      ← the P02 port itself (in progress)
    ├── CMakeLists.txt                           BOARD_ROOT, main/include, driver sources from main/
    ├── prj.conf                                 C++17, full libstdc++, POSIX API, ADC
    ├── app.overlay                              P02's wiring: status LED on D6, TMP36 on A6
    └── src/main.cpp                             runs logic::Logic with factory::Zephyr
```

The Zephyr drivers live next to the ESP32 ones in `main/`, same pattern as `esp32s3` and `stub`:

```
main/include/driver/<name>/zephyr.h       main/source/driver/<name>/zephyr.cpp
```

ESP-IDF's `main/CMakeLists.txt` lists its sources explicitly, so it never compiles the `zephyr.cpp`
files; the Zephyr app lists only those.

The board describes what is soldered onto every Nano ESP32. What we wire up ourselves (P02's LED and
TMP36) belongs in the app's `app.overlay`, written with Arduino pin names via `arduino_nano_header`.

---

## Order of work

Each step maps to a lecture in the Zephyr course (`~/zephyr`): do the exercise on `native_sim` there
first, then apply it to P02 on the real board.

| # | Step | Course topic | Status |
|---|---|---|---|
| 0 | Environment, hello_world on the board | – | ✅ |
| 1 | Blinky on the yellow LED with our own overlay | Devicetree, GPIO | ✅ `zephyr/samples/blinky`: yellow LED + RGB colour cycle |
| 2 | Own Zephyr app skeleton: `Logic` builds and runs with **only stubs** | West, build, Kconfig | ✅ `zephyr/app` runs the unchanged `logic::Logic` with `factory::Zephyr` (real drivers, `nvs::Stub`) |
| 3 | `timer::Zephyr` | Time, timers, work queues | ✅ `k_uptime_get()`, tested: 500 ms period |
| 4 | `gpio::Zephyr`, then pins from devicetree (`gpio_dt_spec`) instead of `9U` | Devicetree, GPIO | ✅ takes a `gpio_dt_spec`; LED on D6 toggles with `timer::Zephyr`. How `9U` maps to a pin is decided in `factory::Zephyr` |
| 5 | `serial::Zephyr` – or replace our command parser with Zephyr's shell (design decision) | Shell | ✅ our parser kept; `serial::Zephyr` uses the console UART (USB) with `uart_poll_in/out` |
| 6 | `adc::Zephyr` + overlay – TMP36 then works as-is | Devicetree, driver model | ✅ takes an `adc_dt_spec`; P02's unchanged `Tmp36` reads ~21 °C |
| 7 | `nvs` via the Settings subsystem | Settings | ⬜ |
| 8 | Own board definition `arduino_nano_esp32` | Devicetree, bindings | ✅ `zephyr/boards/arduino/nano_esp32` – done early, so the port is built on the real board from the start |
| 9 | Own `west.yml` (T2 workspace) + Zephyr build in CI | West manifest | ✅ `west.yml`, `tools/zephyr_setup.sh`, `tools/zephyr_env.sh`, CI job `zephyr-build` |
| 10 | Wi-Fi and MQTT | Networking | ⬜ |

---

## How we work

* **One driver per branch and PR. The other person always reviews.** Reviewing each other's Zephyr code
  is half the learning.
* **Pair programming** for steps that set the structure (2, 8, 9, 10): one types (driver), one thinks and
  reads docs (navigator); switch every 25 minutes.
* **Parallel work** for drivers once the skeleton exists, e.g.:

  | | Person A | Person B |
  |---|---|---|
  | 1 | `timer::Zephyr` | `gpio::Zephyr` |
  | 2 | `serial::Zephyr` | `adc::Zephyr` + overlay |
  | 3 | `nvs` via Settings | Pins from devicetree |

* **Small PRs.** Three small ones are better than one big one.
* **Write down what you learn** in the notes below – every "aha", every error and how it was solved.

---

## Design decisions

| Decision | Choice | Why |
|---|---|---|
| Where the Zephyr drivers live | Next to the ESP32 ones in `main/` (`driver/<name>/zephyr.h/.cpp`) | Each driver's interface, ESP32, stub and Zephyr versions stay together; both build systems list their own files |
| Where pins come from | Devicetree: the drivers take a `gpio_dt_spec` / `adc_dt_spec` | Pin **and** polarity/attenuation come from `app.overlay`; no GPIO numbers in Zephyr code |
| How the factory maps hardware to devicetree | **C:** the factory takes a role (`GpioId::StatusLed`, `AdcId::TempSensor`) instead of a pin number (#60) | Each factory decides what a role means: `factory::Esp32s3` maps it to a pin, `factory::Zephyr` to a devicetree node (`DT_ALIAS(p02_led)`, `zephyr,user` `io-channels`). The roles stay in `driver::factory` |
| ADC attenuation | From `zephyr,gain` in devicetree; `referenceVoltage` is not used | In Zephyr the channel is configured from devicetree, not at runtime |
| Timer | Polled `timeout()` kept, restarts from "now" | `Logic` stays unchanged. A `k_timer` with a callback would be more idiomatic but changes the interface |
| `std::cout` in `Logic` | Kept, via `CONFIG_POSIX_API` | `Logic` stays unchanged. Costs ~225 KB flash; Zephyr logging would be a later improvement |
| ESP-IDF and Zephyr in one repository | Yes | The shared logic is tested on both while the port is in progress; reorganise (`common/`, `esp-idf/`, `zephyr/`) when stable |

---

## Notes

### Board / hardware

* The Arduino Nano ESP32 is not in Zephyr. We first used `esp32s3_devkitc/esp32s3/procpu` + overlays,
  now our own board definition `arduino_nano_esp32/esp32s3/procpu`.
* The console must go over `usb_serial` (USB-Serial/JTAG) – the devkitc uses `uart0`.
* USB re-enumerates on every reset; with usbipd the board is gone for ~8 s. Use `CONFIG_BOOT_DELAY`.
* The devkitc definition says 8 MB flash, the Nano has 16 MB. The reason: the devkitc includes
  `esp32s3_wroom_n8.dtsi` – a WROOM module with 8 MB flash (`n8`). Our board uses
  `esp32s3_wroom_n16r8.dtsi` (16 MB flash, 8 MB PSRAM), which matches the NORA-W106.
* LEDs (verified on the board, defined in `arduino_nano_esp32_procpu.dts`):

  | Alias | LED | GPIO | Zephyr | Polarity |
  |---|---|---|---|---|
  | `led0` | Yellow ("SCK LED", D13) | GPIO48 | `&gpio1 16` | Active high (N-MOSFET, gate high = on) |
  | `led1` | RGB green | GPIO0 | `&gpio0 0` | Active low (common anode to 3V3) |
  | `led2` | RGB blue | GPIO45 | `&gpio1 13` | Active low |
  | `led3` | RGB red | GPIO46 | `&gpio1 14` | Active low |

  The schematic does not show which RGB pin is which colour. Found by testing, and matches Arduino's
  pin definitions for the Nano ESP32 (`LED_RED = 46`, `LED_GREEN = 0`, `LED_BLUE = 45` in
  `variants/arduino_nano_nora/pins_arduino.h` in espressif/arduino-esp32).
* GPIO number → Zephyr: `gpio0` = GPIO 0–31, `gpio1` = GPIO 32–48 (pin = GPIO − 32).
* GPIO0 and GPIO46 are boot strapping pins (GPIO0 low at reset = download mode, same as the B1 trick).
* D13 (GPIO48) is both SPI SCK and the yellow LED – the LED flickers while SPI is in use.
* A0-A3 are on ADC1, A4-A7 on ADC2. **ADC2 cannot be used while Wi-Fi is active** – P02's TMP36 on A6
  will have to move to A0-A3 when Wi-Fi is added.

### Board definition

* A board is found by its directory: `<BOARD_ROOT>/boards/<vendor>/<board>/`. Zephyr only searches its
  own tree, so apps add ours with `list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)` **before**
  `find_package(Zephyr)`. `BOARD_ROOT` is the directory that *contains* `boards/`.
* The boilerplate files (`board.yml`, `Kconfig*`, `board.cmake`, `defconfig`, `.yaml`) can be copied
  from a board with the same chip and renamed. The interesting part is the devicetree.
* **When copying from another board, change every board-specific value.** Copying devkitc's SPI pinctrl
  verbatim put SPI on GPIO11/12 – the same pins as our I2C – and the compiler did not complain.
  Nothing checks pinctrl groups for overlapping pins.
* **`gpio-map` vs `pinmux`** – look alike, do different things:
  * `gpio-map` (connector): "Arduino pin D0 *is* GPIO44" – for apps using a pin as plain GPIO.
  * `pinmux` (pinctrl): "route the UART0 RX *signal* to GPIO44" – one macro (`UART0_RX_GPIO44`) holds
    both signal and pin. The ESP32-S3 GPIO matrix can route almost any signal to almost any pin.
* A pinctrl group holds pins that need the same electrical settings: UART TX `output-high` (idle level),
  RX `bias-pull-up` (no garbage from a floating pin), I2C `drive-open-drain` + pull-up (devices only
  pull low), SPI COPI `output-low` (no noise before SPI starts).
* `pinctrl-0` and `pinctrl-names = "default"` always go together – the binding requires both.
* **Disabled nodes are not checked against their bindings.** `i2c0` without `pinctrl-names` built fine
  while disabled and failed as soon as it was enabled. Test the board with each bus enabled (an overlay
  with `status = "okay"`).
* Buses are `disabled` in the board and enabled by the app that uses them: unused buses cost no memory or
  power, and their pins stay free for GPIO or ADC.
* `arduino_nano_i2c: &i2c0 {};` adds a second label to an existing node, so overlays can use the Arduino
  name and work on any board with a Nano header.
* `#include <zephyr/dt-bindings/gpio/arduino-nano-header.h>` + `gpio-map` makes the connector a GPIO
  nexus: `<&arduino_nano_header 6 ...>` resolves to `<&gpio0 9 ...>` at build time (checked in
  `devicetree_generated.h`: pin 9 on gpio0).
* The devicetree compiler reports a missing `;` on the line *after* the real mistake.

### Devicetree

* `DT_ALIAS(led0)` looks in the **final** devicetree (`build/zephyr/zephyr.dts`), which is the board's
  `.dts` + automatically found overlays (`boards/<board>.overlay`, `app.overlay`) + `EXTRA_DTC_OVERLAY_FILE`.
* A sample's `boards/<board_name_with_underscores>.overlay` is only used when building for exactly that
  board.
* **Label vs alias** – two different names with different rules:

  | | Label (`yellow_led: led_0 {...}`) | Alias (`aliases { led0 = &yellow_led; }`) |
  |---|---|---|
  | Characters | Like C: letters, digits, `_` | Lowercase letters, digits, `-` |
  | Used by | Devicetree only (`&yellow_led`) | C code (`DT_ALIAS(led0)`; `-` becomes `_`) |
  | Name | Free choice | What the code looks for (`led0` for blinky) |

* `devicetree error: /aliases: undefined node label 'led0'` = something refers to `&led0` but no node has
  that label. Read the **first** line of a devicetree error – the CMake lines after it are follow-up noise.
  Devicetree errors happen during `find_package(Zephyr)`, before any C code is compiled.
* Defining the same property twice in one node is **not** an error – the last one silently wins. One LED
  = one child node with one `gpios`; an RGB LED is three LEDs.
* `gpios = <&controller pin flags>` – the flags (`GPIO_ACTIVE_HIGH/LOW`) say what "active" means
  electrically, so the C code can say "on" without knowing the wiring.
* `GPIO_ACTIVE_HIGH` works in an overlay without `#include`, because the board's `.dts` and all overlays
  are concatenated into one file before the C preprocessor runs, and the board includes
  `dt-bindings/gpio/gpio.h` indirectly (via `esp32s3_common.dtsi`). Include it explicitly anyway, so the
  overlay does not depend on what the board happens to include.
* **Label, node name and alias** in one node – `p02_led: led-d6 { ... }` and
  `aliases { p02-led = &p02_led; }`:

  | | Example | Characters | Used for |
  |---|---|---|---|
  | Label | `p02_led` | letters, digits, `_` | `&p02_led` inside devicetree – `&` only works with labels |
  | Node name | `led-d6` | letters, digits, `-` | describes the node; shown in `zephyr.dts` |
  | Alias | `p02-led` | lowercase, digits, `-` | C/C++: `DT_ALIAS(p02_led)` |

  Name the alias after the **role** (`p02-led`), not the pin (`ledd6`) – moving the LED then changes the
  overlay only, not the code.
* Inside `/ { ... }` you create **new** nodes. Changing an **existing** node (`&adc1 { ... };`,
  `&usb_serial { ... };`) is written at the top level, after the root node is closed.
* A node with the same name as one in the board (`leds { }`) is **merged** into it. Give the app's own
  wiring its own parent node (`p02-leds`) to keep board and breadboard separate.
* App overlay order: `app.overlay` is applied **before** `EXTRA_DTC_OVERLAY_FILE`, so a later overlay can
  silently overwrite a value (a wrong `compatible` once "worked" only because of this).
* ADC: `adc0` is the chip's **ADC1**, `adc1` is **ADC2** (`unit = <2>`). `io-channels = <&adc1 2>` takes
  the **channel** number (A6 = GPIO13 = ADC2 channel 2), not the GPIO number. The same number goes in
  `io-channels`, `channel@2` and `reg = <2>`. `zephyr,gain` sets the attenuation: `ADC_GAIN_1` = 0 dB,
  `ADC_GAIN_4_5` = 2.5 dB, `ADC_GAIN_1_2` = 6 dB, `ADC_GAIN_1_4` = 12 dB (~3.1 V full scale).
* `zephyr,user { io-channels = ...; }` is how an app asks for ADC channels;
  `ADC_DT_SPEC_GET(DT_PATH(zephyr_user))` gets index 0.

### Kconfig

* `-DCONFIG_X=y` on the command line works, but belongs in `prj.conf` so everyone builds the same way.
* **No spaces around `=`**: `CONFIG_GPIO = y` is read as the symbol `GPIO ` (with a space), which does
  not exist, and the build fails.
* Check the result in the build's own `build/zephyr/.config` – every app/build has its own. Grepping
  another build directory shows that build's settings, not yours.
* `CONFIG_BOOT_DELAY` is in milliseconds: `1000` = 1 s, too short for usbipd (~8 s). We use `10000`.
* **Board defconfig vs `prj.conf`:** the board's defconfig only enables what every app on the board needs
  (console, GPIO, clock). What an app uses goes in its `prj.conf` (`CONFIG_ADC`, `CONFIG_CPP`, ...) –
  the same split as `disabled` buses in the board and `okay` in the app overlay.
* `CONFIG_ADC=y` enables the ADC subsystem; the driver (`CONFIG_ADC_ESP32`) turns on by itself when an
  ADC node is `okay` in devicetree.
* `prj.conf` must end with a newline – otherwise a line appended later is glued onto the last one
  (`10000CONFIG_POSIX_API=y`).

### CMake (application `CMakeLists.txt`)

* Overlays are set with `set(EXTRA_DTC_OVERLAY_FILE file1 file2)` – a CMake list, one value per line, no
  `;` needed. `find_package` is for finding packages, not for setting variables.
* It must come **before** `find_package(Zephyr ...)`, because that is where the devicetree is generated.
* `DTC_OVERLAY_FILE` **replaces** automatically found overlays (`app.overlay`, `boards/<board>.overlay`);
  `EXTRA_DTC_OVERLAY_FILE` **adds** to them.
* Use paths relative to `${CMAKE_CURRENT_SOURCE_DIR}`, not `$HOME/...` (bash syntax that CMake does not
  understand, and it breaks for anyone who clones the repo elsewhere, including CI).
* Build from the app directory (`zephyr/samples/blinky`), never from the repository root – the root
  `build/` belongs to ESP-IDF.
* Every `.cpp` must be listed in `target_sources`. A missing one gives a **linker** error
  (`undefined reference to 'driver::adc::Zephyr::...'`), not a compiler error.

### C++ in Zephyr

* `prj.conf`: `CONFIG_CPP=y`, `CONFIG_STD_CPP17=y`, `CONFIG_REQUIRES_FULL_LIBCPP=y` (for
  `std::unique_ptr`, `std::string`, `std::cout`).
* `std::cout` fails to link (`undefined reference to 'open'`, `read`, `write`, `close`, `lseek`,
  `fstat`): libstdc++'s streams need POSIX I/O functions. `CONFIG_POSIX_API=y` provides them (and
  `sleep()`, which `logic.cpp` uses). `CONFIG_POSIX_DEVICE_IO` / `CONFIG_POSIX_FILE_SYSTEM` alone were
  not enough. Cost: FLASH went from ~135 KB to ~361 KB.
* Compiler vs linker errors: `error: ... was not declared` = something wrong in the code;
  `undefined reference to ...` = the code is fine but an implementation is missing (a `.cpp` not built,
  a library or Kconfig option not enabled).
* `printk` format must match the type: `%d` int, `%u` unsigned, `%lld` long long. For `int64_t`, cast to
  `long long`; for `std::uint32_t`, cast to `unsigned`. `%d` with an `int64_t` may *look* right and is
  still wrong. `printk` has no `%f` by default – print millivolts as an integer instead.
* `k_uptime_get` without `()` is the function's **address**, not the time.

### Drivers (timer, gpio, adc)

* An implementation must match the interface's signature exactly. `override` catches it
  (`setPeriod(std::int64_t)` vs the interface's `setPeriod(std::uint32_t)`).
* `Zephyr() = delete;` next to a declared `Zephyr() noexcept;` is the same constructor twice – a
  compile error. Only delete the default constructor if the class has no other use for it.
* Timer: `k_uptime_get()` returns `int64_t` milliseconds – store times as `std::int64_t` to avoid
  signed/unsigned mixing. The ESP32 timer is also a "software" timer (it reads `esp_timer_get_time()`);
  the kernel multiplexes all timeouts on one hardware timer. Real hardware timers (TIMG) are available
  through Zephyr's counter API (`timer0`-`timer3`, disabled in the board).
* Zephyr calls return error codes (`gpio_pin_get_dt` returns 1, 0 or **negative**). Store them in an
  `int` before converting to `bool` – otherwise an error becomes `true`.
* Update the driver's state (`myState`) only **after** the call succeeded – a missing update made
  `toggle()` get stuck "on".
* A `noexcept` constructor cannot throw, so a `myReady` flag records whether setup succeeded; the other
  functions check it.
* In Zephyr, `gpio_pin_set_dt(&spec, 1)` means **active**, not high. P02's `gpio::Interface` comments say
  "high/low" – the ESP32 and Zephyr versions differ for an active-low pin. Worth aligning.
* ADC: `adc_channel_setup_dt` configures the channel from devicetree (Zephyr's ESP32 driver also sets
  up the calibration). A read is an `adc_sequence` with a buffer – `buffer_size` is in **bytes**.
  `adc_raw_to_millivolts_dt` converts in place and needs an `int32_t`.
* Remove what the copied ESP32 code needed but Zephyr does not (`microseconds()`, `High`/`Low`,
  `<cstdio>`) – unused code gives warnings that hide real ones.

### Git

* Our `.gitignore` ignores files without an extension, which hit Zephyr's `Kconfig` and `*_defconfig`
  (now excepted). Build from a fresh clone before opening a PR to catch missing files.
* Stacked PRs: merge from the bottom up, use **"Create a merge commit"** (squash breaks the PRs on top),
  and retarget the next PR to `main` by hand (the repo does not delete merged branches).
* Don't change a file in a lower PR if a higher PR deletes it – it gives a modify/delete conflict.
  Reply in the review thread instead.
* Commit first, then `git merge origin/main` – your own work is safe in a commit, and
  `git merge --abort` can always undo the merge.

### C / Zephyr API

* A Zephyr app builds and runs even **without** a `main()`: `kernel/main_weak.c` has
  `int __weak main(void)` that does nothing. A weak symbol is used only if nobody else defines it.
* `#define X DT_ALIAS(led0);` is wrong – `#define` is plain text replacement, so the `;` ends up inside
  `GPIO_DT_SPEC_GET(...)`. A `#define` never ends with `;`.
* `GPIO_DT_SPEC_GET` is evaluated at **compile time**, so it can initialise a `static const` array of
  `struct gpio_dt_spec`. A missing alias is a compile error, not a runtime error.
* `&rgb[i]` is a pointer to **one** element (what `gpio_is_ready_dt` wants); `&rgb` is a pointer to the
  whole array – a different type.
* `ARRAY_SIZE(arr)` (from `zephyr/kernel.h`) gives the number of elements; `(i + 1) % ARRAY_SIZE(arr)`
  wraps an index around.
* `gpio_pin_set_dt(&spec, 1)` means "active" (on), not "high". The polarity flag in devicetree decides
  the electrical level – the RGB LED is active low, yet the code writes `1` to turn it on.
* In C, `int main(void)` means "no arguments"; `int main()` means "unspecified arguments".
* Always `printk` the reason (and the error code) before returning on an error – a silent `return` only
  leaves an LED that does not blink.

### Open questions

* Keep our own command parser or use Zephyr's shell?
* Why does the USB connection sometimes drop right after boot? (Not seen with blinky.)
* `factory::Zephyr`: option B (number → devicetree table) or C (roles in the interface)?
* **TMP36 on the ESP32 build:** the same sensor on the same pin (A6) reads ~21 °C on Zephyr, but the
  ESP-IDF build on the other board read ~12 °C and barely changed. 12 °C ⇔ 0.62 V vs 21 °C ⇔ 0.71 V.
  To check: run the ESP-IDF build on the board where Zephyr reads 21 °C (code or wiring?), look for
  "calibration unavailable" in the boot log, and measure Vout with a multimeter. The ESP32 ADC driver
  ignores all `adc_oneshot_*`/`adc_cali_*` return values, so errors are invisible.
* `logic.cpp` calls `sleep(sleepDuration_ms)` with `sleepDuration_ms = 1` – but `sleep()` takes
  **seconds**, so the loop sleeps 1 s when there is no input and the blink timer is only polled once a
  second.
