#include <cstdio>
#include <cstring>

//! @note Include <utility> to use std::move().

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
//! @note Should be included before "esp_event.h" in my opinion. But maybe you sort like this:
//!       Standard headers -> ESP32-S3 headers -> local headers. That's OK.
#include "communication/wifi/esp32s3.h"

namespace communication::wifi
{

// --------------------------------------------------------------------------------
//! @note I won't mention the string thing again, this is fine, since you use std::move().
Esp32s3::Esp32s3(std::string ssid, std::string password) noexcept
    : mySsid(std::move(ssid))
    , myPassword(std::move(password)) 
{}

// --------------------------------------------------------------------------------
void Esp32s3::init() noexcept
{
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
    //! @note std::uint8_t, prefer to use {} over =.
    constexpr uint8_t maxNetworks = 10;
    constexpr uint8_t maxSsidLen  = 32;

    //! @note Good work with constants here. You may skip = though.
    char ssidsAvailable[maxNetworks][maxSsidLen] = {};

    //! @note I won't mention std::uint8_t again, but the same applies below.
    uint8_t found = detectNetworks(ssidsAvailable, maxNetworks, maxSsidLen);

    //! @note Prefer to initialize with {} or {0U}.
    for (uint8_t i = 0; i < found; i++)
    {
        //! @note Prefer to use Yoda notation (0 != std::strcmp()).
        //!       Note that I wrote std::strcmp from <cstring>, not strcmp from <string.h>.
        //!       Please also use a bracket:
        //!       if (0 != std::strcmp(ssidsAvailable[i], mySsid.c_str()) != 0) { continue; }
        if (strcmp(ssidsAvailable[i], mySsid.c_str()) != 0)
            continue;

        printf("wifi: found target network: %s\n", mySsid.c_str());
        //! @note You may skip = and just use {}; I won't mention it again, but the same applies below.
        wifi_config_t wifi_config = {};

        //! @note std::strncopy, please use a static_cast (safer):
        //!       std::strncpy(static_cast<char*>(wifi_config.sta.ssid), mySsid.c_str(), 
        //!                    sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.ssid,     mySsid.c_str(),     sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.password, myPassword.c_str(), sizeof(wifi_config.sta.password));

        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_wifi_start();
        esp_wifi_connect();

        //! @note Avoid magic numbers, please use a constexpr.
        //! @note The same applies for all magic numbers below (5, 1000, 32 etc).
        vTaskDelay(pdMS_TO_TICKS(5000));
        for (uint8_t retry = 0; retry < 5; retry++)
        {
            wifi_ap_record_t ap_info;
            //! @note Yoda.
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
            {
                printf("wifi: connected to %s\n", mySsid.c_str());
                return true;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    printf("wifi: failed to connect to %s\n", mySsid.c_str());
    return false;
}

// --------------------------------------------------------------------------------
uint8_t Esp32s3::detectNetworks(char ssidList[][32], uint8_t maxNetworks, uint8_t maxSsidLength) noexcept
{
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    wifi_scan_config_t scan_config = {};
    esp_wifi_scan_start(&scan_config, true);

    uint16_t found = 0;
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
} // namespace communication::wifi
