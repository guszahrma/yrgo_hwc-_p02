#pragma once

#include "driver/adc/stub.h"
#include "driver/factory/interface.h"
#include "driver/gpio/stub.h"
#include "driver/serial/stub.h"

namespace driver::factory
{
class Stub final : public Interface
{
public:
    /**
     * @brief Construct a new Stub object
     */
    Stub() noexcept = default;

    /**
     * @brief Destroy the Stub object
     */
    ~Stub() noexcept override = default;

    /**
     * @brief Create a gpio object
     * 
     * @param[in] pinNumber 
     * @param[in] direction
     *  
     * @return unique pointer
     */
    std::unique_ptr<driver::gpio::Interface> create_gpio(std::uint8_t pinNumber, driver::gpio::Direction direction) override
    {
        return std::make_unique<driver::gpio::Stub>(pinNumber, direction);
    }

    /**
     * @brief Create a adc object
     * 
     * @param[in] pinNumber 
     * @param[in] referenceVoltage 
     * 
     * @return unique pointer
     */
    std::unique_ptr<driver::adc::Interface> create_adc(std::uint8_t pinNumber, float referenceVoltage) override
    {
        auto stubAdcPin = static_cast<driver::pin::stub::AdcPin>(pinNumber);
        return std::make_unique<driver::adc::Stub>(stubAdcPin, referenceVoltage);
    }

    /**
     * @brief Create serial object
     * 
     * @param[in] baudRate
     * 
     * @return unique pointer
     */
    std::unique_ptr<driver::serial::Interface> create_serial(int baudRate) override
    {
        (void)(baudRate);
        return std::make_unique<driver::serial::Stub>();
    }
    
};
} // namespace driver::factory