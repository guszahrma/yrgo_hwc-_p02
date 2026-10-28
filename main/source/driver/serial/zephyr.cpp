/**
 * @file Zephyr serial driver implementation.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include "driver/serial/zephyr.h"

namespace driver::serial
{
// --------------------------------------------------------------------------------
Zephyr::Zephyr(const device* uart) noexcept
    : myUart{uart}
    , myReady{false}
    , myInternalBuf{}
    , myInternalLen{0U}
{
    if (nullptr == myUart)
    {
        printk("Zephyr serial: no UART device given.\n");
        return;
    }

    // The device is started by Zephyr at boot; only check that it is ready.
    if (!device_is_ready(myUart))
    {
        printk("Zephyr serial: UART device %s is not ready.\n", myUart->name);
        return;
    }

    myReady = true;
}

// --------------------------------------------------------------------------------
void Zephyr::print(const char* text) noexcept
{
    if ((!myReady) || (nullptr == text)) { return; }

    for (const char* p{text}; '\0' != *p; ++p)
    {
        // Terminals need "\r\n" to start the next line at the left edge.
        if ('\n' == *p) { uart_poll_out(myUart, '\r'); }
        uart_poll_out(myUart, static_cast<unsigned char>(*p));
    }
}

// --------------------------------------------------------------------------------
bool Zephyr::readLine(char* buffer, std::size_t maxSize) noexcept
{
    if ((!myReady) || (nullptr == buffer) || (0U == maxSize)) { return false; }

    unsigned char byte{};

    // uart_poll_in() returns 0 when a character was read and -1 when there is none (non-blocking).
    while (0 == uart_poll_in(myUart, &byte))
    {
        if (('\n' == byte) || ('\r' == byte))
        {
            // Echo a line break so the next output starts on a new line.
            uart_poll_out(myUart, '\r');
            uart_poll_out(myUart, '\n');

            if (0U < myInternalLen)
            {
                const std::size_t bytesToCopy{(maxSize - 1U) > myInternalLen ? myInternalLen
                                                                             : (maxSize - 1U)};

                for (std::size_t i{}; i < bytesToCopy; ++i)
                {
                    buffer[i] = myInternalBuf[i];
                }
                buffer[bytesToCopy] = '\0';

                myInternalLen = 0U;
                return true;
            }
        }
        else if ((InternalBufLen - 1U) > myInternalLen)
        {
            // Echo the character so the user sees what they type.
            uart_poll_out(myUart, byte);
            myInternalBuf[myInternalLen++] = static_cast<char>(byte);
        }
    }
    return false;
}
} // namespace driver::serial