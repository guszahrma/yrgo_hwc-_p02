# Daisy Seed

Reference for the Electrosmith Daisy Seed (Seed 1.2 / Rev7) and our Zephyr board definition: board facts,
setup, building and flashing, and wiring for P02. The Daisy Seed is supported on **Zephyr only** (not
ESP-IDF). Design decisions and lessons learned are in [`AGENTS.md`](../AGENTS.md#daisy-seed-work-in-progress-design-decisions).

> **Work in progress.** P02 runs on the board (verified). Recovery mode on RESET is committed but not yet
> tested on the board – see [manual tests](daisy_seed_manual_tests.md).

## Board facts

| Item | Details |
|---|---|
| Chip | STM32H750IBK6, Cortex-M7 at 480 MHz (16 MHz crystal) |
| Internal flash | **128 KB**, one sector – only room for a bootloader (MCUboot) or small programs |
| RAM | 512 KB AXI SRAM (P02 runs here), 128 KB DTCM (MCUboot's RAM), more SRAM not used |
| QSPI flash | 8 MB IS25LP064A – P02's image and its settings |
| SDRAM, codec | 64 MB SDRAM and the PCM3060 audio codec – not used yet |
| USB | Micro-USB to the chip's USB OTG FS – there is no USB-UART chip, so the console is a USB serial port |
| LEDs | User LED next to BOOT: PC7, active high (`led0`). The LED next to RESET is the power LED, always on |
| Buttons | **BOOT** starts the chip's ROM bootloader (hold BOOT, press RESET); software cannot read it. **RESET** |

### Pins

40 pins, numbered 1–20 down one side and 21–40 back up the other. The board prints only 1, 10, 21, 32
and 40. Our overlays use the Daisy names (D0–D30, A0–A11); wiring comments give all three names, e.g.
`/* D26 = pin 33 = PD11 */`.

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

Verified on the board: pins 33 (D26) and 25 (A3). The names and STM32 pins are from libDaisy
(`daisy_seed.h`). On a breadboard, the row numbers on the 21–40 side run the other way – one row off is an
easy mistake.

## Our board definition

The Daisy Seed is not in Zephyr (checked in v4.4.2), so this repository has its own out-of-tree board in
[`zephyr/boards/electrosmith/daisy_seed/`](../zephyr/boards/electrosmith/daisy_seed/), modelled on
Zephyr's `stm32h750b_dk` (the same chip). It has two targets:

| Target | What runs | Used for |
|---|---|---|
| `daisy_seed` | A program in the internal flash, no bootloader; flashed with `dfu-util` | Small programs: blinky, bring-up, tests – and MCUboot itself |
| `daisy_seed/stm32h750xx/ram_load` | MCUboot in the internal flash; the app in the QSPI flash, copied into RAM at every start | P02 (~290 KB, too big for the internal flash) |

What the board provides:

| Feature | Details |
|---|---|
| Console | USB serial port (CDC ACM, Zephyr's new USB stack) on the micro-USB port, `2fe3:0004` |
| LED | `led0` = user LED (PC7) |
| Pin header | `daisy_seed_header`: Daisy pin names D0–D30 and A0–A11 (constants in `<dt-bindings/gpio/daisy-seed-header.h>`) |
| QSPI flash | `ext_flash`, partitions `slot0` (1 MB) and `slot1` (1 MB) for app images, `storage` (64 KB) for Settings/NVS |
| Bootloader | MCUboot in RAM-load mode, built automatically with `--sysbuild` for `ram_load` |

Overlays describe wiring with Daisy pin names:

```dts
gpios = <&daisy_seed_header DAISY_SEED_HEADER_D26 GPIO_ACTIVE_HIGH>; /* D26 = pin 33 = PD11 */
```

| File | Contents |
|---|---|
| `daisy_seed-common.dtsi` | Shared by both targets: clocks, LED, USB console, QSPI flash and partitions |
| `daisy_seed.dts`, `daisy_seed_stm32h750xx_ram_load.dts` | The two targets: where the program is stored |
| `daisy_seed_connector.dtsi`, `dts/bindings/`, `include/` | The pin header: D0–D30 → STM32 pins, with our own binding and constants |
| `Kconfig.sysbuild`, `sysbuild.cmake`, `mcuboot.conf`, `mcuboot.overlay` | MCUboot for `ram_load` |
| `mcuboot_hooks.c` | MCUboot: a RESET press enters recovery mode |
| `board.yml`, `Kconfig*`, `board.cmake`, `*_defconfig`, `*.yaml` | Board name, chip, flash runners, default Kconfig, Twister metadata |

## How P02 starts: MCUboot and RAM load

```
Internal flash (128 KB)     QSPI flash (8 MB)                        AXI SRAM (512 KB)
┌──────────────────┐        ┌───────────┬───────────┬─────────┐      ┌──────────────┐
│ MCUboot (~61 KB) │──────▶ │ slot0 1MB │ slot1 1MB │ storage │ ───▶ │ P02 runs here│
└──────────────────┘ reads  │ P02 image │ (unused)  │ 64 KB   │ copy └──────────────┘
                            └───────────┴───────────┴─────────┘
```

At every start MCUboot checks the image in slot0, copies it into RAM and starts it. P02 runs from RAM
rather than directly from the QSPI flash because `store` writes to the same QSPI chip.

MCUboot also has a **recovery mode**, in which it accepts a new P02 over USB (`smpmgr`):

| Start | What happens |
|---|---|
| Power-on, `smpmgr os reset` | P02 starts at once |
| RESET pressed | Recovery mode: waits for `smpmgr` as long as needed (*not yet tested on the board*) |
| No valid P02 in slot0 | Recovery mode |

## Setup (once)

In addition to the [general setup](setup.md) (WSL, usbipd, packages, clone):

1. **Zephyr workspace.** [`tools/zephyr_setup.sh`](zephyr_environment.md#setup) also installs what the
   Daisy needs: the Arm toolchain (`arm-zephyr-eabi`), the STM32 and MCUboot modules, and `smpmgr`.
2. **udev rules.** Install the project's rules ([fixed port names](setup.md#fixed-port-names)). For the
   Daisy they name the serial port `/dev/tty_daisy_seed`, in MCUboot and in P02 alike, and give `dfu-util`
   access to the ROM bootloader, which the `dialout` group does not cover (otherwise:
   `LIBUSB_ERROR_ACCESS`).
3. **Attach the board to WSL – two USB devices.** The board shows up as a different USB device depending
   on what runs (use a data cable; a charge-only cable shows nothing):

   | USB ID | When | Windows shows |
   |---|---|---|
   | `0483:df11` | ROM bootloader (BOOT + RESET) | *DFU in FS Mode*, with *Code 28* (no driver) – ignore it, WSL does not need one |
   | `2fe3:0004` | MCUboot or an app (USB serial port) | *USB Serial Device (COMx)* |

   Bind **each** of them once, while it is connected (same BUSID), and keep `--auto-attach` running
   (PowerShell as administrator):
   ```powershell
   usbipd bind --busid <id>
   usbipd attach --wsl --busid <id> --auto-attach
   ```
   After every reset or flash, usbipd takes **5–15 s** to attach the board again; until then there is no
   `/dev/tty_daisy_seed`.
4. **Backup (recommended before the first flash).** Read-only, outside the repository:
   ```bash
   mkdir -p ~/daisy_seed_backup && cd ~/daisy_seed_backup
   dfu-util -a 1 -s 0x5200201C:128 -U option_bytes.bin          # option bytes
   dfu-util -a 0 -s 0x08000000:131072 -U internal_flash.bin     # internal flash (128 KB)
   ```
   Restore with `dfu-util -a 0 -s 0x08000000:leave -D internal_flash.bin`. The ROM bootloader cannot read
   the QSPI flash.

## Building and flashing

All commands from the repository root, after `source tools/zephyr_env.sh`. Zephyr does not know our
board, so every Daisy build passes `-DBOARD_ROOT=$P02_ROOT/zephyr` – also for `zephyr/app`, because
sysbuild looks up the board before the app's `CMakeLists.txt` runs.

### Flashing with the ROM bootloader (`dfu-util`)

Used for the plain target and for MCUboot. Hold **BOOT**, press and release **RESET**, release **BOOT**:
the board shows up as `0483:df11`. Then `west flash`.

`west flash` ends with `dfu-util: Error during download get_status` / `FATAL ERROR: command exited with
status 74`. **This is harmless**: the flash worked if `File downloaded successfully` is printed – the ROM
bootloader starts the program before answering dfu-util's last request. If the program does not start
(`lsusb` still shows `0483:df11`), press RESET.

### Zephyr's blinky (plain target)

```bash
west build -p always -b daisy_seed -s deps/zephyr/samples/basic/blinky -d blinky_build -- -DBOARD_ROOT=$P02_ROOT/zephyr
west flash -d blinky_build
```

The user LED blinks with a 2 s period; `LED state: ON/OFF` is printed on the USB console.

### P02 (`zephyr/app/`)

Build P02 and MCUboot together:

```bash
west build -p always --sysbuild -b daisy_seed/stm32h750xx/ram_load -d zephyr/app/daisy_seed_build zephyr/app -- -DBOARD_ROOT=$P02_ROOT/zephyr
```

**MCUboot** – the first time, and when MCUboot or the board files change. BOOT + RESET, then:

```bash
west flash -d zephyr/app/daisy_seed_build --domain mcuboot
```

**P02** – every time P02 changes. Press **RESET** (recovery mode), then:

```bash
sleep 2; until [ -e /dev/tty_daisy_seed ]; do sleep 0.5; done
smpmgr --port /dev/tty_daisy_seed --timeout 30 image upload zephyr/app/daisy_seed_build/app/zephyr/zephyr.signed.bin
smpmgr --port /dev/tty_daisy_seed os reset
```

`--timeout 30`: MCUboot erases the whole slot when the upload starts, which takes longer than smpmgr's
default 2 s. The warning `Error reading MCUMgr parameters ... ENOTSUP` is harmless (MCUboot does not
support that query). `west flash` cannot do this step: the ROM bootloader cannot write the QSPI flash.

Then open the console and use the [serial commands](usage.md#serial-commands):

```bash
tio /dev/tty_daisy_seed
```

P02's welcome text is usually printed before usbipd has attached the board; type `help`.

## Wiring for P02

| Role | Daisy pin | Part |
|---|---|---|
| Status LED (`GpioId::StatusLed`) | D26 = pin 33 = PD11 | LED + 330 Ω to GND (pin 40), active high |
| Temperature sensor (`AdcId::TempSensor`) | A3 = pin 25 = PA7 (ADC1 channel 7) | A potentiometer simulating a TMP36: wiper to pin 25, outer legs to 3V3 (pin 38) and GND (pin 40) |

The mapping is in [`zephyr/app/boards/daisy_seed_ram_load.overlay`](../zephyr/app/boards/daisy_seed_ram_load.overlay).

## Troubleshooting

| Problem | Cause / fix |
|---|---|
| `FATAL ERROR: command exited with status 74` after `west flash` | Harmless, see [Flashing with the ROM bootloader](#flashing-with-the-rom-bootloader-dfu-util) |
| `dfu-util: Cannot open DFU device ... (LIBUSB_ERROR_ACCESS)` | The udev rule is missing, see [Setup](#setup-once) step 2 |
| No USB device at all, with nothing flashed | The board only shows up in BOOT mode (BOOT + RESET), or the cable is charge-only |
| `tio` shows *Waiting for tty device* after a flash or reset | usbipd has not attached the board yet (5–15 s); `tio` connects by itself |
| No `/dev/tty_daisy_seed`, but a `/dev/ttyACM*` | The udev rules are missing (Setup step 2), or the board runs software from before the USB product name `Daisy Seed`: flash MCUboot and upload P02 again |
| The board stays `0483:df11` after `west flash` | The program did not start: press RESET |
| `smpmgr ... connection timeout` | The board is not in recovery mode, or not attached yet: press RESET, wait for `/dev/tty_daisy_seed` |
| `smpmgr image state-read` says `No images on device!` | A known quirk of MCUboot's image list in RAM-load mode; the image boots anyway |
| `smpmgr` or `west flash` fails with the port busy, or `device reports readiness to read but returned no data` | Another program has the port open, often `tio`: quit it (`Ctrl+T`, `q`). `fuser -v /dev/ttyACM*` shows who |
