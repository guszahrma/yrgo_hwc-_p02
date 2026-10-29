# P02 – Drivers for the Arduino Nano ESP32

C++17 drivers and a small command-line application for the Arduino Nano ESP32 (ESP32-S3), built on
**ESP-IDF 6.0** and ported to **Zephyr RTOS 4.4.2**. The same application code runs on both platforms.

Course project in *Hårdvarunära programmering C/C++* (low-level programming in C/C++).

## Features

- Drivers for GPIO, ADC, timer, serial (USB) and non-volatile storage, behind platform-independent
  interfaces
- TMP36 temperature sensor; Wi-Fi and MQTT (ESP-IDF only)
- Serial commands: LED on/off, blinking, temperature, settings saved across resets
- Unit tests on the host with mocked hardware
- CI: formatting, ESP-IDF build (Linux and Windows) and Zephyr build on every pull request

## Architecture

```
main ──► logic::Logic ──► factory::Interface ──┬──► factory::Esp32s3 ──► drivers ──► ESP-IDF
                                               ├──► factory::Zephyr  ──► drivers ──► Zephyr
                                               └──► factory::Stub    ──► simulated drivers
```

The application asks the factory for hardware by **role** (`GpioId::StatusLed`, `AdcId::TempSensor`), and
each platform's factory decides what the role means. See [Architecture](docs/architecture.md).

## Quick start

Requires Ubuntu 24.04 (WSL2 on Windows) with the board attached through usbipd – see
[Setup](docs/setup.md).

```bash
git clone --recursive git@github.com:guszahrma/yrgo_hwc-_p02.git
cd yrgo_hwc-_p02
```

**ESP-IDF** – requires ESP-IDF 6.0 in `~/esp/esp-idf` ([ESP-IDF environment](docs/espressif_environment.md)):

```bash
. ~/esp/esp-idf/export.sh
cp main/include/communication/wifi/esp32s3_local_config_template.h \
   main/include/communication/wifi/esp32s3_local_config.h      # first time only, then add your Wi-Fi
idf.py set-target esp32s3                                       # first time only
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

**Zephyr** – the setup script fetches Zephyr, the toolchain and the Python tools
([Zephyr environment](docs/zephyr_environment.md)):

```bash
tools/zephyr_setup.sh            # first time only (~800 MB)
source tools/zephyr_env.sh
cd zephyr/app
west build -p always -b arduino_nano_esp32/esp32s3/procpu .
west flash --esp-device /dev/ttyACM0 && python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
```

Then type `help` in the monitor.

## Hardware

| Role | Pin | Part |
|---|---|---|
| Status LED | D6 | LED + resistor to GND |
| Temperature sensor | A6 | TMP36 (Vout) |

## Documentation

| Document | Contents |
|---|---|
| [Setup](docs/setup.md) | WSL, usbipd, packages, cloning, first flash of a new board |
| [ESP-IDF environment](docs/espressif_environment.md) | Installing ESP-IDF, building and flashing, troubleshooting |
| [Zephyr environment](docs/zephyr_environment.md) | Zephyr workspace, building and flashing, troubleshooting, design decisions |
| [Usage](docs/usage.md) | Serial commands, wiring, switching between ESP-IDF and Zephyr |
| [Architecture](docs/architecture.md) | Interfaces, factories, folder structure, drivers |
| [Board](docs/board.md) | Arduino Nano ESP32 facts and our Zephyr board definition |
| [Contributing](docs/contributing.md) | Tests, code style, CI |
| [Git commands](docs/git_commands.md) | Git and SSH reference |
