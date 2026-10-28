/**
 * @file Blinky for the Arduino Nano ESP32.
 *
 * Toggles the yellow LED and cycles the RGB LED red -> green -> blue once per second.
 * All pin information comes from devicetree: the aliases led0-led3 are defined by the board
 * (zephyr/boards/arduino/nano_esp32/arduino_nano_esp32_procpu.dts), so the code contains no pin
 * numbers.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define SLEEP_TIME_MS 1000

/* Yellow LED: alias led0, defined by the board. */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* RGB LED, in the order the colours are shown: red -> green -> blue. */
static const struct gpio_dt_spec rgb[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios), /* red   */
    GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios), /* green */
    GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios), /* blue  */
};

/* Names for printing, same order as rgb[]. */
static const char* const rgb_names[] = {"red", "green", "blue"};

int main(void)
{
    int ret;
    bool led_state = true;
    size_t colour  = 0;

    /* Yellow LED: check and configure (starts on). */
    if (!gpio_is_ready_dt(&led))
    {
        printk("Error: yellow LED not ready\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (0 > ret)
    {
        printk("Error %d: failed to configure yellow LED\n", ret);
        return 0;
    }

    /* RGB LEDs: check and configure each one (all start off). */
    for (size_t i = 0; i < ARRAY_SIZE(rgb); i++)
    {
        if (!gpio_is_ready_dt(&rgb[i]))
        {
            printk("Error: %s LED not ready\n", rgb_names[i]);
            return 0;
        }

        ret = gpio_pin_configure_dt(&rgb[i], GPIO_OUTPUT_INACTIVE);
        if (0 > ret)
        {
            printk("Error %d: failed to configure %s LED\n", ret, rgb_names[i]);
            return 0;
        }
    }

    /* Turn on the first colour. */
    gpio_pin_set_dt(&rgb[colour], 1);

    while (1)
    {
        k_msleep(SLEEP_TIME_MS);

        /* Yellow LED: toggle. */
        ret = gpio_pin_toggle_dt(&led);
        if (0 > ret)
        {
            printk("Error %d: failed to toggle yellow LED\n", ret);
            return 0;
        }
        led_state = !led_state;

        /* RGB LED: turn off the current colour, step to the next, turn it on. */
        gpio_pin_set_dt(&rgb[colour], 0);
        colour = (colour + 1) % ARRAY_SIZE(rgb);
        gpio_pin_set_dt(&rgb[colour], 1);

        printk("Yellow: %s  RGB: %s\n", led_state ? "ON " : "OFF", rgb_names[colour]);
    }

    return 0;
}