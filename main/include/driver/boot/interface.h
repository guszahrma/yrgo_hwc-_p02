/**
 * @file Boot interface: the versions of the images, and confirming the running one after an update
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
     * @return True if the image is confirmed, false if confirming it failed
     */
    virtual bool confirm() noexcept = 0;
};
} // namespace driver::boot
