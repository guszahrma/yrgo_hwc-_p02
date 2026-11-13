/**
 * @brief Unit tests for loading the stored user settings in logic::Logic's constructor.
 */
#include <memory>
#include <string>

#include "driver/factory/stub.h"
#include "logic/logic.h"
#include "gtest/gtest.h"

namespace
{
/**
 * @brief Create Logic and return what it printed while it was constructed.
 *
 * @param[in] factory The factory to create Logic with.
 * @param[out] logic The created Logic.
 *
 * @return Everything printed to stdout by the constructor (the serial stub prints there).
 */
std::string createLogic(driver::factory::Stub& factory, std::unique_ptr<logic::Logic>& logic)
{
    testing::internal::CaptureStdout();
    logic = std::make_unique<logic::Logic>(factory);
    return testing::internal::GetCapturedStdout();
}

/**
 * @brief Send the status command and return the reply.
 *
 * @param[in] factory The factory that Logic was created with.
 * @param[in] logic The Logic under test.
 *
 * @return Everything printed to stdout during one step().
 */
std::string status(driver::factory::Stub& factory, logic::Logic& logic)
{
    factory.serial().simulateInputData("status");
    testing::internal::CaptureStdout();
    logic.step();
    return testing::internal::GetCapturedStdout();
}

/**
 * @brief Check whether a text contains a substring.
 *
 * @param[in] text The text to search.
 * @param[in] part The substring to look for.
 *
 * @return True if part is found in text, false otherwise.
 */
bool contains(const std::string& text, const std::string& part)
{ return std::string::npos != text.find(part); }
} // namespace

TEST(LogicSettings, NoStoredSettingsGiveTheDefaults)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    const std::string output{status(factory, *logic)};
    EXPECT_TRUE(contains(output, "Blink State: off\r\n"));
    EXPECT_TRUE(contains(output, "Blink Period: 1000 ms\r\n"));
}

TEST(LogicSettings, StoredSettingsAreLoaded)
{
    driver::factory::Stub factory{};
    factory.presetNvsString("blink_state", "on");
    factory.presetNvsString("blink_period", "300");
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    const std::string output{status(factory, *logic)};
    EXPECT_TRUE(contains(output, "Blink State: on\r\n"));
    EXPECT_TRUE(contains(output, "Blink Period: 300 ms\r\n"));
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
        EXPECT_TRUE(contains(status(factory, *logic), "Blink Period: 1000 ms\r\n")) << stored;
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
        before.serial().simulateInputData(command);
        logic->step();
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

    const std::string output{status(after, *logic)};
    EXPECT_TRUE(contains(output, "Blink State: on\r\n"));
    EXPECT_TRUE(contains(output, "Blink Period: 300 ms\r\n"));
}
