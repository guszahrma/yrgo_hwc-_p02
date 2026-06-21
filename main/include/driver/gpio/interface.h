//! @note File header missing.
#pragma once

#include <cstdint>

namespace driver::gpio
{
/**
 * @brief GPIO direction enumeration.
 */
enum class Direction : std::uint8_t
{
    //! @note This case is fine; as is PascalCase (my favorite).
    OUTPUT,         // 0
    INPUT_PULL_UP,  // 1
    INPUT_PULL_DOWN // 2
};

/**
 *  @brief GPIO interface class 
*/
class Interface
{
public:
    virtual ~Interface() noexcept = default;

    /**
     * @brief Read function
     * 
     * @return current state
     */
    //! @note Consider marking this method const (it's a read method).
    virtual bool read() noexcept = 0;

    /**
     * @brief Write function
     * 
     * @param state 
     */
    //! @note @param state is not documented.
    virtual void write(bool state) noexcept = 0;

    /**
     * @brief Toggle function
     */
    virtual void toggle() noexcept = 0;
};
} // namespace driver::gpio
