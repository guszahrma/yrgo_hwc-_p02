/**
 * @file Zephyr serial driver
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <zephyr/device.h>
#include <zephyr/sys/ring_buffer.h>

#include "driver/serial/interface.h"

namespace driver::serial
{
/**
 * @brief Serial driver for Zephyr: an interrupt receives into a ring buffer, output by polling
 *        (uart_poll_out).
 *
 * Used with the console device (zephyr,console = USB-C on the Nano ESP32), but works with any
 * UART device. A UART without the interrupt API is read by polling (uart_poll_in) instead.
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
     * @brief Destroy the Zephyr object: stops the receive interrupt, which holds a pointer to it.
     */
    ~Zephyr() noexcept override;

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
    /**
     * @brief Interrupt handler: moves the received bytes into the ring buffer.
     *
     * @param[in] uart The UART device.
     * @param[in] param The Zephyr object (user data).
     */
    static void onInterrupt(const device* uart, void* param) noexcept;

    /**
     * @brief Read one received byte, from the ring buffer or by polling.
     *
     * @param[out] byte The byte read.
     *
     * @return true if a byte was read, false if there was none.
     */
    bool readByte(unsigned char& byte) noexcept;

    /** UART device (from devicetree). */
    const device* const myUart;

    /** True if the device was ready in the constructor. */
    bool myReady;

    /** True if bytes are received by the interrupt, false if read by polling. */
    bool myInterruptDriven;

    static constexpr std::size_t RxBufLen{256U};

    /** Bytes received by the interrupt and not yet read by readLine(). */
    ring_buf myRxRing;

    /** Storage of myRxRing. */
    std::uint8_t myRxStorage[RxBufLen];

    static constexpr std::size_t InternalBufLen{256U};

    /** Internal buffer to accumulate incoming characters. */
    char myInternalBuf[InternalBufLen];

    /** Current number of bytes stored in the internal buffer. */
    std::size_t myInternalLen;

    /** True while a monitor has the port open (DTR set), as seen at the last readLine(). */
    bool myPortOpen;
};
} // namespace driver::serial