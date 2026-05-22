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
    virtual ~Interface() = default;

    /**
     * @brief Print function
     * 
     * @param[in] text 
     */
    virtual void print(const char* text) noexcept = 0;

    /**
     * @brief Read line
     * 
     * @return std::string 
     */
    virtual bool readLine(char* buffer, std::size_t maxSize) noexcept = 0;
};
}