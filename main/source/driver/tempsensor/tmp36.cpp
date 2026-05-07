/**
 * @brief Temperature sensor.
 */
#include <cstdint>
#include <cstdio>

#include "driver/tempsensor/interface.h"
#include "driver/tempsensor/tmp36.h"
#include "driver/adc/interface.h" 

namespace driver::tempsensor
{
    /**
     * @brief constructor
     */
    Tmp36::Tmp36(adc::Interface& adc, const uint8_t pin) noexcept
        : myAdc{adc}  //värde från adc(0-1023)
        , myPin{pin}
        , myInitialized{true} //tillståndet av sensor
    {}

    /**
     * @brief Destructor
     */
    ~Tmp36() noexcept = default;

    /**
     * @brief Check if the temperature sensor is initialized.
     * 
     * @return True if initialized, false otherwise.
     */
    bool Tmp36::isInitialized() const noexcept {
        return myInitialized;
    }

    /**
     * @brief Read temperature.
     * 
     * @return Temperature in degrees Celsius.
     */
    std::int16_t Tmp36::read() const noexcept {
        if(!isInitialized()){return 0;}

        #if 0
        // Beräkna inspänning i V från tempsensorn.
        const double inputVoltage{myAdc.readVoltage(myPin)};
        
        // Omvandla inspänning till temperatur med formeln
        // T  = 100 * Uin - 50.
        return 100.0 * inputVoltage - 50.0;
        #else
            // Just nu skickar vi tillbaka 0, då vi ej har en färdig ADC.
            std::printf("The temperature is 0 degrees.\n");
            return 0;
        #endif
    }

} // namespace driver::tempsensor
