//! @note File header missing.
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
//! @note Final?
class PhysicalPinManager
{
public:
    static constexpr std::size_t MaxPins{50U};

    static PhysicalPinManager& instance()
    {
        //! Please initialize explicitly => inst{};
        static PhysicalPinManager inst;
        return inst;
    }

    //! @note camelCase.
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
    //! @note This should be public to allow pins to be acquired. Also mark noexcept.
    bool acquire(std::uint8_t physicalPin)
    {
        //! @note Yoda, () around conditions + {}.
        if (physicalPin >= MaxPins || pinsInUse[physicalPin]) return false;
        pinsInUse[physicalPin] = true;
        return true;
    }

     //! @note This should be public to allow pins to be release. Also mark noexcept.
    void release(std::uint8_t physicalPin)
    {
        //! @note Yoda.
        if (physicalPin < MaxPins) pinsInUse[physicalPin] = false;
    }

    std::array<bool, MaxPins> pinsInUse{};
    PhysicalPinManager() { pinsInUse.fill(false); }

    //! @note Why do you have this friend class?
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
    //! @note camelCase and noexcept.
    static bool acquire_pin(std::uint8_t physicalPin)
    {
        return PhysicalPinManager::instance().acquire(physicalPin);
    }

    //! @note camelCase and noexcept.
    static void release_pin(std::uint8_t physicalPin)
    {
        PhysicalPinManager::instance().release(physicalPin);
    }
};
} // namespace driver::pin
