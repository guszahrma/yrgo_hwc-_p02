#include <chrono>
#include <stdio.h>
#include <string>
#include <thread>
#include "communication/mqtt/publish/esp32s3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/serial/esp32s3.h" // Din riktiga driver
#include "driver/gpio/stub.h"
#include "driver/gpio/esp32s3.h"
#include "driver/adc/esp32s3.h"
#include "driver/pin/esp32s3.h"
#include "communication/mqtt/publish/esp32s3.h"
#include "communication/mqtt/subscribe/esp32s3.h"
#include "communication/wifi/esp32s3.h"

/**
 * @brief GPIO example.
 */
extern "C" void app_main()
{
    // Todo: Find a relevant MQTT Broker URL for testing, e.g., a local broker or a public test broker.
    // I did the test in my local environment with the Mosquitto MQTT broker running on my machine, so I used the local IP address of my machine in the URL.
    constexpr auto mqttBrokerUrl = "mqtt://192.168.1.3:1883"; 
    constexpr auto mqttTopicLedState = "fmms/led/state";
    constexpr auto mqttTopicLedRequest = "fmms/led/set";

    communication::wifi::Esp32s3 wifi;
    wifi.init();
    bool wifiConnected = wifi.connect();
    if (!wifiConnected)
    {
        printf("WiFi failed, trying again.\n");
        wifiConnected = wifi.connect();
    }
    if (!wifiConnected)
    {
        printf("WiFi failed, halting.\n");
        return;
    }

    //test of MQTT class
    communication::mqtt::publish::Esp32s3 mqttLedStatePublisher(mqttBrokerUrl, mqttTopicLedState, "fmms_pub");

    {
        communication::mqtt::publish::Esp32s3 mqttSetLedPublisher(mqttBrokerUrl, mqttTopicLedRequest, "fmms_sub");
        mqttSetLedPublisher.publish("0", 0, true);
    }

    communication::mqtt::subscribe::Esp32s3 mqttLedRequestSubscriber(mqttBrokerUrl, mqttTopicLedRequest, "fmms_sub");

    // Simulate receiving commands from the serial input and processing them in the logic loop.
    // The logic loop will read commands, execute the corresponding actions, and handle the blinking logic and period adjustments.
    while (!mqttLedStatePublisher.isConnected() || !mqttLedRequestSubscriber.isConnected())
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        printf(".");
    }
    bool ok = mqttLedStatePublisher.publish("Hello from ESP32-S3!", 0, true);
    printf("publish %s\n", ok ? "queued OK" : "FAILED");

    std::string message;
    mqttLedRequestSubscriber.getValue(message);
    printf("found the message: %s, on MQTT\n", message.c_str());


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

        message.assign(buffer, bytesRead);
        mqttLedStatePublisher.publish(message, 0, true);
        if (mqttLedRequestSubscriber.getValue(message))
        {
            // Handle the received message
            printf("Received MQTT message: %s\n, on %s", message.c_str(), mqttTopicLedRequest);
        }

        // Vänta i 500 millisekunder
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}