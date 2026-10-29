/**
 * @file Factory interface
 */
#pragma once

#include <cstdint>
#include <memory>

#include "driver/adc/interface.h"
#include "driver/gpio/interface.h"
#include "driver/nvs/interface.h"
#include "driver/pin/interface.h"
#include "driver/serial/interface.h"
#include "driver/tempsensor/interface.h"
#include "driver/timer/interface.h"
#include "driver/watchdog/interface.h"

namespace driver::factory
{
/**
 * @brief GPIO roles in P02. Each platform's factory maps a role to its pin.
 */
enum class GpioId : std::uint8_t
{
    StatusLed, // Status LED (D6 on the Arduino Nano ESP32).
};

/**
 * @brief ADC roles in P02. Each platform's factory maps a role to its channel.
 */
enum class AdcId : std::uint8_t
{
    TempSensor, // TMP36 temperature sensor (A6 on the Arduino Nano ESP32).
};

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
     * @brief Create a GPIO driver for a role.
     *
     * @param[in] id The GPIO role (which pin is decided by the platform's factory).
     * @param[in] direction
     *
     * @return unique pointer
     */
    virtual std::unique_ptr<driver::gpio::Interface>
    createGpio(GpioId id, driver::gpio::Direction direction) noexcept = 0;

    /**
     * @brief Create an ADC driver for a role.
     *
     * @param[in] id The ADC role (which channel is decided by the platform's factory).
     * @param[in] referenceVoltage
     *
     * @return unique pointer
     */
    virtual std::unique_ptr<driver::adc::Interface> createAdc(AdcId id,
                                                              float referenceVoltage) noexcept = 0;

    /**
     * @brief Create an NVS storage
     *
     * @param[in] namespaceName
     *
     * @return unique pointer
     */
    virtual std::unique_ptr<driver::nvs::Interface>
    createNvs(const char* namespaceName) noexcept = 0;

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
    virtual std::unique_ptr<driver::tempsensor::Interface>
    createTempSensor(adc::Interface& adc) noexcept = 0;

    /**
     * @brief Create a timer object
     *
     * @return unique pointer
     */
    virtual std::unique_ptr<driver::timer::Interface> createTimer() noexcept = 0;

    /**
     * @brief Create a Watchdog object
     * 
     * @param[in] timeoutMs The timeout
     * 
     * @return unique pointer
     */
    virtual std::unique_ptr<driver::watchdog::Interface> createWatchdog(std::uint32_t timeoutMs) noexcept = 0;
};
} // namespace driver::factory
