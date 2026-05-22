/**
 * @brief Stub
 */
#pragma once

#include <cstdint>
#include <iostream>

#include "interface.h"

namespace driver::serial
{
/**
 * @brief Stub class for Serial interface.
 * Simulates UART using standard terminal input/output with \n for line handling.
 */
class Stub final : public Interface
{
public:
    /**
     * @brief Construct a new Stub object
     */
    explicit Stub() noexcept
        : myBuf{}
        , myBufLen{}
    {
        std::cout << "Stub constructed!" << std::endl;
    }

    /**
     * @brief Destroy the Stub object
     * 
     */
    ~Stub() noexcept override = default;

    /**
     * @brief Print function
     * 
     * @param[in] text Pointer to the C-string to be printed.
     */
    void print(const char* text) noexcept override
    {
        if (text != nullptr)
        {
            std::cout << text << std::flush;
        }
    }

    /**
     * @brief ReadLine function
     * 
     * @param[out] buffer Pointer to the buffer where the string will be stored.
     * @param[in] maxSize Maximum number of bytes to read.
     * 
     * @return True if a simulated line was successfully copied else return false
     */
    bool readLine(char* buffer, std::size_t maxSize) noexcept override
    {
        // Check the given buffer, return false if invalid.
        if (nullptr == buffer) { return false; }
        if (0U == maxSize) { return false; }

        // If no simulated data is available, return false immediately (non-blocking).
        if (0U == myBufLen) { return false; }

        // Calculate number of bytes to copy (the smallest of maxSize and myBufLen).
        const std::size_t bytesToCopy{maxSize > myBufLen ? myBufLen : maxSize};

        // Copy simulated input data into the given buffer.
        for (std::size_t i{}; i < bytesToCopy; ++i)
        {
            buffer[i] = myBuf[i];
        }

        // Add terminating null character. Add it last if we have maxed the buffer.
        // Else add it to the end of the string.
        if (maxSize == bytesToCopy) { buffer[bytesToCopy - 1U] = '\0'; }
        else { buffer[bytesToCopy] = '\0'; }

        // Clear the internal buffer length so we don't read the same simulated line again.
        myBufLen = 0U;

        // Return true since a line was successfully processed.
        return true;
    }

    /**
     * @brief Simulate input data.
     * 
     * @param[in] data Simulated input data.
     */
    void simulateInputData(const char* data) noexcept
    {
        if (nullptr == data) { return; }
        
        std::uint8_t i{};

        // Copy data into our own buffer until we reach a terminating null character.
        // The buffer length is set accordingly.
        for (i = 0U; data[i] != '\0'; ++i)
        {
            myBuf[i] = data[i];
        }
        myBufLen = i;
        myBuf[myBufLen] = '\0';
    }

    Stub(const Stub&)            = delete;
    Stub(Stub&&)                 = delete;
    Stub& operator=(const Stub&) = delete;
    Stub& operator=(Stub&&)      = delete;

private:
    static constexpr std::size_t BufLen{100U};

    /** Internal buffer to simulate input data. */
    char myBuf[BufLen];

    /** Number of bytes in the buffer. */
    std::uint8_t myBufLen;
};
} // namespace driver::serial
