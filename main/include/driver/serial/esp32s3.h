/**
 * @brief ESP32-S3 Serial implementation
 */
#pragma once

//! @note Sort headers.
#include <cstdint>
#include <cstddef>

#include "driver/serial/interface.h"

namespace driver::serial
{
/**
 * @brief Hardware class for ESP32-S3 Serial interface.
 */
class Esp32s3 final : public Interface
{
public:
    /**
     * @brief Construct a new Esp32s3 object
     * 
     * @param[in] baudRate Baud rate for communication.
     */
    //! @note How about making baudRate a std::uint32_t? I guess you're using int to be compatible
    //!       with the ESP32-S3 driver, but I think it's better to keep it unsigned and then
    //!       cast it to int if needed. Then I can't pass something like -1, which makes no sense.
    //!       But as long as you check the baud rate in the constructor, I think this is fine.
    explicit Esp32s3(int baudRate) noexcept;

    /**
     * @brief Destroy the Esp32s3 object
     */
    ~Esp32s3() noexcept override;

    /**
     * @brief Print function
     * 
     * @param[in] text Pointer to the C-string to be printed.
     */
    void print(const char* text) noexcept override;
    
    /**
     * @brief Non-blocking ReadLine function.
     * 
     * @param[out] buffer Pointer to the buffer where the completed string will be stored.
     * @param[in] maxSize Maximum number of bytes to read into the buffer.
     * 
     * @return true If a full line (terminated by \n or \r) was successfully buffered and copied else return false.
     */
    bool readLine(char* buffer, std::size_t maxSize) noexcept override;

    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

private:
    static constexpr std::size_t InternalBufLen{256U};
    
    /** Internal buffer to accumulate incoming characters. */
    char myInternalBuf[InternalBufLen];
    
    /** Current number of bytes stored in the internal buffer. */
    std::size_t myInternalLen;
};
} // namespace driver::serial