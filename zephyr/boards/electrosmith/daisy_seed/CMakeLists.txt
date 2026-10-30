# Board code, built into the images for the Daisy Seed that need it.
zephyr_library()
# Apps started by MCUboot: reset the peripherals MCUboot used.
zephyr_library_sources_ifdef(CONFIG_BOOTLOADER_MCUBOOT board.c)
# MCUboot itself: RESET enters serial recovery (see mcuboot.conf).
zephyr_library_sources_ifdef(CONFIG_BOOT_GO_HOOKS mcuboot_hooks.c)
