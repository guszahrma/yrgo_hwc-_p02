/**
 * @file Zephyr serial driver
 */
#pragma once

#include <cstddef>
#include <zephyr/device.h>

#include "driver/serial/interface.h"

namespace driver::serial
{
/**
 * @brief Serial driver for Zephyr, using UART polling (uart_poll_in/uart_poll_out).
 *
 * Used with the console device (zephyr,console = USB-C on the Nano ESP32), but works with any
 * UART device. print() turns "\n" into "\r\n" so terminals start each line at the left edge.
 */
class Zephyr final : public Interface
{
public:
    /**
     * @brief Construct a new Zephyr object
     *
     * @param[in] uart UART device to use, e.g. DEVICE_DT_GET(DT_CHOSEN(zephyr_console))
     */
    explicit Zephyr(const device* uart) noexcept;

    /**
     * @brief Destroy the Zephyr object
     */
    ~Zephyr() noexcept override = default;

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
     * @return true If a full line (terminated by \n or \r) was successfully buffered and copied
     * else return false.
     */
    bool readLine(char* buffer, std::size_t maxSize) noexcept override;

    Zephyr(const Zephyr&)            = delete;
    Zephyr(Zephyr&&)                 = delete;
    Zephyr& operator=(const Zephyr&) = delete;
    Zephyr& operator=(Zephyr&&)      = delete;

private:
    /** UART device (from devicetree). */
    const device* const myUart;

    /** True if the device was ready in the constructor. */
    bool myReady;

    static constexpr std::size_t InternalBufLen{256U};

    /** Internal buffer to accumulate incoming characters. */
    char myInternalBuf[InternalBufLen];

    /** Current number of bytes stored in the internal buffer. */
    std::size_t myInternalLen;
};
} // namespace driver::serial