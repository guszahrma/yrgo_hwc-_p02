/**
 * @file Timer stub driver.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>

#include "driver/timer/interface.h"

namespace driver::timer
{
/**
 * @brief Stub timer for tests. Same interface as the hardware driver, uses the host clock.
 */
class Stub final : public Interface
{
public:
    /**
     * @brief Construct a stub timer.
     *
     * @param[in] periodMs Timeout period in milliseconds.
     * @param[in] startRunning If true, the timer starts immediately.
     */
    explicit Stub(std::uint32_t periodMs = 0, bool startRunning = false) noexcept
        : myLastTime{Clock::now()}
        , myPeriodMs{periodMs}
        , myStarted{startRunning}
    {
        std::printf("Stub timer constructed.\n");
    }

    ~Stub() noexcept override = default;

    /**
     * @brief Start the stub timer.
     */
    void start() noexcept override
    {
        myStarted  = true;
        myLastTime = Clock::now();
        std::printf("Stub timer started.\n");
    }

    /**
     * @brief Stop the stub timer.
     */
    void stop() noexcept override
    {
        myStarted = false;
        std::printf("Stub timer stopped.\n");
    }

    /**
     * @brief Set the timeout period.
     *
     * @param[in] periodMs Period in milliseconds.
     */
    void setPeriod(std::uint32_t periodMs) noexcept override
    {
        myPeriodMs = periodMs;
        std::printf("Stub timer period set to %u ms.\n", static_cast<unsigned>(myPeriodMs));
    }

    /**
     * @brief Check whether the timeout has elapsed.
     *
     * @return true if a timeout has occurred, otherwise false.
     */
    bool timeout() noexcept override
    {
        if (!myStarted) { return false; }

        const auto currentTime = Clock::now();
        const auto elapsedMs   =
            std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - myLastTime).count();

        if (elapsedMs >= myPeriodMs)
        {
            myLastTime = currentTime;
            std::printf("Stub timer timeout detected.\n");
            return true;
        }

        return false;
    }

private:
    using Clock = std::chrono::steady_clock;

    Clock::time_point myLastTime;
    std::uint32_t     myPeriodMs;
    bool              myStarted;
};
} // namespace driver::timer