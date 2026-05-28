/**
 * @brief Temperature sensor interface.
 */
#pragma once

#include <cstdint>

namespace driver::tempsensor
{
/**
 * @brief Temperature sensor interface.
 */
class Interface
{
public:
    /**
     * @brief Destructor.
     */
    virtual ~Interface() noexcept = default;

    /**
     * @brief Check if the temperature sensor is initialized.
     * 
     * @return True if initialized, false otherwise.
     */
    virtual bool isInitialized() const noexcept = 0;

    /**
     * @brief Read temperature.
     * 
     * @return Temperature in degrees Celsius.
     */
    virtual std::int16_t read() const noexcept = 0;
};
} // namespace driver::tempsensor
