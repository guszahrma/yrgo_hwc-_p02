#pragma once
#include <cstdint>


namespace driver::eeprom {

/** @brief Interface for EEPROM drivers
 */
class Interface
{
public:
    virtual ~Interface() noexcept = default;

    /** @brief Write data to EEPROM
     *  @param [in] address The address to write to
     *  @param [in] data The data to write
     *  @param [in] length The length of the data to write
     *  @return true if the write was successful, false otherwise
     */
    virtual bool write(const uint16_t address, const uint8_t* data, const std::size_t length) noexcept = 0;

    /** @brief Read data from EEPROM
     *  @param [in] address The address to read from
     *  @param [out] buffer The buffer to read into
     *  @param [in] length The length of the data that can be stored in the buffer
     *  @return The number of bytes read
     */
    virtual uint16_t read(const uint16_t address, uint8_t* buffer, const std::size_t length) noexcept = 0;
};

} // namespace driver::eeprom