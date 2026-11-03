# Arduino Nano ESP32

Zephyr board definition for the Arduino Nano ESP32 (ABX00083). Zephyr does not include this board (checked
in v4.4.2 and on `main`); this definition is based on the ABX00083 schematic. The board facts below also
apply outside Zephyr (ESP-IDF, Arduino). The reasons behind the design, for those changing the board
definition, are in [`AGENTS.md`](../AGENTS.md).

## Board facts

From the ABX00083 schematic and the ESP32-S3 datasheet.

| Item | Details |
|---|---|
| Chip | ESP32-S3R8, dual-core Xtensa LX7 at 240 MHz, in a u-blox NORA-W106 module |
| Flash | External GD25B128, 128 Mbit = **16 MB** |
| RAM | 512 KB internal SRAM |
| PSRAM | 8 MB, in the chip package – not enabled in the board definition |
| USB | USB-C D+/D− go **directly** to GPIO19/20 (ESP32-S3 USB-Serial/JTAG) – there is no USB-UART chip |
| UART0 | GPIO43 (TX, D1) / GPIO44 (RX, D0) on the header |
| Yellow LED ("SCK LED", D13) | GPIO48 via a MOSFET – **active high** |
| RGB LED | Common anode to 3V3 – **active low**: red = GPIO46, green = GPIO0, blue = GPIO45 |
| Buttons | **RESET**. Connecting pin B1 to GND while resetting starts the ROM bootloader |
| Boot strapping pins | GPIO0 and GPIO46 – take care when using the RGB LED |
| ADC | A0–A3 are on ADC1, A4–A7 on ADC2. ADC2 cannot be used while Wi-Fi is active |

LEDs:

| Zephyr alias | LED | GPIO | Polarity |
|---|---|---|---|
| `led0` | Yellow (D13) | GPIO48 | Active high (N-MOSFET) |
| `led1` / `led2` / `led3` | RGB green / blue / red | GPIO0 / GPIO45 / GPIO46 | Active low (common anode) |

## First flash: leave Arduino mode

The Nano ESP32 ships with the Arduino bootloader, which `esptool` cannot flash. Before the first flash:

1. Connect pin **B1 to GND**.
2. Press **RESET** – the board starts in the ROM bootloader and shows up as `303a:1001`.
3. Flash, remove the wire and press RESET.

After this, `esptool` can put the board into flash mode by itself.

| USB ID | Meaning |
|---|---|
| `303a:1001` | ESP32-S3 USB-Serial/JTAG – ready to flash |
| `2341:0070` | Still the Arduino firmware – do the steps above first |

To restore the Arduino bootloader: B1 to GND + RESET, then *Burn Bootloader* in the Arduino IDE.

The USB-Serial/JTAG disconnects and reconnects at every reset, also while flashing. Under WSL, keep
`usbipd attach --wsl --busid <id> --auto-attach` running. A udev rule for a fixed port name, whatever
`/dev/ttyACM<n>` the board gets (it matches any board with the ESP32-S3's USB-Serial/JTAG):

```
SUBSYSTEM=="tty", ATTRS{idVendor}=="303a", ATTRS{idProduct}=="1001", SYMLINK+="tty_arduino_nano_esp32"
```

## Board definition

Board target:

```
arduino_nano_esp32/esp32s3/procpu
```

What the board definition provides:

| Feature | Details |
|---|---|
| Chip | ESP32-S3 with 16 MB flash and 8 MB PSRAM (`esp32s3_wroom_n16r8`) |
| Console and shell | USB-C port (`usb_serial`, the ESP32-S3 USB-Serial/JTAG) |
| LEDs | `led0` yellow, `led1` RGB green, `led2` RGB blue, `led3` RGB red |
| Pin header | `arduino_nano_header`: Arduino pin names D0-D13 and A0-A7 (A0-A7 = 14-21) |
| UART | `uart0` on D0 (RX) / D1 (TX), enabled – Arduino's `Serial1`, not the console |
| I2C | `i2c0` on A4 (SDA) / A5 (SCL), also named `arduino_nano_i2c` – disabled, enable in the app |
| SPI | `spi2` on D11 (COPI) / D12 (CIPO) / D13 (SCK), also named `arduino_nano_spi` – disabled |
| Flash partitions | Espressif's default layout (`partitions_0x0_amp.dtsi`), including `storage_partition` for settings |

App overlays describe wiring with Arduino pin names instead of ESP32-S3 GPIO numbers, for instance:

```dts
gpios = <&arduino_nano_header 6 GPIO_ACTIVE_HIGH>;   /* D6 (= GPIO9) */
```

| File | Contents |
|---|---|
| `arduino_nano_esp32_procpu.dts` | The board: console, LEDs, aliases, peripherals |
| `arduino_nano_esp32-pinctrl.dtsi` | Which GPIOs UART, I2C and SPI use |
| `arduino_nano_connector.dtsi` | The Nano pin header: D0-D13, A0-A7 → GPIO |
| `board.yml`, `Kconfig*`, `board.cmake`, `*_defconfig`, `*.yaml` | Board name, chip variant, flash runner, default Kconfig, Twister metadata |

## Building and flashing

Zephyr does not know this board, so every build passes `-DBOARD_ROOT=<board root>`: the directory that
contains `boards/arduino/nano_esp32/` (or the app's `CMakeLists.txt` adds it before `find_package(Zephyr)`).
`west flash` uses `esptool` over the USB-C port:

```bash
west build -p always -b arduino_nano_esp32/esp32s3/procpu -s $ZEPHYR_BASE/samples/basic/blinky -d blinky_build -- \
  -DBOARD_ROOT=<board root>
west flash -d blinky_build --esp-device /dev/tty_arduino_nano_esp32
```

The yellow LED blinks with a 2 s period.

## Design decisions

The short version; the reasons are in [`AGENTS.md`](../AGENTS.md).

| Decision | Why |
|---|---|
| Our own board definition, based on the schematic | Zephyr has no Nano ESP32, and `esp32s3_devkitc` has less flash and no PSRAM |
| Chip variant `esp32s3_wroom_n16r8` | The same memory as the Nano's NORA-W106 module: 16 MB flash, 8 MB PSRAM |
| Arduino pin names (`arduino_nano_header`) | Wiring is written with the names printed on the board, not GPIO numbers |
| I2C and SPI disabled | Their pins are also plain GPIOs on the header; an app enables the bus it uses |
