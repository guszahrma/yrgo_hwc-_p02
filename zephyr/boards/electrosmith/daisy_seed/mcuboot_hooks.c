/*
 * MCUboot on the Daisy Seed: a RESET press enters serial recovery, every other reset boots the app.
 *
 * MCUboot's own CONFIG_BOOT_SERIAL_PIN_RESET checks only the pin-reset flag, but the STM32H7 sets
 * that flag on every reset (the reset circuit drives the NRST pin), so MCUboot never booted the
 * app. The other flags tell the resets apart: a RESET press sets only PINRSTF (and CPURSTF),
 * power-on also sets PORRSTF/BORRSTF, a software reset (smpmgr os reset) SFTRSTF, a watchdog
 * IWDG1RSTF/WWDG1RSTF.
 *
 * For a RESET press, the hook reports "no bootable image", and MCUboot enters serial recovery
 * (CONFIG_BOOT_SERIAL_NO_APPLICATION), where it waits for smpmgr as long as needed.
 *
 * USB is switched on only in serial recovery (CONFIG_CDC_ACM_SERIAL_ENABLE_AT_BOOT=n). Switched on
 * at boot, the PC saw MCUboot connect and vanish again when the app started, half-way through
 * enumeration, and Windows (usbipd) was slow or failed to find the app's USB serial port.
 */
#include <soc.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/usb/usbd.h>

#include "bootutil/boot_hooks.h"
#include "bootutil/mcuboot_status.h"

#define OTHER_RESETS                                                                               \
    (RCC_RSR_PORRSTF | RCC_RSR_BORRSTF | RCC_RSR_SFTRSTF | RCC_RSR_IWDG1RSTF | RCC_RSR_WWDG1RSTF | \
     RCC_RSR_LPWRRSTF | RCC_RSR_D1RSTF | RCC_RSR_D2RSTF)

fih_ret boot_go_hook(struct boot_rsp* rsp)
{
    (void)rsp;

    const uint32_t flags = RCC->RSR;
    RCC->RSR |= RCC_RSR_RMVF; /* The flags add up until cleared: clear them for the next reset. */

    if ((flags & RCC_RSR_PINRSTF) && !(flags & OTHER_RESETS))
    {
        FIH_RET(FIH_FAILURE); /* RESET pressed: serial recovery. */
    }
    FIH_RET(FIH_BOOT_HOOK_REGULAR); /* Boot the app as usual. */
}

void mcuboot_status_change(mcuboot_status_type_t status)
{
    if (status == MCUBOOT_STATUS_SERIAL_DFU_ENTERED)
    {
        /* The USB device set up by CONFIG_CDC_ACM_SERIAL_INITIALIZE_AT_BOOT, the only one. */
        STRUCT_SECTION_FOREACH(usbd_context, ctx) { (void)usbd_enable(ctx); }
    }
}
