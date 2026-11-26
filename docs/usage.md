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
| `status` | Show the board, board target and platform (ESP-IDF or Zephyr), the firmware, blink state, period and temperature. The firmware lines: see [Firmware update](#firmware-update) |
| `store` | Save blink state, period and the self-test behaviour to non-volatile storage (loaded at startup) |
| `confirm` | Keep the running firmware after an update. Only on boards with firmware update |
| `selftest pass\|fail\|skip` | Demo: choose how the self-test after the next update behaves. Only on boards with firmware update |
| `hang` | Test the [watchdog](#watchdog): the program stops kicking it, as if it had hung, and the board restarts |

`period` and `blink on` take effect immediately, but are not saved. Run `store` to keep them: the saved
values are loaded again when the board starts. Without `store`, a reset brings back the previous values.

## Firmware update

On the Nucleo-G0B1RE and the Arduino Nano ESP32 with Zephyr, P02 will be updated over the serial port
(not yet: for now a new version is written by hand, see the
[update demo](platform_zephyr.md#update-demo)). `status` shows the running firmware and the second slot, where a new version is received:

```
Firmware: v1.0.0+0 (confirmed)
Second slot: v2.0.0+0 (test at next reset)
Self-test behaviour after next update: skip (demo: confirm by hand, or the next reset reverts)
```

* `(confirmed)`: the running firmware is kept after a reset. After an update it reads `(not confirmed:
  type 'confirm' to keep it, or reset to go back)`: the new version is being tested.
* `Second slot`: the version there, `(test at next reset)` when the next reset will start it as a test,
  or `empty`.
* `Self-test behaviour after next update`: what the new version does when it starts as a test (a demo,
  set with `selftest`): `pass` keeps it, `fail` goes back to the previous version, `skip` leaves it to
  you: `confirm`, or reset to go back. `skip` is the default, so nothing is kept unless you choose it.
  Run `store` before the update: the new version reads the saved value.

When the update tool on the PC asks the board to restart (`smpmgr os reset`), P02 first shuts down:
it turns the LED off, stops blinking and prints

```
Preparing for restart: v2.0.0+0 will be started as a test
Ready for restart
```

(`firmware not changed` instead when no new version is marked for test). Nothing is saved: run `store`
before the restart to keep your settings. Until the board restarts, P02 answers every command with
`Preparing for restart: command rejected`. If the restart is declined (P02 was too slow), P02 says so and
continues with the stored settings, as after a reset. If the restart does not come within about 5 s, P02
lets the [watchdog](#watchdog) restart the board.

For now the lines show `v1.0.0+0 (confirmed)` and `empty`: P02 cannot read the bootloader yet. On the
Daisy Seed and on ESP-IDF there is no firmware update: `status` shows `Firmware update: not supported`,
and `confirm` and `selftest` say so too.

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
