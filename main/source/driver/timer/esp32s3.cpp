//! @note File header missing.

//! @note Sort headers.
#include <cstdio>
#include <cstdint>

#include "driver/timer/esp32s3.h"
#include "esp_timer.h"

namespace driver::timer
{
namespace
{
//! @note Can be marked constexpr -> can be computed during compilation.
std::uint64_t microseconds(std::uint32_t periodMs) noexcept
{
    return static_cast<std::uint64_t>(periodMs) * 1000ULL;
}
} // namespace

// --------------------------------------------------------------------------------
Esp32s3::Esp32s3() noexcept
    : myPeriodMs{500}
    , myLastTimeUs{0}
    , myStarted{false}
{
    std::printf("ESP32-S3 timer initialized.\n");
}

// --------------------------------------------------------------------------------
Esp32s3::~Esp32s3() noexcept = default;

// --------------------------------------------------------------------------------
void Esp32s3::start() noexcept
{
    myStarted = true;
    myLastTimeUs = static_cast<std::uint64_t>(esp_timer_get_time());
    std::printf("ESP32-S3 timer started.\n");
}

// --------------------------------------------------------------------------------
void Esp32s3::stop() noexcept
{
    myStarted = false;
    myLastTimeUs = 0;
    std::printf("ESP32-S3 timer stopped.\n");
}

// --------------------------------------------------------------------------------
void Esp32s3::set_period(std::uint32_t period_ms) noexcept
{
    myPeriodMs = period_ms;
    std::printf("ESP32-S3 timer period set to %u ms.\n", static_cast<unsigned>(myPeriodMs));
}

// --------------------------------------------------------------------------------
bool Esp32s3::timeout() noexcept
{
    //! @note Short statements like this can be placed on one line if desired.
    if (!myStarted)
    {
        return false;
    }

    const std::uint64_t currentTimeUs{static_cast<std::uint64_t>(esp_timer_get_time())};
    const std::uint64_t periodUs{microseconds(myPeriodMs)};

    //! @note Yoda, please.
    if ((currentTimeUs - myLastTimeUs) >= periodUs)
    {
        myLastTimeUs = currentTimeUs;
        std::printf("ESP32-S3 timer timeout detected.\n");
        return true;
    }

    return false;
}
} // namespace driver::timer
