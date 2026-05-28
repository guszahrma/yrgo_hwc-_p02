#pragma once

#include <cstdio>
#include <cstdint>
#include <type_traits>
#include "nvs.h"
#include "esp_err.h"
#include "driver/nvs/interface.h"

namespace driver::nvs
{

/** @brief ESP32-S3 NVS driver.
 *
 *  Wraps the ESP-IDF NVS API behind the driver::nvs::Interface.
 *  Each instance owns one NVS namespace handle opened in read/write mode.
 *  Changes are not persisted to flash until commit() is called.
 *
 *  Supported types for set<T> / get<T>:
 *  uint8_t, uint16_t, uint32_t, uint64_t,
 *  int8_t,  int16_t,  int32_t,  int64_t.
 *
 *  @note nvs_flash_init() must be called before constructing this object.
 *        This is handled automatically by communication::wifi::Esp32s3::init().
 *  @note Only one instance per namespace should exist at a time.
 */
class Esp32s3 final : public Interface
{
public:
    /** @brief Open an NVS namespace.
     *  @param [in] namespaceName  NVS namespace to open (max 15 characters).
     */
    explicit Esp32s3(const char* namespaceName) noexcept;

    /** @brief Close the NVS namespace handle. */
    ~Esp32s3() noexcept override;

    Esp32s3(const Esp32s3&)            = delete;
    Esp32s3(Esp32s3&&)                 = delete;
    Esp32s3& operator=(const Esp32s3&) = delete;
    Esp32s3& operator=(Esp32s3&&)      = delete;

    /** @brief Store a string value.
     *  @param [in] key         Key to store under (max 15 characters).
     *  @param [in] value       Value to store.
     *  @param [in] autoCommit  If true, commit to flash immediately after writing, else caller must call commit() to persist changes.
     *  @return True if successful, false otherwise.
     */
    bool setString(const char* key, const std::string& value, const bool autoCommit = true) noexcept override;

    /** @brief Retrieve a string value.
     *  @param [in]  key    Key to look up.
     *  @param [out] value  Retrieved value, unchanged if key does not exist.
     *  @return True if the key existed and value was written, false otherwise.
     */
    bool getString(const char* key, std::string& value) noexcept override;

    /** @brief Store an integer value.
     *  @tparam     T           One of: uint8_t, uint16_t, uint32_t, uint64_t,
     *                                   int8_t,  int16_t,  int32_t,  int64_t.
     *  @param [in] key         Key to store under (max 15 characters).
     *  @param [in] value       Value to store.
     *  @param [in] autoCommit  If true, commit to flash immediately after writing, else caller must call commit() to persist changes.
     *  @return True if successful, false otherwise.
     */
    template<typename T>
    bool set(const char* key, const T value, const bool autoCommit = true) noexcept
    {
        if (!myValid || !isValidKey(key)) return false;
        esp_err_t err;
        if constexpr      (std::is_same_v<T, uint8_t>)  err = nvs_set_u8 (myHandle, key, value);
        else if constexpr (std::is_same_v<T, uint16_t>) err = nvs_set_u16(myHandle, key, value);
        else if constexpr (std::is_same_v<T, uint32_t>) err = nvs_set_u32(myHandle, key, value);
        else if constexpr (std::is_same_v<T, uint64_t>) err = nvs_set_u64(myHandle, key, value);
        else if constexpr (std::is_same_v<T, int8_t>)   err = nvs_set_i8 (myHandle, key, value);
        else if constexpr (std::is_same_v<T, int16_t>)  err = nvs_set_i16(myHandle, key, value);
        else if constexpr (std::is_same_v<T, int32_t>)  err = nvs_set_i32(myHandle, key, value);
        else if constexpr (std::is_same_v<T, int64_t>)  err = nvs_set_i64(myHandle, key, value);
        else static_assert(false, "Unsupported type for NVS set");
        if (ESP_OK != err)
        {
            printf("nvs: set('%s') failed: %s\n", key, esp_err_to_name(err));
            return false;
        }
        return autoCommit ? commit() : true;
    }

    /** @brief Retrieve an integer value.
     *  @tparam      T      One of: uint8_t, uint16_t, uint32_t, uint64_t,
     *                               int8_t,  int16_t,  int32_t,  int64_t.
     *  @param [in]  key    Key to look up.
     *  @param [out] value  Retrieved value, unchanged if key does not exist.
     *  @return True if the key existed and value was written, false otherwise.
     */
    template<typename T>
    bool get(const char* key, T& value) noexcept
    {
        if (!myValid || !isValidKey(key)) return false;
        esp_err_t err;
        if constexpr      (std::is_same_v<T, uint8_t>)  err = nvs_get_u8 (myHandle, key, &value);
        else if constexpr (std::is_same_v<T, uint16_t>) err = nvs_get_u16(myHandle, key, &value);
        else if constexpr (std::is_same_v<T, uint32_t>) err = nvs_get_u32(myHandle, key, &value);
        else if constexpr (std::is_same_v<T, uint64_t>) err = nvs_get_u64(myHandle, key, &value);
        else if constexpr (std::is_same_v<T, int8_t>)   err = nvs_get_i8 (myHandle, key, &value);
        else if constexpr (std::is_same_v<T, int16_t>)  err = nvs_get_i16(myHandle, key, &value);
        else if constexpr (std::is_same_v<T, int32_t>)  err = nvs_get_i32(myHandle, key, &value);
        else if constexpr (std::is_same_v<T, int64_t>)  err = nvs_get_i64(myHandle, key, &value);
        else static_assert(false, "Unsupported type for NVS get");
        if (ESP_OK != err)
        {
            printf("nvs: get('%s') failed: %s\n", key, esp_err_to_name(err));
            return false;
        }
        return true;
    }

    /** @brief Commit pending writes to flash.
     *  @return True if successful, false otherwise.
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

private:
    static bool isValidKey(const char* key) noexcept;

    nvs_handle_t myHandle;
    bool         myValid;
};

} // namespace driver::nvs
