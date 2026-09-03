//! @note File header missing.
#pragma once

#include <cstdint>
#include <string>

//! @note How about a shorter namespace and folder name, such as 'comm'?
namespace communication::mqtt::publish
{
class Interface
{
public:
    virtual ~Interface() noexcept = default;

    /**
     * @brief Publish a message to the configured topic.
     * @param [in] message  The message to publish.
     * @param [in] qos      Quality of service level (0, 1 or 2).
     * @param [in] retain   Whether the broker should retain the message.
     * @return True if the message was queued successfully, false otherwise.
     */
    //! @note Prefer to use std::uint8_t from <cstdint> instead of indirectly included <uint8_t> from <stdint.h>.
    //!       Usually, default parameters are not encourages for virtual methods, since the code tends
    //!       to become quite messy (what about default parameters in the overridden method, what if 
    //!       they're different).
    virtual bool publish(const std::string& message, uint8_t qos = 0, bool retain = false) noexcept = 0;

    /**
     * @brief Check if the MQTT client is connected.
     * @return True if connected, false otherwise.
     */
    virtual bool isConnected() const noexcept = 0;

};
} // namespace communication::mqtt::publish
