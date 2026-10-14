# P02 - Drivers for ESP32-S3

* C++17 driver library for the `ESP32-S3` microcontroller, built on **ESP-IDF 6.0**
* Target board: **Arduino Nano ESP32** (ESP32-S3, 16 MB flash)
* Part of the course *Hårdvarunära programmering C/C++* (low-level programming in C/C++)
* In progress: port to **Zephyr RTOS** (see [Zephyr port](#zephyr-port))

---

## Contents

* [Architecture](#architecture)
* [Directory structure](#directory-structure)
* [Drivers](#drivers)
* [Hardware and wiring](#hardware-and-wiring)
* [Serial commands](#serial-commands)
* [Development environment (WSL + ESP-IDF)](#development-environment-wsl--esp-idf)
* [Building](#building)
* [Flashing and monitoring](#flashing-and-monitoring)
* [Troubleshooting](#troubleshooting)
* [Tests](#tests)
* [Code style and CI](#code-style-and-ci)
* [Zephyr port](#zephyr-port)

---

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

---

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
├── tools/                    # clang-format scripts
├── docs/                     # Additional documentation (e.g. git_commands.md)
└── .github/workflows/ci.yml  # CI: format check + build on Linux and Windows
```

---

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

---

## Hardware and wiring

| Function | Arduino pin | ESP32-S3 GPIO | Notes |
|---|---|---|---|
| LED | D6 | GPIO9 | `create_gpio(9U, ...)` in `logic.cpp` |
| TMP36 | A6 | GPIO13 | ADC2, **does not work while Wi-Fi is active** |

> **Note:** ADC2 (A4–A7) cannot be used while Wi-Fi is active. Use A0–A3 (ADC1) for analog sensors if
> Wi-Fi is needed.

The full pin mapping is in [`main/include/driver/pin/esp32s3.h`](main/include/driver/pin/esp32s3.h).

---

## Serial commands

Connect with `idf.py -p /dev/ttyACM0 monitor` and type:

| Command | Action |
|---|---|
| `help` | List commands |
| `on` / `off` | Turn the LED on / off |
| `blink on` / `blink off` | Start / stop blinking |
| `period <ms>` | Set the blink period, e.g. `period 500` |
| `status` | Show blink state, period and temperature |
| `store` | Save blink state and period to NVS (loaded at startup) |

---

## Development environment (WSL + ESP-IDF)

The recommended environment is **Ubuntu 24.04 in WSL2** with ESP-IDF 6.0. The USB board is passed from
Windows to WSL with **usbipd-win**.

### 1. WSL and usbipd (Windows, PowerShell as administrator)

```powershell
wsl --install -d Ubuntu-24.04
winget install --exact dorssel.usbipd-win
```

Restart the computer and open Ubuntu from the Start menu.

### 2. Packages in Ubuntu

```bash
sudo apt update && sudo apt install -y --no-install-recommends git cmake ninja-build gperf ccache dfu-util device-tree-compiler wget flex bison python3-dev python3-pip python3-venv xz-utils file make gcc g++ gcc-multilib g++-multilib libffi-dev libssl-dev libusb-1.0-0 libsdl2-dev libmagic1 usbutils clang-format
```

Give your user access to serial ports (takes effect after running `wsl --shutdown` in PowerShell):

```bash
sudo usermod -aG dialout $USER
```

> Keep all projects in the Linux home directory (`~/...`), **not** under `/mnt/c/...` – builds across the
> Windows file system are many times slower.

### 3. ESP-IDF 6.0

ESP-IDF is a framework that is built from source together with the project, so it has to be available
locally. A shallow clone (without history) is enough:

```bash
mkdir -p ~/esp
git clone -b v6.0 --depth 1 --recursive --shallow-submodules https://github.com/espressif/esp-idf.git ~/esp/esp-idf
cd ~/esp/esp-idf && ./install.sh esp32s3
```

`install.sh` downloads the compiler and creates the Python environment in `~/.espressif/`. Adding an alias
is handy:

```bash
echo "alias get_idf='. ~/esp/esp-idf/export.sh'" >> ~/.bashrc
```

### 4. Clone the repo and configure Wi-Fi

```bash
cd ~
git clone --recursive git@github.com:guszahrma/yrgo_hwc-_p02.git
cd yrgo_hwc-_p02
cp main/include/communication/wifi/esp32s3_local_config_template.h main/include/communication/wifi/esp32s3_local_config.h
```

Fill in the SSID and password in `esp32s3_local_config.h`. The file is ignored by git and must **never** be
committed.

### Alternative: ESP-IDF directly on Windows

Install ESP-IDF 6.0 with Espressif's installer and open the ESP-IDF PowerShell
(`C:\Espressif\tools\Microsoft.v6.0.PowerShell_profile.ps1`). If needed:
`Set-ExecutionPolicy RemoteSigned -Scope CurrentUser`. The commands below are the same, but the port is
called `COMx` instead of `/dev/ttyACM0`.

---

## Building

Always run from the **project root directory** (not inside `~/esp/esp-idf`):

```bash
get_idf                       # once per terminal
cd ~/yrgo_hwc-_p02
idf.py set-target esp32s3     # first time only / after fullclean
idf.py build
```

Configuration (e.g. flash size) is changed with `idf.py menuconfig`.

---

## Flashing and monitoring

### 1. Attach the board to WSL (PowerShell as administrator)

```powershell
usbipd list                                    # find the board's BUSID, e.g. 6-4
usbipd bind --busid 6-4                        # once per device and port
usbipd attach --wsl --busid 6-4 --auto-attach  # keep this window open
```

`--auto-attach` is needed because the ESP32-S3 disconnects and reconnects USB on every reset/flash.

Check in WSL:

```bash
lsusb | grep -iE "303a|2341"; ls /dev/ttyACM*
```

| USB ID | Meaning |
|---|---|
| `2341:0070` | Arduino factory firmware – esptool does **not** work, see below |
| `303a:1001` | ESP32-S3 USB-Serial/JTAG – ready to flash |

### 2. First time: get the board out of Arduino mode

The Nano ESP32 ships with the Arduino bootloader. Before the first flash:

1. Connect pin **B1 to GND**.
2. Press **reset** – the board starts in the ROM bootloader and shows up as `303a:1001`.
3. Run `usbipd list` again; if the device is not shared, run `usbipd bind` + `attach` again.
4. Flash, remove the wire and press reset.

After this, esptool can put the board into flash mode by itself.

> To restore the Arduino bootloader: B1 to GND + reset, then run *Burn Bootloader* in the Arduino IDE.

### 3. Flash

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

Exit the monitor with `Ctrl+]`. If the monitor cannot find the port right after flashing (the board is
still being re-attached), wait a few seconds and run `idf.py -p /dev/ttyACM0 monitor`.

### Switching between ESP-IDF and Zephyr

The platforms use different partition layouts. Erase the whole flash when switching:

```bash
idf.py -p /dev/ttyACM0 erase-flash
```

---

## Troubleshooting

| Error | Cause / fix |
|---|---|
| `ESP-IDF Python virtual environment ... not found` | `install.sh` has not completed. Run `cd ~/esp/esp-idf && ./install.sh esp32s3`. |
| `libusb-1.0.so.0: cannot open shared object file` (during install) | `sudo apt install -y libusb-1.0-0`, then run `install.sh` again. |
| `Current directory '.../esp-idf' is not buildable` | You are in the ESP-IDF directory. `cd ~/yrgo_hwc-_p02`. Remove `~/esp/esp-idf/build`. |
| CMake uses `Unix Makefiles` instead of Ninja | `ninja-build` is missing: `sudo apt install -y ninja-build`. |
| `Could not open COM3` | Windows port name used in WSL. Use `/dev/ttyACM0`. |
| `Path '/dev/ttyACM0' is not readable` | The user is not in the `dialout` group. See step 2 above, then run `wsl --shutdown` in PowerShell. |
| `usbipd: error: Device is not shared` | Run `usbipd bind --busid <id>` as administrator first. |
| No `/dev/ttyACM*` in WSL | Check that `usbipd attach` is running and that the board is not in Arduino mode (`2341:0070`). |

---

## Tests

The test suite in `tests/` is built with plain `g++` and runs the drivers against mocked hardware
(`TESTSUITE` is defined, which selects `arch/env/test/hw_platform.h`). Do **not** run it in a terminal where
`export.sh` is active:

```bash
cmake -S tests -B tests/build
cmake --build tests/build
```

The tests run automatically as part of the build (target `run`).

---

## Code style and CI

The code is formatted with `clang-format` according to `.clang-format`.

```bash
# Linux / WSL
clang-format --dry-run --Werror --style=file $(git ls-files '*.c' '*.cpp' '*.h')   # check
clang-format -i --style=file $(git ls-files '*.c' '*.cpp' '*.h')                    # format
```

On Windows, the equivalent scripts are `tools/clang-format-check.ps1` and `tools/clang-format-fix.ps1`.

CI (`.github/workflows/ci.yml`) runs on every push and pull request:
* format check with clang-format
* build with ESP-IDF 6.0 on Ubuntu (Docker image `espressif/idf:release-v6.0`) and Windows

Git commands and SSH setup are described in [`docs/git_commands.md`](docs/git_commands.md).

---

## Zephyr port

Work is in progress to port the drivers to Zephyr RTOS (board: `arduino_nano_esp32/esp32s3/procpu`).
Thanks to the factory pattern, `logic::Logic` does not need to change – instead, Zephyr implementations are
added next to the ESP32-S3 implementations, together with a `factory::Zephyr`.

Planned order:

1. Timer, GPIO and Serial on Zephyr – stubs for ADC, NVS and temp sensor
2. ADC (devicetree overlay) – TMP36 then works as-is
3. NVS via Zephyr's Settings subsystem
4. Wi-Fi and MQTT

Instructions for the Zephyr environment will be added here once the port is under way.
