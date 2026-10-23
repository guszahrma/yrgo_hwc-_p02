#include <zephyr/kernel.h>

#include "driver/adc/zephyr.h"
#include "driver/tempsensor/tmp36.h"

// TMP36 on A6: index 0 in io-channels of zephyr,user (app.overlay).
static const adc_dt_spec tempSpec = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

int main()
{
    driver::adc::Zephyr adc{tempSpec};
    driver::tempsensor::Tmp36 tempSensor{adc};

    while (true)
    {
        const std::uint16_t raw{adc.readValue()};
        const float voltage{adc.readVoltage()};
        printk("raw %u  voltage %d mV  temperature %d C\n", static_cast<unsigned>(raw),
               static_cast<int>(voltage * 1000.0f), static_cast<int>(tempSensor.read()));
        k_msleep(1000);
    }
    return 0;
}