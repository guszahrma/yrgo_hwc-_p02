# P02 on Zephyr – the Arduino Nano ESP32.

cmake_minimum_required(VERSION 3.20.0)

# Our board definition lives in this repository (zephyr/boards/arduino/nano_esp32).
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# The firmware update module, a git submodule: Zephyr only finds west projects by itself.
set(FW_UPDATE_MODULE ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/fw_update)
if(NOT EXISTS ${FW_UPDATE_MODULE}/zephyr/module.yml)
  message(FATAL_ERROR "modules/fw_update is empty: run 'git submodule update --init modules/fw_update'")
endif()
list(APPEND ZEPHYR_EXTRA_MODULES ${FW_UPDATE_MODULE})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(p02_zephyr)

# P02's shared code: interfaces, stubs and drivers live in main/ (shared with the ESP-IDF build).
set(P02_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/../../main)
target_include_directories(app PRIVATE ${P02_MAIN}/include)

target_sources(app PRIVATE
    src/main.cpp
    ${P02_MAIN}/source/driver/timer/zephyr.cpp
    ${P02_MAIN}/source/driver/gpio/zephyr.cpp
    ${P02_MAIN}/source/driver/adc/zephyr.cpp
    ${P02_MAIN}/source/driver/serial/zephyr.cpp
    ${P02_MAIN}/source/driver/tempsensor/tmp36.cpp
    ${P02_MAIN}/source/driver/factory/zephyr.cpp
    ${P02_MAIN}/source/driver/nvs/zephyr.cpp
    ${P02_MAIN}/source/driver/watchdog/zephyr.cpp
    ${P02_MAIN}/source/logic/logic.cpp
)