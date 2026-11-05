# P02 on the Nucleo-G0B1RE

What is specific to P02 on the ST Nucleo-G0B1RE: how it uses the board, setup, building and flashing, and
wiring. P02 runs on the Nucleo with **Zephyr only** (not ESP-IDF). The board is in Zephyr itself, so this
repository has no board definition or board document for it; the board is described in Zephyr's
[Nucleo-G0B1RE](https://docs.zephyrproject.org/latest/boards/st/nucleo_g0b1re/doc/index.html) page. What
every board needs: [Boards](boards.md).

> **Work in progress.** P02 builds for the board; it has not been run on the board yet.

## How P02 uses the board

| What | On the Nucleo-G0B1RE |
|---|---|
| Board target | `nucleo_g0b1re` (Zephyr's own board definition) |
| Flash | P02's image is ~48 KB of the 512 KB internal flash. No MCUboot: P02 starts directly |
| RAM | ~11 KB of the 144 KB |
| Settings (`store`) | The `storage` partition, the last 64 KB of the internal flash |
| Console | The ST-Link's serial port (USART2) on the ST-Link USB port, `/dev/tty_nucleo_g0b1re` |
| Flashing | The on-board ST-Link, with OpenOCD (comes with the Zephyr SDK) |
| App overlay | [`zephyr/app/boards/nucleo_g0b1re.overlay`](../zephyr/app/boards/nucleo_g0b1re.overlay) |

## Setup (once)

In addition to the [general setup](setup.md) (WSL, usbipd, packages, clone) and the
[Zephyr setup](platform_zephyr.md#setup) (`tools/zephyr_setup.sh` also installs the Arm toolchain and the
STM32 modules):

1. **udev rules.** Install the project's rules ([fixed port names](setup.md#fixed-port-names)). They give
   the port its fixed name, and let OpenOCD open the ST-Link without `sudo`.
2. **Attach the board to WSL.** See [Setup → Attach the board to WSL](setup.md#4-attach-the-board-to-wsl-powershell-as-administrator).
   The board is one USB device, `0483:374b` (`ST-Link Debug, USB Mass Storage Device, USB Serial Device`).

## Building and flashing

From the repository root, after `source tools/zephyr_env.sh`:

```bash
west build -p always -b nucleo_g0b1re -s zephyr/app -d zephyr/app/build/nucleo_g0b1re
west flash -d zephyr/app/build/nucleo_g0b1re --runner openocd && tio /dev/tty_nucleo_g0b1re
```

`--runner openocd` is needed because the board's default runner is STM32CubeProgrammer, which is not
installed. The board does not leave USB when it is reset, so `tio` stays connected. Then use the
[serial commands](usage.md#serial-commands).

## Wiring for P02

All pins are on the Arduino header.

| Role | Arduino pin | STM32 pin | Part |
|---|---|---|---|
| Status LED (`GpioId::StatusLed`) | D6 | PB14 | D6 → 330 Ω → LED anode (long leg), LED cathode → GND. Active high |
| Temperature sensor (`AdcId::TempSensor`) | A0 | PA0 (ADC1 channel 0) | TMP36: Vout to A0, Vs to 3V3, GND to GND |

The pins are the same as on the Nano (D6 for the LED); only the sensor moves from A6 to A0. With the flat
side of the TMP36 facing you, its pins are Vs, Vout, GND – swapping Vs and GND makes it hot.

The green LED LD4 on the board (PA5 = D13) is not used by P02.

## Troubleshooting

| Problem | Cause / fix |
|---|---|
| `west flash`: `libusb_open() failed with LIBUSB_ERROR_ACCESS` | The udev rules are not installed, or were installed before the Nucleo's rule was added. Install them again (see Setup) |
| `west flash` fails looking for STM32CubeProgrammer | The default runner was used. Add `--runner openocd` |
