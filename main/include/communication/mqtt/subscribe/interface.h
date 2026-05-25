#pragma once

#include <string>

namespace communication::mqtt::subscribe
{
class Interface
{
public:
    virtual ~Interface() noexcept = default;

    /**
     * @brief Get the latest value received on the configured topic.
     * @param [out] value  String to store the received value.
     * @return True if a value was available, false if none received yet.
     */
    virtual bool getValue(std::string& value) noexcept = 0;

    /**
     * @brief Check if the MQTT client is connected.
     * @return True if connected, false otherwise.
     */
    virtual bool isConnected() const noexcept = 0;
};
} // namespace communication::mqtt::subscribe
