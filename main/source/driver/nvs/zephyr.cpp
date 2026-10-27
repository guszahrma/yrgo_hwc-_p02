/**
 * @file the Zephyr specific nvs implementation
 */
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

#include "driver/nvs/zephyr.h"

namespace driver::nvs
{

bool Zephyr::isValidKey(const char* key) noexcept
{
    constexpr std::size_t maxKeyLength{15};
    if ((nullptr == key) || (maxKeyLength < std::strlen(key)))
    {
        printf("nvs: key is null or exceeds %zu characters\n", maxKeyLength);
        return false;
    }
    return true;
}

std::string Zephyr::fullName(const char* key) const noexcept { return myNamespace + "/" + key; }

Zephyr::Zephyr(const char* namespaceName) noexcept
    : myValid{false}
    , myNamespace{namespaceName}
{
    const int err{settings_subsys_init()};
    if (0 != err)
    {
        printf("nvs: settings_subsys_init() failed: %d\n", err);
        return;
    }
    myValid = true;
}

bool Zephyr::setString(const char* key, const std::string& value, const bool autoCommit) noexcept
{
    (void)autoCommit; // settings_save_one() always writes to flash immediately

    if ((!myValid) || (!isValidKey(key))) { return false; }

    const int err{settings_save_one(fullName(key).c_str(), value.data(), value.size())};
    if (0 != err)
    {
        printf("nvs: setString('%s') failed: %d\n", key, err);
        return false;
    }
    return true;
}

bool Zephyr::getString(const char* key, std::string& value) noexcept
{
    if ((!myValid) || (!isValidKey(key))) { return false; }

    const std::string name{fullName(key)};

    const ssize_t len{settings_get_val_len(name.c_str())};
    if (0 >= len) { return false; }

    std::string buf(static_cast<std::size_t>(len), '\0');
    const ssize_t readBytes{settings_load_one(name.c_str(), buf.data(), buf.size())};
    if (len != readBytes)
    {
        printf("nvs: getString('%s') failed: %d\n", key, static_cast<int>(readBytes));
        return false;
    }

    value = std::move(buf);
    return true;
}

bool Zephyr::eraseKey(const char* key) noexcept
{
    if ((!myValid) || (!isValidKey(key))) { return false; }

    const int err{settings_delete(fullName(key).c_str())};
    if (0 != err)
    {
        printf("nvs: eraseKey('%s') failed: %d\n", key, err);
        return false;
    }
    return true;
}

bool Zephyr::eraseAll() noexcept
{
    if (!myValid) { return false; }

    std::vector<std::string> keys{};
    const int err{settings_load_subtree_direct(
        myNamespace.c_str(),
        [](const char* key, size_t, settings_read_cb, void*, void* param) -> int
        {
            static_cast<std::vector<std::string>*>(param)->push_back(key);
            return 0;
        },
        &keys)};

    if (0 != err)
    {
        printf("nvs: eraseAll() failed to list keys: %d\n", err);
        return false;
    }

    bool ok{true};
    for (const auto& key : keys)
    {
        if (!eraseKey(key.c_str())) { ok = false; }
    }
    return ok;
}

bool Zephyr::commit() noexcept { return myValid; }
} // namespace driver::nvs
