#pragma once

#include <cstdint>
#include <cstdio>
#include <chrono>

#include "driver/timer/interface.h"

namespace driver::timer
{
/**
 * @brief Stub class for timer interface. This class is used for testing and debugging purposes.
 * It simulates the behavior of a timer without interacting with actual hardware.
 */
class Stub final : public Interface
{
public:
    /**
     * @brief Construct a new Stub object
     */
    Stub(std::uint32_t period_ms = 0, bool start_running = false) noexcept
        : myPeriodMs{period_ms}
        , myStarted{start_running}
        , myLastTime{Clock::now()}
    {
        std::printf("Stub timer constructed.\n");
    }

    /**
     * @brief Destroy the Stub object
     */
    ~Stub() noexcept override = default;

    /**
     * @brief Start timer
     */
    void start() noexcept override
    {
        myStarted = true;
        myLastTime = Clock::now();
        std::printf("Stub timer started.\n");
    }

    /**
     * @brief Stop timer
     */
    void stop() noexcept override
    {
        myStarted = false;
        std::printf("Stub timer stopped.\n");
    }

    /**
     * @brief Set timeout period in milliseconds
     * 
     * @param[in] period_ms timeout period in milliseconds
     */
    void set_period(std::uint32_t period_ms) noexcept override
    {
        myPeriodMs = period_ms;
        std::printf("Stub timer period set to %u ms.\n", static_cast<unsigned>(myPeriodMs));
    }

    /**
     * @brief Check if timeout has occurred
     * 
     * @return true if timeout occurred, otherwise false
     */
    bool timeout() noexcept override
    {
        if (!myStarted)
        {
            return false;
        }

        const auto currentTime{Clock::now()};
        const auto elapsedMs{
            std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - myLastTime).count()
        };

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

    std::uint32_t myPeriodMs;
    bool myStarted;
    Clock::time_point myLastTime;
};

}