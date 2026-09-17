/**
 * @file GPIO driver interface.
 */

#pragma once

#include <cstdint>

namespace driver::gpio
{
/**
 * @brief GPIO pin direction.
 */
enum class Direction : std::uint8_t
{
    OUTPUT,         // 0
    INPUT_PULL_UP,  // 1
    INPUT_PULL_DOWN // 2
};

/**
 * @brief Abstract GPIO pin.
 */
class Interface
{
public:
    virtual ~Interface() noexcept = default;

    /**
     * @brief Read the pin level.
     *
     * @return true if high, false if low.
     */
    virtual bool read() noexcept = 0;

    /**
     * @brief Write a level to the pin.
     *
     * @param[in] state true drives high, false drives low.
     */
    virtual void write(bool state) noexcept = 0;

    /**
     * @brief Toggle an output pin between high and low.
     */
    virtual void toggle() noexcept = 0;
};
} // namespace driver::gpio