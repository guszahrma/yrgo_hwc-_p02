#!/usr/bin/env python3
"""Tests for flash_layout.py, on fake build directories (no Zephyr build needed).

Run from the repository root:
    python3 -m unittest discover -s tools -p "test_*.py"
"""

import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

import check_image_size
import flash_layout
import test_check_image_size
from test_check_image_size import ESTIMATE, SECTOR, fake_edt


def fake_partition(label: str, offset: int, size: int):
    return SimpleNamespace(labels=[label], name=label, regs=[SimpleNamespace(addr=offset, size=size)])


def fake_flash_edt(flash_size: int, partitions: list, sector: int = SECTOR):
    """Stand-in for the devicetree: only what flash_layout.memory_table() reads."""
    flash = SimpleNamespace(labels=["flash0"], name="flash@0", regs=[SimpleNamespace(size=flash_size)],
                            props={"erase-block-size": SimpleNamespace(val=sector)})
    node = SimpleNamespace(parent=flash, children={label: fake_partition(label, offset, size)
                                                   for label, offset, size in partitions})
    return SimpleNamespace(compat2nodes={"fixed-partitions": [node]})


class MemoryTableTest(unittest.TestCase):
    def test_partitions_in_order_with_unused_ranges(self):
        # Partitions given out of order; a gap after slot 0 and free space at the end.
        edt = fake_flash_edt(0x10000, [("slot1_partition", 0x6800, 0x4000),
                                       ("boot_partition", 0x0, 0x2000),
                                       ("slot0_partition", 0x2000, 0x4000)])
        rows = flash_layout.memory_table(edt)[2:]
        self.assertEqual(rows, [
            "| `flash0` | 64 KB | 2 KB | `boot_partition` | 0x0 | 8 KB |",
            "|  |  |  | `slot0_partition` | 0x2000 | 16 KB |",
            "|  |  |  | *unused* | 0x6000 | 2 KB |",
            "|  |  |  | `slot1_partition` | 0x6800 | 16 KB |",
            "|  |  |  | *unused* | 0xA800 | 22 KB |",
        ])

    def test_no_unused_rows_when_full(self):
        edt = fake_flash_edt(0x4000, [("slot0_partition", 0x0, 0x2000), ("slot1_partition", 0x2000, 0x2000)])
        self.assertNotIn("*unused*", "\n".join(flash_layout.memory_table(edt)))


class ExactSizeTextTest(unittest.TestCase):
    def test_just_under_a_whole_size_is_not_rounded_up(self):
        # The Daisy's maximum: 512 KB slot minus a 48 B trailer.
        self.assertEqual(flash_layout.exact_size_text(512 * 1024 - 48), "511.95 KB (524,240 B = 0x7FFD0)")

    def test_whole_sizes_stay_short(self):
        self.assertEqual(flash_layout.exact_size_text(196 * 1024), "196 KB (0x31000)")
        self.assertEqual(flash_layout.exact_size_text(40), "40 B (0x28)")

    def test_without_hex(self):
        self.assertEqual(flash_layout.exact_size_text(152956, with_hex=False), "149.37 KB (152,956 B)")


class P02TableTest(unittest.TestCase):
    """The P02 table must show the limits that check_image_size.py enforces."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.build = Path(self.tmp.name)
        self.addCleanup(self.tmp.cleanup)

    def make_build(self, used: int, **kwargs):
        # The size check's fake build; imported as a module, so its tests do not run twice.
        test_check_image_size.CheckImageSizeTest.make_build(self, used, **kwargs)

    def row(self, lines: list, name: str) -> str:
        return next(line for line in lines if line.startswith(f"| {name} |"))

    def test_limit_and_margin_are_the_size_checks(self):
        # Swap using offset, our layout: slot 1 one sector more. Trailer 0x800, so the limit is slot 0 - 0x800.
        self.make_build(0x1000, end=0x800 + ESTIMATE, offset=True)
        edt = fake_edt(0x8000, 0x8000 + SECTOR)
        lines = flash_layout.p02_table(self.build, edt)
        result = check_image_size.measure(self.build, edt=edt)
        self.assertEqual(result.limit, ("slot0_partition", 0x7800))
        self.assertIn("(0x7800), set by `slot0_partition`", self.row(lines, "Maximum header + code + TLVs"))
        self.assertIn("(4,136 B = 0x1028)", self.row(lines, "P02 today: header + code + TLVs"))
        self.assertEqual(self.row(lines, "Margin"), f"| Margin | {flash_layout.size_text(0x7800 - 0x1028)} |")
        self.assertIn("MCUboot's trailer 2 KB + its estimate for the TLVs 40 B",
                      self.row(lines, "End of the slot kept free (`ROM_END_OFFSET`)"))

    def test_smaller_slot1_sets_the_limit(self):
        self.make_build(0x1000)
        lines = flash_layout.p02_table(self.build, fake_edt(0x8000, 0x4000))
        self.assertIn("(0x4000), set by `slot1_partition`", self.row(lines, "Maximum header + code + TLVs"))

    def test_ram_load_shows_ram_used(self):
        self.make_build(0x1000, ram_load=0x3000)
        with mock.patch.object(check_image_size, "ram_used", return_value=0x2000):
            lines = flash_layout.p02_table(self.build, fake_edt(0x8000, 0x8000))
        self.assertIn("set by MCUboot's RAM for the image", self.row(lines, "Maximum header + code + TLVs"))
        self.assertEqual(self.row(lines, "RAM MCUboot loads P02 into"),
                         "| RAM MCUboot loads P02 into | 12 KB at 0x24000000 |")
        self.assertEqual(self.row(lines, "RAM margin"), "| RAM margin | 4 KB |")

    def test_build_that_cannot_be_measured_stops(self):
        self.make_build(0x1000, magic=0)
        with self.assertRaises(SystemExit) as stop:
            flash_layout.p02_table(self.build, fake_edt(0x8000, 0x8000))
        self.assertIn("not an MCUboot image", str(stop.exception.code))


if __name__ == "__main__":
    unittest.main()
