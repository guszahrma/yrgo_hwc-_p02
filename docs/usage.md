# Usage

How to use the program once it is flashed: serial commands, wiring, and switching between ESP-IDF and
Zephyr. The commands and the wiring are the same on both platforms.

## Serial commands

Open the monitor after flashing:

| Platform | Monitor | Exit |
|---|---|---|
| ESP-IDF | `idf.py -p /dev/ttyACM0 monitor` | `Ctrl+]` or `Ctrl+T`, `Ctrl+X` |
| Zephyr | `python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200` | `Ctrl+X` |

Then type:

| Command | Action |
|---|---|
| `help` | List commands |
| `on` / `off` | Turn the LED on / off |
| `blink on` / `blink off` | Start / stop blinking |
| `period <ms>` | Set the blink period, e.g. `period 500` |
| `status` | Show blink state, period and temperature |
| `store` | Save blink state and period to non-volatile storage (loaded at startup) |

`period` and `blink on` take effect immediately, but are not saved. Run `store` to keep them: the saved
values are loaded again when the board starts. Without `store`, a reset brings back the previous values.

## Hardware and wiring

| Role | Arduino pin | ESP32-S3 GPIO | ESP-IDF (`factory::Esp32s3`) | Zephyr (`zephyr/app/app.overlay`) |
|---|---|---|---|---|
| `GpioId::StatusLed` – status LED | D6 | GPIO9 | Arduino pin `D6` | Alias `p02-led` on `&arduino_nano_header 6` |
| `AdcId::TempSensor` – TMP36 | A6 | GPIO13 | Arduino pin `A6` | `zephyr,user` `io-channels = <&adc1 2>` (ADC2 channel 2) |

> **Note:** A6 is on ADC2 (A4–A7), which should not be used while Wi-Fi is active (the Wi-Fi driver uses
> ADC2). Use A0–A3 (ADC1) for analog sensors if Wi-Fi is needed.

How to add or change hardware is described in [Architecture](architecture.md#adding-or-changing-hardware).
The full ESP32-S3 pin mapping is in [`main/include/driver/pin/esp32s3.h`](../main/include/driver/pin/esp32s3.h);
the board's pins and LEDs are in [Board](board.md).

## Switching between ESP-IDF and Zephyr

Switching works without any extra step: flashing the other platform writes its own bootloader and
application. Keep in mind that this happens automatically – the two platforms use different partition
layouts, so the other platform's code and data are overwritten, and saved settings start from the defaults
on the new platform.

If saved settings behave oddly after a switch, erase the whole flash once and flash again:

| Platform | Command |
|---|---|
| ESP-IDF | `idf.py -p /dev/ttyACM0 erase-flash` (in the ESP-IDF environment) |
| Zephyr | `esptool --chip esp32s3 -p /dev/ttyACM0 erase-flash` (in the Zephyr environment) |
