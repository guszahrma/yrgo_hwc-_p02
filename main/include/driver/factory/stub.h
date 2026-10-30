/**
 * @file Stub factory
 */
#pragma once

#include "driver/adc/stub.h"
#include "driver/factory/interface.h"
#include "driver/gpio/stub.h"
#include "driver/nvs/stub.h"
#include "driver/pin/stub.h"
#include "driver/serial/stub.h"
#include "driver/tempsensor/stub.h"
#include "driver/timer/stub.h"
#include "driver/watchdog/stub.h"

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
     * @brief Create a stub GPIO driver for a role.
     *
     * @param[in] id        The GPIO role, mapped to a stub pin.
     * @param[in] direction Pin direction.
     *
     * @return unique pointer to the stub GPIO driver.
     */
    std::unique_ptr<driver::gpio::Interface>
    createGpio(GpioId id, driver::gpio::Direction direction) noexcept override
    {
        switch (id)
        {
            case GpioId::StatusLed:
                return std::make_unique<driver::gpio::Stub>(
                    static_cast<std::uint8_t>(driver::pin::stub::GpioPin::D6), direction);
        }

        // Unreachable while every GpioId is handled above.
        return nullptr;
    }

    /**
     * @brief Create a stub ADC driver for a role.
     *
     * @param[in] id        The ADC role, mapped to a stub pin.
     * @param[in] direction Pin direction.
     *
     * @return unique pointer to the stub ADC driver.
     */
    std::unique_ptr<driver::adc::Interface> createAdc(AdcId id,
                                                      float referenceVoltage) noexcept override
    {
        switch (id)
        {
            case AdcId::TempSensor:
                return std::make_unique<driver::adc::Stub>(driver::pin::stub::AdcPin::A6,
                                                           referenceVoltage);
        }

        // Unreachable while every AdcId is handled above.
        return nullptr;
    }

    /**
     * @brief Create an NVS storage
     *
     * @param[in] namespaceName
     *
     * @return unique pointer
     */
    std::unique_ptr<driver::nvs::Interface> createNvs(const char* namespaceName) noexcept override
    { return std::make_unique<driver::nvs::Stub>(namespaceName); }

    /**
     * @brief Create serial object
     *
     * @return unique pointer
     */
    std::unique_ptr<driver::serial::Interface> createSerial() noexcept override
    { return std::make_unique<driver::serial::Stub>(); }

    /**
     * @brief Create a tempsensor object
     *
     * @param[in] adc
     *
     * @return unique pointer
     */
    std::unique_ptr<driver::tempsensor::Interface>
    createTempSensor(driver::adc::Interface& adc) noexcept override
    {
        (void)(adc);
        return std::make_unique<driver::tempsensor::Stub>(static_cast<std::uint16_t>(0));
    }

    /**
     * @brief Create a timer object
     *
     * @return unique pointer
     */
    std::unique_ptr<driver::timer::Interface> createTimer() noexcept override
    { return std::make_unique<driver::timer::Stub>(); }

    /**
     * @brief Create a Watchdog object
     *
     * @param[in] timeoutMs The timeout
     *
     * @return unique pointer
     */
    std::unique_ptr<driver::watchdog::Interface>
    createWatchdog(std::uint32_t timeoutMs) noexcept override
    { return std::make_unique<driver::watchdog::Stub>(timeoutMs); }
};
} // namespace driver::factory