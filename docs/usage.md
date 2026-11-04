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
| `period <ms>` | Set the blink period, 50–10 000 ms, e.g. `period 500`. Other values are rejected and the period is kept |
| `status` | Show blink state, period and temperature |
| `store` | Save blink state and period to non-volatile storage (loaded at startup) |
| `hang` | Test the [watchdog](#watchdog): the program stops as if it had hung, and the board restarts |

`period` and `blink on` take effect immediately, but are not saved. Run `store` to keep them: the saved
values are loaded again when the board starts. Without `store`, a reset brings back the previous values.

## Watchdog

The program kicks a watchdog on every pass of its main loop (timeout 3 s). If the loop stops – a bug, or
the `hang` command – the kicks stop and the watchdog restarts the board. On the Arduino Nano ESP32 that
takes about 3 s with ESP-IDF and about 6 s with Zephyr. At the next start the program prints
`Last reset was caused by the watchdog`. On the Daisy Seed the watchdog is not tested yet.

## Hardware and wiring

P02 needs a status LED and a temperature sensor. Which pins they are on depends on the board:

| Board | Wiring |
|---|---|
| Arduino Nano ESP32 | LED on D6, TMP36 on A6: [P02 on the Arduino Nano ESP32 → Wiring](p02_arduino_nano_esp32.md#wiring-for-p02) |
| Daisy Seed | LED on D26 (pin 33), potentiometer on A3 (pin 25): [P02 on the Daisy Seed → Wiring](p02_daisy_seed.md#wiring-for-p02) |

How to add or change hardware is described in [Architecture](architecture.md#adding-or-changing-hardware).

## Switching between ESP-IDF and Zephyr

Only the Arduino Nano ESP32 runs both platforms. To switch, open a **new terminal** and follow the other
platform's guide: [ESP-IDF environment](platform_esp_idf.md) or [Zephyr environment](platform_zephyr.md).
Never activate both environments in one terminal.

Flashing the other platform overwrites everything on the board: the two platforms use different partition
layouts, so saved settings start from the defaults. If they behave oddly after a switch, erase the whole
flash once and flash again (`erase-flash` under *Useful files and commands* in each guide).
