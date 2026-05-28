#include "driver/factory/esp32s3.h"

// Inkludera de riktiga ESP32-S3-drivrutinerna
#include "driver/gpio/esp32s3.h"
#include "driver/adc/esp32s3.h"
#include "driver/serial/esp32s3.h"
#include "driver/tempsensor/tmp36.h"
#include "driver/timer/esp32s3.h"

namespace driver::factory
{

// -----------------------------------------------------------------------------
std::unique_ptr<driver::gpio::Interface> Esp32s3::create_gpio(std::uint8_t pinNumber, driver::gpio::Direction direction) noexcept
{
    return std::make_unique<driver::gpio::Esp32s3>(pinNumber, direction);
}

// -----------------------------------------------------------------------------
std::unique_ptr<driver::adc::Interface> Esp32s3::create_adc(std::uint8_t pinNumber, float referenceVoltage) noexcept
{
    auto realAdcPin = static_cast<driver::pin::esp32s3::AdcPin>(pinNumber);
    return std::make_unique<driver::adc::Esp32s3>(realAdcPin, referenceVoltage);
}

// -----------------------------------------------------------------------------
std::unique_ptr<driver::serial::Interface> Esp32s3::create_serial(int baudRate) noexcept
{
    return std::make_unique<driver::serial::Esp32s3>(baudRate);
}

// -----------------------------------------------------------------------------
std::unique_ptr<driver::tempsensor::Interface> Esp32s3::create_tempsensor(adc::Interface& adc) noexcept
{
    return std::make_unique<driver::tempsensor::Tmp36>(adc);
}

// -----------------------------------------------------------------------------
std::unique_ptr<driver::timer::Interface> Esp32s3::create_timer() noexcept
{
    return std::make_unique<driver::timer::Esp32s3>();
}

} // namespace driver::factory