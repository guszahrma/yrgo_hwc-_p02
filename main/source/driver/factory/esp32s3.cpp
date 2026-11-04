/**
 * @file Esp32s3 factory
 */
#include "driver/factory/esp32s3.h"

#include "driver/adc/esp32s3.h"
#include "driver/gpio/esp32s3.h"
#include "driver/nvs/esp32s3.h"
#include "driver/pin/esp32s3.h"
#include "driver/serial/esp32s3.h"
#include "driver/tempsensor/tmp36.h"
#include "driver/timer/esp32s3.h"
#include "driver/watchdog/esp32s3.h"

namespace driver::factory
{

// -----------------------------------------------------------------------------
std::unique_ptr<driver::gpio::Interface>
Esp32s3::createGpio(GpioId id, driver::gpio::Direction direction) noexcept
{
    // Each role is mapped to its Arduino pin here; the compiler warns (-Wswitch) if a role is
    // missing.
    switch (id)
    {
        case GpioId::StatusLed:
            // Status LED on D6 (GPIO9).
            return std::make_unique<driver::gpio::Esp32s3>(
                static_cast<std::uint8_t>(
                    driver::pin::esp32s3::toNumber(driver::pin::esp32s3::GpioPin::D6)),
                direction);
    }

    // Unreachable while every GpioId is handled above.
    return nullptr;
}

// -----------------------------------------------------------------------------
std::unique_ptr<driver::adc::Interface> Esp32s3::createAdc(AdcId id,
                                                           float referenceVoltage) noexcept
{
    switch (id)
    {
        case AdcId::TempSensor:
            // TMP36 on A6 (GPIO13, ADC2).
            return std::make_unique<driver::adc::Esp32s3>(driver::pin::esp32s3::AdcPin::A6,
                                                          referenceVoltage);
    }

    // Unreachable while every AdcId is handled above.
    return nullptr;
}

// -----------------------------------------------------------------------------
std::unique_ptr<driver::nvs::Interface> Esp32s3::createNvs(const char* namespaceName) noexcept
{ return std::make_unique<driver::nvs::Esp32s3>(namespaceName); }

// -----------------------------------------------------------------------------
std::unique_ptr<driver::serial::Interface> Esp32s3::createSerial() noexcept
{ return std::make_unique<driver::serial::Esp32s3>(); }

// -----------------------------------------------------------------------------
std::unique_ptr<driver::tempsensor::Interface>
Esp32s3::createTempSensor(adc::Interface& adc) noexcept
{ return std::make_unique<driver::tempsensor::Tmp36>(adc); }

// -----------------------------------------------------------------------------
std::unique_ptr<driver::timer::Interface> Esp32s3::createTimer() noexcept
{ return std::make_unique<driver::timer::Esp32s3>(); }

// -----------------------------------------------------------------------------
std::unique_ptr<driver::watchdog::Interface>
Esp32s3::createWatchdog(std::uint32_t timeoutMs) noexcept
{
    // Running stubs until real drivers are finished
    return std::make_unique<driver::watchdog::Esp32s3>(timeoutMs);
}
} // namespace driver::factory
