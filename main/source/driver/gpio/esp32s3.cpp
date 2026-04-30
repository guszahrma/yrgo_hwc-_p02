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
namespace
{
constexpr int High{1};
constexpr int Low{0};
}

// --------------------------------------------------------------------------------
Esp32s3::Esp32s3(std::uint8_t pinNumber, Direction direction) noexcept
    : myPinNumber{pinNumber}
    , myDirection{direction}
    , myState{false}
{
    // Reset pin
    const gpio_num_t pin = static_cast<gpio_num_t>(myPinNumber);
    gpio_reset_pin(pin);

    // Switch case
    switch (myDirection) 
    { 
        case Direction::OUTPUT: 
            gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT); // Input/Output så vi kan läsa vad vi skrivit
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
Esp32s3::~Esp32s3() noexcept = default;

// --------------------------------------------------------------------------------
bool Esp32s3::read() noexcept
{
    // Cast and save the state of the pin
    int level{gpio_get_level(static_cast<gpio_num_t>(myPinNumber))};
    myState = (level != Low);
    
    std::printf("Read %s from pin %u.\n", (myState ? "High" : "Low"), myPinNumber);
    return myState;
}

// --------------------------------------------------------------------------------
void Esp32s3::write(bool state) noexcept 
{
    if (Direction::OUTPUT == myDirection) 
    { 
        const gpio_num_t pin{static_cast<gpio_num_t>(myPinNumber)};
        
        // Set level
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
}