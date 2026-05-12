#pragma once

#include <cstdint>

#include "driver/timer/interface.h"

namespace driver::timer
{
/**
 * @brief ESP32-S3 timer class
 */
class Esp32s3 final : public Interface
{
public:
    Esp32s3() noexcept;

    ~Esp32s3() noexcept override;

    /**
     * @brief Start timer
     */
    void start() noexcept override;

    /**
     * @brief Stop timer
     */
    void stop() noexcept override;

    /**
     * @brief Set timeout period in milliseconds
     * 
     * @param[in] period_ms timeout period in milliseconds
     */
    void period(std::uint32_t period_ms) noexcept override;

    /**
     * @brief Check if timeout has occurred
     * 
     * @return true if timeout occurred, otherwise false
     */
    bool timeout() noexcept override;

    /**
     * @brief Check if driver is initialized
     * 
     * @return true if initialized, otherwise false
     */
    bool initialized() noexcept override;

private:
    std::uint32_t myPeriodMs;
    std::uint64_t myLastTimeUs;
    bool myStarted;
    bool myInitialized;
};

}