#pragma once

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include "driver/adc/interface.h"

namespace driver::adc
{
/**
 * @brief Pin is a singleton class to prevent use of pins that doesn't exist or are already in use. 
 */

enum class AdcPin : std::uint8_t
{
    D0 = 0,
    D1 = 1,
    D2 = 2,
    D3 = 3,
    D4 = 4,
    D5 = 5,
    D6 = 6,
    D7 = 7,
    Count =8
};

const char* adcPinToString(AdcPin pin) {
    switch (pin) {
        case AdcPin::D0:   return "D0";
        case AdcPin::D1:   return "D1";
        case AdcPin::D2:   return "D2";
        case AdcPin::D3:   return "D3";
        case AdcPin::D4:   return "D4";
        case AdcPin::D5:   return "D5";
        case AdcPin::D6:   return "D6";
        case AdcPin::D7:   return "D7";
        default:          return "UNKNOWN";
    }
}

uint8_t adcPinTouint_8t(AdcPin pin) {
    switch (pin) {
        case AdcPin::D0:   return (uint8_t) 1;
        case AdcPin::D1:   return (uint8_t) 2;
        case AdcPin::D2:   return (uint8_t) 3;
        case AdcPin::D3:   return (uint8_t) 4;
        case AdcPin::D4:   return (uint8_t) 5;
        case AdcPin::D5:   return (uint8_t) 6;
        case AdcPin::D6:   return (uint8_t) 7;
        case AdcPin::D7:   return (uint8_t) 8;
        default:           return (uint8_t) 9;
    }
}


class AdcPinManager {
public:
    static AdcPinManager& instance() {
        static AdcPinManager inst;
        return inst;
    }

    bool acquire(AdcPin pin) {
        auto idx = static_cast<std::size_t>(pin);
        if (idx >= static_cast<std::size_t>(AdcPin::Count) || pinsInUse[idx]) {
            return false;
        }
        pinsInUse[idx] = true;
        return true;
    }

    void release(AdcPin pin) {
        auto idx = static_cast<std::size_t>(pin);
        if (idx < static_cast<std::size_t>(AdcPin::Count)) {
            pinsInUse[idx] = false;
        }
    }

    bool is_in_use(AdcPin pin) const {
        auto idx = static_cast<std::size_t>(pin);
        if (idx >= static_cast<std::size_t>(AdcPin::Count)) return false;
        return pinsInUse[idx];
    }
    // disable copy and move constructors and assignment operators
    AdcPinManager(const AdcPinManager&) = delete;
    AdcPinManager& operator=(const AdcPinManager&) = delete;
    AdcPinManager(AdcPinManager&&) = delete;
    AdcPinManager& operator=(AdcPinManager&&) = delete;

private:
    std::array<bool, static_cast<std::size_t>(AdcPin::Count)> pinsInUse;
    AdcPinManager() { pinsInUse.fill(false); }
};

/**
 *  @brief ADC stub class 
*/

class Stub : public Interface
{

public:
    explicit Stub(AdcPin pin, float referenceVoltage = defaultVref) 
        : myPin(pin), myValue(defaultValue), myReferenceVoltage(referenceVoltage)
    {
        if (!AdcPinManager::instance().acquire(pin)) {
            std::printf("ADC Stub construction failed: pin %u already in use or invalid.\n", static_cast<unsigned>(pin));
            
//            throw(std::runtime_error( "AdcPin " + std::to_string(adcPinToString(pin)) + " already in use.\n"))
        }
        std::printf("ADC Stub constructed on pin %s, value is set as default to %u.\n", adcPinToString(pin), myValue);
    }

    ~Stub() noexcept override 
    {
        AdcPinManager::instance().release(myPin);
        std::printf("ADC Stub destroyed on pin %s.\n", adcPinToString(myPin));
    }

    /**
     * @brief Read function for the value of the ADC channel
     * 
     * @return value read from the ADC channel , between 0 and 4095 (12-bit resolution)
     */
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
    float read_voltage() noexcept override
    {
        float voltage = static_cast<float>(myValue) / numberOfLevels * myReferenceVoltage;
        std::printf("ADC Stub read_voltage called with varef: %.1f.\n", myReferenceVoltage);
        return voltage;
    }

    // disable default constructor, copy and move constructors and assignment operators
    Stub() = delete;
    Stub(const Stub&) = delete;
    Stub& operator=(const Stub&) = delete;
    Stub(Stub&&) = delete;
    Stub& operator=(Stub&&) = delete;

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
    const AdcPin myPin;
    std::uint16_t myValue;
    float myReferenceVoltage;
};
}