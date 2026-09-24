/**
 * @file main.cpp
 * @brief Application entry point.
 */

#include "logic/logic.h"

extern "C" void app_main()
{
    logic::Logic logic{};
    logic.run();
}
