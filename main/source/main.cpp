/**
 * @brief GPIO example.
 */
#include <chrono>
#include <thread>

#include "driver/gpio/stub.h"
#include "driver/gpio/esp32s3.h"
#include "driver/adc/esp32s3.h"
#include "driver/pin/esp32s3.h"

#include "driver/tempsensor/tmp36.h" 
#include "esp_log.h"

extern "C" void app_main()
{
    driver::gpio::Esp32s3 esp32s3{9U, driver::gpio::Direction::OUTPUT};
    
    esp32s3.write(true);
    esp32s3.read();

    constexpr auto adcPin = driver::pin::esp32s3::AdcPin::A6;
    if (!driver::pin::esp32s3::is_available(adcPin))
    {
        printf("app_main: ADC pin %s is already in use, aborting.\n", driver::pin::esp32s3::to_string(adcPin));
        return;
    }
    driver::adc::Esp32s3 esp32s3Adc{adcPin, 3.3f};

    printf("testing: adc value is %i.\n", esp32s3Adc.read_value());
    printf("testing: adc voltage is %.4f.\n", esp32s3Adc.read_voltage());


    driver::tempsensor::Tmp36 tmp36{esp32s3Adc}; 
    tmp36.isInitialized();
    //ESP_LOGI("main", "The temperature is %d degrees.\n", tmp36.read()); 

    while(1)
    {
        esp32s3.toggle();

        printf("testing: adc value is %i.\n", esp32s3Adc.read_value());
        printf("testing: adc voltage is %.4f.\n", esp32s3Adc.read_voltage());
        // Vänta i 500 millisekunder
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        ESP_LOGI("main", "The temperature is %d degrees.\n", tmp36.read()); 
    }
}
