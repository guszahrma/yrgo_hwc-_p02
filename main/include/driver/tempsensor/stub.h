/**
 * @brief Temperature sensor stub.
 */
#pragma once

#include <cstdint>
#include <cstdio>

#include "driver/tempsensor/interface.h"

namespace driver::tempsensor
{
/**
 * @brief Temperature sensor stub.
 */
class Stub final : public Interface
{
public:
    /**
     * @brief constructor
     * 
     * @param[in] initialTemp Initial temperature (default = 0).
     */
    explicit Stub(std::int16_t initialTemp = 0) noexcept
        : myTemp{initialTemp}
        , myInitialized{true}
    {}

    /**
     * @brief Destructor.
     */
    ~Stub() noexcept override = default;

    /**
     * @brief Check if the temperature sensor is initialized.
     * 
     * @return True if initialized, false otherwise.
     */
    bool isInitialized() const noexcept override{
        return myInitialized;
    }

    /**
     * @brief Read temperature.
     * 
     * @return Temperature in degrees Celsius.
     */
    std::int16_t read() const noexcept override {
        // Return simulated temperature if initialized, else 0.
        return myInitialized ? myTemp : 0;
    }


    /**
     * @brief simulate a temperature.
     */
    void simulateTemperature(const std::int16_t temp){
        if(myInitialized){
            myTemp = temp;
            std::printf("The temperature is %d degrees.\n", myTemp);
        }
    }

    /**
     * @brief Simulate initialization state.
     * 
     * @param[in] initialized True if initialized, false otherwise.
     */
    void simulateInitialized(const bool initialized) noexcept
    {
        myInitialized = initialized;
    }

private:
    /** Simulated temperature. */
    std::int16_t myTemp;

    /** True if initialized, false otherwise. */
    bool myInitialized;
};
} // namespace driver::tempsensor
