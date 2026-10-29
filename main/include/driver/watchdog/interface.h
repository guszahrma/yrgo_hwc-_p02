/**
 * @file Watchdog driver interface
 */
#pragma once

namespace driver::watchdog
{
/**
 * @brief Interface class for watchdog driver
 */
class Interface
{
public:
    /**
     * @brief Destructor
     */
    virtual ~Interface() noexcept = default;

    /**
     * @brief Reset the watchdog timer. Must be called more often than the timeout
     */
    virtual void kick() noexcept = 0;

    /**
     * @brief Check if the last reset was caused by the watchdog
     * 
     * @return True if last reset was caused by the watchdog, false otherwise
     */
    virtual bool causedLastReset() const noexcept = 0;
};
} // namespace driver::watchdog