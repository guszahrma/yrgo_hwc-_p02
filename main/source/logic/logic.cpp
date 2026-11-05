/**
 * @file Implementation of the Logic class for handling commands and controlling the LED and other
 * components.
 */
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>
#include <thread>

#include "logic/logic.h"

namespace logic
{

// --------------------------------------------------------------------------------
Logic::Logic(driver::factory::Interface& factory) noexcept
    : myPeriodLengthMs{1000} // Set default blinking period to 1000 ms
    , myBlinkState{false}    // Initialize blink state to off
    , myHung{false}          // Kick the watchdog until 'hang'
    , myBoard{factory.board()}
    , myBoardTarget{factory.boardTarget()}
    , myPlatform{factory.platform()}
{
    // Create the necessary drivers using the factory and initialize the system state.
    // For example, you might create a serial driver for communication and a GPIO driver for
    // controlling the LED. Initialize the LED as off, set the blink state to off, and set a default
    // blinking period (e.g., 1000 ms). Example of creating drivers using the factory:
    mySerialDriver = factory.createSerial();
    myStatusLed =
        factory.createGpio(driver::factory::GpioId::StatusLed, driver::gpio::Direction::OUTPUT);
    myTempSensorAdc = factory.createAdc(driver::factory::AdcId::TempSensor, 3.3f);
    myTempSensor    = factory.createTempSensor(*myTempSensorAdc);
    myTimerDriver   = factory.createTimer();
    myTimerDriver->stop(); // Start the timer for blinking
    myNvsUserSettingsStorage = factory.createNvs("user_settings");
    myWatchdog               = factory.createWatchdog(WatchdogTimeoutMs);

    if (myWatchdog->causedLastReset())
    {
        mySerialDriver->print("Last reset was caused by the watchdog\n");
    }

    // Initialize other necessary components and state variables here.
}

// --------------------------------------------------------------------------------
Logic::~Logic() noexcept
{
    // Stop blinking. The drivers themselves are destroyed automatically afterwards, in reverse
    // order of declaration, so the temperature sensor goes before the ADC it refers to.
    myTimerDriver->stop();
}

// --------------------------------------------------------------------------------
void Logic::run() noexcept
{
    loadStoredUserSettings();

    mySerialDriver->print(
        "write relevant command, write help to get a list of available commands\n");

    // The main loop: one pass at a time, forever. Tests call step() directly instead, so that they
    // get control back after each pass.
    while (true)
    {
        step();
    }
}

// --------------------------------------------------------------------------------
void Logic::step() noexcept
{
    constexpr std::size_t bufferSize{64};

    // Must be at least one FreeRTOS tick (10 ms with CONFIG_FREERTOS_HZ=100): shorter sleeps
    // busy-wait in ESP-IDF, so the idle task never runs and the task watchdog fires
    constexpr std::uint8_t sleepDuration_ms{10};

    char buffer[bufferSize]{};

    // Tell the watchdog that the loop is still running. After 'hang' (see handleHang()) the
    // kicks stop, and the watchdog restarts the system as if the loop had hung.
    if (!myHung) { myWatchdog->kick(); }

    // Handle a command if a whole line has arrived; otherwise sleep briefly so the loop does not
    // busy-wait.
    if (mySerialDriver->readLine(buffer, sizeof(buffer)))
    {
        std::cout << std::endl << std::endl << "Reading line..." << buffer << std::endl;

        std::string_view input{buffer, std::strlen(buffer)};

        // Split into first word and the rest
        auto spacePos = input.find(' ');
        auto cmd      = input.substr(0, spacePos);
        auto args =
            (spacePos != std::string_view::npos) ? input.substr(spacePos + 1) : std::string_view{};
        if ("help" == cmd || "Help" == cmd)
            handleHelp();
        else if (cmd == "on")
            handleOn();
        else if (cmd == "off")
            handleOff();
        else if (cmd == "status")
            handleStatus();
        else if (cmd == "hang")
            handleHang();
        else if (cmd == "blink")
        {
            if (args == "on")
                handleBlinkOn();
            else if (args == "off")
                handleBlinkOff();
            else
                mySerialDriver->print("Usage: blink on | blink off\n");
        }
        else if (cmd == "period")
        {
            std::uint16_t ms{};

            // Parse the argument as a number. std::from_chars returns where it stopped (ptr) and an
            // error code (ec), unpacked with a structured binding. ec is set if args is empty, not
            // a number or too large for std::uint16_t.
            const auto [ptr, ec] = std::from_chars(args.data(), args.data() + args.size(), ms);

            // ptr must reach the end of args, so "500abc" is not accepted as 500.
            if ((std::errc{} == ec) && (args.data() + args.size() == ptr))
                handlePeriod(ms);
            else
                mySerialDriver->print("Usage: period <ms>, e.g. period 500\n");
        }
        else if (cmd == "store")
            handleStore();
        else
            handleUnknownCommand(cmd);
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepDuration_ms));
    }

    // Blinking: toggle the LED each time the blink period has elapsed. timeout() is only true while
    // the timer is running, i.e. after 'blink on'.
    if (myTimerDriver->timeout())
    {
        myStatusLed->toggle();
        myTimerDriver->start(); // Restart the timer for the next period
    }
}

// --------------------------------------------------------------------------------
void Logic::handleHelp() noexcept
{
    std::cout << std::endl << "Handling 'Help' command..." << std::endl;
    std::cout << std::endl << "Available commands:" << std::endl;
    std::cout << "  on" << std::endl;
    std::cout << "  off" << std::endl;
    std::cout << "  blink on" << std::endl;
    std::cout << "  blink off" << std::endl;
    std::cout << "  period <ms>" << std::endl;
    std::cout << "  status" << std::endl;
    std::cout << "  store" << std::endl;
    std::cout << "  hang  (tests the watchdog: the board restarts after a few seconds)"
              << std::endl;
    std::cout << "  help" << std::endl;
}

// --------------------------------------------------------------------------------
void Logic::handleUnknownCommand(const std::string_view& cmd) noexcept
{
    std::cout << std::endl << "Handling unknown command: " << cmd << std::endl;
    std::cout << "Type 'help' to see the list of available commands." << std::endl;
}

// --------------------------------------------------------------------------------
void Logic::handleOn() noexcept
{
    std::cout << std::endl << "Handling 'on' command..." << std::endl;

    // Implement the logic to turn the LED on.
    myStatusLed->write(true);
}

// --------------------------------------------------------------------------------
void Logic::handleOff() noexcept
{
    std::cout << std::endl << "Handling 'off' command..." << std::endl;

    // Implement the logic to turn the LED off.
    myStatusLed->write(false);
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
    myBlinkState = 0;
}

// --------------------------------------------------------------------------------
void Logic::handlePeriod(uint16_t periodLengthMs) noexcept
{
    std::cout << std::endl
              << "Handling 'period' command..." << periodLengthMs << " ms" << std::endl;

    if ((MinPeriodMs > periodLengthMs) || (MaxPeriodMs < periodLengthMs))
    {
        std::cout << "Invalid period (valid: " << MinPeriodMs << "-" << MaxPeriodMs
                  << " ms), keeping " << myPeriodLengthMs << " ms" << std::endl;
        return;
    }

    // Implement the logic to set the blinking period.
    myTimerDriver->setPeriod(periodLengthMs);
    myPeriodLengthMs = periodLengthMs; // Store the new period length
}

// --------------------------------------------------------------------------------
void Logic::handleStatus() noexcept
{
    // Implement the logic to print the current status.
    // This is a placeholder implementation - replace with actual status printing logic.
    mySerialDriver->print("Status:\n");
    mySerialDriver->print("Board: ");
    mySerialDriver->print(myBoard);
    mySerialDriver->print("\n");
    mySerialDriver->print("Board target: ");
    mySerialDriver->print(myBoardTarget);
    mySerialDriver->print("\n");
    mySerialDriver->print("Platform: ");
    mySerialDriver->print(myPlatform);
    mySerialDriver->print("\n");
    mySerialDriver->print("Blink State: ");
    mySerialDriver->print(myBlinkState ? "on\n" : "off\n");
    mySerialDriver->print("Blink Period: ");
    mySerialDriver->print(std::to_string(myPeriodLengthMs).c_str());
    mySerialDriver->print(" ms\n");
    mySerialDriver->print("Current Temperature: ");
    mySerialDriver->print(std::to_string(myTempSensor->read()).c_str());
    mySerialDriver->print(" °C\n");
}

// --------------------------------------------------------------------------------
void Logic::handleStore() noexcept
{
    std::cout << std::endl << "Handling 'store' command..." << std::endl;

    myNvsUserSettingsStorage->setString("blink_state", myBlinkState ? "on" : "off");
    myNvsUserSettingsStorage->setString("blink_period", std::to_string(myPeriodLengthMs));

    // Implement the logic to store the current state to non-volatile memory.
    // This is a placeholder implementation - replace with actual storage logic.
    mySerialDriver->print("Storing current state to non-volatile memory...\n");
}

// --------------------------------------------------------------------------------
void Logic::handleHang() noexcept
{
    std::cout << std::endl << "Handling 'hang' command..." << std::endl;
    std::cout << "Simulating a hung program: the watchdog is no longer kicked and restarts the "
                 "system in a few seconds"
              << std::endl;

    // Stop kicking the watchdog, as a hung program would. The flag rather than an endless loop
    // here: the rest of the program keeps running until the restart, so 'hang' can also be tested
    // on the host.
    myHung = true;
}

// --------------------------------------------------------------------------------
void Logic::loadStoredUserSettings() noexcept
{
    std::cout << std::endl << "Loading stored user settings..." << std::endl;

    std::string blinkState;
    std::string blinkPeriod;

    if (!(myNvsUserSettingsStorage->getString("blink_state", blinkState)))
        blinkState = "off"; // Default to "off" if not found
    if (!(myNvsUserSettingsStorage->getString("blink_period", blinkPeriod)))
        blinkPeriod = "1000"; // Default to 1000 ms if not found

    myBlinkState = (blinkState == "on");

    // Parse the stored period without exceptions (std::stoi throws on bad data, and this function
    // is noexcept) and without silent wrap-around (a cast to uint16_t turns 70000 into 4464).
    constexpr std::uint16_t DefaultPeriodMs{1000U};

    std::uint32_t parsed{};
    const auto [ptr, ec] =
        std::from_chars(blinkPeriod.data(), blinkPeriod.data() + blinkPeriod.size(), parsed);

    // Valid only if it is a number, the whole string was read and it is in the range that `period`
    // accepts (old or corrupt data in NVS falls back to the default)
    const bool valid{(std::errc{} == ec) && (blinkPeriod.data() + blinkPeriod.size() == ptr) &&
                     (MinPeriodMs <= parsed) && (MaxPeriodMs >= parsed)};

    if (valid) { myPeriodLengthMs = static_cast<std::uint16_t>(parsed); }
    else
    {
        std::cout << "Invalid stored blink period '" << blinkPeriod << "', using "
                  << DefaultPeriodMs << " ms" << std::endl;
        myPeriodLengthMs = DefaultPeriodMs;
    }

    // Implement the logic to load the stored state from non-volatile memory.
    // This is a placeholder implementation - replace with actual loading logic.
    mySerialDriver->print("Loaded user settings:\n");
    mySerialDriver->print("Blink State: ");
    mySerialDriver->print(myBlinkState ? "on\n" : "off\n");
    mySerialDriver->print("Blink Period: ");
    mySerialDriver->print(std::to_string(myPeriodLengthMs).c_str());
    mySerialDriver->print(" ms\n");

    myTimerDriver->setPeriod(myPeriodLengthMs);
    if (myBlinkState)
        myTimerDriver->start(); // Start the timer for blinking if it was previously on
}
} // namespace logic
