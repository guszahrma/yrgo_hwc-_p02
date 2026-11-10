/**
 * @file Zephyr serial driver implementation.
 */

#include <cstdint>

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
    , myInterruptDriven{false}
    , myRxRing{}
    , myRxStorage{}
    , myInternalBuf{}
    , myInternalLen{0U}
    , myPortOpen{false}
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

    // Receive by interrupt, so bytes arriving between two readLine() calls are kept.
    ring_buf_init(&myRxRing, RxBufLen, myRxStorage);
    if (0 == uart_irq_callback_user_data_set(myUart, onInterrupt, this))
    {
        uart_irq_rx_enable(myUart);
        myInterruptDriven = true;
    }

    myReady = true;
}

// --------------------------------------------------------------------------------
Zephyr::~Zephyr() noexcept
{
    if (myInterruptDriven)
    {
        uart_irq_rx_disable(myUart);
        uart_irq_callback_user_data_set(myUart, nullptr, nullptr);
    }
}

// --------------------------------------------------------------------------------
void Zephyr::onInterrupt(const device* uart, void* param) noexcept
{
    auto* self{static_cast<Zephyr*>(param)};

    while ((0 < uart_irq_update(uart)) && (0 < uart_irq_rx_ready(uart)))
    {
        std::uint8_t data[16U];
        const int count{uart_fifo_read(uart, data, sizeof(data))};
        if (0 >= count) { break; }

        // Bytes that do not fit are dropped, as a full line would be anyway.
        ring_buf_put(&self->myRxRing, data, static_cast<std::uint32_t>(count));
    }
}

// --------------------------------------------------------------------------------
bool Zephyr::readByte(unsigned char& byte) noexcept
{
    if (myInterruptDriven) { return 1U == ring_buf_get(&myRxRing, &byte, 1U); }

    // uart_poll_in() returns 0 when a character was read and -1 when there is none (non-blocking).
    return 0 == uart_poll_in(myUart, &byte);
}

// --------------------------------------------------------------------------------
void Zephyr::print(const char* text) noexcept
{
    if ((!myReady) || (nullptr == text)) { return; }

    for (const char* p{text}; '\0' != *p; ++p)
    {
        uart_poll_out(myUart, static_cast<unsigned char>(*p));
    }
}

// --------------------------------------------------------------------------------
bool Zephyr::readLine(char* buffer, std::size_t maxSize) noexcept
{
    if ((!myReady) || (nullptr == buffer) || (0U == maxSize)) { return false; }

    unsigned char byte{};

    // A monitor has just opened the port (DTR set): drop what was received before. When the PC
    // opens a USB serial port with echo on, it sends the app's waiting output straight back, which
    // would otherwise end up in front of the first command. Only USB serial ports (CDC ACM) report
    // DTR.
    std::uint32_t dtr{};
    if (0 == uart_line_ctrl_get(myUart, UART_LINE_CTRL_DTR, &dtr))
    {
        const bool portOpen{0U != dtr};
        if (portOpen && !myPortOpen)
        {
            while (readByte(byte)) {}
            myInternalLen = 0U;
        }
        myPortOpen = portOpen;
    }

    while (readByte(byte))
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