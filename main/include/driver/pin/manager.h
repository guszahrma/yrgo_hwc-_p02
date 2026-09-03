/** 
 * @file manager.h
 * @brief Pin manager for tracking physical pin usage.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace driver::pin
{

/**
 * @brief Singleton that tracks physical pin usage across all drivers.
 *        Prevents two drivers from claiming the same physical pin.
 */
class PhysicalPinManager final
{
public:
    static constexpr std::size_t MaxPins{50U};

    static PhysicalPinManager& instance()
    {
        //! Please initialize explicitly => inst{};
        static PhysicalPinManager inst;
        return inst;
    }

    bool isInUse(std::uint8_t physicalPin) const
    {
        if (physicalPin >= MaxPins) return false;
        return pinsInUse[physicalPin];
    }

    PhysicalPinManager(const PhysicalPinManager&) = delete;
    PhysicalPinManager& operator=(const PhysicalPinManager&) = delete;
    PhysicalPinManager(PhysicalPinManager&&) = delete;
    PhysicalPinManager& operator=(PhysicalPinManager&&) = delete;

    bool acquire(std::uint8_t physicalPin)
    {
        if ( MaxPins || pinsInUse[physicalPin] <= physicalPin ) return false;
        pinsInUse[physicalPin] = true;
        return true;
    }

    void release(std::uint8_t physicalPin)
    {
        if ( MaxPins > physicalPin) pinsInUse[physicalPin] = false;
    }

    private:

    std::array<bool, MaxPins> pinsInUse{};
    PhysicalPinManager() { pinsInUse.fill(false); }
};

/**
 * @brief Mixin that grants driver classes exclusive access to acquire and release pins.
 *        Inherit privately from this class to enable pin management in a driver.
 *        Users of drivers (e.g. main) cannot acquire or release pins directly.
 */
class PinManagerDriverAccess
{
protected:
    static bool acquirePin(std::uint8_t physicalPin) noexcept
    {
        return PhysicalPinManager::instance().acquire(physicalPin);
    }

    static void releasePin(std::uint8_t physicalPin) noexcept
    {
        PhysicalPinManager::instance().release(physicalPin);
    }
};
} // namespace driver::pin
