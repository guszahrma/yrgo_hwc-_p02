/**
 * @file the MQTT implementation of the subscribe interface on esp32s3
 */
#include <cstdio>
#include <utility>

#include "communication/mqtt/subscribe/esp32s3.h"

namespace comm::mqtt::subscribe
{

// --------------------------------------------------------------------------------
Esp32s3::Esp32s3(std::string url, std::string topic, std::string clientId, std::string username,
                 std::string password) noexcept
    : myUrl(std::move(url))
    , myTopic(std::move(topic))
    , myClientId(std::move(clientId))
    , myUsername(std::move(username))
    , myPassword(std::move(password))
    , myClient(nullptr)
    , myConnected(false)
{
    //! @note Same comments here as for the publisher; the logic looks good though.
    esp_mqtt_client_config_t config = {};
    config.broker.address.uri       = myUrl.c_str();
    config.credentials.client_id    = myClientId.c_str();
    if (!myUsername.empty()) { config.credentials.username = myUsername.c_str(); }
    if (!myPassword.empty()) { config.credentials.authentication.password = myPassword.c_str(); }
    myClient = esp_mqtt_client_init(&config);
    esp_mqtt_client_register_event(myClient, MQTT_EVENT_ANY, eventHandler, this);
    esp_mqtt_client_start(myClient);
}

// --------------------------------------------------------------------------------
Esp32s3::~Esp32s3() noexcept { esp_mqtt_client_destroy(myClient); }

// --------------------------------------------------------------------------------
bool Esp32s3::isConnected() const noexcept { return myConnected; }

// --------------------------------------------------------------------------------
bool Esp32s3::getValue(std::string& value) noexcept
{
    if (myLastValue.empty()) { return false; }
    value = myLastValue;
    return true;
}

// --------------------------------------------------------------------------------
void Esp32s3::eventHandler(void* arg, esp_event_base_t, long int eventId, void* eventData) noexcept
{
    auto* self  = static_cast<Esp32s3*>(arg);
    auto* event = static_cast<esp_mqtt_event_handle_t>(eventData);

    // verify that the input is valid.
    if (nullptr == self || nullptr == event) { return; }

    switch (eventId)
    {
        case MQTT_EVENT_CONNECTED:
            self->myConnected = true;
            std::printf("mqtt::subscribe: connected\n");
            esp_mqtt_client_subscribe(self->myClient, self->myTopic.c_str(), 0);
            break;
        case MQTT_EVENT_DATA:
            self->myLastValue.assign(event->data, event->data_len);
            std::printf("mqtt::subscribe: value on %.*s: %.*s\n", event->topic_len, event->topic,
                        event->data_len, event->data);
            break;
        case MQTT_EVENT_DISCONNECTED:
            self->myConnected = false;
            std::printf("mqtt::subscribe: disconnected\n");
            break;
        case MQTT_EVENT_ERROR:
            std::printf("mqtt::subscribe: error\n");
            break;
        default:
            break;
    }
}
} // namespace comm::mqtt::subscribe
