/**
 * @file the esp32s3 specific ADC interface
 */
#include <cstdio>

#include "driver/adc/esp32s3.h"
#include "driver/pin/esp32s3.h"
#include "driver/pin/manager.h"

namespace
{
constexpr float voltage0_95V{0.95f};
constexpr float voltage1_25V{1.25f};
constexpr float voltage1_75V{1.75f};
constexpr float voltage3_10V{3.1f};

constexpr std::uint8_t getPhysPin(const driver::pin::esp32s3::AdcPin pin) noexcept
{
    return static_cast<std::uint8_t>(driver::pin::esp32s3::toNumber(pin));
}
} // namespace

namespace driver::adc
{
Esp32s3::Esp32s3(driver::pin::esp32s3::AdcPin pin, float referenceVoltage)
    : myPin{pin}
    , myFullScaleVoltage{}
    , myHandle{nullptr}
    , myChannel{ADC_CHANNEL_0}
    , myCaliHandle{nullptr}
{
    if (!acquirePin(getPhysPin(pin)))
    {
        std::printf("ESP32-S3 ADC init failed: pin %s already in use.\n",
                    driver::pin::esp32s3::toString(pin));
        return;
    }

    if (MaxVoltage < referenceVoltage)
    {
        std::printf("ESP32-S3 ADC init failed: referenceVoltage %.2f exceeds maximum %.2f V.\n",
                    referenceVoltage, MaxVoltage);

        releasePin(getPhysPin(pin));
        return;
    }

    adc_unit_t unit = driver::pin::esp32s3::isAdc2(pin) ? ADC_UNIT_2 : ADC_UNIT_1;

    adc_oneshot_unit_init_cfg_t unit_cfg{};
    unit_cfg.unit_id = unit;
    adc_oneshot_new_unit(&unit_cfg, &myHandle);

    adc_oneshot_io_to_channel(driver::pin::esp32s3::toNumber(pin), &unit, &myChannel);

    adc_oneshot_chan_cfg_t chan_cfg = {};
    if (voltage0_95V >= referenceVoltage)
    {
        chan_cfg.atten     = ADC_ATTEN_DB_0;
        myFullScaleVoltage = voltage0_95V;
    }
    else if (voltage1_25V >= referenceVoltage)
    {
        chan_cfg.atten     = ADC_ATTEN_DB_2_5;
        myFullScaleVoltage = voltage1_25V;
    }
    else if (voltage1_75V >= referenceVoltage)
    {
        chan_cfg.atten     = ADC_ATTEN_DB_6;
        myFullScaleVoltage = voltage1_75V;
    }
    else
    {
        chan_cfg.atten     = ADC_ATTEN_DB_12;
        myFullScaleVoltage = voltage3_10V;
    }
    chan_cfg.bitwidth = ADC_BITWIDTH_12;
    adc_oneshot_config_channel(myHandle, myChannel, &chan_cfg);

    adc_cali_curve_fitting_config_t cali_cfg{};
    cali_cfg.unit_id  = unit;
    cali_cfg.chan     = myChannel;
    cali_cfg.atten    = chan_cfg.atten;
    cali_cfg.bitwidth = ADC_BITWIDTH_12;

    if (ESP_OK != adc_cali_create_scheme_curve_fitting(&cali_cfg, &myCaliHandle))
    {
        myCaliHandle = nullptr;
        std::printf("ESP32-S3 ADC: calibration unavailable, using linear scaling.\n");
    }

    std::printf("ESP32-S3 ADC initialized on pin %s.\n", driver::pin::esp32s3::toString(pin));
}

Esp32s3::~Esp32s3() noexcept
{
    if (nullptr != myCaliHandle) { adc_cali_delete_scheme_curve_fitting(myCaliHandle); }
    if (nullptr != myHandle) { adc_oneshot_del_unit(myHandle); }
    releasePin(getPhysPin(myPin));
    std::printf("ESP32-S3 ADC released on pin %s.\n", driver::pin::esp32s3::toString(myPin));
}

uint16_t Esp32s3::readValue() noexcept
{
    if (nullptr == myHandle) { return 0U; }
    int raw{};
    adc_oneshot_read(myHandle, myChannel, &raw);
    return static_cast<std::uint16_t>(raw);
}

float Esp32s3::readVoltage() noexcept
{
    if (nullptr == myHandle) { return 0.0f; }
    int raw{};
    adc_oneshot_read(myHandle, myChannel, &raw);

    if (nullptr != myCaliHandle)
    {
        int millivolts{};
        adc_cali_raw_to_voltage(myCaliHandle, raw, &millivolts);

        return static_cast<float>(millivolts) / 1000.0f;
    }
    return static_cast<float>(raw) / MaxRawValue * myFullScaleVoltage;
}
} // namespace driver::adc
