#include <cstdint>
#include "driver/eeprom/esp32s3.h"

namespace driver::eeprom {

driver::eeprom::Esp32s3::~Esp32s3() noexcept
{
    delete[] myStorage;
}

bool driver::eeprom::Esp32s3::write(const uint16_t address, const uint8_t* data, const std::size_t length) noexcept
{
    return false;
}

uint16_t driver::eeprom::Esp32s3::read(const uint16_t address, uint8_t* buffer, const std::size_t length) noexcept
{
    return 0;
}

} // namespace driver::eeprom