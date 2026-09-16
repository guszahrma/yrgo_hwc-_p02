/**
 * @file Timer driver interface.
 */

#pragma once

#include <cstdint>

namespace driver::timer
{
/**
 * @brief Abstract timer.
 */
class Interface
{
public:
    virtual ~Interface() noexcept = default;

    /**
     * @brief Start the timer.
     */
    virtual void start() noexcept = 0;

    /**
     * @brief Stop the timer.
     */
    virtual void stop() noexcept = 0;

    /**
     * @brief Set the timeout period.
     *
     * @param[in] periodMs Period in milliseconds.
     */
    virtual void setPeriod(std::uint32_t periodMs) noexcept = 0;

    /**
     * @brief Check whether the timeout has elapsed.
     *
     * @return true if a timeout has occurred, otherwise false.
     */
    virtual bool timeout() noexcept = 0;
};
} // namespace driver::timer