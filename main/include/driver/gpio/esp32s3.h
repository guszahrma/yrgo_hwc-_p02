/**
 * @file ESP32-S3 GPIO driver.
 */

#pragma once

#include <cstdint>

#include "driver/gpio/interface.h"

namespace driver::gpio
{
/**
 * @brief GPIO driver for the ESP32-S3.
 */
class Esp32s3 final : public Interface
{
public:
    /**
     * @brief Construct a GPIO pin.
     *
     * @param[in] pinNumber Hardware pin number.
     * @param[in] direction Pin direction (input or output).
     */
    explicit Esp32s3(std::uint8_t pinNumber, Direction direction) noexcept;

    ~Esp32s3() noexcept override = default;

    /**
     * @brief Read the current pin level.
     *
     * @return true if the pin is high, false if it is low.
     */
    bool read() noexcept override;

    /**
     * @brief Write a level to the pin.
     *
     * @param[in] state true drives the pin high, false drives it low.
     */
    void write(bool state) noexcept override;

    /**
     * @brief Toggle an output pin between high and low.
     */
    void toggle() noexcept override;

    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

private:
    const std::uint8_t myPinNumber;
    const Direction    myDirection;
    bool               myState;
};
} // namespace driver::gpio