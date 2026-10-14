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
  * [The board is not in Zephyr](#the-board-is-not-in-zephyr)
  * [Zephyr environment](#zephyr-environment)
  * [Building and flashing with Zephyr](#building-and-flashing-with-zephyr)
  * [Zephyr troubleshooting](#zephyr-troubleshooting)

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
├── zephyr/                   # Zephyr port (work in progress)
│   └── boards/nano_esp32/    # Devicetree overlays adapting esp32s3_devkitc to the Nano ESP32
├── docs/                     # Additional documentation (git_commands.md, zephyr_port.md)
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

### Board facts (from the ABX00083 schematic)

| Item | Details |
|---|---|
| Module | u-blox NORA-W106 (ESP32-S3R8, 8 MB PSRAM) |
| Flash | External GD25B128, 128 Mbit = **16 MB** |
| USB | USB-C D+/D− go **directly** to GPIO19/20 (ESP32-S3 USB-Serial/JTAG) – there is no USB-UART chip |
| UART0 | GPIO43 (TX, D1) / GPIO44 (RX, D0) on the header |
| Yellow LED ("SCK LED", D13) | GPIO48 via a MOSFET – **active high** |
| RGB LED | Common anode to 3V3 on GPIO46, GPIO0 and GPIO45 – **active low** |
| Boot strapping pins | GPIO0 and GPIO46 – take care when using the RGB LED |

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
locally. A shallow clone (without history) is enough, place it at a relevant location for instance ~/esp (if you chose a different location you need to adjust some paths in commands, as described below):

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

Work is in progress to port the drivers to **Zephyr RTOS v4.4.2**. Thanks to the factory pattern,
`logic::Logic` does not need to change – instead, Zephyr implementations are added next to the ESP32-S3
implementations, together with a `factory::Zephyr`.

The plan, the division of work and our notes are in [`docs/zephyr_port.md`](docs/zephyr_port.md).

### The board is not in Zephyr

**The Arduino Nano ESP32 is not a supported board in Zephyr** (checked in v4.4.2 and on `main`). It uses
the same chip as Espressif's `esp32s3_devkitc`, so we build for that board and adapt it to the Nano with
devicetree overlays in [`zephyr/boards/nano_esp32/`](zephyr/boards/nano_esp32/):

| Overlay | Purpose |
|---|---|
| `usb_console.overlay` | Moves the console from `uart0` (D0/D1 pins) to the USB-C port (`usb_serial`) |

Board target used for all Zephyr builds:

```
esp32s3_devkitc/esp32s3/procpu
```

Known differences from the devkitc definition: the devkitc has no LED alias (`led0`) and assumes 8 MB flash
(the Nano has 16 MB). The long-term plan is an out-of-tree board definition `arduino_nano_esp32` in this
repository.

### Zephyr environment

The Zephyr workspace lives in `~/zephyrproject` (a separate west workspace, see
[`docs/zephyr_port.md`](docs/zephyr_port.md) for why Zephyr is not a git submodule). It needs the packages
from [step 2](#2-packages-in-ubuntu) above.

```bash
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate
pip install west ninja
west init -m https://github.com/zephyrproject-rtos/zephyr --mr v4.4.2 ~/zephyrproject
cd ~/zephyrproject
west update --narrow -o=--depth=1
west zephyr-export
west packages pip --install
cd zephyr
west sdk install -t xtensa-espressif_esp32s3_zephyr-elf   # Zephyr SDK with the ESP32-S3 compiler
west blobs fetch hal_espressif                           # Espressif binary blobs (Wi-Fi, BT, PHY)
```

Add an alias to activate the environment:

```bash
echo "alias get_zephyr='unset ZEPHYR_TOOLCHAIN_VARIANT; source ~/zephyrproject/.venv/bin/activate && export ZEPHYR_BASE=~/zephyrproject/zephyr && cd ~/zephyrproject/zephyr'" >> ~/.bashrc
```

> **Never mix environments in one terminal.** Use a fresh terminal for each of:
>
> | Environment | Activate with | Used for |
> |---|---|---|
> | ESP-IDF | `get_idf` | This project as it is today |
> | Zephyr (board) | `get_zephyr` | The Zephyr port, flashing the board |
> | Zephyr course | `source ~/zephyr/env.sh` | Course exercises on `native_sim` |
>
> The course's `env.sh` sets `ZEPHYR_TOOLCHAIN_VARIANT=host` (the PC's own GCC), which breaks builds for
> the ESP32-S3.

### Building and flashing with Zephyr

Until we have our own Zephyr application, Zephyr's samples are built with our overlay. Example with
`hello_world`:

```bash
get_zephyr
west build -p always -b esp32s3_devkitc/esp32s3/procpu samples/hello_world -- \
  -DEXTRA_DTC_OVERLAY_FILE=$HOME/yrgo_hwc-_p02/zephyr/boards/nano_esp32/usb_console.overlay \
  -DCONFIG_BOOT_DELAY=10000
```

* `-p always` – pristine build (start from scratch)
* `EXTRA_DTC_OVERLAY_FILE` – our board overlays; separate several with `;`
* `CONFIG_BOOT_DELAY=10000` – waits 10 s before booting, see below

Flash (attach the board with usbipd first, see [Flashing and monitoring](#flashing-and-monitoring)):

```bash
esptool --chip esp32s3 -p /dev/ttyACM0 erase-flash   # once, when switching from ESP-IDF
west flash --esp-device /dev/ttyACM0 && sleep 9 && python -m serial.tools.miniterm /dev/ttyACM0 115200
```

Exit the monitor with `Ctrl+]`. Expected output:

```
*** Booting Zephyr OS build v4.4.2 (delayed boot 10000ms) ***
Hello World! esp32s3_devkitc/esp32s3/procpu
```

**Why `BOOT_DELAY`?** The console runs over the ESP32-S3's built-in USB. Every reset disconnects USB, and
usbipd needs ~8 s to re-attach the board to WSL. Without the delay, everything printed at boot is lost
before the monitor can open the port.

Useful files after a build:

| File | Contents |
|---|---|
| `build/zephyr/zephyr.dts` | The final devicetree (board + overlays), with the source file of every line |
| `build/zephyr/.config` | All active Kconfig options |
| `west build -t menuconfig` | Interactive Kconfig browser (`/` to search, `?` for help) |

### Zephyr troubleshooting

| Error | Cause / fix |
|---|---|
| `No board named 'arduino_nano_esp32' found` | The board does not exist in Zephyr. Use `esp32s3_devkitc/esp32s3/procpu` + overlays. |
| `CMake was unable to find a build program corresponding to "Ninja"` | `pip install ninja` in the Zephyr venv, or `sudo apt install ninja-build`. |
| `west blobs fetch` fails with `504 Gateway Time-out` | Temporary GitHub error. Run `west blobs fetch hal_espressif` again. |
| Flashing works but the monitor shows nothing | Boot output was printed before the monitor connected. Build with `-DCONFIG_BOOT_DELAY=10000`. |
| `SerialException: device reports readiness to read but returned no data` | The board disconnected from USB (reset/re-attach). Start the monitor again. |
| Strange compiler errors when building for the ESP32-S3 | The course's `env.sh` is active in the terminal. Open a new terminal and run `get_zephyr`. |
