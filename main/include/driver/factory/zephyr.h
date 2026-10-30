/**
 * @file Zephyr factory header file
 */
#pragma once

#include <cstdint>
#include <memory>

#include "driver/factory/interface.h"

namespace driver::factory
{
/**
 * @brief Driver factory for Zephyr.
 *
 * Maps P02's roles to devicetree (zephyr/app/app.overlay): GpioId::StatusLed -> alias p02-led,
 * AdcId::TempSensor -> io-channels in zephyr,user. NVS uses nvs::Zephyr (Settings subsystem).
 */
class Zephyr final : public Interface
{
public:
    /**
     * @brief Construct a new Zephyr Hardware Factory object
     */
    Zephyr() noexcept = default;

    /**
     * @brief Destroy the Zephyr Hardware Factory object
     */
    ~Zephyr() noexcept override = default;

    /**
     * @brief Create a real ESP32-S3 gpio object
     *
     * @param[in] id The GPIO role, mapped to an ESP32-S3 pin.
     * @param[in] direction
     *
     * @return unique pointer to the hardware GPIO driver
     */
    std::unique_ptr<driver::gpio::Interface>
    createGpio(GpioId id, driver::gpio::Direction direction) noexcept override;

    /**
     * @brief Create a real ESP32-S3 adc object
     *
     * @param[in] id The ADC role, mapped to an ESP32-S3 ADC pin.
     * @param[in] referenceVoltage
     *
     * @return unique pointer to the hardware ADC driver
     */
    std::unique_ptr<driver::adc::Interface> createAdc(AdcId id,
                                                      float referenceVoltage) noexcept override;

    /**
     * @brief Create a real ESP32-S3 NVS storage
     *
     * @param[in] namespaceName
     *
     * @return unique pointer to the hardware NVS driver
     */
    std::unique_ptr<driver::nvs::Interface> createNvs(const char* namespaceName) noexcept override;

    /**
     * @brief Create a real ESP32-S3 serial object
     *
     * @return unique pointer to the hardware Serial driver
     */
    std::unique_ptr<driver::serial::Interface> createSerial() noexcept override;

    /**
     * @brief Create a real ESP32-S3 tempsensor object
     *
     * @param[in] adc
     *
     * @return unique pointer to the hardware tempsensor driver
     */
    std::unique_ptr<driver::tempsensor::Interface>
    createTempSensor(adc::Interface& adc) noexcept override;

    /**
     * @brief Create a timer object
     *
     * @return unique pointer to the hardware timer driver
     */
    std::unique_ptr<driver::timer::Interface> createTimer() noexcept override;

    Zephyr(const Zephyr&)            = delete;
    Zephyr(Zephyr&&)                 = delete;
    Zephyr& operator=(const Zephyr&) = delete;
    Zephyr& operator=(Zephyr&&)      = delete;
};
} // namespace driver::factory
