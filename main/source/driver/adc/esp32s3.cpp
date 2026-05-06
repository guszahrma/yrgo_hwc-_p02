#include <cstdio>
#include "driver/adc/esp32s3.h"
#include "driver/pin/manager.h"
#include "driver/pin/esp32s3.h"

namespace driver::adc
{

Esp32s3::Esp32s3(driver::pin::esp32s3::AdcPin pin, float referenceVoltage)
    : myPin(pin), myHandle(nullptr), myChannel(ADC_CHANNEL_0)
{
    if (!acquire_pin(static_cast<std::uint8_t>(driver::pin::esp32s3::to_number(pin))))
    {
        std::printf("ESP32-S3 ADC init failed: pin %s already in use.\n", driver::pin::esp32s3::to_string(pin));
        return;
    }

    if (referenceVoltage > maxVoltage)
    {
        std::printf("ESP32-S3 ADC init failed: referenceVoltage %.2f exceeds maximum %.2f V.\n", referenceVoltage, maxVoltage);
        release_pin(static_cast<std::uint8_t>(driver::pin::esp32s3::to_number(pin)));
        return;
    }

    adc_unit_t unit = driver::pin::esp32s3::is_adc2(pin) ? ADC_UNIT_2 : ADC_UNIT_1;

    adc_oneshot_unit_init_cfg_t unit_cfg = {};
    unit_cfg.unit_id = unit;
    adc_oneshot_new_unit(&unit_cfg, &myHandle);

    adc_oneshot_io_to_channel(driver::pin::esp32s3::to_number(pin), &unit, &myChannel);

    adc_oneshot_chan_cfg_t chan_cfg = {};
    if      (referenceVoltage <= 0.95f) { chan_cfg.atten = ADC_ATTEN_DB_0;   myFullScaleVoltage = 0.95f; }
    else if (referenceVoltage <= 1.25f) { chan_cfg.atten = ADC_ATTEN_DB_2_5; myFullScaleVoltage = 1.25f; }
    else if (referenceVoltage <= 1.75f) { chan_cfg.atten = ADC_ATTEN_DB_6;   myFullScaleVoltage = 1.75f; }
    else                                { chan_cfg.atten = ADC_ATTEN_DB_12;  myFullScaleVoltage = 3.1f;  }
    chan_cfg.bitwidth = ADC_BITWIDTH_12;
    adc_oneshot_config_channel(myHandle, myChannel, &chan_cfg);

    std::printf("ESP32-S3 ADC initialized on pin %s.\n", driver::pin::esp32s3::to_string(pin));
}

Esp32s3::~Esp32s3() noexcept
{
    if (myHandle != nullptr)
    {
        adc_oneshot_del_unit(myHandle);
    }
    release_pin(static_cast<std::uint8_t>(driver::pin::esp32s3::to_number(myPin)));
    std::printf("ESP32-S3 ADC released on pin %s.\n", driver::pin::esp32s3::to_string(myPin));
}

uint16_t Esp32s3::read_value() noexcept
{
    if (myHandle == nullptr)
    {
        return 0U;
    }
    int raw{};
    adc_oneshot_read(myHandle, myChannel, &raw);
    return static_cast<uint16_t>(raw);
}

float Esp32s3::read_voltage() noexcept
{
    return static_cast<float>(read_value()) / maxRawValue * myFullScaleVoltage;
}

} // namespace driver::adc
