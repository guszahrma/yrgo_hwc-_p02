# Report material

Records the report is written from: background, iterations, verification and measurements. Decisions,
terms and lessons are in [`AGENTS.md`](../AGENTS.md) (and the board definitions' own `AGENTS.md`); this file
does not repeat them. New rows go at the bottom of each table.

Records about a board definition (`zephyr/boards/...`) stay in its own `AGENTS.md`, which is copied with
the board. For records written down before this file existed, *Commit* is the commit that first wrote them
down, not necessarily the code that was tested.

## Report structure (planned)

1. Background and requirements
2. Design decisions, with the alternatives considered
3. Implementation
4. Verification: what was tested, how, on which board, and the result
5. Problems and lessons learned
6. Results and measurements (image sizes, timings)
7. Future work and open items

## Background and requirements

* P02 gets firmware updates on Zephyr through MCUboot's two slots (see *Firmware update* in the glossary).
  ESP-IDF gets no firmware updates.
* Two variants: the swap variant, where P02 fits in the internal flash (Nano, Nucleo), and the RAM-load
  variant, where it does not (Daisy).
* Developed in iterations (see [Iterations](#iterations)). Secure boot (protecting MCUboot itself) and
  encrypted images are future work.

## Iterations

| Iteration | Goal | Boards | State | Branch / PR |
|---|---|---|---|---|
| 1. Prototype | A firmware update by hand: v2 signed with `west sign --pad`, written to slot 1, started as a test, reverted at the next reset | Nano, Nucleo | In progress | `feature/mcuboot-zephyr-boards`, `feature/mcuboot-update-demo` |
| 2. Generalised for Zephyr | Firmware updates in both variants: swap (Nano, Nucleo) and RAM load (Daisy) | Nano, Nucleo, Daisy | Planned | – |
| 3. Signed images | Images signed with our own key and checked by MCUboot, in both variants | Nano, Nucleo, Daisy | Planned | – |

## Verification

| What | Board target | How | Result | Commit |
|---|---|---|---|---|
| `StatusLed` pin: physical pin 33 = D26 | Daisy Seed (target not recorded) | Zephyr's blinky on the external LED through `daisy_seed_header` | The external LED blinks | f670cd8 |
| `TempSensor` pin: physical pin 25 = A3 | Daisy Seed (target not recorded) | Zephyr's `adc_dt` sample with P02's overlay, turning the potentiometer | The reading follows the potentiometer, ~0–3300 mV | f670cd8 |
| `serial::Zephyr::readLine()` drops input received before a monitor opens the port | Daisy Seed (target not recorded) | `tio`: a command right after P02 started, after 20 s of blinking with `tio` closed, and with `ECHO` on and no udev rule | Clean first command in all three. Nano: builds, not tested | c0170e9 |

## Measurements

| What | Board target | Value | How measured | Commit |
|---|---|---|---|---|
| P02 image without `std::cout` (with it) | `nucleo_g0b1re` | ~48 KB (~206 KB) | Not recorded | d580a43 |
| P02 image without `std::cout` (with it) | `arduino_nano_esp32/esp32s3/procpu` (Zephyr) | ~141 KB (~432 KB) | Not recorded | d580a43 |
| P02 image without `std::cout` (with it) | Nano ESP32 (ESP-IDF) | ~231 KB (~402 KB) | Not recorded | d580a43 |
| P02 signed image without `std::cout` (with it) | `daisy_seed/stm32h750xx/ram_load` | ~133 KB (~258 KB) | Not recorded | d580a43 |
| P02 in AXI SRAM (code, data and stack) | `daisy_seed/stm32h750xx/ram_load` | ~153 KB of 512 KB | Not recorded | d580a43 |
