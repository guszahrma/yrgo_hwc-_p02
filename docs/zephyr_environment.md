# Zephyr environment

P02 on **Zephyr RTOS v4.4.2**: a port of the ESP-IDF build. The Zephyr drivers are in `main/` next to the
ESP-IDF ones (`driver/<name>/zephyr.h/.cpp`), and `zephyr/app/src/main.cpp` runs the unchanged
`logic::Logic` with `factory::Zephyr`. Wi-Fi and MQTT are not ported yet. Common setup (WSL, usbipd,
cloning) is in [Setup](setup.md).

## Setup

### 1. Install Zephyr (once)

The Zephyr version is pinned in [`west.yml`](../west.yml) (the west manifest), and the repository itself is
the west workspace: Zephyr and the modules it needs are fetched into `deps/` inside the repository
(ignored by git).

From the **repository root** (~800 MB, a few minutes):

```bash
tools/zephyr_setup.sh
```

The script creates `.venv/` (west and Zephyr's Python tools), `.west/config` and `deps/` (Zephyr and
`hal_espressif`), installs the Zephyr SDK with the ESP32-S3 compiler in `~/zephyr-sdk-<version>` if it is
not there yet, and fetches Espressif's binary blobs. Run it again after `west.yml` has changed (e.g. a new
Zephyr version) – it only updates what is needed.

### 2. Activate the environment (every new terminal)

```bash
source tools/zephyr_env.sh
```

It activates `.venv`, sets `ZEPHYR_BASE` to `deps/zephyr` and exports `P02_ROOT` (the repository root),
which commands and scripts can use instead of a hard-coded path.

Optionally add an alias. Run this from the repository root: `$(pwd)` is replaced with the repository's
path when the line is written to `~/.bashrc`, so the alias works from any directory:

```bash
echo "alias get_zephyr='source $(pwd)/tools/zephyr_env.sh'" >> ~/.bashrc
```

Do not mix it with the ESP-IDF environment in the same terminal (see [Setup](setup.md#next-set-up-a-platform)).

## Building and flashing

### P02 (`zephyr/app/`)

From the **repository root** (attach the board first, see [Setup](setup.md#4-attach-the-board-to-wsl-powershell-as-administrator)):

```bash
source tools/zephyr_env.sh
west build -p always -b arduino_nano_esp32/esp32s3/procpu -s zephyr/app -d zephyr/app/build
west flash -d zephyr/app/build --esp-device /dev/ttyACM0 && python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
```

`-s` is the app and `-d` its build directory. Always give `-d`: without it, west builds in `./build`, which
belongs to ESP-IDF. Inside `zephyr/app` the short form `west build -p always -b arduino_nano_esp32/esp32s3/procpu .`
and `west flash --esp-device /dev/ttyACM0` do the same.

Exit the monitor with `Ctrl+X` (`--exit-char 24`; the default `Ctrl+]` is awkward on a Swedish keyboard).

Then use the [serial commands](usage.md#serial-commands).

### Our samples (`zephyr/samples/`)

Our apps find the board through `list(APPEND BOARD_ROOT ...)` in their own `CMakeLists.txt` and set
`CONFIG_BOOT_DELAY` in `prj.conf`, so no extra flags are needed:

```bash
west build -p always -b arduino_nano_esp32/esp32s3/procpu -s zephyr/samples/blinky -d zephyr/samples/blinky/build
west flash -d zephyr/samples/blinky/build --esp-device /dev/ttyACM0 && python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
```

Expected: the yellow LED blinks and the RGB LED cycles red → green → blue, with one line per second:

```
Yellow: OFF  RGB: green
Yellow: ON   RGB: blue
```

### Zephyr's samples

Zephyr's own samples don't know about our board, so `BOARD_ROOT` is passed on the command line.
Example with `hello_world`:

```bash
west build -p always -b arduino_nano_esp32/esp32s3/procpu -s $ZEPHYR_BASE/samples/hello_world -d hello_world_build -- \
  -DBOARD_ROOT=$P02_ROOT/zephyr \
  -DCONFIG_BOOT_DELAY=1000
west flash -d hello_world_build --esp-device /dev/ttyACM0 && python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
```

* `-p always` – pristine build (start from scratch)
* `BOARD_ROOT` – the directory that contains `boards/`, i.e. the repository's `zephyr/` directory
* `CONFIG_BOOT_DELAY=1000` – waits 1 s before booting, see below

Expected output:

```
*** Booting Zephyr OS build v4.4.2 (delayed boot 1000ms) ***
Hello World! arduino_nano_esp32/esp32s3/procpu
```

**Why `BOOT_DELAY`?** The console runs over the ESP32-S3's built-in USB. Every reset disconnects USB, and
usbipd has to re-attach the board to WSL before the monitor can open `/dev/ttyACM0`. The 1 s boot delay
keeps the boot output until the monitor is open. If the boot output is missing on your computer,
increase `CONFIG_BOOT_DELAY` in the app's `prj.conf` (e.g. to `3000`).

### Useful files and commands

| File / command | Contents |
|---|---|
| `west build -d zephyr/app/build -t menuconfig` | Interactive Kconfig browser (`/` to search, `?` for help) |
| `zephyr/app/build/zephyr/.config` | All active Kconfig options |
| `zephyr/app/build/zephyr/zephyr.dts` | The final devicetree (board + overlays), with the source file of every line |
| `zephyr/app/build/zephyr/zephyr.bin`, `zephyr.elf` | Firmware, ELF with symbols |

## Drivers

| Driver | Zephyr API | Configured from |
|---|---|---|
| `timer::Zephyr` | `k_uptime_get()`, polled | – |
| `gpio::Zephyr` | `gpio_dt_spec`, `gpio_pin_*_dt` | `app.overlay`: alias `p02-led` (D6) |
| `adc::Zephyr` (+ `Tmp36`) | `adc_dt_spec`, `adc_read_dt`, average of 5 samples | `app.overlay`: `zephyr,user` `io-channels` (A6 = ADC2 ch 2) |
| `serial::Zephyr` | `uart_poll_in/out` on the console UART (USB) | `zephyr,console` |
| `nvs::Zephyr` | Settings subsystem, NVS backend | `storage_partition` |

Where things are:

| Path | Contents |
|---|---|
| `main/include/driver/<name>/zephyr.h`, `main/source/driver/<name>/zephyr.cpp` | The drivers, next to the ESP32 and stub versions |
| `main/source/driver/factory/zephyr.cpp` | Maps the roles (`GpioId::StatusLed`, ...) to devicetree |
| `zephyr/app/` | The app: `CMakeLists.txt` (lists the sources from `main/`), `prj.conf`, `app.overlay`, `main.cpp` |
| `zephyr/boards/arduino/nano_esp32/` | Our board definition (Zephyr has none for this board), see [Board](board.md) |
| `zephyr/samples/blinky/` | First exercise: LEDs from devicetree |
| `west.yml`, `tools/zephyr_setup.sh`, `tools/zephyr_env.sh` | The pinned Zephyr workspace |
