/**
 * @brief ESP32-S3 Serial implementation
 */
#pragma once

#include <cstddef>
#include <cstdint>

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
     */
    Esp32s3() noexcept;

    /**
     * @brief Destroy the Esp32s3 object
     */
    ~Esp32s3() noexcept override;

    /**
     * @brief Print a C-string, of any length.
     *
     * Sent in chunks of WriteChunkLen bytes, each waiting up to WriteTimeoutMs for room in the
     * driver's TX buffer. If the host does not read the port, the rest of the text is dropped, so
     * that the caller (and the watchdog kicks in Logic's loop) is not held up for long.
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
     * @return true If a full line (terminated by \n or \r) was successfully buffered and copied
     * else return false.
     */
    bool readLine(char* buffer, std::size_t maxSize) noexcept override;

    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

private:
    /** Size of the driver's TX and RX buffers, and of the line buffer below. */
    static constexpr std::size_t InternalBufLen{256U};

    /**
     * Bytes per usb_serial_jtag_write_bytes() call. The driver takes a write whole or not at all,
     * so one larger than the TX buffer would never be sent.
     */
    static constexpr std::size_t WriteChunkLen{64U};
    static_assert(WriteChunkLen <= InternalBufLen, "A chunk must fit in the driver's TX buffer");

    /** How long one chunk may wait for room in the TX buffer: well below the watchdog timeout. */
    static constexpr std::uint32_t WriteTimeoutMs{20U};

    /** Internal buffer to accumulate incoming characters. */
    char myInternalBuf[InternalBufLen];

    /** Current number of bytes stored in the internal buffer. */
    std::size_t myInternalLen;
};
} // namespace driver::serial