# Report material

Records the report is written from: background, iterations, verification and measurements. Decisions,
terms and lessons are in [`AGENTS.md`](../AGENTS.md) (and the board definitions' own `AGENTS.md`); this file
does not repeat them. New rows go at the bottom of each table.

Records about a board definition (`zephyr/boards/...`) stay in its own `AGENTS.md`, which is copied with
the board. For records written down before this file existed, *Commit* is the commit that first wrote them
down, not necessarily the code that was tested.

## Background and requirements

## Iterations

| Iteration | Goal | Boards | State | Branch / PR |
|---|---|---|---|---|

## Verification

| What | Board target | How | Result | Commit |
|---|---|---|---|---|
| `StatusLed` pin: physical pin 33 = D26 | Daisy Seed (target not recorded) | Zephyr's blinky on the external LED through `daisy_seed_header` | The external LED blinks | f670cd8 |
| `TempSensor` pin: physical pin 25 = A3 | Daisy Seed (target not recorded) | Zephyr's `adc_dt` sample with P02's overlay, turning the potentiometer | The reading follows the potentiometer, ~0–3300 mV | f670cd8 |
| `serial::Zephyr::readLine()` drops input received before a monitor opens the port | Daisy Seed (target not recorded) | `tio`: a command right after P02 started, after 20 s of blinking with `tio` closed, and with `ECHO` on and no udev rule | Clean first command in all three. Nano: builds, not tested | c0170e9 |
| The watchdog restarts P02 through MCUboot, and P02 reports it as the reset cause | `nucleo_g0b1re` (with MCUboot) | The `hang` command | P02 restarts through MCUboot and prints `Last reset was caused by the watchdog` | 1abe641 |

## Measurements

| What | Board target | Value | How measured | Commit |
|---|---|---|---|---|
| P02 image without `std::cout` (with it) | `nucleo_g0b1re` | ~48 KB (~206 KB) | Not recorded | d580a43 |
| P02 image without `std::cout` (with it) | `arduino_nano_esp32/esp32s3/procpu` (Zephyr) | ~141 KB (~432 KB) | Not recorded | d580a43 |
| P02 image without `std::cout` (with it) | Nano ESP32 (ESP-IDF) | ~231 KB (~402 KB) | Not recorded | d580a43 |
| P02 signed image without `std::cout` (with it) | `daisy_seed/stm32h750xx/ram_load` | ~133 KB (~258 KB) | Not recorded | d580a43 |
| P02 in AXI SRAM (code, data and stack) | `daisy_seed/stm32h750xx/ram_load` | ~153 KB of 512 KB | Not recorded | d580a43 |
| MCUboot image (in the 48 KB `boot_partition`) | `nucleo_g0b1re` | ~26 KB | Not recorded | b3680ee |
