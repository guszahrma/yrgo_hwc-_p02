/**
 * @file Esp32s3 serial driver
 */
#include "driver/serial/esp32s3.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "freertos/FreeRTOS.h"
#include <cstring>

namespace driver::serial
{

// --------------------------------------------------------------------------------
Esp32s3::Esp32s3() noexcept
    : myInternalBuf{}
    , myInternalLen{}
{
    usb_serial_jtag_driver_config_t cfg{.tx_buffer_size{InternalBufLen},
                                        .rx_buffer_size{InternalBufLen}};
    (void)usb_serial_jtag_driver_install(&cfg);

    // printf (stdout) writes to the same USB port. Without this it writes straight to the hardware
    // past the driver's TX buffer, and text printed right after print() is lost in part or whole.
    usb_serial_jtag_vfs_use_driver();
}

// --------------------------------------------------------------------------------
Esp32s3::~Esp32s3() noexcept
{
    // Back to writing straight to the hardware first: stdout must not use a removed driver.
    usb_serial_jtag_vfs_use_nonblocking();
    (void)usb_serial_jtag_driver_uninstall();
}

// --------------------------------------------------------------------------------
void Esp32s3::print(const char* text) noexcept
{
    if (nullptr == text) { return; }

    // usb_serial_jtag_write_bytes() puts the whole write in the TX buffer or nothing (and with no
    // wait, nothing as soon as the buffer is a little full), so send the text in chunks that fit.
    std::size_t left{std::strlen(text)};

    while (0U < left)
    {
        const std::size_t chunk{(WriteChunkLen < left) ? WriteChunkLen : left};

        // 0 means no room within the timeout: the host is not reading, so drop the rest.
        if (0 >= usb_serial_jtag_write_bytes(text, chunk, pdMS_TO_TICKS(WriteTimeoutMs)))
        {
            return;
        }

        text += chunk;
        left -= chunk;
    }
}

// --------------------------------------------------------------------------------
bool Esp32s3::readLine(char* buffer, std::size_t maxSize) noexcept
{
    if ((nullptr == buffer) || (0U == maxSize)) { return false; }

    std::uint8_t byte{};

    while (0 < usb_serial_jtag_read_bytes(&byte, 1U, 0U))
    {
        (void)usb_serial_jtag_write_bytes(&byte, 1U, 0U);

        if (('\n' == byte) || ('\r' == byte))
        {
            if (0U < myInternalLen)
            {
                const std::size_t bytesToCopy{maxSize - 1U > myInternalLen ? myInternalLen
                                                                           : maxSize - 1U};

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
            myInternalBuf[myInternalLen++] = static_cast<char>(byte);
        }
    }
    return false;
}
} // namespace driver::serial
