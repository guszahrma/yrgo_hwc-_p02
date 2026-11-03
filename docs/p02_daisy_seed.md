# P02 on the Daisy Seed

What is specific to P02 on the Electrosmith Daisy Seed: setup in this repository, building and flashing,
and wiring. P02 runs on the Daisy Seed with **Zephyr only** (not ESP-IDF). The board itself – facts, pins,
board targets, how an app starts, troubleshooting – is described in its board document,
[Daisy Seed](../zephyr/boards/electrosmith/daisy_seed/doc/index.md). What every board needs:
[Boards](boards.md).

> **Work in progress.** P02 runs on the board, including `store` and recovery mode on RESET.

## How P02 uses the board

| What | On the Daisy Seed |
|---|---|
| Board target | `daisy_seed/stm32h750xx/ram_load`: P02's image (~258 KB) does not fit in the 128 KB internal flash. MCUboot copies it from the QSPI flash into RAM, so `store` can write to the QSPI flash |
| RAM | ~283 KB of the 512 KB AXI SRAM, code included |
| Settings (`store`) | The `storage` partition in the QSPI flash |
| Console | USB serial port on the micro-USB port, `/dev/tty_daisy_seed` |
| App overlay | [`zephyr/app/boards/daisy_seed_ram_load.overlay`](../zephyr/app/boards/daisy_seed_ram_load.overlay) |

## Setup (once)

In addition to the [general setup](setup.md) (WSL, usbipd, packages, clone):

1. **Zephyr workspace.** [`tools/zephyr_setup.sh`](platform_zephyr.md#setup) installs everything the board
   document lists: the Arm toolchain, the STM32 and MCUboot modules, and `smpmgr` (into `.venv`). `dfu-util`
   is among the packages in the general setup.
2. **udev rules.** Install the project's rules ([fixed port names](setup.md#fixed-port-names)); they include
   the Daisy's.
3. **Attach the board to WSL.** See [Setup → Attach the board to WSL](setup.md#4-attach-the-board-to-wsl-powershell-as-administrator):
   the board is two USB devices, bind both.
4. **Backup (recommended before the first flash),** also in the board document's Setup. Keep it outside the
   repository, e.g. in `~/daisy_seed_backup`.

## Building and flashing

From the repository root, after `source tools/zephyr_env.sh`. What each step does is explained in the
board document's [Building and flashing](../zephyr/boards/electrosmith/daisy_seed/doc/index.md#building-and-flashing).

Build P02 and MCUboot together:

```bash
west build -p always --sysbuild -b daisy_seed/stm32h750xx/ram_load -d zephyr/app/daisy_seed_build zephyr/app -- \
  -DBOARD_ROOT=$P02_ROOT/zephyr
```

**MCUboot** – the first time, and when MCUboot or the board definition changes. BOOT + RESET, then:

```bash
west flash -d zephyr/app/daisy_seed_build --domain mcuboot
```

**P02** – every time P02 changes. Press **RESET** (recovery mode), then:

```bash
until [ -e /dev/tty_daisy_seed ]; do sleep 0.5; done
smpmgr --port /dev/tty_daisy_seed image state-read     # recovery mode answers "No images on device!"
smpmgr --port /dev/tty_daisy_seed --timeout 30 image upload \
  zephyr/app/daisy_seed_build/app/zephyr/zephyr.signed.bin
smpmgr --port /dev/tty_daisy_seed os reset
```

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

## Troubleshooting

The board's own problems (USB, `dfu-util`, `smpmgr`) are in the board document's
[Troubleshooting](../zephyr/boards/electrosmith/daisy_seed/doc/index.md#troubleshooting).

| Problem | Cause / fix |
|---|---|
| `smpmgr`: `Frame could not be parsed` | P02 is running, not recovery mode: it echoes smpmgr's request. Press RESET |
| P02 runs (the LED blinks) but `/dev/tty_daisy_seed` never appears, after `smpmgr os reset` or a flash | A known, intermittent fault: P02 started with its USB off. Press RESET (recovery), then `smpmgr ... os reset` or a power cycle |
| No `/dev/tty_daisy_seed` after an update from an old branch | Software from before the USB product name `Daisy Seed`: flash MCUboot and upload P02 again |
