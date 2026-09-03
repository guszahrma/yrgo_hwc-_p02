//! @note File header missing.
#include "logic/logic.h"

//! @note This is no longer an example. Maybe you can write something like "Application entry point".
/**
 * @brief GPIO, timer, serial and ADC example.
 */
extern "C" void app_main()
{
    //! @note Initialize explicitly with {} (but in your defense, the default constructor will be 
    //!       called anyway).
    logic::logic::Logic logic;
    logic.run();
}
