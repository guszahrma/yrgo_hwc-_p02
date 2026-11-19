# P02 on the Nucleo-G0B1RE

What is specific to P02 on the ST Nucleo-G0B1RE: how it uses the board, setup, building and flashing, and
wiring. P02 runs on the Nucleo with **Zephyr only** (not ESP-IDF). The board is in Zephyr itself, so this
repository has no board definition or board document for it; the board is described in Zephyr's
[Nucleo-G0B1RE](https://docs.zephyrproject.org/latest/boards/st/nucleo_g0b1re/doc/index.html) page. What
every board needs: [Boards](boards.md).

> **Work in progress.** P02 runs on the board, started by MCUboot.

## How P02 uses the board

| What | On the Nucleo-G0B1RE |
|---|---|
| Board target | `nucleo_g0b1re` (Zephyr's own board definition) |
| Flash | MCUboot (~26 KB) at the start of the internal flash starts P02 (~48 KB) in the 198 KB `slot0_partition` |
| RAM | ~11 KB of the 144 KB |
| Settings (`store`) | The `storage` partition, the last 64 KB of the internal flash |
| Console | The ST-Link's serial port (USART2) on the ST-Link USB port, `/dev/tty_nucleo_g0b1re` |
| Flashing | The on-board ST-Link, with OpenOCD (comes with the Zephyr SDK) |
| App overlay | [`zephyr/app/boards/nucleo_g0b1re.overlay`](../zephyr/app/boards/nucleo_g0b1re.overlay) |

## Memory layout

The hardware gives one 512 KB flash, the chip's internal flash (`flash0`), with 2 KB sectors (the smallest
block the flash can erase). How it is divided into partitions is the devicetree's choice, not the
hardware's: Zephyr's board (`boards/st/nucleo_g0b1re/nucleo_g0b1re.dts` in Zephyr) defines the partitions,
and P02 changes its slots so that slot 1 is one sector larger than slot 0
(`zephyr/app/partitions/nucleo_g0b1re.overlay`). The tables show the partitions P02 is built with, and the
room P02 has in its slot; offsets are from the start of the flash. Printed by `tools/flash_layout.py` from a
build of 10342b2; what the terms mean: [Boards → Memory layout](boards.md#memory-layout).

| Column | Set by |
|---|---|
| Memory, Size, Sector | The hardware |
| Partition, Offset, Size | The devicetree: change it to move, resize, add or remove partitions |

| Memory | Size | Sector | Partition | Offset | Size |
|---|---|---|---|---|---|
| `flash0` | 512 KB | 2 KB | `boot_partition` | 0x0 | 48 KB |
|  |  |  | `slot0_partition` | 0xC000 | 198 KB |
|  |  |  | `slot1_partition` | 0x3D800 | 202 KB |
|  |  |  | `storage_partition` | 0x70000 | 64 KB |

| P02 in `slot0_partition` | Value |
|---|---|
| Board target | `nucleo_g0b1re/stm32g0b1xx` |
| MCUboot mode | swap using offset |
| Slot size | 198 KB (0x31800) |
| Header (`ROM_START_OFFSET`) | 0x200 (512 B) |
| End of the slot reserved for MCUboot (`ROM_END_OFFSET`) | 0x828 (2.0 KB) |
| Maximum header + code | 196.0 KB (0x30FD8) |
| P02 today: header + code | 48.9 KB (0xC380) |
| Margin in the slot | 147.1 KB |

The flash is fully used: more storage would have to come from the slots, which means building and flashing
MCUboot again with a cable.

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
west build -p always --sysbuild -b nucleo_g0b1re -s zephyr/app -d zephyr/app/build/nucleo_g0b1re
west flash -d zephyr/app/build/nucleo_g0b1re --runner openocd && tio /dev/tty_nucleo_g0b1re
```

`--sysbuild` builds MCUboot and P02 together, and `west flash` writes both (see
[Zephyr environment](platform_zephyr.md#building-and-flashing)). No `-DBOARD_ROOT`: the board is in Zephyr.
`--runner openocd` is needed because the board's default runner is STM32CubeProgrammer, which is not
installed. The board does not leave USB when it is reset, so `tio` stays connected and shows MCUboot's
output (`*** Booting MCUboot ***`, `Jumping to the first image slot`) before P02's. Then use the
[serial commands](usage.md#serial-commands).

MCUboot trying a new version and rolling it back: [Zephyr environment → Update demo](platform_zephyr.md#update-demo).

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
