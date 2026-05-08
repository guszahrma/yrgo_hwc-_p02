#pragma once

#include "interface.h"
#include <cstddef>

namespace driver::serial
{
class Esp32s3 final : public Interface
{
public:
    /**
     * @brief Construct a new Serial Esp32s3 object
     * 
     * @param[in] baudRate Default 115200 
     */
    explicit Esp32s3(int baudRate = 115200) noexcept;

    /**
     * @brief Destroy the Esp32s3 object
     */
    ~Esp32s3() noexcept override;

    /**
     * @brief 
     * 
     * @param[in] text Pointer to the C-string to be printed.
     */
    void print(const char* text) noexcept override;

    /**
     * @brief ReadLine function
     * 
     * @param[out] buffer Pointer to the buffer where the string will be stored.
     * @param[in] maxSize Maximum number of bytes to read.
     * @return std::size_t Number of characters read.
     */
    std::size_t readLine(char* buffer, std::size_t maxSize) noexcept override;

private:
};
} // namespace driver::serial