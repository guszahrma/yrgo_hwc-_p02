/**
 * @file Zephyr ADC driver
 */
#pragma once

#include <cstdint>
#include <zephyr/drivers/adc.h>

#include "driver/adc/interface.h"

namespace driver::adc
{
/**
 * @brief ADC driver for Zephyr.
 *
 * The channel, attenuation (zephyr,gain) and resolution come from devicetree (io-channels in
 * zephyr,user and the channel node in app.overlay), so there is no referenceVoltage parameter as
 * in the ESP32-S3 driver. Calibration is done by Zephyr's ESP32 ADC driver.
 */
class Zephyr final : public Interface
{
public:
    /**
     * @brief Construct the ADC driver and set up the channel.
     *
     * @param[in] spec ADC channel from devicetree (io-channels in zephyr,user).
     */
    explicit Zephyr(const adc_dt_spec& spec) noexcept;

    /**
     * @brief Default destructor
     */
    ~Zephyr() noexcept override = default;

    /**
     * @brief Read the raw value of the ADC channel.
     *
     * @return Raw value between 0 and 4095 (12-bit resolution), or 0 if the driver is not ready
     *         or the read failed.
     */
    std::uint16_t readValue() noexcept override;

    /**
     * @brief Read the voltage of the ADC channel.
     *
     * @return Voltage in volts, or 0.0 if the driver is not ready or the read failed.
     */
    float readVoltage() noexcept override;

    Zephyr()                         = delete;
    Zephyr(const Zephyr&)            = delete;
    Zephyr(Zephyr&&)                 = delete;
    Zephyr& operator=(const Zephyr&) = delete;
    Zephyr& operator=(Zephyr&&)      = delete;

private:
    /**
     * @brief Read a raw sample from the ADC channel.
     *
     * @param[out] raw Raw sample, only written if the read succeeded.
     *
     * @return true if the read succeeded, false otherwise.
     */
    bool readRaw(std::int32_t& raw) noexcept;

    /** ADC channel from Devicetree. */
    const adc_dt_spec mySpec;

    /** True once the channel has been set up successfully. */
    bool myReady;
};
} // namespace driver::adc