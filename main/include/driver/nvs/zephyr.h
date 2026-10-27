/**
 * @file the Zephyr specific nvs interface
 */
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>

#include <zephyr/settings/settings.h>

#include "driver/nvs/interface.h"

namespace driver::nvs
{

/** @brief Zephyr NVS driver.
 *
 *  Implements driver::nvs::Interface with Zephyr's Settings subsystem, which stores its data in
 *  the board's `storage_partition` (NVS backend, enabled in prj.conf).
 *
 *  Keys are stored as "<namespace>/<key>", e.g. "user_settings/blink_state", so the namespace
 *  works like an ESP-IDF NVS namespace.
 *
 *  Every write goes to flash immediately. autoCommit is accepted for compatibility with the
 *  interface, but has no effect, and commit() does nothing.
 *
 *  Supported types for set<T> / get<T>: all integral types, e.g.
 *  uint8_t, uint16_t, uint32_t, uint64_t,
 *  int8_t,  int16_t,  int32_t,  int64_t.
 *  A value must be read back with the same type it was stored with.
 *
 *  @note Only one instance per namespace should exist at a time.
 */
class Zephyr final : public Interface
{
public:
    /** @brief Initialise the Settings subsystem and use the given namespace.
     *  @param [in] namespaceName  Namespace, used as the first part of every key.
     */
    explicit Zephyr(const char* namespaceName) noexcept;

    /** @brief Nothing to release: Settings keeps no handle per namespace. */
    ~Zephyr() noexcept override = default;

    /** @brief Store a string value.
     *  @param [in] key         Key to store under (max 15 characters).
     *  @param [in] value       Value to store.
     *  @param [in] autoCommit  No effect: the value is always written to flash immediately.
     *  @return True if successful, false otherwise.
     */
    bool setString(const char* key, const std::string& value,
                   const bool autoCommit = true) noexcept override;

    /** @brief Retrieve a string value.
     *  @param [in]  key    Key to look up.
     *  @param [out] value  Retrieved value, unchanged if key does not exist.
     *  @return True if the key existed and value was written, false otherwise.
     */
    bool getString(const char* key, std::string& value) noexcept override;

    /** @brief Store an integer value.
     *  @tparam     T           An integral type, e.g. uint8_t ... uint64_t, int8_t ... int64_t.
     *  @param [in] key         Key to store under (max 15 characters).
     *  @param [in] value       Value to store, saved as its sizeof(T) raw bytes.
     *  @param [in] autoCommit  No effect: the value is always written to flash immediately.
     *  @return True if successful, false otherwise.
     */
    template<typename T>
    bool set(const char* key, const T value, const bool autoCommit = true) noexcept
    {
        static_assert(std::is_integral_v<T>, "nvs::Zephyr::set only supports integral types");
        (void)autoCommit; // settings_save_one() always writes to flash immediately

        if ((!myValid) || (!isValidKey(key))) { return false; }

        const int err{settings_save_one(fullName(key).c_str(), &value, sizeof(T))};
        if (0 != err)
        {
            printf("nvs: set('%s') failed: %d\n", key, err);
            return false;
        }
        return true;
    }

    /** @brief Retrieve an integer value.
     *  @tparam      T      The same integral type the value was stored with.
     *  @param [in]  key    Key to look up.
     *  @param [out] value  Retrieved value. Only valid if true is returned.
     *  @return True if exactly sizeof(T) bytes were read, false if the key does not exist, was
     *          stored with another type, or the read failed.
     */
    template<typename T>
    bool get(const char* key, T& value) noexcept
    {
        static_assert(std::is_integral_v<T>, "nvs::Zephyr::get only supports integral types");

        if ((!myValid) || (!isValidKey(key))) { return false; }

        // Returns the number of bytes read, or a negative error code.
        const ssize_t readBytes{settings_load_one(fullName(key).c_str(), &value, sizeof(T))};

        if (static_cast<ssize_t>(sizeof(T)) != readBytes)
        {
            printf("nvs: get('%s') failed: %d\n", key, static_cast<int>(readBytes));
            return false;
        }
        return true;
    }

    /** @brief Does nothing: every write already goes to flash immediately.
     *  @return True if the driver is initialised, false otherwise.
     */
    bool commit() noexcept override;

    /** @brief Erase a single key from the namespace.
     *  @param [in] key  Key to erase (max 15 characters).
     *  @return True if successful, false otherwise.
     */
    bool eraseKey(const char* key) noexcept override;

    /** @brief Erase all keys in the namespace.
     *  @return True if successful, false otherwise.
     */
    bool eraseAll() noexcept override;

    Zephyr(const Zephyr&)            = delete;
    Zephyr(Zephyr&&)                 = delete;
    Zephyr& operator=(const Zephyr&) = delete;
    Zephyr& operator=(Zephyr&&)      = delete;

private:
    /** @brief Check that the key is not null and at most 15 characters (the same limit as on
     *         ESP-IDF, so both platforms accept the same keys).
     */
    static bool isValidKey(const char* key) noexcept;

    /** @brief Build the Settings name for a key: "<namespace>/<key>". */
    std::string fullName(const char* key) const noexcept;

    /** True if the Settings subsystem was initialised. */
    bool myValid;

    /** The namespace, copied so it stays valid however the caller's string is stored. */
    std::string myNamespace;
};
} // namespace driver::nvs
