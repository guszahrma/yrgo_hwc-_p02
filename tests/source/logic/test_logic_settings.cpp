/**
 * @brief Unit tests for loading the stored user settings in logic::Logic's constructor.
 */
#include <memory>
#include <string>

#include "driver/factory/stub.h"
#include "logic/logic.h"
#include "logic/logic_test_helpers.h"
#include "gtest/gtest.h"

using logic_test::contains;
using logic_test::createLogic;
using logic_test::sendCommand;

TEST(LogicSettings, NoStoredSettingsGiveTheDefaults)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    const std::string output{sendCommand(factory, *logic, "status")};
    EXPECT_TRUE(contains(output, "Blink State: off\r\n"));
    EXPECT_TRUE(contains(output, "Blink Period: 1000 ms\r\n"));

    // The timer got the values too, not only what status prints.
    EXPECT_EQ(1000U, factory.timer().periodMs());
    EXPECT_FALSE(factory.timer().isRunning());
}

TEST(LogicSettings, StoredSettingsAreLoaded)
{
    driver::factory::Stub factory{};
    factory.presetNvsString("blink_state", "on");
    factory.presetNvsString("blink_period", "300");
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    const std::string output{sendCommand(factory, *logic, "status")};
    EXPECT_TRUE(contains(output, "Blink State: on\r\n"));
    EXPECT_TRUE(contains(output, "Blink Period: 300 ms\r\n"));

    EXPECT_EQ(300U, factory.timer().periodMs());
    EXPECT_TRUE(factory.timer().isRunning());
}

TEST(LogicSettings, InvalidStoredPeriodGivesTheDefault)
{
    // In the order of the list: not a number, digits then letters, far above MaxPeriodMs (10000),
    // just below MinPeriodMs (50), just above MaxPeriodMs, and empty.
    for (const char* stored : {"abc", "500abc", "70000", "49", "10001", ""})
    {
        driver::factory::Stub factory{};
        factory.presetNvsString("blink_period", stored);
        std::unique_ptr<logic::Logic> logic{};
        const std::string output{createLogic(factory, logic)};

        EXPECT_TRUE(contains(output, std::string{"Invalid stored blink period '"} + stored +
                                         "', using 1000 ms"))
            << stored;
        EXPECT_TRUE(contains(sendCommand(factory, *logic, "status"), "Blink Period: 1000 ms\r\n"))
            << stored;
        EXPECT_EQ(1000U, factory.timer().periodMs()) << stored;
    }
}

TEST(LogicSettings, StoreSurvivesARestart)
{
    // Before the restart: change the settings and store them.
    driver::factory::Stub before{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(before, logic);
    for (const char* command : {"blink on", "period 300", "store"})
    {
        sendCommand(before, *logic, command);
    }

    std::string blinkState{};
    std::string blinkPeriod{};
    ASSERT_TRUE(before.nvs().getString("blink_state", blinkState));
    ASSERT_TRUE(before.nvs().getString("blink_period", blinkPeriod));
    logic.reset();

    // After the restart: a new Logic on a factory with what the first one stored.
    driver::factory::Stub after{};
    after.presetNvsString("blink_state", blinkState);
    after.presetNvsString("blink_period", blinkPeriod);
    createLogic(after, logic);

    const std::string output{sendCommand(after, *logic, "status")};
    EXPECT_TRUE(contains(output, "Blink State: on\r\n"));
    EXPECT_TRUE(contains(output, "Blink Period: 300 ms\r\n"));

    EXPECT_EQ(300U, after.timer().periodMs());
    EXPECT_TRUE(after.timer().isRunning());
}
