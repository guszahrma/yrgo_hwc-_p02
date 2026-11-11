/**
 * @file Stub factory
 */
#pragma once

#include <cassert>
#include <map>
#include <string>

#include "driver/adc/stub.h"
#include "driver/boot/stub.h"
#include "driver/factory/interface.h"
#include "driver/gpio/stub.h"
#include "driver/nvs/stub.h"
#include "driver/pin/stub.h"
#include "driver/serial/stub.h"
#include "driver/tempsensor/stub.h"
#include "driver/timer/stub.h"
#include "driver/watchdog/stub.h"

namespace driver::factory
{
/**
 * @brief Factory that creates stub drivers, for running Logic on the host in tests.
 *
 * The factory keeps a pointer to every stub it creates, so that a test can reach the stubs after
 * Logic has taken them over, e.g. to simulate input or check what Logic did. Logic owns the stubs:
 * the accessors (statusLed(), watchdog(), ...) may only be used while that Logic object exists.
 */
class Stub final : public Interface
{
public:
    /**
     * @brief Construct a new Stub object
     */
    Stub() noexcept = default;

    /**
     * @brief Destroy the Stub object
     */
    ~Stub() noexcept override = default;

    /**
     * @brief Create a stub GPIO driver for a role.
     *
     * @param[in] id        The GPIO role, mapped to a stub pin.
     * @param[in] direction Pin direction.
     *
     * @return unique pointer to the stub GPIO driver.
     */
    std::unique_ptr<driver::gpio::Interface>
    createGpio(GpioId id, driver::gpio::Direction direction) noexcept override
    {
        switch (id)
        {
            case GpioId::StatusLed:
            {
                auto gpio{std::make_unique<driver::gpio::Stub>(
                    static_cast<std::uint8_t>(driver::pin::stub::GpioPin::D6), direction)};
                myStatusLed = gpio.get(); // Logic owns the stub; keep a pointer for the tests
                return gpio;
            }
        }

        // Unreachable while every GpioId is handled above.
        return nullptr;
    }

    /**
     * @brief Create a stub ADC driver for a role.
     *
     * @param[in] id               The ADC role, mapped to a stub pin.
     * @param[in] referenceVoltage The reference voltage in volts.
     *
     * @return unique pointer to the stub ADC driver.
     */
    std::unique_ptr<driver::adc::Interface> createAdc(AdcId id,
                                                      float referenceVoltage) noexcept override
    {
        switch (id)
        {
            case AdcId::TempSensor:
            {
                auto adc{std::make_unique<driver::adc::Stub>(driver::pin::stub::AdcPin::A6,
                                                             referenceVoltage)};
                myTempSensorAdc = adc.get(); // Logic owns the stub; keep a pointer for the tests
                return adc;
            }
        }

        // Unreachable while every AdcId is handled above.
        return nullptr;
    }

    /**
     * @brief Create a stub NVS storage.
     *
     * @param[in] namespaceName The NVS namespace.
     *
     * @return unique pointer to the stub NVS storage.
     */
    std::unique_ptr<driver::nvs::Interface> createNvs(const char* namespaceName) noexcept override
    {
        auto nvs{std::make_unique<driver::nvs::Stub>(namespaceName)};
        for (const auto& [key, value] : myNvsPreset)
        {
            nvs->setString(key.c_str(), value);
        }
        myNvs = nvs.get(); // Logic owns the stub; keep a pointer for the tests
        return nvs;
    }

    /**
     * @brief Store a string in the NVS stub that createNvs() creates next, as if it had been saved
     *        before a restart.
     *
     *        Call it before creating Logic, which creates the NVS stub in its constructor.
     *
     * @param[in] key The key, e.g. "blink_period".
     * @param[in] value The stored value.
     */
    void presetNvsString(const std::string& key, const std::string& value) noexcept
    { myNvsPreset[key] = value; }

    /**
     * @brief Create a stub serial driver.
     *
     * @return unique pointer to the stub serial driver.
     */
    std::unique_ptr<driver::serial::Interface> createSerial() noexcept override
    {
        auto serial{std::make_unique<driver::serial::Stub>()};
        mySerial = serial.get(); // Logic owns the stub; keep a pointer for the tests
        return serial;
    }

    /**
     * @brief Create a stub temperature sensor.
     *
     * @param[in] adc The ADC the sensor would read from (not used by the stub).
     *
     * @return unique pointer to the stub temperature sensor.
     */
    std::unique_ptr<driver::tempsensor::Interface>
    createTempSensor(driver::adc::Interface& adc) noexcept override
    {
        (void)(adc);
        auto tempSensor{std::make_unique<driver::tempsensor::Stub>(static_cast<std::uint16_t>(0))};
        myTempSensor = tempSensor.get(); // Logic owns the stub; keep a pointer for the tests
        return tempSensor;
    }

    /**
     * @brief Create a stub timer.
     *
     * @return unique pointer to the stub timer.
     */
    std::unique_ptr<driver::timer::Interface> createTimer() noexcept override
    {
        auto timer{std::make_unique<driver::timer::Stub>()};
        myTimer = timer.get(); // Logic owns the stub; keep a pointer for the tests
        return timer;
    }

    /**
     * @brief Create a watchdog that restarts the system if it is not kicked in time
     *
     * @param[in] timeoutMs Time in milliseconds without a kick before the system restarts
     *
     * @return unique pointer to the simulated watchdog
     */
    std::unique_ptr<driver::watchdog::Interface>
    createWatchdog(std::uint32_t timeoutMs) noexcept override
    {
        auto watchdog{std::make_unique<driver::watchdog::Stub>(timeoutMs)};
        myWatchdog = watchdog.get(); // Logic owns the stub; keep a pointer for the tests
        return watchdog;
    }

    /**
     * @brief Create a simulated boot driver (v1.0.0, confirmed, nothing in the second slot).
     *
     * @return unique pointer to the boot driver stub.
     */
    std::unique_ptr<driver::boot::Interface> createBoot() noexcept override
    {
        auto boot{std::make_unique<driver::boot::Stub>()};
        myBoot = boot.get(); // Logic owns the stub; keep a pointer for the tests
        return boot;
    }

    /**
     * @brief The board: "stub" in the tests.
     *
     * @return C-string literal.
     */
    const char* board() const noexcept override { return "stub"; }

    /**
     * @brief The board target: "stub" in the tests.
     *
     * @return C-string literal.
     */
    const char* boardTarget() const noexcept override { return "stub"; }

    /**
     * @brief The platform: "stub" in the tests.
     *
     * @return C-string literal.
     */
    const char* platform() const noexcept override { return "stub"; }

    /**
     * @brief The status LED stub created by createGpio(GpioId::StatusLed, ...).
     *
     * @return reference to the GPIO stub.
     */
    driver::gpio::Stub& statusLed() noexcept
    {
        assert(nullptr != myStatusLed && "createGpio(GpioId::StatusLed) has not been called");
        return *myStatusLed;
    }

    /**
     * @brief The temperature sensor's ADC stub created by createAdc(AdcId::TempSensor, ...).
     *
     * @return reference to the ADC stub.
     */
    driver::adc::Stub& tempSensorAdc() noexcept
    {
        assert(nullptr != myTempSensorAdc && "createAdc(AdcId::TempSensor) has not been called");
        return *myTempSensorAdc;
    }

    /**
     * @brief The NVS stub created by createNvs().
     *
     * @return reference to the NVS stub.
     */
    driver::nvs::Stub& nvs() noexcept
    {
        assert(nullptr != myNvs && "createNvs() has not been called");
        return *myNvs;
    }

    /**
     * @brief The serial stub created by createSerial().
     *
     * @return reference to the serial stub.
     */
    driver::serial::Stub& serial() noexcept
    {
        assert(nullptr != mySerial && "createSerial() has not been called");
        return *mySerial;
    }

    /**
     * @brief The temperature sensor stub created by createTempSensor().
     *
     * @return reference to the temperature sensor stub.
     */
    driver::tempsensor::Stub& tempSensor() noexcept
    {
        assert(nullptr != myTempSensor && "createTempSensor() has not been called");
        return *myTempSensor;
    }

    /**
     * @brief The timer stub created by createTimer().
     *
     * @return reference to the timer stub.
     */
    driver::timer::Stub& timer() noexcept
    {
        assert(nullptr != myTimer && "createTimer() has not been called");
        return *myTimer;
    }

    /**
     * @brief The watchdog stub created by createWatchdog().
     *
     * @return reference to the watchdog stub.
     */
    driver::watchdog::Stub& watchdog() noexcept
    {
        assert(nullptr != myWatchdog && "createWatchdog() has not been called");
        return *myWatchdog;
    }

    /**
     * @brief The boot driver stub created by createBoot().
     *
     * @return reference to the boot driver stub.
     */
    driver::boot::Stub& boot() noexcept
    {
        assert(nullptr != myBoot && "createBoot() has not been called");
        return *myBoot;
    }

    Stub(const Stub&)            = delete;
    Stub(Stub&&)                 = delete;
    Stub& operator=(const Stub&) = delete;
    Stub& operator=(Stub&&)      = delete;

private:
    // The stubs created last, owned by Logic (see the class description).
    driver::gpio::Stub* myStatusLed{nullptr};
    driver::adc::Stub* myTempSensorAdc{nullptr};
    driver::nvs::Stub* myNvs{nullptr};
    driver::serial::Stub* mySerial{nullptr};
    driver::tempsensor::Stub* myTempSensor{nullptr};
    driver::timer::Stub* myTimer{nullptr};
    driver::watchdog::Stub* myWatchdog{nullptr};
    driver::boot::Stub* myBoot{nullptr};

    /** Strings put into every NVS stub that createNvs() creates (see presetNvsString()). */
    std::map<std::string, std::string> myNvsPreset{};
};
} // namespace driver::factory
