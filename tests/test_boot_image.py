#!/usr/bin/env python3
import ctypes as C
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("packer", ROOT / "tools/pack_foe_image.py")
PACKER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACKER)
PAYLOAD = bytes([0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88]) + bytes(range(256))
IMAGE = PACKER.pack(PAYLOAD)


class BootImage(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.library = Path(cls.tmp.name) / "boot_image.so"
        subprocess.run([
            "cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-fPIC", "-shared",
            "-I" + str(ROOT), str(ROOT / "boot_image.c"),
            str(ROOT / "tests/boot_image_harness.c"), "-o", str(cls.library)
        ], check=True)
        cls.lib = C.CDLL(str(cls.library))
        cls.lib.test_load.argtypes = [C.c_void_p, C.c_uint32]

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def setUp(self):
        self.lib.test_reset()

    def load(self):
        data = C.create_string_buffer(IMAGE)
        self.assertEqual(self.lib.test_load(data, len(IMAGE)), 1)

    def test_packer_and_validator_agree(self):
        self.load()
        self.assertEqual(self.lib.test_valid(0), 1)
        self.assertEqual(self.lib.test_valid(1), 0)
        self.assertEqual(self.lib.test_finish(len(IMAGE)), 1)
        self.assertEqual(self.lib.test_valid(1), 1)

    def test_payload_corruption_is_rejected(self):
        self.load()
        self.lib.test_corrupt(40)
        self.assertEqual(self.lib.test_valid(0), 0)

    def test_wrong_wire_size_cannot_commit(self):
        self.load()
        self.assertEqual(self.lib.test_finish(len(IMAGE) - 16), 0)

    def test_packer_rejects_blank_or_oversized_images(self):
        for payload in (b"", b"\0" * 8, b"\xff" * 8,
                        b"x" * (PACKER.CAPACITY + 1)):
            with self.assertRaises(ValueError):
                PACKER.pack(payload)


if __name__ == "__main__":
    unittest.main(verbosity=2)
