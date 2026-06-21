//! @note File header missing.
#pragma once

#include <cstdint>
#include <string>

//! @note Sort headers.
#include "communication/wifi/interface.h"
//! @note Nice informative inline comment.
// TODO: replace this by things stored in memory when memory driver is implemented
// This include file is kept under.gitignore to avoid hardcoding sensitive information in the repository
// check "communication/wifi/esp32s3_local_config_template.h" for syntax of the file to be created by the user
#include "communication/wifi/esp32s3_local_config.h"

namespace communication::wifi
{
//! @note Class documentation missing.
class Esp32s3 final : public Interface
{
public:
    //! @note Same comment here as before regarding passing strings by value.
    Esp32s3(std::string ssid = std::string(defaultSsid), std::string password = std::string(defaultPassword)) noexcept;
    ~Esp32s3() noexcept override = default;

    //! @note I would place these at the bottom of the public segment, but this placement is very
    //!       common as well. In some ways it's better (I would place these methods here if they
    //!       were to be implemented), but I prefer placing the deleted stuff at the bottom.
    //!       First show what you class is "offering", then delete stuff etc.
    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

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
    //! @note std::uint8_t, please avoid magic numbers (use a static constexpr or something).
    uint8_t detectNetworks(char ssidList[][32], uint8_t maxNetworks, uint8_t maxSsidLength) noexcept;

private:
    std::string mySsid;
    std::string myPassword;
};
} // namespace communication::wifi
