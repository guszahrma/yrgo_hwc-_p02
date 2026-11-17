/**
 * @file Helpers for the logic::Logic tests: create Logic, send a command, search the output.
 *
 * Logic prints through the serial stub, which writes to stdout, so the helpers capture stdout.
 */
#pragma once

#include <memory>
#include <string>

#include "driver/factory/stub.h"
#include "logic/logic.h"
#include "gtest/gtest.h"

namespace logic_test
{
/**
 * @brief Create Logic and return what it printed while it was constructed.
 *
 * Logic's constructor loads the stored settings, so fill the factory's NVS
 * (factory::Stub::presetNvsString()) before calling this.
 *
 * @param[in] factory The factory to create Logic with.
 * @param[out] logic The created Logic.
 *
 * @return Everything printed to stdout by the constructor.
 */
inline std::string createLogic(driver::factory::Stub& factory, std::unique_ptr<logic::Logic>& logic)
{
    testing::internal::CaptureStdout();
    logic = std::make_unique<logic::Logic>(factory);
    return testing::internal::GetCapturedStdout();
}

/**
 * @brief Send one command to Logic and return what was printed while it was handled.
 *
 * @param[in] factory The factory that Logic was created with.
 * @param[in] logic The Logic under test.
 * @param[in] command The command line, without line ending.
 *
 * @return Everything printed to stdout during one step().
 */
inline std::string sendCommand(driver::factory::Stub& factory, logic::Logic& logic,
                               const std::string& command)
{
    factory.serial().simulateInputData(command.c_str());
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
inline bool contains(const std::string& text, const std::string& part)
{ return std::string::npos != text.find(part); }
} // namespace logic_test
