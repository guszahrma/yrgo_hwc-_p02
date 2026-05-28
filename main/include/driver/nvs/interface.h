#pragma once

#include <cstdint>
#include <string>

namespace driver::nvs
{

class Interface
{
public:
    virtual ~Interface() noexcept = default;

    /**
     * @note Concrete implementations are expected to provide:
     *
     *   template<typename T> bool set(const char* key, const T value, const bool autoCommit = true) noexcept;
     *   template<typename T> bool get(const char* key, T& value) noexcept;
     *
     * Supporting at minimum: uint8_t, uint16_t, uint32_t, uint64_t,
     *                          int8_t,  int16_t,  int32_t,  int64_t.
     *
     * These cannot be declared virtual because C++ does not allow virtual
     * function templates. They are therefore not part of this interface.
     */

    /** @brief Store a string value.
     *  @param [in] key         Key to store under.
     *  @param [in] value       Value to store.
     *  @param [in] autoCommit  If true, commit to flash immediately after writing.
     *  @return True if successful, false otherwise.
     */
    virtual bool setString(const char* key, const std::string& value, const bool autoCommit = true) noexcept = 0;

    /** @brief Retrieve a string value.
     *  @param [in]  key    Key to look up.
     *  @param [out] value  Retrieved value.
     *  @return True if the key existed and value was written, false otherwise.
     */
    virtual bool getString(const char* key, std::string& value) noexcept = 0;

    /** @brief Commit pending writes to flash.
     *  @return True if successful, false otherwise.
     */
    virtual bool commit() noexcept = 0;

    /** @brief Erase a single key from the namespace.
     *  @param [in] key  Key to erase.
     *  @return True if successful, false otherwise.
     */
    virtual bool eraseKey(const char* key) noexcept = 0;

    /** @brief Erase all keys in the namespace.
     *  @return True if successful, false otherwise.
     */
    virtual bool eraseAll() noexcept = 0;
};

} // namespace driver::nvs
