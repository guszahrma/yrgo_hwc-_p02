# Setup

Setting up a computer for the project: WSL, packages, cloning, and attaching the board. Platform-specific
setup is in [ESP-IDF environment](espressif_environment.md) and [Zephyr environment](zephyr_environment.md).

## Development environment (WSL + ESP-IDF)

The recommended environment is **Ubuntu 24.04 in WSL2** with ESP-IDF 6.0. The USB board is passed from
Windows to WSL with **usbipd-win**.

### 1. WSL and usbipd (Windows, PowerShell as administrator)

```powershell
wsl --install -d Ubuntu-24.04
winget install --exact dorssel.usbipd-win
```

Restart the computer and open Ubuntu from the Start menu.

### 2. Packages in Ubuntu

```bash
sudo apt update && sudo apt install -y --no-install-recommends git cmake ninja-build gperf ccache dfu-util device-tree-compiler wget flex bison python3-dev python3-pip python3-venv xz-utils file make gcc g++ gcc-multilib g++-multilib libffi-dev libssl-dev libusb-1.0-0 libsdl2-dev libmagic1 usbutils
```

Give your user access to serial ports (takes effect after running `wsl --shutdown` in PowerShell):

```bash
sudo usermod -aG dialout $USER
```

> Keep all projects in the Linux home directory (`~/...`), **not** under `/mnt/c/...` – builds across the
> Windows file system are many times slower.

### 3. Clone the repository

```bash
cd ~
git clone --recursive git@github.com:guszahrma/yrgo_hwc-_p02.git
cd yrgo_hwc-_p02
```

Then set up the platform you want to build for: [ESP-IDF](espressif_environment.md#setup) or
[Zephyr](zephyr_environment.md#setup).

## Attaching the board to WSL

### 1. Attach the board to WSL (PowerShell as administrator)
Run usbipd list first without the esp connected and then with the esp connected to see which device is added and with which VID:PID it appeared. in the example below it appeared with VID:PID 6-4.
```powershell
usbipd list                                    # find the board's BUSID, e.g. 6-4
usbipd bind --busid 6-4                        # once per device and port
usbipd attach --wsl --busid 6-4 --auto-attach  # keep this window open
```

`--auto-attach` is needed because the ESP32-S3 disconnects and reconnects USB on every reset/flash.

Check in WSL:

```bash
lsusb | grep -iE "303a|2341"; ls /dev/ttyACM*
```

| USB ID | Meaning |
|---|---|
| `2341:0070` | Arduino factory firmware – esptool does **not** work, see below |
| `303a:1001` | ESP32-S3 USB-Serial/JTAG – ready to flash |

### 2. First time: get the board out of Arduino mode

The Nano ESP32 ships with the Arduino bootloader. Before the first flash:

1. Connect pin **B1 to GND**.
2. Press **reset** – the board starts in the ROM bootloader and shows up as `303a:1001`.
3. Run `usbipd list` again; if the device is not shared, run `usbipd bind` + `attach` again.
4. Flash, remove the wire and press reset.

After this, esptool can put the board into flash mode by itself.

> To restore the Arduino bootloader: B1 to GND + reset, then run *Burn Bootloader* in the Arduino IDE.

## Troubleshooting

| Error | Cause / fix |
|---|---|
| `Could not open COM3` | Windows port name used in WSL. Use `/dev/ttyACM0`. |
| `Path '/dev/ttyACM0' is not readable` | The user is not in the `dialout` group. See step 2 above, then run `wsl --shutdown` in PowerShell. |
| `usbipd: error: Device is not shared` | Run `usbipd bind --busid <id>` as administrator first. |
| No `/dev/ttyACM*` in WSL | Check that `usbipd attach` is running and that the board is not in Arduino mode (`2341:0070`). |
