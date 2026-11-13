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

| Iteration | Goal | Boards | State | Commit |
|---|---|---|---|---|
| Hardware test in CI | On every push, flash the P02 that CI built onto a board on a self-hosted runner and check its replies to serial commands | Nano ESP32 (ESP-IDF) | Merged 2026-11-09 | b26b038 |
| Hardware tests on more boards and platforms | The same for P02 under Zephyr on the Nano and on the Nucleo-G0B1RE, one runner per board, MCUboot and P02 flashed together | Nano ESP32 (ESP-IDF and Zephyr), Nucleo-G0B1RE | Passed in CI with MCUboot (run #337) | 89e0599 |
| Prototype: MCUboot on the boards | P02 boots through MCUboot on every Zephyr board (built with `--sysbuild`), no signature yet | Nano, Nucleo, Daisy | Merged 2026-11-09 | bb17d47 |
| Prototype: firmware update by hand | v2 signed with `west sign --pad`, written to slot 1, started as a test, reverted at the next reset | Nano, Nucleo | Verified on both boards (2026-11-05), not merged yet (`feature/mcuboot-update-demo`) | 371c8b5 |
| Generalised for Zephyr | Firmware updates in both variants: swap (Nano, Nucleo) and RAM load (Daisy) | Nano, Nucleo, Daisy | Planned | – |
| Signed images | Images signed with our own key and checked by MCUboot, in both variants | Nano, Nucleo, Daisy | Planned | – |

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
| `hardware-test-nucleo` in CI with MCUboot: OpenOCD programs MCUboot's `zephyr.hex` and P02's `zephyr.signed.hex`, then `tools/hil/p02_test.py` | `nucleo_g0b1re` (with MCUboot) | Push of `ci/hardware-test-nucleo`, CI run #337 (2026-11-10) | The job passed in 16 s | 89e0599 |
| `hardware-test-nano` and `hardware-test-nano-zephyr` in CI, the Zephyr job with MCUboot at 0x0 and P02 at 0x20000 | Nano ESP32 (ESP-IDF) and `arduino_nano_esp32/esp32s3/procpu` (with MCUboot) | Push of `ci/hardware-test-nucleo`, CI run #337 (2026-11-10) | Both jobs passed, in 27 s (ESP-IDF) and 24 s (Zephyr) | 89e0599 |
| Runner guard starts a stopped runner whose port is there | `nucleo_g0b1re` (runner on QR-PF2KTLE6) | Runner's service stopped with `systemctl stop`, guard run by hand | The guard started the service; `systemctl is-active` gave `active` | a955a31 |
| Runner guard stops a runner whose port is gone, and starts it again | Daisy Seed (runner on QR-PF2KTLE6) | USB cable pulled ~11:34, guard on its timer (2026-11-11) | Stopped at 11:36:37 (`gone for 120 s`); started again within 30 s of the port coming back | a955a31 |
| Runner guard's job check (`pgrep -f "^<runner folder>/bin/Runner.Worker"`) | Nano ESP32 (runner on QR-PF2KTLE6) | `pgrep -af Runner.Worker` while a job ran | `/home/ghrunner/runners/arduino_nano_esp32/bin/Runner.Worker spawnclient ...`, which the pattern matches | a955a31 |
| `ghrunner` can open the Nano's port with `GROUP="dialout"` in `99-p02.rules` (Ubuntu's `openocd` package gave it `plugdev`) | Nano ESP32 (ESP-IDF, runner on QR-PF2KTLE6) | `hardware-test-nano` in CI, before and after installing the rule (2026-11-11) | Before: esptool `Permission denied`; after: `/dev/ttyACM0` in `dialout`, and esptool opened the port | a955a31 |
| `p02-runners.ps1` attaches by USB ID: Nucleo `0483:374b`, Nano `303a:1001` (Daisy by bus ID 4-4) | Nucleo-G0B1RE, Nano ESP32, Daisy Seed (QR-PF2KTLE6) | Old bus-ID loops stopped, new loops started by hand; `usbipd list` (2026-11-13) | All three `Attached`, with `/dev/tty_nucleo_g0b1re`, `/dev/tty_arduino_nano_esp32` and `/dev/tty_daisy_seed`. Before: the script attached the Nano on 3-3 while it was on 2-3 | – |
| A board attached by USB ID or by bus ID comes back after a detach | Nano ESP32 (USB ID), Daisy Seed (bus ID 4-4), QR-PF2KTLE6 | `usbipd detach --hardware-id 303a:1001`, then `usbipd detach --busid 4-4` (2026-11-13) | Both `Attached` again within a few seconds, with a new port symlink | – |
| Update demo: v2 starts as a test, and the next reset reverts to v1 (swap using move) | `nucleo_g0b1re` (with MCUboot) | v1 flashed; v2 (2.0.0) built in its own directory, `west sign --pad`, written to slot 1 (0x0803E000) with OpenOCD; RESET twice, `tio` open (2026-11-05) | First reset: `Swap type: test`, `Starting swap using move algorithm.`, `Image version: v2.0.0`. Second reset: `Swap type: revert`, `Image version: v1.0.0`; the settings in `storage_partition` kept | 371c8b5 |
| Update demo: v2 starts as a test, and the next reset reverts to v1 (swap using move) | `arduino_nano_esp32/esp32s3/procpu` (with MCUboot) | As on the Nucleo, v2 written to slot 1 (0x170000) with esptool; MCUboot's output is not seen on the Nano, so slot 0's image header read with `esptool read-flash 0x20000 32` after each reset (2026-11-05) | Image magic `0x96f3b83d`; slot 0 `v2.0.0` after the first reset, `v1.0.0` after the second | 371c8b5 |

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
