# Board code, built into every app for the Daisy Seed (see board.c).
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_BOOTLOADER_MCUBOOT board.c)
