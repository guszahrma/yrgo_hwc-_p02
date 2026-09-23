#include <cstdio>
#include <cstring>
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

namespace comm::wifi
{

// --------------------------------------------------------------------------------
Esp32s3::Esp32s3(std::string ssid, std::string password) noexcept
    : mySsid(std::move(ssid))
    , myPassword(std::move(password))
{}

// --------------------------------------------------------------------------------
void Esp32s3::init() noexcept
{
    // reserve A4 - A7 if Wifi is used to avoid conflicts
    auto& manager  = driver::pin::PhysicalPinManager::instance();
    bool a4Acquired{manager.acquire(driver::pin::esp32s3::toNumber(driver::pin::esp32s3::AdcPin::A4))};
    bool a5Acquired{manager.acquire(driver::pin::esp32s3::toNumber(driver::pin::esp32s3::AdcPin::A5))};
    bool a6Acquired{manager.acquire(driver::pin::esp32s3::toNumber(driver::pin::esp32s3::AdcPin::A6))};
    bool a7Acquired{manager.acquire(driver::pin::esp32s3::toNumber(driver::pin::esp32s3::AdcPin::A7))};

    // if all relevant pins can be acquired, then initialize wifi
    if (a4Acquired && a5Acquired && a6Acquired && a7Acquired)
    {
        nvs_flash_init();
        esp_netif_init();
        esp_event_loop_create_default();
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);

    }
    else 
    {
        if (!a4Acquired) {manager.release(driver::pin::esp32s3::toNumber(driver::pin::esp32s3::AdcPin::A4));}
        if (!a5Acquired) {manager.release(driver::pin::esp32s3::toNumber(driver::pin::esp32s3::AdcPin::A5));}
        if (!a6Acquired) {manager.release(driver::pin::esp32s3::toNumber(driver::pin::esp32s3::AdcPin::A6));}
        if (!a7Acquired) {manager.release(driver::pin::esp32s3::toNumber(driver::pin::esp32s3::AdcPin::A7));}
    }

}

// --------------------------------------------------------------------------------
bool Esp32s3::connect() noexcept
{
    constexpr std::uint8_t maxNetworks{10};
    constexpr std::uint8_t maxSsidLen{32};
    constexpr std::uint16_t wifiConnectionInitialWaitTime{5000};
    constexpr std::uint16_t wifiConnectionAttempts{5};
    constexpr std::uint16_t wifiConnectionAdditionalWaitTime{1000};

    char ssidsAvailable[maxNetworks][maxSsidLen]{};

    std::uint8_t found = detectNetworks(ssidsAvailable, maxNetworks, maxSsidLen);

    for (std::uint8_t i{}; i < found; i++)
    {
        if (0 != std::strcmp(ssidsAvailable[i], mySsid.c_str())) { continue; }

        std::printf("wifi: found target network: %s\n", mySsid.c_str());
        wifi_config_t wifi_config{};

        // @TOD blir detta rätt? uint8 eller char?
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

    //! @note How about using a std::unique_ptr here?
    wifi_ap_record_t* records = new wifi_ap_record_t[found];
    esp_wifi_scan_get_ap_records(&found, records);

    for (uint16_t i = 0; i < found; i++)
    {
        strncpy(ssidList[i], (char*)records[i].ssid, maxSsidLength - 1);
        ssidList[i][maxSsidLength - 1] = '\0';
        printf("wifi: detected network: %s\n", ssidList[i]);
    }

    //! @note Good work! I still would prefer a std::unique_ptr, but you handled it correctly.
    delete[] records;
    return static_cast<uint8_t>(found);
}
} // namespace comm::wifi
