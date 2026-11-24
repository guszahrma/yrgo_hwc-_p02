# Boards

Which boards P02 runs on, and what a board needs to run it. Each supported board has two documents: a
board document, which describes the board and its Zephyr board definition and can be used by any project,
and a page about P02 on that board (wiring, building and flashing).

## Supported boards

| Board | ESP-IDF | Zephyr | Zephyr board target | Board document | P02 on the board |
|---|---|---|---|---|---|
| Arduino Nano ESP32 (ABX00083) | Yes | Yes | `arduino_nano_esp32/esp32s3/procpu` | [Arduino Nano ESP32](../zephyr/boards/arduino/nano_esp32/doc/index.md) | [P02 on the Arduino Nano ESP32](p02_arduino_nano_esp32.md) |
| Electrosmith Daisy Seed (Seed 1.2 / Rev7) | No | Yes, work in progress | `daisy_seed/stm32h750xx/ram_load` (with MCUboot) | [Daisy Seed](../zephyr/boards/electrosmith/daisy_seed/doc/index.md) | [P02 on the Daisy Seed](p02_daisy_seed.md) |
| ST Nucleo-G0B1RE | No | Yes, work in progress | `nucleo_g0b1re` | Zephyr's [Nucleo-G0B1RE](https://docs.zephyrproject.org/latest/boards/st/nucleo_g0b1re/doc/index.html) | [P02 on the Nucleo-G0B1RE](p02_nucleo_g0b1re.md) |

The Nano ESP32 and the Daisy Seed are not in Zephyr, so they have out-of-tree board definitions in
[`zephyr/boards/`](../zephyr/boards/). The Nucleo-G0B1RE is in Zephyr: it needs only an app overlay.

Where each board's setup, building and flashing are described:

| Board | Setup | Building and flashing |
|---|---|---|
| Arduino Nano ESP32 | [Setup](setup.md) | [ESP-IDF environment](platform_esp_idf.md), [Zephyr environment](platform_zephyr.md#building-and-flashing) |
| Daisy Seed | [P02 on the Daisy Seed → Setup](p02_daisy_seed.md#setup-once) | [P02 on the Daisy Seed → Building and flashing](p02_daisy_seed.md#building-and-flashing) |
| Nucleo-G0B1RE | [P02 on the Nucleo-G0B1RE → Setup](p02_nucleo_g0b1re.md#setup-once) | [P02 on the Nucleo-G0B1RE → Building and flashing](p02_nucleo_g0b1re.md#building-and-flashing) |

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
| Flash for the program | The chip or external flash | P02's image is ~141 KB on the Nano ESP32, ~133 KB on the Daisy Seed (Cortex-M7) and ~48 KB on the Nucleo-G0B1RE (Cortex-M0+) |
| RAM | The chip | ~53 KB for variables and stacks on the Nano ESP32, ~11 KB on the Nucleo-G0B1RE, plus the code if it runs from RAM (Daisy Seed: ~153 KB in all). The Arduino Uno R4 (32 KB RAM, 256 KB flash) is too small |
| Board definition | Zephyr, or our own in `zephyr/boards/` | Found through `BOARD_ROOT` when it is ours |
| Console (serial commands) | `zephyr,console` chosen by the board | USB CDC ACM or UART |
| Settings storage (NVS) | `storage_partition` in the board's flash partitions | Must be in flash that P02 can write while it runs |
| Watchdog | Alias `watchdog0` in the board definition | A watchdog with a Zephyr driver that can reset the chip (`WDT_FLAG_RESET_SOC`) |
| Status LED (`GpioId::StatusLed`) | Alias `p02-led` in the app overlay | A GPIO output; polarity is set in devicetree |
| Temperature sensor (`AdcId::TempSensor`) | `io-channels` in `zephyr,user` in the app overlay | One ADC channel, with gain and resolution set in the overlay; its range must cover the sensor's output |
| C++17 | `zephyr/app/prj.conf` | `CONFIG_CPP`, full libstdc++, and `CONFIG_POSIX_API` for `std::this_thread::sleep_for`. P02 prints through the serial driver, not `std::cout`, which would add ~160 KB of flash |
| Main stack | `zephyr/app/prj.conf` | 4 KB (`CONFIG_MAIN_STACK_SIZE=4096`); Zephyr's default 1 KB overflows on `store` |

The app overlay is `zephyr/app/boards/<board>.overlay`, named after the full board target with `/` → `_`
(e.g. `daisy_seed_ram_load.overlay`). The file name rules are in
[`AGENTS.md`](../AGENTS.md#board-and-devicetree).

### ESP-IDF

`factory::Esp32s3` supports only the ESP32-S3 on the Arduino Nano ESP32: roles map to Arduino pin names
(`D6`, `A6`) in [`main/include/driver/pin/esp32s3.h`](../main/include/driver/pin/esp32s3.h). Another
ESP-IDF board needs its own pin mapping and factory.

## Memory layout

Where each board's flash is divided into partitions, and how much room P02 has in its slot:

| Board | Partitions | P02 in its slot |
|---|---|---|
| Arduino Nano ESP32 | [Board document → Memory layout](../zephyr/boards/arduino/nano_esp32/doc/index.md#memory-layout) | [P02 on the Arduino Nano ESP32 → Memory layout](p02_arduino_nano_esp32.md#memory-layout) |
| Daisy Seed | [Board document → Memory layout](../zephyr/boards/electrosmith/daisy_seed/doc/index.md#memory-layout) | [P02 on the Daisy Seed → Memory layout](p02_daisy_seed.md#memory-layout) |
| Nucleo-G0B1RE | [P02 on the Nucleo-G0B1RE → Memory layout](p02_nucleo_g0b1re.md#memory-layout) | The same page |

MCUboot starts P02 from `slot0_partition` and receives an update in `slot1_partition`. P02 cannot use its
whole slot:

```
slot0_partition
┌────────┬───────────────────────────┬────────┬──────────────────────────────────┐
│ header │ P02's code                │ free   │ reserved for MCUboot             │
└────────┴───────────────────────────┴────────┴──────────────────────────────────┘
 ROM_START_OFFSET                               ROM_END_OFFSET
 ◄──────── maximum header + code ─────────────►
```

| Term | Meaning |
|---|---|
| Sector | The smallest block the flash can erase. MCUboot erases and moves whole sectors |
| Header (`CONFIG_ROM_START_OFFSET`) | MCUboot's image header (version, sizes), written by `imgtool` when it signs the image |
| Trailer | MCUboot's update state at the end of the slot: magic, `image_ok`, `copy_done`, swap progress |
| `CONFIG_ROM_END_OFFSET` | What the build keeps free at the end of the slot: the trailer, room for the image's TLVs (hash, signature) and, depending on the mode, one sector |
| Maximum header + code | Slot size − `ROM_END_OFFSET` (for swap using offset with equal slots, also − one sector, see below) |
| Margin | Maximum − what P02 uses today |

What is kept free at the end of the slot depends on the MCUboot mode:

| MCUboot mode | Kept free | Boards |
|---|---|---|
| Swap using move | Trailer + one sector, which the swap moves the image into. Both are in `ROM_END_OFFSET` | None |
| Swap using offset | Trailer; an update is written one sector into slot 1. With slot 1 one sector larger than slot 0 (MCUboot's recommendation), nothing more. With equal slots, also one sector, which MCUboot's build leaves out of `ROM_END_OFFSET`; P02's build stops with equal slots (see below) | Nucleo, Nano (slot 1 one sector larger) |
| RAM load | Trailer only. MCUboot copies header + code into RAM, where P02's data and stacks are too, so the RAM is the real limit | Daisy |

Who stops an image that is too large:

* **P02's build**, on every board: after every build with `--sysbuild`, `tools/check_image_size.py` checks
  that the signed image fits in both slots (and for RAM load in MCUboot's RAM), and otherwise stops with
  `P02 does not fit`, the limit and the bytes over. For swap using offset it also stops when slot 1 is not
  exactly one sector larger than slot 0.
* **The linker**, with `region 'FLASH' overflowed`, on Cortex-M (Nucleo, Daisy): its FLASH region ends at
  `ROM_END_OFFSET`. Not on the Nano: the ESP32-S3's linker script does not use `ROM_END_OFFSET`.
* **`imgtool`**, when it signs the image (every board), but against the whole slot and with its own
  trailer estimate, so it can pass an image that MCUboot later refuses.
* **MCUboot**, at the update: it keeps the old version if the new one leaves no room for the trailer (from
  MCUboot's code, not tested).

To see how much room is left, run `tools/flash_layout.py` after a build, in the Zephyr environment:

```bash
tools/flash_layout.py zephyr/app/build/nucleo_g0b1re
```

It prints both tables as Markdown, from the build's devicetree and Kconfig and the signed image's header.
Paste them into the board's documents when the layout, the MCUboot mode or P02's size changes noticeably.

## Adding a board (Zephyr)

1. If Zephyr has no definition for the board, add one in `zephyr/boards/<vendor>/<board>/`. Copy a board
   with the same chip, and change **every** board-specific value.
2. Make sure the board has a console and a `storage_partition`, and slots that P02 fits in (the build
   checks it): build, then run `tools/flash_layout.py` and add its tables to the board's documents (see
   [Memory layout](#memory-layout)).
3. Add the app overlay `zephyr/app/boards/<board>.overlay` with `p02-led` and `zephyr,user` (see
   [Architecture](architecture.md#adding-or-changing-hardware)), and check the `Found devicetree overlay:`
   line in the build output.
4. Add a CI job that builds P02 for the board (see [Contributing](contributing.md)).
5. Add a board document (`zephyr/boards/<vendor>/<board>/doc/index.md`, nothing about P02 in it), a
   `docs/p02_<board>.md` page, and a row in the tables above.
