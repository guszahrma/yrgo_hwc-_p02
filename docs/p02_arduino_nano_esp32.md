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
| Flash (Zephyr) | P02's image is ~421 KB of the 16 MB |
| RAM (Zephyr) | ~53 KB for variables and stacks, plus ~41 KB of code in internal RAM |
| Settings (`store`) | NVS in the flash: ESP-IDF's `nvs` partition, Zephyr's `storage_partition` |
| Console | USB-C port (USB-Serial/JTAG), `/dev/tty_arduino_nano_esp32` |
| Pins, ESP-IDF | Arduino names mapped to GPIOs in [`main/include/driver/pin/esp32s3.h`](../main/include/driver/pin/esp32s3.h) |
| Pins, Zephyr | App overlay [`zephyr/app/boards/arduino_nano_esp32_procpu.overlay`](../zephyr/app/boards/arduino_nano_esp32_procpu.overlay) |

## Setup, building and flashing

The Nano is the board the general guides use:

| Step | Where |
|---|---|
| Setup (WSL, usbipd, packages, udev rules) | [Setup](setup.md) |
| First flash: leave Arduino mode | The board document's [First flash](../zephyr/boards/arduino/nano_esp32/doc/index.md#first-flash-leave-arduino-mode) |
| Build and flash with ESP-IDF | [ESP-IDF environment](espressif_environment.md#building-and-flashing) |
| Build and flash with Zephyr | [Zephyr environment](zephyr_environment.md#building-and-flashing) |

## Wiring for P02

| Role | Arduino pin | ESP32-S3 GPIO | Part |
|---|---|---|---|
| Status LED (`GpioId::StatusLed`) | D6 | GPIO9 | LED + 330 Ω to GND, active high |
| Temperature sensor (`AdcId::TempSensor`) | A6 | GPIO13 (ADC2 channel 2) | TMP36: Vout to A6, Vs to 3V3, GND to GND |

> **Note:** A6 is on ADC2, which cannot be used while Wi-Fi is active. Move the sensor to A0–A3 (ADC1)
> when P02 uses Wi-Fi.
