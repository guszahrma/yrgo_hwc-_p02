/**
 * @brief GPIO example.
 */
#include <cstdint>

#include "driver/gpio/stub.h"


int main()
{
    driver::gpio::Stub stub{9U, driver::gpio::OUTPUT};
}
