/**
 * @file Watchdog driver stub
 */
#pragma once

#include <cstdint>
#include <cstdio>

#include "driver/watchdog/interface.h"

namespace driver::watchdog
{
/**
 * @brief Stub class for watchdog driver
 */
class Stub final : public Interface
{
public:
    /**
     * @brief Constructor
     *
     * @param[in] timeoutMs The timeout to set in milliseconds
     * @param[in] causedLastReset The flag to check whether watchdog caused the last reset or not
     */
    explicit Stub(std::uint32_t timeoutMs, bool causedLastReset = false) noexcept
        : myTimeoutMs{timeoutMs}
        , myKickCount{}
        , myCausedLastReset{causedLastReset}
    {
        if (0U == myTimeoutMs)
        {
            std::printf("Can't create watchdog timer with no timeout\n");
            return;
        }

        std::printf("Stub watchdog timer started, timeout %u ms\n",
                    static_cast<unsigned>(timeoutMs));
    }

    /**
     * @brief Destructor
     */
    ~Stub() noexcept override = default;

    /**
     * @brief Reset the watchdog timer. Must be called more often than the timeout
     */
    void kick() noexcept override { ++myKickCount; }

    /**
     * @brief Check if the last reset was caused by the watchdog
     *
     * @return True if last reset was caused by the watchdog, false otherwise
     */
    bool causedLastReset() const noexcept override { return myCausedLastReset; }

    /**
     * @brief Check the kick count
     *
     * @return Number of kick counts recorded
     */
    std::uint32_t kickCount() const noexcept { return myKickCount; }

    /**
     * @brief Check the set timeout
     *
     * @return The timeout in milliseconds
     */
    std::uint32_t timeoutMs() const noexcept { return myTimeoutMs; }

    /**
     * @brief Simulate a watchdog reset
     *
     * @param[in] state State to set
     */
    void simulateWatchdogReset(bool state) noexcept { myCausedLastReset = state; }

    Stub()                       = delete;
    Stub(const Stub&)            = delete;
    Stub(Stub&&)                 = delete;
    Stub& operator=(const Stub&) = delete;
    Stub& operator=(Stub&&)      = delete;

private:
    /** Timeout in ms */
    std::uint32_t myTimeoutMs;

    /** The number of kicks */
    std::uint32_t myKickCount;

    /** Save if last reset was made by watchdog */
    bool myCausedLastReset;
};
} // namespace driver::watchdog