# AGENTS.md – Arduino Nano ESP32 board definition

Context for AI coding assistants changing this board definition: design decisions and lessons from the
bring-up. People should start with [`doc/index.md`](doc/index.md).

## Design decisions

| Decision | Choice | Why |
|---|---|---|
| Board definition | Own out-of-tree board, based on the ABX00083 schematic, found through `BOARD_ROOT` | The Nano ESP32 is not in Zephyr (checked in v4.4.2 and on `main`) |
| Chip variant | `SOC_ESP32S3_WROOM_N16R8` (`esp32s3_wroom_n16r8.dtsi`) | The same memory as the NORA-W106: 16 MB flash, 8 MB PSRAM. `esp32s3_devkitc` uses the N8 variant (8 MB flash, no PSRAM) |
| Pin names | `arduino_nano_header` in `arduino_nano_connector.dtsi`: D0–D13, A0–A7 (= 14–21) → GPIO | App overlays use the names printed on the board |
| I2C and SPI | `i2c0` and `spi2` configured but disabled | An app enables the bus it uses; until then its pins stay free for GPIO or ADC |

## Devicetree facts

* `gpio0` = GPIO 0–31, `gpio1` = GPIO 32–48 (pin = GPIO − 32).
* ADC: `adc0` = the chip's ADC1, `adc1` = ADC2. `io-channels = <&adc1 2>` takes the **channel** number
  (A6 = GPIO13 = ADC2 ch 2), not the GPIO number. `ADC_GAIN_1_4` = 12 dB (~3.1 V full scale).
* The schematic does not say which RGB pin is which colour; found by testing on the board, and matches
  Arduino's `pins_arduino.h` for the Nano ESP32. All LEDs verified on the board.

## Lessons learned

* When copying another board, change **every** board-specific value: copied SPI pinctrl landed on the
  I2C pins and nothing complained.
* Disabled nodes are not checked against their bindings – test the board with each bus `okay`
  (`pinctrl-names ... is marked as required` only showed after enabling `i2c0`/`spi2`).
