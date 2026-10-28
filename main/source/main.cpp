/**
 * @file main.cpp
 * @brief Application entry point.
 */

#include "driver/factory/esp32s3.h"
#include "logic/logic.h"

extern "C" void app_main()
{
    // The ESP-IDF build uses the real ESP32-S3 drivers.
    driver::factory::Esp32s3 factory{};
    logic::Logic logic{factory};
    logic.run();
}