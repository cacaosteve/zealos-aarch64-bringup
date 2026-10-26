#!/usr/bin/env python3
"""Round-trip the sparse ARM64 installer payload against a raw disk image."""
import hashlib
import importlib.util
import pathlib
import struct
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "build_install_image", ROOT / "scripts/build-install-image.py")
module = importlib.util.module_from_spec(spec)
assert spec.loader
spec.loader.exec_module(module)


def digest(path: pathlib.Path) -> bytes:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        while block := stream.read(1024 * 1024):
            h.update(block)
    return h.digest()


def verify(source: pathlib.Path, packed: pathlib.Path, restored: pathlib.Path) -> None:
    raw_size = source.stat().st_size
    payload = packed.read_bytes()
    magic, version, sector_size, sectors, count, record_size, payload_size = \
        module.HEADER.unpack_from(payload)
    assert magic == module.MAGIC and version == 1
    assert sector_size == module.SECTOR_SIZE
    assert sectors * sector_size == raw_size
    assert record_size == module.EXTENT.size and payload_size == len(payload)
    table_end = module.HEADER.size + count * record_size
    assert table_end <= len(payload)
    expected_lba = 0
    data_offset = table_end
    with source.open("rb") as original, restored.open("wb") as output:
        for index in range(count):
            lba, extent_sectors, reserved = module.EXTENT.unpack_from(
                payload, module.HEADER.size + index * record_size)
            assert reserved == 0 and extent_sectors > 0 and lba >= expected_lba
            gap_bytes = (lba - expected_lba) * sector_size
            gap = original.read(gap_bytes)
            assert not any(gap)
            output.write(gap)
            data_bytes = extent_sectors * sector_size
            original_data = original.read(data_bytes)
            packed_data = payload[data_offset:data_offset + data_bytes]
            assert len(original_data) == data_bytes and original_data == packed_data
            output.write(packed_data)
            data_offset += data_bytes
            expected_lba = lba + extent_sectors
        tail = original.read((sectors - expected_lba) * sector_size)
        assert not any(tail)
        output.write(tail)
        assert not original.read(1)
    assert data_offset == len(payload)
    assert restored.stat().st_size == raw_size
    assert digest(source) == digest(restored)


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: check-install-image.py RAW_DISK_IMAGE")
    source = pathlib.Path(sys.argv[1])
    with tempfile.TemporaryDirectory(prefix="zinst-check-") as temp:
        packed = pathlib.Path(temp) / "install.img"
        restored = pathlib.Path(temp) / "restored.img"
        raw, compact, extents = module.pack_image(source, packed)
        verify(source, packed, restored)
        print(f"PASS sparse installer payload round-trip: {extents} extents, "
              f"{compact}/{raw} bytes, sha256={digest(restored).hex()}")


if __name__ == "__main__":
    main()
