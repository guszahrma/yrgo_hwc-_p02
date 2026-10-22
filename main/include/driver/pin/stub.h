/**
 * @file Pin driver stub.
 */
#pragma once

#include <cstdint>

namespace driver::pin::stub
{
/**
 * @brief ADC-capable pins for stub/test usage.
 *        Maps directly to physical numbers 0–7.
 */
enum class AdcPin : std::uint8_t
{
    A0 = 0,
    A1,
    A2,
    A3,
    A4,
    A5,
    A6,
    A7
};

/**
 * @brief GPIO-capable pins for stub/test usage.
 *        Maps directly to physical numbers 0–13.
 */
enum class GpioPin : std::uint8_t
{
    D0 = 0,
    D1,
    D2,
    D3,
    D4,
    D5,
    D6,
    D7,
    D8,
    D9,
    D10,
    D11,
    D12,
    D13
};

/**
 * @brief Returns the label string for a stub ADC pin (e.g. "D1").
 * @param [in] pin The ADC pin.
 * @return String literal, never null.
 */
inline const char* toString(AdcPin pin) noexcept
{
    switch (pin)
    {
        case AdcPin::A0:
            return "A0";
        case AdcPin::A1:
            return "A1";
        case AdcPin::A2:
            return "A2";
        case AdcPin::A3:
            return "A3";
        case AdcPin::A4:
            return "A4";
        case AdcPin::A5:
            return "A5";
        case AdcPin::A6:
            return "A6";
        case AdcPin::A7:
            return "A7";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Returns the label string for a stub GPIO pin (e.g. "D2").
 * @param [in] pin The GPIO pin.
 * @return String literal, never null.
 */
inline const char* toString(GpioPin pin) noexcept
{
    switch (pin)
    {
        case GpioPin::D0:
            return "D0";
        case GpioPin::D1:
            return "D1";
        case GpioPin::D2:
            return "D2";
        case GpioPin::D3:
            return "D3";
        case GpioPin::D4:
            return "D4";
        case GpioPin::D5:
            return "D5";
        case GpioPin::D6:
            return "D6";
        case GpioPin::D7:
            return "D7";
        case GpioPin::D8:
            return "D8";
        case GpioPin::D9:
            return "D9";
        case GpioPin::D10:
            return "D10";
        case GpioPin::D11:
            return "D11";
        case GpioPin::D12:
            return "D12";
        case GpioPin::D13:
            return "D13";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Returns the physical pin number for a stub ADC pin.
 * @param [in] pin The ADC pin (D0–D7).
 * @return Physical pin number (0–7).
 */
inline std::uint8_t toNumber(AdcPin pin) noexcept { return static_cast<std::uint8_t>(pin); }

/**
 * @brief Returns the physical pin number for a stub GPIO pin.
 * @param [in] pin The GPIO pin (D0–D13).
 * @return Physical pin number (0–13).
 */
inline std::uint8_t toNumber(GpioPin pin) noexcept { return static_cast<std::uint8_t>(pin); }
} // namespace driver::pin::stub
