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
 * Maps P02's roles to devicetree (each board's overlay in zephyr/app/boards/):
 * GpioId::StatusLed -> alias p02-led, AdcId::TempSensor -> io-channels in zephyr,user. NVS uses
 * nvs::Zephyr (Settings subsystem).
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

    /**
     * @brief Create a watchdog that restarts the system if it is not kicked in time
     *
     * @param[in] timeoutMs Time in milliseconds without a kick before the system restarts
     *
     * @return unique pointer to the watchdog driver
     */
    std::unique_ptr<driver::watchdog::Interface>
    createWatchdog(std::uint32_t timeoutMs) noexcept override;

    /**
     * @brief Create a boot driver: the version of the images, and confirming the running one
     *
     * @return unique pointer
     */
    std::unique_ptr<driver::boot::Interface> createBoot() noexcept override;

    /**
     * @brief The board, from the build (CONFIG_BOARD)
     *
     * @return C-string literal, e.g. "daisy_seed"
     */
    const char* board() const noexcept override;

    /**
     * @brief The board target, from the build (CONFIG_BOARD_TARGET)
     *
     * @return C-string literal, e.g. "daisy_seed/stm32h750xx/ram_load"
     */
    const char* boardTarget() const noexcept override;

    /**
     * @brief The platform
     *
     * @return "Zephyr"
     */
    const char* platform() const noexcept override;

    Zephyr(const Zephyr&)            = delete;
    Zephyr(Zephyr&&)                 = delete;
    Zephyr& operator=(const Zephyr&) = delete;
    Zephyr& operator=(Zephyr&&)      = delete;
};
} // namespace driver::factory
