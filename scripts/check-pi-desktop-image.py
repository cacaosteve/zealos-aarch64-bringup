#!/usr/bin/env python3
"""Validate the staged Pi native/runtime/recovery boot chain, without card IO."""
import argparse
import pathlib
import struct
import tarfile


def elf(path):
    data = path.read_bytes()
    assert data[:6] == b'\x7fELF\x02\x01', f'{path}: expected little-endian ELF64'
    header = struct.unpack_from('<HHIQQQIHHHHHH', data, 16)
    kind, machine, _, entry, phoff, shoff, _, _, phent, phnum, shent, shnum, _ = header
    assert kind == 3 and machine == 183, f'{path}: expected AArch64 PIE'
    assert phent == 56 and shent == 64
    loads = []
    for i in range(phnum):
        typ, flags, off, va, _, size, memory, _ = struct.unpack_from('<IIQQQQQQ', data, phoff+i*phent)
        if typ == 1:
            assert size <= memory and off+size <= len(data)
            loads.append((va, va+memory, flags))
    assert any(lo <= entry < hi and flags & 1 for lo, hi, flags in loads)
    assert max(hi for _, hi, _ in loads) < 512*1024*1024, 'oversized Pi kernel span'
    sections = [struct.unpack_from('<IIQQQQIIQQ', data, shoff+i*shent) for i in range(shnum)]
    names = set()
    for s in sections:
        _, typ, _, _, off, size, link, _, _, ent = s
        if typ == 2:
            strings = sections[link]
            table = data[strings[4]:strings[4]+strings[5]]
            assert ent == 24 and off+size <= len(data)
            for pos in range(off, off+size, ent):
                name, _, _, index, _, _ = struct.unpack_from('<IBBHQQ', data, pos)
                if index and name:
                    end = table.index(0, name)
                    names.add(table[name:end].decode())
        if typ == 4 and size:
            assert ent == 24 and off+size <= len(data)
            for pos in range(off, off+size, ent):
                target, info, _ = struct.unpack_from('<QQq', data, pos)
                assert info == 1027, f'{path}: unsupported booter relocation {info}'
                assert any(lo <= target and target+8 <= hi for lo, hi, _ in loads)
    return data, names


def check(root):
    full, symbols = elf(root/'boot/kernel.elf')
    required = {'zc_init', 'zc_start_graphics', 'zc_start_winmgr',
                'zc_start_gui_terminal', 'pi4_usb_bus_poll', 'input_poll', 'zc_key_read'}
    assert required <= symbols, f'native Pi kernel missing {required-symbols}'
    assert b'PI-DESKTOP-2' in full
    recovery, symbols = elf(root/'boot/kernel-diag.elf')
    assert 'pi4_usb_bus_poll' in symbols and 'zc_init' not in symbols
    assert b'MULTI-USB2-6' in recovery
    assert full != recovery, 'desktop and recovery must be distinct images'
    config = (root/'limine.conf').read_text()
    preamble, desktop, diagnostic = config.split('\n/')[0:3]
    assert config.count('\n/') == 2 and 'default_entry: 1' in preamble
    assert 'timeout: 5' in preamble
    for text in (desktop, diagnostic):
        assert 'cmdline: zeal.plat=pi4' in text
        assert 'dtb_path: boot():/bcm2711-rpi-4-b.dtb' in text
        assert 'path: boot():/boot/zealbooter.elf' in text
        assert 'kaslr: no' in text and 'resolution: 800x600x32' in text
    def modules(text):
        return [line.strip().split('boot():/', 1)[1] for line in text.splitlines()
                if line.strip().startswith('module_path:')]
    assert modules(desktop) == ['boot/kernel.elf', 'boot/demo.hcbc', 'boot/sources.tar']
    assert modules(diagnostic) == ['boot/kernel-diag.elf', 'boot/demo.hcbc']
    with tarfile.open(root/'boot/sources.tar') as archive:
        members = archive.getmembers()
        assert all(m.isfile() and not m.name.startswith('/') and '..' not in pathlib.PurePosixPath(m.name).parts
                   for m in members)
        paths = {m.name for m in members}
        assert {'Kernel/KernelA.HH', 'Kernel/KernelB.HH', 'System/WinMgr.ZC',
                'System/GuiTerminal.ZC', 'System/TaskBridge.ZC', 'System/ZSplash.ZC',
                'HomeSys.ZC', 'Home/Once.ZC'} <= paths
    for name in ('boot/zealbooter.elf', 'boot/demo.hcbc', 'EFI/BOOT/BOOTAA64.EFI'):
        assert (root/name).stat().st_size > 0
    return len(members)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=pathlib.Path, nargs='?', default=pathlib.Path('build/pi-desktop-esp'))
    args = parser.parse_args()
    count = check(args.root)
    print(f'PASS Pi native/recovery ELF identity, load bounds/relocations, two-entry boot chain and {count} source modules')
