#!/usr/bin/env python3
"""Hardware test: sends commands to P02 on a board over USB serial and checks the replies.

Run after flashing, e.g. by the CI's self-hosted runner:
    python tools/hil/p02_test.py --port /dev/tty_arduino_nano_esp32
    python3 tools/hil/p02_test.py --port /dev/tty_nucleo_g0b1re --board nucleo_g0b1re --platform Zephyr

Needs pyserial (included in ESP-IDF's Python environment; python3-serial on Ubuntu). Exits with 1 if
any test fails.
"""

import argparse
import os
import sys
import time

import serial


def wait_for_port(port: str, timeout_s: float) -> None:
    """After a reset the board re-enumerates on USB and usbipd re-attaches it; wait for the port."""
    deadline = time.monotonic() + timeout_s
    while not os.path.exists(port):
        if time.monotonic() > deadline:
            sys.exit(f"Port {port} did not appear within {timeout_s:.0f} s")
        time.sleep(0.5)


def open_port(port: str) -> serial.Serial:
    # DTR/RTS low before opening: on the ESP32-S3's USB-Serial/JTAG they drive reset and boot mode.
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = 115200
    ser.timeout = 0.1
    ser.dtr = False
    ser.rts = False
    for _ in range(20):
        try:
            ser.open()
            return ser
        except serial.SerialException:
            time.sleep(0.5)  # The port exists but udev/usbipd is not done with it yet
    sys.exit(f"Could not open {port}")


def read_until(ser: serial.Serial, expected: str, timeout_s: float) -> str:
    """Reads until `expected` appears or the time runs out; returns everything read."""
    received = ""
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        received += ser.read(256).decode("utf-8", errors="replace")
        if expected in received:
            break
    return received


def command(ser: serial.Serial, cmd: str, expected: str, timeout_s: float = 2.0) -> str:
    ser.reset_input_buffer()
    ser.write(f"{cmd}\r\n".encode())
    return read_until(ser, expected, timeout_s)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--port", default="/dev/tty_arduino_nano_esp32")
    # What 'status' must show: CONFIG_BOARD on Zephyr, arduino_nano_esp32 on ESP-IDF
    parser.add_argument("--board", default="arduino_nano_esp32")
    parser.add_argument("--platform", default="ESP-IDF", choices=["ESP-IDF", "Zephyr"])
    parser.add_argument("--boot-timeout", type=float, default=30.0)
    args = parser.parse_args()

    wait_for_port(args.port, args.boot_timeout)
    ser = open_port(args.port)

    # The boot output may be gone before the port was open, so wait for P02 to answer instead:
    # 'help' until it replies. A line break first ends any junk left in P02's input buffer.
    ser.write(b"\r\n")
    deadline = time.monotonic() + args.boot_timeout
    while "Available commands:" not in command(ser, "help", "Available commands:"):
        if time.monotonic() > deadline:
            print("FAIL: P02 does not answer 'help'")
            return 1

    # (name, command, text the reply must contain)
    tests = [
        ("help lists the commands", "help", "period <ms>"),
        ("status shows the board", "status", f"Board: {args.board}"),
        ("status shows the platform", "status", f"Platform: {args.platform}"),
        ("period accepts 500 ms", "period 500", "Handling 'period' command...500 ms"),
        ("status shows the new period", "status", "Blink Period: 500 ms"),
        ("period rejects 5 ms", "period 5", "Invalid period"),
        ("period rejects text", "period abc", "Usage: period <ms>"),
        ("blink on", "blink on", "Handling 'blink on' command..."),
        ("status shows blinking", "status", "Blink State: on"),
        ("blink off", "blink off", "Handling 'blink off' command..."),
        ("unknown command", "foo", "Handling unknown command: foo"),
    ]

    failures = 0
    for name, cmd, expected in tests:
        reply = command(ser, cmd, expected)
        if expected in reply:
            print(f"PASS: {name}")
        else:
            failures += 1
            print(f"FAIL: {name}\n  sent:     {cmd!r}\n  expected: {expected!r}\n  got:      {reply!r}")

    ser.close()
    print(f"\n{len(tests) - failures}/{len(tests)} passed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
