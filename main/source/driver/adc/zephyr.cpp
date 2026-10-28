/**
 * @file Zephyr ADC driver implementation.
 */
#include <cstdint>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>

#include "driver/adc/zephyr.h"

namespace driver::adc
{
// --------------------------------------------------------------------------------
Zephyr::Zephyr(const adc_dt_spec& spec) noexcept
    : mySpec{spec}
    , myReady{false}
{
    // The ADC controller must be initialised before the channel can be used.
    if (!adc_is_ready_dt(&mySpec))
    {
        printk("Zephyr ADC: controller for channel %u is not ready.\n",
               static_cast<unsigned>(mySpec.channel_id));
        return;
    }

    // Configures the channel with gain, reference and resolution from devicetree.
    const int ret{adc_channel_setup_dt(&mySpec)};

    if (0 > ret)
    {
        printk("Zephyr ADC: failed to set up channel %u (error %d)\n",
               static_cast<unsigned>(mySpec.channel_id), ret);
        return;
    }

    myReady = true;
    printk("Zephyr ADC initialized on channel %u\n", static_cast<unsigned>(mySpec.channel_id));
}

// --------------------------------------------------------------------------------
std::uint16_t Zephyr::readValue() noexcept
{
    std::int32_t raw{}, sumRaw{};

    // read the value multiple times and average them to avoid distortions
    for (std::uint8_t i{}; i < AdcSamples; i++)
    {
        if (!readRaw(raw)) { return 0U; }
        sumRaw += raw;
    }

    return static_cast<std::uint16_t>(sumRaw / AdcSamples);
}

// --------------------------------------------------------------------------------
float Zephyr::readVoltage() noexcept
{
    std::int32_t value{readValue()};
    if (0 == value) { return 0.0f; }

    // Converts the raw value to millivolts in place, using the gain and reference from devicetree.
    const int ret{adc_raw_to_millivolts_dt(&mySpec, &value)};

    if (0 > ret)
    {
        printk("Zephyr ADC: failed to convert channel %u to millivolts (error %d)\n",
               static_cast<unsigned>(mySpec.channel_id), ret);
        return 0.0f;
    }

    return static_cast<float>(value) / 1000.0f;
}

// --------------------------------------------------------------------------------
bool Zephyr::readRaw(std::int32_t& raw) noexcept
{
    if (!myReady) { return false; }

    // One 12-bit sample fits in 16 bits.
    std::int16_t sample{};

    adc_sequence sequence{};
    sequence.buffer      = &sample;
    sequence.buffer_size = sizeof(sample); // In bytes, not number of samples.

    // Fills in the channel and resolution from devicetree.
    int ret{adc_sequence_init_dt(&mySpec, &sequence)};

    if (0 > ret)
    {
        printk("Zephyr ADC: failed to init sequence for channel %u (error %d)\n",
               static_cast<unsigned>(mySpec.channel_id), ret);
        return false;
    }

    ret = adc_read_dt(&mySpec, &sequence);

    if (0 > ret)
    {
        printk("Zephyr ADC: failed to read channel %u (error %d)\n",
               static_cast<unsigned>(mySpec.channel_id), ret);
        return false;
    }

    raw = sample;
    return true;
}
} // namespace driver::adc