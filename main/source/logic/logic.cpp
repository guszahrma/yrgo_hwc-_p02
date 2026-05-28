#include <charconv>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <sys/_intsup.h>
#include <sys/unistd.h>
#include "driver/factory/stub.h"
#include "logic/logic.h"

namespace logic::logic 
{

    Logic::Logic() noexcept
: myBlinkState(0), // Initialize blink state to off
   myPeriodLengthMs(1000) // Set default blinking period to 1000 ms
{
    // Create the necessary drivers using the factory and initialize the system state.
    // For example, you might create a serial driver for communication and a GPIO driver for controlling the LED.
    // Initialize the LED as off, set the blink state to off, and set a default blinking period (e.g., 1000 ms).
    driver::factory::Stub factory{};
    // Example of creating drivers using the factory:   
    mySerialDriver = factory.create_serial(115200);
    myGpioDriver = factory.create_gpio(9U, driver::gpio::Direction::OUTPUT);
    myAdcDriver = factory.create_adc(7U, 3.3f);

    // Initialize other necessary components and state variables here.
}

// --------------------------------------------------------------------------------
Logic::~Logic() noexcept
{
    // Clean up resources, if necessary.
    // For example, you might want to delete any dynamically allocated drivers or other resources here.
}

// --------------------------------------------------------------------------------
void Logic::run() noexcept
{

    // Implementation for running the logic loop will go here.
    // This will include reading commands from the serial input, processing them, and controlling the LED
    // and other components accordingly. It will also handle the blinking logic and period adjustments.
    
    // The loop should be designed to be non-blocking and responsive, allowing for real-time command processing and state updates.
    constexpr std::size_t bufferSize = 64;
    char buffer[bufferSize];
    uint8_t bufferLength = 0;


    while(true)
    {
        if (0 < (bufferLength = mySerialDriver->readLine(buffer, sizeof(buffer))))
        {
            std::cout << std::endl << std::endl << "Reading line..." << buffer << std::endl;

            std::string_view input{buffer, bufferLength};

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
                uint16_t ms{};
                auto [ptr, ec] = std::from_chars(args.data(), args.data() + args.size(), ms);
                if (ec == std::errc{})
                    handlePeriod(ms);
            }
       }
       // Todo Add blinking logic here, using myBlinkState and myPeriodLengthMs to control the LED blinking behavior.

       // TEMP: randomize command input for testing
       {
           static const char* commands[] = {
               "on", "off", "blink on", "blink off",
               "period 500", "period 1000", "period 2000", "status"
           };
           constexpr std::size_t numCommands = sizeof(commands) / sizeof(commands[0]);
           sleep(3);
           auto* stub = static_cast<driver::serial::Stub*>(mySerialDriver.get());
           stub->simulateInputData(commands[std::rand() % numCommands]);
       }
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
    myBlinkState = 1;
}

// --------------------------------------------------------------------------------
void Logic::handleBlinkOff() noexcept
{
    std::cout << std::endl << "Handling 'blink off' command..." << std::endl;

    // Implement the logic to turn blinking off.
    myBlinkState = 0;
}

// --------------------------------------------------------------------------------
void Logic::handlePeriod(uint16_t periodLengthMs) noexcept
{
    std::cout << std::endl << "Handling 'period' command..." << periodLengthMs << " ms" << std::endl;

    // Implement the logic to set the blinking period.
    myPeriodLengthMs = periodLengthMs;
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
}