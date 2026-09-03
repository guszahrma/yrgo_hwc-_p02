//! @note File header missing.
#pragma once

//! @note Sort headers.
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
    //! @note Mark explicit.
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
    //! @note camelCase.
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
        //! @note Can be placed on one line if desired.
        if (!myStarted)
        {
            return false;
        }

        //! @note Do not fix auto and {}; as I wrote before, it's hard for the compiler to know if
        //!       something like {100U} is an integer or an array/a list holding one argument.
        //!       Since C++17 this has improved (and that's why you can use it here, who wants
        //!       an array with space for only one argument anyway), but please use auto and =.
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
    //! @note Good choice to use an alias here; way easier to read in my opinion.
    using Clock = std::chrono::steady_clock;

    //! @note Sort member variables by size (largest first => myLastTime -> myPeriodMs -> myStarted).
    std::uint32_t myPeriodMs;
    bool myStarted;
    Clock::time_point myLastTime;
};
} // namespace driver::timer
