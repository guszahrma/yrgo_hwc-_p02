#pragma once

#include <cstdint>
#include "driver/adc/interface.h"
#include "driver/pin/esp32s3.h"
#include "esp_adc/adc_oneshot.h"

/**
 * @brief ADC driver for the ESP32-S3 using the adc_oneshot API.
 *
 * Acquires a physical pin via PhysicalPinManager on construction and
 * releases it on destruction. The attenuation is selected automatically
 * based on the expected signal range (referenceVoltage), and read_voltage()
 * scales against the actual hardware ceiling for that attenuation.
 */
namespace driver::adc
{
class Esp32s3 : public Interface, private driver::pin::PinManagerDriverAccess
{
public:
    /**
     * @brief Constructs and initialises the ADC channel.
     * @param [in] pin              The ADC pin to use (A0–A7).
     * @param [in] referenceVoltage Maximum expected input voltage in volts (≤ 3.3 V).
     *                              Selects the optimal attenuation automatically:
     *                                 ≤ 0.95 V → DB_0, ≤ 1.25 V → DB_2_5,
     *                                 ≤ 1.75 V → DB_6, otherwise → DB_12 (ceiling ~3.1 V).
     * @note Pins A4–A7 belong to ADC2, which cannot be used while Wi-Fi is active.
     */
    explicit Esp32s3(driver::pin::esp32s3::AdcPin pin, float referenceVoltage = defaultVref);
    ~Esp32s3() noexcept override;

    /**
     * @brief Read function for the value of the ADC channel.
     * @return Value read from the ADC channel, between 0 and 4095 (12-bit resolution),
     *         or 0 if the driver failed to initialise.
     */
    uint16_t read_value() noexcept override;

    /**
     * @brief Read function for the voltage of the ADC channel.
     * @return Value read from the ADC channel converted to voltage, between 0 V and
     *         the hardware full-scale ceiling (0.95, 1.25, 1.75, or ~3.1 V).
     */
    float read_voltage() noexcept override;

    Esp32s3()                          = delete;
    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

private:
    static constexpr float maxRawValue{4095.0f};
    static constexpr float defaultVref{3.3f};
    static constexpr float maxVoltage{3.3f};
    const driver::pin::esp32s3::AdcPin myPin;
    float myFullScaleVoltage{};
    adc_oneshot_unit_handle_t myHandle;
    adc_channel_t myChannel;
};    
} // namespace driver::adc
