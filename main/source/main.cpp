#include "logic/logic.h"

/**
 * @brief GPIO, timer, serial and ADC example.
 */
extern "C" void app_main()
{
    logic::logic::Logic logic;
    logic.run();
} 