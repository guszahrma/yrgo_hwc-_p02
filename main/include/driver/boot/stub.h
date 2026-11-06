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
 * can count the calls to confirm() (confirmCount()). confirm() always succeeds.
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
     * @brief Get the number of calls to confirm()
     *
     * @return The number of calls to confirm(), also those made when already confirmed
     */
    std::uint32_t confirmCount() const noexcept { return myConfirmCount; }

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
};
} // namespace driver::boot