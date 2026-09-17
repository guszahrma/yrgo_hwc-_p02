/**
 * @file ESP32-S3 GPIO driver implementation.
 */

#include <cstdint>
#include <cstdio>

#include "arch/env/hw_platform.h"
#include "driver/gpio/esp32s3.h"
#include "driver/gpio/interface.h"

namespace driver::gpio
{
namespace
{
constexpr int High{1};
constexpr int Low{0};
} // namespace

// --------------------------------------------------------------------------------
Esp32s3::Esp32s3(std::uint8_t pinNumber, Direction direction) noexcept
    : myPinNumber{pinNumber}
    , myDirection{direction}
    , myState{false}
{
    const auto pin = static_cast<gpio_num_t>(myPinNumber);
    gpio_reset_pin(pin);

    switch (myDirection)
    {
        case Direction::OUTPUT:
            // Input/output so we can read back what we wrote.
            gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT);
            break;

        case Direction::INPUT_PULL_UP:
            gpio_set_direction(pin, GPIO_MODE_INPUT);
            gpio_pullup_en(pin);
            gpio_pulldown_dis(pin);
            break;

        case Direction::INPUT_PULL_DOWN:
            gpio_set_direction(pin, GPIO_MODE_INPUT);
            gpio_pulldown_en(pin);
            gpio_pullup_dis(pin);
            break;
    }

    std::printf("ESP32-S3 GPIO initialized on pin %u.\n", myPinNumber);
}

// --------------------------------------------------------------------------------
bool Esp32s3::read() noexcept
{
    const auto level = gpio_get_level(static_cast<gpio_num_t>(myPinNumber));

    myState = (Low != level);

    std::printf("Read %s from pin %u.\n", (myState ? "High" : "Low"), myPinNumber);
    return myState;
}

// --------------------------------------------------------------------------------
void Esp32s3::write(bool state) noexcept
{
    if (Direction::OUTPUT == myDirection)
    {
        const auto pin = static_cast<gpio_num_t>(myPinNumber);

        gpio_set_level(pin, state ? High : Low);

        myState = state;
        std::printf("Wrote %s to pin %u.\n", (myState ? "High" : "Low"), myPinNumber);
    }
    else
    {
        std::printf("Warning: Attempted to write to input pin %u.\n", myPinNumber);
    }
}

// --------------------------------------------------------------------------------
void Esp32s3::toggle() noexcept
{
    if (Direction::OUTPUT == myDirection)
    {
        write(!myState);
        std::printf("Toggling %s on pin %u.\n", (myState ? "True" : "False"), myPinNumber);
    }
    else
    {
        std::printf("Warning: Cannot toggle pin %u, it is an input.\n", myPinNumber);
    }
}
} // namespace driver::gpio