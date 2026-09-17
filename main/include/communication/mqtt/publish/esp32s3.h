/**
 * @file the MQTT implementation of the publish interface on esp32s3
 */

#pragma once

#include <cstdint>
#include <string>

#include "communication/mqtt/publish/interface.h"
#include "mqtt_client.h"

namespace comm::mqtt::publish
{
class Esp32s3 final : public Interface
{
public:
    explicit Esp32s3(std::string url, std::string topic, std::string clientId, std::string username,
                     std::string password) noexcept;
    ~Esp32s3() noexcept override;

    bool isConnected() const noexcept override;
    bool publish(const std::string& message, uint8_t qos, bool retain) noexcept override;

    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

private:
    static void eventHandler(void* arg, esp_event_base_t base, long int eventId,
                             void* eventData) noexcept;

    std::string myUrl;
    std::string myTopic;
    std::string myClientId;
    std::string myUsername;
    std::string myPassword;
    esp_mqtt_client_handle_t myClient;
    bool myConnected;
};
} // namespace comm::mqtt::publish
