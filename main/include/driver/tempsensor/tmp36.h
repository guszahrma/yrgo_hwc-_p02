/**
 * @brief Temperature sensor.
 */
#pragma once

#include <cstdint>

#include "driver/tempsensor/interface.h"

// ADC interface.
namespace driver::adc { class Interface; }

namespace driver::tempsensor
{
/**
 * @brief Tmp36
 */
class Tmp36 final : public Interface
{
    public:
    /**
     * @brief constructor
     * 
     * @param[in] initialTemp Initial temperature (default = 0).
     */
    explicit Tmp36(adc::Interface& adc) noexcept;

    /**
     * @brief Destructor.
     */
    ~Tmp36() noexcept override = default;

    /**
     * @brief Check if the temperature sensor is initialized.
     * 
     * @return True if initialized, false otherwise.
     */
    bool isInitialized() const noexcept override;

    /**
     * @brief Read temperature.
     * 
     * @return Temperature in degrees Celsius.
     */
    std::int16_t read() const noexcept override;

    Tmp36()                         = delete;
    Tmp36(const Tmp36&)             = delete;
    Tmp36(Tmp36&&)                  = delete;
    Tmp36& operator=(const Tmp36&)  = delete;
    Tmp36& operator=(Tmp36&&)       = delete;

private:
    adc::Interface& myAdc;
};
} // namespace driver::tempsensor
