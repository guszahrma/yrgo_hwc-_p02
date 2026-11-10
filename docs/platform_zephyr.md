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

## Building and flashing

### P02 (`zephyr/app/`)

From the **repository root** (attach the board first, see [Setup](setup.md#4-attach-the-board-to-wsl-powershell-as-administrator)):

```bash
source tools/zephyr_env.sh
west build -p always --sysbuild -b arduino_nano_esp32/esp32s3/procpu -s zephyr/app -d zephyr/app/build/arduino_nano_esp32 -- -DBOARD_ROOT=$P02_ROOT/zephyr
west flash -d zephyr/app/build/arduino_nano_esp32 --esp-device /dev/tty_arduino_nano_esp32 && tio /dev/tty_arduino_nano_esp32
```

`-s` is the app and `-d` its build directory, one per board (`zephyr/app/build/<board>`). Always give `-d`:
without it, west builds in `./build`, which belongs to ESP-IDF.

**P02 boots through MCUboot** on every Zephyr board. `--sysbuild` builds MCUboot and P02 together
(`zephyr/app/sysbuild.conf`), and `west flash` writes both: MCUboot at the start of the flash, P02 in
`slot0_partition`. `-DBOARD_ROOT` is needed on the command line for our own boards (Nano, Daisy): sysbuild
looks up the board before the app's `CMakeLists.txt` runs. There is no signature yet, only a hash check.

**The first time on a Nano** that ran ESP-IDF or Zephyr without MCUboot, erase the whole flash once
(`esptool --chip esp32s3 -p /dev/tty_arduino_nano_esp32 erase-flash`, see below): the old program at the
start of the flash has another format.

On the Nano, the monitor does not show MCUboot's own output (`MCUboot 2nd stage bootloader`) right after
`west flash`: it is printed before the monitor has opened the port. P02 starting at all shows that MCUboot
started it. After a restart with the monitor already open, such as the watchdog after `hang`, MCUboot's
output is shown.

The monitor is `tio`: it waits until the port is there and reconnects after every reset, so it can stay
open. Exit with `Ctrl+T`, `q` – and quit it before the next `west flash`, which needs the port.

Then use the [serial commands](usage.md#serial-commands).

The other boards: [Nucleo-G0B1RE → Building and flashing](p02_nucleo_g0b1re.md#building-and-flashing)
(OpenOCD) and [Daisy Seed → Building and flashing](p02_daisy_seed.md#building-and-flashing) (MCUboot's
recovery mode and `smpmgr`).

### Our samples (`zephyr/samples/`)

Our apps find the board through `list(APPEND BOARD_ROOT ...)` in their own `CMakeLists.txt` and set
`CONFIG_BOOT_DELAY` in `prj.conf`, so no extra flags are needed:

```bash
west build -p always -b arduino_nano_esp32/esp32s3/procpu -s zephyr/samples/blinky -d zephyr/samples/blinky/build
west flash -d zephyr/samples/blinky/build --esp-device /dev/tty_arduino_nano_esp32 && tio /dev/tty_arduino_nano_esp32
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
west flash -d hello_world_build --esp-device /dev/tty_arduino_nano_esp32 && tio /dev/tty_arduino_nano_esp32
```

* `-p always` – pristine build (start from scratch)
* `BOARD_ROOT` – the directory that contains `boards/`, i.e. the repository's `zephyr/` directory
* `CONFIG_BOOT_DELAY=1000` – waits 1 s before booting, see below

Expected output:

```
*** Booting Zephyr OS build v4.4.2 (delayed boot 1000ms) ***
Hello World! arduino_nano_esp32/esp32s3/procpu
```

**Why `BOOT_DELAY`?** The console runs over the ESP32-S3's built-in USB, and the monitor can only show
what is printed after it has opened `/dev/tty_arduino_nano_esp32`. The 1 s boot delay keeps P02's boot
output until the monitor is open after `west flash`. If the boot output is missing on your computer,
increase `CONFIG_BOOT_DELAY` in the app's `prj.conf` (e.g. to `3000`). The RESET button disconnects USB,
and usbipd needs a few seconds to re-attach the board, so the boot output after RESET is lost; a watchdog
restart keeps USB connected.

### Useful files and commands

| File / command | Contents |
|---|---|
| `west build -d zephyr/app/build/<board> -t menuconfig` | Interactive Kconfig browser for P02 (`/` to search, `?` for help); `-t mcuboot_menuconfig` for MCUboot, `-t sysbuild_menuconfig` for sysbuild |
| `zephyr/app/build/<board>/app/zephyr/.config` | All active Kconfig options of P02 (`mcuboot/zephyr/.config` for MCUboot) |
| `zephyr/app/build/<board>/app/zephyr/zephyr.dts` | The final devicetree (board + overlays), with the source file of every line |
| `zephyr/app/build/<board>/app/zephyr/zephyr.signed.bin`, `zephyr.elf` | P02 as MCUboot loads it (with its image header), ELF with symbols |
| `zephyr/app/build/<board>/mcuboot/zephyr/zephyr.bin` | MCUboot |
| `esptool --chip esp32s3 -p /dev/tty_arduino_nano_esp32 erase-flash` | Nano ESP32: erase the whole flash, including saved settings – e.g. after [switching from ESP-IDF](usage.md#switching-between-esp-idf-and-zephyr) |

## Drivers

| Driver | Zephyr API | Configured from |
|---|---|---|
| `timer::Zephyr` | `k_uptime_get()`, polled | – |
| `gpio::Zephyr` | `gpio_dt_spec`, `gpio_pin_*_dt` | The board's overlay: alias `p02-led` (D6 on the Nano) |
| `adc::Zephyr` (+ `Tmp36`) | `adc_dt_spec`, `adc_read_dt`, average of 5 samples | The board's overlay: `zephyr,user` `io-channels` (A6 = ADC2 ch 2 on the Nano) |
| `serial::Zephyr` | `uart_poll_in/out` on the console UART (USB) | `zephyr,console` |
| `nvs::Zephyr` | Settings subsystem, NVS backend | `storage_partition` |
| `watchdog::Zephyr` | `wdt_install_timeout`, `wdt_setup`, `wdt_feed`; `hwinfo_get_reset_cause()` | The board's alias `watchdog0` (`wdt0` on the Nano, the IWDG on the Daisy Seed); timeout from `Logic` (3 s) |

Where things are:

| Path | Contents |
|---|---|
| `main/include/driver/<name>/zephyr.h`, `main/source/driver/<name>/zephyr.cpp` | The drivers, next to the ESP32 and stub versions |
| `main/source/driver/factory/zephyr.cpp` | Maps the roles (`GpioId::StatusLed`, ...) to devicetree |
| `zephyr/app/` | The app: `CMakeLists.txt` (lists the sources from `main/`), `prj.conf`, `boards/<board>.overlay` (one per board: the wiring), `main.cpp` |
| `zephyr/boards/arduino/nano_esp32/` | Our board definition (Zephyr has none for this board), see [Arduino Nano ESP32](../zephyr/boards/arduino/nano_esp32/doc/index.md) |
| `zephyr/samples/blinky/` | First exercise: LEDs from devicetree |
| `west.yml`, `tools/zephyr_setup.sh`, `tools/zephyr_env.sh` | The pinned Zephyr workspace |
