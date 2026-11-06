/**
 * @file Esp32s3 factory header file
 */
#pragma once

#include <cstdint>
#include <memory>

#include "driver/factory/interface.h"

namespace driver::factory
{
/**
 * @brief Esp32s3 factory
 */
class Esp32s3 final : public Interface
{
public:
    /**
     * @brief Construct a new Esp32s3 Hardware Factory object
     */
    Esp32s3() noexcept = default;

    /**
     * @brief Destroy the Esp32s3 Hardware Factory object
     */
    ~Esp32s3() noexcept override = default;

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
     * @return unique pointer to the hardware watchdog driver
     */
    std::unique_ptr<driver::watchdog::Interface>
    createWatchdog(std::uint32_t timeoutMs) noexcept override;

    /**
     * @brief The board. ESP-IDF knows only the chip, and this factory maps the Nano's pins.
     *
     * @return "arduino_nano_esp32"
     */
    const char* board() const noexcept override;

    /**
     * @brief The board target, named like Zephyr's for the same board
     *
     * @return "arduino_nano_esp32/esp32s3/procpu"
     */
    const char* boardTarget() const noexcept override;

    /**
     * @brief The platform
     *
     * @return "ESP-IDF"
     */
    const char* platform() const noexcept override;

    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;
};
} // namespace driver::factory
