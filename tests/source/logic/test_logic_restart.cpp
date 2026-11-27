/**
 * @brief Unit tests for logic::Logic's shutdown before a restart for a firmware update (step A3):
 *        the lines, the shutdown, rejected commands, a declined restart and a restart that does
 *        not come.
 */
#include <cstdint>
#include <memory>
#include <string>

#include "driver/factory/stub.h"
#include "logic/logic.h"
#include "logic/logic_test_helpers.h"
#include "gtest/gtest.h"

using logic_test::contains;
using logic_test::createLogic;
using logic_test::sendCommand;

namespace
{
/** Version of the image in the second slot in these tests. */
constexpr driver::boot::Version SecondaryVersion{2U, 0U, 0U, 0U};

/**
 * @brief Run one pass of the main loop without input and return what was printed.
 *
 * @param[in] logic The Logic under test.
 *
 * @return Everything printed to stdout during one step().
 */
std::string stepWithoutInput(logic::Logic& logic)
{
    testing::internal::CaptureStdout();
    logic.step();
    return testing::internal::GetCapturedStdout();
}
} // namespace

// ---------------------------------------------------------------------------- shutdown

TEST(LogicRestart, NothingHappensWithoutARestartRequest)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    const std::string output{stepWithoutInput(*logic)};
    EXPECT_FALSE(contains(output, "Preparing for restart"));
    EXPECT_EQ(0U, factory.boot().setReadyForRestartCount());
}

TEST(LogicRestart, RestartRequestShutsDownAndReportsReady)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    sendCommand(factory, *logic, "on");
    sendCommand(factory, *logic, "blink on");
    factory.boot().simulateRestartRequested(true);

    const std::string output{stepWithoutInput(*logic)};
    // The stubs print their own lines in between, so check each line and their order
    const auto preparing{output.find("Preparing for restart: firmware not changed\r\n")};
    const auto ready{output.find("Ready for restart\r\n")};
    ASSERT_NE(std::string::npos, preparing);
    ASSERT_NE(std::string::npos, ready);
    EXPECT_LT(preparing, ready);
    EXPECT_FALSE(factory.statusLed().read());
    EXPECT_FALSE(factory.timer().isRunning());
    EXPECT_EQ(1U, factory.boot().setReadyForRestartCount());
    EXPECT_FALSE(factory.boot().isRestartRequested());
    EXPECT_FALSE(contains(output, "Restart declined"));
}

TEST(LogicRestart, RestartRequestNamesTheImageMarkedForTest)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateSecondary(true, SecondaryVersion);
    factory.boot().simulateSecondaryPending(true);
    factory.boot().simulateRestartRequested(true);

    const std::string output{stepWithoutInput(*logic)};
    EXPECT_TRUE(contains(output, "Preparing for restart: v2.0.0+0 will be started as a test\r\n"));
}

TEST(LogicRestart, ShutdownHappensOncePerRequest)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateRestartRequested(true);

    stepWithoutInput(*logic);
    const std::string output{stepWithoutInput(*logic)};
    EXPECT_FALSE(contains(output, "Preparing for restart"));
    EXPECT_EQ(1U, factory.boot().setReadyForRestartCount());
}

// ---------------------------------------------------------------------------- after "ready"

TEST(LogicRestart, CommandsAreRejectedUntilTheRestart)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateRestartRequested(true);
    stepWithoutInput(*logic);

    std::string output{sendCommand(factory, *logic, "store")};
    EXPECT_TRUE(contains(output, "Preparing for restart: command rejected\r\n"));
    EXPECT_FALSE(contains(output, "Handling 'store'"));
    std::string blinkState{};
    EXPECT_FALSE(factory.nvs().getString("blink_state", blinkState));

    output = sendCommand(factory, *logic, "on");
    EXPECT_TRUE(contains(output, "Preparing for restart: command rejected\r\n"));
    EXPECT_FALSE(factory.statusLed().read());
}

TEST(LogicRestart, WatchdogIsKickedWhileWaitingForTheRestart)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateRestartRequested(true);
    stepWithoutInput(*logic);

    const std::uint32_t kicksAtReady{factory.watchdog().kickCount()};
    stepWithoutInput(*logic);
    EXPECT_EQ(kicksAtReady + 1U, factory.watchdog().kickCount());
}

TEST(LogicRestart, WatchdogRestartsTheBoardIfTheRestartDoesNotCome)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateRestartRequested(true);
    stepWithoutInput(*logic); // "Ready for restart"

    // One more pass kicks the watchdog, then gives up: Logic does not wait any longer than that
    const std::string output{stepWithoutInput(*logic)};
    EXPECT_TRUE(contains(
        output, "Restart is about to happen: stopping the watchdog kicks, the board will restart "
                "soon\r\n"));

    // No kicks after that: on the board, the watchdog now restarts the system
    const std::uint32_t kicksAtGivingUp{factory.watchdog().kickCount()};
    const std::string nextOutput{stepWithoutInput(*logic)};
    EXPECT_EQ(kicksAtGivingUp, factory.watchdog().kickCount());
    EXPECT_FALSE(contains(nextOutput, "about to happen"));
}

// ---------------------------------------------------------------------------- declined

TEST(LogicRestart, DeclinedRestartContinuesWithTheStoredSettings)
{
    driver::factory::Stub factory{};
    factory.presetNvsString("blink_state", "on");
    factory.presetNvsString("blink_period", "300");
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    sendCommand(factory, *logic, "period 200"); // Not stored: lost, as at a reset
    factory.boot().simulateRestartTimedOut(true);
    factory.boot().simulateRestartRequested(true);

    const std::string output{stepWithoutInput(*logic)};
    EXPECT_TRUE(contains(output, "Ready for restart\r\n"));
    EXPECT_TRUE(contains(output, "Restart declined (too late): firmware not changed, continuing "
                                 "with the stored settings\r\n"));
    EXPECT_TRUE(contains(output, "Loading stored user settings..."));
    EXPECT_TRUE(factory.timer().isRunning());
    EXPECT_EQ(300U, factory.timer().periodMs());
    EXPECT_FALSE(factory.statusLed().read());
}

TEST(LogicRestart, CommandsWorkAgainAfterADeclinedRestart)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateRestartTimedOut(true);
    factory.boot().simulateRestartRequested(true);
    stepWithoutInput(*logic);

    const std::string output{sendCommand(factory, *logic, "on")};
    EXPECT_FALSE(contains(output, "command rejected"));
    EXPECT_TRUE(factory.statusLed().read());
}

TEST(LogicRestart, NewRequestAfterADeclinedRestartShutsDownAgain)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateRestartTimedOut(true);
    factory.boot().simulateRestartRequested(true);
    stepWithoutInput(*logic);

    factory.boot().simulateRestartTimedOut(false);
    factory.boot().simulateRestartRequested(true);
    const std::string output{stepWithoutInput(*logic)};
    EXPECT_TRUE(contains(output, "Ready for restart\r\n"));
    EXPECT_FALSE(contains(output, "Restart declined"));
    EXPECT_EQ(2U, factory.boot().setReadyForRestartCount());
}
