#include <cstdio>
#include "communication/mqtt/publish/esp32s3.h"

namespace communication::mqtt::publish
{

Esp32s3::Esp32s3(std::string url, std::string topic, std::string clientId,
                 std::string username, std::string password) noexcept
: myUrl(std::move(url))
, myTopic(std::move(topic))
, myClientId(std::move(clientId))
, myUsername(std::move(username))
, myPassword(std::move(password))
{
    esp_mqtt_client_config_t config = {};
    config.broker.address.uri                  = myUrl.c_str();
    config.credentials.client_id               = myClientId.c_str();
    if (!myUsername.empty())
        config.credentials.username            = myUsername.c_str();
    if (!myPassword.empty())
        config.credentials.authentication.password = myPassword.c_str();
    myClient = esp_mqtt_client_init(&config);
    esp_mqtt_client_register_event(myClient, MQTT_EVENT_ANY, eventHandler, this);
    esp_mqtt_client_start(myClient);
}

// --------------------------------------------------------------------------------
Esp32s3::~Esp32s3() noexcept
{
    esp_mqtt_client_destroy(myClient);
}

// --------------------------------------------------------------------------------
bool Esp32s3::isConnected() const noexcept
{
    return myConnected;
}

// --------------------------------------------------------------------------------
bool Esp32s3::publish(const std::string& value, uint8_t qos, bool retain) noexcept
{
    int msgId = esp_mqtt_client_publish(
        myClient, myTopic.c_str(), value.c_str(), 0, qos, retain ? 1 : 0);
    return msgId >= 0;
}

// --------------------------------------------------------------------------------
void Esp32s3::eventHandler(void* arg, esp_event_base_t, long int eventId, void*) noexcept
{
    auto* self = static_cast<Esp32s3*>(arg);
    switch (eventId)
    {
        case MQTT_EVENT_CONNECTED:
            self->myConnected = true;
            printf("mqtt::publish: connected\n");
            break;
        case MQTT_EVENT_DISCONNECTED:
            self->myConnected = false;
            printf("mqtt::publish: disconnected\n");
            break;
        case MQTT_EVENT_ERROR:
            printf("mqtt::publish: error\n");
            break;
        default: break;
    }
}

} // namespace communication::mqtt::publish
