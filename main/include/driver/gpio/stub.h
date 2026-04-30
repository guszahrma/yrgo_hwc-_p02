/**
 * @brief Stub
 */
#pragma once

#include <cstdio>

#include "driver/gpio/interface.h"

namespace driver::gpio
{
/**
 * @brief Stub class for GPIO interface. This class is used for testing and debugging purposes.
 * It simulates the behavior of a GPIO pin without interacting with actual hardware.
 */
class Stub final : public Interface
{
public:
    /**
     * @brief Construct a new Stub object
     * 
     * @param[in] pinNumber 
     * @param[in] direction 
     */
    explicit Stub(std::uint8_t pinNumber, Direction direction) noexcept
    : myPinNumber{pinNumber}
    , myDirection{direction}
    , myState{false}
    {
        char directionStr[20];
        switch (direction)
        {
            case Direction::OUTPUT:
                std::snprintf(directionStr, sizeof(directionStr), "OUTPUT");
                break;
            case Direction::INPUT_PULL_UP:
                std::snprintf(directionStr, sizeof(directionStr), "INPUT_PULL_UP");
                break;
            case Direction::INPUT_PULL_DOWN:
                std::snprintf(directionStr, sizeof(directionStr), "INPUT_PULL_DOWN");
                break;
            default:
                std::snprintf(directionStr, sizeof(directionStr), "UNKNOWN");
                break;
        }
        std::printf("Stub GPIO constructed on pin %u and direction %s.\n", pinNumber, directionStr);
    }

    /**
     * @brief Destroy the Stub object
     */
    ~Stub() noexcept override = default;

    /**
     * @brief Read function
     * 
     * @return true 
     * @return false 
     */
    bool read() noexcept override
    {
        std::printf("%s state on pin %u\n", (myState ? "True" : "False"), myPinNumber);
        return myState;
    }

    /**
     * @brief Write function
     * 
     * @param[in] state 
     */
    void write(bool state) noexcept override
    {
        if (Direction::OUTPUT == myDirection ) 
        { 
            myState = state;
            std::printf("Writing %s on pin %u\n", (state ? "True" : "False"), myPinNumber);
        }
    }

    /**
     * @brief Toggle function
     */
    void toggle() noexcept override
    {
        if (Direction::OUTPUT == myDirection)
        {
            myState = !myState;
            std::printf("Toggling %s on pin %u.\n", (myState ? "True" : "False"), myPinNumber);
        }
    }

private:
    const std::uint8_t myPinNumber;
    const Direction myDirection;
    bool myState;
};
}