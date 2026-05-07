/**
 * @brief GPIO example.
 */
#include <cstdint>
#include <chrono>
#include <thread>

#include "driver/gpio/stub.h"
#include "driver/gpio/esp32s3.h"

#include "driver/tempsensor/stub.h" //
#include "driver/tempsensor/tmp36.h" //
#include "driver/tempsensor/stub.h" //

#include "esp_log.h"

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

        //driver::tempsensor::Stub tmp1;  //
        //tmp1.simulateInitialized(true); //
        //tmp1.simulateTemperature(17); //

        driver::tempsensor:: tmp1{};  //




        ESP_LOGI("main", "Temperature: %d\n", tmp1.read());
    }
}
