# Setup

Setting up a computer for the project: WSL, packages, cloning and attaching the board. This is the same for
both platforms; after it, continue with [ESP-IDF environment](platform_esp_idf.md) or
[Zephyr environment](platform_zephyr.md).

The recommended environment is **Ubuntu 24.04 in WSL2**. The USB board is passed from Windows to WSL with
**usbipd-win**.

## 1. WSL and usbipd (Windows, PowerShell as administrator)

```powershell
wsl --install -d Ubuntu-24.04
winget install --exact dorssel.usbipd-win
```

Restart the computer and open Ubuntu from the Start menu.

## 2. Packages in Ubuntu

```bash
sudo apt update && sudo apt install -y --no-install-recommends git cmake ninja-build gperf ccache dfu-util device-tree-compiler wget flex bison python3-dev python3-pip python3-venv xz-utils file make gcc g++ gcc-multilib g++-multilib libffi-dev libssl-dev libusb-1.0-0 libsdl2-dev libmagic1 usbutils tio
```

Give your user access to serial ports (takes effect after running `wsl --shutdown` in PowerShell):

```bash
sudo usermod -aG dialout $USER
```

> Keep all projects in the Linux home directory (`~/...`), **not** under `/mnt/c/...` – builds across the
> Windows file system are many times slower.

## 3. Clone the repository

Clone it wherever you like in the Linux file system:

```bash
git clone --recursive git@github.com:guszahrma/yrgo_hwc-_p02.git
cd yrgo_hwc-_p02
```

`--recursive` also fetches GoogleTest (`tests/googletest`). In an existing clone without it:
`git submodule update --init`.

### Fixed port names

A board's serial port is `/dev/ttyACM0`, `/dev/ttyACM1`, ... depending on the order things were attached,
and the name can change after a reset. The project's udev rules give each board a name that stays the
same, whatever runs on it:

| Board | Port |
|---|---|
| Arduino Nano ESP32 | `/dev/tty_arduino_nano_esp32` |
| Daisy Seed | `/dev/tty_daisy_seed` |

Each name is a symbolic link to the board's current `/dev/ttyACM<n>`, created by udev every time the board
appears and removed when it disappears; `ls -l /dev/tty_*` shows which. Only installing the rules is
manual – once, and again when [`tools/udev/99-p02.rules`](../tools/udev/99-p02.rules) changes:

```bash
sudo cp tools/udev/99-p02.rules /etc/udev/rules.d/ && sudo udevadm control --reload-rules && sudo udevadm trigger
```

The commands in these docs use these names. The monitor is `tio` (installed in step 2): it waits for the
port and reconnects after every reset. Exit it with `Ctrl+T`, `q`.

## 4. Attach the board to WSL (PowerShell as administrator)

Run `usbipd list` once without the board connected and once with it, to see which device is the board and
its BUSID (`6-4` in the example below):

```powershell
usbipd list                                    # find the board's BUSID, e.g. 6-4
usbipd bind --busid 6-4                        # only needed once during first setup per device and port
usbipd attach --wsl --busid 6-4 --auto-attach  # every time the computer or WSL restarts; keep the window open
```

`--auto-attach` is needed because both boards disconnect and reconnect USB at every reset and flash. The
Daisy Seed is a different USB device in its ROM bootloader: bind both of its devices.

Check in WSL which mode the board is in:

```bash
lsusb | grep -iE "303a|2341|0483|2fe3"; ls /dev/tty_*
```

| Board | USB ID | Meaning |
|---|---|---|
| Arduino Nano ESP32 | `303a:1001` | ESP32-S3 USB-Serial/JTAG – ready to flash, `/dev/tty_arduino_nano_esp32` |
| Arduino Nano ESP32 | `2341:0070` | Still the Arduino factory firmware – esptool does **not** work, do step 5 first |
| Daisy Seed | `0483:df11` | ROM bootloader (BOOT + RESET), for `dfu-util` – no serial port |
| Daisy Seed | `2fe3:0004` | MCUboot's recovery mode or an app, `/dev/tty_daisy_seed` |

No `/dev/tty_*` name, only a `/dev/ttyACM*`: the udev rules are not installed ([fixed port
names](#fixed-port-names)). More about each board's USB devices is in its board document, linked from
[Boards](boards.md).

## 5. First time: get the board out of Arduino mode

The Nano ESP32 ships with the Arduino bootloader, which cannot be flashed from here (`2341:0070` in the
table above). Before the first flash, start it in the ROM bootloader once (B1 to GND + RESET): see the
board document's [First flash](../zephyr/boards/arduino/nano_esp32/doc/index.md#first-flash-leave-arduino-mode).

## Next: set up a platform

The project supports two platforms, ESP-IDF and Zephyr. You can set up either one, or both.

| Platform | Setup guide |
|---|---|
| ESP-IDF 6.0 | [ESP-IDF environment](platform_esp_idf.md#setup) |
| Zephyr 4.4.2 | [Zephyr environment](platform_zephyr.md#setup) |

> **Never mix environments in one terminal.** Use a fresh terminal for ESP-IDF and for Zephyr.

> **If you take the QAcademy Zephyr course:** do not use the course's `source ~/zephyr/env.sh` in the same
> terminal either. It sets `ZEPHYR_TOOLCHAIN_VARIANT=host` (the PC's own GCC), which breaks builds for the
> ESP32-S3.
