#pragma once

#include <cstdio>

#include "driver/gpio/interface.h"

namespace driver::gpio
{
class Stub final : public Interface
{
public:
    explicit Stub(std::uint8_t pinNumber, Direction_t direction) noexcept
    : myPinNumber{pinNumber}
    , myDirection{direction}
    , myState{false}
    {
        std::printf("Stub GPIO constructed on pin %u and direction %u\n", pinNumber, direction);
    }

     ~Stub() noexcept override = default;

    /**
     * @brief Read function
     * 
     * @return true 
     * @return false 
     */
     bool read() noexcept override
     {
        // Todo Ska vi implementera en check av direction?

        return myState;
     }

    /**
     * @brief Write function
     * 
     * @param state 
     */
     void write(bool state) noexcept override
     {
        if (OUTPUT == myDirection ) { std::printf("Writing %s on pin: %u\n", (state ? "True" : "False"), myPinNumber); }
     }


private:
    const std::uint8_t myPinNumber;
    const Direction_t myDirection;
    bool myState;
};
}