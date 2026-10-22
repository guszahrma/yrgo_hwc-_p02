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
| T1 | Zephyr itself | `~/zephyrproject` (what we use today) |
| T2 ("star") | The application | The course repository `~/zephyr` |
| T3 | A separate manifest repository | Large organisations with many applications |

**Today:** we build against the T1 workspace in `~/zephyrproject`.

**Planned:** make this repository a T2 workspace, like the course repository:

```
yrgo_hwc-_p02/
├── west.yml       ← Zephyr v4.4.2 + name-allowlist: [hal_espressif]
├── .west/         ← created locally by `west init -l .`, not committed
├── deps/          ← created by `west update`, in .gitignore
└── zephyr/        ← our Zephyr code
```

Then anyone (or CI) can get the exact same Zephyr with `west init -l . && west update`.

---

## Planned directory structure

```
zephyr/
├── boards/
│   └── nano_esp32/            ← adapts esp32s3_devkitc to the Nano ESP32, shared by all apps
│       ├── usb_console.overlay   (done)
│       └── leds.overlay          (done)
├── samples/                   ← our own learning exercises
│   └── blinky/                   (done)
│       ├── CMakeLists.txt        ← picks up the board overlays
│       ├── prj.conf
│       └── src/main.c
└── app/                       ← the P02 port itself
```

Later, `zephyr/boards/nano_esp32/` becomes a real out-of-tree board definition `arduino_nano_esp32`
(based on the schematic), and the overlays go away.

---

## Order of work

Each step maps to a lecture in the Zephyr course (`~/zephyr`): do the exercise on `native_sim` there
first, then apply it to P02 on the real board.

| # | Step | Course topic | Status |
|---|---|---|---|
| 0 | Environment, hello_world on the board | – | ✅ |
| 1 | Blinky on the yellow LED with our own overlay | Devicetree, GPIO | ✅ `zephyr/samples/blinky`: yellow LED + RGB colour cycle |
| 2 | Own Zephyr app skeleton: `Logic` builds and runs with **only stubs** | West, build, Kconfig | ⬜ |
| 3 | `timer::Zephyr` | Time, timers, work queues | ⬜ |
| 4 | `gpio::Zephyr`, then pins from devicetree (`gpio_dt_spec`) instead of `9U` | Devicetree, GPIO | ⬜ |
| 5 | `serial::Zephyr` – or replace our command parser with Zephyr's shell (design decision) | Shell | ⬜ |
| 6 | `adc::Zephyr` + overlay – TMP36 then works as-is | Devicetree, driver model | ⬜ |
| 7 | `nvs` via the Settings subsystem | Settings | ⬜ |
| 8 | Own board definition `arduino_nano_esp32` | Devicetree, bindings | ⬜ |
| 9 | Own `west.yml` (T2 workspace) + Zephyr build in CI | West manifest | ⬜ |
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

## Notes

### Board / hardware

* The Arduino Nano ESP32 is not in Zephyr. We use `esp32s3_devkitc/esp32s3/procpu` + overlays.
* The console must go over `usb_serial` (USB-Serial/JTAG) – the devkitc uses `uart0`.
* USB re-enumerates on every reset; with usbipd the board is gone for ~8 s. Use `CONFIG_BOOT_DELAY`.
* The devkitc definition says 8 MB flash, the Nano has 16 MB. The reason: the devkitc includes
  `esp32s3_wroom_n8.dtsi` – a WROOM module with 8 MB flash (`n8`).
* LEDs (verified on the board, defined in `zephyr/boards/nano_esp32/leds.overlay`):

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

### Kconfig

* `-DCONFIG_X=y` on the command line works, but belongs in `prj.conf` so everyone builds the same way.
* **No spaces around `=`**: `CONFIG_GPIO = y` is read as the symbol `GPIO ` (with a space), which does
  not exist, and the build fails.
* Check the result in the build's own `build/zephyr/.config` – every app/build has its own. Grepping
  another build directory shows that build's settings, not yours.
* `CONFIG_BOOT_DELAY` is in milliseconds: `1000` = 1 s, too short for usbipd (~8 s). We use `10000`.

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
