/**
 * @file Mocked hardware platform.
 */
#pragma once

#ifdef TRACE_ENABLED
#define TRACE(...) do { printf("%s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); } while (0)
#else
#define TRACE(...)
#endif


/** Alias for ESP32 types. */
typedef int esp_err_t;

// Här kan ni lägga globala definitioner, makron och så vidare, eller
// inkludera mockheaders.
#include "gpio/gpio_mock.h"