/**
 * @file Factory interface
 */
#pragma once

#include <cstdint>
#include <memory>

#include "driver/adc/interface.h"
#include "driver/gpio/interface.h"
#include "driver/nvs/interface.h"
#include "driver/serial/interface.h"
#include "driver/pin/interface.h"
#include "driver/timer/interface.h"
#include "driver/tempsensor/interface.h"

namespace driver::factory
{
/**
 * @brief Factory interface
 */
class Interface
{
public:
    /**
     * @brief Destroy the Interface object
     */
    virtual ~Interface() noexcept = default;

    /**
     * @brief Create a gpio object
     * 
     * @param[in] pinNumber 
     * @param[in] direction
     *  
     * @return unique pointer
     */
    virtual std::unique_ptr<driver::gpio::Interface> createGpio(std::uint8_t pinNumber, driver::gpio::Direction direction) noexcept = 0;

    /**
     * @brief Create a adc object
     * 
     * @param[in] pinNumber 
     * @param[in] referenceVoltage 
     * 
     * @return unique pointer
     */
    virtual std::unique_ptr<driver::adc::Interface> createAdc(std::uint8_t pinNumber, float referenceVoltage) noexcept = 0;

    /**
     * @brief Create an NVS storage
     * 
     * @param[in] namespaceName 
     * 
     * @return unique pointer
     */
    virtual std::unique_ptr<driver::nvs::Interface> createNvs(const char* namespaceName) noexcept = 0;

    /**
     * @brief Create serial object
     * 
     * @return unique pointer
     */
    virtual std::unique_ptr<driver::serial::Interface> createSerial() noexcept = 0;

    /**
     * @brief Create a tempsensor object
     * 
     * @param[in] adc 
     * 
     * @return unique pointer
     */
    virtual std::unique_ptr<driver::tempsensor::Interface> createTempSensor(adc::Interface& adc) noexcept = 0;

    /**
     * @brief Create a timer object
     * 
     * @return unique pointer 
     */
    virtual std::unique_ptr<driver::timer::Interface> createTimer() noexcept = 0;
};
} // namespace driver::factory
