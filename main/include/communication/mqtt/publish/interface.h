/**
 * @file the generic MQTT publish interface
 */
#pragma once

#include <cstdint>
#include <string>

namespace comm::mqtt::publish
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
    virtual bool publish(const std::string& message, std::uint8_t qos, bool retain) noexcept = 0;

    /**
     * @brief Check if the MQTT client is connected.
     * @return True if connected, false otherwise.
     */
    virtual bool isConnected() const noexcept = 0;
};
} // namespace comm::mqtt::publish
