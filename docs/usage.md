# Usage

How to use the program once it is flashed: serial commands, wiring, and switching between ESP-IDF and
Zephyr. The commands and the wiring are the same on both platforms.

## Serial commands

Open the monitor after flashing:

| Platform | Monitor | Exit |
|---|---|---|
| ESP-IDF | `idf.py -p /dev/tty_arduino_nano_esp32 monitor` | `Ctrl+]` or `Ctrl+T`, `Ctrl+X` |
| Zephyr | `tio /dev/tty_arduino_nano_esp32` (Daisy Seed: `tio /dev/tty_daisy_seed`) | `Ctrl+T`, `q` |

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

P02 needs a status LED and a temperature sensor. Which pins they are on depends on the board:

| Board | Wiring |
|---|---|
| Arduino Nano ESP32 | LED on D6, TMP36 on A6: [P02 on the Arduino Nano ESP32 → Wiring](p02_arduino_nano_esp32.md#wiring-for-p02) |
| Daisy Seed | LED on D26 (pin 33), potentiometer on A3 (pin 25): [P02 on the Daisy Seed → Wiring](p02_daisy_seed.md#wiring-for-p02) |

How to add or change hardware is described in [Architecture](architecture.md#adding-or-changing-hardware).

## Switching between ESP-IDF and Zephyr

Switching works without any extra step: flashing the other platform writes its own bootloader and
application. Keep in mind that this happens automatically – the two platforms use different partition
layouts, so the other platform's code and data are overwritten, and saved settings start from the defaults
on the new platform.

If saved settings behave oddly after a switch, erase the whole flash once and flash again:

| Platform | Command |
|---|---|
| ESP-IDF | `idf.py -p /dev/tty_arduino_nano_esp32 erase-flash` (in the ESP-IDF environment) |
| Zephyr | `esptool --chip esp32s3 -p /dev/tty_arduino_nano_esp32 erase-flash` (in the Zephyr environment) |
