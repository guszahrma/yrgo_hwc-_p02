# Blinky for the Arduino Nano ESP32.
#
# Build (from this directory, with the Zephyr environment active):
#   west build -p always -b arduino_nano_esp32/esp32s3/procpu .

cmake_minimum_required(VERSION 3.20.0)

# Our own board definition lives in this repository (zephyr/boards/arduino/nano_esp32), not in
# Zephyr. BOARD_ROOT tells Zephyr where to look; it must point to the directory that CONTAINS
# boards/, and must be set BEFORE find_package(Zephyr), which is where the board is looked up.
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nano_blinky)

target_sources(app PRIVATE src/main.c)
