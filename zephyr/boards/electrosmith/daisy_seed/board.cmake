# Electrosmith Daisy Seed – flash runners.
#
# dfu-util (default): the STM32's ROM bootloader over the micro-USB port. Hold BOOT, press and release
# RESET, then release BOOT. It can only write the internal flash. No `leave` (--dfuse-modifiers empty):
# the board stays in the ROM bootloader, and RESET starts the program; with `leave` every flash ends with
# an error (see AGENTS.md: dfu-util without leave).
# stm32cubeprogrammer: an ST-Link on the SWD header, if one is connected (`west flash -r stm32cubeprogrammer`).

board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse" "--dfuse-modifiers=")
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
