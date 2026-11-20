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
