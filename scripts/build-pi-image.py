#!/usr/bin/env python3
"""Build a Pi 4B raw image with a preserved UEFI/FAT boot volume and ZealOS data partition.

This writes only the output image. It never opens or edits a block device.
The firmware root must be a directory copied from a working pftf/RPi4 boot
volume; the ZealOS payload is overlaid onto that tree before FAT32 is built.
"""
from __future__ import annotations

import argparse
import hashlib
import os
import pathlib
import re
import shutil
import struct
import subprocess
import sys
import tempfile

SECTOR = 512
ALIGN_SECTORS = 2048  # 1 MiB
MBR_EFI_FAT32 = 0xEF
MBR_ZEAL_DATA = 0xDA
REDSEA_SIGNATURE = 0x88
REDSEA_DIR = 0x10
REDSEA_CONTIGUOUS = 0x400
REDSEA_BITS_PER_SECTOR = SECTOR * 8
REQUIRED_FIRMWARE = (
    "RPI_EFI.fd",
    "start4.elf",
    "fixup4.dat",
    "bcm2711-rpi-4-b.dtb",
    "config.txt",
)
REQUIRED_PAYLOAD = (
    "EFI/BOOT/BOOTAA64.EFI",
    "boot/zealbooter.elf",
    "boot/kernel.elf",
    "boot/kernel-diag.elf",
    "boot/demo.hcbc",
    "boot/sources.tar",
    "limine.conf",
)
MACOS_METADATA = {".ds_store", ".spotlight-v100", ".trashes", ".fseventsd"}


def align_up(value: int, alignment: int) -> int:
    return (value + alignment - 1) // alignment * alignment


def mbr_partition(status: int, kind: int, first: int, count: int) -> bytes:
    if not 0 <= first <= 0xFFFFFFFF or not 0 < count <= 0xFFFFFFFF:
        raise ValueError("MBR partition extent exceeds 32-bit LBA fields")
    if first + count > 0x100000000:
        raise ValueError("MBR partition end exceeds 32-bit LBA address space")
    # CHS is deliberately saturated; all supported tools use the LBA fields.
    return struct.pack("<B3sB3sII", status, b"\xfe\xff\xff", kind,
                       b"\xfe\xff\xff", first, count)


def make_mbr(total_sectors: int, boot_start: int, boot_count: int,
             data_start: int, data_count: int) -> bytes:
    if total_sectors > 0xFFFFFFFF:
        raise ValueError("Pi image is too large for the selected MBR layout")
    if boot_start + boot_count > data_start or data_start + data_count != total_sectors:
        raise ValueError("partition extents do not fit the requested image")
    sector = bytearray(SECTOR)
    table = 446
    sector[table:table + 16] = mbr_partition(0, MBR_EFI_FAT32,
                                             boot_start, boot_count)
    sector[table + 16:table + 32] = mbr_partition(0, MBR_ZEAL_DATA,
                                                  data_start, data_count)
    sector[510:512] = b"\x55\xaa"
    return bytes(sector)


def source_store_layout(root: pathlib.Path) -> dict[str, int]:
    layout = (root / "src/disk_layout.h").read_text()
    values: dict[str, int] = {}
    for name in ("ZEAL_RS_SECTS", "ZEAL_SRC_SECTS", "ZEAL_SRC_SHADOW_SECTS"):
        match = re.search(rf"^#define\s+{name}\s+(\d+)(?:ull|u|ll)?\s*$",
                          layout, re.MULTILINE | re.IGNORECASE)
        if not match:
            raise ValueError(f"could not read {name} from src/disk_layout.h")
        values[name] = int(match.group(1))
    return values


def source_store_min_sectors(root: pathlib.Path) -> int:
    return sum(source_store_layout(root).values())


def initialize_source_banks(image: pathlib.Path, data_start: int,
                            data_count: int, layout: dict[str, int]) -> None:
    """Initialize both empty source-bank headers inside the ZealOS partition."""
    source_a = layout["ZEAL_SRC_SECTS"]
    source_b = layout["ZEAL_SRC_SHADOW_SECTS"]
    redsea = data_count - source_a - source_b
    if redsea < layout["ZEAL_RS_SECTS"]:
        raise ValueError("ZealOS data partition is too small for RedSea and source banks")
    for first_lba, sectors in ((data_start + redsea, source_a),
                               (data_start + redsea + source_a, source_b)):
        header = bytearray(SECTOR)
        header[:8] = b"ZCSRC001"
        struct.pack_into("<II", header, 8, 1, sectors)
        with image.open("r+b") as stream:
            stream.seek(first_lba * SECTOR)
            stream.write(header)


def redsea_layout(data_count: int, layout: dict[str, int]) -> dict[str, int]:
    source_sectors = (layout["ZEAL_SRC_SECTS"] +
                      layout["ZEAL_SRC_SHADOW_SECTS"])
    redsea_sectors = data_count - source_sectors
    if redsea_sectors < layout["ZEAL_RS_SECTS"]:
        raise ValueError("ZealOS data partition is too small for RedSea and source banks")
    # RedSea uses one bitmap bit per 512-byte sector. The bitmap itself is
    # stored at LBA 1, and the data area starts after all bitmap sectors.
    bitmap_sectors = (redsea_sectors + REDSEA_BITS_PER_SECTOR - 1) // REDSEA_BITS_PER_SECTOR
    root_cluster = bitmap_sectors + 1  # cluster 0 is reserved by RedSeaFormat
    if bitmap_sectors < 1 or root_cluster >= redsea_sectors:
        raise ValueError("ZealOS data partition is too small for a RedSea root directory")
    return {
        "redsea_sectors": redsea_sectors,
        "bitmap_sectors": bitmap_sectors,
        "root_cluster": root_cluster,
    }


def initialize_redsea(image: pathlib.Path, data_start: int,
                       data_count: int, layout: dict[str, int]) -> None:
    """Write the pinned ZealOS RedSea boot sector, bitmap, and empty root."""
    geometry = redsea_layout(data_count, layout)
    redsea_sectors = geometry["redsea_sectors"]
    bitmap_sectors = geometry["bitmap_sectors"]
    root_cluster = geometry["root_cluster"]

    boot = bytearray(SECTOR)
    boot[3] = REDSEA_SIGNATURE
    # RedSea stores this field as an I64. Keep the random identifier positive
    # so struct packing matches the on-disk signed field for every build.
    unique_id = int.from_bytes(os.urandom(8), "little") & ((1 << 63) - 1)
    if not unique_id:
        unique_id = 1
    struct.pack_into("<qqqqq", boot, 8, 0, redsea_sectors, root_cluster,
                     bitmap_sectors, unique_id)
    boot[510:512] = b"\x55\xaa"

    # Match RedSeaFormat: reserve the first data cluster, then allocate the
    # root directory in the next cluster. Both bits are relative to data_area.
    bitmap_first = bytearray(SECTOR)
    bitmap_first[0] = 0x03

    root = bytearray(SECTOR)
    entry_attr = REDSEA_DIR | REDSEA_CONTIGUOUS
    struct.pack_into("<H38sQQQ", root, 0, entry_attr, b".\0" + bytes(36),
                     root_cluster, SECTOR, 0)
    struct.pack_into("<H38sQQQ", root, 64, entry_attr, b"..\0" + bytes(35),
                     root_cluster, 0, 0)

    with image.open("r+b") as stream:
        stream.seek(data_start * SECTOR)
        stream.write(boot)
        stream.seek((data_start + 1) * SECTOR)
        stream.write(bitmap_first)
        stream.seek((data_start + root_cluster) * SECTOR)
        stream.write(root)


def verify_redsea_image(image: pathlib.Path, data_start: int,
                        data_count: int,
                        layout: dict[str, int]) -> None:
    geometry = redsea_layout(data_count, layout)
    redsea_sectors = geometry["redsea_sectors"]
    bitmap_sectors = geometry["bitmap_sectors"]
    root_cluster = geometry["root_cluster"]
    with image.open("rb") as stream:
        stream.seek(data_start * SECTOR)
        boot = stream.read(SECTOR)
        if (len(boot) != SECTOR or boot[3] != REDSEA_SIGNATURE or
                boot[510:512] != b"\x55\xaa"):
            raise ValueError("raw image RedSea boot-sector signature mismatch")
        drv_offset, sectors, root, bitmap, _unique_id = struct.unpack_from(
            "<qqqqq", boot, 8)
        if (drv_offset != 0 or sectors != redsea_sectors or
                root != root_cluster or bitmap != bitmap_sectors or
                root <= bitmap or root >= sectors):
            raise ValueError("raw image RedSea geometry mismatch")

        stream.seek((data_start + 1) * SECTOR)
        allocation = stream.read(SECTOR)
        if len(allocation) != SECTOR or allocation[0] & 0x03 != 0x03:
            raise ValueError("raw image RedSea reserved/root bitmap bits are missing")

        stream.seek((data_start + root_cluster) * SECTOR)
        directory = stream.read(SECTOR)
        if len(directory) != SECTOR:
            raise ValueError("raw image RedSea root directory is truncated")
        dot_attr, dot_name, dot_cluster, dot_size, _dot_time = struct.unpack_from(
            "<H38sQQQ", directory, 0)
        dotdot_attr, dotdot_name, dotdot_cluster, _dotdot_size, _dotdot_time = struct.unpack_from(
            "<H38sQQQ", directory, 64)
        if (dot_attr != (REDSEA_DIR | REDSEA_CONTIGUOUS) or
                dot_name.split(b"\0", 1)[0] != b"." or
                dot_cluster != root_cluster or dot_size != SECTOR or
                dotdot_attr != (REDSEA_DIR | REDSEA_CONTIGUOUS) or
                dotdot_name.split(b"\0", 1)[0] != b".." or
                dotdot_cluster != root_cluster or any(directory[128:])):
            raise ValueError("raw image RedSea root directory verification failed")


def verify_source_banks(image: pathlib.Path, data_start: int,
                        data_count: int, layout: dict[str, int]) -> None:
    source_a = layout["ZEAL_SRC_SECTS"]
    source_b = layout["ZEAL_SRC_SHADOW_SECTS"]
    redsea = data_count - source_a - source_b
    expected = ((data_start + redsea, source_a),
                (data_start + redsea + source_a, source_b))
    with image.open("rb") as stream:
        for first_lba, sectors in expected:
            stream.seek(first_lba * SECTOR)
            header = stream.read(SECTOR)
            if (len(header) != SECTOR or header[:8] != b"ZCSRC001" or
                    struct.unpack_from("<II", header, 8) != (1, sectors)):
                raise ValueError("raw image source-bank header verification failed")


def overlay_tree(source: pathlib.Path, destination: pathlib.Path) -> None:
    """Copy a regular-file tree, preserving firmware files and overlaying payload."""
    if not source.is_dir():
        raise ValueError(f"input directory does not exist: {source}")
    destination.mkdir(parents=True, exist_ok=True)
    for path in sorted(source.rglob("*")):
        rel = path.relative_to(source)
        if any(part.casefold() in MACOS_METADATA or part.startswith("._")
               for part in rel.parts):
            continue
        target = destination / rel
        if path.is_symlink():
            raise ValueError(f"symlinks are not supported in boot input: {path}")
        if path.is_dir():
            target.mkdir(parents=True, exist_ok=True)
        elif path.is_file():
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)


def require_files(root: pathlib.Path, names: tuple[str, ...], label: str) -> None:
    missing = [name for name in names if not (root / name).is_file()]
    if missing:
        raise ValueError(f"{label} is missing required files: {', '.join(missing)}")


def mtools_copy_tree(fat_image: pathlib.Path, tree: pathlib.Path,
                     mmd: str, mcopy: str) -> None:
    directories = [path.relative_to(tree) for path in tree.rglob("*")
                   if path.is_dir()]
    for rel in sorted(directories, key=lambda item: (len(item.parts), item.as_posix().casefold())):
        dest = "::/" + rel.as_posix()
        subprocess.run([mmd, "-i", str(fat_image), dest], check=True,
                       stdout=subprocess.DEVNULL)
    for path in sorted((p for p in tree.rglob("*") if p.is_file()),
                       key=lambda item: item.relative_to(tree).as_posix().casefold()):
        rel = path.relative_to(tree).as_posix()
        subprocess.run([mcopy, "-i", str(fat_image), str(path), "::/" + rel],
                       check=True, stdout=subprocess.DEVNULL)


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def verify_fat_payload(fat_image: pathlib.Path, tree: pathlib.Path,
                       mcopy: str) -> None:
    with tempfile.TemporaryDirectory(prefix="zealos-pi-fat-check-") as temp_name:
        temp = pathlib.Path(temp_name)
        for index, rel in enumerate(REQUIRED_FIRMWARE + REQUIRED_PAYLOAD):
            extracted = temp / f"readback-{index}"
            subprocess.run([mcopy, "-i", str(fat_image), "::/" + rel,
                            str(extracted)], check=True,
                           stdout=subprocess.DEVNULL)
            if sha256(extracted) != sha256(tree / rel):
                raise ValueError(f"FAT32 read-back mismatch: {rel}")


def verify_image_layout(image: pathlib.Path, total_sectors: int,
                        boot_start: int, boot_count: int,
                        data_start: int, data_count: int) -> None:
    with image.open("rb") as stream:
        mbr = stream.read(SECTOR)
        if len(mbr) != SECTOR or mbr[510:512] != b"\x55\xaa":
            raise ValueError("raw image MBR signature is missing")
        first = struct.unpack_from("<B3sB3sII", mbr, 446)
        second = struct.unpack_from("<B3sB3sII", mbr, 462)
        if first[2] != MBR_EFI_FAT32 or first[4:] != (boot_start, boot_count):
            raise ValueError("raw image FAT32 EFI partition entry mismatch")
        if second[2] != MBR_ZEAL_DATA or second[4:] != (data_start, data_count):
            raise ValueError("raw image ZealOS data partition entry mismatch")
        if boot_start + boot_count > data_start or data_start + data_count != total_sectors:
            raise ValueError("raw image partition ranges overlap or do not cover the image")
        stream.seek(boot_start * SECTOR)
        boot = stream.read(SECTOR)
        if (boot[510:512] != b"\x55\xaa" or
                struct.unpack_from("<H", boot, 11)[0] != SECTOR or
                boot[13] == 0 or boot[13] & (boot[13] - 1) or
                struct.unpack_from("<I", boot, 36)[0] == 0 or
                boot[82:90] != b"FAT32   "):
            raise ValueError("raw image first partition is not a valid FAT32 boot volume")


def copy_partition(image: pathlib.Path, fat_image: pathlib.Path,
                   first_lba: int) -> None:
    with image.open("r+b") as output, fat_image.open("rb") as source:
        output.seek(first_lba * SECTOR)
        shutil.copyfileobj(source, output, 8 * 1024 * 1024)


def build(args: argparse.Namespace) -> None:
    root = pathlib.Path(__file__).resolve().parent.parent
    firmware = args.firmware_root.resolve()
    payload = args.payload.resolve()
    output_arg = args.output.expanduser()
    if output_arg.is_symlink():
        raise ValueError("output must be a regular image file path, not a device or symlink")
    output = output_arg.resolve()
    if (output == firmware or output == payload or firmware in output.parents or
            payload in output.parents):
        raise ValueError("output must not be inside either input tree")
    if output.exists() and not output.is_file():
        raise ValueError("output must be a regular image file path, not a device or symlink")
    if output.exists() and not args.force:
        raise ValueError(f"output already exists (pass --force to replace): {output}")
    require_files(firmware, REQUIRED_FIRMWARE, "firmware root")
    require_files(payload, REQUIRED_PAYLOAD, "Pi payload")

    check_image = root / "scripts/check-pi-desktop-image.py"
    subprocess.run([sys.executable, os.fspath(check_image), os.fspath(payload)], check=True)

    mkfs = shutil.which(args.mkfs_fat)
    mcopy = shutil.which(args.mcopy)
    mmd = shutil.which(args.mmd)
    if not mkfs or not mcopy or not mmd:
        raise ValueError("FAT32 tools missing; install dosfstools (mkfs.fat) and mtools (mcopy, mmd)")

    total_sectors = args.size_mib * 1024 * 1024 // SECTOR
    if args.size_mib <= 0:
        raise ValueError("image size must be positive")
    boot_start = ALIGN_SECTORS
    if args.boot_mib <= 0:
        raise ValueError("boot partition size must be positive")
    boot_count = args.boot_mib * 1024 * 1024 // SECTOR
    data_start = align_up(boot_start + boot_count, ALIGN_SECTORS)
    data_count = total_sectors - data_start
    source_layout = source_store_layout(root)
    required_data = sum(source_layout.values())
    if args.size_mib * 1024 * 1024 % SECTOR or args.boot_mib * 1024 * 1024 % SECTOR:
        raise ValueError("image and boot sizes must be multiples of 512 bytes")
    if args.boot_mib < 512:
        raise ValueError("boot FAT32 partition must be at least 512 MiB")
    if data_count < required_data:
        raise ValueError(f"data partition needs at least {required_data} sectors for current RedSea/source layout")
    if total_sectors > 0xFFFFFFFF:
        raise ValueError("requested image exceeds 2 TiB MBR limit")

    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="zealos-pi-image-") as temp_name:
        temp = pathlib.Path(temp_name)
        merged = temp / "boot-root"
        overlay_tree(firmware, merged)
        overlay_tree(payload, merged)
        require_files(merged, REQUIRED_FIRMWARE + REQUIRED_PAYLOAD,
                      "merged boot root")

        boot_image = temp / "boot-fat32.img"
        with boot_image.open("wb") as image:
            image.truncate(boot_count * SECTOR)
        subprocess.run([mkfs, "-F", "32", "-S", str(SECTOR), "-n", "ZEALBOOT",
                        str(boot_image)], check=True)
        mtools_copy_tree(boot_image, merged, mmd, mcopy)
        verify_fat_payload(boot_image, merged, mcopy)

        mbr = make_mbr(total_sectors, boot_start, boot_count,
                       data_start, data_count)
        # Always start with a fresh zero-filled image. In particular, --force
        # must not preserve stale sectors from an older image of the same size.
        with output.open("w+b") as image:
            image.truncate(total_sectors * SECTOR)
            image.seek(0)
            image.write(mbr)
        copy_partition(output, boot_image, boot_start)
        initialize_redsea(output, data_start, data_count, source_layout)
        initialize_source_banks(output, data_start, data_count, source_layout)
        verify_image_layout(output, total_sectors, boot_start, boot_count,
                            data_start, data_count)
        verify_redsea_image(output, data_start, data_count, source_layout)
        verify_source_banks(output, data_start, data_count, source_layout)
        if output.stat().st_size != total_sectors * SECTOR:
            raise ValueError("raw image file size changed during assembly")

    print(f"Pi 4B image written: {output}")
    print(f"  size: {args.size_mib} MiB ({total_sectors} sectors)")
    print(f"  FAT32 boot: LBA {boot_start} + {boot_count} sectors")
    print(f"  ZealOS data (MBR type 0xDA): LBA {data_start} + {data_count} sectors")
    print("  ZealOS data: empty full-size RedSea volume and both source banks initialized")
    print("  `psdopen` binds this partition at runtime; ordinary full RedSea writes are not yet enabled")
    print("  no physical disk or microSD card was opened")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--firmware-root", required=True, type=pathlib.Path,
                        help="directory copied from a working pftf/RPi4 FAT boot volume")
    parser.add_argument("--payload", type=pathlib.Path,
                        default=pathlib.Path("build/pi-desktop-esp"),
                        help="staged Pi desktop payload directory")
    parser.add_argument("--output", required=True, type=pathlib.Path,
                        help="new raw .img output path; never a block device")
    parser.add_argument("--size-mib", type=int, required=True,
                        help="target image size in MiB (choose a size supported by the card)")
    parser.add_argument("--boot-mib", type=int, default=512,
                        help="FAT32 boot partition size in MiB (default: 512)")
    parser.add_argument("--mkfs-fat", default="mkfs.fat")
    parser.add_argument("--mcopy", default="mcopy")
    parser.add_argument("--mmd", default="mmd")
    parser.add_argument("--force", action="store_true",
                        help="replace an existing output image file")
    args = parser.parse_args()
    try:
        build(args)
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        parser.error(str(exc))


if __name__ == "__main__":
    main()
