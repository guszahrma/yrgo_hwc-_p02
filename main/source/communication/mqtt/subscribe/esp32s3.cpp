//! @note File header missing.
#include <cstdio>

//! @note Include <utility> to use std::move().

#include "communication/mqtt/subscribe/esp32s3.h"

namespace communication::mqtt::subscribe
{

    // --------------------------------------------------------------------------------
Esp32s3::Esp32s3(std::string url, std::string topic, std::string clientId,
                 std::string username, std::string password) noexcept
    : myUrl(std::move(url))
    , myTopic(std::move(topic))
    , myClientId(std::move(clientId))
    , myUsername(std::move(username))
    , myPassword(std::move(password))
{
    //! @note Same comments here as for the publisher; the logic looks good though.
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
bool Esp32s3::getValue(std::string& value) noexcept
{
    //! @note Prefer to place brackets around 'return false;' for safety.
    if (myLastValue.empty())
        return false;
    value = myLastValue;
    return true;
}

// --------------------------------------------------------------------------------
void Esp32s3::eventHandler(void* arg, esp_event_base_t, long int eventId, void* eventData) noexcept
{
    auto* self  = static_cast<Esp32s3*>(arg);
    auto* event = static_cast<esp_mqtt_event_handle_t>(eventData);

    //! @note Please add nullptr checks for self and event; they might become nullptr if an invalid
    //!       type is passed. This is highly unlikely if this handler is called from the ESP32s3 driver.

    switch (eventId)
    {
        case MQTT_EVENT_CONNECTED:
            self->myConnected = true;
            printf("mqtt::subscribe: connected\n");
            esp_mqtt_client_subscribe(self->myClient, self->myTopic.c_str(), 0);
            break;
        case MQTT_EVENT_DATA:
            self->myLastValue.assign(event->data, event->data_len);
            printf("mqtt::subscribe: value on %.*s: %.*s\n",
                event->topic_len, event->topic,
                event->data_len,  event->data);
            break;
        case MQTT_EVENT_DISCONNECTED:
            self->myConnected = false;
            printf("mqtt::subscribe: disconnected\n");
            break;
        //! @note Weird syntax here; please place printf and break on separate lines.
        //!       Please also invoke std::printf() from <cstdio> instead of printf()
        //!       from <stdio.h> (but most likely it's the same function, but please maintain
        //!       C++ style code).
        case MQTT_EVENT_ERROR:        printf("mqtt::subscribe: error\n");        break;
        default: break;
    }
}
} // namespace communication::mqtt::subscribe
