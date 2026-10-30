/*
 * Apps started by MCUboot: reset the peripherals MCUboot used, before any driver starts.
 *
 * MCUboot leaves USB running (it only switches off the legacy USB stack) and the QSPI controller
 * configured, and neither Zephyr driver resets its peripheral. Without this, the app's drivers
 * would start on top of MCUboot's state. A precaution: not yet shown to be needed on the board.
 */
#include <soc.h>
#include <zephyr/init.h>

static int daisy_seed_reset_mcuboot_peripherals(void)
{
    RCC->AHB1RSTR |= RCC_AHB1RSTR_USB2OTGHSRST; /* USB OTG FS (usbotg_fs) */
    RCC->AHB3RSTR |= RCC_AHB3RSTR_QSPIRST;
    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_USB2OTGHSRST;
    RCC->AHB3RSTR &= ~RCC_AHB3RSTR_QSPIRST;
    return 0;
}

SYS_INIT(daisy_seed_reset_mcuboot_peripherals, PRE_KERNEL_1, 0);
