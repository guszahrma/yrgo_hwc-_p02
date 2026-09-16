/**
 * @file ESP32-S3 timer driver.
 */

#pragma once

#include <cstdint>

#include "driver/timer/interface.h"

namespace driver::timer
{
/**
 * @brief Hardware timer driver for the ESP32-S3.
 */
class Esp32s3 final : public Interface
{
public:
    Esp32s3() noexcept;

    ~Esp32s3() noexcept override;

    /**
     * @brief Start the timer.
     */
    void start() noexcept override;

    /**
     * @brief Stop the timer.
     */
    void stop() noexcept override;

    /**
     * @brief Set the timeout period.
     *
     * @param[in] periodMs Period in milliseconds.
     */
    void setPeriod(std::uint32_t periodMs) noexcept override;

    /**
     * @brief Check whether the timeout has elapsed.
     *
     * @return true if a timeout has occurred, otherwise false.
     */
    bool timeout() noexcept override;

private:
    std::uint64_t myLastTimeUs;
    std::uint32_t myPeriodMs;
    bool          myStarted;
};
} // namespace driver::timer