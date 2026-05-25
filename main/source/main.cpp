#include <chrono>
#include <cstdint>
#include <stdio.h>
#include <thread>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/serial/esp32s3.h" // Din riktiga driver
#include "driver/gpio/stub.h"
#include "driver/gpio/esp32s3.h"
#include "driver/adc/esp32s3.h"
#include "driver/pin/esp32s3.h"
#include "driver/eeprom/esp32s3.h"


constexpr std::string dataToStore{"Hello, EEPROM!"};
/**
 * @brief GPIO example.
 */
extern "C" void app_main()
{
    // test code for eeprom
    driver::eeprom::Esp32s3 myEeprom(1024); // 1KB EEPROM
    myEeprom.write(0, reinterpret_cast<const uint8_t*>(dataToStore.c_str()), dataToStore.length());
    uint8_t eepromBuffer[64];
    uint16_t bytesRead{0};
    if ((bytesRead = myEeprom.read(0, eepromBuffer, sizeof(eepromBuffer))) > 0)
    {
        eepromBuffer[bytesRead] = '\0'; // Null-terminate the string
        printf("Read from EEPROM: %s\n", eepromBuffer);
    }
    // end of test code for eeprom

    // 1. Skapa drivrutinen
    driver::serial::Esp32s3 mySerial(115200); 
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    mySerial.print("Systemet är redo! Skriv något:\n");

    char buffer[64];
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

    while(1)
    {
        // 2. Försök läsa
        std::size_t bytesRead = mySerial.readLine(buffer, sizeof(buffer));

        if (bytesRead > 0)
        {
            mySerial.print("\n[Mottaget]: ");
            mySerial.print(buffer);
            mySerial.print("\n");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
        esp32s3.toggle();

        printf("testing: adc value is %i.\n", esp32s3Adc.read_value());
        printf("testing: adc voltage is %.4f.\n", esp32s3Adc.read_voltage());
        // Vänta i 500 millisekunder
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}