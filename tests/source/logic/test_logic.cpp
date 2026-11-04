/**
 * @brief Unit tests for logic::Logic with factory::Stub
 */
#include <cstdint>

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
