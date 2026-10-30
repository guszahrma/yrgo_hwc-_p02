# Electrosmith Daisy Seed – flash runners.
#
# dfu-util (default): the STM32's ROM bootloader over the micro-USB port. Hold BOOT, press and release
# RESET, then release BOOT. It can only write the internal flash. `leave` (the default) starts the
# program after flashing; the board resets without answering, so the flash ends with the harmless
# "Error during download get_status". (`leave:will-reset` breaks the download itself: LIBUSB_ERROR_PIPE.)
# stm32cubeprogrammer: an ST-Link on the SWD header, if one is connected (`west flash -r stm32cubeprogrammer`).

board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse")
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
