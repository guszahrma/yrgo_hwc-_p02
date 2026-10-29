# Architecture

How the code is organised: interfaces, factories and drivers, the folder structure and the driver
list. For whoever wants to understand or change the code.

## Architecture

Each driver consists of three parts:

| Part | File | Purpose |
|---|---|---|
| Interface | `driver/<name>/interface.h` | Abstract class (`Interface`) that the application logic programs against |
| Implementation | `driver/<name>/esp32s3.h/.cpp` | Real driver using ESP-IDF |
| Stub | `driver/<name>/stub.h` | Simulated driver for testing without hardware |

Drivers are created through a **factory** (`driver::factory::Interface`) with one implementation per platform
(`factory::Esp32s3`, `factory::Stub`). The application logic (`logic::Logic`) only knows about the interfaces,
so the same logic can run on real hardware, on stubs or on another platform (e.g. Zephyr) just by swapping
the factory.

```
main.cpp ──► logic::Logic ──► factory::Interface ──┬──► factory::Esp32s3 ──► driver::*::Esp32s3 ──► ESP-IDF
                                                   └──► factory::Stub    ──► driver::*::Stub
```

Pins are handled by `driver::pin`:
* `driver/pin/esp32s3.h` maps Arduino names (`A0`–`A7`, `D0`–`D13`) to ESP32-S3 GPIO numbers.
* `driver/pin/manager.h` (`PhysicalPinManager`) keeps track of which physical pins are in use, so that two
  drivers cannot claim the same pin.

## Directory structure

```
.
├── CMakeLists.txt            # ESP-IDF project file
├── dependencies.lock         # Locked versions (ESP-IDF 6.0, espressif/mqtt)
├── sdkconfig                 # ESP-IDF configuration (idf.py menuconfig)
├── main/
│   ├── CMakeLists.txt        # Source file list + ESP-IDF components
│   ├── idf_component.yml     # External components (espressif/mqtt)
│   ├── include/
│   │   ├── arch/env/         # Platform selection (real hardware / test environment)
│   │   ├── communication/    # wifi, mqtt (publish/subscribe)
│   │   ├── driver/           # adc, factory, gpio, nvs, pin, serial, tempsensor, timer
│   │   ├── logic/            # Application logic (command parser)
│   │   └── ml/               # Linear regression
│   └── source/               # Implementations, same layout as include/
├── tests/                    # Test suite run on the PC (g++) with mocked hardware
├── tools/                    # Zephyr setup (zephyr_setup.sh, zephyr_env.sh)
│   └── clang-format/         # Pinned clang-format version and scripts
├── west.yml                  # Zephyr manifest: pins the Zephyr version for the port
├── zephyr/                   # Zephyr port
│   ├── app/                        # P02 on Zephyr: logic::Logic with factory::Zephyr
│   ├── boards/arduino/nano_esp32/  # Our Zephyr board definition: arduino_nano_esp32
│   └── samples/blinky/             # First Zephyr app: yellow LED + RGB colour cycle
├── docs/                     # Additional documentation (git_commands.md, zephyr_design.md)
└── .github/workflows/ci.yml  # CI: format check + build on Linux and Windows
```

## Drivers

| Module | Namespace | ESP-IDF API | Notes |
|---|---|---|---|
| GPIO | `driver::gpio` | `driver/gpio.h` | Output, input with pull-up/pull-down, `toggle()` |
| ADC | `driver::adc` | `esp_adc/adc_oneshot.h` + calibration | Attenuation is chosen automatically from `referenceVoltage` |
| Serial | `driver::serial` | `driver/usb_serial_jtag.h` | Over the board's USB port, `readLine()` is non-blocking |
| Timer | `driver::timer` | `esp_timer.h` | Software timer, polled with `timeout()` |
| NVS | `driver::nvs` | `nvs_flash.h` | Strings + integers (template), keys max 15 characters |
| Temp sensor | `driver::tempsensor` | – | `Tmp36` uses an `adc::Interface`, platform independent |
| Wi-Fi | `comm::wifi` | `esp_wifi.h` | Scans for and connects to the configured network |
| MQTT | `comm::mqtt::publish/subscribe` | `espressif/mqtt` | Publish and subscribe to topics |
| ML | `ml::lin_reg` | – | Adaptive linear regression |
