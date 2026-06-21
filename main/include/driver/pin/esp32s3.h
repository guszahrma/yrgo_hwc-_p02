//! @note File header missing.
#pragma once

#include <cstdint>

//! @note Sort headers.
#include "soc/gpio_num.h"
#include "driver/pin/manager.h"

namespace driver::pin::esp32s3
{
/**
 * @brief ADC1-capable pins on ESP32-S3.
 *        ADC1: GPIO1 (CH0) ... GPIO10 (CH9).
 */
enum class AdcPin : std::uint8_t
{
    //! @note You don't need to explicitly specify the numbers here. I guess you did it to be clear?
    A0 = 0,
    A1 = 1,
    A2 = 2,
    A3 = 3,
    A4 = 4,
    A5 = 5,
    A6 = 6,
    A7 = 7
};

/**
 * @brief GPIO-capable pins on ESP32-S3 (non-ADC pins).
 */
enum class GpioPin : std::uint8_t
{
    D0 = 0,
    D1 = 1,
    D2 = 2,
    D3 = 3,
    D4 = 4,
    D5 = 5,
    D6 = 6,
    D7 = 7,
    D8 = 8,
    D9 = 9,
    D10 = 10,
    D11 = 11,
    D12 = 12,
    D13 = 13
};

/**
 * @brief Returns the Arduino label string for an ADC pin (e.g. "A0").
 * @param [in] pin The ADC pin.
 * @return String literal, never null.
 */
//! @note Use camelCase and mark noexcept.
inline const char* to_string(AdcPin pin)
{
    switch (pin)
    {
        case AdcPin::A0: return "A0";
        case AdcPin::A1: return "A1";
        case AdcPin::A2: return "A2";
        case AdcPin::A3: return "A3";
        case AdcPin::A4: return "A4";
        case AdcPin::A5: return "A5";
        case AdcPin::A6: return "A6";
        case AdcPin::A7: return "A7";
        default:         return "UNKNOWN";
   }
}

/**
 * @brief Returns the Arduino label string for a GPIO pin (e.g. "D2").
 * @param [in] pin The GPIO pin.
 * @return String literal, never null.
 */
//! @note camelCase and noexcept.
inline const char* to_string(GpioPin pin)
{
    switch (pin)
    {
        case GpioPin::D0:  return "D0";
        case GpioPin::D1:  return "D1";
        case GpioPin::D2:  return "D2";
        case GpioPin::D3: return "D3";
        case GpioPin::D4: return "D4";
        case GpioPin::D5: return "D5";
        case GpioPin::D6: return "D6";
        case GpioPin::D7: return "D7";
        case GpioPin::D8: return "D8";
        case GpioPin::D9: return "D9";
        case GpioPin::D10: return "D10";
        case GpioPin::D11: return "D11";
        case GpioPin::D12: return "D12";
        case GpioPin::D13: return "D13";
        default:          return "UNKNOWN";
    }
}

/**
 * @brief Returns the physical GPIO number for an ADC pin.
 * @param [in] pin The ADC pin (A0–A7).
 * @return Corresponding gpio_num_t:
 *         A0=GPIO1 (ADC1_CH0), A1=GPIO2 (ADC1_CH1),
 *         A2=GPIO3 (ADC1_CH2), A3=GPIO4 (ADC1_CH3),
 *         A4=GPIO11 (ADC2_CH0), A5=GPIO12 (ADC2_CH1),
 *         A6=GPIO13 (ADC2_CH2), A7=GPIO14 (ADC2_CH3),
 *         or GPIO_NUM_NC for an unknown pin.
 */
//! @note camelCase and noexcept.
inline gpio_num_t to_number(AdcPin pin)
{
    switch (pin)
    {
        case AdcPin::A0: return GPIO_NUM_1;
        case AdcPin::A1: return GPIO_NUM_2;
        case AdcPin::A2: return GPIO_NUM_3;
        case AdcPin::A3: return GPIO_NUM_4;
        case AdcPin::A4: return GPIO_NUM_11;
        case AdcPin::A5: return GPIO_NUM_12;
        case AdcPin::A6: return GPIO_NUM_13;
        case AdcPin::A7: return GPIO_NUM_14;
        default:         return GPIO_NUM_NC;
    }
}

/**
 * @brief Checks if a pin belongs to ADC2.
 * @param [in] pin The ADC pin to check.
 * @return True if the pin is on ADC2 (A4–A7, GPIO11–GPIO14), false if on ADC1 (A0–A3).
 * @note ADC2 cannot be used while Wi-Fi is active.
 */
//! @note camelCase and noexcept.
inline bool is_adc2(AdcPin pin)
{
    return pin >= AdcPin::A4;
}

/**
 * @brief Returns the physical GPIO number for a GPIO pin.
 * @param [in] pin The GPIO pin (D0–D13).
 * @return Corresponding gpio_num_t (Arduino Nano ESP32 / NORA-W106 mapping):
 *         D0=GPIO44, D1=GPIO43, D2=GPIO5,  D3=GPIO6,  D4=GPIO7,
 *         D5=GPIO8,  D6=GPIO9,  D7=GPIO10, D8=GPIO17, D9=GPIO18,
 *         D10=GPIO21, D11=GPIO38, D12=GPIO47, D13=GPIO48,
 *         or GPIO_NUM_NC for an unknown pin.
 */
//! @note camelCase and noexcept.
inline gpio_num_t to_number(GpioPin pin)
{
    switch (pin)
    {
        case GpioPin::D0:  return GPIO_NUM_44;
        case GpioPin::D1:  return GPIO_NUM_43;
        case GpioPin::D2:  return GPIO_NUM_5;
        case GpioPin::D3:  return GPIO_NUM_6;
        case GpioPin::D4:  return GPIO_NUM_7;
        case GpioPin::D5:  return GPIO_NUM_8;
        case GpioPin::D6:  return GPIO_NUM_9;
        case GpioPin::D7:  return GPIO_NUM_10;
        case GpioPin::D8:  return GPIO_NUM_17;
        case GpioPin::D9:  return GPIO_NUM_18;
        case GpioPin::D10: return GPIO_NUM_21;
        case GpioPin::D11: return GPIO_NUM_38;
        case GpioPin::D12: return GPIO_NUM_47;
        case GpioPin::D13: return GPIO_NUM_48;
        default:           return GPIO_NUM_NC;
    }
}

/**
 * @brief Checks whether an ADC pin is available (not currently claimed by a driver).
 * @param [in] pin The ADC pin to check.
 * @return True if the pin is free to use, false if already acquired.
 */
//! @note camelCase and noexcept.
inline bool is_available(AdcPin pin)
{
    //! @note This is a very long line, consider acquiring a reference to the pin manager first:
    //!
    //! auto& manager      = PhysicalPinManager::instance();
    //! const auto physPin = static_cast<std::uint8_t>(to_number(pin);
    //! return manager.is_in_use(physPin);
    return !driver::pin::PhysicalPinManager::instance().is_in_use(static_cast<std::uint8_t>(to_number(pin)));
}

/**
 * @brief Checks whether a GPIO pin is available (not currently claimed by a driver).
 * @param [in] pin The GPIO pin to check.
 * @return True if the pin is free to use, false if already acquired.
 */
//! @note camelCase and noexcept.
inline bool is_available(GpioPin pin)
{
    //! @note Same here! At lease omit driver::pin, since you're already in namespace driver::pin::x.
    return !driver::pin::PhysicalPinManager::instance().is_in_use(static_cast<std::uint8_t>(to_number(pin)));
}
} // namespace driver::pin::esp32s3
