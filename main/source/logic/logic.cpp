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
#include <string>
#include <string_view>
#include <thread>

#include "logic/logic.h"

namespace logic
{
// All output goes through the serial driver, not std::cout: <iostream> costs ~160 KB of flash on
// Zephyr (streams and locales). Lines with numbers are formatted with std::snprintf first.

// --------------------------------------------------------------------------------
Logic::Logic(driver::factory::Interface& factory) noexcept
    : myPeriodLengthMs{DefaultPeriodMs} // Until loadStoredUserSettings() reads the stored one
    , myBlinkState{false}               // Initialize blink state to off
    , myHung{false}                     // Kick the watchdog until 'hang'
    , mySelfTestBehaviour{SelfTestBehaviour::Skip} // Until the stored one is loaded
    , myBoard{factory.board()}
    , myBoardTarget{factory.boardTarget()}
    , myPlatform{factory.platform()}
{
    // Create the drivers for this platform, then load the stored user settings.
    mySerialDriver = factory.createSerial();
    myStatusLed =
        factory.createGpio(driver::factory::GpioId::StatusLed, driver::gpio::Direction::OUTPUT);
    myTempSensorAdc =
        factory.createAdc(driver::factory::AdcId::TempSensor, TempSensorReferenceVoltage);
    myTempSensor  = factory.createTempSensor(*myTempSensorAdc);
    myTimerDriver = factory.createTimer();
    myTimerDriver->stop(); // Blinking is off until 'blink on' or the stored settings start it
    myNvsUserSettingsStorage = factory.createNvs("user_settings");
    myWatchdog               = factory.createWatchdog(WatchdogTimeoutMs);
    myBoot                   = factory.createBoot();

    if (myWatchdog->causedLastReset())
    {
        mySerialDriver->print("Last reset was caused by the watchdog\r\n");
    }

    loadStoredUserSettings();
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
    mySerialDriver->print(
        "write relevant command, write help to get a list of available commands\r\n");

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
    // Must be at least one FreeRTOS tick (10 ms with CONFIG_FREERTOS_HZ=100): shorter sleeps
    // busy-wait in ESP-IDF, so the idle task never runs and the task watchdog fires
    constexpr std::uint8_t sleepDuration_ms{10};

    char buffer[InputLineSize]{};

    // Tell the watchdog that the loop is still running. After 'hang' (see handleHang()) the
    // kicks stop, and the watchdog restarts the system as if the loop had hung.
    if (!myHung) { myWatchdog->kick(); }

    // Handle a command if a whole line has arrived; otherwise sleep briefly so the loop does not
    // busy-wait.
    if (mySerialDriver->readLine(buffer, sizeof(buffer)))
    {
        mySerialDriver->print("\r\n\r\nReading line...");
        mySerialDriver->print(buffer);
        mySerialDriver->print("\r\n");

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
                mySerialDriver->print("Usage: blink on | blink off\r\n");
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
                mySerialDriver->print("Usage: period <ms>, e.g. period 500\r\n");
        }
        else if (cmd == "store")
            handleStore();
        else if (cmd == "confirm")
            handleConfirm();
        else if (cmd == "selftest")
            handleSelfTest(args);
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
    mySerialDriver->print("\r\nHandling 'Help' command...\r\n"
                          "\r\nAvailable commands:\r\n"
                          "  on\r\n"
                          "  off\r\n"
                          "  blink on\r\n"
                          "  blink off\r\n"
                          "  period <ms>\r\n"
                          "  status\r\n"
                          "  store\r\n");

    // Only the commands that work on this board
    if (myBoot->isUpdateSupported())
    {
        mySerialDriver->print("  confirm  (keeps the running firmware after an update)\r\n"
                              "  selftest pass|fail|skip  (demo: how the self-test after the next "
                              "update behaves; save with 'store')\r\n");
    }
    mySerialDriver->print("  hang  (tests the watchdog: the board restarts after a few seconds)\r\n"
                          "  help\r\n");
}

// --------------------------------------------------------------------------------
void Logic::handleUnknownCommand(const std::string_view& cmd) noexcept
{
    // A string_view is not null-terminated: %.*s prints exactly cmd.size() characters.
    char line[OutputLineSize]{};
    std::snprintf(line, sizeof(line), "\r\nHandling unknown command: %.*s\r\n",
                  static_cast<int>(cmd.size()), cmd.data());
    mySerialDriver->print(line);
    mySerialDriver->print("Type 'help' to see the list of available commands.\r\n");
}

// --------------------------------------------------------------------------------
void Logic::handleOn() noexcept
{
    mySerialDriver->print("\r\nHandling 'on' command...\r\n");

    myStatusLed->write(true);
}

// --------------------------------------------------------------------------------
void Logic::handleOff() noexcept
{
    mySerialDriver->print("\r\nHandling 'off' command...\r\n");

    myStatusLed->write(false);
}

// --------------------------------------------------------------------------------
void Logic::handleBlinkOn() noexcept
{
    mySerialDriver->print("\r\nHandling 'blink on' command...\r\n");

    myTimerDriver->start(); // Start the timer for blinking
    myBlinkState = true;
}

// --------------------------------------------------------------------------------
void Logic::handleBlinkOff() noexcept
{
    mySerialDriver->print("\r\nHandling 'blink off' command...\r\n");

    myTimerDriver->stop(); // Stop the timer for blinking
    myBlinkState = 0;
}

// --------------------------------------------------------------------------------
void Logic::handlePeriod(uint16_t periodLengthMs) noexcept
{
    char line[OutputLineSize]{};
    std::snprintf(line, sizeof(line), "\r\nHandling 'period' command...%u ms\r\n",
                  static_cast<unsigned>(periodLengthMs));
    mySerialDriver->print(line);

    if ((MinPeriodMs > periodLengthMs) || (MaxPeriodMs < periodLengthMs))
    {
        std::snprintf(line, sizeof(line), "Invalid period (valid: %u-%u ms), keeping %u ms\r\n",
                      static_cast<unsigned>(MinPeriodMs), static_cast<unsigned>(MaxPeriodMs),
                      static_cast<unsigned>(myPeriodLengthMs));
        mySerialDriver->print(line);
        return;
    }

    myTimerDriver->setPeriod(periodLengthMs);
    myPeriodLengthMs = periodLengthMs; // Store the new period length
}

// --------------------------------------------------------------------------------
void Logic::handleStatus() noexcept
{
    mySerialDriver->print("Status:\r\n");
    mySerialDriver->print("Board: ");
    mySerialDriver->print(myBoard);
    mySerialDriver->print("\r\n");
    mySerialDriver->print("Board target: ");
    mySerialDriver->print(myBoardTarget);
    mySerialDriver->print("\r\n");
    mySerialDriver->print("Platform: ");
    mySerialDriver->print(myPlatform);
    mySerialDriver->print("\r\n");

    // The firmware: the running image and, with firmware update, the image in the second slot (a
    // new image to install, or the previous one after an update) and the demo self-test
    mySerialDriver->print("Firmware: ");
    printVersion(myBoot->version());
    if (!myBoot->isUpdateSupported())
    {
        mySerialDriver->print("\r\nFirmware update: not supported\r\n");
    }
    else
    {
        mySerialDriver->print(myBoot->isConfirmed()
                                  ? " (confirmed)\r\n"
                                  : " (not confirmed: type 'confirm' to keep it, or reset to go "
                                    "back)\r\n");

        // Without an image in the second slot there is nothing to swap in, so no test mark
        driver::boot::Version secondary{};
        if (myBoot->secondaryVersion(secondary))
        {
            mySerialDriver->print("Second slot: ");
            printVersion(secondary);
            mySerialDriver->print(myBoot->isSecondaryPending() ? " (test at next reset)\r\n"
                                                               : "\r\n");
        }
        else
        {
            mySerialDriver->print("Second slot: empty\r\n");
        }
        printSelfTestBehaviour();
    }
    mySerialDriver->print("Blink State: ");
    mySerialDriver->print(myBlinkState ? "on\r\n" : "off\r\n");
    mySerialDriver->print("Blink Period: ");
    mySerialDriver->print(std::to_string(myPeriodLengthMs).c_str());
    mySerialDriver->print(" ms\r\n");
    mySerialDriver->print("Current Temperature: ");
    mySerialDriver->print(std::to_string(myTempSensor->read()).c_str());
    mySerialDriver->print(" °C\r\n");
}

// --------------------------------------------------------------------------------
void Logic::handleStore() noexcept
{
    mySerialDriver->print("\r\nHandling 'store' command...\r\n");

    myNvsUserSettingsStorage->setString("blink_state", myBlinkState ? "on" : "off");
    myNvsUserSettingsStorage->setString("blink_period", std::to_string(myPeriodLengthMs));
    myNvsUserSettingsStorage->setString("selftest", selfTestName(mySelfTestBehaviour));

    mySerialDriver->print("Storing current state to non-volatile memory...\r\n");
}

// --------------------------------------------------------------------------------
void Logic::handleHang() noexcept
{
    mySerialDriver->print("\r\nHandling 'hang' command...\r\n");
    mySerialDriver->print(
        "Simulating a hung program: the watchdog is no longer kicked and restarts "
        "the system in a few seconds\r\n");

    // Stop kicking the watchdog, as a hung program would. The flag rather than an endless loop
    // here: the rest of the program keeps running until the restart, so 'hang' can also be tested
    // on the host.
    myHung = true;
}

// --------------------------------------------------------------------------------
void Logic::handleConfirm() noexcept
{
    mySerialDriver->print("\r\nHandling 'confirm' command...\r\n");

    if (!myBoot->isUpdateSupported())
    {
        mySerialDriver->print("Firmware update not supported on this board\r\n");
        return;
    }

    switch (myBoot->confirm())
    {
        case driver::boot::ConfirmResult::Confirmed:
            mySerialDriver->print("Firmware ");
            printVersion(myBoot->version());
            mySerialDriver->print(" confirmed: kept after the next reset\r\n");
            break;
        case driver::boot::ConfirmResult::AlreadyConfirmed:
            mySerialDriver->print("Firmware ");
            printVersion(myBoot->version());
            mySerialDriver->print(" is already confirmed\r\n");
            break;
        case driver::boot::ConfirmResult::NotAllowed:
            mySerialDriver->print(
                "Confirm not allowed: only the client confirms on this board (smpmgr image "
                "confirm)\r\n");
            break;
        case driver::boot::ConfirmResult::Failed:
            mySerialDriver->print("Confirm failed: ");
            printVersion(myBoot->version());
            mySerialDriver->print(
                " is still being tested, the previous version comes back at the next reset\r\n");
            break;
    }
}

// --------------------------------------------------------------------------------
void Logic::handleSelfTest(const std::string_view& args) noexcept
{
    mySerialDriver->print("\r\nHandling 'selftest' command...\r\n");

    if (!myBoot->isUpdateSupported())
    {
        mySerialDriver->print("Firmware update not supported on this board\r\n");
        return;
    }

    if (args == "pass")
    {
        mySelfTestBehaviour = SelfTestBehaviour::Pass;
        mySerialDriver->print(
            "After the next update, the self-test will pass (demo): the update is kept.");
    }
    else if (args == "fail")
    {
        mySelfTestBehaviour = SelfTestBehaviour::Fail;
        mySerialDriver->print("After the next update, the self-test will fail (demo): the "
                              "previous version comes back.");
    }
    else if (args == "skip")
    {
        mySelfTestBehaviour = SelfTestBehaviour::Skip;
        mySerialDriver->print("After the next update, the self-test will not report (demo): "
                              "confirm by hand, or the next reset reverts.");
    }
    else
    {
        mySerialDriver->print("Usage: selftest pass|fail|skip\r\n");
        return;
    }
    mySerialDriver->print(" Save with 'store' to keep it after a reset\r\n");
}

// --------------------------------------------------------------------------------
void Logic::printVersion(const driver::boot::Version& version) noexcept
{
    char text[OutputLineSize]{};
    std::snprintf(text, sizeof(text), "v%u.%u.%u+%u", static_cast<unsigned>(version.major),
                  static_cast<unsigned>(version.minor), static_cast<unsigned>(version.revision),
                  static_cast<unsigned>(version.build));
    mySerialDriver->print(text);
}

// --------------------------------------------------------------------------------
void Logic::printSelfTestBehaviour() noexcept
{
    mySerialDriver->print("Self-test behaviour after next update: ");
    mySerialDriver->print(selfTestName(mySelfTestBehaviour));
    mySerialDriver->print(SelfTestBehaviour::Skip == mySelfTestBehaviour
                              ? " (demo: confirm by hand, or the next reset reverts)\r\n"
                              : " (demo)\r\n");
}

// --------------------------------------------------------------------------------
const char* Logic::selfTestName(const SelfTestBehaviour behaviour) noexcept
{
    switch (behaviour)
    {
        case SelfTestBehaviour::Pass:
            return "pass";
        case SelfTestBehaviour::Fail:
            return "fail";
        case SelfTestBehaviour::Skip:
            break;
    }
    return "skip";
}

// --------------------------------------------------------------------------------
void Logic::loadStoredUserSettings() noexcept
{
    mySerialDriver->print("\r\nLoading stored user settings...\r\n");

    std::string blinkState;
    std::string blinkPeriod;
    std::string selfTest;

    if (!(myNvsUserSettingsStorage->getString("blink_state", blinkState)))
        blinkState = "off"; // Default to "off" if not found
    if (!(myNvsUserSettingsStorage->getString("blink_period", blinkPeriod)))
        blinkPeriod = std::to_string(DefaultPeriodMs); // Default if not found

    myBlinkState = (blinkState == "on");

    // Missing or unknown gives skip: an update is not kept unless someone chooses it
    if (!(myNvsUserSettingsStorage->getString("selftest", selfTest))) { selfTest = "skip"; }
    mySelfTestBehaviour = ("pass" == selfTest)   ? SelfTestBehaviour::Pass
                          : ("fail" == selfTest) ? SelfTestBehaviour::Fail
                                                 : SelfTestBehaviour::Skip;

    // Parse the stored period without exceptions (std::stoi throws on bad data, and this function
    // is noexcept) and without silent wrap-around (a cast to uint16_t turns 70000 into 4464).
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
        char line[OutputLineSize]{};
        std::snprintf(line, sizeof(line), "Invalid stored blink period '%s', using %u ms\r\n",
                      blinkPeriod.c_str(), static_cast<unsigned>(DefaultPeriodMs));
        mySerialDriver->print(line);
        myPeriodLengthMs = DefaultPeriodMs;
    }

    mySerialDriver->print("Loaded user settings:\r\n");
    mySerialDriver->print("Blink State: ");
    mySerialDriver->print(myBlinkState ? "on\r\n" : "off\r\n");
    mySerialDriver->print("Blink Period: ");
    mySerialDriver->print(std::to_string(myPeriodLengthMs).c_str());
    mySerialDriver->print(" ms\r\n");
    if (myBoot->isUpdateSupported()) { printSelfTestBehaviour(); }

    myTimerDriver->setPeriod(myPeriodLengthMs);
    if (myBlinkState)
        myTimerDriver->start(); // Start the timer for blinking if it was previously on
}
} // namespace logic
