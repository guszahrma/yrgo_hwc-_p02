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
| MCUboot starts P02, and the watchdog restarts P02 through MCUboot | `arduino_nano_esp32/esp32s3/procpu` (with MCUboot) | `west flash`, then `status` and `hang` over the serial port | MCUboot loads slot 0 and starts P02; `status` prints the board, board target and platform. After `hang`: `rst:0x7 (TG0WDT_SYS_RST)`, MCUboot starts P02 again, and P02 prints `Last reset was caused by the watchdog`. Saved settings kept | b59f002 |
| Which boot output `tio` shows after each kind of restart | `arduino_nano_esp32/esp32s3/procpu` (with MCUboot) | `west flash … && tio …`; `hang` with `tio` open; RESET with `tio` open | After `west flash`: P02's output from `*** Booting Zephyr OS` on, not MCUboot's. After `hang`: everything from `ESP-ROM` on, MCUboot's included, without a disconnect. After RESET: `tio` disconnected, reconnected ~6 s later, and showed no boot output | b59f002 |
| Serial input by interrupt: commands sent at once are read correctly | `nucleo_g0b1re` | `status` sent 10 times with no gap between bytes, then 10 times with 5 ms; then `tools/hil/p02_test.py` | 10/10 and 10/10; 11/11 passed | 964a67d |
| Serial input by interrupt on a USB serial port (CDC ACM) | `daisy_seed/stm32h750xx/ram_load` | As on the Nucleo, P02 uploaded with `smpmgr` | 10/10 and 10/10; 11/11 passed | 964a67d |
| Serial input by interrupt on the Nano's USB-Serial/JTAG | `arduino_nano_esp32/esp32s3/procpu` | Built without sysbuild (as CI), flashed with `west flash`; `tools/hil/p02_test.py --platform Zephyr` under `timeout 120`, then `help` in `tio` | No input taken: the script hung until `timeout` stopped it, `tio` stopped responding; in CI the job ran until GitHub cancelled it after 6 h | 08c904b |
| Serial input polled on the Nano's USB-Serial/JTAG | `arduino_nano_esp32/esp32s3/procpu` | As above, built with `-DCONFIG_UART_INTERRUPT_DRIVEN=n` | 11/11 passed | 08c904b |
| Polling from `zephyr/app/boards/arduino_nano_esp32_procpu.conf`; `p02_test.py` with a write timeout | `arduino_nano_esp32/esp32s3/procpu` | The interrupt build, then a build with the conf file, each tested with the changed `p02_test.py` | `FAIL: P02 takes no input` after 2 s; 11/11 passed | d38b7d5 |
| `StatusLed` on D6 = PB14 | `nucleo_g0b1re` (with MCUboot) | `on`, `off`, then `period 500` and `blink on` over the serial port, watching the external LED | The LED lights, goes out, then blinks once a second; P02 prints `Wrote active to pin 14` | c313f55 |
| `TempSensor` on A0 = PA0 | `nucleo_g0b1re` (with MCUboot) | `status` once a second for 20 s while turning the input from one end to the other and back | The temperature follows the input from 280 °C to −50 °C and back, i.e. ~3300 mV to 0 mV | c313f55 |
| `store` keeps the settings after RESET | `nucleo_g0b1re` (with MCUboot) | `blink on`, `period 500`, `store`; then `blink off`, `period 1000` without `store`; RESET (B2) | MCUboot starts P02, which loads `Blink State: on` and `Blink Period: 500 ms`; no watchdog reset cause printed | c313f55 |
| `hang` restarts P02 through MCUboot | `nucleo_g0b1re` (with MCUboot) | `hang` over the serial port, reading the output with timestamps | MCUboot starts ~3 s after `hang` (see Measurements); P02 prints `Last reset was caused by the watchdog` and loads the saved settings | c313f55 |
| `hardware-test-nucleo` in CI: flashes P02 with OpenOCD on the self-hosted runner and runs `tools/hil/p02_test.py` | `nucleo_g0b1re` (without MCUboot) | Push of `ci/hardware-test-nucleo`, CI run #331 (re-run 2026-11-10) | The run passed; the job took 17 s | d38b7d5 |
| `hardware-test-nano` and `hardware-test-nano-zephyr` in CI: both platforms flashed and tested in turn on the Nano's one runner | Nano ESP32 (ESP-IDF) and `arduino_nano_esp32/esp32s3/procpu` (Zephyr, without MCUboot) | Push of `ci/hardware-test-nucleo`, CI run #331 (re-run 2026-11-10) | The run passed; each job took 28 s | d38b7d5 |

## Measurements

| What | Board target | Value | How measured | Commit |
|---|---|---|---|---|
| P02 image without `std::cout` (with it) | `nucleo_g0b1re` | ~48 KB (~206 KB) | Not recorded | d580a43 |
| P02 image without `std::cout` (with it) | `arduino_nano_esp32/esp32s3/procpu` (Zephyr) | ~141 KB (~432 KB) | Not recorded | d580a43 |
| P02 image without `std::cout` (with it) | Nano ESP32 (ESP-IDF) | ~231 KB (~402 KB) | Not recorded | d580a43 |
| P02 signed image without `std::cout` (with it) | `daisy_seed/stm32h750xx/ram_load` | ~133 KB (~258 KB) | Not recorded | d580a43 |
| P02 in AXI SRAM (code, data and stack) | `daisy_seed/stm32h750xx/ram_load` | ~153 KB of 512 KB | Not recorded | d580a43 |
| MCUboot image (in the 48 KB `boot_partition`) | `nucleo_g0b1re` | ~26 KB | Not recorded | b3680ee |
| Shortest gap between received bytes, serial input polled once per loop | `nucleo_g0b1re` | 5 ms: 0/5 commands read correctly; 12 ms and 20 ms: 5/5 | pyserial sends `status\r\n` one byte at a time with a pause, checks the reply for `Blink Period` | 5624358 |
| Time from `hang` to the watchdog restart (timeout 3000 ms) | `nucleo_g0b1re` (with MCUboot) | 2.9 s | pyserial sends `hang` and timestamps each line it reads; time to MCUboot's first line (`*** Booting MCUboot`) | c313f55 |
| MCUboot image (written from 0x0) | `arduino_nano_esp32/esp32s3/procpu` | ~37 KB (37,344 bytes, up to 0x91E0) | Size of `mcuboot/zephyr/zephyr.bin` from `west build --sysbuild` | c313f55 |
