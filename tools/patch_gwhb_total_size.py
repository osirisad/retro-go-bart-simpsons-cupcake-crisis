#!/usr/bin/env python3
"""Patch the total_size field of a GWHB header in-place.

gwhb_header_t.total_size (offset 16, little-endian uint32) can't be known
at link time — it depends on the final objcopy'd .bin size. Run after objcopy.
"""
import struct
import sys

MAGIC = 0x42485747  # 'GWHB' little-endian
TOTAL_SIZE_OFFSET = 16


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <cupcake.bin>", file=sys.stderr)
        return 1

    path = sys.argv[1]
    with open(path, "r+b") as f:
        data = f.read()
        if len(data) < 512:
            print(f"error: {path} is smaller than the 512-byte GWHB header", file=sys.stderr)
            return 1

        magic = struct.unpack_from("<I", data, 0)[0]
        if magic != MAGIC:
            print(f"error: {path} does not start with the GWHB magic (got 0x{magic:08x})", file=sys.stderr)
            return 1

        f.seek(TOTAL_SIZE_OFFSET)
        f.write(struct.pack("<I", len(data)))

    print(f"patched total_size = {len(data)} bytes into {path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
