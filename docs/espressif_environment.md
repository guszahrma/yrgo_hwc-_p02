# ESP-IDF environment

P02 on **ESP-IDF 6.0**: the original platform. The ESP-IDF drivers are in `main/` next to the interfaces
(`driver/<name>/esp32s3.h/.cpp`), and `main/source/main.cpp` runs `logic::Logic` with `factory::Esp32s3`.
Common setup (WSL, usbipd, cloning) is in [Setup](setup.md).

## Setup

### 1. Install ESP-IDF (once)

ESP-IDF is built from source together with the project, so it has to be available locally. A shallow
clone is enough. The commands below use `~/esp/esp-idf`; if you choose another location, adjust the paths.

```bash
mkdir -p ~/esp
git clone -b v6.0 --depth 1 --recursive --shallow-submodules https://github.com/espressif/esp-idf.git ~/esp/esp-idf
cd ~/esp/esp-idf && ./install.sh esp32s3
```

`install.sh` downloads the compiler and creates the Python environment in `~/.espressif/`. Unlike Zephyr,
ESP-IDF lives outside the repository and its version is not pinned by a file in the repository.

### 2. Activate the environment (every new terminal)

```bash
. ~/esp/esp-idf/export.sh
```

Optionally add an alias:

```bash
echo "alias get_idf='. ~/esp/esp-idf/export.sh'" >> ~/.bashrc
```

Do not mix it with the Zephyr environment in the same terminal (see [Setup](setup.md#next-set-up-a-platform)).

### 3. Configure Wi-Fi (once)

The build needs `esp32s3_local_config.h`. From the repository root:

```bash
cp main/include/communication/wifi/esp32s3_local_config_template.h main/include/communication/wifi/esp32s3_local_config.h
```

Fill in the SSID and password in `esp32s3_local_config.h`. The file is ignored by git and must **never** be
committed.

### Alternative: ESP-IDF directly on Windows

Install ESP-IDF 6.0 with Espressif's installer and open the ESP-IDF PowerShell
(`C:\Espressif\tools\Microsoft.v6.0.PowerShell_profile.ps1`). If needed:
`Set-ExecutionPolicy RemoteSigned -Scope CurrentUser`. The commands below are the same, but the port is
called `COMx` instead of `/dev/ttyACM0`.

## Building and flashing

### P02 (`main/`)

From the **repository root** (attach the board first, see [Setup](setup.md#4-attach-the-board-to-wsl-powershell-as-administrator)):

```bash
. ~/esp/esp-idf/export.sh
idf.py set-target esp32s3     # first time only / after fullclean
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Exit the monitor with `Ctrl+]`. If the monitor cannot find the port right after flashing (the board is
still being re-attached), wait a few seconds and run `idf.py -p /dev/ttyACM0 monitor`.

Then use the [serial commands](usage.md#serial-commands).

### Useful files and commands

| File / command | Contents |
|---|---|
| `idf.py menuconfig` | Interactive configuration (e.g. flash size), saved to `sdkconfig` |
| `sdkconfig` | The active configuration (generated, not committed) |
| `build/*.bin`, `build/*.elf`, `build/*.map` | Firmware, ELF with symbols, memory map |
| `idf.py fullclean` | Delete the build; run `set-target` again afterwards |

## Troubleshooting

| Error | Cause / fix |
|---|---|
| `ESP-IDF Python virtual environment ... not found` | `install.sh` has not completed. Run `cd ~/esp/esp-idf && ./install.sh esp32s3`. |
| `libusb-1.0.so.0: cannot open shared object file` (during install) | `sudo apt install -y libusb-1.0-0`, then run `install.sh` again. |
| `Current directory '.../esp-idf' is not buildable` | You are in the ESP-IDF directory. `cd` to the repository root. Remove `~/esp/esp-idf/build`. |
| CMake uses `Unix Makefiles` instead of Ninja | `ninja-build` is missing: `sudo apt install -y ninja-build`. |
| `esp32s3_local_config.h: No such file or directory` | Copy the Wi-Fi template, see [Configure Wi-Fi](#3-configure-wi-fi-once). |
| `task_wdt: Task watchdog got triggered ... IDLE0` | A task never blocks, so the idle task cannot run. See *Sleeps shorter than one tick* below. |

## Design decisions and lessons learned

> Draft – to be reviewed and extended. Counterpart of
> [Zephyr environment → Design decisions and lessons learned](zephyr_environment.md#design-decisions-and-lessons-learned).

### Overview

| Driver | ESP-IDF API | Configured from |
|---|---|---|
| `timer::Esp32s3` | `esp_timer_get_time()`, polled | – |
| `gpio::Esp32s3` | `driver/gpio.h` | Pin from `factory::Esp32s3` (Arduino pin names in `driver/pin/esp32s3.h`) |
| `adc::Esp32s3` (+ `Tmp36`) | `esp_adc/adc_oneshot.h` + calibration, average of 5 samples | Pin and `referenceVoltage` from `factory::Esp32s3` |
| `serial::Esp32s3` | `driver/usb_serial_jtag.h` | The board's USB port |
| `nvs::Esp32s3` | `nvs_flash.h` | Default NVS partition |
| `comm::wifi`, `comm::mqtt` | `esp_wifi.h`, `espressif/mqtt` | `esp32s3_local_config.h` |

#### Known issues

* The ADC driver ignores the return values of `adc_oneshot_*`/`adc_cali_*`, so a failed read is counted
  into the average without any error.

### Design decisions

| Decision | Choice | Why |
|---|---|---|
| ADC attenuation | Chosen from `referenceVoltage` in the constructor | The caller says which voltage range it needs; the driver picks the attenuation |
| ADC noise | Average of `AdcSamples` (5) reads in `readValue()`; `readVoltage()` uses it | Single reads are noisy; the same on Zephyr |
| NVS initialisation | `nvs_flash_init()` in the NVS driver's constructor (erase and retry if the partition is full or from a newer version) | The driver works without the Wi-Fi driver having initialised flash first |
| Pins | Arduino names (`D6`, `A6`) mapped to GPIO numbers in `driver/pin/esp32s3.h`; `PhysicalPinManager` stops two drivers from claiming the same pin | Code and wiring use the names printed on the board |

### Pitfalls and lessons learned

* **Sleeps shorter than one tick.** FreeRTOS ticks every 10 ms (`CONFIG_FREERTOS_HZ=100`). A shorter
  sleep busy-waits in ESP-IDF, so the task never blocks, the idle task cannot run, and the task watchdog
  fires. `Logic` therefore sleeps 10 ms.
* **`sleep()` takes seconds**, not milliseconds – use `std::this_thread::sleep_for(std::chrono::milliseconds(...))`.
* **ADC2 (A4–A7) cannot be used while Wi-Fi is active.** Use A0–A3 (ADC1) for analog sensors if Wi-Fi is
  needed.
* The Nano ESP32 ships with the Arduino bootloader – the first flash needs B1 to GND (see
  [Setup](setup.md#5-first-time-get-the-board-out-of-arduino-mode)).
