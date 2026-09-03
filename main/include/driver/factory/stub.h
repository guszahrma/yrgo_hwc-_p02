//! @note File header missing.
#pragma once

#include "driver/adc/stub.h"
#include "driver/factory/interface.h"
#include "driver/gpio/stub.h"
#include "driver/serial/stub.h"
#include "driver/timer/stub.h"
#include "driver/tempsensor/stub.h"

//! @note Please remove this later when your real factory is implemented.
#define DRIVER_SERIAL_ESP32S3

#ifdef DRIVER_SERIAL_ESP32S3
#include "driver/serial/esp32s3.h"
#else
#include "driver/serial/stub.h"
#endif
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
    std::unique_ptr<driver::gpio::Interface> create_gpio(std::uint8_t pinNumber, driver::gpio::Direction direction) noexcept override
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
    std::unique_ptr<driver::adc::Interface> create_adc(std::uint8_t pinNumber, float referenceVoltage) noexcept override
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
    std::unique_ptr<driver::serial::Interface> create_serial(int baudRate) noexcept override
    {
        (void) (baudRate);
        //! @note This is fine for now, but remove this later when you have a real factory.
#ifdef DRIVER_SERIAL_ESP32S3
        return std::make_unique<driver::serial::Esp32s3>(baudRate);
#else
        return std::make_unique<driver::serial::Stub>();
#endif
    }

    /**
     * @brief Create a tempsensor object
     * 
     * @param[in] adc 
     * 
     * @return unique pointer
     */
    std::unique_ptr<driver::tempsensor::Interface> create_tempsensor(driver::adc::Interface& adc) noexcept override
    {
        (void)(adc);
        return std::make_unique<driver::tempsensor::Stub>(static_cast<std::uint16_t>(0));
    }

    /**
     * @brief Create a timer object
     * 
     * @return unique pointer
     */
    std::unique_ptr<driver::timer::Interface> create_timer() noexcept override
    {
        return std::make_unique<driver::timer::Stub>();
    }
};
} // namespace driver::factory