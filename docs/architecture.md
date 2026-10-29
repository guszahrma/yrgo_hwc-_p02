# Architecture

How the code is organised: interfaces, factories and drivers, the folder structure and the driver list.
For whoever wants to understand or change the code.

## Interfaces, factories and drivers

Each driver consists of an interface and one implementation per platform:

| Part | File | Purpose |
|---|---|---|
| Interface | `driver/<name>/interface.h` | Abstract class (`Interface`) that the application logic programs against |
| ESP-IDF implementation | `driver/<name>/esp32s3.h/.cpp` | Real driver using ESP-IDF |
| Zephyr implementation | `driver/<name>/zephyr.h/.cpp` | Real driver using Zephyr |
| Stub | `driver/<name>/stub.h` | Simulated driver for testing without hardware |

Drivers are created through a **factory** (`driver::factory::Interface`) with one implementation per platform
(`factory::Esp32s3`, `factory::Zephyr`, `factory::Stub`). The application logic (`logic::Logic`) only knows
about the interfaces, so the same logic runs on ESP-IDF, on Zephyr or on stubs just by swapping the factory.

```
main/source/main.cpp    ──► logic::Logic ──► factory::Interface ──┬──► factory::Esp32s3 ──► driver::*::Esp32s3 ──► ESP-IDF
zephyr/app/src/main.cpp ──►                                       ├──► factory::Zephyr  ──► driver::*::Zephyr  ──► Zephyr
(tests)                                                           └──► factory::Stub    ──► driver::*::Stub
```

Each build system lists its own source files (`main/CMakeLists.txt` for ESP-IDF, `zephyr/app/CMakeLists.txt`
for Zephyr), so a platform never compiles the other platform's drivers.

Pins on ESP-IDF are handled by `driver::pin`:
* `driver/pin/esp32s3.h` maps Arduino names (`A0`–`A7`, `D0`–`D13`) to ESP32-S3 GPIO numbers.
* `driver/pin/manager.h` (`PhysicalPinManager`) keeps track of which physical pins are in use, so that two
  drivers cannot claim the same pin.

On Zephyr, pins come from devicetree instead (see [Zephyr environment](zephyr_environment.md#drivers)).

## Adding or changing hardware

`Logic` asks the driver factory for a **role**, not a pin, e.g.
`factory.createGpio(driver::factory::GpioId::StatusLed, ...)`. Each platform's factory decides what a role
is. To add or change hardware:

1. Define the role in `main/include/driver/factory/interface.h` (`GpioId` or `AdcId`).
2. Map the role in each platform's factory:
   - ESP-IDF: `main/source/driver/factory/esp32s3.cpp` (via the Arduino pin names in
     `driver/pin/esp32s3.h`)
   - Zephyr: `main/source/driver/factory/zephyr.cpp`, and describe the wiring in `zephyr/app/app.overlay`
     (an alias for a GPIO, `io-channels` in `zephyr,user` for an ADC channel)
   - Stub: `main/include/driver/factory/stub.h`
3. The compiler warns (`-Wswitch`) if a factory does not handle a new role.

The current wiring is listed in [Usage](usage.md#hardware-and-wiring).

## Writing a driver

* Add `zephyr.h/.cpp` or `esp32s3.h/.cpp` next to the interface, and add the `.cpp` to the platform's
  `CMakeLists.txt` – a missing one is a **linker** error (`undefined reference to ...`).
* Mark overrides with `override`: it catches signatures that do not match the interface.
* A `noexcept` constructor cannot report failure – a `myReady`/`myValid` flag records it and the other
  functions check it.
* Update the driver's state (`myState`) only after the hardware call succeeded.
* Do not keep code the platform does not need (e.g. when copying another platform's driver) – unused code
  gives warnings that hide real ones.

## Directory structure

```
.
├── CMakeLists.txt            # ESP-IDF project file
├── dependencies.lock         # Locked ESP-IDF component versions (espressif/mqtt)
├── west.yml                  # Zephyr manifest: pins the Zephyr version
├── main/
│   ├── CMakeLists.txt        # ESP-IDF source file list + components
│   ├── idf_component.yml     # External ESP-IDF components (espressif/mqtt)
│   ├── include/
│   │   ├── arch/env/         # Platform selection (real hardware / test environment)
│   │   ├── communication/    # wifi, mqtt (publish/subscribe) – ESP-IDF only
│   │   ├── driver/           # adc, factory, gpio, nvs, pin, serial, tempsensor, timer
│   │   ├── logic/            # Application logic (command parser)
│   │   └── ml/               # Linear regression
│   └── source/               # Implementations, same layout as include/
├── zephyr/
│   ├── app/                        # P02 on Zephyr: logic::Logic with factory::Zephyr
│   ├── boards/arduino/nano_esp32/  # Our Zephyr board definition: arduino_nano_esp32
│   └── samples/blinky/             # First Zephyr app: yellow LED + RGB colour cycle
├── tests/                    # Unit tests on the host (GoogleTest, mocked hardware)
│   └── googletest/           # GoogleTest (git submodule)
├── tools/                    # Zephyr setup (zephyr_setup.sh, zephyr_env.sh)
│   └── clang-format/         # Pinned clang-format version and scripts
├── docs/                     # Documentation (see the readme)
├── .git-blame-ignore-revs    # Formatting-only commits hidden from git blame
└── .github/workflows/ci.yml  # CI: formatting, ESP-IDF and Zephyr builds, unit tests
```

Generated and not committed: `sdkconfig` and `build/` (ESP-IDF), `.west/`, `deps/` and `zephyr/*/build/`
(Zephyr), `.venv/` (Python tools for Zephyr and clang-format).

## Drivers

| Module | Namespace | Description | ESP-IDF | Zephyr |
|---|---|---|---|---|
| GPIO | `driver::gpio` | Output, input with pull-up/pull-down, `toggle()` | ✅ | ✅ |
| ADC | `driver::adc` | Raw value and voltage, average of 5 samples | ✅ | ✅ |
| Serial | `driver::serial` | Over the board's USB port, `readLine()` is non-blocking | ✅ | ✅ |
| Timer | `driver::timer` | Software timer, polled with `timeout()` | ✅ | ✅ |
| NVS | `driver::nvs` | Strings + integers (template), keys max 15 characters | ✅ | ✅ |
| Temp sensor | `driver::tempsensor` | `Tmp36` uses an `adc::Interface`, platform independent | ✅ | ✅ |
| Wi-Fi | `comm::wifi` | Scans for and connects to the configured network | ✅ | – |
| MQTT | `comm::mqtt::publish/subscribe` | Publish and subscribe to topics | ✅ | – |
| ML | `ml::lin_reg` | Adaptive linear regression, platform independent | ✅ | – |

Which platform API each driver uses is listed in [ESP-IDF environment](espressif_environment.md#drivers)
and [Zephyr environment](zephyr_environment.md#drivers).

## Known issues

* `Logic`'s destructor resets the ADC before the temperature sensor that refers to it. Harmless today
  (`run()` never returns), but wrong.
* `gpio::Interface` says "high/low"; Zephyr's `gpio_pin_set_dt` means **active/inactive**. The two
  platforms differ for an active-low pin.
* ESP-IDF: the ADC driver ignores the return values of `adc_oneshot_*`/`adc_cali_*`, so a failed read is
  counted into the average without any error.
* Zephyr: Wi-Fi and MQTT are not ported (the application does not use them on ESP-IDF either). ADC2 cannot
  be used while Wi-Fi is on, so the TMP36 must move from A6 to A0–A3 first.
