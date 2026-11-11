/**
 * @brief Unit tests for logic::Logic with factory::Stub
 */
#include <cstdint>
#include <string>

#include "driver/factory/stub.h"
#include "logic/logic.h"
#include "gtest/gtest.h"

namespace
{
/**
 * @brief Send one command to Logic and return what was printed while it was handled.
 *
 * @param[in] factory The factory that Logic was created with.
 * @param[in] logic The Logic under test.
 * @param[in] command The command line, without line ending.
 *
 * @return Everything printed to stdout during one step() (the serial stub prints there).
 */
std::string sendCommand(driver::factory::Stub& factory, logic::Logic& logic, const char* command)
{
    factory.serial().simulateInputData(command);
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
bool contains(const std::string& text, const char* part)
{ return std::string::npos != text.find(part); }
} // namespace

TEST(Logic, StepKicksWatchdogOncePerPass)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    logic.step();
    logic.step();
    logic.step();

    EXPECT_EQ(3U, factory.watchdog().kickCount());
}

TEST(Logic, HangStopsKickingTheWatchdog)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    // 'hang' must return, so the test gets control back (handleHang() used to loop forever).
    factory.serial().simulateInputData("hang");
    logic.step();
    const std::uint32_t kicksAtHang{factory.watchdog().kickCount()};

    logic.step();
    logic.step();

    // No kicks after 'hang': on the board, the watchdog now restarts the system.
    EXPECT_EQ(kicksAtHang, factory.watchdog().kickCount());
}

TEST(Logic, StatusPrintsBoardBoardTargetAndPlatform)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    // The serial stub prints to std::cout.
    factory.serial().simulateInputData("status");
    testing::internal::CaptureStdout();
    logic.step();
    const std::string output{testing::internal::GetCapturedStdout()};

    EXPECT_NE(std::string::npos, output.find("Board: stub\r\n"));
    EXPECT_NE(std::string::npos, output.find("Board target: stub\r\n"));
    EXPECT_NE(std::string::npos, output.find("Platform: stub\r\n"));
}

TEST(Logic, PeriodAcceptsAValidValue)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    const std::string output{sendCommand(factory, logic, "period 500")};
    EXPECT_FALSE(contains(output, "Invalid period"));
    EXPECT_FALSE(contains(output, "Usage: period"));

    EXPECT_TRUE(contains(sendCommand(factory, logic, "status"), "Blink Period: 500 ms\r\n"));

    EXPECT_EQ(500U, factory.timer().periodMs());
}

TEST(Logic, PeriodAcceptsTheLimits)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    // MinPeriodMs = 50 and MaxPeriodMs = 10000 in logic.h.
    sendCommand(factory, logic, "period 50");
    EXPECT_TRUE(contains(sendCommand(factory, logic, "status"), "Blink Period: 50 ms\r\n"));
    EXPECT_EQ(50U, factory.timer().periodMs());

    sendCommand(factory, logic, "period 10000");
    EXPECT_TRUE(contains(sendCommand(factory, logic, "status"), "Blink Period: 10000 ms\r\n"));
    EXPECT_EQ(10000U, factory.timer().periodMs());
}

TEST(Logic, PeriodOutsideTheLimitsKeepsTheOldPeriod)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    // Not the default (1000 ms), so "keeps the old period" cannot pass by falling back to it.
    sendCommand(factory, logic, "period 300");

    for (const char* command : {"period 49", "period 10001", "period 0"})
    {
        const std::string output{sendCommand(factory, logic, command)};
        EXPECT_TRUE(contains(output, "Invalid period (valid: 50-10000 ms), keeping 300 ms"))
            << command;
        EXPECT_TRUE(contains(sendCommand(factory, logic, "status"), "Blink Period: 300 ms\r\n"))
            << command;
        EXPECT_EQ(300U, factory.timer().periodMs()) << command;
    }
}

TEST(Logic, PeriodRejectsAnArgumentThatIsNotANumber)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    // Not the default (1000 ms), so "keeps the old period" cannot pass by falling back to it.
    sendCommand(factory, logic, "period 300");

    // 70000 does not fit in std::uint16_t (max 65535), so std::from_chars reports out of range.
    for (const char* command :
         {"period", "period abc", "period 500abc", "period -5", "period 70000"})
    {
        const std::string output{sendCommand(factory, logic, command)};
        EXPECT_TRUE(contains(output, "Usage: period <ms>")) << command;
        EXPECT_TRUE(contains(sendCommand(factory, logic, "status"), "Blink Period: 300 ms\r\n"))
            << command;
        EXPECT_EQ(300U, factory.timer().periodMs()) << command;
    }
}

TEST(Logic, OnAndOffSetTheStatusLed)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    sendCommand(factory, logic, "on");
    EXPECT_TRUE(factory.statusLed().read());

    sendCommand(factory, logic, "off");
    EXPECT_FALSE(factory.statusLed().read());
}

TEST(Logic, BlinkOnAndOffChangeTheBlinkState)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    // A new factory::Stub has empty NVS, so blinking starts off.
    EXPECT_TRUE(contains(sendCommand(factory, logic, "status"), "Blink State: off\r\n"));
    EXPECT_FALSE(factory.timer().isRunning());

    sendCommand(factory, logic, "blink on");
    EXPECT_TRUE(contains(sendCommand(factory, logic, "status"), "Blink State: on\r\n"));
    EXPECT_TRUE(factory.timer().isRunning());

    sendCommand(factory, logic, "blink off");
    EXPECT_TRUE(contains(sendCommand(factory, logic, "status"), "Blink State: off\r\n"));
    EXPECT_FALSE(factory.timer().isRunning());
}

TEST(Logic, BlinkWithoutOnOrOffPrintsUsage)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    for (const char* command : {"blink", "blink maybe"})
    {
        EXPECT_TRUE(contains(sendCommand(factory, logic, command), "Usage: blink on | blink off"))
            << command;
    }
}

TEST(Logic, StoreSavesBlinkStateAndPeriod)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    sendCommand(factory, logic, "blink on");
    sendCommand(factory, logic, "period 300");
    sendCommand(factory, logic, "store");

    std::string blinkState{};
    std::string blinkPeriod{};
    ASSERT_TRUE(factory.nvs().getString("blink_state", blinkState));
    ASSERT_TRUE(factory.nvs().getString("blink_period", blinkPeriod));
    EXPECT_EQ("on", blinkState);
    EXPECT_EQ("300", blinkPeriod);
}

TEST(Logic, UnknownCommandIsNamedInTheReply)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    const std::string output{sendCommand(factory, logic, "dance")};
    EXPECT_TRUE(contains(output, "Handling unknown command: dance\r\n"));
    EXPECT_TRUE(contains(output, "Type 'help'"));
}

TEST(Logic, HelpListsEveryCommand)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    const std::string output{sendCommand(factory, logic, "help")};
    // Whole lines: the output also echoes the input ("Reading line...help"), and "on" is part of
    // "blink on".
    for (const char* line :
         {"  on\r\n", "  off\r\n", "  blink on\r\n", "  blink off\r\n", "  period <ms>\r\n",
          "  status\r\n", "  store\r\n", "  hang  (", "  help\r\n"})
    {
        EXPECT_TRUE(contains(output, line)) << line;
    }
}

TEST(Logic, StatusPrintsFirmwareVersions)
{
    driver::factory::Stub factory{};
    logic::Logic logic{factory};

    factory.serial().simulateInputData("status");
    testing::internal::CaptureStdout();
    logic.step();
    const std::string output{testing::internal::GetCapturedStdout()};

    EXPECT_NE(std::string::npos, output.find("Firmware: v1.0.0+0 (confirmed)\r\n"));
    EXPECT_NE(std::string::npos, output.find("Second slot: empty\r\n"));
}
