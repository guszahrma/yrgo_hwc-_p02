/**
 * @brief Temperature sensor.
 */
#include <cstdint>

#include "driver/tempsensor/tmp36.h"
#include "driver/adc/interface.h"

namespace driver
{
namespace tempsensor
{
    /**
     * @brief constructor
     */
    Tmp36::Tmp36(adc::Interface& adc) noexcept
        : myAdc{adc}  
        , myInitialized{true} 
    {}

    /**
     * @brief Check if the temperature sensor is initialized.
     * 
     * @return True if initialized, false otherwise.
     */
    bool Tmp36::isInitialized() const noexcept{
        return myInitialized;
    }

    /**
     * @brief Read temperature.
     * 
     * @return Temperature in degrees Celsius.
     */
    std::int16_t Tmp36::read() const noexcept{
        if(!isInitialized()){return 0;}

        // Beräkna inspänning i V från tempsensorn.
        const double inputVoltage{myAdc.read_voltage()};
        
        // Omvandla inspänning till temperatur med formeln
        // T  = 100 * Uin - 50.
        return 100.0 * inputVoltage - 50.0;
    }
}
} // namespace driver::tempsensor
