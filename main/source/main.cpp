/**
 * @brief GPIO and timer example.
 */
#include <cstdint>
#include <chrono>
#include <thread>

#include "driver/gpio/esp32s3.h"
#include "driver/timer/esp32s3.h"

extern "C" void app_main()
{
    driver::gpio::Esp32s3 esp32s3{9U, driver::gpio::Direction::OUTPUT};
    driver::timer::Esp32s3 timer{};

    timer.set_period(500);
    timer.start();

    while (1)
    {
        if (timer.timeout())
        {
            esp32s3.toggle();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}