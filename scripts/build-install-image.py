#!/usr/bin/env python3
"""Pack a sparse ZealOS disk image for the ARM64 ISO installer."""
import argparse
import pathlib
import struct

SECTOR_SIZE = 512
MAGIC = b"ZINST001"
HEADER = struct.Struct("<8sIIQIIQ")
EXTENT = struct.Struct("<QII")


def pack_image(source: pathlib.Path, destination: pathlib.Path) -> tuple[int, int, int]:
    size = source.stat().st_size
    if size == 0 or size % SECTOR_SIZE:
        raise ValueError("source size must be a nonzero multiple of 512 bytes")
    extents: list[tuple[int, int]] = []
    data = bytearray()
    active_start = None
    active_count = 0
    with source.open("rb") as stream:
        lba = 0
        while block := stream.read(SECTOR_SIZE):
            if len(block) != SECTOR_SIZE:
                raise ValueError("source ended on a partial sector")
            if any(block):
                if active_start is None:
                    active_start = lba
                active_count += 1
                data.extend(block)
            elif active_start is not None:
                extents.append((active_start, active_count))
                active_start = None
                active_count = 0
            lba += 1
    if active_start is not None:
        extents.append((active_start, active_count))

    table_bytes = len(extents) * EXTENT.size
    payload_size = HEADER.size + table_bytes + len(data)
    with destination.open("wb") as out:
        out.write(HEADER.pack(MAGIC, 1, SECTOR_SIZE, size // SECTOR_SIZE,
                              len(extents), EXTENT.size, payload_size))
        for start, count in extents:
            out.write(EXTENT.pack(start, count, 0))
        out.write(data)
    return size, payload_size, len(extents)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("destination", type=pathlib.Path)
    args = parser.parse_args()
    raw, packed, count = pack_image(args.source, args.destination)
    print(f"installer image: {packed} bytes ({count} extents) from {raw} bytes")


if __name__ == "__main__":
    main()
