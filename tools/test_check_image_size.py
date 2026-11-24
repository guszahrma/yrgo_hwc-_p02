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

    def make_build(self, used: int, *, end: int = 0, offset: bool = False, ram_load: int | None = None,
                   magic: int = check_image_size.IMAGE_MAGIC, hdr_size: int = 0x200):
        """Writes the Kconfig files and a signed image whose header + code is `used` bytes."""
        app = self.build / "app" / "zephyr"
        mcuboot = self.build / "mcuboot" / "zephyr"
        sysbuild = self.build / "zephyr"
        for directory in (app, mcuboot, sysbuild):
            directory.mkdir(parents=True)
        (sysbuild / ".config").write_text("SB_CONFIG_MCUBOOT_MODE_SWAP_USING_OFFSET=y\n" if offset else "")
        (app / ".config").write_text(f"CONFIG_ROM_END_OFFSET=0x{end:x}\n")
        (mcuboot / ".config").write_text(
            f"CONFIG_BOOT_RAM_LOAD=y\nCONFIG_BOOT_IMAGE_EXECUTABLE_RAM_SIZE={ram_load}\n" if ram_load else "")
        header = struct.pack("<IIHHI", magic, 0, hdr_size, 0, used - hdr_size)
        (app / "zephyr.signed.bin").write_bytes(header)

    def run_check(self, edt):
        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(check_image_size, "load_edt", return_value=edt), \
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

    def test_ram_load_one_byte_too_many(self):
        # RAM load (Daisy), image 1 B too large for the RAM: fails, Error message names the RAM.
        self.make_build(0x4000 + 1, ram_load=0x4000)
        code, _, err = self.run_check(fake_edt(0x8000, 0x8000))
        self.assertEqual(code, 1)
        self.assertIn("MCUboot's RAM for the image allows", err)
        self.assertIn("1 B too many", err)

    def test_smallest_limit_wins(self):
        # Slot 1 smallest of slot 0, slot 1 and RAM: fails, Error message names slot 1.
        self.make_build(0x7000, ram_load=0x7000)
        code, _, err = self.run_check(fake_edt(0x8000, 0x6000))
        self.assertEqual(code, 1)
        self.assertIn("slot1_partition allows 24576 B", err)

    def test_no_slot1(self):
        # No slot 1: only slot 0 is checked.
        self.make_build(0x8000)
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
