#include "logic/logic.h"
// #include <chrono>
// #include <stdio.h>
// #include <thread>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/serial/esp32s3.h" // Din riktiga driver
// #include "driver/gpio/stub.h"
// #include "driver/gpio/esp32s3.h"
// #include "driver/adc/esp32s3.h"
// #include "driver/pin/esp32s3.h"

/**
 * @brief GPIO, timer, serial and ADC example.
 */
extern "C" void app_main()
{
    logic::logic::Logic logic;
    logic.run();
    // // 1. Skapa stub-fabriken enkelt på stacken
    // driver::factory::Stub factory{};

    // // 2. Skapa logiken och skicka in fabriken (om Logic är byggd för att ta emot fabriken)
    // // app::Logic logic{factory};
    // // logic.run();

    // // 3. Om du bara vill snabbtesta din kod manuellt just nu:
    // auto fakeLed = factory.create_gpio(9, driver::gpio::Direction::OUTPUT);
    // if (fakeLed) {
    //     fakeLed->write(true);
    // }
}