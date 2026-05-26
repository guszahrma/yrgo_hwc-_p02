#include <chrono>
#include <cstdint>
#include <stdio.h>
#include <thread>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/serial/esp32s3.h"
#include "driver/gpio/stub.h"
#include "driver/gpio/esp32s3.h"
#include "driver/adc/esp32s3.h"
#include "driver/pin/esp32s3.h"
#include "driver/timer/esp32s3.h"

#include "driver/tempsensor/tmp36.h"
#include "esp_log.h"

/**
 * @brief GPIO, timer, serial and ADC example.
 */
extern "C" void app_main()
{
    driver::serial::Esp32s3 mySerial(115200);

    vTaskDelay(pdMS_TO_TICKS(2000));
    mySerial.print("Systemet är redo! Skriv något:\n");

    char buffer[64];

    driver::gpio::Esp32s3 esp32s3{9U, driver::gpio::Direction::OUTPUT};
    driver::timer::Esp32s3 timer{};

    timer.set_period(500);
    timer.start();

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

    while (1)
    {
        std::size_t bytesRead = mySerial.readLine(buffer, sizeof(buffer));

        if (bytesRead > 0)
        {
            mySerial.print("\n[Mottaget]: ");
            mySerial.print(buffer);
            mySerial.print("\n");
        }

        if (timer.timeout())
        {
            esp32s3.toggle();
        }

        printf("testing: adc value is %i.\n", esp32s3Adc.read_value());
        printf("testing: adc voltage is %.4f.\n", esp32s3Adc.read_voltage());
        ESP_LOGI("main", "The temperature is %d degrees.\n", tmp36.read());

        vTaskDelay(pdMS_TO_TICKS(100));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}