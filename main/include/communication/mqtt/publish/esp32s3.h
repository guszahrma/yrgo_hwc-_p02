//! @note Documentation missing.
#pragma once

#include <cstdint>
#include <string>

//! @note Sort local headers.
#include "mqtt_client.h"
#include "communication/mqtt/publish/interface.h"

namespace communication::mqtt::publish
{
class Esp32s3 final : public Interface
{
public:
    //! @note I would mark the constructor explicit, even though explicit is only necessary
    //!       when implementing constructors that takes one input argument.
    //!
    //! @note Try not to pass string by value (copy), prefer by reference.
    //!       However, you need it for the default parameters, but then again, you may
    //!       just use const char* and be done with it.
    //!
    //! @note I saw that you move the string content in the constructor, then it's fine.
    //!       I usually pass as const char*, then copy the content. You pass a copy here,
    //!       which then is moved to your member variables => about the same.
    Esp32s3(std::string url, std::string topic, std::string clientId,
            std::string username = {}, std::string password = {}) noexcept;
    ~Esp32s3() noexcept override;

    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

    //! @note Place these above the deleted methods.
    bool isConnected() const noexcept override;
    bool publish(const std::string& message, uint8_t qos = 0, bool retain = false) noexcept override;

private:
    static void eventHandler(void* arg, esp_event_base_t base, long int eventId, void* eventData) noexcept;

    std::string myUrl;
    std::string myTopic;
    std::string myClientId;
    std::string myUsername;
    std::string myPassword;

    //! @note Initialize in the construction, along with the strings.
    esp_mqtt_client_handle_t myClient = nullptr;
    bool myConnected = false;
};
} // namespace communication::mqtt::publish
