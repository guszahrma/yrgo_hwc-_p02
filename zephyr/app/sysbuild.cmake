# Gives MCUboot and P02 the same partitions for swap using offset, with slot 1 one sector larger than
# slot 0: zephyr/app/partitions/<board>.overlay. Only for boards with such a file; with another MCUboot
# mode, the board's own layout is used.
if(SB_CONFIG_BOOTLOADER_MCUBOOT AND SB_CONFIG_MCUBOOT_MODE_SWAP_USING_OFFSET)
  set(p02_partitions ${CMAKE_CURRENT_LIST_DIR}/partitions/${BOARD}.overlay)
  if(EXISTS ${p02_partitions})
    foreach(image mcuboot ${DEFAULT_IMAGE})
      if(NOT p02_partitions IN_LIST ${image}_EXTRA_DTC_OVERLAY_FILE)
        list(APPEND ${image}_EXTRA_DTC_OVERLAY_FILE ${p02_partitions})
        set(${image}_EXTRA_DTC_OVERLAY_FILE ${${image}_EXTRA_DTC_OVERLAY_FILE}
            CACHE INTERNAL "Devicetree overlays for ${image}" FORCE)
      endif()
    endforeach()
  endif()
endif()

# Checks after every build that P02's signed image fits in its slots (tools/check_image_size.py): the
# linker only checks slot 0, and not at all on the ESP32-S3.
if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  add_custom_target(p02_image_size_check ALL
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/../../tools/check_image_size.py
            ${CMAKE_BINARY_DIR} --image ${DEFAULT_IMAGE}
    COMMENT "Checking that P02's image fits in its slots"
    VERBATIM)
  add_dependencies(p02_image_size_check ${DEFAULT_IMAGE})
endif()
