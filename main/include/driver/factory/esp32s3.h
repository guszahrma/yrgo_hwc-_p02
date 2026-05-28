#pragma once

#include <cstdint>
#include <memory>

#include "driver/factory/interface.h"

namespace driver::factory
{
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
    std::unique_ptr<driver::gpio::Interface> create_gpio(std::uint8_t pinNumber, driver::gpio::Direction direction) noexcept override;

    /**
     * @brief Create a real ESP32-S3 adc object
     * 
     * @param[in] pinNumber Matched against the hardware ADC channels
     * @param[in] referenceVoltage 
     * 
     * @return unique pointer to the hardware ADC driver
     */
    std::unique_ptr<driver::adc::Interface> create_adc(std::uint8_t pinNumber, float referenceVoltage) noexcept override;

    /**
     * @brief Create a real ESP32-S3 serial object
     * 
     * @param[in] baudRate
     * 
     * @return unique pointer to the hardware Serial driver
     */
    std::unique_ptr<driver::serial::Interface> create_serial(int baudRate) noexcept override;

    /**
     * @brief Create a real ESP32-S3 tempsensor object
     * 
     * @param[in] adc 
     * 
     * @return unique pointer to the hardware tempsensor driver
     */
    std::unique_ptr<driver::tempsensor::Interface> create_tempsensor(adc::Interface& adc) noexcept override;

    /**
     * @brief Create a timer object
     * 
     * @return unique pointer to the hardware timer driver
     */
    std::unique_ptr<driver::timer::Interface> create_timer() noexcept override;
};
} // namespace driver::factory