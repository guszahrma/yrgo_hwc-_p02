# Report material

Records the report is written from: background, iterations, verification and measurements. Decisions,
terms and lessons are in [`AGENTS.md`](../AGENTS.md) (and the board definitions' own `AGENTS.md`); this file
does not repeat them. New rows go at the bottom of each table.

## Background and requirements

## Iterations

| Iteration | Goal | Boards | State | Branch / PR |
|---|---|---|---|---|

## Verification

| What | Board target | How | Result | Commit |
|---|---|---|---|---|
| Serial input by interrupt: commands sent at once are read correctly | `nucleo_g0b1re` | `status` sent 10 times with no gap between bytes, then 10 times with 5 ms; then `tools/hil/p02_test.py` | 10/10 and 10/10; 11/11 passed | `fix/zephyr-serial-rx-buffer` |
| Serial input by interrupt on a USB serial port (CDC ACM) | `daisy_seed/stm32h750xx/ram_load` | As on the Nucleo, P02 uploaded with `smpmgr` | 10/10 and 10/10; 11/11 passed | `fix/zephyr-serial-rx-buffer` |
| Serial input by interrupt on the Nano's USB-Serial/JTAG | `arduino_nano_esp32/esp32s3/procpu` | Built without sysbuild (as CI), flashed with `west flash`; `tools/hil/p02_test.py --platform Zephyr` under `timeout 120`, then `help` in `tio` | No input taken: the script hung until `timeout` stopped it, `tio` stopped responding; in CI the job ran until GitHub cancelled it after 6 h | 08c904b |
| Serial input polled on the Nano's USB-Serial/JTAG | `arduino_nano_esp32/esp32s3/procpu` | As above, built with `-DCONFIG_UART_INTERRUPT_DRIVEN=n` | 11/11 passed | 08c904b |
| Polling from `zephyr/app/boards/arduino_nano_esp32_procpu.conf`; `p02_test.py` with a write timeout | `arduino_nano_esp32/esp32s3/procpu` | The interrupt build, then a build with the conf file, each tested with the changed `p02_test.py` | `FAIL: P02 takes no input` after 2 s; 11/11 passed | `ci/hardware-test-nucleo` |

## Measurements

| What | Board target | Value | How measured | Commit |
|---|---|---|---|---|
| Shortest gap between received bytes, serial input polled once per loop | `nucleo_g0b1re` | 5 ms: 0/5 commands read correctly; 12 ms and 20 ms: 5/5 | pyserial sends `status\r\n` one byte at a time with a pause, checks the reply for `Blink Period` | 5624358 |
