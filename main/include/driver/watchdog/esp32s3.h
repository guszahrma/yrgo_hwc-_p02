/**
 * @file ESP32s3 watchdog driver
 */
#pragma once

#include <cstdint>

#include "driver/watchdog/interface.h"

namespace driver::watchdog
{
/**
 * @brief Esp32s3 watchdog driver class
 *
 */
class Esp32s3 final : public Interface
{
public:
    /**
     * @brief Constructor
     *
     * @param[in] timeoutMs The timeout to set in milliseconds
     */
    explicit Esp32s3(std::uint32_t timeoutMs) noexcept;

    /**
     * @brief Destructor
     */
    ~Esp32s3() noexcept override;

    /**
     * @brief Reset the watchdog timer. Must be called more often than the timeout
     */
    void kick() noexcept override;

    /**
     * @brief Check if the last reset was caused by the watchdog
     *
     * @return True if last reset was caused by the watchdog, false otherwise
     */
    bool causedLastReset() const noexcept override;

    Esp32s3()                          = delete;
    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

private:
    /** Timeout in ms */
    std::uint32_t myTimeoutMs;

    /** Save if last reset was made by watchdog */
    bool myCausedLastReset;

    /** True if setup succeeded */
    bool mySubscribed;
};
} // namespace driver::watchdog