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
class PhysicalPinManager
{
public:
    static constexpr std::size_t MaxPins{50U};

    static PhysicalPinManager& instance()
    {
        static PhysicalPinManager inst;
        return inst;
    }

    bool is_in_use(std::uint8_t physicalPin) const
    {
        if (physicalPin >= MaxPins) return false;
        return pinsInUse[physicalPin];
    }

    PhysicalPinManager(const PhysicalPinManager&) = delete;
    PhysicalPinManager& operator=(const PhysicalPinManager&) = delete;
    PhysicalPinManager(PhysicalPinManager&&) = delete;
    PhysicalPinManager& operator=(PhysicalPinManager&&) = delete;

private:
    bool acquire(std::uint8_t physicalPin)
    {
        if (physicalPin >= MaxPins || pinsInUse[physicalPin]) return false;
        pinsInUse[physicalPin] = true;
        return true;
    }

    void release(std::uint8_t physicalPin)
    {
        if (physicalPin < MaxPins) pinsInUse[physicalPin] = false;
    }

    std::array<bool, MaxPins> pinsInUse{};
    PhysicalPinManager() { pinsInUse.fill(false); }

    friend class PinManagerDriverAccess;
};

/**
 * @brief Mixin that grants driver classes exclusive access to acquire and release pins.
 *        Inherit privately from this class to enable pin management in a driver.
 *        Users of drivers (e.g. main) cannot acquire or release pins directly.
 */
class PinManagerDriverAccess
{
protected:
    static bool acquire_pin(std::uint8_t physicalPin)
    {
        return PhysicalPinManager::instance().acquire(physicalPin);
    }

    static void release_pin(std::uint8_t physicalPin)
    {
        PhysicalPinManager::instance().release(physicalPin);
    }
};

} // namespace driver::pin
