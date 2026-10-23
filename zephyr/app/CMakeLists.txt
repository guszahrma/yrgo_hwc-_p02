# P02 on Zephyr – the Arduino Nano ESP32.

cmake_minimum_required(VERSION 3.20.0)

# Our board definition lives in this repository (zephyr/boards/arduino/nano_esp32).
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

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
)