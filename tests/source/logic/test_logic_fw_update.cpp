/**
 * @brief Unit tests for logic::Logic's firmware update commands and lines (step A1): the firmware
 *        lines in 'status', 'confirm', 'selftest' and 'help', with and without firmware update.
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

namespace
{
/** Version of the image in the second slot in these tests. */
constexpr driver::boot::Version SecondaryVersion{2U, 0U, 0U, 0U};
} // namespace

// ---------------------------------------------------------------------------- status

TEST(LogicFwUpdate, StatusShowsAConfirmedImageAndAnEmptySecondSlot)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    const std::string output{sendCommand(factory, *logic, "status")};
    EXPECT_TRUE(contains(output, "Firmware: v1.0.0+0 (confirmed)\r\n"));
    EXPECT_TRUE(contains(output, "Second slot: empty\r\n"));
    EXPECT_FALSE(contains(output, "Firmware update: not supported"));
}

TEST(LogicFwUpdate, StatusTellsWhatToDoWithAnImageNotConfirmed)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateConfirmed(false);

    const std::string output{sendCommand(factory, *logic, "status")};
    EXPECT_TRUE(contains(output, "Firmware: v1.0.0+0 (not confirmed: type 'confirm' to keep it, or "
                                 "reset to go back)\r\n"));
}

TEST(LogicFwUpdate, StatusShowsTheSecondSlotWithAndWithoutTheTestMark)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateSecondary(true, SecondaryVersion);

    std::string output{sendCommand(factory, *logic, "status")};
    EXPECT_TRUE(contains(output, "Second slot: v2.0.0+0\r\n"));

    factory.boot().simulateSecondaryPending(true);
    output = sendCommand(factory, *logic, "status");
    EXPECT_TRUE(contains(output, "Second slot: v2.0.0+0 (test at next reset)\r\n"));
}

TEST(LogicFwUpdate, StatusNeverMarksAnEmptySecondSlot)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateSecondaryPending(true);

    const std::string output{sendCommand(factory, *logic, "status")};
    EXPECT_TRUE(contains(output, "Second slot: empty\r\n"));
    EXPECT_FALSE(contains(output, "(test at next reset)"));
}

TEST(LogicFwUpdate, StatusWithoutFirmwareUpdateShowsOnlyTheVersion)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateUpdateSupported(false);
    factory.boot().simulateSecondary(true, SecondaryVersion);

    const std::string output{sendCommand(factory, *logic, "status")};
    EXPECT_TRUE(contains(output, "Firmware: v1.0.0+0\r\nFirmware update: not supported\r\n"));
    EXPECT_FALSE(contains(output, "Second slot"));
    EXPECT_FALSE(contains(output, "Self-test"));
}

// ---------------------------------------------------------------------------- confirm

TEST(LogicFwUpdate, ConfirmConfirmsAnImageBeingTested)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateConfirmed(false);

    const std::string output{sendCommand(factory, *logic, "confirm")};
    EXPECT_TRUE(contains(output, "Firmware v1.0.0+0 confirmed: kept after the next reset\r\n"));
    EXPECT_EQ(1U, factory.boot().confirmCount());
    EXPECT_TRUE(factory.boot().isConfirmed());
}

TEST(LogicFwUpdate, ConfirmOnAConfirmedImageSaysSo)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    const std::string output{sendCommand(factory, *logic, "confirm")};
    EXPECT_TRUE(contains(output, "Firmware v1.0.0+0 is already confirmed\r\n"));
}

TEST(LogicFwUpdate, ConfirmWhereOnlyTheClientConfirmsIsNotAllowed)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateConfirmed(false);
    factory.boot().simulateConfirmResult(driver::boot::ConfirmResult::NotAllowed);

    const std::string output{sendCommand(factory, *logic, "confirm")};
    EXPECT_TRUE(contains(output, "Confirm not allowed: only the client confirms on this board "
                                 "(smpmgr image confirm)\r\n"));
    EXPECT_FALSE(factory.boot().isConfirmed());
}

TEST(LogicFwUpdate, ConfirmThatFailsSaysThePreviousVersionComesBack)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateConfirmed(false);
    factory.boot().simulateConfirmResult(driver::boot::ConfirmResult::Failed);

    const std::string output{sendCommand(factory, *logic, "confirm")};
    EXPECT_TRUE(contains(output, "Confirm failed: v1.0.0+0 is still being tested, the previous "
                                 "version comes back at the next reset\r\n"));
    EXPECT_FALSE(factory.boot().isConfirmed());
}

TEST(LogicFwUpdate, ConfirmWithoutFirmwareUpdateIsRefused)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateUpdateSupported(false);
    factory.boot().simulateConfirmed(false);

    const std::string output{sendCommand(factory, *logic, "confirm")};
    EXPECT_TRUE(contains(output, "Firmware update not supported on this board\r\n"));
    EXPECT_EQ(0U, factory.boot().confirmCount());
}

// ---------------------------------------------------------------------------- selftest

TEST(LogicFwUpdate, SelfTestBehaviourIsSkipByDefault)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    const std::string output{sendCommand(factory, *logic, "status")};
    EXPECT_TRUE(contains(output, "Self-test behaviour after next update: skip (demo: confirm by "
                                 "hand, or the next reset reverts)\r\n"));
}

TEST(LogicFwUpdate, SelfTestSetsTheBehaviourShownInStatus)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    std::string output{sendCommand(factory, *logic, "selftest fail")};
    EXPECT_TRUE(contains(output, "the self-test will fail (demo)"));
    EXPECT_TRUE(contains(output, "Save with 'store' to keep it after a reset\r\n"));
    output = sendCommand(factory, *logic, "status");
    EXPECT_TRUE(contains(output, "Self-test behaviour after next update: fail (demo)\r\n"));

    sendCommand(factory, *logic, "selftest pass");
    output = sendCommand(factory, *logic, "status");
    EXPECT_TRUE(contains(output, "Self-test behaviour after next update: pass (demo)\r\n"));

    sendCommand(factory, *logic, "selftest skip");
    output = sendCommand(factory, *logic, "status");
    EXPECT_TRUE(contains(output, "Self-test behaviour after next update: skip (demo:"));
}

TEST(LogicFwUpdate, SelfTestWithAWrongArgumentPrintsUsageAndKeepsTheBehaviour)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    sendCommand(factory, *logic, "selftest fail");

    for (const char* command : {"selftest", "selftest maybe", "selftest pass now"})
    {
        const std::string output{sendCommand(factory, *logic, command)};
        EXPECT_TRUE(contains(output, "Usage: selftest pass|fail|skip\r\n")) << command;
    }
    const std::string output{sendCommand(factory, *logic, "status")};
    EXPECT_TRUE(contains(output, "Self-test behaviour after next update: fail (demo)\r\n"));
}

TEST(LogicFwUpdate, SelfTestWithoutFirmwareUpdateIsRefused)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateUpdateSupported(false);

    const std::string output{sendCommand(factory, *logic, "selftest pass")};
    EXPECT_TRUE(contains(output, "Firmware update not supported on this board\r\n"));
    EXPECT_FALSE(contains(output, "Save with 'store'"));
}

TEST(LogicFwUpdate, StoreSavesTheSelfTestBehaviour)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    sendCommand(factory, *logic, "selftest fail");
    sendCommand(factory, *logic, "store");

    std::string selfTest{};
    ASSERT_TRUE(factory.nvs().getString("selftest", selfTest));
    EXPECT_EQ("fail", selfTest);
}

TEST(LogicFwUpdate, StoredSelfTestBehaviourIsLoaded)
{
    driver::factory::Stub factory{};
    factory.presetNvsString("selftest", "pass");
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    const std::string output{sendCommand(factory, *logic, "status")};
    EXPECT_TRUE(contains(output, "Self-test behaviour after next update: pass (demo)\r\n"));
}

TEST(LogicFwUpdate, UnknownStoredSelfTestBehaviourGivesSkip)
{
    driver::factory::Stub factory{};
    factory.presetNvsString("selftest", "maybe");
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    const std::string output{sendCommand(factory, *logic, "status")};
    EXPECT_TRUE(contains(output, "Self-test behaviour after next update: skip (demo:"));
}

// ---------------------------------------------------------------------------- help

TEST(LogicFwUpdate, HelpListsConfirmAndSelfTest)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);

    const std::string output{sendCommand(factory, *logic, "help")};
    EXPECT_TRUE(contains(output, "  confirm  (keeps the running firmware after an update)\r\n"));
    EXPECT_TRUE(contains(output, "  selftest pass|fail|skip  ("));
}

TEST(LogicFwUpdate, HelpWithoutFirmwareUpdateLeavesConfirmAndSelfTestOut)
{
    driver::factory::Stub factory{};
    std::unique_ptr<logic::Logic> logic{};
    createLogic(factory, logic);
    factory.boot().simulateUpdateSupported(false);

    const std::string output{sendCommand(factory, *logic, "help")};
    EXPECT_FALSE(contains(output, "  confirm"));
    EXPECT_FALSE(contains(output, "  selftest"));
    EXPECT_TRUE(contains(output, "  hang  ("));
}
