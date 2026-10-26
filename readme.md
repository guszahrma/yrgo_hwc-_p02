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
  * [Our board definition](#our-board-definition)
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
├── tools/clang-format/       # Pinned clang-format version and scripts
├── zephyr/                   # Zephyr port (work in progress)
│   ├── boards/arduino/nano_esp32/  # Our Zephyr board definition: arduino_nano_esp32
│   └── samples/blinky/             # First Zephyr app: yellow LED + RGB colour cycle
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

| Role | Arduino pin | ESP32-S3 GPIO | Notes |
|---|---|---|---|
| `GpioId::StatusLed` – status LED | D6 | GPIO9 | Output |
| `AdcId::TempSensor` – TMP36 | A6 | GPIO13 | ADC2, **does not work while Wi-Fi is active** |

`Logic` asks the driver factory for a **role**, not a pin, e.g.
`factory.createGpio(driver::factory::GpioId::StatusLed, ...)`. Each platform's factory decides which pin
a role is. To add or change hardware:

1. Define the role in `main/include/driver/factory/interface.h` (`GpioId` or `AdcId`).
2. Map the role to a pin in each platform's factory:
   - ESP32-S3: `main/source/driver/factory/esp32s3.cpp` (via the Arduino pin names in
     `driver/pin/esp32s3.h`)
   - Stub: `main/include/driver/factory/stub.h`
3. The compiler warns (`-Wswitch`) if a factory does not handle a new role.

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
| RGB LED | Common anode to 3V3 – **active low**: red = GPIO46, green = GPIO0, blue = GPIO45 (verified on the board) |
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
sudo apt update && sudo apt install -y --no-install-recommends git cmake ninja-build gperf ccache dfu-util device-tree-compiler wget flex bison python3-dev python3-pip python3-venv xz-utils file make gcc g++ gcc-multilib g++-multilib libffi-dev libssl-dev libusb-1.0-0 libsdl2-dev libmagic1 usbutils
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
is handy (if you chose a different location for esp you need to adjust the path below) : 

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

Always run from the **project root directory** (not inside `~/esp/esp-idf`) (in the example below we assume you have the repo in  ~/yrgo_hwc-_p02, this may need to be adapted to your folder structure):

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
Run usbipd list first without the esp connected and then with the esp connected to see which device is added and with which VID:PID it appeared. in the example below it appeared with VID:PID 6-4.
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

The code is formatted with `clang-format` according to `.clang-format`. Everyone, including CI, uses the
version in `tools/clang-format/version.txt`, since different versions format the code differently.

```bash
# Linux / WSL
tools/clang-format/check.sh # check
tools/clang-format/fix.sh   # format
```

On Windows, the equivalent scripts are `tools/clang-format/check.ps1` and `tools/clang-format/fix.ps1`.

The scripts install the pinned version into `.venv/` in the repo root the first time they run (and again if the
version changes), so no system-wide clang-format is needed. To run it on single files, use `tools/clang-format/run.sh`
(`tools/clang-format/run.ps1` on Windows) the same way as `clang-format`.

To make VS Code format with the same version, add this to `.vscode/settings.json` (run one of the scripts first so
that `.venv/` exists; on Windows the path is `${workspaceFolder}/.venv/Scripts/clang-format.exe`):

```json
"C_Cpp.clang_format_path": "${workspaceFolder}/.venv/bin/clang-format"
```

Commits that only reformat code are listed in `.git-blame-ignore-revs`, so `git blame` shows who last changed
the code rather than who reformatted it. GitHub uses the file automatically; locally, run this once per clone:

```bash
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

CI (`.github/workflows/ci.yml`) runs on every push and pull request:
* format check with clang-format
* build with ESP-IDF 6.0 on Ubuntu (Docker image `espressif/idf:release-v6.0`) and Windows

Git commands and SSH setup are described in [`docs/git_commands.md`](docs/git_commands.md).

---

## Zephyr port

This is work is in progress to port the drivers to **Zephyr RTOS v4.4.2**, Do not rely too much on this yet since its will evolve iteratively during the development phase. especially in the begining.
Thanks to the factory pattern,
`logic::Logic` does not need to change – instead, Zephyr implementations are added next to the ESP32-S3
implementations, together with a `factory::Zephyr`.

The plan, the division of work and our notes are in [`docs/zephyr_port.md`](docs/zephyr_port.md).

### Our board definition

**The Arduino Nano ESP32 is not a supported board in Zephyr** (checked in v4.4.2 and on `main`), so this
repository has its own out-of-tree board definition in
[`zephyr/boards/arduino/nano_esp32/`](zephyr/boards/arduino/nano_esp32/), based on the ABX00083 schematic.
Board target for all Zephyr builds:

```
arduino_nano_esp32/esp32s3/procpu
```

What the board provides:

| Feature | Details |
|---|---|
| Chip | ESP32-S3 with 16 MB flash and 8 MB PSRAM (`esp32s3_wroom_n16r8`) |
| Console and shell | USB-C port (`usb_serial`, the ESP32-S3 USB-Serial/JTAG) |
| LEDs | `led0` yellow, `led1` RGB green, `led2` RGB blue, `led3` RGB red |
| Pin header | `arduino_nano_header`: Arduino pin names D0-D13 and A0-A7 (A0-A7 = 14-21) |
| UART | `uart0` on D0 (RX) / D1 (TX), enabled – Arduino's `Serial1`, not the console |
| I2C | `i2c0` on A4 (SDA) / A5 (SCL), also named `arduino_nano_i2c` – disabled, enable in the app |
| SPI | `spi2` on D11 (COPI) / D12 (CIPO) / D13 (SCK), also named `arduino_nano_spi` – disabled |

Thanks to the pin header, app overlays describe wiring with Arduino pin names instead of ESP32-S3 GPIO
numbers:

```dts
gpios = <&arduino_nano_header 6 GPIO_ACTIVE_HIGH>;   /* D6 (= GPIO9) */
```

| File | Contents |
|---|---|
| `arduino_nano_esp32_procpu.dts` | The board: console, LEDs, aliases, peripherals |
| `arduino_nano_esp32-pinctrl.dtsi` | Which GPIOs UART, I2C and SPI use |
| `arduino_nano_connector.dtsi` | The Nano pin header: D0-D13, A0-A7 → GPIO |
| `board.yml`, `Kconfig*`, `board.cmake`, `*_defconfig`, `*.yaml` | Board name, chip variant, flash runner, default Kconfig, Twister metadata |

Zephyr only looks for boards in its own tree, so apps point it to ours with `BOARD_ROOT` (see below).

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
echo "alias get_zephyr='unset ZEPHYR_TOOLCHAIN_VARIANT; source ~/zephyrproject/.venv/bin/activate && export ZEPHYR_BASE=~/zephyrproject/zephyr'" >> ~/.bashrc
```

The alias does not change directory, so you stay where you are (e.g. in this repository). If you
added an older version of the alias, remove it first: `sed -i '/alias get_zephyr=/d' ~/.bashrc`.

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

#### Our own apps (`zephyr/samples/`)

Our apps find the board through `list(APPEND BOARD_ROOT ...)` in their own `CMakeLists.txt` and set
`CONFIG_BOOT_DELAY` in `prj.conf`, so no extra flags are needed. Build from the **app directory** (never
from the repository root – the root `build/` belongs to ESP-IDF):

```bash
get_zephyr
cd ~/yrgo_hwc-_p02/zephyr/samples/blinky
west build -p always -b arduino_nano_esp32/esp32s3/procpu .
west flash --esp-device /dev/ttyACM0 && sleep 9 && python -m serial.tools.miniterm /dev/ttyACM0 115200
```

Expected: the yellow LED blinks and the RGB LED cycles red → green → blue, with one line per second:

```
Yellow: OFF  RGB: green
Yellow: ON   RGB: blue
```

#### Zephyr's samples

Zephyr's own samples don't know about our board, so `BOARD_ROOT` is passed on the command line.
Example with `hello_world`:

```bash
get_zephyr
west build -p always -b arduino_nano_esp32/esp32s3/procpu $ZEPHYR_BASE/samples/hello_world -- \
  -DBOARD_ROOT=$HOME/yrgo_hwc-_p02/zephyr \
  -DCONFIG_BOOT_DELAY=10000
```

* `-p always` – pristine build (start from scratch)
* `BOARD_ROOT` – the directory that contains `boards/`, i.e. the repository's `zephyr/` directory
* `CONFIG_BOOT_DELAY=10000` – waits 10 s before booting, see below

Flash (attach the board with usbipd first, see [Flashing and monitoring](#flashing-and-monitoring)):

```bash
esptool --chip esp32s3 -p /dev/ttyACM0 erase-flash   # once, when switching from ESP-IDF
west flash --esp-device /dev/ttyACM0 && sleep 9 && python -m serial.tools.miniterm /dev/ttyACM0 115200
```

Exit the monitor with `Ctrl+]`. Expected output:

```
*** Booting Zephyr OS build v4.4.2 (delayed boot 10000ms) ***
Hello World! arduino_nano_esp32/esp32s3/procpu
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
| `No board named 'arduino_nano_esp32' found` | Zephyr cannot find our board. Our apps: `list(APPEND BOARD_ROOT ...)` must come before `find_package(Zephyr)` in `CMakeLists.txt`. Zephyr's samples: add `-- -DBOARD_ROOT=$HOME/yrgo_hwc-_p02/zephyr`. |
| `devicetree error: ... 'pinctrl-names' is marked as required` (after enabling `i2c0`/`spi2`) | Disabled nodes are not checked against their bindings – errors only show when a node is enabled. Test the board with each bus enabled. |
| `CMake was unable to find a build program corresponding to "Ninja"` | `pip install ninja` in the Zephyr venv, or `sudo apt install ninja-build`. |
| `west blobs fetch` fails with `504 Gateway Time-out` | Temporary GitHub error. Run `west blobs fetch hal_espressif` again. |
| Flashing works but the monitor shows nothing | Boot output was printed before the monitor connected. Build with `-DCONFIG_BOOT_DELAY=10000`. |
| `SerialException: device reports readiness to read but returned no data` | The board disconnected from USB (reset/re-attach). Start the monitor again. |
| Strange compiler errors when building for the ESP32-S3 | The course's `env.sh` is active in the terminal. Open a new terminal and run `get_zephyr`. |
