#include "driver/serial/esp32s3.h"
#include "driver/usb_serial_jtag.h" // Viktig för Nano ESP32
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace driver::serial
{

// --------------------------------------------------------------------------------  
Esp32s3::Esp32s3(int baudRate) noexcept
{
    usb_serial_jtag_driver_config_t cfg = {
        .tx_buffer_size = 256,
        .rx_buffer_size = 256
    };
    
    // Install USB-serial driver
    usb_serial_jtag_driver_install(&cfg);
}

// --------------------------------------------------------------------------------
Esp32s3::~Esp32s3() noexcept
{
    // Uninstall USB-serial driver
    usb_serial_jtag_driver_uninstall();
}

// --------------------------------------------------------------------------------
void Esp32s3::print(const char* text) noexcept
{
    if (text != nullptr)
    {
        usb_serial_jtag_write_bytes(text, std::strlen(text), pdMS_TO_TICKS(100));
    }
}

// --------------------------------------------------------------------------------
std::size_t Esp32s3::readLine(char* buffer, std::size_t maxSize) noexcept
{
    if (buffer == nullptr || maxSize == 0) return 0;

    std::size_t index = 0;
    
    // Run until maxSize
    while (index < maxSize - 1)
    {
        uint8_t byte;
        // Wait a while for every character
        int len = usb_serial_jtag_read_bytes(&byte, 1, pdMS_TO_TICKS(10));
        
        if (len > 0)
        {
            // Send back character
            usb_serial_jtag_write_bytes(&byte, 1, pdMS_TO_TICKS(10));

            // If character is '\n' or '\r, break. Else continue
            if (byte == '\n' || byte == '\r')
            {
                if (index > 0) 
                {
                    break; 
                }
                else 
                {
                    continue; 
                }
            }
            
            // Save character in buffer
            buffer[index++] = static_cast<char>(byte);
        }
        else 
        {
            vTaskDelay(pdMS_TO_TICKS(10)); 
        }
    }
    
    buffer[index] = '\0';
    return index;
}

} // namespace driver::serial