# Board code for MCUboot itself: RESET enters serial recovery, USB is on only there (see mcuboot.conf).
# Apps need no board code: an empty zephyr_library() would only add a CMake warning to every build.
if(CONFIG_BOOT_GO_HOOKS)
  zephyr_library()
  zephyr_library_sources(mcuboot_hooks.c)
endif()
