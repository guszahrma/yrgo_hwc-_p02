#pragma once

#include <string>
#include "mqtt_client.h"
#include "communication/mqtt/subscribe/interface.h"

namespace communication::mqtt::subscribe
{
class Esp32s3 final : public Interface
{
public:
    /**
     *  @brief Construct a new Esp32s3 object for subscribing to MQTT topics.
     *  @param url The MQTT broker URL.
     *  @param topic The topic to subscribe to.
     *  @param clientId The client ID for the MQTT connection.
     *  @param username The username for the MQTT connection.
     *  @param password The password for the MQTT connection.
     */
    Esp32s3(std::string url, std::string topic, std::string clientId,
            std::string username = {}, std::string password = {});
    ~Esp32s3() noexcept override;


    /**
     *  @brief Get the value of the subscribed MQTT topic.
     *  @param value Reference to store the retrieved value.
     *  @return true if successful, false otherwise.
     */
    bool getValue(std::string& value) noexcept override;

    /**
    *  @brief Check if the MQTT client is connected.
    *  @return true if connected, false otherwise.
    */
    bool isConnected() const noexcept override;

    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

private:
    static void eventHandler(void* arg, esp_event_base_t base, long int eventId, void* eventData);

    std::string myUrl;
    std::string myTopic;
    std::string myClientId;
    std::string myUsername;
    std::string myPassword;
    std::string myLastValue;
    esp_mqtt_client_handle_t myClient = nullptr;
    bool myConnected = false;
};
} // namespace communication::mqtt::subscribe
