#include <zephyr/kernel.h>

#include "driver/timer/zephyr.h"

int main()
{
    driver::timer::Zephyr timer{};
    timer.setPeriod(500);
    timer.start();

    while (true)
    {
        if (timer.timeout()) { printk("%lld\n", static_cast<long long>(k_uptime_get())); }
        k_msleep(1);
    }
    return 0;
}