/**
 * @file GPIO stub driver.
 */

#pragma once

#include <cstdint>
#include <cstdio>

#include "driver/gpio/interface.h"

namespace driver::gpio
{
/**
 * @brief Stub GPIO pin for tests. Same interface as the hardware driver, no real pin.
 */
class Stub final : public Interface
{
public:
    /**
     * @brief Construct a stub GPIO pin.
     *
     * @param[in] pinNumber Logical pin number used in log output.
     * @param[in] direction Pin direction (input or output).
     */
    explicit Stub(std::uint8_t pinNumber, Direction direction) noexcept
        : myPinNumber{pinNumber}
        , myDirection{direction}
        , myState{false}
    {
        constexpr std::size_t DirectionStrSize{20U};
        char directionStr[DirectionStrSize]{};

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

    ~Stub() noexcept override = default;

    /**
     * @brief Read the stubbed pin level.
     *
     * @return true if the stored level is high, otherwise false.
     */
    bool read() noexcept override
    {
        std::printf("%s state on pin %u.\n", (myState ? "True" : "False"), myPinNumber);
        return myState;
    }

    /**
     * @brief Write a level if the stub is an output.
     *
     * @param[in] state true stores high, false stores low.
     */
    void write(bool state) noexcept override
    {
        if (Direction::OUTPUT == myDirection)
        {
            myState = state;
            std::printf("Writing %s on pin %u.\n", (state ? "True" : "False"), myPinNumber);
        }
    }

    /**
     * @brief Toggle the stored level if the stub is an output.
     */
    void toggle() noexcept override
    {
        if (Direction::OUTPUT == myDirection)
        {
            myState = !myState;
            std::printf("Toggling %s on pin %u.\n", (myState ? "True" : "False"), myPinNumber);
        }
    }

    Stub(const Stub&)            = delete;
    Stub(Stub&&)                 = delete;
    Stub& operator=(const Stub&) = delete;
    Stub& operator=(Stub&&)      = delete;

private:
    const std::uint8_t myPinNumber;
    const Direction    myDirection;
    bool               myState;
};
} // namespace driver::gpio