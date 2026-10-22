# Blinky for the Arduino Nano ESP32.
#
# Build (from this directory, with the Zephyr environment active):
#   west build -p always -b esp32s3_devkitc/esp32s3/procpu .

cmake_minimum_required(VERSION 3.20.0)

# Board overlays that adapt esp32s3_devkitc to the Nano ESP32 (console on USB-C, LEDs).
# Must be set BEFORE find_package(Zephyr), because that is where the devicetree is generated.
# EXTRA_DTC_OVERLAY_FILE adds to (rather than replaces) any automatically found overlays.
# Paths are relative to this file, so the build works wherever the repository is cloned.
set(NANO_BOARD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../boards/nano_esp32)
set(EXTRA_DTC_OVERLAY_FILE
    ${NANO_BOARD_DIR}/usb_console.overlay
    ${NANO_BOARD_DIR}/leds.overlay
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nano_blinky)

target_sources(app PRIVATE src/main.c)
