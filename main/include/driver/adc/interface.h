//! @note Missing file header.
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
    //! @note Please use std::uint16_t and camelCase, i.e. std::uint16_t readValue();
    virtual uint16_t read_value() noexcept = 0;

    /**
     * @brief Read function for the voltage of the ADC channel
     */
    //! @note Use camelCase here as well.
    virtual float read_voltage() noexcept = 0;
};
} // namespace driver::adc
