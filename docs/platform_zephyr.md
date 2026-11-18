# Zephyr environment

P02 on **Zephyr RTOS v4.4.2**: a port of the ESP-IDF build. The Zephyr drivers are in `main/` next to the
ESP-IDF ones (`driver/<name>/zephyr.h/.cpp`), and `zephyr/app/src/main.cpp` runs the unchanged
`logic::Logic` with `factory::Zephyr`. Wi-Fi and MQTT are not ported yet. Common setup (WSL, usbipd,
cloning) is in [Setup](setup.md).

## Setup

### 1. Install Zephyr (once)

The Zephyr version is pinned in [`west.yml`](../west.yml) (the west manifest), and the repository itself is
the west workspace: Zephyr and the modules it needs are fetched into `deps/` inside the repository
(ignored by git).

From the **repository root** (~800 MB, a few minutes):

```bash
tools/zephyr_setup.sh
```

The script creates `.venv/` (west and Zephyr's Python tools), `.west/config` and `deps/` (Zephyr and
`hal_espressif`), installs the Zephyr SDK with the ESP32-S3 compiler in `~/zephyr-sdk-<version>` if it is
not there yet, and fetches Espressif's binary blobs. Run it again after `west.yml` has changed (e.g. a new
Zephyr version) – it only updates what is needed.

### 2. Activate the environment (every new terminal)

```bash
source tools/zephyr_env.sh
```

It activates `.venv`, sets `ZEPHYR_BASE` to `deps/zephyr` and exports `P02_ROOT` (the repository root),
which commands and scripts can use instead of a hard-coded path.

Optionally add an alias. Run this from the repository root: `$(pwd)` is replaced with the repository's
path when the line is written to `~/.bashrc`, so the alias works from any directory:

```bash
echo "alias get_zephyr='source $(pwd)/tools/zephyr_env.sh'" >> ~/.bashrc
```

## Building and flashing

### P02 (`zephyr/app/`)

From the **repository root** (attach the board first, see [Setup](setup.md#4-attach-the-board-to-wsl-powershell-as-administrator)):

```bash
source tools/zephyr_env.sh
west build -p always --sysbuild -b arduino_nano_esp32/esp32s3/procpu -s zephyr/app -d zephyr/app/build/arduino_nano_esp32 -- -DBOARD_ROOT=$P02_ROOT/zephyr
west flash -d zephyr/app/build/arduino_nano_esp32 --esp-device /dev/tty_arduino_nano_esp32 && tio /dev/tty_arduino_nano_esp32
```

`-s` is the app and `-d` its build directory, one per board (`zephyr/app/build/<board>`). Always give `-d`:
without it, west builds in `./build`, which belongs to ESP-IDF.

**P02 boots through MCUboot** on every Zephyr board. `--sysbuild` builds MCUboot and P02 together
(`zephyr/app/sysbuild.conf`), and `west flash` writes both: MCUboot at the start of the flash, P02 in
`slot0_partition`. `-DBOARD_ROOT` is needed on the command line for our own boards (Nano, Daisy): sysbuild
looks up the board before the app's `CMakeLists.txt` runs. There is no signature yet, only a hash check.
On the Nano and the Nucleo, MCUboot runs in swap using offset (`zephyr/app/Kconfig.sysbuild`), so it can
roll an update back: see the [update demo](#update-demo).

**The first time on a Nano** that ran ESP-IDF or Zephyr without MCUboot, erase the whole flash once
(`esptool --chip esp32s3 -p /dev/tty_arduino_nano_esp32 erase-flash`, see below): the old program at the
start of the flash has another format.

On the Nano, the monitor does not show MCUboot's own output (`MCUboot 2nd stage bootloader`) right after
`west flash`: it is printed before the monitor has opened the port. P02 starting at all shows that MCUboot
started it. After a restart with the monitor already open, such as the watchdog after `hang`, MCUboot's
output is shown.

The monitor is `tio`: it waits until the port is there and reconnects after every reset, so it can stay
open. Exit with `Ctrl+T`, `q` – and quit it before the next `west flash`, which needs the port.

Then use the [serial commands](usage.md#serial-commands).

The other boards: [Nucleo-G0B1RE → Building and flashing](p02_nucleo_g0b1re.md#building-and-flashing)
(OpenOCD) and [Daisy Seed → Building and flashing](p02_daisy_seed.md#building-and-flashing) (MCUboot's
recovery mode and `smpmgr`).

### Update demo

Shows what MCUboot does with a new version, on the **Nucleo-G0B1RE and the Nano**: it starts v2 once as a
test, and since v2 does not confirm itself, it goes back to v1 at the next reset. On these boards MCUboot
runs in **swap using offset** (`zephyr/app/Kconfig.sysbuild`): the new image is written **one sector into**
`slot1_partition`, and the first sector of slot 1 is the free one that MCUboot moves the sectors into during
the swap. The old version is kept in slot 1 and can come back. The Daisy keeps RAM load, which has no
rollback here.

The steps are done by hand for now: v2 is marked "test me" with `west sign --pad` and written to slot 1 with
OpenOCD or esptool. With signing and updates over MCUmgr/SMP, the build signs the image, and P02 itself
writes slot 1 and asks MCUboot for the test (`boot_request_upgrade()`), so these steps go away. P02 never
confirms an image yet (`boot_write_img_confirmed()`), so every test ends in a revert.

P02's version is in `zephyr/app/VERSION` (1.0.0) and goes into the image header when the image is signed.

| Board | `slot0_partition` | `slot1_partition` | Sector | v2 written to | `--slot-size` for `--pad` |
|---|---|---|---|---|---|
| Nucleo-G0B1RE | 0x0800C000 | 0x0803E000 | 2 KB | **0x0803E800** | **0x31800** |
| Nano ESP32 | 0x20000 | 0x170000 | 4 KB | **0x171000** | **0x14F000** |

The addresses are the `reg` of the partitions in `zephyr/app/build/<board>/app/zephyr/zephyr.dts` (on the
Nucleo plus the flash's start, 0x08000000), and the sector is the flash's `erase-block-size` in the same file.
v2 is written at slot 1 plus one sector, and is padded to the slot minus one sector, so that its trailer ends
exactly at the end of slot 1. Padded to the whole slot, the trailer would land one sector after slot 1 (on the
Nucleo in `storage_partition`, where the settings are).

**1. Erase the slots and flash v1.** Erase once if MCUboot ran in another mode before (swap using move,
overwrite only, or ESP-IDF on the Nano): the trailers at the end of the slots have another layout.

```bash
# Nucleo: slot 0 and slot 1 (0x0800C000-0x08070000); storage_partition is kept
OPENOCD=~/zephyr-sdk-1.0.1/hosttools/sysroots/x86_64-pokysdk-linux/usr
$OPENOCD/bin/openocd -s deps/zephyr/boards/st/nucleo_g0b1re/support -s $OPENOCD/share/openocd/scripts \
  -f deps/zephyr/boards/st/nucleo_g0b1re/support/openocd.cfg \
  -c "init" -c "reset halt" -c "flash erase_address 0x0800C000 0x64000" -c "shutdown"
# Nano: the whole flash, the settings included
esptool --chip esp32s3 -p /dev/tty_arduino_nano_esp32 erase-flash
```

Then flash v1 as usual: [above](#p02-zephyrapp) for the Nano, [Nucleo-G0B1RE → Building and
flashing](p02_nucleo_g0b1re.md#building-and-flashing) for the Nucleo. `grep MCUBOOT_MODE
zephyr/app/build/<board>/zephyr/.config` shows `SB_CONFIG_MCUBOOT_MODE_SWAP_USING_OFFSET=y`.

**2. Build v2** in its own build directory, with version 2.0.0 in the image header. The `app_` prefix gives
the option to P02's build only, not MCUboot's:

```bash
# Nucleo
west build -p always --sysbuild -b nucleo_g0b1re -s zephyr/app -d zephyr/app/build/nucleo_g0b1re_v2 -- -Dapp_CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\"2.0.0\"
# Nano
west build -p always --sysbuild -b arduino_nano_esp32/esp32s3/procpu -s zephyr/app -d zephyr/app/build/arduino_nano_esp32_v2 -- -DBOARD_ROOT=$P02_ROOT/zephyr -Dapp_CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\"2.0.0\"
```

**3. Mark v2 for a test.** `--pad` fills the image up to `--slot-size` and writes the trailer's boot magic at
the end, which tells MCUboot that slot 1 holds an update to test. `west sign` passes the whole slot as
`--slot-size` itself; the one after `--` comes last and wins (`<v2>` is the build directory from step 2,
`<size>` from the table):

```bash
west sign -d <v2>/app -t imgtool -- --pad --version 2.0.0 --slot-size <size>
stat -c %s <v2>/app/zephyr/zephyr.signed.bin
python3 deps/bootloader/mcuboot/scripts/imgtool.py dumpinfo <v2>/app/zephyr/zephyr.signed.bin
```

`stat` must give `<size>` (202752 on the Nucleo, 1372160 on the Nano); with the whole slot, `--slot-size`
did not take effect. `dumpinfo` should show `version: 2.0.0+0` in the header and the bytes of `boot magic`
in the trailer.

**4. Write v2 one sector into slot 1.** Nucleo, with the Zephyr SDK's OpenOCD:

```bash
$OPENOCD/bin/openocd -s deps/zephyr/boards/st/nucleo_g0b1re/support -s $OPENOCD/share/openocd/scripts \
  -f deps/zephyr/boards/st/nucleo_g0b1re/support/openocd.cfg \
  -c "init" -c "reset halt" \
  -c "flash write_image erase zephyr/app/build/nucleo_g0b1re_v2/app/zephyr/zephyr.signed.bin 0x0803E800" \
  -c "reset run" -c "shutdown"
```

OpenOCD resets the board at the end (`reset run`), so open `tio` before step 4 to see MCUboot's output. The
Nucleo stays on USB through a reset and is flashed over SWD, so `tio` can stay open the whole time.

Nano:

```bash
esptool --chip esp32s3 -p /dev/tty_arduino_nano_esp32 --after no-reset write-flash 0x171000 zephyr/app/build/arduino_nano_esp32_v2/app/zephyr/zephyr.signed.bin
```

`--after no-reset` on every esptool command: esptool otherwise resets the chip when it is done, and every
reset runs MCUboot. With it, the chip stays in its ROM bootloader until you press RESET, so every press is
one run of MCUboot. esptool and `tio` use the same port on the Nano: quit `tio` before every esptool command.

**5. Reset: v2 starts as a test.** Wait a few seconds after the reset: MCUboot swaps the images first. On the
Nucleo, `tio` shows `Secondary image: magic=good`, `Swap type: test`, `Starting swap using offset algorithm.`
and `Image version: v2.0.0`. After the swap, v1 is at the **start** of slot 1 (no longer one sector in), and
slot 1 plus one sector holds no image header, only v1's code.

**6. Reset again: back to v1.** v2 did not confirm itself, so MCUboot swaps back: `Swap type: revert` and
`Image version: v1.0.0`. The settings in `storage_partition` are kept.

**On the Nano,** MCUboot's output is not seen (see above), so read the image headers with esptool before
step 5, after step 5 and after step 6: slot 0 (0x20000), the start of slot 1 (0x170000) and slot 1 plus one
sector (0x171000):

```bash
for a in 0x20000 0x170000 0x171000; do
  esptool --chip esp32s3 -p /dev/tty_arduino_nano_esp32 --after no-reset read-flash $a 32 /tmp/hdr.bin > /dev/null &&
  python3 -c 'import struct,sys; d=open("/tmp/hdr.bin","rb").read(); m=struct.unpack_from("<I",d,0)[0]; print(sys.argv[1], hex(m), ("v%d.%d.%d" % struct.unpack_from("<BBH",d,20)) if m==0x96f3b83d else "(no image)")' $a
done
```

`0x96f3b83d` is MCUboot's image magic (a valid image header), followed by the version. Before step 5: v1 in
slot 0, v2 at slot 1 plus one sector. After step 5: v2 in slot 0, v1 at the start of slot 1. After step 6:
v1 in slot 0. On the Nucleo, OpenOCD's `mdw` reads the same words (the magic at the start of a header, the
version at `+0x14`) while P02 runs: `-c "halt" -c "mdw 0x0800C000" -c "mdw 0x0800C014" ... -c "resume"`.

### Our samples (`zephyr/samples/`)

Our apps find the board through `list(APPEND BOARD_ROOT ...)` in their own `CMakeLists.txt` and set
`CONFIG_BOOT_DELAY` in `prj.conf`, so no extra flags are needed:

```bash
west build -p always -b arduino_nano_esp32/esp32s3/procpu -s zephyr/samples/blinky -d zephyr/samples/blinky/build
west flash -d zephyr/samples/blinky/build --esp-device /dev/tty_arduino_nano_esp32 && tio /dev/tty_arduino_nano_esp32
```

Expected: the yellow LED blinks and the RGB LED cycles red → green → blue, with one line per second:

```
Yellow: OFF  RGB: green
Yellow: ON   RGB: blue
```

### Zephyr's samples

Zephyr's own samples don't know about our board, so `BOARD_ROOT` is passed on the command line.
Example with `hello_world`:

```bash
west build -p always -b arduino_nano_esp32/esp32s3/procpu -s $ZEPHYR_BASE/samples/hello_world -d hello_world_build -- \
  -DBOARD_ROOT=$P02_ROOT/zephyr \
  -DCONFIG_BOOT_DELAY=1000
west flash -d hello_world_build --esp-device /dev/tty_arduino_nano_esp32 && tio /dev/tty_arduino_nano_esp32
```

* `-p always` – pristine build (start from scratch)
* `BOARD_ROOT` – the directory that contains `boards/`, i.e. the repository's `zephyr/` directory
* `CONFIG_BOOT_DELAY=1000` – waits 1 s before booting, see below

Expected output:

```
*** Booting Zephyr OS build v4.4.2 (delayed boot 1000ms) ***
Hello World! arduino_nano_esp32/esp32s3/procpu
```

**Why `BOOT_DELAY`?** The console runs over the ESP32-S3's built-in USB, and the monitor can only show
what is printed after it has opened `/dev/tty_arduino_nano_esp32`. The 1 s boot delay keeps P02's boot
output until the monitor is open after `west flash`. If the boot output is missing on your computer,
increase `CONFIG_BOOT_DELAY` in the app's `prj.conf` (e.g. to `3000`). The RESET button disconnects USB,
and usbipd needs a few seconds to re-attach the board, so the boot output after RESET is lost; a watchdog
restart keeps USB connected.

### Useful files and commands

| File / command | Contents |
|---|---|
| `west build -d zephyr/app/build/<board> -t menuconfig` | Interactive Kconfig browser for P02 (`/` to search, `?` for help); `-t mcuboot_menuconfig` for MCUboot, `-t sysbuild_menuconfig` for sysbuild |
| `zephyr/app/build/<board>/app/zephyr/.config` | All active Kconfig options of P02 (`mcuboot/zephyr/.config` for MCUboot) |
| `zephyr/app/build/<board>/app/zephyr/zephyr.dts` | The final devicetree (board + overlays), with the source file of every line |
| `zephyr/app/build/<board>/app/zephyr/zephyr.signed.bin`, `zephyr.elf` | P02 as MCUboot loads it (with its image header), ELF with symbols |
| `zephyr/app/build/<board>/mcuboot/zephyr/zephyr.bin` | MCUboot |
| `zephyr/app/sysbuild.conf`, `zephyr/app/Kconfig.sysbuild` | Sysbuild settings: for every board (MCUboot, no signature), and per board (the MCUboot mode) |
| `zephyr/app/VERSION` | P02's version, written into the image header when it is signed |
| `esptool --chip esp32s3 -p /dev/tty_arduino_nano_esp32 erase-flash` | Nano ESP32: erase the whole flash, including saved settings – e.g. after [switching from ESP-IDF](usage.md#switching-between-esp-idf-and-zephyr) |

## Drivers

| Driver | Zephyr API | Configured from |
|---|---|---|
| `timer::Zephyr` | `k_uptime_get()`, polled | – |
| `gpio::Zephyr` | `gpio_dt_spec`, `gpio_pin_*_dt` | The board's overlay: alias `p02-led` (D6 on the Nano) |
| `adc::Zephyr` (+ `Tmp36`) | `adc_dt_spec`, `adc_read_dt`, average of 5 samples | The board's overlay: `zephyr,user` `io-channels` (A6 = ADC2 ch 2 on the Nano) |
| `serial::Zephyr` | `uart_poll_in/out` on the console UART (USB) | `zephyr,console` |
| `nvs::Zephyr` | Settings subsystem, NVS backend | `storage_partition` |
| `watchdog::Zephyr` | `wdt_install_timeout`, `wdt_setup`, `wdt_feed`; `hwinfo_get_reset_cause()` | The board's alias `watchdog0` (`wdt0` on the Nano, the IWDG on the Daisy Seed); timeout from `Logic` (3 s) |

Where things are:

| Path | Contents |
|---|---|
| `main/include/driver/<name>/zephyr.h`, `main/source/driver/<name>/zephyr.cpp` | The drivers, next to the ESP32 and stub versions |
| `main/source/driver/factory/zephyr.cpp` | Maps the roles (`GpioId::StatusLed`, ...) to devicetree |
| `zephyr/app/` | The app: `CMakeLists.txt` (lists the sources from `main/`), `prj.conf`, `boards/<board>.overlay` (one per board: the wiring), `main.cpp` |
| `zephyr/boards/arduino/nano_esp32/` | Our board definition (Zephyr has none for this board), see [Arduino Nano ESP32](../zephyr/boards/arduino/nano_esp32/doc/index.md) |
| `zephyr/samples/blinky/` | First exercise: LEDs from devicetree |
| `west.yml`, `tools/zephyr_setup.sh`, `tools/zephyr_env.sh` | The pinned Zephyr workspace |
