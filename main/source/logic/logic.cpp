<<<<<<< HEAD
/**
 * @file Implementation of the Logic class for handling commands and controlling the LED and other components.
=======
/**  @file Implementation of the Logic class for handling commands and controlling the LED and other components.
>>>>>>> 47d69850e36567a986acb822598cc5a6ad83e2de
 */

#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>
#include <sys/unistd.h>

#include "driver/factory/esp32s3.h"
#include "driver/factory/stub.h"
#include "logic/logic.h"

namespace logic
{

// --------------------------------------------------------------------------------
Logic::Logic() noexcept
    : myPeriodLengthMs{1000} // Set default blinking period to 1000 ms
    , myBlinkState{false}    // Initialize blink state to off
{
    // Create the necessary drivers using the factory and initialize the system state.
    // For example, you might create a serial driver for communication and a GPIO driver for controlling the LED.
    // Initialize the LED as off, set the blink state to off, and set a default blinking period (e.g., 1000 ms).
    driver::factory::Esp32s3 factory{};
    // Example of creating drivers using the factory:   
    mySerialDriver = factory.create_serial(115200);
    myGpioDriver = factory.create_gpio(9U , driver::gpio::Direction::OUTPUT);
    myAdcDriver = factory.create_adc(6U, 3.3f);
    myTimerDriver = factory.create_timer();
    myTimerDriver->stop(); // Start the timer for blinking

    // Initialize other necessary components and state variables here.
}

// --------------------------------------------------------------------------------
Logic::~Logic() noexcept
{
    // Clean up resources, if necessary.
    // For example, you might want to delete any dynamically allocated drivers or other resources here.
    mySerialDriver.reset();
    myGpioDriver.reset();
    myAdcDriver.reset();
    myTimerDriver->stop(); // Stop the timer when the Logic instance is destroyed
    myTimerDriver.reset();
}

// --------------------------------------------------------------------------------
void Logic::run() noexcept
{
    // Implementation for running the logic loop will go here.
    // This will include reading commands from the serial input, processing them, and controlling the LED
    // and other components accordingly. It will also handle the blinking logic and period adjustments.
    
    // The loop should be designed to be non-blocking and responsive, allowing for real-time command processing and state updates.
    constexpr std::size_t bufferSize{64};
    constexpr std::uint8_t sleepDuration_ms{1};
    char buffer[bufferSize]{};

    while(true)
    {
        if (mySerialDriver->readLine(buffer, sizeof(buffer)))
        {
            std::cout << std::endl << std::endl << "Reading line..." << buffer << std::endl;

            std::string_view input{buffer, std::strlen(buffer)};

            // Split into first word and the rest
            auto spacePos = input.find(' ');
            auto cmd  = input.substr(0, spacePos);
            auto args = (spacePos != std::string_view::npos) ? input.substr(spacePos + 1) : std::string_view{};
            if      (cmd == "on")     handleOn();
            else if (cmd == "off")    handleOff();
            else if (cmd == "status") handleStatus();
            else if (cmd == "blink") {
                if      (args == "on")  handleBlinkOn();
                else if (args == "off") handleBlinkOff();
            }
            else if (cmd == "period")
            {
                std::uint16_t ms{};

                //! @note Please add an inline comment for this; it's always a good thing to do
                //!       when using lambdas (lambdas = a necessary evil; works fine, but 
                //!       usually totally unreadable, haha).
                // I left @note as is since I don't understand the function.
                auto [ptr, ec] = std::from_chars(args.data(), args.data() + args.size(), ms);
                if (ec == std::errc{})
                    handlePeriod(ms);
            }
        }
        else {
            sleep(sleepDuration_ms); // Sleep briefly to avoid busy-waiting when no input is available
        }


        // timeout can retrurn true if timer is running and the set period has elapsed, otherwise false
        if (myTimerDriver->timeout())
        {
            // Toggle the LED state
            myGpioDriver->toggle();
            myTimerDriver->start(); // Restart the timer for the next period
        }

#ifndef DRIVER_SERIAL_ESP32S3
        // TEMP: randomize command input for testing
        {
            static const char* commands[] {
                "on", "off", "blink on", "blink off",
                "period 500", "period 1000", "period 2000", "status"
            };
            constexpr std::size_t numCommands{sizeof(commands) / sizeof(commands[0])};

            sleep(sleepDuration_ms); // Sleep briefly to avoid busy-waiting when no input is available
            auto* stub = static_cast<driver::serial::Stub*>(mySerialDriver.get());
            // Randomly simulate if random input is generated or not

            if (0 == std::rand() % 10 )
            stub->simulateInputData(commands[std::rand() % numCommands]);
        }
#endif
    }
}

// --------------------------------------------------------------------------------
void Logic::handleOn() noexcept
{
    std::cout << std::endl << "Handling 'on' command..." << std::endl;

    // Implement the logic to turn the LED on.
    myGpioDriver->write(true);
}

// --------------------------------------------------------------------------------
void Logic::handleOff() noexcept
{
    std::cout << std::endl << "Handling 'off' command..." << std::endl;

    // Implement the logic to turn the LED off.
    myGpioDriver->write(false);
}

// --------------------------------------------------------------------------------
void Logic::handleBlinkOn() noexcept
{
    std::cout << std::endl << "Handling 'blink on' command..." << std::endl;

    // Implement the logic to turn blinking on.
    myTimerDriver->start(); // Start the timer for blinking
    myBlinkState = true;
}

// --------------------------------------------------------------------------------
void Logic::handleBlinkOff() noexcept
{
    std::cout << std::endl << "Handling 'blink off' command..." << std::endl;

    // Implement the logic to turn blinking off.
    myTimerDriver->stop(); // Stop the timer for blinking
}

// --------------------------------------------------------------------------------
void Logic::handlePeriod(uint16_t periodLengthMs) noexcept
{
    std::cout << std::endl << "Handling 'period' command..." << periodLengthMs << " ms" << std::endl;

    // Implement the logic to set the blinking period.
    myTimerDriver->set_period(periodLengthMs);
    myPeriodLengthMs = periodLengthMs; // Store the new period length
}

// --------------------------------------------------------------------------------
void Logic::handleStatus() noexcept
{
    // Implement the logic to print the current status.
    // This is a placeholder implementation - replace with actual status printing logic.
    mySerialDriver->print("Status:\n");
    mySerialDriver->print("Blink State: ");
    mySerialDriver->print(myBlinkState ? "On\n" : "Off\n");
    mySerialDriver->print("Blink Period: ");    
    mySerialDriver->print(std::to_string(myPeriodLengthMs).c_str());
    mySerialDriver->print(" ms\n");
    mySerialDriver->print("Current Temperature: ");
    // TODO Replace this with a call to the TMP36 class to get actual temperature reading
    mySerialDriver->print(std::to_string(0).c_str());
    mySerialDriver->print(" °C\n");
}
} // namespace logic::logic
