#pragma once

#include <cstdint>

namespace driver::timer
{

/**
 * @brief Timer interface class
 */
class Interface
{
public:

    virtual ~Interface() noexcept = default;

    /**
     * @brief Start timer
     */
    virtual void start() noexcept = 0;

    /**
     * @brief Stop timer
     */
    virtual void stop() noexcept = 0;

    /**
     * @brief Set timeout period in milliseconds
     * 
     * @param period_ms timeout period in milliseconds
     */
    virtual void period(std::uint32_t period_ms) noexcept = 0;

    /**
     * @brief Check if timeout has occurred
     * 
     * @return true if timeout occurred, otherwise false
     */
    virtual bool timeout() noexcept = 0;

    /**
     * @brief Check if driver is initialized
     * 
     * @return true if initialized, otherwise false
     */
    virtual bool initialized() noexcept = 0;
};

}