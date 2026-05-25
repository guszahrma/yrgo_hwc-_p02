#include <cstdio>
#include <cstring>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "communication/wifi/esp32s3.h"

namespace communication::wifi
{

Esp32s3::Esp32s3(std::string ssid, std::string password)
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
    constexpr uint8_t maxNetworks = 10;
    constexpr uint8_t maxSsidLen  = 32;
    char ssidsAvailable[maxNetworks][maxSsidLen] = {};

    uint8_t found = detectNetworks(ssidsAvailable, maxNetworks, maxSsidLen);

    for (uint8_t i = 0; i < found; i++)
    {
        if (strcmp(ssidsAvailable[i], mySsid.c_str()) != 0)
            continue;

        printf("wifi: found target network: %s\n", mySsid.c_str());
        wifi_config_t wifi_config = {};
        strncpy((char*)wifi_config.sta.ssid,     mySsid.c_str(),     sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.password, myPassword.c_str(), sizeof(wifi_config.sta.password));

        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_wifi_start();
        esp_wifi_connect();

        vTaskDelay(pdMS_TO_TICKS(5000));
        for (uint8_t retry = 0; retry < 5; retry++)
        {
            wifi_ap_record_t ap_info;
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

    wifi_ap_record_t* records = new wifi_ap_record_t[found];
    esp_wifi_scan_get_ap_records(&found, records);

    for (uint16_t i = 0; i < found; i++)
    {
        strncpy(ssidList[i], (char*)records[i].ssid, maxSsidLength - 1);
        ssidList[i][maxSsidLength - 1] = '\0';
        printf("wifi: detected network: %s\n", ssidList[i]);
    }

    delete[] records;
    return static_cast<uint8_t>(found);
}

} // namespace communication::wifi
