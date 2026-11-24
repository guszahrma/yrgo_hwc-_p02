#!/usr/bin/env python3
"""Fails the build if P02's signed image does not fit where MCUboot needs it.

Run by sysbuild after the app's image is signed (zephyr/app/sysbuild.cmake); can also be run by hand on a
sysbuild build directory, in the Zephyr environment:
    tools/check_image_size.py zephyr/app/build/nucleo_g0b1re

Header + code + TLVs (the image as MCUboot reads it, boot_read_image_size() in
mcuboot/boot/bootutil/src/bootutil_misc.c; the TLVs' size is read from the TLV areas right after the code)
must fit in:
* slot 0 minus MCUboot's trailer, where P02 runs or is stored;
* slot 1 minus MCUboot's trailer, where an update is written; for swap using offset one sector further in,
  since MCUboot's sysbuild code leaves that sector out of ROM_END_OFFSET (a #todo in
  mcuboot/boot/zephyr/CMakeLists.txt);
* for RAM load, the RAM MCUboot copies the image into, from the load address in the image header
  (boot_verify_ram_load_address() in mcuboot/boot/bootutil/src/ram_load.c).

For RAM load, P02's code, data and stacks (the app ELF's sections from the start of that RAM) must also fit
in it: MCUboot's RAM is set in its own Kconfig, the app's in its devicetree, so they can differ.

The trailer is ROM_END_OFFSET minus MCUboot's fixed estimate for the TLVs (tlv_estimate()), so that the
image's real TLVs are counted, our own ones included, which the estimate leaves out.

The linker checks slot 0 on Cortex-M, with the estimate instead of the real TLVs, but not on the ESP32-S3,
and nothing checks slot 1.

For swap using offset, slot 1 must also have the same sector size and exactly one sector more than slot 0,
the layout in zephyr/app/partitions/<board>.overlay.
"""

import argparse
import pickle
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

IMAGE_MAGIC = 0x96F3B83D  # MCUboot's image header magic
TLV_INFO_MAGIC = 0x6907  # the TLV area: hash, key hash, signature
TLV_PROT_INFO_MAGIC = 0x6908  # the protected TLV area, covered by the signature


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


def tlv_size(image: bytes, hdr_size: int, img_size: int, protect_tlv_size: int):
    """Size of the image's TLV areas after the code, as MCUboot reads it, or an error message."""
    off = hdr_size + img_size
    if protect_tlv_size:
        if len(image) < off + 4:
            return None, "no protected TLV area after the code"
        magic, total = struct.unpack_from("<HH", image, off)
        if magic != TLV_PROT_INFO_MAGIC or total != protect_tlv_size:
            return None, f"the protected TLV area does not match the header (magic 0x{magic:04x}, {total} B)"
        off += protect_tlv_size
    if len(image) < off + 4:
        return None, "no TLV area after the code"
    magic, total = struct.unpack_from("<HH", image, off)
    if magic != TLV_INFO_MAGIC:
        return None, f"no TLV area after the code (magic 0x{magic:04x})"
    return protect_tlv_size + total, None


def tlv_estimate(mcuboot: dict) -> int:
    """MCUboot's estimate for the TLVs in ROM_END_OFFSET, by signature type.

    The same table as mcuboot/boot/zephyr/CMakeLists.txt (boot_tlv_estimate); change both together.
    """
    if mcuboot.get("CONFIG_BOOT_SIGNATURE_TYPE_RSA") == "y":
        return 464 if mcuboot.get("CONFIG_BOOT_SIGNATURE_TYPE_RSA_LEN") == "3072" else 336
    if mcuboot.get("CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256") == "y":
        return 150
    if mcuboot.get("CONFIG_BOOT_SIGNATURE_TYPE_ED25519") == "y":
        return 144
    return 40


def app_ram_end(edt, ram_start: int, ram_size: int) -> int:
    """Where to stop looking for P02's sections: the end of the app's RAM (zephyr,sram) if MCUboot's RAM
    starts in it, so a section past the end of MCUboot's RAM is counted too; otherwise MCUboot's end."""
    end = ram_start + ram_size
    sram = getattr(edt, "chosen_nodes", {}).get("zephyr,sram")
    if sram is not None and sram.regs and sram.regs[0].addr <= ram_start < sram.regs[0].addr + sram.regs[0].size:
        end = max(end, sram.regs[0].addr + sram.regs[0].size)
    return end


def ram_used(elf_path: Path, ram_start: int, search_end: int) -> int:
    """Bytes from ram_start to the end of the last allocated section of the ELF below search_end."""
    from elftools.elf.constants import SH_FLAGS  # pyelftools: in the Zephyr environment, not needed by tests
    from elftools.elf.elffile import ELFFile

    end = ram_start
    with open(elf_path, "rb") as file:
        for section in ELFFile(file).iter_sections():
            address, size = section["sh_addr"], section["sh_size"]
            if section["sh_flags"] & SH_FLAGS.SHF_ALLOC and size and ram_start <= address < search_end:
                end = max(end, address + size)
    return end - ram_start


def kb(size: int) -> str:
    return f"{size / 1024:.1f} KB"


class SizeError(Exception):
    """The build cannot be measured: a file or partition is missing, or the image is not what MCUboot reads."""


@dataclass
class RamUse:
    start: int  # MCUboot's RAM for the image (CONFIG_BOOT_IMAGE_EXECUTABLE_RAM_START)
    size: int  # CONFIG_BOOT_IMAGE_EXECUTABLE_RAM_SIZE
    used: int  # P02's code, data and stacks from start: the app ELF's sections


@dataclass
class Measurement:
    hdr_size: int  # MCUboot's image header
    code: int  # code and data after the header
    tlvs: int  # the TLV areas after the code, protected ones included
    rom_end_offset: int  # what the build keeps free at the end of the slot (CONFIG_ROM_END_OFFSET)
    tlv_estimate: int  # MCUboot's estimate for the TLVs in ROM_END_OFFSET
    limits: list  # (where, maximum header + code + TLVs), one per place the image must fit
    ram: RamUse | None = None  # RAM load only

    @property
    def used(self) -> int:
        """Header + code + TLVs: the image as MCUboot reads it."""
        return self.hdr_size + self.code + self.tlvs

    @property
    def trailer(self) -> int:
        """MCUboot's trailer: ROM_END_OFFSET minus its estimate for the TLVs, which `used` counts for real."""
        return self.rom_end_offset - self.tlv_estimate

    @property
    def limit(self) -> tuple:
        """The smallest limit: (where, maximum header + code + TLVs)."""
        return min(self.limits, key=lambda item: item[1])


def measure(build: Path, image_name: str = "app", edt=None) -> Measurement:
    """Reads a sysbuild build directory: the signed image's size and every limit it must fit in.

    Raises SizeError. Used by tools/flash_layout.py too, so the docs show the limits the build checks.
    """
    image_dir = build / image_name
    sysbuild = read_kconfig(build / "zephyr" / ".config")
    config = read_kconfig(image_dir / "zephyr" / ".config")
    mcuboot = read_kconfig(build / "mcuboot" / "zephyr" / ".config")
    if edt is None:
        edt = load_edt(image_dir)

    signed = image_dir / "zephyr" / "zephyr.signed.bin"
    image = signed.read_bytes()
    magic, load_addr, hdr_size, protect_tlv_size, img_size = struct.unpack_from("<IIHHI", image)
    if magic != IMAGE_MAGIC:
        raise SizeError(f"{signed} is not an MCUboot image")
    tlvs, error = tlv_size(image, hdr_size, img_size, protect_tlv_size)
    if error:
        raise SizeError(f"{signed}: {error}")
    rom_end_offset = int(config.get("CONFIG_ROM_END_OFFSET", "0"), 0)
    estimate = tlv_estimate(mcuboot)
    trailer = rom_end_offset - estimate
    if trailer < 0:
        raise SizeError(f"ROM_END_OFFSET ({rom_end_offset} B) is smaller than MCUboot's estimate for the TLVs "
                        f"({estimate} B): tlv_estimate() no longer matches MCUboot")

    limits = []
    slot0 = partition(edt, "slot0_partition")
    if slot0 is None:
        raise SizeError("no slot0_partition in the app's devicetree")
    limits.append(("slot0_partition", slot0[0] - trailer))
    slot1 = partition(edt, "slot1_partition")
    if slot1 is not None:
        size, sector = slot1
        if sysbuild.get("SB_CONFIG_MCUBOOT_MODE_SWAP_USING_OFFSET") == "y":
            error = slots_incompatible_for_offset(slot0, slot1)
            if error:
                raise SizeError(error)
            limits.append(("slot1_partition, written one sector in", size - sector - trailer))
        else:
            limits.append(("slot1_partition", size - trailer))
    ram = None
    if mcuboot.get("CONFIG_BOOT_RAM_LOAD") == "y":
        ram_start = int(mcuboot["CONFIG_BOOT_IMAGE_EXECUTABLE_RAM_START"], 0)
        ram_size = int(mcuboot["CONFIG_BOOT_IMAGE_EXECUTABLE_RAM_SIZE"], 0)
        if not ram_start <= load_addr < ram_start + ram_size:
            raise SizeError(f"the image's load address 0x{load_addr:X} is outside MCUboot's RAM for the image "
                            f"(0x{ram_start:X}, {ram_size} B), so MCUboot refuses it")
        limits.append(("MCUboot's RAM for the image", ram_start + ram_size - load_addr))
        elf = image_dir / "zephyr" / "zephyr.elf"
        ram = RamUse(ram_start, ram_size, ram_used(elf, ram_start, app_ram_end(edt, ram_start, ram_size)))
    return Measurement(hdr_size, img_size, tlvs, rom_end_offset, estimate, limits, ram)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("build", type=Path, help="sysbuild build directory, e.g. zephyr/app/build/nucleo_g0b1re")
    parser.add_argument("--image", default="app", help="the app's image name in sysbuild (default: app)")
    args = parser.parse_args()

    try:
        result = measure(args.build, args.image)
    except SizeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    used = result.used
    where, limit = result.limit
    if used > limit:
        print(f"error: P02 does not fit: header + code + TLVs {used} B ({kb(used)}), but {where} allows "
              f"{limit} B ({kb(limit)}), {used - limit} B too many",
              file=sys.stderr)
        return 1
    ram = result.ram
    if ram is not None and ram.used > ram.size:
        print(f"error: P02 does not fit in MCUboot's RAM: code, data and stacks {ram.used} B ({kb(ram.used)}), "
              f"but the RAM is {ram.size} B ({kb(ram.size)}), {ram.used - ram.size} B too many",
              file=sys.stderr)
        return 1
    print(f"P02 fits: header + code + TLVs {kb(used)} of {kb(limit)} ({where}), margin {kb(limit - used)}")
    if ram is not None:
        print(f"P02 fits in MCUboot's RAM: code, data and stacks {kb(ram.used)} of {kb(ram.size)}, "
              f"margin {kb(ram.size - ram.used)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
