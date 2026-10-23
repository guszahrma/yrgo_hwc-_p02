#include <zephyr/kernel.h>

#include "driver/gpio/zephyr.h"
#include "driver/timer/zephyr.h"

// P02's status LED: alias p02-led in app.overlay (D6 via the Arduino Nano header).
static const gpio_dt_spec ledSpec = GPIO_DT_SPEC_GET(DT_ALIAS(p02_led), gpios);

int main()
{
    driver::gpio::Zephyr led{ledSpec, driver::gpio::Direction::OUTPUT};
    driver::timer::Zephyr timer{};
    timer.setPeriod(500);
    timer.start();

    while (true)
    {
        if (timer.timeout()) { led.toggle(); }
        k_msleep(1);
    }
    return 0;
}