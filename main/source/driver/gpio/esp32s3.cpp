//! @note File header missing.

//! @note Sort headers.
#include <cstdio>
#include <cstdint>

#include "driver/gpio/esp32s3.h"
#include "driver/gpio/interface.h"
#include "driver/gpio.h"

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
    //! @note You can use auto here => less "kaka på kaka", since gpio_num_t would only be written once.
    const gpio_num_t pin = static_cast<gpio_num_t>(myPinNumber);
    gpio_reset_pin(pin);

    // Switch case
    switch (myDirection) 
    { 
        case Direction::OUTPUT: 
            //! @note Please write comments in English.
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
//! @note Can be set to default in the header instead of here.
Esp32s3::~Esp32s3() noexcept = default;

// --------------------------------------------------------------------------------
bool Esp32s3::read() noexcept
{
    // Cast and save the state of the pin
    int level{gpio_get_level(static_cast<gpio_num_t>(myPinNumber))};

    //! @note Yoda?
    myState = (level != Low);
    
    std::printf("Read %s from pin %u.\n", (myState ? "High" : "Low"), myPinNumber);
    return myState;
}

// --------------------------------------------------------------------------------
void Esp32s3::write(bool state) noexcept 
{
    if (Direction::OUTPUT == myDirection) 
    { 
        //! @note auto would be nice here instead of gpio_num_t twice, especially since
        //!       you use a cast. Note: auto and {} historically don't work too well together,
        //!       instead auto and = are usually used. The reason for this is simple.
        //!       
        //!       Let's say you are a compiler and you have this line:
        //!       auto x{5};
        //!       
        //!       Is {5} a single integer, or is it maybe an array holding one element?
        //!       Before C++17, most compilers thought it was an array => auto x = 5 solves that.
        //!       After C++17 it works better, but still, if you use auto, please use =.
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
    //! @note Yoda would be pleased.
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
