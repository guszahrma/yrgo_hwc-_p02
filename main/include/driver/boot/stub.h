/**
 * @file Boot driver stub
 */
#pragma once

#include <cstdint>
#include <cstdio>

#include "driver/boot/interface.h"

namespace driver::boot
{
/**
 * @brief Simulated boot driver for factory::Stub and tests, without a bootloader.
 *
 * A test chooses the running version, whether it is confirmed and what the second slot holds, and
 * can count the calls to confirm() (confirmCount()). confirm() always succeeds. With the simulate
 * functions, a test also chooses whether the board can be updated, whether the second slot is
 * marked for test and whether a restart has been requested, and reads back the calls to
 * readyForRestart() and selfTestDone(). Nothing restarts: a failed self-test is only recorded.
 */
class Stub final : public Interface
{
public:
    /**
     * @brief Constructor
     *
     * @param[in] version The version of the running image
     * @param[in] confirmed True if the running image is confirmed, false if it is being tested
     * @param[in] hasSecondary True if the second slot holds an image
     * @param[in] secondary The version of the image in the second slot (if hasSecondary is true)
     */
    explicit Stub(Version version = {1U, 0U, 0U, 0U}, bool confirmed = true,
                  bool hasSecondary = false, Version secondary = {}) noexcept
        : myVersion{version}
        , myConfirmed{confirmed}
        , myHasSecondary{hasSecondary}
        , mySecondary{secondary}
        , myConfirmCount{}
        , myUpdateSupported{true}
        , mySecondaryPending{false}
        , myRestartRequested{false}
        , myReadyCount{}
        , mySelfTestCount{}
        , mySelfTestPassed{false}
    {
        std::printf(
            "Stub boot: running v%u.%u.%u+%u (%s)\n", static_cast<unsigned>(myVersion.major),
            static_cast<unsigned>(myVersion.minor), static_cast<unsigned>(myVersion.revision),
            static_cast<unsigned>(myVersion.build), myConfirmed ? "confirmed" : "not confirmed");
    }

    /**
     * @brief Destructor
     */
    ~Stub() noexcept override = default;

    /**
     * @brief Get the version of the running image
     *
     * @return The version of the running image, 0.0.0+0 if it cannot be read
     */
    Version version() const noexcept override { return myVersion; }

    /**
     * @brief Check if the running image is confirmed
     *
     * @return True if the image is confirmed and is kept after a reset, false if it is being
     *         tested and the previous image comes back at the next reset
     */
    bool isConfirmed() const noexcept override { return myConfirmed; }

    /**
     * @brief Get the version of the image in the second slot, if there is one
     *
     * The image is either a new image to install, or the previous image after an update (the one
     * the bootloader goes back to if the running image is not confirmed).
     *
     * @param[out] version The version of the image in the second slot (unchanged if there is none)
     *
     * @return True if the second slot holds an image and its version was read, false otherwise
     */
    bool secondaryVersion(Version& version) const noexcept override
    {
        if (!myHasSecondary) { return false; }
        version = mySecondary;
        return true;
    }

    /**
     * @brief Confirm the running image, so it is kept after the next reset
     *
     * Does nothing if the image is already confirmed.
     *
     * @return True if the image is confirmed, false if confirming it failed
     */
    bool confirm() noexcept override
    {
        ++myConfirmCount;
        myConfirmed = true;
        return true;
    }

    /**
     * @brief Check if this board can be updated
     *
     * @return True if the board can receive, test, confirm and revert a new image, false otherwise
     */
    bool isUpdateSupported() const noexcept override { return myUpdateSupported; }

    /**
     * @brief Check if the image in the second slot will be started as a test at the next reset
     *
     * @return True if the image in the second slot is marked for test, false if it is not or if
     *         this cannot be read
     */
    bool isSecondaryPending() const noexcept override { return mySecondaryPending; }

    /**
     * @brief Check if a restart has been requested for a firmware update
     *
     * @return True if a restart has been requested, false otherwise
     */
    bool restartRequested() const noexcept override { return myRestartRequested; }

    /**
     * @brief Report that the shutdown is done, so the restart may go ahead
     *
     * @return True if a restart has been requested, false otherwise
     */
    bool readyForRestart() noexcept override
    {
        ++myReadyCount;
        return myRestartRequested;
    }

    /**
     * @brief Report the result of the self-test after an update
     *
     * A passed self-test confirms the running image; a failed one is only recorded.
     *
     * @param[in] passed True if the self-test passed, false if it failed
     *
     * @return True if the self-test passed, false otherwise
     */
    bool selfTestDone(const bool passed) noexcept override
    {
        ++mySelfTestCount;
        mySelfTestPassed = passed;
        if (passed) { myConfirmed = true; }
        return passed;
    }

    /**
     * @brief Get the number of calls to confirm()
     *
     * @return The number of calls to confirm(), also those made when already confirmed
     */
    std::uint32_t confirmCount() const noexcept { return myConfirmCount; }

    /**
     * @brief Choose whether this board can be updated
     *
     * @param[in] supported True if the board can be updated (the default), false otherwise
     */
    void simulateUpdateSupported(const bool supported) noexcept { myUpdateSupported = supported; }

    /**
     * @brief Choose whether the image in the second slot is marked for test
     *
     * @param[in] pending True if it is marked for test, false otherwise (the default)
     */
    void simulateSecondaryPending(const bool pending) noexcept { mySecondaryPending = pending; }

    /**
     * @brief Choose whether a restart has been requested, as the client's restart request would
     *
     * @param[in] requested True if a restart has been requested, false otherwise (the default)
     */
    void simulateRestartRequested(const bool requested) noexcept { myRestartRequested = requested; }

    /**
     * @brief Get the number of calls to readyForRestart()
     *
     * @return The number of calls to readyForRestart()
     */
    std::uint32_t readyForRestartCount() const noexcept { return myReadyCount; }

    /**
     * @brief Get the number of calls to selfTestDone()
     *
     * @return The number of calls to selfTestDone()
     */
    std::uint32_t selfTestCount() const noexcept { return mySelfTestCount; }

    /**
     * @brief Get the result given to the last call to selfTestDone()
     *
     * @return True if the last self-test passed, false if it failed or none has been reported
     */
    bool selfTestPassed() const noexcept { return mySelfTestPassed; }

    Stub(const Stub&)            = delete;
    Stub(Stub&&)                 = delete;
    Stub& operator=(const Stub&) = delete;
    Stub& operator=(Stub&&)      = delete;

private:
    /** Version of the running image */
    Version myVersion;

    /** True if the running image is confirmed */
    bool myConfirmed;

    /** True if the second slot holds an image */
    bool myHasSecondary;

    /** Version of the image in the second slot (only valid if myHasSecondary is true) */
    Version mySecondary;

    /** Number of calls to confirm() */
    std::uint32_t myConfirmCount;

    /** True if the board can be updated */
    bool myUpdateSupported;

    /** True if the image in the second slot is marked for test */
    bool mySecondaryPending;

    /** True if a restart has been requested */
    bool myRestartRequested;

    /** Number of calls to readyForRestart() */
    std::uint32_t myReadyCount;

    /** Number of calls to selfTestDone() */
    std::uint32_t mySelfTestCount;

    /** Result given to the last call to selfTestDone() */
    bool mySelfTestPassed;
};
} // namespace driver::boot
