/**#include <iostream>

#include "driver/serial/stub.h"
#include "driver/serial/esp32s3.h"

extern "C" void app_main()
{
    driver::serial::Stub serial{};
    serial.simulateInputData("Hej, jag heter Fredrik!\n");

    constexpr std::size_t bufLen{100U};
    char buf[bufLen]{};
    const std::size_t bytesRead{serial.readLine(buf,bufLen)};
    std::cout << buf << "\n";
    return 0;
}
**/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/serial/esp32s3.h" // Din riktiga driver

extern "C" void app_main()
{
    // 1. Skapa drivrutinen
    driver::serial::Esp32s3 mySerial(115200); 
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    mySerial.print("Systemet är redo! Skriv något:\n");

    char buffer[64];
    while (true)
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
    }
}