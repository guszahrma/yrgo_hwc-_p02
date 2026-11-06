/**
 * @file Serial interface
 */
#pragma once

#include <cstddef>

namespace driver::serial
{
/**
 * @brief Interface class for serial
 */
class Interface
{
public:
    /**
     * @brief Destructor
     */
    virtual ~Interface() noexcept = default;

    /**
     * @brief Print function
     *
     * The text is sent as it is: '\n' is not turned into "\r\n". End lines with "\r\n", which
     * terminals need to start the next line at the left edge.
     *
     * @param[in] text Text to print
     */
    virtual void print(const char* text) noexcept = 0;

    /**
     * @brief Read line
     *
     * @return std::string
     */
    virtual bool readLine(char* buffer, std::size_t maxSize) noexcept = 0;
};
} // namespace driver::serial