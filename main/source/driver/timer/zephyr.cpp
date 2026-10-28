/**
 * @file Zephyr timer driver implementation.
 */

#include <cstdint>
#include <zephyr/kernel.h>

#include "driver/timer/zephyr.h"

namespace driver::timer
{
// --------------------------------------------------------------------------------
Zephyr::Zephyr() noexcept
    : myLastTimeMs{0}
    , myPeriodMs{500}
    , myStarted{false}
{ printk("Zephyr timer initialized.\n"); }

// --------------------------------------------------------------------------------
Zephyr::~Zephyr() noexcept = default;

// --------------------------------------------------------------------------------
void Zephyr::start() noexcept
{
    myStarted    = true;
    myLastTimeMs = k_uptime_get();
    printk("Zephyr timer started.\n");
}

// --------------------------------------------------------------------------------
void Zephyr::stop() noexcept
{
    myStarted    = false;
    myLastTimeMs = 0;
    printk("Zephyr timer stopped.\n");
}

// --------------------------------------------------------------------------------
void Zephyr::setPeriod(std::uint32_t periodMs) noexcept
{
    myPeriodMs = static_cast<std::int64_t>(periodMs);
    printk("Zephyr timer period set to %u ms.\n", static_cast<unsigned>(myPeriodMs));
}

// --------------------------------------------------------------------------------
bool Zephyr::timeout() noexcept
{
    if (!myStarted) { return false; }

    const std::int64_t currentTimeMs{k_uptime_get()};

    if (myPeriodMs <= (currentTimeMs - myLastTimeMs))
    {
        // Restart from now (not from the previous deadline): if timeout() is called late,
        // the next period starts over instead of firing several times to catch up.
        myLastTimeMs = currentTimeMs;
        printk("Zephyr timer timeout detected.\n");
        return true;
    }

    return false;
}
} // namespace driver::timer