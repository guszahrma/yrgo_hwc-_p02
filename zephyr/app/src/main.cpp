/**
 * @file Test of serial::Zephyr: echoes each line typed in the terminal.
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "driver/serial/zephyr.h"

int main()
{
    // The console device: USB-C on the Nano ESP32 (zephyr,console in the board).
    driver::serial::Zephyr serial{DEVICE_DT_GET(DT_CHOSEN(zephyr_console))};
    serial.print("Type a line and press Enter:\n");

    char line[64]{};

    while (true)
    {
        // Non-blocking: returns true only when a whole line has been received.
        if (serial.readLine(line, sizeof(line)))
        {
            serial.print("You wrote: ");
            serial.print(line);
            serial.print("\n");
        }
        k_msleep(10);
    }
    return 0;
}