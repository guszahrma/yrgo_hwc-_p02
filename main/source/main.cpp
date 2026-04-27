/**
 * @brief GPIO example.
 */
#include <cstdint>

#include "driver/gpio/stub.h"
#include "driver/gpio/esp32s3.h"


extern "C" void app_main()
{
    driver::gpio::Esp32s3 esp32s3{9U, driver::gpio::OUTPUT};

    esp32s3.write(true);
    esp32s3.read();
    while(1)
    {
    }
}
