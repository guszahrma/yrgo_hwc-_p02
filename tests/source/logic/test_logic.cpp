/**
 * @brief Unit tests for logic::Logic with factory::Stub
 */
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