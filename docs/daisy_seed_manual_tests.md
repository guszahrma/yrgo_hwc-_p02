# Daisy Seed: manual tests

Tests that need the board, for work that is committed but **not yet tested on hardware**. Run them in
order; each says which commit it checks. Write the result in the table at the end.

All commands run from the repository root, in WSL, with the Zephyr environment:

```bash
source tools/zephyr_env.sh
```

Before you start:

* The board is connected with a data cable, and the PowerShell window with
  `usbipd attach --wsl --busid <id> --auto-attach` is running (see [setup](setup.md)).
* The LED on pin 33 and the pot on pin 25 are wired as in `zephyr/app/boards/daisy_seed_ram_load.overlay`.
* Only the Daisy is attached, so it is `/dev/ttyACM0` (`ls /dev/ttyACM*`).

## 1. Build P02 with the new MCUboot

```bash
west build -p always --sysbuild -b daisy_seed/stm32h750xx/ram_load -d zephyr/app/daisy_seed_build zephyr/app -- -DBOARD_ROOT=$P02_ROOT/zephyr
```

Expected: the build ends without errors; MCUboot is about 61 KB (`FLASH:` line of the `mcuboot` image).

## 2. Flash MCUboot (checks commit a6a09c9, the RESET hook)

Hold BOOT, press and release RESET, release BOOT. Then:

```bash
west flash -d zephyr/app/daisy_seed_build --domain mcuboot
```

The flash ends with the harmless `dfu-util: Error during download get_status` / `FATAL ERROR ...
status 74`; it worked if `File downloaded successfully` is printed. If `lsusb | grep -v -i hub` still
shows `0483:df11` after a few seconds, press RESET alone.

Expected, either of:

* **P02 starts** (the P02 from Friday is still in the QSPI flash): the console answers within a few
  seconds of usbipd attaching the board. Note it in the table.
* **MCUboot is in recovery** (the reset flags still showed the BOOT + RESET press):
  `smpmgr --port /dev/ttyACM0 image state-read` answers (with "No images on device!" – a known quirk).
  Then `smpmgr --port /dev/ttyACM0 os reset` should start P02 at once. Note it in the table.

## 3. Power-on starts P02 at once

Unplug the USB cable, wait 2 s, plug it back in.

Expected: P02 starts within about 2 s – no 15 s wait. If blinking was stored with `store`, the LED on
pin 33 blinks without any command. Otherwise, check that the console answers `help` once
`/dev/ttyACM0` is back.

## 4. RESET enters recovery, and stays there

Press RESET (alone). Then:

```bash
sleep 2; until [ -e /dev/ttyACM0 ]; do sleep 0.5; done
smpmgr --port /dev/ttyACM0 image state-read
```

Expected: the LED on pin 33 stops; `state-read` answers (with "No images on device!"). Wait one minute
and run `state-read` again: it still answers – recovery has no timeout.

## 5. Upload P02, `os reset` starts it at once

Still in recovery from test 4:

```bash
smpmgr --port /dev/ttyACM0 --timeout 30 image upload zephyr/app/daisy_seed_build/app/zephyr/zephyr.signed.bin
smpmgr --port /dev/ttyACM0 os reset
```

Expected: P02 starts within about 2 s. Open the console and check `help`, `blink on`, `period 500`,
`store`; then a power cycle (test 3) keeps the stored settings.

```bash
python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
```

## 6. Is `board.c` needed? (checks commit 044480f)

`board.c` resets USB and QSPI before an app's drivers start, as a precaution. Build P02 without it:

```bash
sed -i 's/^zephyr_library_sources_ifdef(CONFIG_BOOTLOADER_MCUBOOT board.c)/#&/' zephyr/boards/electrosmith/daisy_seed/CMakeLists.txt
west build -p always --sysbuild -b daisy_seed/stm32h750xx/ram_load -d zephyr/app/daisy_seed_build zephyr/app -- -DBOARD_ROOT=$P02_ROOT/zephyr
git checkout zephyr/boards/electrosmith/daisy_seed/CMakeLists.txt
```

Press RESET, upload and start it as in tests 4 and 5. Then check the console (`help`), `blink on`,
`store` and a power cycle.

* Everything works: `board.c` is not needed and can be removed.
* Something fails (no console, hangs on `store`): it is needed; note what failed.

Afterwards, build and upload the normal P02 again (tests 1, 4 and 5).

## 7. QSPI chip ID with 0 dummy cycles (an open quirk, nothing committed yet)

The chip's JEDEC ID reads back rotated (`17 9d 60` instead of `9d 60 17`). Likely cause: the driver
waits 8 dummy cycles before reading the ID (`st,read-id-dummy-cycles`, default 8), but the IS25LP064A
sends it at once. This test reads the ID with Zephyr's `jesd216` sample, first as the board is, then with
0 dummy cycles. It flashes the plain target, which **replaces MCUboot** – flash MCUboot again afterwards.

```bash
printf '&ext_flash_ctrl {\n\tst,read-id-dummy-cycles = <0>;\n};\n' > ~/daisy_id0.overlay
west build -p always -b daisy_seed -s deps/zephyr/samples/drivers/jesd216 -d jesd216_build -- -DBOARD_ROOT=$P02_ROOT/zephyr
west flash -d jesd216_build        # BOOT + RESET first
python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
west build -p always -b daisy_seed -s deps/zephyr/samples/drivers/jesd216 -d jesd216_build -- -DBOARD_ROOT=$P02_ROOT/zephyr -DEXTRA_DTC_OVERLAY_FILE=$HOME/daisy_id0.overlay
west flash -d jesd216_build        # BOOT + RESET first
python -m serial.tools.miniterm --exit-char 24 /dev/ttyACM0 115200
```

Note the `jedec-id` line of each run. Expected: rotated first, `9d 60 17` with 0 dummy cycles. If so,
`st,read-id-dummy-cycles = <0>;` goes into `daisy_seed-common.dtsi`. Then flash MCUboot again (test 2) –
P02 is still in the QSPI flash.

## If the RESET hook does not work

* **RESET boots P02** (test 4 fails), or **power-on or `os reset` enters recovery** (tests 3 or 5
  fail): the reset flags do not behave as the reference manual says. Note which test failed. Fallback:
  `git revert a6a09c9` gives back the 15 s wait after every reset (verified on the board).

## Results

| Test | Date | Result | Notes |
|---|---|---|---|
| 1. Build | | | |
| 2. Flash MCUboot | | | P02 started / recovery? |
| 3. Power-on starts P02 at once | | | |
| 4. RESET enters recovery, no timeout | | | |
| 5. Upload, `os reset` starts P02 at once | | | |
| 6. P02 without `board.c` | | | needed / not needed |
| 7. JEDEC ID, as is / 0 dummy cycles | | | |
