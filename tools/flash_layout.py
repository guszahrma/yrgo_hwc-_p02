#!/usr/bin/env python3
"""Prints a board's memory layout and how much room P02 has in its slot, as Markdown tables.

Reads a sysbuild build directory (MCUboot and P02 built together with --sysbuild), so the numbers are the
ones the build used: partitions from the app's devicetree, the MCUboot mode from sysbuild's Kconfig, and
P02's size and limits from check_image_size.measure(), the same the build checks.

Run in the Zephyr environment (source tools/zephyr_env.sh), after a build:
    tools/flash_layout.py zephyr/app/build/nucleo_g0b1re

The tables go into the board documents and the P02 board documents (see docs/boards.md: Memory layout).
"""

import argparse
import re
import sys
from pathlib import Path

from check_image_size import SizeError, load_edt, measure, read_kconfig

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


def size_text(size: int) -> str:
    """Bytes as MB, KB or B, whichever divides exactly; otherwise KB with one decimal."""
    if size % (1024 * 1024) == 0:
        return f"{size // (1024 * 1024)} MB"
    if size % 1024 == 0:
        return f"{size // 1024} KB"
    if size < 1024:
        return f"{size} B"
    return f"{size / 1024:.1f} KB"


def exact_size_text(size: int, with_hex: bool = True) -> str:
    """A size to compare with the build's output: a whole size as size_text(), any other in KB with two
    decimals and in bytes, so that 512 KB - 48 B is not shown as "512.0 KB"."""
    hex_text = f"0x{size:X}" if with_hex else ""
    if size % 1024 == 0 or size < 1024:
        return f"{size_text(size)} ({hex_text})" if with_hex else size_text(size)
    return f"{size / 1024:.2f} KB ({size:,} B" + (f" = {hex_text}" if with_hex else "") + ")"


def flash_of(partitions_node):
    """The flash device a fixed-partitions node belongs to."""
    return partitions_node.parent


def erase_block_size(flash) -> int | None:
    prop = flash.props.get("erase-block-size")
    return prop.val if prop else None


def memory_table(edt) -> list[str]:
    """One row per partition, with unused ranges in between, grouped by flash device."""
    lines = [
        "| Memory | Size | Sector | Partition | Offset | Size |",
        "|---|---|---|---|---|---|",
    ]
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
    return lines


def p02_table(build: Path, edt) -> list[str]:
    """P02 in its slot: the limits are the ones the build checks (check_image_size.measure())."""
    app = build / "app"
    sysbuild = read_kconfig(build / "zephyr" / ".config")
    config = read_kconfig(app / "zephyr" / ".config")
    mode = next((text for key, text in MODES.items() if sysbuild.get(key) == "y"), "unknown")
    try:
        result = measure(build, edt=edt)
    except SizeError as error:
        sys.exit(f"error: {error}")
    slot = edt.label2node["slot0_partition"].regs[0].size
    start = int(config.get("CONFIG_ROM_START_OFFSET", "0"), 0)
    end = result.rom_end_offset
    where, limit = result.limit
    used = result.used
    where_text = re.sub(r"(slot\d_partition)", r"`\1`", where)  # the partition name as code

    lines = [
        "| P02 in `slot0_partition` | Value |",
        "|---|---|",
        f"| Board target | `{config.get('CONFIG_BOARD_TARGET', '?')}` |",
        f"| MCUboot mode | {mode} |",
        f"| Slot size | {size_text(slot)} (0x{slot:X}) |",
        f"| Header (`ROM_START_OFFSET`) | 0x{start:X} ({size_text(start)}) |",
        f"| End of the slot kept free (`ROM_END_OFFSET`) | 0x{end:X} ({size_text(end)}): MCUboot's trailer "
        f"{size_text(result.trailer)} + its estimate for the TLVs {size_text(result.tlv_estimate)} |",
        f"| Maximum header + code + TLVs | {exact_size_text(limit)}, set by {where_text} |",
        f"| P02 today: header + code + TLVs | {exact_size_text(used)}, of which TLVs "
        f"{size_text(result.tlvs)} |",
        f"| Margin | {size_text(limit - used)} |",
    ]

    if result.ram is not None:
        # RAM load: MCUboot copies the image into this RAM, and P02's data and stacks are in it too.
        ram = result.ram
        lines.append(f"| RAM MCUboot loads P02 into | {size_text(ram.size)} at 0x{ram.start:08X} |")
        lines.append(f"| P02 today in RAM: code, data, stacks | {exact_size_text(ram.used, with_hex=False)} |")
        lines.append(f"| RAM margin | {size_text(ram.size - ram.used)} |")
    return lines


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("build", type=Path, help="sysbuild build directory, e.g. zephyr/app/build/nucleo_g0b1re")
    args = parser.parse_args()

    edt = load_edt(args.build / "app")
    memory = memory_table(edt)
    print("\n".join(memory))
    print()
    print("\n".join(p02_table(args.build, edt)))


if __name__ == "__main__":
    main()
