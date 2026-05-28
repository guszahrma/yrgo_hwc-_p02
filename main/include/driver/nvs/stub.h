#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <type_traits>
#include <vector>
#include "driver/nvs/interface.h"

namespace driver::nvs
{

/**
 * @brief Stub class for the NVS interface. Used for testing and debugging.
 *        Stores all key-value pairs in RAM instead of flash.
 *        Behaviour mirrors driver::nvs::Esp32s3 without any hardware dependency, but it does not persist data across resets.
 */
class Stub final : public Interface
{
public:
    /** @brief Construct a stub NVS namespace.
     *  @param [in] namespaceName  Name used only for identification in log output.
     */
    explicit Stub(const char* namespaceName) noexcept
    : myNamespace{namespaceName}
    {
        std::printf("nvs::Stub '%s' constructed.\n", namespaceName);
    }

    /** @brief Destroy the stub and release RAM storage. */
    ~Stub() noexcept override = default;

    Stub(const Stub&)            = delete;
    Stub(Stub&&)                 = delete;
    Stub& operator=(const Stub&) = delete;
    Stub& operator=(Stub&&)      = delete;

    /** @brief Store a string value in RAM.
     *  @param [in] key         Key to store under (max 15 characters).
     *  @param [in] value       Value to store.
     *  @param [in] autoCommit  Accepted for interface compatibility; always a no-op.
     *  @return True if successful, false if key is invalid.
     */
    bool setString(const char* key, const std::string& value, const bool autoCommit = true) noexcept override
    {
        if (!isValidKey(key)) return false;
        myStrings[key] = value;
        std::printf("nvs::Stub '%s': setString('%s', '%s')\n", myNamespace, key, value.c_str());
        return autoCommit ? commit() : true;
    }

    /** @brief Retrieve a string value from RAM.
     *  @param [in]  key    Key to look up.
     *  @param [out] value  Retrieved value, unchanged if key does not exist.
     *  @return True if the key existed and value was written, false otherwise.
     */
    bool getString(const char* key, std::string& value) noexcept override
    {
        if (!isValidKey(key)) return false;
        const auto it = myStrings.find(key);
        if (myStrings.end() == it)
        {
            std::printf("nvs::Stub '%s': getString('%s') not found\n", myNamespace, key);
            return false;
        }
        value = it->second;
        std::printf("nvs::Stub '%s': getString('%s') = '%s'\n", myNamespace, key, value.c_str());
        return true;
    }

    /** @brief Store an integer value in RAM.
     *  @tparam     T           One of: uint8_t, uint16_t, uint32_t, uint64_t,
     *                                   int8_t,  int16_t,  int32_t,  int64_t.
     *  @param [in] key         Key to store under (max 15 characters).
     *  @param [in] value       Value to store.
     *  @param [in] autoCommit  Accepted for interface compatibility; always a no-op.
     *  @return True if successful, false if key is invalid.
     */
    template<typename T>
    bool set(const char* key, const T value, const bool autoCommit = true) noexcept
    {
        static_assert(std::is_integral_v<T>, "nvs::Stub::set only supports integral types");
        if (!isValidKey(key)) return false;
        std::vector<uint8_t> bytes(sizeof(T));
        std::memcpy(bytes.data(), &value, sizeof(T));
        myBlobs[key] = std::move(bytes);
        std::printf("nvs::Stub '%s': set('%s')\n", myNamespace, key);
        return autoCommit ? commit() : true;
    }

    /** @brief Retrieve an integer value from RAM.
     *  @tparam      T      One of: uint8_t, uint16_t, uint32_t, uint64_t,
     *                               int8_t,  int16_t,  int32_t,  int64_t.
     *  @param [in]  key    Key to look up.
     *  @param [out] value  Retrieved value, unchanged if key does not exist.
     *  @return True if the key existed and value was written, false otherwise.
     */
    template<typename T>
    bool get(const char* key, T& value) noexcept
    {
        static_assert(std::is_integral_v<T>, "nvs::Stub::get only supports integral types");
        if (!isValidKey(key)) return false;
        const auto it = myBlobs.find(key);
        if (myBlobs.end() == it || sizeof(T) != it->second.size())
        {
            std::printf("nvs::Stub '%s': get('%s') not found\n", myNamespace, key);
            return false;
        }
        std::memcpy(&value, it->second.data(), sizeof(T));
        std::printf("nvs::Stub '%s': get('%s')\n", myNamespace, key);
        return true;
    }

    /** @brief No-op commit — data is already in RAM.
     *  @return Always true.
     */
    bool commit() noexcept override
    {
        std::printf("nvs::Stub '%s': commit (no-op)\n", myNamespace);
        return true;
    }

    /** @brief Erase a single key from RAM storage.
     *  @param [in] key  Key to erase (max 15 characters).
     *  @return True if successful, false if key is invalid.
     */
    bool eraseKey(const char* key) noexcept override
    {
        if (!isValidKey(key)) return false;
        myStrings.erase(key);
        myBlobs.erase(key);
        std::printf("nvs::Stub '%s': eraseKey('%s')\n", myNamespace, key);
        return true;
    }

    /** @brief Erase all keys from RAM storage.
     *  @return Always true.
     */
    bool eraseAll() noexcept override
    {
        myStrings.clear();
        myBlobs.clear();
        std::printf("nvs::Stub '%s': eraseAll()\n", myNamespace);
        return true;
    }

private:
    static bool isValidKey(const char* key) noexcept
    {
        constexpr std::size_t maxKeyLength = 15;
        if (nullptr == key || maxKeyLength < std::strlen(key))
        {
            std::printf("nvs: key is null or exceeds %zu characters\n", maxKeyLength);
            return false;
        }
        return true;
    }

    const char*                             myNamespace;
    std::map<std::string, std::string>      myStrings;
    std::map<std::string, std::vector<uint8_t>> myBlobs;
};

} // namespace driver::nvs
