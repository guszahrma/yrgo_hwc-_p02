# P02 on the Arduino Nano ESP32

What is specific to P02 on the Arduino Nano ESP32: how it uses the board, and wiring. P02 runs on the Nano
with **ESP-IDF and Zephyr**. The board itself – facts, LEDs, the first flash out of Arduino mode, the
Zephyr board definition – is described in its board document,
[Arduino Nano ESP32](../zephyr/boards/arduino/nano_esp32/doc/index.md). What every board needs:
[Boards](boards.md).

## How P02 uses the board

| What | On the Nano ESP32 |
|---|---|
| Zephyr board target | `arduino_nano_esp32/esp32s3/procpu` |
| Boot | ESP-IDF: ESP-IDF's own bootloader. Zephyr: MCUboot, which starts P02 in `slot0_partition` |
| Flash (Zephyr) | P02's image is ~141 KB of the 16 MB |
| RAM (Zephyr) | ~53 KB for variables and stacks, plus ~41 KB of code in internal RAM |
| Settings (`store`) | NVS in the flash: ESP-IDF's `nvs` partition, Zephyr's `storage_partition` |
| Console | USB-C port (USB-Serial/JTAG), `/dev/tty_arduino_nano_esp32` |
| Pins, ESP-IDF | Arduino names mapped to GPIOs in [`main/include/driver/pin/esp32s3.h`](../main/include/driver/pin/esp32s3.h) |
| Pins, Zephyr | App overlay [`zephyr/app/boards/arduino_nano_esp32_procpu.overlay`](../zephyr/app/boards/arduino_nano_esp32_procpu.overlay) |

## Memory layout

The partitions P02 is built with on Zephyr, and the room P02 has in its slot. P02 changes the board's slots
(the board's own layout is in the board document's
[Memory layout](../zephyr/boards/arduino/nano_esp32/doc/index.md#memory-layout)): slot 0 is one sector smaller
than slot 1, and that sector is unused (`zephyr/app/partitions/arduino_nano_esp32.overlay`). Printed by
`tools/flash_layout.py` from a build of 5d52fd9; what the terms mean: [Boards → Memory layout](boards.md#memory-layout).

| Memory | Size | Sector | Partition | Offset | Size |
|---|---|---|---|---|---|
| `flash0` | 16 MB | 4 KB | `boot_partition` | 0x0 | 64 KB |
|  |  |  | `sys_partition` | 0x10000 | 64 KB |
|  |  |  | `slot0_partition` | 0x20000 | 1340 KB |
|  |  |  | *unused* | 0x16F000 | 4 KB |
|  |  |  | `slot1_partition` | 0x170000 | 1344 KB |
|  |  |  | `slot0_appcpu_partition` | 0x2C0000 | 448 KB |
|  |  |  | `slot1_appcpu_partition` | 0x330000 | 448 KB |
|  |  |  | `slot0_lpcore_partition` | 0x3A0000 | 32 KB |
|  |  |  | `slot1_lpcore_partition` | 0x3A8000 | 32 KB |
|  |  |  | `storage_partition` | 0x3B0000 | 192 KB |
|  |  |  | `scratch_partition` | 0x3E0000 | 124 KB |
|  |  |  | `coredump_partition` | 0x3FF000 | 4 KB |
|  |  |  | *unused* | 0x400000 | 12 MB |

| P02 in `slot0_partition` | Value |
|---|---|
| Board target | `arduino_nano_esp32/esp32s3/procpu` |
| MCUboot mode | swap using offset |
| Slot size | 1340 KB (0x14F000) |
| Header (`ROM_START_OFFSET`) | 0x20 (32 B) |
| End of the slot kept free (`ROM_END_OFFSET`) | 0x1028 (4.0 KB): MCUboot's trailer 4 KB + its estimate for the TLVs 40 B |
| Maximum header + code + TLVs | 1336 KB (0x14E000), set by `slot0_partition` |
| P02 today: header + code + TLVs | 138.43 KB (141,756 B = 0x229BC), of which TLVs 40 B |
| Margin | 1197.6 KB |

The ESP32-S3's linker does not check this maximum; the build's size check (`tools/check_image_size.py`) does.

## Setup, building and flashing

The Nano is the board the general guides use:

| Step | Where |
|---|---|
| Setup (WSL, usbipd, packages, udev rules) | [Setup](setup.md) |
| First flash: leave Arduino mode | The board document's [First flash](../zephyr/boards/arduino/nano_esp32/doc/index.md#first-flash-leave-arduino-mode) |
| Build and flash with ESP-IDF | [ESP-IDF environment](platform_esp_idf.md#building-and-flashing) |
| Build and flash with Zephyr | [Zephyr environment](platform_zephyr.md#building-and-flashing) |

## Wiring for P02

| Role | Arduino pin | ESP32-S3 GPIO | Part |
|---|---|---|---|
| Status LED (`GpioId::StatusLed`) | D6 | GPIO9 | LED + 330 Ω to GND, active high |
| Temperature sensor (`AdcId::TempSensor`) | A6 | GPIO13 (ADC2 channel 2) | TMP36: Vout to A6, Vs to 3V3, GND to GND |

> **Note:** A6 is on ADC2, which cannot be used while Wi-Fi is active. Move the sensor to A0–A3 (ADC1)
> when P02 uses Wi-Fi.
