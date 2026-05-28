#pragma once

#include <cstdint>
#include <string>
#include "mqtt_client.h"
#include "communication/mqtt/publish/interface.h"

namespace communication::mqtt::publish
{
class Esp32s3 final : public Interface
{
public:
    Esp32s3(std::string url, std::string topic, std::string clientId,
            std::string username = {}, std::string password = {}) noexcept;
    ~Esp32s3() noexcept override;

    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

    bool isConnected() const noexcept override;
    bool publish(const std::string& message, uint8_t qos = 0, bool retain = false) noexcept override;

private:
    static void eventHandler(void* arg, esp_event_base_t base, long int eventId, void* eventData) noexcept;

    std::string myUrl;
    std::string myTopic;
    std::string myClientId;
    std::string myUsername;
    std::string myPassword;
    esp_mqtt_client_handle_t myClient = nullptr;
    bool myConnected = false;
};
} // namespace communication::mqtt::publish
