/**
 * @brief Unit tests for logic::Logic with factory::Stub
 */
#include <cstdint>
#include <string>

#include "driver/factory/stub.h"
#include "logic/logic.h"
#include "gtest/gtest.h"

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
