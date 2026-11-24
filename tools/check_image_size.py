#!/usr/bin/env python3
"""Fails the build if P02's signed image does not fit where MCUboot needs it.

Run by sysbuild after the app's image is signed (zephyr/app/sysbuild.cmake); can also be run by hand on a
sysbuild build directory, in the Zephyr environment:
    tools/check_image_size.py zephyr/app/build/nucleo_g0b1re

Header + code must fit in:
* slot 0 minus ROM_END_OFFSET (MCUboot's trailer and room for the image's TLVs, plus one sector for swap
  using move), where P02 runs or is stored;
* slot 1 minus ROM_END_OFFSET, where an update is written; for swap using offset one sector further in,
  since MCUboot's sysbuild code leaves that sector out of ROM_END_OFFSET (a #todo in
  mcuboot/boot/zephyr/CMakeLists.txt);
* for RAM load, the RAM MCUboot copies the image into.

The linker checks the first one on Cortex-M, but not on the ESP32-S3, and none of them checks slot 1.

For swap using offset, slot 1 must also have the same sector size and exactly one sector more than slot 0,
the layout in zephyr/app/partitions/<board>.overlay.
"""

import argparse
import pickle
import re
import struct
import sys
from pathlib import Path

IMAGE_MAGIC = 0x96F3B83D  # MCUboot's image header magic


def read_kconfig(path: Path) -> dict:
    values = {}
    for line in path.read_text().splitlines():
        match = re.match(r"^(\w+)=(.*)$", line)
        if match:
            values[match.group(1)] = match.group(2).strip('"')
    return values


def read_cmake_cache(path: Path, name: str) -> str:
    for line in path.read_text().splitlines():
        if line.startswith(name + ":"):
            return line.split("=", 1)[1]
    sys.exit(f"{name} not found in {path}")


def load_edt(image_dir: Path):
    zephyr_base = read_cmake_cache(image_dir / "CMakeCache.txt", "ZEPHYR_BASE")
    sys.path.insert(0, str(Path(zephyr_base) / "scripts" / "dts" / "python-devicetree" / "src"))
    with open(image_dir / "zephyr" / "edt.pickle", "rb") as file:
        return pickle.load(file)


def partition(edt, label: str):
    """Size of a partition and the erase-block size (sector) of the flash it is on, or None."""
    node = edt.label2node.get(label)
    if node is None:
        return None
    flash = node.parent.parent  # partition -> partitions -> flash
    sector = flash.props.get("erase-block-size")
    return node.regs[0].size, sector.val if sector else 0


def slots_incompatible_for_offset(slot0, slot1):
    """Why these slots are not our layout for swap using offset, or None.

    Our layout: the same sector size, and slot 1 exactly one sector more than slot 0, so an update can fill
    slot 0. MCUboot also accepts equal slots (boot_slots_compatible() in swap_offset.c), but then slot 0's
    last sector is never used; any other count, and MCUboot refuses every update.
    """
    (size0, sector0), (size1, sector1) = slot0, slot1
    if sector0 != sector1:
        return f"slot0_partition and slot1_partition have different sectors ({sector0} B and {sector1} B)"
    sectors0, sectors1 = size0 // sector0, size1 // sector1
    if sectors1 != sectors0 + 1:
        return (f"slots for swap using offset: slot0_partition {sectors0} sectors, slot1_partition "
                f"{sectors1}; slot 1 must have exactly one sector more than slot 0")
    return None


def kb(size: int) -> str:
    return f"{size / 1024:.1f} KB"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("build", type=Path, help="sysbuild build directory, e.g. zephyr/app/build/nucleo_g0b1re")
    parser.add_argument("--image", default="app", help="the app's image name in sysbuild (default: app)")
    args = parser.parse_args()

    image_dir = args.build / args.image
    sysbuild = read_kconfig(args.build / "zephyr" / ".config")
    config = read_kconfig(image_dir / "zephyr" / ".config")
    mcuboot = read_kconfig(args.build / "mcuboot" / "zephyr" / ".config")
    edt = load_edt(image_dir)

    signed = image_dir / "zephyr" / "zephyr.signed.bin"
    header = signed.read_bytes()[:16]
    magic, _load_addr, hdr_size, _protect, img_size = struct.unpack("<IIHHI", header)
    if magic != IMAGE_MAGIC:
        print(f"error: {signed} is not an MCUboot image", file=sys.stderr)
        return 1
    used = hdr_size + img_size
    end = int(config.get("CONFIG_ROM_END_OFFSET", "0"), 0)

    limits = []  # (where, maximum header + code)
    slot0 = partition(edt, "slot0_partition")
    if slot0 is None:
        print("error: no slot0_partition in the app's devicetree", file=sys.stderr)
        return 1
    limits.append(("slot0_partition", slot0[0] - end))
    slot1 = partition(edt, "slot1_partition")
    if slot1 is not None:
        size, sector = slot1
        if sysbuild.get("SB_CONFIG_MCUBOOT_MODE_SWAP_USING_OFFSET") == "y":
            error = slots_incompatible_for_offset(slot0, slot1)
            if error:
                print(f"error: {error}", file=sys.stderr)
                return 1
            limits.append(("slot1_partition, written one sector in", size - sector - end))
        else:
            limits.append(("slot1_partition", size - end))
    if mcuboot.get("CONFIG_BOOT_RAM_LOAD") == "y":
        limits.append(("MCUboot's RAM for the image", int(mcuboot["CONFIG_BOOT_IMAGE_EXECUTABLE_RAM_SIZE"], 0)))

    where, limit = min(limits, key=lambda item: item[1])
    if used > limit:
        print(f"error: P02 does not fit: header + code {used} B ({kb(used)}), but {where} allows "
              f"{limit} B ({kb(limit)}), {used - limit} B too many",
              file=sys.stderr)
        return 1
    print(f"P02 fits: header + code {kb(used)} of {kb(limit)} ({where}), margin {kb(limit - used)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
