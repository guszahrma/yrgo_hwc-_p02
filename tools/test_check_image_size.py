#!/usr/bin/env python3
"""Tests for check_image_size.py, on fake build directories (no Zephyr build needed).

Run from the repository root:
    python3 -m unittest discover -s tools -p "test_*.py"
"""

import contextlib
import io
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

import check_image_size

SECTOR = 0x800
TLVS = 0x28  # a TLV area with a SHA256 hash, as in an unsigned P02 image
RAM_START = 0x24000000  # MCUboot's RAM for the image, as on the Daisy
ESTIMATE = 40  # MCUboot's estimate for the TLVs without a signature (check_image_size.tlv_estimate())


def fake_edt(slot0: int, slot1: int | None, sector: int = SECTOR):
    """Stand-in for the devicetree: only what check_image_size.partition() reads."""
    flash = SimpleNamespace(props={"erase-block-size": SimpleNamespace(val=sector)})
    partitions = SimpleNamespace(parent=flash)
    nodes = {"slot0_partition": SimpleNamespace(parent=partitions, regs=[SimpleNamespace(size=slot0)])}
    if slot1 is not None:
        nodes["slot1_partition"] = SimpleNamespace(parent=partitions, regs=[SimpleNamespace(size=slot1)])
    return SimpleNamespace(label2node=nodes)


class CheckImageSizeTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.build = Path(self.tmp.name)
        self.addCleanup(self.tmp.cleanup)

    def make_build(self, used: int, *, end: int = ESTIMATE, offset: bool = False, ram_load: int | None = None,
                   magic: int = check_image_size.IMAGE_MAGIC, hdr_size: int = 0x200, tlvs: int | None = TLVS,
                   protected: int = 0, load_addr: int | None = None):
        """Writes the Kconfig files and a signed image whose header + code is `used` bytes, followed by a
        protected TLV area of `protected` bytes (none if 0) and a TLV area of `tlvs` bytes (none if None).
        `end` is ROM_END_OFFSET: MCUboot's trailer + ESTIMATE (by default no trailer). `load_addr` is the
        header's load address (by default the start of MCUboot's RAM, as on the Daisy)."""
        app = self.build / "app" / "zephyr"
        mcuboot = self.build / "mcuboot" / "zephyr"
        sysbuild = self.build / "zephyr"
        for directory in (app, mcuboot, sysbuild):
            directory.mkdir(parents=True)
        (sysbuild / ".config").write_text("SB_CONFIG_MCUBOOT_MODE_SWAP_USING_OFFSET=y\n" if offset else "")
        (app / ".config").write_text(f"CONFIG_ROM_END_OFFSET=0x{end:x}\n")
        (mcuboot / ".config").write_text(
            f"CONFIG_BOOT_RAM_LOAD=y\nCONFIG_BOOT_IMAGE_EXECUTABLE_RAM_START=0x{RAM_START:x}\n"
            f"CONFIG_BOOT_IMAGE_EXECUTABLE_RAM_SIZE={ram_load}\n" if ram_load else "")
        load_addr = RAM_START if load_addr is None else load_addr
        image = bytearray(struct.pack("<IIHHI", magic, load_addr, hdr_size, protected, used - hdr_size))
        image += bytes(used - len(image))
        if protected:
            image += struct.pack("<HH", check_image_size.TLV_PROT_INFO_MAGIC, protected)
            image += bytes(protected - 4)
        if tlvs is not None:
            image += struct.pack("<HH", check_image_size.TLV_INFO_MAGIC, tlvs) + bytes(tlvs - 4)
        (app / "zephyr.signed.bin").write_bytes(image)

    def run_check(self, edt, ram_in_use: int = 0):
        """Runs the check; for RAM load, P02's code, data and stacks in the RAM are `ram_in_use` bytes
        (instead of reading an ELF)."""
        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(check_image_size, "load_edt", return_value=edt), \
                mock.patch.object(check_image_size, "ram_used", return_value=ram_in_use), \
                mock.patch.object(sys, "argv", ["check_image_size.py", str(self.build)]), \
                contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            code = check_image_size.main()
        return code, stdout.getvalue(), stderr.getvalue()

    def test_fits(self):
        # Small image, large slots: passes.
        self.make_build(0x1000)
        code, out, _ = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 0)
        self.assertIn("P02 fits", out)

    def test_slot0_one_byte_too_many(self):
        # Image 1 B too large for slot 0: fails, Error message names slot 0.
        self.make_build(0x8000 - 0x100 + 1, end=0x100)
        code, _, err = self.run_check(fake_edt(0x8000, 0x9000))
        self.assertEqual(code, 1)
        self.assertIn("slot0_partition allows", err)
        self.assertIn("1 B too many", err)

    def test_slot1_one_byte_too_many(self):
        # Slot 1 smaller than slot 0, image 1 B too large for slot 1: fails, Error message names slot 1.
        self.make_build(0x7000 - 0x100 + 1, end=0x100)
        code, _, err = self.run_check(fake_edt(0x8000, 0x7000))
        self.assertEqual(code, 1)
        self.assertIn("slot1_partition allows", err)
        self.assertIn("1 B too many", err)

    def test_swap_using_offset_slot1_one_sector_larger_fits(self):
        # Our Nucleo/Nano layout, image exactly at the limit: passes.
        self.make_build(0x8000 - 0x100, end=0x100, offset=True)
        code, out, _ = self.run_check(fake_edt(0x8000, 0x8000 + SECTOR))
        self.assertEqual(code, 0)
        self.assertIn("P02 fits", out)

    def test_swap_using_offset_slot1_one_sector_larger_one_byte_too_many(self):
        # Our Nucleo/Nano layout, image 1 B too large: fails.
        self.make_build(0x8000 - 0x100 + 1, end=0x100, offset=True)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000 + SECTOR))
        self.assertEqual(code, 1)
        self.assertIn("1 B too many", err)

    def test_swap_using_offset_equal_slots(self):
        # Swap using offset, equal slots: fails (not our layout).
        self.make_build(0x1000, offset=True)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 1)
        self.assertIn("slot0_partition 16 sectors, slot1_partition 16", err)

    def test_swap_using_offset_two_sectors_more_in_slot1(self):
        # Swap using offset, slot 1 two sectors larger: fails (not our layout).
        self.make_build(0x1000, offset=True)
        code, _, err = self.run_check(fake_edt(0x8000 - SECTOR, 0x8000 + SECTOR))
        self.assertEqual(code, 1)
        self.assertIn("slot0_partition 15 sectors, slot1_partition 17", err)

    def test_swap_using_offset_one_sector_less_in_slot1(self):
        # Swap using offset, slot 1 one sector smaller: fails (not our layout).
        self.make_build(0x1000, offset=True)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000 - SECTOR))
        self.assertEqual(code, 1)
        self.assertIn("slot0_partition 16 sectors, slot1_partition 15", err)

    def test_swap_using_offset_different_sector_sizes(self):
        # Swap using offset, different sector sizes: fails (not our layout).
        self.make_build(0x1000, offset=True)
        edt = fake_edt(0x8000, 0x8000 + SECTOR)
        edt.label2node["slot1_partition"].parent = SimpleNamespace(
            parent=SimpleNamespace(props={"erase-block-size": SimpleNamespace(val=2 * SECTOR)}))
        code, _, err = self.run_check(edt)
        self.assertEqual(code, 1)
        self.assertIn("different sectors", err)

    def test_without_swap_using_offset_sectors_not_compared(self):
        # Other mode, slot 1 two sectors larger: passes (sectors only checked for swap using offset).
        self.make_build(0x1000)
        code, _, _ = self.run_check(fake_edt(0x8000 - SECTOR, 0x8000 + SECTOR))
        self.assertEqual(code, 0)

    def test_without_swap_using_offset_slot1_is_whole(self):
        # Other mode, equal slots, image exactly at the limit: passes (all of slot 1 counts).
        self.make_build(0x8000 - 0x100, end=0x100)
        code, _, _ = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 0)

    def test_ram_load_fits_with_tlvs(self):
        # RAM load (Daisy), image + TLVs exactly as large as the RAM: passes.
        self.make_build(0x4000 - TLVS, ram_load=0x4000)
        code, _, _ = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 0)

    def test_ram_load_one_byte_too_many(self):
        # RAM load (Daisy), image + TLVs 1 B too large for the RAM: fails, Error message names the RAM.
        self.make_build(0x4000 - TLVS + 1, ram_load=0x4000)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 1)
        self.assertIn("MCUboot's RAM for the image allows", err)
        self.assertIn("1 B too many", err)

    def test_ram_load_tlvs_counted(self):
        # RAM load, image exactly as large as the RAM, but the TLVs do not fit: fails.
        self.make_build(0x4000, ram_load=0x4000)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 1)
        self.assertIn("40 B too many", err)

    def test_ram_load_protected_tlvs_counted(self):
        # RAM load with a protected TLV area (e.g. our own TLVs), which does not fit: fails.
        self.make_build(0x4000 - TLVS, ram_load=0x4000, protected=0x10)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 1)
        self.assertIn("16 B too many", err)

    def test_ram_load_code_data_stacks_fit(self):
        # RAM load, code, data and stacks exactly as large as the RAM: passes, and the output shows them.
        self.make_build(0x1000, ram_load=0x4000)
        code, out, _ = self.run_check(fake_edt(0x8000, 0x8000), ram_in_use=0x4000)
        self.assertEqual(code, 0)
        self.assertIn("P02 fits in MCUboot's RAM: code, data and stacks 16.0 KB of 16.0 KB", out)

    def test_ram_load_code_data_stacks_one_byte_too_many(self):
        # RAM load, a small image, but code, data and stacks 1 B larger than the RAM: fails.
        self.make_build(0x1000, ram_load=0x4000)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000), ram_in_use=0x4001)
        self.assertEqual(code, 1)
        self.assertIn("P02 does not fit in MCUboot's RAM: code, data and stacks 16385 B", err)
        self.assertIn("1 B too many", err)

    def test_ram_load_limit_from_the_load_address(self):
        # RAM load, image loaded 0x1000 into the RAM: the image may use only the rest of it, as in MCUboot.
        self.make_build(0x3000 - TLVS + 1, ram_load=0x4000, load_addr=RAM_START + 0x1000)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 1)
        self.assertIn("MCUboot's RAM for the image allows 12288 B", err)
        self.assertIn("1 B too many", err)

    def test_ram_load_address_outside_the_ram(self):
        # RAM load, load address below MCUboot's RAM: fails with an error, as MCUboot refuses the image.
        self.make_build(0x1000, ram_load=0x4000, load_addr=RAM_START - 0x400)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 1)
        self.assertIn("load address 0x23FFFC00 is outside MCUboot's RAM", err)

    def test_app_ram_end_counts_the_apps_whole_ram(self):
        # MCUboot's RAM inside the app's zephyr,sram: sections up to the end of the app's RAM count.
        sram = SimpleNamespace(regs=[SimpleNamespace(addr=RAM_START, size=0x80000)])
        edt = SimpleNamespace(chosen_nodes={"zephyr,sram": sram})
        self.assertEqual(check_image_size.app_ram_end(edt, RAM_START, 0x40000), RAM_START + 0x80000)
        # The app's RAM elsewhere, or no zephyr,sram: MCUboot's end.
        self.assertEqual(check_image_size.app_ram_end(edt, 0x20000000, 0x40000), 0x20040000)
        self.assertEqual(check_image_size.app_ram_end(SimpleNamespace(), RAM_START, 0x40000), RAM_START + 0x40000)

    def test_slot0_own_tlvs_counted(self):
        # Our Nucleo/Nano layout, image at the limit with MCUboot's TLV estimate, plus 16 B of our own TLVs:
        # fails, Error message names slot 0.
        self.make_build(0x8000 - 0x100, end=0x100, offset=True, protected=0x10)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000 + SECTOR))
        self.assertEqual(code, 1)
        self.assertIn("slot0_partition allows", err)
        self.assertIn("16 B too many", err)

    def test_rom_end_offset_smaller_than_estimate(self):
        # ROM_END_OFFSET smaller than MCUboot's TLV estimate (tlv_estimate() out of date): fails with an error.
        self.make_build(0x1000, end=ESTIMATE - 1)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 1)
        self.assertIn("smaller than MCUboot's estimate for the TLVs", err)

    def test_tlv_estimate_by_signature_type(self):
        # MCUboot's TLV estimate for each signature type, as in mcuboot/boot/zephyr/CMakeLists.txt.
        estimate = check_image_size.tlv_estimate
        self.assertEqual(estimate({}), 40)
        self.assertEqual(estimate({"CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256": "y"}), 150)
        self.assertEqual(estimate({"CONFIG_BOOT_SIGNATURE_TYPE_ED25519": "y"}), 144)
        self.assertEqual(estimate({"CONFIG_BOOT_SIGNATURE_TYPE_RSA": "y"}), 336)
        self.assertEqual(estimate({"CONFIG_BOOT_SIGNATURE_TYPE_RSA": "y",
                                   "CONFIG_BOOT_SIGNATURE_TYPE_RSA_LEN": "3072"}), 464)

    def test_ram_load_no_tlv_area(self):
        # RAM load, no TLV area after the code: fails with an error.
        self.make_build(0x1000, ram_load=0x4000, tlvs=None)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 1)
        self.assertIn("no TLV area after the code", err)

    def test_ram_load_protected_tlv_area_does_not_match_header(self):
        # RAM load, the header names a protected TLV area that is not there: fails with an error.
        self.make_build(0x1000, ram_load=0x4000)
        image = self.build / "app" / "zephyr" / "zephyr.signed.bin"
        data = bytearray(image.read_bytes())
        struct.pack_into("<H", data, 10, 0x10)  # protect_tlv_size in the header
        image.write_bytes(data)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 1)
        self.assertIn("protected TLV area does not match", err)


    def test_smallest_limit_wins(self):
        # Slot 1 smallest of slot 0, slot 1 and RAM: fails, Error message names slot 1.
        self.make_build(0x7000, ram_load=0x7000)
        code, _, err = self.run_check(fake_edt(0x8000, 0x6000))
        self.assertEqual(code, 1)
        self.assertIn("slot1_partition allows 24576 B", err)

    def test_no_slot1(self):
        # No slot 1: only slot 0 is checked.
        self.make_build(0x8000 - TLVS)
        code, out, _ = self.run_check(fake_edt(0x8000, None))
        self.assertEqual(code, 0)
        self.assertIn("(slot0_partition)", out)

    def test_no_slot0(self):
        # No slot 0: fails with an error.
        self.make_build(0x1000)
        edt = fake_edt(0x8000, 0x8000)
        del edt.label2node["slot0_partition"]
        code, _, err = self.run_check(edt)
        self.assertEqual(code, 1)
        self.assertIn("no slot0_partition", err)

    def test_not_an_mcuboot_image(self):
        # Not an MCUboot image: fails with an error.
        self.make_build(0x1000, magic=0x12345678)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 1)
        self.assertIn("is not an MCUboot image", err)


if __name__ == "__main__":
    unittest.main()
