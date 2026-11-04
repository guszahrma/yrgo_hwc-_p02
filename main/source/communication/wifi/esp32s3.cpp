#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdint.h>
#include <utility>

#include "driver/pin/esp32s3.h"
#include "driver/pin/manager.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "communication/wifi/esp32s3.h"

namespace
{
/** The ADC2 pins. Wi-Fi uses ADC2, so these cannot be used as analog inputs while Wi-Fi is on. */
constexpr std::array<driver::pin::esp32s3::AdcPin, 4U> Adc2Pins{
    driver::pin::esp32s3::AdcPin::A4, driver::pin::esp32s3::AdcPin::A5,
    driver::pin::esp32s3::AdcPin::A6, driver::pin::esp32s3::AdcPin::A7};

std::uint8_t physicalPin(const driver::pin::esp32s3::AdcPin pin) noexcept
{ return static_cast<std::uint8_t>(driver::pin::esp32s3::toNumber(pin)); }
} // namespace

namespace comm::wifi
{

// --------------------------------------------------------------------------------
Esp32s3::Esp32s3(std::string ssid, std::string password) noexcept
    : mySsid{std::move(ssid)}
    , myPassword{std::move(password)}
    , myPinsAcquired{false}
{}

// --------------------------------------------------------------------------------
Esp32s3::~Esp32s3() noexcept
{
    if (!myPinsAcquired) { return; }

    auto& manager{driver::pin::PhysicalPinManager::instance()};
    for (const auto pin : Adc2Pins)
    {
        manager.release(physicalPin(pin));
    }
}

// --------------------------------------------------------------------------------
void Esp32s3::init() noexcept
{
    if (myPinsAcquired) { return; } // Already initialised

    // Reserve A4–A7 (ADC2) so no analog driver uses them while Wi-Fi is on.
    auto& manager{driver::pin::PhysicalPinManager::instance()};
    std::array<bool, Adc2Pins.size()> acquired{};
    bool allAcquired{true};

    for (std::size_t i{}; i < Adc2Pins.size(); ++i)
    {
        acquired[i] = manager.acquire(physicalPin(Adc2Pins[i]));
        allAcquired = allAcquired && acquired[i];
    }

    if (!allAcquired)
    {
        // Give back only the pins this driver got; the others belong to another driver
        // (e.g. the TMP36 on A6).
        for (std::size_t i{}; i < Adc2Pins.size(); ++i)
        {
            if (acquired[i]) { manager.release(physicalPin(Adc2Pins[i])); }
        }
        std::printf("wifi: init failed: A4-A7 (ADC2) are used by another driver\n");
        return;
    }

    myPinsAcquired = true;

    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
}

// --------------------------------------------------------------------------------
bool Esp32s3::connect() noexcept
{
    constexpr std::uint8_t maxNetworks{10U};
    constexpr std::uint16_t wifiConnectionInitialWaitTime{5000U};
    constexpr std::uint16_t wifiConnectionAttempts{5U};
    constexpr std::uint16_t wifiConnectionAdditionalWaitTime{1000U};

    char ssidsAvailable[maxNetworks][maxNetworkNameLength]{};

    std::uint8_t found = detectNetworks(ssidsAvailable, maxNetworks, maxNetworkNameLength);

    for (std::uint8_t i{}; i < found; i++)
    {
        if (0 != std::strcmp(ssidsAvailable[i], mySsid.c_str())) { continue; }

        std::printf("wifi: found target network: %s\n", mySsid.c_str());
        wifi_config_t wifi_config{};

        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), mySsid.c_str(),
                     sizeof(wifi_config.sta.ssid));
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), myPassword.c_str(),
                     sizeof(wifi_config.sta.password));

        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_wifi_start();
        esp_wifi_connect();

        vTaskDelay(pdMS_TO_TICKS(wifiConnectionInitialWaitTime));
        for (std::uint8_t retry{0}; retry < wifiConnectionAttempts; retry++)
        {
            wifi_ap_record_t ap_info;
            if (ESP_OK == esp_wifi_sta_get_ap_info(&ap_info))
            {
                std::printf("wifi: connected to %s\n", mySsid.c_str());
                return true;
            }
            vTaskDelay(pdMS_TO_TICKS(wifiConnectionAdditionalWaitTime));
        }
    }

    std::printf("wifi: failed to connect to %s\n", mySsid.c_str());
    return false;
}

// --------------------------------------------------------------------------------
uint8_t Esp32s3::detectNetworks(char ssidList[][maxNetworkNameLength], std::uint8_t maxNetworks,
                                std::uint8_t maxSsidLength) noexcept
{
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    wifi_scan_config_t scan_config{};
    esp_wifi_scan_start(&scan_config, true);

    uint16_t found{0};
    esp_wifi_scan_get_ap_num(&found);
    if (found > maxNetworks) found = maxNetworks;

    auto records = std::make_unique<wifi_ap_record_t[]>(found);
    esp_wifi_scan_get_ap_records(&found, records.get());

    for (uint16_t i = 0; i < found; i++)
    {
        strncpy(ssidList[i], reinterpret_cast<char*>(records[i].ssid), maxSsidLength - 1);
        ssidList[i][maxSsidLength - 1] = '\0';
        printf("wifi: detected network: %s\n", ssidList[i]);
    }

    return static_cast<uint8_t>(found);
}
} // namespace comm::wifi
