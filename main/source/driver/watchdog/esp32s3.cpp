/**
 * @file Esp32s3 watchdog driver implementation
 */
#include <cstdio>

#include "driver/watchdog/esp32s3.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

namespace driver::watchdog
{
// --------------------------------------------------------------------------------
Esp32s3::Esp32s3(std::uint32_t timeoutMs) noexcept
    : myTimeoutMs{timeoutMs}
    , myCausedLastReset{esp_reset_reason() == ESP_RST_TASK_WDT}
    , mySubscribed{false}
{
    if (0U == myTimeoutMs)
    {
        std::printf("Can't create watchdog timer with no timeout\n");
        return;
    }

    esp_task_wdt_config_t config{};
    config.timeout_ms = myTimeoutMs;
    config.idle_core_mask =
        (1U << 0) | (1U << 1); // Watch the idle tasks on both cores, CPU0 & CPU1
    config.trigger_panic = true;

    // ESP-idf usually already started the TWDT
    esp_err_t err{esp_task_wdt_reconfigure(&config)};

    // If it wasn't started, initialize it
    if (ESP_ERR_INVALID_STATE == err) { err = esp_task_wdt_init(&config); }

    if (ESP_OK != err)
    {
        std::printf("watchdog: configuration failed: %s\n", esp_err_to_name(err));
        return;
    }

    err = esp_task_wdt_add(nullptr);

    if (ESP_OK != err)
    {
        std::printf("watchdog: failed to watch the task: %s\n", esp_err_to_name(err));
        return;
    }

    mySubscribed = true;
    std::printf("ESP32s3 watchdog started, timeout %u ms\n", static_cast<unsigned>(myTimeoutMs));
}

// --------------------------------------------------------------------------------
Esp32s3::~Esp32s3() noexcept
{
    if (mySubscribed) { esp_task_wdt_delete(nullptr); }
}

// --------------------------------------------------------------------------------
void Esp32s3::kick() noexcept
{
    if (!mySubscribed) { return; }
    const esp_err_t err{esp_task_wdt_reset()};
    if (ESP_OK != err) { std::printf("watchdog: kick failed: %s\n", esp_err_to_name(err)); }
}

// --------------------------------------------------------------------------------
bool Esp32s3::causedLastReset() const noexcept { return myCausedLastReset; }
} // namespace driver::watchdog
