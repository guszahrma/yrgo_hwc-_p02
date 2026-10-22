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
     * @param[in] pinNumber Matched against the hardware mapping
     * @param[in] direction
     * 
     * @return unique pointer to the hardware GPIO driver
     */
    std::unique_ptr<driver::gpio::Interface> createGpio(std::uint8_t pinNumber, driver::gpio::Direction direction) noexcept override;

    /**
     * @brief Create a real ESP32-S3 adc object
     *
     * @param[in] pinNumber Matched against the hardware ADC channels
     * @param[in] referenceVoltage
     *
     * @return unique pointer to the hardware ADC driver
     */
    std::unique_ptr<driver::adc::Interface> createAdc(std::uint8_t pinNumber, float referenceVoltage) noexcept override;

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
     * @param[in] baudRate
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
    std::unique_ptr<driver::tempsensor::Interface> createTempSensor(adc::Interface& adc) noexcept override;

    /**
     * @brief Create a timer object
     * 
     * @return unique pointer to the hardware timer driver
     */
    std::unique_ptr<driver::timer::Interface> createTimer() noexcept override;
    
    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;
};
} // namespace driver::factory
