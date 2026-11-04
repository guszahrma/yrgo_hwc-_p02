# MCUboot runs from internal flash, so it is built for the plain daisy_seed target, with its own
# devicetree overlay and Kconfig fragment from this directory.
if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  set_target_properties(mcuboot PROPERTIES BOARD daisy_seed/stm32h750xx)
  set(mcuboot_EXTRA_DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/mcuboot.overlay
      CACHE INTERNAL "MCUboot devicetree overlay for the Daisy Seed")
  set(mcuboot_EXTRA_CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/mcuboot.conf
      CACHE INTERNAL "MCUboot Kconfig fragment for the Daisy Seed")
endif()
