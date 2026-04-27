/**
 * @brief Esp32s3
 */

#include <cstdio>
#include <cstdint>

#include "driver/gpio/esp32s3.h"
#include "driver/gpio/interface.h"

#include "driver\gpio.h"

namespace driver::gpio
{
// --------------------------------------------------------------------------------
Esp32s3::Esp32s3(std::uint8_t pinNumber, Direction_t direction) noexcept
    : myPinNumber{pinNumber}
    , myDirection{direction}
    , myState{false}
    {
        // Switch case for direction
        switch (myDirection) 
        { 
        case Direction_t::OUTPUT: 
            gpio_set_direction(static_cast<gpio_num_t>(myPinNumber), GPIO_MODE_OUTPUT);
            break;
        case Direction_t::INPUT_PULL_UP:
            gpio_set_direction(static_cast<gpio_num_t(myPinNumber), GPIO_MODE_INPUT);
            gpio_pullup_en(static_cast<gpio_num_t>(myPinNumber));
            break;
        case Direction_t::INPUT_PULL_DOWN:
            gpio_set_direction(static_cast<gpio_num_t(myPinNumber), GPIO_MODE_INPUT);
            gpio_pulldown_en(static_cast<gpio_num_t>(myPinNumber));
            break;
    }
        std::printf("Stub GPIO constructed on pin %u and direction %u\n", pinNumber, direction);
        gpio_reset_pin(static_cast<gpio_num_t>(pinNumber));
    }

// --------------------------------------------------------------------------------
Esp32s3::~Esp32s3() noexcept = default;

// --------------------------------------------------------------------------------
bool Esp32s3::read() noexcept
{
    gpio_get_level(static_cast<gpio_num_t>(myPinNumber));
    std::printf("%s state on pin %u\n", (myState ? "True" : "False"), myPinNumber);
    return myState;
}

// --------------------------------------------------------------------------------
void Esp32s3::write(bool state) noexcept 
{
    if (Direction_t::OUTPUT == myDirection) 
    { 
        gpio_set_level(static_cast<gpio_num_t>(myPinNumber), static_cast<gpio_num_t>(state));
        myState = state;
        std::printf("Writing %s on pin %u\n", (state ? "True" : "False"), myPinNumber);
    }
}
}