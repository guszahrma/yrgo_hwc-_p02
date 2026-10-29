# Board

Reference for the Arduino Nano ESP32 (ABX00083) and our Zephyr board definition.

## Board facts (from the ABX00083 schematic)

| Item | Details |
|---|---|
| Module | u-blox NORA-W106 (ESP32-S3R8, 8 MB PSRAM) |
| Flash | External GD25B128, 128 Mbit = **16 MB** |
| USB | USB-C D+/D− go **directly** to GPIO19/20 (ESP32-S3 USB-Serial/JTAG) – there is no USB-UART chip |
| UART0 | GPIO43 (TX, D1) / GPIO44 (RX, D0) on the header |
| Yellow LED ("SCK LED", D13) | GPIO48 via a MOSFET – **active high** |
| RGB LED | Common anode to 3V3 – **active low**: red = GPIO46, green = GPIO0, blue = GPIO45 (verified on the board) |
| Boot strapping pins | GPIO0 and GPIO46 – take care when using the RGB LED |

LEDs (verified on the board):

| Zephyr alias | LED | GPIO | Polarity |
|---|---|---|---|
| `led0` | Yellow (D13) | GPIO48 | Active high (N-MOSFET) |
| `led1` / `led2` / `led3` | RGB green / blue / red | GPIO0 / GPIO45 / GPIO46 | Active low (common anode) |

The schematic does not say which RGB pin is which colour; found by testing, and matches Arduino's
`pins_arduino.h` for the Nano ESP32.

## Our board definition

**The Arduino Nano ESP32 is not a supported board in Zephyr** (checked in v4.4.2 and on `main`), so this
repository has its own out-of-tree board definition in
[`zephyr/boards/arduino/nano_esp32/`](../zephyr/boards/arduino/nano_esp32/), based on the ABX00083 schematic.
Board target for all Zephyr builds:

```
arduino_nano_esp32/esp32s3/procpu
```

What the board provides:

| Feature | Details |
|---|---|
| Chip | ESP32-S3 with 16 MB flash and 8 MB PSRAM (`esp32s3_wroom_n16r8`) |
| Console and shell | USB-C port (`usb_serial`, the ESP32-S3 USB-Serial/JTAG) |
| LEDs | `led0` yellow, `led1` RGB green, `led2` RGB blue, `led3` RGB red |
| Pin header | `arduino_nano_header`: Arduino pin names D0-D13 and A0-A7 (A0-A7 = 14-21) |
| UART | `uart0` on D0 (RX) / D1 (TX), enabled – Arduino's `Serial1`, not the console |
| I2C | `i2c0` on A4 (SDA) / A5 (SCL), also named `arduino_nano_i2c` – disabled, enable in the app |
| SPI | `spi2` on D11 (COPI) / D12 (CIPO) / D13 (SCK), also named `arduino_nano_spi` – disabled |

Thanks to the pin header, app overlays describe wiring with Arduino pin names instead of ESP32-S3 GPIO
numbers:

```dts
gpios = <&arduino_nano_header 6 GPIO_ACTIVE_HIGH>;   /* D6 (= GPIO9) */
```

| File | Contents |
|---|---|
| `arduino_nano_esp32_procpu.dts` | The board: console, LEDs, aliases, peripherals |
| `arduino_nano_esp32-pinctrl.dtsi` | Which GPIOs UART, I2C and SPI use |
| `arduino_nano_connector.dtsi` | The Nano pin header: D0-D13, A0-A7 → GPIO |
| `board.yml`, `Kconfig*`, `board.cmake`, `*_defconfig`, `*.yaml` | Board name, chip variant, flash runner, default Kconfig, Twister metadata |

Zephyr only looks for boards in its own tree, so apps point it to ours with `BOARD_ROOT` (see
[Zephyr environment](zephyr_environment.md#building-and-flashing)).
