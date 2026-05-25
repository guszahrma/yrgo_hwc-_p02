#pragma once
#include <cstdint>
#include "driver/eeprom/interface.h"
#include "driver/gpio/stub.h"

namespace driver::eeprom {

    constexpr uint16_t default_size{1024}; // 1KB EEPROM


/** @brief Stub EEPROM driver
 *  This is a singelton class which makes use of the internal flash memory to emulate an EEPROM. 
 *  It provides basic read and write operations.
 *  Note: This is a very basic implementation and does not handle wear leveling or other complexities of real EEPROMs.
 */
class Stub : public Interface
{
public:
    Stub(const uint16_t size = default_size) noexcept
    : mySize(size)
    {
        myStorage = new uint8_t[mySize]();
    }
    ;
    ~Stub();

    /** @brief Write data to EEPROM
     *  @param [in] address The address to write to
     *  @param [in] data The data to write
     *  @param [in] length The length of the data to write
     *  @return true if the write was successful, false otherwise
     */
    bool write(const uint16_t address, const uint8_t* data, const std::size_t length) noexcept override{ return false;}

    /** @brief Read data from EEPROM
     *  @param [in] address The address to read from
     *  @param [out] buffer The buffer to read into
     *  @param [in] length The length of the data that can be stored in the buffer
     *  @return The number of bytes read
     */
    uint16_t read(const uint16_t address, uint8_t* buffer, const std::size_t length) noexcept override {return 0;}
private:
    uint8_t* myStorage;
    uint16_t mySize;
};

} // namespace driver::eeprom