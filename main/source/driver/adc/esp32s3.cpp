//! @note Missing file header.
#include <cstdio>

//! @note Sort headers.
#include "driver/adc/esp32s3.h"
#include "driver/pin/manager.h"
#include "driver/pin/esp32s3.h"

namespace driver::adc
{
Esp32s3::Esp32s3(driver::pin::esp32s3::AdcPin pin, float referenceVoltage)
    //! @note Please use the universal initializer {} instead of ().
    : myPin(pin)
    , myHandle(nullptr)
    , myChannel(ADC_CHANNEL_0)
    , myCaliHandle(nullptr)
{
    if (!acquire_pin(static_cast<std::uint8_t>(driver::pin::esp32s3::to_number(pin))))
    {
        std::printf("ESP32-S3 ADC init failed: pin %s already in use.\n", driver::pin::esp32s3::to_string(pin));
        return;
    }

    //! @note Please use Yoda notation, i.e. if (maxVoltage < referenceVoltage).
    if (referenceVoltage > maxVoltage)
    {
        std::printf("ESP32-S3 ADC init failed: referenceVoltage %.2f exceeds maximum %.2f V.\n", referenceVoltage, maxVoltage);
        
        //! @note Nice to see that you're using static_cast. Consider adding a static function
        //!       for this -> it would look way nicer.
        //!
        //! Let's say you had this in an anomyous namespace:
        //!
        //! constexpr std::uint8_t getPhysPin(const pin::esp32s3::AdcPin pin) noexcept
        //! {
        //!     return static_cast<std::uint8_t>(pin::esp32s3::to_number(pin));
        //! }
        //!
        //! Note that if you only cast integers, you can mark the methods constexpr.
        //! The same applies to pin::esp32s3::to_number().
        release_pin(static_cast<std::uint8_t>(driver::pin::esp32s3::to_number(pin)));
        return;
    }

    //! @note Feel free to use auto here.
    adc_unit_t unit = driver::pin::esp32s3::is_adc2(pin) ? ADC_UNIT_2 : ADC_UNIT_1;

    //! @note Please skip =, the universal initializer {} will do.
    adc_oneshot_unit_init_cfg_t unit_cfg = {};
    unit_cfg.unit_id = unit;
    adc_oneshot_new_unit(&unit_cfg, &myHandle);

    adc_oneshot_io_to_channel(driver::pin::esp32s3::to_number(pin), &unit, &myChannel);

    adc_oneshot_chan_cfg_t chan_cfg = {};
    //! @note Please replace the magic numbers below with constexpr.
    if      (referenceVoltage <= 0.95f) { chan_cfg.atten = ADC_ATTEN_DB_0;   myFullScaleVoltage = 0.95f; }
    else if (referenceVoltage <= 1.25f) { chan_cfg.atten = ADC_ATTEN_DB_2_5; myFullScaleVoltage = 1.25f; }
    else if (referenceVoltage <= 1.75f) { chan_cfg.atten = ADC_ATTEN_DB_6;   myFullScaleVoltage = 1.75f; }
    else                                { chan_cfg.atten = ADC_ATTEN_DB_12;  myFullScaleVoltage = 3.1f;  }
    chan_cfg.bitwidth = ADC_BITWIDTH_12;
    adc_oneshot_config_channel(myHandle, myChannel, &chan_cfg);

    //! @note Skip =.
    adc_cali_curve_fitting_config_t cali_cfg = {};
    cali_cfg.unit_id  = unit;
    cali_cfg.chan     = myChannel;
    cali_cfg.atten   = chan_cfg.atten;
    cali_cfg.bitwidth = ADC_BITWIDTH_12;

    //! @note Please use Yoda notation.
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &myCaliHandle) != ESP_OK)
    {
        myCaliHandle = nullptr;
        std::printf("ESP32-S3 ADC: calibration unavailable, using linear scaling.\n");
    }

    std::printf("ESP32-S3 ADC initialized on pin %s.\n", driver::pin::esp32s3::to_string(pin));
}

Esp32s3::~Esp32s3() noexcept
{
    //! @note Please use Yoda notation.
    if (myCaliHandle != nullptr)
    {
        adc_cali_delete_scheme_curve_fitting(myCaliHandle);
    }
    if (myHandle != nullptr)
    {
        adc_oneshot_del_unit(myHandle);
    }
    release_pin(static_cast<std::uint8_t>(driver::pin::esp32s3::to_number(myPin)));
    std::printf("ESP32-S3 ADC released on pin %s.\n", driver::pin::esp32s3::to_string(myPin));
}

uint16_t Esp32s3::read_value() noexcept
{
    //! @note "In grave danger you are." - Yoda in episode II.
    if (myHandle == nullptr)
    {
        return 0U;
    }
    int raw{};
    adc_oneshot_read(myHandle, myChannel, &raw);
    //! std::uint16_t.
    return static_cast<uint16_t>(raw);
}

float Esp32s3::read_voltage() noexcept
{
    //! @note "Destroy the Sith we must." - Yoda in episode III.
    if (myHandle == nullptr)
    {
        return 0.0f;
    }
    int raw{};
    adc_oneshot_read(myHandle, myChannel, &raw);

    //! @note "Do, or do not. There is no try." - Yoda in episode V.
    if (myCaliHandle != nullptr)
    {
        int millivolts{};
        adc_cali_raw_to_voltage(myCaliHandle, raw, &millivolts);

        //! @note This magic number is fine, I think.
        return static_cast<float>(millivolts) / 1000.0f;
    }
    return static_cast<float>(raw) / maxRawValue * myFullScaleVoltage;
}
} // namespace driver::adc
