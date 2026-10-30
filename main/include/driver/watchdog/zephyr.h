/**
 * @file Zephyr watchdog timer header
 */
#pragma once

#include <cstdint>
#include <zephyr/device.h>

#include "driver/watchdog/interface.h"

namespace driver::watchdog
{
/**
 * @brief Zephyr watchdog driver.
 *
 * Uses the watchdog device from devicetree (alias watchdog0 = wdt0, Timer Group 0) and resets the
 * whole chip when it is not kicked in time. Note: Zephyr's ESP32 driver uses two stages with the
 * same time, so the reset comes after about twice the configured timeout.
 */
class Zephyr final : public Interface
{
public:
    /**
     * @brief Constructor
     * 
     * @param[in] wdt Watchdog timer device
     * @param[in] timeoutMs Timeout to set in milliseconds
     */
    explicit Zephyr(const device* wdt, std::uint32_t timeoutMs) noexcept;

    /**
     * @brief Default destructor
     */
    ~Zephyr() noexcept override = default;

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

    Zephyr()                         = delete;
    Zephyr(const Zephyr&)            = delete;
    Zephyr(Zephyr&&)                 = delete;
    Zephyr& operator=(const Zephyr&) = delete;
    Zephyr& operator=(Zephyr&&)      = delete;

private:
    /** Watchdog device from devicetree. */
    const device* myWdt;

    /** Channel returned by wdt_install_timeout(), used by wdt_feed(). */
    int myChannel;

    /** True if the previous reset was caused by a watchdog. */
    bool myCausedLastReset;

    /** True once the watchdog has been set up and started. */
    bool myReady;
};
} // namespace driver::watchdog