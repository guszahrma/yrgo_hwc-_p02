/**
 * @brief TMP36 driver implementation details.
 */
#include <cstdint>

//! @note Sort headers.
#include "driver/tempsensor/tmp36.h"
#include "driver/adc/interface.h"

namespace driver::tempsensor
{
// -----------------------------------------------------------------------------
Tmp36::Tmp36(adc::Interface& adc) noexcept
    : myAdc{adc}  
{}

// -----------------------------------------------------------------------------
bool Tmp36::isInitialized() const noexcept
{
    // No initialization needed for TMP36, always return true.
    return true;
}

// -----------------------------------------------------------------------------
std::int16_t Tmp36::read() const noexcept
{
    // Calculate input voltage from the temperature sensor.
    const double inputVoltage{myAdc.read_voltage()};

    // Convert voltage to temperature (T  = 100 * Vin - 50).
    const double temperature{100.0 * inputVoltage - 50.0};

    // Select value to round with - 0.5 for positive value, -0.5 for negative values.
    const double roundValue{0.0 < temperature ? 0.5 : -0.5};

    // Round to the nearest integer (27.738 => 28).
    return static_cast<std::int16_t>(temperature + roundValue);
}
} // namespace driver::tempsensor
