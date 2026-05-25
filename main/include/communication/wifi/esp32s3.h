#pragma once

#include <cstdint>
#include <string>
#include "communication/wifi/interface.h"
// TODO: replace this by things stored in memory when memory driver is implemented
// This include file is kept under.gitignore to avoid hardcoding sensitive information in the repository
// check "communication/wifi/esp32s3_local_config_template.h" for syntax of the file to be created by the user
#include "communication/wifi/esp32s3_local_config.h"

namespace communication::wifi
{
class Esp32s3 final : public Interface
{
public:
    Esp32s3(std::string ssid = std::string(defaultSsid), std::string password = std::string(defaultPassword));
    ~Esp32s3() noexcept override = default;

    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

    /**
     *  @brief Initialize the ESP32-S3 WiFi module.
     */
    void    init()    noexcept override;

    /**
     *  @brief Connect to the WiFi network.
     *  @return true if connected, false otherwise.
     */
    bool    connect() noexcept override;

    /**
     *  @brief Detect available WiFi networks.
     *  @param ssidList Array to store detected SSIDs.
     *  @param maxNetworks Maximum number of networks to detect.
     *  @param maxSsidLength Maximum length of each SSID.
     *  @return Number of detected networks.
     */
    uint8_t detectNetworks(char ssidList[][32], uint8_t maxNetworks, uint8_t maxSsidLength) noexcept;

private:
    std::string mySsid;
    std::string myPassword;
};
} // namespace communication::wifi
