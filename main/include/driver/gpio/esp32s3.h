/**
 * @brief Stub
 */
#pragma once

#include <cstdio>

#include "driver/gpio/interface.h"

namespace driver::gpio
{
class Esp32s3 final : public Interface
{
public:
    explicit Esp32s3(std::uint8_t pinNumber, Direction direction) noexcept;

     ~Esp32s3() noexcept override; 

    /**
     * @brief Read function
     * 
     * @return true 
     * @return false 
     */
     bool read() noexcept override;

    /**
     * @brief Write function
     * 
     * @param[in] state 
     */
     void write(bool state) noexcept override;

     void toggle() noexcept override;

private:
    const std::uint8_t myPinNumber;
    const Direction myDirection;
    bool myState;
};
}