/**
 * @file the esp32s3 specific WiFi interface
 */
#pragma once

#include <cstdint>
#include <string>

#include "communication/wifi/interface.h"

// @TODO: replace this by things stored in memory when memory driver is implemented
// This include file is kept under.gitignore to avoid hardcoding sensitive information in the
// repository check "communication/wifi/esp32s3_local_config_template.h" for syntax of the file to
// be created by the user
#include "communication/wifi/esp32s3_local_config.h"

namespace comm::wifi
{

namespace
{
constexpr std::uint8_t maxNetworkNameLength{32};

}
/**
 * @brief wifi driver for the esp32s3
 */
class Esp32s3 final : public Interface
{
public:
    Esp32s3(std::string ssid     = std::string(defaultSsid),
            std::string password = std::string(defaultPassword)) noexcept;
    ~Esp32s3() noexcept override = default;

    /**
     *  @brief Initialize the ESP32-S3 WiFi module.
     */
    void init() noexcept override;

    /**
     *  @brief Connect to the WiFi network.
     *  @return true if connected, false otherwise.
     */
    bool connect() noexcept override;

    /**
     *  @brief Detect available WiFi networks.
     *  @param ssidList Array to store detected SSIDs.
     *  @param maxNetworks Maximum number of networks to detect.
     *  @param maxSsidLength Maximum length of each SSID.
     *  @return Number of detected networks.
     */
    uint8_t detectNetworks(char ssidList[][maxNetworkNameLength], std::uint8_t maxNetworks,
                           std::uint8_t maxSsidLength) noexcept;

    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

private:
    std::string mySsid;
    std::string myPassword;
};
} // namespace comm::wifi
