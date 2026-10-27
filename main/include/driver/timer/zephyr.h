/**
 * @file Zephyr timer driver.
 */

#pragma once

#include <cstdint>

#include "driver/timer/interface.h"

namespace driver::timer
{
/**
 * @brief Timer driver based on the Zephyr kernel uptime.
 */
class Zephyr final : public Interface
{
public:
    Zephyr() noexcept;

    ~Zephyr() noexcept override;

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

    Zephyr(const Zephyr&)            = delete;
    Zephyr(Zephyr&&)                 = delete;
    Zephyr& operator=(const Zephyr&) = delete;
    Zephyr& operator=(Zephyr&&)      = delete;

private:
    std::int64_t myLastTimeMs;
    std::int64_t myPeriodMs;
    bool myStarted;
};
} // namespace driver::timer