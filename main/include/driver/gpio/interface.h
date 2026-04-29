#pragma once

#include <cstdint>

namespace driver::gpio
{

// TODO Ändra till class
enum class Direction : std::uint8_t
{
    OUTPUT,         // 0
    INPUT_PULL_UP,  // 1
    INPUT_PULL_DOWN // 2
};

class Interface
{
public:

    virtual ~Interface() noexcept = default;

    /**
     * @brief Read function
     * 
     * @return true 
     * @return false 
     */
    virtual bool read() noexcept = 0;

    /**
     * @brief Write function
     * 
     * @param state 
     */
    virtual void write(bool state) noexcept = 0;

    /**
     * @brief Toggle function
     */
    virtual void toggle() noexcept = 0;
};
}