#!/usr/bin/env python3
"""Package a flat F29H85x application binary for this FoE bootloader."""

import argparse
import struct
import zlib
from pathlib import Path

APP_ADDRESS = 0x10040000
CAPACITY = 0x1BE000
MANIFEST_SIZE = 32
MAX_WIRE_SIZE = CAPACITY + MANIFEST_SIZE
IMAGE_MAGIC = 0x46323948
IMAGE_TARGET = 0xF29A8501


def pack(payload: bytes) -> bytes:
    if len(payload) < 8 or len(payload) > CAPACITY:
        raise ValueError(f"payload must be 8..{CAPACITY} octets")
    if payload[:4] in (b"\xff" * 4, b"\0" * 4):
        raise ValueError("blank codestart")
    header = struct.pack("<7I", IMAGE_MAGIC, 1, IMAGE_TARGET, len(payload),
                         APP_ADDRESS, zlib.crc32(payload), 0)
    manifest = header + struct.pack("<I", zlib.crc32(header))
    return manifest + payload


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.input.resolve() == args.output.resolve():
        parser.error("input and output must differ")
    image = pack(args.input.read_bytes())
    args.output.write_bytes(image)
    print(f"{args.output}: {len(image)} wire octets; FoE password 0xF290")


if __name__ == "__main__":
    main()
