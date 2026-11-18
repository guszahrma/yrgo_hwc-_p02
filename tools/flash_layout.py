#!/usr/bin/env python3
"""Prints a board's memory layout and how much room P02 has in its slot, as Markdown tables.

Reads a sysbuild build directory (MCUboot and P02 built together with --sysbuild), so the numbers are the
ones the build used: partitions from the app's devicetree, the MCUboot mode from sysbuild's Kconfig, the
header and end reservation from the app's Kconfig, and the signed image's header.

Run in the Zephyr environment (source tools/zephyr_env.sh), after a build:
    tools/flash_layout.py zephyr/app/build/nucleo_g0b1re

The tables go into the board documents and the P02 board documents (see docs/boards.md: Memory layout).
"""

import argparse
import pickle
import re
import struct
import sys
from pathlib import Path

IMAGE_MAGIC = 0x96F3B83D  # MCUboot's image header magic

MODES = {
    "SB_CONFIG_MCUBOOT_MODE_SINGLE_APP": "single app",
    "SB_CONFIG_MCUBOOT_MODE_SWAP_USING_OFFSET": "swap using offset",
    "SB_CONFIG_MCUBOOT_MODE_SWAP_USING_MOVE": "swap using move",
    "SB_CONFIG_MCUBOOT_MODE_SWAP_SCRATCH": "swap using scratch",
    "SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY": "overwrite only",
    "SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP": "direct XIP",
    "SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT": "direct XIP with revert",
    "SB_CONFIG_MCUBOOT_MODE_RAM_LOAD": "RAM load",
    "SB_CONFIG_MCUBOOT_MODE_RAM_LOAD_WITH_REVERT": "RAM load with revert",
    "SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER": "firmware updater",
    "SB_CONFIG_MCUBOOT_MODE_SINGLE_APP_RAM_LOAD": "single app RAM load",
}


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


def size_text(size: int) -> str:
    """Bytes as MB, KB or B, whichever divides exactly; otherwise KB with one decimal."""
    if size % (1024 * 1024) == 0:
        return f"{size // (1024 * 1024)} MB"
    if size % 1024 == 0:
        return f"{size // 1024} KB"
    if size < 1024:
        return f"{size} B"
    return f"{size / 1024:.1f} KB"


def load_edt(app_dir: Path):
    zephyr_base = read_cmake_cache(app_dir / "CMakeCache.txt", "ZEPHYR_BASE")
    sys.path.insert(0, str(Path(zephyr_base) / "scripts" / "dts" / "python-devicetree" / "src"))
    with open(app_dir / "zephyr" / "edt.pickle", "rb") as file:
        return pickle.load(file)


def flash_of(partitions_node):
    """The flash device a fixed-partitions node belongs to."""
    return partitions_node.parent


def erase_block_size(flash) -> int | None:
    prop = flash.props.get("erase-block-size")
    return prop.val if prop else None


def memory_table(edt) -> tuple[list[str], dict]:
    """One row per partition, with unused ranges in between, grouped by flash device."""
    lines = [
        "| Memory | Size | Sector | Partition | Offset | Size |",
        "|---|---|---|---|---|---|",
    ]
    partitions = {}
    for node in edt.compat2nodes.get("fixed-partitions", []):
        flash = flash_of(node)
        flash_size = flash.regs[0].size if flash.regs else None
        sector = erase_block_size(flash)
        children = sorted(node.children.values(), key=lambda child: child.regs[0].addr)
        first = True
        position = 0
        rows = []
        for child in children:
            offset, size = child.regs[0].addr, child.regs[0].size
            if offset > position:
                rows.append(("*unused*", position, offset - position))
            name = f"`{child.labels[0]}`" if child.labels else f"`{child.name}`"
            rows.append((name, offset, size))
            position = offset + size
            if child.labels:
                partitions[child.labels[0]] = (flash, offset, size)
        if flash_size is not None and position < flash_size:
            rows.append(("*unused*", position, flash_size - position))
        for name, offset, size in rows:
            if first:
                flash_name = f"`{flash.labels[0]}`" if flash.labels else f"`{flash.name}`"
                memory = flash_name
                total = size_text(flash_size) if flash_size else "?"
                sector_text = size_text(sector) if sector else "?"
                first = False
            else:
                memory = total = sector_text = ""
            lines.append(f"| {memory} | {total} | {sector_text} | {name} | 0x{offset:X} | {size_text(size)} |")
    return lines, partitions


def image_size(signed_bin: Path) -> tuple[int, int]:
    """Header size and header + code size from an MCUboot image header."""
    data = signed_bin.read_bytes()[:16]
    magic, _load_addr, hdr_size, _protect, img_size = struct.unpack("<IIHHI", data)
    if magic != IMAGE_MAGIC:
        sys.exit(f"{signed_bin} is not an MCUboot image")
    return hdr_size, hdr_size + img_size


def p02_table(build: Path, partitions: dict) -> list[str]:
    app = build / "app"
    sysbuild = read_kconfig(build / "zephyr" / ".config")
    config = read_kconfig(app / "zephyr" / ".config")
    mcuboot = read_kconfig(build / "mcuboot" / "zephyr" / ".config")

    mode = next((text for key, text in MODES.items() if sysbuild.get(key) == "y"), "unknown")
    if "slot0_partition" not in partitions:
        sys.exit("No slot0_partition in the app's devicetree")
    flash, _offset, slot_size = partitions["slot0_partition"]
    sector = erase_block_size(flash) or 0
    start = int(config.get("CONFIG_ROM_START_OFFSET", "0"), 0)
    end = int(config.get("CONFIG_ROM_END_OFFSET", "0"), 0)

    # The space for header + code: the slot minus ROM_END_OFFSET, which holds MCUboot's trailer, room for
    # the image's TLVs and, for swap using move, one sector.
    limit = slot_size - end
    end_text = f"0x{end:X} ({size_text(end)})"
    if mode == "swap using offset":
        # MCUboot's sysbuild code leaves this sector out of ROM_END_OFFSET (a #todo in
        # mcuboot/boot/zephyr/CMakeLists.txt), but an update needs it in slot 1.
        limit -= sector
        end_text += f" + one sector, {size_text(sector)} (not in `ROM_END_OFFSET`)"

    lines = [
        "| P02 in `slot0_partition` | Value |",
        "|---|---|",
        f"| Board target | `{config.get('CONFIG_BOARD_TARGET', '?')}` |",
        f"| MCUboot mode | {mode} |",
        f"| Slot size | {size_text(slot_size)} (0x{slot_size:X}) |",
        f"| Header (`ROM_START_OFFSET`) | 0x{start:X} ({size_text(start)}) |",
        f"| End of the slot reserved for MCUboot (`ROM_END_OFFSET`) | {end_text} |",
        f"| Maximum header + code | {size_text(limit)} (0x{limit:X}) |",
    ]

    signed = app / "zephyr" / "zephyr.signed.bin"
    if signed.exists():
        _hdr, used = image_size(signed)
        lines.append(f"| P02 today: header + code | {size_text(used)} (0x{used:X}) |")
        lines.append(f"| Margin in the slot | {size_text(limit - used)} |")

    if mcuboot.get("CONFIG_BOOT_RAM_LOAD") == "y":
        # RAM load: MCUboot copies header + code into this RAM, and P02's data and stacks are in it too,
        # so the RAM, not the slot, is the limit.
        ram_start = int(mcuboot["CONFIG_BOOT_IMAGE_EXECUTABLE_RAM_START"], 0)
        ram_size = int(mcuboot["CONFIG_BOOT_IMAGE_EXECUTABLE_RAM_SIZE"], 0)
        lines.append(f"| RAM MCUboot loads P02 into | {size_text(ram_size)} at 0x{ram_start:08X} |")
        used = ram_used(app / "zephyr" / "zephyr.elf", ram_start, ram_size)
        lines.append(f"| P02 today in RAM: code, data, stacks | {size_text(used)} |")
        lines.append(f"| RAM margin | {size_text(ram_size - used)} |")
    return lines


def ram_used(elf_path: Path, ram_start: int, ram_size: int) -> int:
    """Bytes from the start of the RAM to the end of the last allocated section in it."""
    from elftools.elf.constants import SH_FLAGS
    from elftools.elf.elffile import ELFFile

    end = ram_start
    with open(elf_path, "rb") as file:
        for section in ELFFile(file).iter_sections():
            address, size = section["sh_addr"], section["sh_size"]
            if section["sh_flags"] & SH_FLAGS.SHF_ALLOC and size and ram_start <= address < ram_start + ram_size:
                end = max(end, address + size)
    return end - ram_start


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("build", type=Path, help="sysbuild build directory, e.g. zephyr/app/build/nucleo_g0b1re")
    args = parser.parse_args()

    edt = load_edt(args.build / "app")
    memory, partitions = memory_table(edt)
    print("\n".join(memory))
    print()
    print("\n".join(p02_table(args.build, partitions)))


if __name__ == "__main__":
    main()
