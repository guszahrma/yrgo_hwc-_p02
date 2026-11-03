# ESP-IDF environment

P02 on **ESP-IDF 6.0**: the original platform. The ESP-IDF drivers are in `main/` next to the interfaces
(`driver/<name>/esp32s3.h/.cpp`), and `main/source/main.cpp` runs `logic::Logic` with `factory::Esp32s3`.
Common setup (WSL, usbipd, cloning) is in [Setup](setup.md).

## Setup

### 1. Install ESP-IDF (once)

ESP-IDF is built from source together with the project, so it has to be available locally. A shallow clone
is enough. The example uses `~/esp/esp-idf` (Espressif's default); any other location works – use that path
instead below.

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

Optionally add an alias. Run this from the ESP-IDF directory (e.g. `~/esp/esp-idf`): `$(pwd)` is replaced
with its path when the line is written to `~/.bashrc`, so the alias works wherever ESP-IDF is installed:

```bash
echo "alias get_idf='. $(pwd)/export.sh'" >> ~/.bashrc
```

### 3. Create the Wi-Fi config (once)

The build needs `esp32s3_local_config.h`, even if you do not use Wi-Fi. From the repository root:

```bash
cp main/include/communication/wifi/esp32s3_local_config_template.h main/include/communication/wifi/esp32s3_local_config.h
```

If you want to use Wi-Fi, fill in the SSID and password in `esp32s3_local_config.h`. The file is ignored by
git and must **never** be committed.

### Alternative: ESP-IDF directly on Windows

Install ESP-IDF 6.0 with Espressif's installer and open the ESP-IDF PowerShell
(`C:\Espressif\tools\Microsoft.v6.0.PowerShell_profile.ps1`). If needed:
`Set-ExecutionPolicy RemoteSigned -Scope CurrentUser`. The commands below are the same, but the port is
called `COMx` instead of `/dev/ttyACM0`.

## Building and flashing

### P02 (`main/`)

From the **repository root** (attach the board first, see [Setup](setup.md#4-attach-the-board-to-wsl-powershell-as-administrator)):

```bash
. ~/esp/esp-idf/export.sh     # or use the get_idf alias
idf.py set-target esp32s3     # first time only / after fullclean
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Exit the monitor with `Ctrl+]` or `Ctrl+T`, `Ctrl+X`. If the monitor cannot find the port right after
flashing (the board is still being re-attached), wait a few seconds and run `idf.py -p /dev/ttyACM0 monitor`.

Then use the [serial commands](usage.md#serial-commands).

### Useful files and commands

| File / command | Contents |
|---|---|
| `idf.py menuconfig` | Interactive configuration (e.g. flash size), saved to `sdkconfig` |
| `sdkconfig` | The active configuration (generated, not committed) |
| `build/*.bin`, `build/*.elf`, `build/*.map` | Firmware, ELF with symbols, memory map |
| `idf.py fullclean` | Delete the build; run `set-target` again afterwards |

## Drivers

| Driver | ESP-IDF API | Configured from |
|---|---|---|
| `timer::Esp32s3` | `esp_timer_get_time()`, polled | – |
| `gpio::Esp32s3` | `driver/gpio.h` | Pin from `factory::Esp32s3` (Arduino pin names in `driver/pin/esp32s3.h`) |
| `adc::Esp32s3` (+ `Tmp36`) | `esp_adc/adc_oneshot.h` + calibration, average of 5 samples | Pin and `referenceVoltage` from `factory::Esp32s3` |
| `serial::Esp32s3` | `driver/usb_serial_jtag.h` | The board's USB port |
| `nvs::Esp32s3` | `nvs_flash.h` | Default NVS partition |
| `comm::wifi::Esp32s3` | `esp_wifi.h` | SSID and password from `esp32s3_local_config.h` |
| `comm::mqtt::publish::Esp32s3` | `mqtt_client.h` (`espressif/mqtt`) | Broker URL, topic, client ID and credentials in the constructor |
| `comm::mqtt::subscribe::Esp32s3` | `mqtt_client.h` (`espressif/mqtt`) | Broker URL, topic, client ID and credentials in the constructor |
