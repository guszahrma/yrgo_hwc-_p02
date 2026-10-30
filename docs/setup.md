# Setup

Setting up a computer for the project: WSL, packages, cloning and attaching the board. This is the same for
both platforms; after it, continue with [ESP-IDF environment](espressif_environment.md) or
[Zephyr environment](zephyr_environment.md).

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
sudo apt update && sudo apt install -y --no-install-recommends git cmake ninja-build gperf ccache dfu-util device-tree-compiler wget flex bison python3-dev python3-pip python3-venv xz-utils file make gcc g++ gcc-multilib g++-multilib libffi-dev libssl-dev libusb-1.0-0 libsdl2-dev libmagic1 usbutils
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

## 4. Attach the board to WSL (PowerShell as administrator)

Run `usbipd list` once without the board connected and once with it, to see which device is the board and
its BUSID (`6-4` in the example below):

```powershell
usbipd list                                    # find the board's BUSID, e.g. 6-4
usbipd bind --busid 6-4                        # only needed once during first setup per device and port
usbipd attach --wsl --busid 6-4 --auto-attach  # every time the computer or WSL restarts; keep the window open
```

`--auto-attach` is needed because the ESP32-S3 disconnects and reconnects USB on every reset/flash.

Check in WSL which mode the board is in:

```bash
lsusb | grep -iE "303a|2341"; ls /dev/ttyACM*
```

| USB ID | Meaning |
|---|---|
| `303a:1001` | ESP32-S3 USB-Serial/JTAG – ready to flash |
| `2341:0070` | Still the Arduino factory firmware – esptool does **not** work, do step 5 first |

## 5. First time: get the board out of Arduino mode

The Nano ESP32 ships with the Arduino bootloader. Before the first flash:

1. Connect pin **B1 to GND**.
2. Press **reset** – the board starts in the ROM bootloader and shows up as `303a:1001`.
3. Run `usbipd list` again; if the device is not shared, run `usbipd bind` + `attach` again.
4. Flash, remove the wire and press reset.

After this, esptool can put the board into flash mode by itself.

> To restore the Arduino bootloader: B1 to GND + reset, then run *Burn Bootloader* in the Arduino IDE.

## Next: set up a platform

The project supports two platforms, ESP-IDF and Zephyr. You can set up either one, or both.

| Platform | Setup guide |
|---|---|
| ESP-IDF 6.0 | [ESP-IDF environment](espressif_environment.md#setup) |
| Zephyr 4.4.2 | [Zephyr environment](zephyr_environment.md#setup) |

> **Never mix environments in one terminal.** Use a fresh terminal for ESP-IDF and for Zephyr.

> **If you take the QAcademy Zephyr course:** do not use the course's `source ~/zephyr/env.sh` in the same
> terminal either. It sets `ZEPHYR_TOOLCHAIN_VARIANT=host` (the PC's own GCC), which breaks builds for the
> ESP32-S3.
