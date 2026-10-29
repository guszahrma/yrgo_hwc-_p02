# Usage

How to use the program once it is flashed: serial commands, wiring, and switching between
ESP-IDF and Zephyr.

## Serial commands

Connect with `idf.py -p /dev/ttyACM0 monitor` and type:

| Command | Action |
|---|---|
| `help` | List commands |
| `on` / `off` | Turn the LED on / off |
| `blink on` / `blink off` | Start / stop blinking |
| `period <ms>` | Set the blink period, e.g. `period 500` |
| `status` | Show blink state, period and temperature |
| `store` | Save blink state and period to NVS (loaded at startup) |

## Hardware and wiring

| Role | Arduino pin | ESP32-S3 GPIO | Notes |
|---|---|---|---|
| `GpioId::StatusLed` – status LED | D6 | GPIO9 | Output |
| `AdcId::TempSensor` – TMP36 | A6 | GPIO13 | ADC2, **does not work while Wi-Fi is active** |

`Logic` asks the driver factory for a **role**, not a pin, e.g.
`factory.createGpio(driver::factory::GpioId::StatusLed, ...)`. Each platform's factory decides which pin
a role is. To add or change hardware:

1. Define the role in `main/include/driver/factory/interface.h` (`GpioId` or `AdcId`).
2. Map the role to a pin in each platform's factory:
   - ESP32-S3: `main/source/driver/factory/esp32s3.cpp` (via the Arduino pin names in
     `driver/pin/esp32s3.h`)
   - Stub: `main/include/driver/factory/stub.h`
3. The compiler warns (`-Wswitch`) if a factory does not handle a new role.

> **Note:** ADC2 (A4–A7) cannot be used while Wi-Fi is active. Use A0–A3 (ADC1) for analog sensors if
> Wi-Fi is needed.

The full pin mapping is in [`main/include/driver/pin/esp32s3.h`](../main/include/driver/pin/esp32s3.h).

## Switching between ESP-IDF and Zephyr

The platforms use different partition layouts. Erase the whole flash when switching:

```bash
idf.py -p /dev/ttyACM0 erase-flash
```
