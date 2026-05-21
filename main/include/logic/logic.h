#pragma once
#include <cstdint>
#include "driver/factory/interface.h"


/**
 * @brief A simple logic class for handling basic logic operations.
 * it implements the commands:
 * - on → which activates the LED
 * - off → which deactivates the LED
 * - blink on → which makes the LED blink
 * - blink off → which deactivates the blinking (LED remains in its current state)
 * - period [period_length_ms] → which sets the blinking period to the specified length in milliseconds (e.g., "period 500" sets a 500 ms period)
 *           If the LED is currently blinking, it should immediately apply the new period.
 *           If the LED is not currently blinking, it should store the new period and use it when blinking is activated.
 * - status → prints the current status of the system, including:
 *           blinkstate (on/off)
 *           period_length_ms (ms)
 *           current temperature (°C)
 */

namespace logic::logic
{
class Logic
{
public:
    /** 
     * @brief Constructs a new Logic instance.
     * Uses the factory to create the necessary drivers and initializes the system state.
     * Initialises Led as off and Blink as off, and sets a default blinking period (e.g., 1000 ms).
     */
    Logic();
    ~Logic() noexcept;

    /**
     * @brief Runs the main logic loop.
     * Continuously listens for commands from the serial input and executes the corresponding actions.
     * Will later be extended to also receive commands from MQTT and possibly other sources.
     * The loop should be designed to be non-blocking and responsive, allowing for real-time command processing and state updates.  
     */
    void run() noexcept;

    Logic(const Logic&)            = delete;
    Logic(Logic&&)                 = delete;
    Logic& operator=(const Logic&) = delete;
    Logic& operator=(Logic&&)      = delete;
private:
    uint8_t myBlinkState; // 0 for off, 1 for on
    uint16_t myPeriodLengthMs; // Blinking period in milliseconds
    std::unique_ptr<driver::serial::Interface> mySerialDriver; // Serial driver for communication
    std::unique_ptr<driver::gpio::Interface> myGpioDriver; // GPIO driver for controlling the LED
    std::unique_ptr<driver::adc::Interface> myAdcDriver; // ADC driver for reading temperature or other analog values

    // Command handler functions
    void handleOn();
    void handleOff();
    void handleBlinkOn();
    void handleBlinkOff();
    void handlePeriod(uint16_t periodLengthMs);
    void handleStatus();
};
}