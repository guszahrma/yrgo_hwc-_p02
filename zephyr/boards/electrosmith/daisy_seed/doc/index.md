# Electrosmith Daisy Seed

Zephyr board definition for the Electrosmith Daisy Seed (Seed 1.2 / Rev7). Zephyr does not include this
board (checked in v4.4.2); this definition is modelled on Zephyr's `stm32h750b_dk`, which has the same
chip. The reasons behind the design, for those changing the board definition, are in
[`AGENTS.md`](../AGENTS.md).

> **Work in progress.** Tested with Zephyr 4.4.2.

## Board facts

| Item | Details |
|---|---|
| Chip | STM32H750IBK6, Cortex-M7 at 480 MHz (16 MHz crystal) |
| Internal flash | **128 KB** – room for a bootloader (MCUboot) or small programs |
| RAM | 512 KB AXI SRAM (apps built for `ram_load` run here), 128 KB DTCM (MCUboot's RAM) |
| QSPI flash | 8 MB IS25LP064A – app images and settings |
| SDRAM, codec | 64 MB SDRAM and the PCM3060 audio codec – not in the board definition yet |
| USB | Micro-USB to the chip's USB OTG FS – there is no USB-UART chip, so the console is a USB serial port |
| LEDs | User LED next to BOOT: PC7, active high (`led0`). The LED next to RESET is the power LED |
| Buttons | **BOOT**: hold it and press **RESET** to start the chip's ROM bootloader; software cannot read it. **RESET** |

### Pins

40 pins, numbered 1–20 down one side and 21–40 back up the other. The board prints only 1, 10, 21, 32
and 40. Devicetree uses the Daisy names (D0–D30, A0–A11).

| Pin | Name | STM32 | | Pin | Name | STM32 |
|---|---|---|---|---|---|---|
| 1 | D0 | PB12 | | 40 | DGND | |
| 2 | D1 | PC11 | | 39 | VIN | |
| 3 | D2 | PC10 | | 38 | 3V3 digital | |
| 4 | D3 | PC9 | | 37 | D30 | PB15 |
| 5 | D4 | PC8 | | 36 | D29 | PB14 |
| 6 | D5 | PD2 | | 35 | D28 / A11 | PA2 |
| 7 | D6 | PC12 | | 34 | D27 | PG9 |
| 8 | D7 | PG10 | | 33 | D26 | PD11 |
| 9 | D8 | PG11 | | 32 | D25 / A10 | PA0 |
| 10 | D9 | PB4 | | 31 | D24 / A9 | PA1 |
| 11 | D10 | PB5 | | 30 | D23 / A8 | PA4 |
| 12 | D11 | PB8 | | 29 | D22 / A7 | PA5 |
| 13 | D12 | PB9 | | 28 | D21 / A6 | PC4 |
| 14 | D13 | PB6 | | 27 | D20 / A5 | PC1 |
| 15 | D14 | PB7 | | 26 | D19 / A4 | PA6 |
| 16 | Audio in L | | | 25 | D18 / A3 | PA7 |
| 17 | Audio in R | | | 24 | D17 / A2 | PB1 |
| 18 | Audio out L | | | 23 | D16 / A1 | PA3 |
| 19 | Audio out R | | | 22 | D15 / A0 | PC0 |
| 20 | AGND | | | 21 | 3V3 analog | |

The names and STM32 pins are from libDaisy (`daisy_seed.h`). On a breadboard, the row numbers on the 21–40
side run the other way – one row off is an easy mistake.

## Board definition

Two board targets:

| Board target | What runs | Used for |
|---|---|---|
| `daisy_seed` | A program in the internal flash, no bootloader | Small programs (bring-up, tests) and MCUboot itself |
| `daisy_seed/stm32h750xx/ram_load` | MCUboot in the internal flash; the app in the QSPI flash, copied into RAM at every start | Apps larger than 128 KB |

What the board definition provides:

| Feature | Details |
|---|---|
| Console | USB serial port (CDC ACM) on the micro-USB port, USB ID `2fe3:0004`, product name `Daisy Seed` |
| LED | `led0` = user LED (PC7) |
| Watchdog | `watchdog0` = the independent watchdog (IWDG1) |
| Pin header | `daisy_seed_header`: Daisy pin names D0–D30 and A0–A11 (constants in `<dt-bindings/gpio/daisy-seed-header.h>`) |
| QSPI flash | `ext_flash`, partitions `slot0` (1 MB) and `slot1` (1 MB) for app images, `storage` (64 KB) for settings |
| Bootloader | MCUboot, built automatically with `--sysbuild` for `ram_load` |

App overlays describe wiring with Daisy pin names. Give all three names in a comment: only the physical pin
matches the board's print, and the STM32 name is what datasheets and errors use, for instance:

```dts
gpios = <&daisy_seed_header DAISY_SEED_HEADER_D26 GPIO_ACTIVE_HIGH>; /* D26 = pin 33 = PD11 */
```

| File | Contents |
|---|---|
| `daisy_seed-common.dtsi` | Shared by both targets: clocks, LED, USB console, QSPI flash and partitions |
| `daisy_seed.dts`, `daisy_seed_stm32h750xx_ram_load.dts` | The two targets: where the program is stored |
| `daisy_seed_connector.dtsi`, `dts/bindings/`, `include/` | The pin header: D0–D30 → STM32 pins |
| `Kconfig.sysbuild`, `sysbuild.cmake`, `mcuboot.conf`, `mcuboot.overlay`, `mcuboot_hooks.c` | MCUboot for `ram_load` |
| `board.yml`, `Kconfig*`, `board.cmake`, `*_defconfig`, `*.yaml` | Board name, chip, flash runners, default Kconfig, Twister metadata |

## How an app starts: MCUboot and RAM load

```
Internal flash (128 KB)     QSPI flash (8 MB)                        AXI SRAM (512 KB)
┌──────────────────┐        ┌───────────┬───────────┬─────────┐      ┌──────────────┐
│ MCUboot (~61 KB) │──────▶ │ slot0 1MB │ slot1 1MB │ storage │ ───▶ │ app runs here│
└──────────────────┘ reads  │ app image │ (unused)  │ 64 KB   │ copy └──────────────┘
                            └───────────┴───────────┴─────────┘
```

At every start MCUboot checks the image in slot0, copies it into RAM and starts it. The app runs from RAM,
not directly from the QSPI flash, so that it can write its settings to the same chip.

MCUboot also has a **recovery mode**, in which it accepts a new app over USB (`smpmgr`). Which one starts
depends on how the board was reset:

```
BOOT held + RESET ──► ROM bootloader (DFU, 0483:df11)
                          │ RESET (also after west flash)
                          ▼
power-on ───────────► MCUboot ──── RESET pressed alone ────────────► Recovery mode
RESET ──────────────►    │                                             ▲  waits for smpmgr,
smpmgr os reset ────►    │ any other reset                             │  no timeout
(from recovery)          ▼                                             │
                     valid app in slot0? ──── no ──────────────────────┘
                         │ yes
                         ▼
                     App running
```

| You are in | Do | You end up in |
|---|---|---|
| The app | Power cycle (unplug, plug in) | The app |
| The app | RESET | Recovery mode |
| The app | BOOT held + RESET | ROM bootloader |
| Recovery mode | `smpmgr ... image upload <file>` | Recovery mode, with the new app in slot0 |
| Recovery mode | `smpmgr ... os reset`, or a power cycle | The app |
| ROM bootloader | `west flash ...` | ROM bootloader, with the new program in internal flash |
| ROM bootloader | RESET | Recovery mode |

How to tell them apart: in recovery mode `smpmgr ... image state-read` answers; the ROM bootloader shows up
as `0483:df11` in `lsusb`. The app and recovery mode use the same serial port.

## Setup (once)

In addition to a Zephyr workspace:

| What | Why |
|---|---|
| Toolchain `arm-zephyr-eabi` (Zephyr SDK) | Cortex-M7 |
| Modules `hal_stm32`, `cmsis_6`, `mcuboot`, `zcbor` | The STM32H7 drivers, and MCUboot with its serial recovery |
| `dfu-util` | Flashes the internal flash through the ROM bootloader |
| `smpmgr` (`pip install smpmgr`) | Uploads apps to MCUboot's recovery mode |

**udev rules (Linux).** A fixed port name, whatever `/dev/ttyACM<n>` the board gets, and access to the ROM
bootloader for `dfu-util` (otherwise `LIBUSB_ERROR_ACCESS`). In `/etc/udev/rules.d/`, then
`sudo udevadm control --reload-rules && sudo udevadm trigger`:

```
SUBSYSTEM=="tty", ATTRS{idVendor}=="2fe3", ATTRS{idProduct}=="0004", ATTRS{product}=="Daisy Seed", SYMLINK+="tty_daisy_seed"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="df11", MODE="0660", GROUP="dialout"
```

**WSL.** The board is two different USB devices, depending on what runs; bind each once with `usbipd` while
it is connected, and keep `usbipd attach --wsl --busid <id> --auto-attach` running:

| USB ID | When | Windows shows |
|---|---|---|
| `0483:df11` | ROM bootloader (BOOT + RESET) | *DFU in FS Mode*, *Code 28* (no driver) – WSL does not need one |
| `2fe3:0004` | MCUboot or an app | *USB Serial Device (COMx)* |

After every reset usbipd takes a few seconds to attach the board again. Use a data cable – a charge-only
cable shows nothing.

**Backup (before the first flash).** The ROM bootloader can read the option bytes and the internal flash,
but not the QSPI flash:

```bash
dfu-util -a 1 -s 0x5200201C:128 -U option_bytes.bin          # option bytes
dfu-util -a 0 -s 0x08000000:131072 -U internal_flash.bin     # internal flash (128 KB)
```

Restore with `dfu-util -a 0 -s 0x08000000:leave -D internal_flash.bin`.

## Building and flashing

Zephyr does not know this board, so every build passes `-DBOARD_ROOT=<board root>`: the directory that
contains `boards/electrosmith/daisy_seed/`. Pass it on the command line also when the app's `CMakeLists.txt`
sets it – with `--sysbuild`, the board is looked up before that file runs.

### The ROM bootloader (`dfu-util`)

For the plain target and for MCUboot. Hold **BOOT**, press and release **RESET**, release **BOOT**: the
board shows up as `0483:df11`. Then `west flash`. The board stays in the ROM bootloader afterwards: **press
RESET** to start the program.

### Plain target: Zephyr's blinky

```bash
west build -p always -b daisy_seed -s $ZEPHYR_BASE/samples/basic/blinky -d blinky_build -- \
  -DBOARD_ROOT=<board root>
west flash -d blinky_build
```

Then press RESET. The user LED blinks with a 2 s period.

### An app on `ram_load` (MCUboot)

Build the app and MCUboot together:

```bash
west build -p always --sysbuild -b daisy_seed/stm32h750xx/ram_load -d <build dir> <app> -- \
  -DBOARD_ROOT=<board root>
```

**MCUboot** – the first time, and when MCUboot or the board definition changes. BOOT + RESET, then:

```bash
west flash -d <build dir> --domain mcuboot
```

**The app** – every time it changes. Press **RESET** (recovery mode), wait for the port, and check that
recovery mode answers:

```bash
until [ -e /dev/tty_daisy_seed ]; do sleep 0.5; done
smpmgr --port /dev/tty_daisy_seed image state-read
```

Recovery mode answers `No images on device!`, even with an app in slot0 – a known quirk of MCUboot in
RAM-load mode. If it fails with `Frame could not be parsed` instead, the app is still running: press RESET
again. Then upload the app and start it:

```bash
smpmgr --port /dev/tty_daisy_seed --timeout 30 image upload <build dir>/<app name>/zephyr/zephyr.signed.bin
smpmgr --port /dev/tty_daisy_seed os reset
```

`--timeout 30`, because MCUboot erases the whole slot before it writes. `west flash` cannot do this step:
the ROM bootloader cannot write the QSPI flash.

## Troubleshooting

| Problem | Cause / fix |
|---|---|
| `dfu-util: Cannot open DFU device ... (LIBUSB_ERROR_ACCESS)` | The udev rule for `0483:df11` is missing, see [Setup](#setup-once) |
| No USB device at all, with nothing flashed | The board only shows up in the ROM bootloader (BOOT + RESET), or the cable is charge-only |
| No `/dev/tty_daisy_seed`, but a `/dev/ttyACM*` | The udev rules are missing (see [Setup](#setup-once)) |
| The board stays `0483:df11` after `west flash` | As intended: press RESET to start the program |
| `smpmgr ... connection timeout` | The board is not in recovery mode, or not attached yet: press RESET and wait for the port |
| `smpmgr`: `Frame could not be parsed` | The app is running, not recovery mode: press RESET |
| `smpmgr`: `Error reading MCUMgr parameters ... ENOTSUP` | Harmless: MCUboot does not support that query |
| `dfu-util`: `Invalid DFU suffix signature` | Harmless: `zephyr.bin` has no DFU suffix |
| Port busy, or `device reports readiness to read but returned no data` | Another program has the port open, often a serial monitor: quit it. `fuser -v /dev/ttyACM*` shows who |

## Design decisions

The short version; the reasons and how each was tested are in [`AGENTS.md`](../AGENTS.md).

| Decision | Why |
|---|---|
| Only Seed 1.2 / Rev7 | The only version tested. The versions differ in the codec, which is not in the board definition |
| Flashing over USB only: `dfu-util` for the internal flash, MCUboot and `smpmgr` for the QSPI flash | No debug probe is needed. The ROM bootloader cannot write the QSPI flash |
| Apps run from RAM (`ram_load`), not from the QSPI flash | The QSPI flash cannot be read as memory while it is written, so an app running from it could not save settings |
| A RESET press enters recovery mode; power-on and software resets start the app | MCUboot cannot read any button on the Daisy. `mcuboot_hooks.c` tells RESET apart from other resets by the reset flags |
| MCUboot switches USB on only in recovery mode | Switched on at every start, the PC lost the board half-way through connecting when MCUboot started the app |
| QSPI writes with `PP_1_1_4` | Zephyr's default command corrupts writes on this flash chip |
| `dfu-util` without `leave` | With it, every flash ended with an error, although the program was written |
| Daisy pin names (`daisy_seed_header`) | Wiring is written with the names printed in the Daisy's pinout, not STM32 names |
| No SDRAM yet | Added when something needs it, as its own tested step |
