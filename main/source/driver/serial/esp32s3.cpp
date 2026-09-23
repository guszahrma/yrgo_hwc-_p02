/**
 * @file Esp32s3 serial driver.
 */
#include "driver/serial/esp32s3.h"
#include "driver/usb_serial_jtag.h"
#include <cstring>

namespace driver::serial
{

// --------------------------------------------------------------------------------  
Esp32s3::Esp32s3(std::uint32_t baudRate) noexcept
    : myInternalBuf{}
    , myInternalLen{}
{
    usb_serial_jtag_driver_config_t cfg = {
        .tx_buffer_size{InternalBufLen},
        .rx_buffer_size{InternalBufLen}
    };
    (void)usb_serial_jtag_driver_install(&cfg);
}

// --------------------------------------------------------------------------------
Esp32s3::~Esp32s3() noexcept
{
    (void)usb_serial_jtag_driver_uninstall();
}

// --------------------------------------------------------------------------------
void Esp32s3::print(const char* text) noexcept
{
    if (nullptr != text)
    {
        (void)usb_serial_jtag_write_bytes(text, std::strlen(text), 0U);
    }
}

// --------------------------------------------------------------------------------
bool Esp32s3::readLine(char* buffer, std::size_t maxSize) noexcept
{
    if ((nullptr == buffer) || (0U == maxSize)) { return false; }

    std::uint8_t byte{};
    
    while ( 0 < usb_serial_jtag_read_bytes(&byte, 1U, 0U))
    {
        (void)usb_serial_jtag_write_bytes(&byte, 1U, 0U);

        if (('\n' == byte) || ('\r' == byte))
        {
            if (0U < myInternalLen)
            {
                const std::size_t bytesToCopy{maxSize - 1U > myInternalLen ? myInternalLen : maxSize - 1U};
                
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
