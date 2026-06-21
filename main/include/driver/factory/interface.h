//! @note File header missing.
#pragma once

#include <cstdint>
#include <memory>

#include "driver/adc/interface.h"
#include "driver/gpio/interface.h"
#include "driver/serial/interface.h"
#include "driver/pin/interface.h"

namespace driver::factory
{
//! @note Class documentation missing.
class Interface
{
public:
    /**
     * @brief Destroy the Interface object
     */
    virtual ~Interface() noexcept = default;

    /**
     * @brief Create a gpio object
     * 
     * @param[in] pinNumber 
     * @param[in] direction
     *  
     * @return unique pointer
     */
    //! @note Mark noexcept => the device will reboot if allocation fails. That's good, since
    //!       the system can't run without all drivers initialized anyway.
    //!       Also, please use camelCase.
    virtual std::unique_ptr<driver::gpio::Interface> create_gpio(std::uint8_t pinNumber, driver::gpio::Direction direction) = 0;

    /**
     * @brief Create a adc object
     * 
     * @param[in] pinNumber 
     * @param[in] referenceVoltage 
     * 
     * @return unique pointer
     */
    //! @note noexcept and camelCase.
    virtual std::unique_ptr<driver::adc::Interface> create_adc(std::uint8_t pinNumber, float referenceVoltage) = 0;

    /**
     * @brief Create serial object
     * 
     * @param[in] baudRate
     * 
     * @return unique pointer
     */
    //! @note noexcept and camelCase. Also consider making the baud rate unsigned (std::uint32_t).
    virtual std::unique_ptr<driver::serial::Interface> create_serial(int baudRate) = 0;
};
} // namespace driver::factory
