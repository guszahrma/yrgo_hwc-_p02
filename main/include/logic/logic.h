/**
 * @file A simple logic class for handling basic logic operations.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "driver/factory/interface.h"

/**
 * @brief A simple logic class for handling basic logic operations.
 * it implements the commands:
 * - on → which activates the LED
 * - off → which deactivates the LED
 * - blink on → which makes the LED blink
 * - blink off → which deactivates the blinking (LED remains in its current state)
 * - period [period_length_ms] → which sets the blinking period to the specified length in
 * milliseconds (e.g., "period 500" sets a 500 ms period) If the LED is currently blinking, it
 * should immediately apply the new period. If the LED is not currently blinking, it should store
 * the new period and use it when blinking is activated.
 * - status → prints the current status of the system, including:
 *           board, board target and platform (e.g. daisy_seed, daisy_seed/stm32h750xx/ram_load,
 *           Zephyr)
 *           blinkstate (on/off)
 *           period_length_ms (ms)
 *           current temperature (°C)
 * - hang → stops kicking the watchdog, as a hung program would, to test it: the system restarts
 *
 * When the client requests a restart for a firmware update, Logic shuts down (LED off, blinking
 * stopped) and reports that it is ready; until the restart, commands are rejected.
 */

namespace logic
{
class Logic
{
public:
    /**
     * @brief Constructs a new Logic instance.
     * Uses the factory to create the necessary drivers and initializes the system state.
     * Initialises Led as off and loads the stored user settings (blink state and period); without
     * stored settings, blinking is off and the period is 1000 ms.
     *
     * @param[in] factory Factory that creates the drivers for the current platform
     *                    (e.g. driver::factory::Esp32s3 or driver::factory::Stub).
     */
    explicit Logic(driver::factory::Interface& factory) noexcept;
    ~Logic() noexcept;

    /**
     * @brief Runs the application: prints the welcome text and then calls step() forever. Never
     *        returns.
     *
     * Commands come from the serial input; MQTT and other sources may be added later.
     */
    void run() noexcept;

    /**
     * @brief Runs one pass of the main loop: kicks the watchdog, handles a command if a line has
     *        arrived, and toggles the LED when the blink period has elapsed.
     *
     * run() calls it forever on the board; tests call it one pass at a time.
     */
    void step() noexcept;

    Logic(const Logic&)            = delete;
    Logic(Logic&&)                 = delete;
    Logic& operator=(const Logic&) = delete;
    Logic& operator=(Logic&&)      = delete;

private:
    void handleOn() noexcept;
    void handleOff() noexcept;
    void handleBlinkOn() noexcept;
    void handleBlinkOff() noexcept;
    void handlePeriod(std::uint16_t periodLengthMs) noexcept;
    void handleStatus() noexcept;
    void handleHelp() noexcept;
    void handleUnknownCommand(const std::string_view& cmd) noexcept;
    void handleStore() noexcept;
    void handleHang() noexcept;
    void handleConfirm() noexcept;
    void handleSelfTest(const std::string_view& args) noexcept;
    void printVersion(const driver::boot::Version& version) noexcept;
    void printSelfTestBehaviour() noexcept;
    void loadStoredUserSettings() noexcept;
    void prepareForRestart() noexcept;
    void waitForRestart() noexcept;
    void shutDown() noexcept;

    /**
     * How the self-test after the next update behaves (demo, the command 'selftest'): it passes and
     * confirms the image, fails and reverts, or does not report, so the user confirms by hand or
     * the next reset reverts.
     */
    enum class SelfTestBehaviour : std::uint8_t
    {
        Pass,
        Fail,
        Skip,
    };

    /** The name of a self-test behaviour, as stored in NVS and typed after 'selftest'. */
    static const char* selfTestName(SelfTestBehaviour behaviour) noexcept;

    /** One line of input from the serial port, including the terminating '\0'. */
    static constexpr std::size_t InputLineSize{64U};

    /**
     * One output line formatted with std::snprintf before print(), including the '\0'.
     * std::snprintf cuts off anything longer.
     */
    static constexpr std::size_t OutputLineSize{96U};

    /**
     * The text around the command in the longest output line, the unknown-command line:
     * "\r\nHandling unknown command: " and "\r\n".
     */
    static constexpr std::size_t UnknownCommandTextSize{30U};

    static_assert(OutputLineSize >= InputLineSize + UnknownCommandTextSize,
                  "An output line must hold a whole input line (an unknown command) plus its text");

    /**
     * Reference voltage of the temperature sensor's ADC in volts: the TMP36 is supplied from 3V3.
     * Only ESP-IDF uses it; on Zephyr the ADC's range comes from devicetree.
     */
    static constexpr float TempSensorReferenceVoltage{3.3F};

    /** Blink period at start, and when no valid period is stored. */
    static constexpr std::uint16_t DefaultPeriodMs{1000U};

    /** Valid range for the blink period; other values are rejected. */
    static constexpr std::uint16_t MinPeriodMs{50U};
    static constexpr std::uint16_t MaxPeriodMs{10000U};

    std::uint16_t myPeriodLengthMs; // Blinking period in milliseconds
    bool myBlinkState;              // false for off, true for on
    bool myHung;                    // true after 'hang': the watchdog is no longer kicked
    bool myRestarting; // true after setReadyForRestart() went ahead: commands are rejected
    SelfTestBehaviour mySelfTestBehaviour; // The self-test after the next update ('selftest')
    const char* myBoard;                   // Board, from the factory (printed by 'status')
    const char* myBoardTarget;             // Board target, from the factory (printed by 'status')
    const char* myPlatform; // ESP-IDF or Zephyr, from the factory (printed by 'status')
    std::unique_ptr<driver::serial::Interface> mySerialDriver; // Serial driver for communication
    std::unique_ptr<driver::gpio::Interface> myStatusLed; // Status LED (role GpioId::StatusLed)
    std::unique_ptr<driver::adc::Interface>
        myTempSensorAdc; // ADC for the temperature sensor (role AdcId::TempSensor)
    std::unique_ptr<driver::timer::Interface>
        myTimerDriver; // Timer driver for handling blinking timing
    std::unique_ptr<driver::tempsensor::Interface> myTempSensor; // TMP36 temperature sensor driver
    std::unique_ptr<driver::nvs::Interface>
        myNvsUserSettingsStorage; // NVS driver for non-volatile storage of Usersettings
    std::unique_ptr<driver::watchdog::Interface> myWatchdog;
    std::unique_ptr<driver::boot::Interface>
        myBoot; // Boot driver: firmware versions, update support, confirm
};
} // namespace logic
