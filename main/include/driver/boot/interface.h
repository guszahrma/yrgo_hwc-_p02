/**
 * @file Boot interface: the versions of the images, the restart for an update, and confirming or
 *       reverting the running image after it
 */
#pragma once

#include <cstdint>

namespace driver::boot
{
/**
 * @brief Version of an image: major.minor.revision+build, e.g. 2.0.0+0
 *
 * The same fields and sizes as the version in MCUboot's image header.
 */
struct Version
{
    std::uint8_t major{};     // 2 in 2.0.0+0
    std::uint8_t minor{};     // 0 in 2.0.0+0
    std::uint16_t revision{}; // The third number, 0 in 2.0.0+0
    std::uint32_t build{};    // The number after '+', 0 in 2.0.0+0
};

/**
 * @brief Result of Interface::confirm()
 */
enum class ConfirmResult : std::uint8_t
{
    Confirmed,        // The image is now confirmed
    AlreadyConfirmed, // It was confirmed already, nothing was written
    NotAllowed,       // This board's confirm mode does not let the product confirm
    Failed,           // The confirmation could not be written
};

/**
 * @brief Interface class for the boot driver: which image is running, and whether it is kept
 *
 * After an update, the bootloader starts the new image as a test. If the image is not confirmed
 * before the next reset, the bootloader goes back to the previous image. The second slot holds
 * a new image to install, or the previous image after an update.
 */
class Interface
{
public:
    /**
     * @brief Destructor
     */
    virtual ~Interface() noexcept = default;

    /**
     * @brief Get the version of the running image
     *
     * @return The version of the running image, 0.0.0+0 if it cannot be read
     */
    virtual Version version() const noexcept = 0;

    /**
     * @brief Check if the running image is confirmed
     *
     * @return True if the image is confirmed and is kept after a reset, false if it is being
     *         tested and the previous image comes back at the next reset
     */
    virtual bool isConfirmed() const noexcept = 0;

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
    virtual bool secondaryVersion(Version& version) const noexcept = 0;

    /**
     * @brief Confirm the running image, so it is kept after the next reset
     *
     * Does nothing if the image is already confirmed.
     *
     * @return Confirmed, AlreadyConfirmed, NotAllowed if only the client may confirm on this
     *         board, or Failed if the confirmation could not be written
     */
    virtual ConfirmResult confirm() noexcept = 0;

    /**
     * @brief Check if this board can be updated
     *
     * Without firmware update, only version() means anything: the running image is the only one.
     *
     * @return True if the board can receive, test, confirm and revert a new image, false otherwise
     */
    virtual bool isUpdateSupported() const noexcept = 0;

    /**
     * @brief Check if the image in the second slot will be started as a test at the next reset
     *
     * @return True if the image in the second slot is marked for test, false if it is not or if
     *         this cannot be read
     */
    virtual bool isSecondaryPending() const noexcept = 0;

    /**
     * @brief Check if a restart has been requested for a firmware update
     *
     * Ask this in every pass of the main loop. When it is true, shut down in an orderly way and
     * then call setReadyForRestart().
     *
     * @return True if a restart has been requested, false otherwise
     */
    virtual bool isRestartRequested() const noexcept = 0;

    /**
     * @brief Report that the shutdown is done, so the restart may go ahead
     *
     * Keep the main loop running after this call until the restart, so that the reply to the
     * restart request is sent.
     *
     * @return True if the restart goes ahead, false if none was requested or it is too late
     */
    virtual bool setReadyForRestart() noexcept = 0;

    /**
     * @brief Reject the running image and restart, so the bootloader goes back to the previous one
     *
     * Call it when the self-test after an update has failed. Shut down in an orderly way first, as
     * before setReadyForRestart().
     *
     * Does not return if the restart goes ahead.
     *
     * @return False if the image could not be reverted, for example because it is already confirmed
     */
    virtual bool revert() noexcept = 0;
};
} // namespace driver::boot
