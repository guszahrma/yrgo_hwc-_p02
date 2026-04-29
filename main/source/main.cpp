/**
 * @brief GPIO example.
 */
#include <cstdint>
#include <chrono>
#include <thread>

#include "driver/gpio/stub.h"
#include "driver/gpio/esp32s3.h"


extern "C" void app_main()
{
    driver::gpio::Esp32s3 esp32s3{9U, driver::gpio::Direction::OUTPUT};

    esp32s3.write(true);
    esp32s3.read();
    while(1)
    {
    esp32s3.toggle();
    // Vänta i 500 millisekunder
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
}
