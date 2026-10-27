/**
 * @file P02 on Zephyr: application entry point.
 */

#include "driver/factory/zephyr.h"
#include "logic/logic.h"

int main()
{
    // The Zephyr build uses the Zephyr drivers (NVS is still a stub).
    driver::factory::Zephyr factory{};
    logic::Logic logic{factory};
    logic.run();
    return 0;
}