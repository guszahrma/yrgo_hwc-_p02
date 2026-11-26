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
 * whether the board can be updated. The factories use the constructor for boards without a real
 * boot driver. With the simulate functions, a test changes that state later, chooses what confirm()
 * returns, marks the second slot for test and requests a restart, and it reads back the calls to
 * confirm(), setReadyForRestart() and revert(). Nothing restarts: revert() only counts and returns.
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
     * @param[in] updateSupported True if the board can be updated
     */
    explicit Stub(Version version = {1U, 0U, 0U, 0U}, bool confirmed = true,
                  bool hasSecondary = false, Version secondary = {},
                  bool updateSupported = true) noexcept
        : myVersion{version}
        , myConfirmed{confirmed}
        , myHasSecondary{hasSecondary}
        , mySecondary{secondary}
        , myConfirmCount{}
        , myConfirmResult{ConfirmResult::Confirmed}
        , myUpdateSupported{updateSupported}
        , mySecondaryPending{false}
        , myRestartRequested{false}
        , mySetReadyForRestartCount{}
        , myRevertCount{}
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
     * @return AlreadyConfirmed if the image was confirmed already, otherwise Confirmed or the
     * result chosen with simulateConfirmResult()
     */
    ConfirmResult confirm() noexcept override
    {
        ++myConfirmCount;
        if (myConfirmed) { return ConfirmResult::AlreadyConfirmed; }
        if (ConfirmResult::Confirmed != myConfirmResult) { return myConfirmResult; }
        myConfirmed = true;
        return ConfirmResult::Confirmed;
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
    bool isRestartRequested() const noexcept override { return myRestartRequested; }

    /**
     * @brief Report that the shutdown is done, so the restart may go ahead
     *
     * @return True if a restart has been requested, false otherwise
     */
    bool setReadyForRestart() noexcept override
    {
        ++mySetReadyForRestartCount;
        return myRestartRequested;
    }

    /**
     * @brief Reject the running image and restart, so the bootloader goes back to the previous one
     *
     * Nothing restarts: the call is only counted.
     *
     * @return False if the image is already confirmed, true where the board would restart
     */
    bool revert() noexcept override
    {
        ++myRevertCount;
        return !myConfirmed;
    }

    /**
     * @brief Get the number of calls to confirm()
     *
     * @return The number of calls to confirm(), also those made when already confirmed
     */
    std::uint32_t confirmCount() const noexcept { return myConfirmCount; }

    /**
     * @brief Choose whether the running image is confirmed
     *
     * @param[in] confirmed True if it is confirmed, false if it is being tested
     */
    void simulateConfirmed(const bool confirmed) noexcept { myConfirmed = confirmed; }

    /**
     * @brief Choose what confirm() returns for an image that is not confirmed
     *
     * @param[in] result Confirmed (the default) confirms the image; NotAllowed or Failed leave it
     *                   not confirmed, as a board where only the client confirms, or a failed write
     */
    void simulateConfirmResult(const ConfirmResult result) noexcept { myConfirmResult = result; }

    /**
     * @brief Choose what the second slot holds
     *
     * @param[in] hasSecondary True if the second slot holds an image
     * @param[in] secondary The version of the image in the second slot (if hasSecondary is true)
     */
    void simulateSecondary(const bool hasSecondary, const Version secondary = {}) noexcept
    {
        myHasSecondary = hasSecondary;
        mySecondary    = secondary;
    }

    /**
     * @brief Choose whether this board can be updated
     *
     * @param[in] supported True if the board can be updated, false otherwise
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
     * @brief Get the number of calls to setReadyForRestart()
     *
     * @return The number of calls to setReadyForRestart()
     */
    std::uint32_t setReadyForRestartCount() const noexcept { return mySetReadyForRestartCount; }

    /**
     * @brief Get the number of calls to revert()
     *
     * @return The number of calls to revert(), also those made when already confirmed
     */
    std::uint32_t revertCount() const noexcept { return myRevertCount; }

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

    /** What confirm() returns for an image that is not confirmed */
    ConfirmResult myConfirmResult;

    /** True if the board can be updated */
    bool myUpdateSupported;

    /** True if the image in the second slot is marked for test */
    bool mySecondaryPending;

    /** True if a restart has been requested */
    bool myRestartRequested;

    /** Number of calls to setReadyForRestart() */
    std::uint32_t mySetReadyForRestartCount;

    /** Number of calls to revert() */
    std::uint32_t myRevertCount;
};
} // namespace driver::boot
