#!/usr/bin/env python3
"""Seed and maintain files in the native source partition of a GPT image.

The format is a sector-aligned append log. Each record writes data first and
its header last, so an interrupted append cannot replace a previous version.
This is a bootstrap source store, not a general RedSea implementation.
"""
import argparse
import os
import pathlib
import shutil
import struct
import tarfile
import tempfile
import zlib

SECTOR = 512
BANK_A_START = 131039
BANK_A_SECTORS = 65536
BANK_B_START = 196575
BANK_B_SECTORS = 65536
START = BANK_A_START
SECTORS = BANK_A_SECTORS
SUPER_MAGIC = b'ZCSRC001'
FILE_MAGIC = b'ZCFILE1\0'
DELETE_MAGIC = b'ZCDEL1\0\0'
MAX_NAME = 95
MAX_FILE = 4 * 1024 * 1024


def valid_name(name):
    try:
        encoded = name.encode('ascii')
    except UnicodeEncodeError as exc:
        raise ValueError(f'source path must be printable ASCII: {name!r}') from exc
    parts = name.split('/')
    if not encoded or len(encoded) > MAX_NAME or any(
        not part or part in ('.', '..') for part in parts
    ) or any(byte < 33 or byte > 126 or byte in (ord(':'), ord('\\'))
             for byte in encoded):
        raise ValueError(f'invalid source path: {name!r}')
    return encoded


def at(f, sector):
    f.seek((START + sector) * SECTOR)


def probe(f, start, sectors):
    f.seek(start * SECTOR)
    header = f.read(SECTOR)
    if len(header) != SECTOR or struct.unpack_from('<I', header, 12)[0] != sectors:
        return None
    if header[:8] == SUPER_MAGIC and struct.unpack_from('<I', header, 8)[0] == 1:
        return 0
    if header[:8] == b'ZCSRC002' and struct.unpack_from('<I', header, 8)[0] == 2 \
            and struct.unpack_from('<I', header, 20)[0] == zlib.crc32(header[:20]):
        generation = struct.unpack_from('<I', header, 16)[0]
        return generation or None
    return None


def select_bank(f):
    global START, SECTORS
    a = probe(f, BANK_A_START, BANK_A_SECTORS)
    b = probe(f, BANK_B_START, BANK_B_SECTORS)
    if a is None and b is None:
        raise ValueError('source partition has no supported superblock')
    if b is not None and (a is None or b > a):
        START, SECTORS = BANK_B_START, BANK_B_SECTORS
        return 'B', b
    START, SECTORS = BANK_A_START, BANK_A_SECTORS
    return 'A', a


def scan(f, with_deleted=False):
    select_bank(f)
    at(f, 0)
    superblock = f.read(SECTOR)
    pos, files, deleted_names = 1, {}, set()
    while pos < SECTORS:
        at(f, pos)
        header = f.read(SECTOR)
        if header == bytes(SECTOR):
            break
        if len(header) != SECTOR or header[:8] not in (FILE_MAGIC, DELETE_MAGIC):
            raise ValueError(f'damaged record header at source sector {pos}')
        length, checksum, blocks, name_len = struct.unpack_from('<IIII', header, 8)
        deleted = header[:8] == DELETE_MAGIC
        valid_body = ((length, checksum, blocks) == (0, 0, 0) if deleted else
                      0 < length <= MAX_FILE and blocks == (length + 511) // 512)
        if not (valid_body and 0 < name_len <= MAX_NAME and
                pos + 1 + blocks <= SECTORS):
            raise ValueError(f'invalid record size at source sector {pos}')
        name = header[32:32 + name_len].decode('ascii')
        valid_name(name)
        if deleted:
            files.pop(name, None)
            deleted_names.add(name)
        else:
            files[name] = (pos, length, checksum)
            deleted_names.discard(name)
        pos += 1 + blocks
    return (pos, files, deleted_names) if with_deleted else (pos, files)


def append(f, pos, name, data):
    encoded = valid_name(name)
    if not data or len(data) > MAX_FILE:
        raise ValueError('source must contain 1 byte to 4 MiB')
    blocks = (len(data) + 511) // 512
    if pos + 1 + blocks > SECTORS:
        raise ValueError('source partition is full')
    at(f, pos + 1)
    f.write(data)
    f.write(bytes(blocks * SECTOR - len(data)))
    f.flush()
    if pos + 1 + blocks < SECTORS:
        at(f, pos + 1 + blocks)
        f.write(bytes(SECTOR))
        f.flush()
    header = bytearray(SECTOR)
    header[:8] = FILE_MAGIC
    struct.pack_into('<IIII', header, 8, len(data), zlib.crc32(data), blocks, len(encoded))
    header[32:32 + len(encoded)] = encoded
    at(f, pos)
    f.write(header)
    f.flush()
    return pos + 1 + blocks


def append_tombstone(f, pos, name):
    encoded = valid_name(name)
    if pos >= SECTORS:
        raise ValueError('source partition is full')
    if pos + 1 < SECTORS:
        at(f, pos + 1)
        f.write(bytes(SECTOR))
        f.flush()
    header = bytearray(SECTOR)
    header[:8] = DELETE_MAGIC
    struct.pack_into('<I', header, 20, len(encoded))
    header[32:32 + len(encoded)] = encoded
    at(f, pos)
    f.write(header)
    f.flush()
    return pos + 1


def compact(disk):
    # Build a replacement image in the same directory, then atomically swap it
    # into place. All partitions outside the native source region are copied.
    with disk.open('rb') as original:
        old_tail, files, deleted_names = scan(original, with_deleted=True)
        live = []
        for name, (sector, length, checksum) in sorted(files.items()):
            at(original, sector + 1)
            data = original.read(length)
            if len(data) != length or zlib.crc32(data) != checksum:
                raise ValueError(f'bad source checksum: {name}')
            live.append((name, data))
    fd, temp_name = tempfile.mkstemp(prefix=f'.{disk.name}.compact-',
                                     dir=disk.parent)
    os.close(fd)
    temp = pathlib.Path(temp_name)
    try:
        shutil.copy2(disk, temp)
        with temp.open('r+b') as target:
            at(target, 1)
            target.write(bytes((SECTORS - 1) * SECTOR))
            pos = 1
            for name, data in live:
                pos = append(target, pos, name, data)
            # A tombstone also masks a legacy RedSea file with the same name.
            # Retain the latest deletion when reclaiming old source records.
            for name in sorted(deleted_names):
                pos = append_tombstone(target, pos, name)
            target.flush()
            os.fsync(target.fileno())
        os.replace(temp, disk)
    finally:
        temp.unlink(missing_ok=True)
    count = len(deleted_names)
    print(f'Source partition: compacted {len(live)} files, retained '
          f'{count} deleted {"path" if count == 1 else "paths"}, '
          f'{old_tail} -> {pos}/{SECTORS} sectors used')


def merge(disk, bundle_path, replace_names=()):
    """Append missing files and explicitly selected refreshed bundle files."""
    replace_names = set(replace_names)
    with disk.open('r+b') as f:
        pos, files, deleted_names = scan(f, with_deleted=True)
        added = replaced = 0
        with tarfile.open(bundle_path) as bundle:
            members = sorted((m for m in bundle.getmembers()
                              if m.isfile() and m.name.endswith(('.ZC', '.HH'))),
                             key=lambda m: m.name)
            for member in members:
                name = member.name
                valid_name(name)
                # Existing records may contain local edits. Tombstones are
                # deliberate masks and must not be undone unless a caller
                # explicitly marks a repository-owned file for refresh.
                refresh = name in replace_names
                if (name in files or name in deleted_names) and not refresh:
                    continue
                data = bundle.extractfile(member).read()
                checksum = zlib.crc32(data)
                if refresh and name in files:
                    _, old_length, old_checksum = files[name]
                    if old_length == len(data) and old_checksum == checksum:
                        continue
                record_pos = pos
                pos = append(f, pos, name, data)
                files[name] = (record_pos, len(data), checksum)
                deleted_names.discard(name)
                if refresh:
                    replaced += 1
                else:
                    added += 1
        f.flush()
        os.fsync(f.fileno())
    print(f'Source partition: merged {added} new files, refreshed {replaced}; '
          f'{len(files)} live, {len(deleted_names)} deleted, '
          f'{pos}/{SECTORS} sectors used')


p = argparse.ArgumentParser(description=__doc__)
sub = p.add_subparsers(dest='command', required=True)
seed = sub.add_parser('seed', help='format and seed a newly built GPT image')
seed.add_argument('disk', type=pathlib.Path)
seed.add_argument('bundle', type=pathlib.Path)
put = sub.add_parser('put', help='append a host file to an existing image (VM stopped)')
put.add_argument('disk', type=pathlib.Path)
put.add_argument('source', type=pathlib.Path)
put.add_argument('name', help='guest path, e.g. Kernel/Module.ZC')
inspect = sub.add_parser('inspect', help='list the latest files in an image')
inspect.add_argument('disk', type=pathlib.Path)
delete = sub.add_parser('delete', help='hide a native source path (VM stopped)')
delete.add_argument('disk', type=pathlib.Path)
delete.add_argument('name')
repack = sub.add_parser('compact', help='reclaim old source records (VM stopped)')
repack.add_argument('disk', type=pathlib.Path)
merge_parser = sub.add_parser('merge', help='add missing bundled files; optionally refresh selected files')
merge_parser.add_argument('disk', type=pathlib.Path)
merge_parser.add_argument('bundle', type=pathlib.Path)
merge_parser.add_argument('--replace', action='append', default=[],
                          help='refresh this bundled source path even if it exists or is deleted')
args = p.parse_args()
if args.disk.stat().st_size < (START + SECTORS) * SECTOR:
    p.error('disk image is too small for the source partition')

if args.command == 'compact':
    compact(args.disk)
elif args.command == 'merge':
    merge(args.disk, args.bundle, args.replace)
else:
    with args.disk.open('r+b' if args.command != 'inspect' else 'rb') as f:
        if args.command == 'seed':
            START, SECTORS = BANK_A_START, BANK_A_SECTORS
            # Formatting an existing image must not leave a newer shadow bank
            # selected over the freshly seeded primary bank.
            f.seek(BANK_B_START * SECTOR)
            f.write(bytes(SECTOR))
            at(f, 0)
            superblock = bytearray(SECTOR)
            superblock[:8] = SUPER_MAGIC
            struct.pack_into('<II', superblock, 8, 1, SECTORS)
            f.write(superblock)
            pos = 1
            with tarfile.open(args.bundle) as bundle:
                members = sorted((m for m in bundle.getmembers()
                                  if m.isfile() and m.name.endswith(('.ZC', '.HH'))),
                                 key=lambda m: m.name)
                for member in members:
                    pos = append(f, pos, member.name, bundle.extractfile(member).read())
            print(f'Source partition: seeded {len(members)} files, {pos}/{SECTORS} sectors used')
        elif args.command == 'put':
            pos, files = scan(f)
            pos = append(f, pos, args.name, args.source.read_bytes())
            print(f'Source partition: saved {args.name}; {len(files) + (args.name not in files)} names, {pos}/{SECTORS} sectors used')
        elif args.command == 'delete':
            pos, files = scan(f)
            if args.name not in files:
                raise ValueError(f'missing native source: {args.name}')
            pos = append_tombstone(f, pos, args.name)
            print(f'Source partition: deleted {args.name}; {len(files)-1} names, {pos}/{SECTORS} sectors used')
        else:
            pos, files = scan(f)
            bank, generation = select_bank(f)
            print(f'Source partition: bank {bank} generation {generation}, '
                  f'{len(files)} names, {pos}/{SECTORS} sectors used')
            for name, (_, length, checksum) in sorted(files.items()):
                print(f'{name}\t{length}\tcrc32={checksum:08x}')
