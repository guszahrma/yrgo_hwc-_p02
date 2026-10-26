/**
 * @file Zephyr driver factory implementation
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include "driver/adc/zephyr.h"
#include "driver/factory/zephyr.h"
#include "driver/gpio/zephyr.h"
#include "driver/serial/zephyr.h"
#include "driver/tempsensor/tmp36.h"
#include "driver/timer/zephyr.h"

// Not ported to Zephyr yet
#include "driver/nvs/stub.h"

namespace driver::factory
{
// -----------------------------------------------------------------------------
std::unique_ptr<driver::gpio::Interface>
Zephyr::createGpio(GpioId id, driver::gpio::Direction direction) noexcept
{
    switch (id)
    {
        case GpioId::StatusLed:
        {
            // Status LED: Aalias p02-led in app.overlay (D6 via the arduino nano header.)
            static const gpio_dt_spec spec = GPIO_DT_SPEC_GET(DT_ALIAS(p02_led), gpios);
            return std::make_unique<driver::gpio::Zephyr>(spec, direction);
        }
    }
    // Unreachable while every GpioId is handled above.
    return nullptr;
}

// -----------------------------------------------------------------------------
std::unique_ptr<driver::adc::Interface> Zephyr::createAdc(AdcId id, float referenceVoltage) noexcept
{
    // Not used on Zephyr: the attenuation comes from zephyr,gain in devicetree.
    (void)referenceVoltage;

    switch (id)
    {
        case AdcId::TempSensor:
        {
            // TMP36: index 0 in io-channels of zephyr,user in app.overlay (A6).
            static const adc_dt_spec spec = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
            return std::make_unique<driver::adc::Zephyr>(spec);
        }
    }

    // Unreachable while every AdcId is handled above.
    return nullptr;
}

// -----------------------------------------------------------------------------
std::unique_ptr<driver::nvs::Interface> Zephyr::createNvs(const char* namespaceName) noexcept
{
    // TODO, nvs::Zephyr (zephyr settings subsystem). Values are not kept after a reset
    return std::make_unique<driver::nvs::Stub>(namespaceName);
}

// -----------------------------------------------------------------------------
std::unique_ptr<driver::serial::Interface> Zephyr::createSerial() noexcept
{
    // The console device: USB-C on the nano. (zephyr,console in the board)
    return std::make_unique<driver::serial::Zephyr>(DEVICE_DT_GET(DT_CHOSEN(zephyr_console)));
}

// -----------------------------------------------------------------------------
std::unique_ptr<driver::tempsensor::Interface>
Zephyr::createTempSensor(adc::Interface& adc) noexcept
{
    // TMP36 is platform independent, it only uses adc::Interface
    return std::make_unique<driver::tempsensor::Tmp36>(adc);
}

// -----------------------------------------------------------------------------
std::unique_ptr<driver::timer::Interface> Zephyr::createTimer() noexcept
{
    return std::make_unique<driver::timer::Zephyr>();
}
} // namespace driver::factory