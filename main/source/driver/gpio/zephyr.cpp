/**
 * @file Zephyr GPIO driver implementation.
 */
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "driver/gpio/zephyr.h"

namespace driver::gpio
{
// --------------------------------------------------------------------------------
Zephyr::Zephyr(const gpio_dt_spec& spec, Direction direction) noexcept
    : myPin{spec}
    , myDirection{direction}
    , myState{false}
    , myReady{false}
{
    if (!gpio_is_ready_dt(&myPin))
    {
        printk("Zephyr GPIO: controller for pin %u is not ready.\n",
               static_cast<unsigned>(myPin.pin));
        return;
    }

    gpio_flags_t flags{};

    switch (myDirection)
    {
        case Direction::OUTPUT:
            // Input/output so we can read back what we wrote.
            flags = GPIO_INPUT | GPIO_OUTPUT_INACTIVE;
            break;

        case Direction::INPUT_PULL_UP:
            flags = GPIO_INPUT | GPIO_PULL_UP;
            break;

        case Direction::INPUT_PULL_DOWN:
            flags = GPIO_INPUT | GPIO_PULL_DOWN;
            break;
    }

    const int ret{gpio_pin_configure_dt(&myPin, flags)};

    if (0 > ret)
    {
        printk("Zephyr GPIO: failed to configure pin %u (error %d)\n",
               static_cast<unsigned>(myPin.pin), ret);
        return;
    }

    myReady = true;
    printk("Zephyr GPIO initialized on pin %u\n", static_cast<unsigned>(myPin.pin));
}

// --------------------------------------------------------------------------------
bool Zephyr::read() noexcept
{
    if (!myReady) { return false; }

    // Return 1 (active), 0 (inactive) or a negative error code.
    const int ret{gpio_pin_get_dt(&myPin)};

    if (0 > ret)
    {
        printk("Zephyr GPIO: failed to read pin %u (error %d)\n", static_cast<unsigned>(myPin.pin),
               ret);
        return false;
    }

    myState = (0 != ret);
    printk("Read %s from pin %u\n", (myState ? "active" : "inactive"),
           static_cast<unsigned>(myPin.pin));

    return myState;
}

// --------------------------------------------------------------------------------
void Zephyr::write(bool state) noexcept
{
    if (!myReady) { return; }

    if (Direction::OUTPUT != myDirection)
    {
        printk("Warning: attempted to write to input pin %u.\n", static_cast<unsigned>(myPin.pin));
        return;
    }

    const int ret{gpio_pin_set_dt(&myPin, state ? 1 : 0)};

    if (0 > ret)
    {
        printk("Zephyr GPIO: failed to write pin %u (error %d).\n",
               static_cast<unsigned>(myPin.pin), ret);
        return;
    }

    // Only update the state once the write has succeeded.
    myState = state;
    printk("Wrote %s to pin %u.\n", (myState ? "active" : "inactive"),
           static_cast<unsigned>(myPin.pin));
}

// --------------------------------------------------------------------------------
void Zephyr::toggle() noexcept
{
    // write() already checks if the pin is ready and is configured as an output.
    write(!myState);
}
} // namespace driver::gpio