/**
 * @file Zephyr watchdog timer implementation
 */
#include <cstdint>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>

#include "driver/watchdog/zephyr.h"

namespace driver::watchdog
{
// --------------------------------------------------------------------------------
Zephyr::Zephyr(const device* wdt, std::uint32_t timeoutMs) noexcept
    : myWdt{wdt}
    , myChannel{-1}
    , myCausedLastReset{false}
    , myReady{false}
{
    std::uint32_t cause{};

    if (0 == hwinfo_get_reset_cause(&cause))
    {
        myCausedLastReset = (0U != (cause & RESET_WATCHDOG));
    }

    if (0U == timeoutMs) 
    {
        printk("Can't create watchdog timer with no timeout\n");
        return;
    }

    if ((nullptr == myWdt) || (!device_is_ready(myWdt)))
    {
        printk("watchdog: device is not ready\n");
        return;
    }

    wdt_timeout_cfg cfg{};
    cfg.window.min = 0U;
    cfg.window.max = timeoutMs;
    cfg.callback = nullptr;
    cfg.flags = WDT_FLAG_RESET_SOC;

    myChannel = wdt_install_timeout(myWdt, &cfg);
    if (0 > myChannel)
    {
        printk("watchdog: install timeout failed: %d\n", myChannel);
        return;
    }

    const int err = wdt_setup(myWdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
    if (0 > err) 
    {
        printk("watchdog: setup failed: %d\n", err);
        return;
    }

    myReady = true;
    printk("Zephyr watchdog started, timeout %u ms\n", static_cast<unsigned>(timeoutMs));
}

// --------------------------------------------------------------------------------
void Zephyr::kick() noexcept
{
    if (!myReady) { return; }
    
    const int err{wdt_feed(myWdt, myChannel)};
    if (0 > err)
    {
        printk("watchdog: feed failed: %d\n", err);
    }
}

// --------------------------------------------------------------------------------
bool Zephyr::causedLastReset() const noexcept
{
    return myCausedLastReset;
}
} // namespace driver::watchdog