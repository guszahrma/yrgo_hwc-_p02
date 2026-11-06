# AGENTS.md – Daisy Seed board definition

Context for AI coding assistants changing this board definition: design decisions, how they were tested,
and lessons from the bring-up. People should start with [`doc/index.md`](doc/index.md). The board
definition was brought up for P02, a course project; "P02" below is the app the tests were run with.
Records of how this board definition was tested and measured are kept here, not in the project that
uses it, so they are copied with the board.

## Design decisions

| Decision | Choice | Why |
|---|---|---|
| Supported hardware | Seed 1.2 / Rev7 only; no board revisions | The only version we have and can test. The codec (the only difference between versions) is not in devicetree |
| Two targets | `daisy_seed` (internal flash, no bootloader) and `daisy_seed/stm32h750xx/ram_load` for apps too big for the internal flash; shared `daisy_seed-common.dtsi` with the flash partitions (MCUboot 128 KB internal; QSPI: slot0 1 MB, slot1 1 MB, storage 64 KB) | Bring the board up one layer at a time (LED, USB, MCUboot) with small programs |
| RAM load, not XIP | MCUboot copies the app from QSPI into AXI SRAM (512 KB) and runs it there | The QSPI driver leaves memory-mapped mode to write, so writing settings (NVS in QSPI) would crash a program running from QSPI. Rejected: XIP from QSPI (only without NVS; kept as the fallback). Verified: all P02 commands, including `store` surviving a power cycle, `ram_load`. |
| Flashing | USB only: `dfu-util` via the ROM bootloader (BOOT + RESET) for internal flash; apps via MCUboot over USB with `smpmgr`. `stm32cubeprogrammer` (SWD) listed in `board.cmake` in case a probe turns up | No debug probe. The ROM bootloader cannot write QSPI |
| MCUboot recovery mode | A RESET press alone enters recovery (waits for `smpmgr` as long as needed); power-on and `smpmgr os reset` boot the app at once. `mcuboot_hooks.c` (`CONFIG_BOOT_GO_HOOKS`, built into MCUboot only) reads the reset flags: pin flag set and power-on/brown-out/software/watchdog/low-power/domain flags not set means RESET; it then reports "no bootable image", and MCUboot enters recovery through `CONFIG_BOOT_SERIAL_NO_APPLICATION`. The flags are cleared at every start. Also recovery when QSPI holds no valid app | The Daisy has no button MCUboot can read (BOOT selects the ROM bootloader in hardware). Rejected: MCUboot's own `CONFIG_BOOT_SERIAL_PIN_RESET` (checks only the pin flag, and tested on the board the STM32H7 sets it on every reset – RESET, power-on, `smpmgr os reset` – so MCUboot never booted the app); waiting for `smpmgr` after every reset (`CONFIG_BOOT_SERIAL_WAIT_FOR_DFU`; works, verified with 15 s, but costs 15 s at every start, because usbipd needs 5–15 s to re-attach the board; kept as the fallback). Verified: RESET enters recovery and stays there; power-on, `smpmgr os reset` and the end of `west flash` boot the app at once, `ram_load`. |
| MCUboot build | Sysbuild from the board directory: `Kconfig.sysbuild` makes MCUboot (RAM-load mode, hash check, no signature) the default for `ram_load`; `sysbuild.cmake` builds it for plain `daisy_seed` with `mcuboot.overlay` (RAM = DTCM, code in `boot_partition`) and `mcuboot.conf`. Build: `west build --sysbuild -b daisy_seed/stm32h750xx/ram_load ...` | Every app built for `ram_load` gets the same MCUboot without per-app files. DTCM keeps all of AXI SRAM free for the app. Rejected: a signature with MCUboot's public development key (would protect nothing). |
| MCUboot USB | Zephyr's new USB stack: `BOOT_SERIAL_UART` on the CDC ACM UART (`zephyr,uart-mcumgr`), console off. USB is switched on only when MCUboot enters recovery (`CONFIG_CDC_ACM_SERIAL_ENABLE_AT_BOOT=n`, `mcuboot_status_change()` in `mcuboot_hooks.c`). MCUboot is 61 KB of the 128 KB internal flash. Known issue: `smpmgr image state-read` answers "No images on device!" even with a valid image in slot0 (checked by reading slot0 back). Cause (MCUboot, harmless): in RAM-load mode the image hash is computed over the copy in RAM at the load address (`bootutil_img_hash.c`), and in recovery nothing has been copied there, so the image fails the check and is not listed | One USB stack for MCUboot and apps. Zephyr's own board files do not enable the new stack for MCUboot (`default y if !MCUBOOT`), so this path is less tested. USB only in recovery: switched on at every boot, the PC saw MCUboot connect and vanish again half-way through enumeration when it started the app. Rejected: `BOOT_SERIAL_CDC_ACM` (the legacy stack, deprecated in 4.4.2; kept as the fallback). Verified: with QSPI empty, MCUboot enters recovery and shows up as `2fe3:0004`; MCUboot loads blinky from QSPI into AXI SRAM and starts it; a reset after `west flash` boots the app (not recovery), `ram_load`. |
| QSPI write command | `writeoc = "PP_1_1_4"` (0x32) on `ext_flash_ctrl` | libDaisy uses 0x32. Rejected: Zephyr's default `PP_1_4_4` (0x38, the address on four lines; on the IS25LP064A 0x38 is also 1-1-4, so uploads through MCUboot landed as a few scattered fragments – found by reading slot0 back and comparing; reads and erases were fine). |
| Peripheral reset for apps started by MCUboot | None | Zephyr's USB driver resets the USB core itself (`HAL_PCD_Init`), the QSPI driver sets up the controller from scratch, and MCUboot no longer leaves USB running (USB only in recovery). Rejected: a `board.c` that reset USB OTG FS and QUADSPI at `PRE_KERNEL_1` (removed; it had been added while the real fault was unknown – MCUboot never booted the app, see recovery mode). Verified without it: P02's console, `store` and a power cycle work, `ram_load`. |
| Console | USB CDC ACM on the micro-USB port (Zephyr's new USB stack), via Zephyr's `boards/common/usb/cdc_acm_serial` like the Arduino GIGA R1 (STM32H747). Product name `Daisy Seed` in `Kconfig.defconfig`, so udev can give the port a fixed name | One cable for power, console and uploads. Rejected: the legacy USB stack (deprecated in 4.4.2). Verified: blinky's console, `daisy_seed`. |
| Pin names | Daisy names (`D15`, `A0`) through `daisy_seed_connector.dtsi` (`daisy_seed_header`). Needs our own binding and constants header – upstream has none for the Daisy | Overlays use the board's pin names, not STM32 names. The board prints only some physical pin numbers (1, 10, 21, 32, 40; 1–20 down one side, 21–40 up the other) |
| SDRAM | Not in devicetree yet; added as its own verified step when something needs it | Apps run from internal RAM and store settings in QSPI flash (SDRAM loses its contents at power-off). Untested SDRAM timing gives intermittent faults |

## Bring-up

Zephyr's own blinky (only `led0`, the user LED on PC7). Verified on the board: the LED next to BOOT (PC7)
blinks; the LED next to RESET is the power LED and is always on. With nothing flashed, only the power LED
is lit. The period is 2.00 s (1 s on, 1 s off), which confirms the clock setup (16 MHz crystal, 480 MHz).

## Lessons learned

* **`dfu-util` without `leave`.** With `leave` (Zephyr's default) every flash ended with `FATAL ERROR: command
  exited with status 74`: the ROM bootloader restarts without answering dfu-util's last request. Tried on
  the board: `leave:will-reset` breaks the download after the first 1 KB (it relaxes the wait after every
  block, `LIBUSB_ERROR_PIPE` on `SET_ADDRESS`); detach (`-e`) is not supported ("can't detach"); a USB reset
  (`-R`) hangs under usbipd until the cable is pulled. So `board.cmake` sets `--dfuse-modifiers=` (empty):
  the flash ends cleanly, and RESET starts the program – with MCUboot straight into recovery, ready for
  the app upload. `Warning: Invalid DFU suffix signature` is harmless (`zephyr.bin` has no DFU suffix).
* **Backup.** Byte 1 of `option_bytes.bin` is RDP: `0xAA` = no read protection (level 0); anything else and
  the read fails. Our board's QSPI held one 64 KB block of `0xDEADBEEF` at 0x020000 (probably a factory
  test), read with a small Zephyr program (twice, with a CRC-32) and backed up outside the repository.
* **USB console in WSL.** Run the monitor (`tio`) and `smpmgr` in WSL, not PowerShell: Windows has no Python,
  and its "Python was not found" is only a Microsoft Store shortcut. With another board attached, the Daisy
  may be `/dev/ttyACM1` – the fixed name (`/dev/tty_daisy_seed`) avoids guessing.
* **`Frame could not be parsed`.** smpmgr's request was echoed back (frame `08 00 00 01 00 00 00 06 a0`): an
  app with a console is running, not recovery mode.
* **Debugging without a probe or console.** Two LEDs driven through raw registers at fixed init levels
  (`SYS_INIT` at `PRE_KERNEL_1` and `APPLICATION`, plus `k_sys_fatal_error_handler`) showed how far an app
  got. Reading a QSPI range back over USB and comparing it with the uploaded file found the write bug.
  Check first whether MCUboot is still in recovery (`smpmgr image state-read` answers) before suspecting
  the app.
