#pragma once

#include <cstdint>

namespace driver::adc
{
/**
 *  @brief ADC interface class 
*/
class Interface
{
public:

    virtual ~Interface() noexcept = default;

    /**
     * @brief Read function for the value of the ADC channel
     * 
     * @return value read from the ADC channel , between 0 and 4095 (12-bit resolution)
     */
    virtual uint16_t read_value() noexcept = 0;

    /**
     * @brief Read function for the voltage of the ADC channel
     */
    virtual float read_voltage() noexcept = 0;
};
}