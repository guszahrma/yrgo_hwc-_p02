/**
 * @brief Stub
 */
#pragma once

#include <cstdio>

#include "driver/gpio/interface.h"

namespace driver::gpio
{
/**
 * @brief ESP32-S3 GPIO class
 */
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

     /**
      * @brief Toggle function
      * switches the state of the pin from high to low or from low to high
      */
     void toggle() noexcept override;

private:
    const std::uint8_t myPinNumber;
    const Direction myDirection;
    bool myState;
};
}