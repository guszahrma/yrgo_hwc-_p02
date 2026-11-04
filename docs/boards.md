# Boards

Which boards P02 runs on, and what a board needs to run it. Each supported board has two documents: a
board document, which describes the board and its Zephyr board definition and can be used by any project,
and a page about P02 on that board (wiring, building and flashing).

## Supported boards

| Board | ESP-IDF | Zephyr | Zephyr board target | Board document | P02 on the board |
|---|---|---|---|---|---|
| Arduino Nano ESP32 (ABX00083) | Yes | Yes | `arduino_nano_esp32/esp32s3/procpu` | [Arduino Nano ESP32](../zephyr/boards/arduino/nano_esp32/doc/index.md) | [P02 on the Arduino Nano ESP32](p02_arduino_nano_esp32.md) |
| Electrosmith Daisy Seed (Seed 1.2 / Rev7) | No | Yes, work in progress | `daisy_seed/stm32h750xx/ram_load` (with MCUboot) | [Daisy Seed](../zephyr/boards/electrosmith/daisy_seed/doc/index.md) | [P02 on the Daisy Seed](p02_daisy_seed.md) |

Neither board is in Zephyr, so both have out-of-tree board definitions in [`zephyr/boards/`](../zephyr/boards/).

Where each board's setup, building and flashing are described:

| Board | Setup | Building and flashing |
|---|---|---|
| Arduino Nano ESP32 | [Setup](setup.md) | [ESP-IDF environment](platform_esp_idf.md), [Zephyr environment](platform_zephyr.md#building-and-flashing) |
| Daisy Seed | [P02 on the Daisy Seed → Setup](p02_daisy_seed.md#setup-once) | [P02 on the Daisy Seed → Building and flashing](p02_daisy_seed.md#building-and-flashing) |

## What a board needs

P02 (`logic::Logic`) asks the factory for hardware by role, not by pin. A board can run P02 when the
factory for its platform can provide every role.

### Zephyr

`factory::Zephyr` takes everything from devicetree, so any board Zephyr supports can run P02 if it has
everything below. The needs are listed in the order that rules a board out: first the chip, which cannot
be changed and can be checked in the datasheet; then the board definition, which takes work; then the app
overlay (wiring); last `prj.conf`, which is already set for every board.

| Need | Provided by | Details |
|---|---|---|
| Chip supported by Zephyr | Zephyr's architectures and SoCs | e.g. ARM Cortex-M, Xtensa, RISC-V. Not 8-bit AVR, so not the Arduino Uno R3 |
| Flash for the program | The chip or external flash | P02's image is ~421 KB on the Nano ESP32 and ~258 KB on the Daisy Seed (Cortex-M7) |
| RAM | The chip | ~53 KB for variables and stacks on the Nano ESP32, ~60 KB on the Daisy Seed, plus the code if it runs from RAM (Daisy Seed: ~283 KB in all). The Arduino Uno R4 (32 KB RAM, 256 KB flash) is too small |
| Board definition | Zephyr, or our own in `zephyr/boards/` | Found through `BOARD_ROOT` when it is ours |
| Console (serial commands) | `zephyr,console` chosen by the board | USB CDC ACM or UART |
| Settings storage (NVS) | `storage_partition` in the board's flash partitions | Must be in flash that P02 can write while it runs |
| Watchdog | Alias `watchdog0` in the board definition | A watchdog with a Zephyr driver that can reset the chip (`WDT_FLAG_RESET_SOC`) |
| Status LED (`GpioId::StatusLed`) | Alias `p02-led` in the app overlay | A GPIO output; polarity is set in devicetree |
| Temperature sensor (`AdcId::TempSensor`) | `io-channels` in `zephyr,user` in the app overlay | One ADC channel, with gain and resolution set in the overlay; its range must cover the sensor's output |
| C++17 and `std::cout` | `zephyr/app/prj.conf` | `CONFIG_CPP`, full libstdc++ and `CONFIG_POSIX_API`; `std::cout` costs ~225 KB of flash |
| Main stack | `zephyr/app/prj.conf` | 4 KB (`CONFIG_MAIN_STACK_SIZE=4096`); Zephyr's default 1 KB overflows on `store` |

The app overlay is `zephyr/app/boards/<board>.overlay`, named after the full board target with `/` → `_`
(e.g. `daisy_seed_ram_load.overlay`). The file name rules are in
[`AGENTS.md`](../AGENTS.md#board-and-devicetree).

### ESP-IDF

`factory::Esp32s3` supports only the ESP32-S3 on the Arduino Nano ESP32: roles map to Arduino pin names
(`D6`, `A6`) in [`main/include/driver/pin/esp32s3.h`](../main/include/driver/pin/esp32s3.h). Another
ESP-IDF board needs its own pin mapping and factory.

## Adding a board (Zephyr)

1. If Zephyr has no definition for the board, add one in `zephyr/boards/<vendor>/<board>/`. Copy a board
   with the same chip, and change **every** board-specific value.
2. Make sure the board has a console and a `storage_partition`.
3. Add the app overlay `zephyr/app/boards/<board>.overlay` with `p02-led` and `zephyr,user` (see
   [Architecture](architecture.md#adding-or-changing-hardware)), and check the `Found devicetree overlay:`
   line in the build output.
4. Add a CI job that builds P02 for the board (see [Contributing](contributing.md)).
5. Add a board document (`zephyr/boards/<vendor>/<board>/doc/index.md`, nothing about P02 in it), a
   `docs/p02_<board>.md` page, and a row in the tables above.
