/**
 * @file Zephyr GPIO driver.
 */

#pragma once

#include <cstdint>
#include <zephyr/drivers/gpio.h>

#include "driver/gpio/interface.h"

namespace driver::gpio
{
/**
 * @brief GPIO driver for Zephyr.
 */
class Zephyr final : public Interface
{
public:
    /**
     * @brief Construct a GPIO pin.
     *
     * @param[in] spec GPIO spec from Devicetree.
     * @param[in] direction Pin direction (input or output).
     */
    explicit Zephyr(const gpio_dt_spec& spec, Direction direction) noexcept;

    ~Zephyr() noexcept override = default;

    /**
     * @brief Read the current pin state.
     *
     * @return true if the pin is active, false if it is inactive.
     *         Where active means high or low is set by the polarity flag in Devicetree.
     */
    bool read() noexcept override;

    /**
     * @brief Set the pin state.
     *
     * @param[in] state true makes the pin active, false makes it inactive.
     */
    void write(bool state) noexcept override;

    /**
     * @brief Toggle an output pin between active and inactive.
     */
    void toggle() noexcept override;

    Zephyr()                         = delete;
    Zephyr(const Zephyr&)            = delete;
    Zephyr(Zephyr&&)                 = delete;
    Zephyr& operator=(const Zephyr&) = delete;
    Zephyr& operator=(Zephyr&&)      = delete;

private:
    const gpio_dt_spec myPin;
    const Direction myDirection;
    bool myState;
    bool myReady;
};
} // namespace driver::gpio