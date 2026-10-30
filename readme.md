# P02 – Drivers for the Arduino Nano ESP32

C++17 drivers and a small command-line application for the Arduino Nano ESP32 (ESP32-S3), built on
**ESP-IDF 6.0** and ported to **Zephyr RTOS 4.4.2**. The same application code runs on both platforms.

Course project in:
- *Hårdvarunära programmering C/C++* (low-level programming in C/C++)
- *Mjuk- och hårdvarutestning* (software and hardware testing)
- *Maskininlärning* (machine learning)

## Features

- Drivers for GPIO, ADC, timer, serial (USB) and non-volatile storage, behind platform-independent
  interfaces
- TMP36 temperature sensor; Wi-Fi and MQTT (ESP-IDF only)
- Serial commands: LED on/off, blinking, temperature, settings saved across resets

Development: unit tests on the development PC (the *host*, as opposed to the board) with mocked hardware,
and CI (formatting, ESP-IDF and Zephyr builds, unit tests) on every pull request.

## Architecture

```
main ──► logic::Logic ──► factory::Interface ──┬──► factory::Esp32s3 ──► drivers ──► ESP-IDF
                                               ├──► factory::Zephyr  ──► drivers ──► Zephyr
                                               └──► factory::Stub    ──► simulated drivers
```

The application asks the factory for hardware by **role** (`GpioId::StatusLed`, `AdcId::TempSensor`), and
each platform's factory decides what the role means on the board: a pin on ESP-IDF (e.g. `StatusLed` = D6),
a devicetree node on Zephyr. See [Architecture](docs/architecture.md).

## Quick start

We assume Ubuntu 24.04 (WSL2 on Windows) – see [Setup](docs/setup.md) for the first-time setup. All
commands run from the repository root.

```bash
git clone --recursive git@github.com:guszahrma/yrgo_hwc-_p02.git
cd yrgo_hwc-_p02
```

**Attach the board to WSL** – again after every restart of the computer or WSL (PowerShell as
administrator; the first time, see [Setup](docs/setup.md#4-attach-the-board-to-wsl-powershell-as-administrator)):

```powershell
usbipd attach --wsl --busid <BUSID> --auto-attach   # find the BUSID with: usbipd list
```

**ESP-IDF** – requires ESP-IDF 6.0, see [ESP-IDF environment](docs/espressif_environment.md):

```bash
. ~/esp/esp-idf/export.sh                                       # or the get_idf alias
cp main/include/communication/wifi/esp32s3_local_config_template.h \
   main/include/communication/wifi/esp32s3_local_config.h      # first time only (needed to build)
idf.py set-target esp32s3                                       # first time only
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

**Zephyr** – the setup script fetches Zephyr, the toolchain and the Python tools
([Zephyr environment](docs/zephyr_environment.md)):

```bash
tools/zephyr_setup.sh            # first time only (~800 MB)
source tools/zephyr_env.sh       # or the get_zephyr alias
west build -p always -b arduino_nano_esp32/esp32s3/procpu -s zephyr/app -d zephyr/app/build
west flash -d zephyr/app/build --esp-device /dev/ttyACM0 && python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
```

Then type `help` in the monitor ([serial commands](docs/usage.md#serial-commands)). Exit the monitor with
`Ctrl+]` or `Ctrl+T`, `Ctrl+X` (ESP-IDF), or `Ctrl+X` (Zephyr).

## Hardware

| Role | Pin | Part |
|---|---|---|
| Status LED | D6 | LED + resistor to GND |
| Temperature sensor | A6 | TMP36: Vout to A6, V+ to 3V3 or VBUS (2.7–5.5 V), 0.1 µF capacitor between V+ and GND |

## Documentation

| Document | Contents |
|---|---|
| [Setup](docs/setup.md) | WSL, usbipd, packages, cloning, first flash of a new board |
| [ESP-IDF environment](docs/espressif_environment.md) | Installing ESP-IDF, building and flashing, drivers |
| [Zephyr environment](docs/zephyr_environment.md) | Installing Zephyr, building and flashing, samples, drivers |
| [Usage](docs/usage.md) | Serial commands, wiring, switching between ESP-IDF and Zephyr |
| [Architecture](docs/architecture.md) | Interfaces, factories, adding hardware or drivers, folder structure |
| [Board](docs/board.md) | Arduino Nano ESP32 facts and our Zephyr board definition |
| [Contributing](docs/contributing.md) | Workflow, tests, code style, CI |

Troubleshooting tables, design decisions and lessons learned are collected in [`AGENTS.md`](AGENTS.md),
mainly written as context for AI coding assistants.
