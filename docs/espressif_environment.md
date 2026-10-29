# ESP-IDF environment

Setting up, building and flashing P02 with **ESP-IDF 6.0**. Common setup (WSL, usbipd, cloning) is in
[Setup](setup.md).

## Setup

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

### Configure Wi-Fi

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

### Building

Always run from the **project root directory** (not inside `~/esp/esp-idf`) (in the example below we assume you have the repo in  ~/yrgo_hwc-_p02, this may need to be adapted to your folder structure):

```bash
get_idf                       # once per terminal
cd ~/yrgo_hwc-_p02
idf.py set-target esp32s3     # first time only / after fullclean
idf.py build
```

Configuration (e.g. flash size) is changed with `idf.py menuconfig`.

### 3. Flash

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

Exit the monitor with `Ctrl+]`. If the monitor cannot find the port right after flashing (the board is
still being re-attached), wait a few seconds and run `idf.py -p /dev/ttyACM0 monitor`.

## Troubleshooting

| Error | Cause / fix |
|---|---|
| `ESP-IDF Python virtual environment ... not found` | `install.sh` has not completed. Run `cd ~/esp/esp-idf && ./install.sh esp32s3`. |
| `libusb-1.0.so.0: cannot open shared object file` (during install) | `sudo apt install -y libusb-1.0-0`, then run `install.sh` again. |
| `Current directory '.../esp-idf' is not buildable` | You are in the ESP-IDF directory. `cd ~/yrgo_hwc-_p02`. Remove `~/esp/esp-idf/build`. |
| CMake uses `Unix Makefiles` instead of Ninja | `ninja-build` is missing: `sudo apt install -y ninja-build`. |

## Design decisions and lessons learned

> **TODO:** to be written – the ESP-IDF counterpart of
> [Zephyr environment → Design decisions and lessons learned](zephyr_environment.md#design-decisions-and-lessons-learned).
> Suggested structure: overview (driver → ESP-IDF API), design decisions, pitfalls. Candidates from the
> project so far: ADC attenuation chosen from `referenceVoltage`; ADC2 cannot be used while Wi-Fi is on;
> `nvs_flash_init()` in the NVS driver; sleeps shorter than one FreeRTOS tick (10 ms) busy-wait and
> trigger the task watchdog.
