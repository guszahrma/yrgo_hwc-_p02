//! @note File header missing.
#pragma once

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "driver/adc/interface.h"
#include "driver/pin/stub.h"
#include "driver/pin/manager.h"

namespace driver::adc
{
/**
 *  @brief ADC stub class
*/
//! @note Should be marked final.
class Stub : public Interface, private driver::pin::PinManagerDriverAccess
{
public:
    /**
     * @brief Constructs the ADC stub and acquires the pin.
     * @param pin              The stub ADC pin to use.
     * @param referenceVoltage Full-scale voltage used by read_voltage() (defaults to 5.0 V for stub).
     */
    //! @note Should be marked noexcept.
    explicit Stub(driver::pin::stub::AdcPin pin, float referenceVoltage = defaultVref) 
        : myPin(pin), myValue(defaultValue), myReferenceVoltage(referenceVoltage)
    {
        if (!acquirePin(static_cast<std::uint8_t>(driver::pin::stub::to_number(pin)))) {
            std::printf("ADC Stub construction failed: pin %s already in use or invalid.\n", driver::pin::stub::to_string(pin));
        }
        std::printf("ADC Stub constructed on pin %s, value is set as default to %u.\n", driver::pin::stub::to_string(pin), myValue);
    }

    /** @brief Releases the acquired pin. */
    ~Stub() noexcept override 
    {
        releasePin(static_cast<std::uint8_t>(driver::pin::stub::to_number(myPin)));
        std::printf("ADC Stub destroyed on pin %s.\n", driver::pin::stub::to_string(myPin));
    }

    /**
     * @brief Read function for the value of the ADC channel
     * 
     * @return value read from the ADC channel , between 0 and 4095 (12-bit resolution)
     */
    //! @note camelCase.
    uint16_t read_value() noexcept override
    {
        std::printf("ADC Stub read_value called, current value is %u.\n", myValue);
        return myValue;
    }

    /**
     * @brief Read function for the voltage of the ADC channel
     * 
     * @return value read from the ADC channel coverted to voltage, between 0 and DefaultVref
     */
    //! @note camelCase.
    float read_voltage() noexcept override
    {
        float voltage = static_cast<float>(myValue) / numberOfLevels * myReferenceVoltage;
        std::printf("ADC Stub read_voltage called with vref: %.2f.\n", myReferenceVoltage);
        return voltage;
    }

    // disable default constructor, copy and move constructors and assignment operators
    Stub() = delete;
    Stub(const Stub&) = delete;
    Stub& operator=(const Stub&) = delete;
    Stub(Stub&&) = delete;
    Stub& operator=(Stub&&) = delete;

    /**
     * @brief Test helper: overrides the value returned by read_value().
     * @param value Raw ADC value to inject (0–4095).
     * @return Always true.
     */
    //! @note camelCase.
    bool test_enabler_set_value(std::uint16_t value) noexcept
    {
        std::printf("ADC Stub test_enabler_set_value called with value: %u\n", value);
        myValue = value;
        return true;
    }
private:
    static constexpr float numberOfLevels{4095.0f};
    static constexpr uint16_t defaultValue{0};
    static constexpr float defaultVref{5.0f};
    const driver::pin::stub::AdcPin myPin;
    std::uint16_t myValue;
    float myReferenceVoltage;
};
} // namespace driver::adc
