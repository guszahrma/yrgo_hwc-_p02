/**
 * @brief Unit tests for driver::gpio::Esp32s3 against the ESP-IDF GPIO mock.
 */
#include "driver/gpio/esp32s3.h"
#include "gtest/gtest.h"

TEST(GpioEsp32s3, WriteThenReadHigh)
{
    driver::gpio::Esp32s3 led{9U, driver::gpio::Direction::OUTPUT};
    led.write(true);
    EXPECT_TRUE(led.read());
}

TEST(GpioEsp32s3, WriteThenReadLow)
{
    driver::gpio::Esp32s3 led{9U, driver::gpio::Direction::OUTPUT};
    led.write(true);
    led.write(false);
    EXPECT_FALSE(led.read());
}

TEST(GpioEsp32s3, PinsAreIndependent)
{
    driver::gpio::Esp32s3 led{9U, driver::gpio::Direction::OUTPUT};
    driver::gpio::Esp32s3 other{10U, driver::gpio::Direction::OUTPUT};
    led.write(true);
    other.write(false);
    EXPECT_TRUE(led.read());
    EXPECT_FALSE(other.read());
}