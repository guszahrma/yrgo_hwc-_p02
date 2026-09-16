/**
 * @file Mocked ESP32 GPIO driver.
 */
#ifndef ESP32_GPIO_MOCK_H_
#define ESP32_GPIO_MOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "arch/env/hw_platform.h"

typedef uint8_t gpio_num_t;

typedef enum
{
    GPIO_MODE_INPUT,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_INPUT_OUTPUT,
} gpio_mode_t;

esp_err_t gpio_reset_pin(gpio_num_t gpio_num);

esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode);

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);

int gpio_get_level(gpio_num_t gpio_num);

esp_err_t gpio_pullup_en(gpio_num_t gpio_num);

esp_err_t gpio_pullup_dis(gpio_num_t gpio_num);

esp_err_t gpio_pulldown_en(gpio_num_t gpio_num);

esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num);

#ifdef __cplusplus
}
#endif

#endif /** ESP32_GPIO_MOCK_H_ */