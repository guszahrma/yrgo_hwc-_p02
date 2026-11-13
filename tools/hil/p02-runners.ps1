# Keeps WSL running and the boards attached to it, for P02's GitHub runners (service in WSL).
# Run at logon by a scheduled task (see docs/contributing.md). Remove the boards this PC does not have.
#
# Each command runs in its own endless loop, so it is started again when it ends: after hibernation
# or a WSL restart (e.g. a WSL update), the task does not run again, since neither is a logon.
$distro = "Ubuntu"

# Boards attached by USB ID (VID:PID, see usbipd list), so any USB port works. Each ID must match
# only one device on this PC: ST-Link/V2-1 and ESP32-S3 USB-Serial/JTAG.
$byHardwareId = @{
    "0483:374b" = "Nucleo-G0B1RE (ST-Link/V2-1)"
    "303a:1001" = "Arduino Nano ESP32 (ESP32-S3 USB-Serial/JTAG)"
}

# Boards attached by bus ID (the USB port; a board moved to another port needs its new bus ID here).
# The Daisy changes its USB ID in MCUboot's serial recovery and in its ROM bootloader (DFU, 0483:df11);
# its port stays the same.
$byBusId = @{
    "4-4" = "Daisy Seed"
}

# WSL stops soon after its last process ends; this one never ends (started again if WSL restarts)
Start-Process -WindowStyle Hidden powershell.exe -ArgumentList "-NoProfile -Command while (`$true) { wsl.exe -d $distro --exec /bin/sleep infinity; Start-Sleep -Seconds 10 }"

# Re-attaches after every reset/re-plug. usbipd allows --unplugged (wait for a board that is not
# plugged in) only with --busid; with --hardware-id, the attach fails and the loop tries again.
foreach ($hwid in $byHardwareId.Keys) {
    Start-Process -WindowStyle Hidden powershell.exe -ArgumentList "-NoProfile -Command while (`$true) { usbipd attach --wsl $distro --hardware-id $hwid --auto-attach; Start-Sleep -Seconds 10 }"
}
foreach ($busid in $byBusId.Keys) {
    Start-Process -WindowStyle Hidden powershell.exe -ArgumentList "-NoProfile -Command while (`$true) { usbipd attach --wsl $distro --busid $busid --auto-attach --unplugged; Start-Sleep -Seconds 10 }"
}
