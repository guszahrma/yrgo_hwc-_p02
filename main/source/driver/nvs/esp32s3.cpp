#include <cstdio>
#include <cstring>
#include "nvs_flash.h"
#include "driver/nvs/esp32s3.h"

namespace driver::nvs
{

bool Esp32s3::isValidKey(const char* key) noexcept
{
    constexpr std::size_t maxKeyLength = 15;
    if (nullptr == key || maxKeyLength < strlen(key))
    {
        printf("nvs: key is null or exceeds %zu characters\n", maxKeyLength);
        return false;
    }
    return true;
}

Esp32s3::Esp32s3(const char* namespaceName) noexcept
: myHandle{}
, myValid{false}
{
    esp_err_t err = nvs_flash_init();
    if (ESP_ERR_NVS_NO_FREE_PAGES == err || ESP_ERR_NVS_NEW_VERSION_FOUND == err)
    {
        printf("nvs: partition needs erase, erasing and reinitialising\n");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (ESP_OK != err)
    {
        printf("nvs: flash init failed: %s\n", esp_err_to_name(err));
        return;
    }

    err = nvs_open(namespaceName, NVS_READWRITE, &myHandle);
    if (ESP_OK != err)
    {
        printf("nvs: failed to open namespace '%s': %s\n", namespaceName, esp_err_to_name(err));
        return;
    }
    myValid = true;
}

Esp32s3::~Esp32s3() noexcept
{
    if (myValid)
        nvs_close(myHandle);
}

bool Esp32s3::setString(const char* key, const std::string& value, const bool autoCommit) noexcept
{
    if (!myValid || !isValidKey(key)) return false;
    esp_err_t err = nvs_set_str(myHandle, key, value.c_str());
    if (ESP_OK != err)
    {
        printf("nvs: setString('%s') failed: %s\n", key, esp_err_to_name(err));
        return false;
    }
    return autoCommit ? commit() : true;
}

bool Esp32s3::getString(const char* key, std::string& value) noexcept
{
    if (!myValid || !isValidKey(key)) return false;
    std::size_t required = 0;
    // get required size for the string (including null terminator)
    esp_err_t err = nvs_get_str(myHandle, key, nullptr, &required);
    if (ESP_OK != err)
    {
        printf("nvs: getString('%s') failed: %s\n", key, esp_err_to_name(err));
        return false;
    }
    value.resize(required);
    // now get the string value
    err = nvs_get_str(myHandle, key, value.data(), &required);
    if (ESP_OK != err)
    {
        printf("nvs: getString('%s') read failed: %s\n", key, esp_err_to_name(err));
        return false;
    }
    return true;
}

bool Esp32s3::eraseKey(const char* key) noexcept
{
    if (!myValid || !isValidKey(key)) return false;
    esp_err_t err = nvs_erase_key(myHandle, key);
    if (ESP_OK != err)
    {
        printf("nvs: eraseKey('%s') failed: %s\n", key, esp_err_to_name(err));
        return false;
    }
    return commit();
}

bool Esp32s3::eraseAll() noexcept
{
    if (!myValid) return false;
    esp_err_t err = nvs_erase_all(myHandle);
    if (ESP_OK != err)
    {
        printf("nvs: eraseAll() failed: %s\n", esp_err_to_name(err));
        return false;
    }
    return commit();
}

bool Esp32s3::commit() noexcept
{
    if (!myValid) return false;
    esp_err_t err = nvs_commit(myHandle);
    if (ESP_OK != err)
    {
        printf("nvs: commit failed: %s\n", esp_err_to_name(err));
        return false;
    }
    return true;
}

} // namespace driver::nvs
