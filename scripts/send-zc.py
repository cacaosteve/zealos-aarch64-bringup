#!/usr/bin/env python3
"""Send a .ZC/.HH file to a running guest's `zrecv` command.

Use --serial with a 115200-baud USB/PTY serial device. With --emit, print the
command and hex chunks for pasting into UTM Terminal 1 one line at a time;
wait for each `zrecv: ack N` before pasting the next chunk.
"""
import argparse
import os
import pathlib
import select
import sys
import termios
import time
import tty
import zlib

CHUNK = 128
MAX_FILE = 4 * 1024 * 1024


def valid_name(name):
    raw = name.encode('ascii')
    parts = name.split('/')
    if not raw or len(raw) > 95 or any(not p or p in ('.', '..') for p in parts) or any(
        c < 33 or c > 126 or c in (ord(':'), ord('\\')) for c in raw
    ):
        raise ValueError('guest name must be a printable source path of at most 95 bytes')


def write_all(fd, data):
    while data:
        written = os.write(fd, data)
        data = data[written:]


def wait_for(fd, pending, marker, timeout=30):
    deadline = time.monotonic() + timeout
    errors = (b'zrecv: timeout', b'zrecv: malformed', b'zrecv: disk write failed',
              b'zrecv: checksum mismatch')
    while True:
        if any(error in pending for error in errors):
            raise RuntimeError(pending[-300:].decode(errors='replace'))
        if marker in pending:
            break
        remain = deadline - time.monotonic()
        if remain <= 0 or not select.select([fd], [], [], remain)[0]:
            raise TimeoutError(f'waiting for {marker!r}; received {pending[-300:]!r}')
        data = os.read(fd, 4096)
        if not data:
            raise EOFError('serial port closed')
        pending.extend(data)
    end = pending.index(marker) + len(marker)
    result = bytes(pending[:end])
    del pending[:end]
    return result


def send(fd, command, chunks, name, length):
    pending = bytearray()
    write_all(fd, command + b'\r')
    wait_for(fd, pending, b' chunk=128\r\n')
    for offset, chunk in chunks:
        write_all(fd, chunk + b'\r')
        wait_for(fd, pending, f'zrecv: ack {offset}\r\n'.encode())
        print(f'{offset}/{length} bytes acknowledged', file=sys.stderr)
    wait_for(fd, pending, f'zrecv: saved {name} bytes={length}\r\n'.encode())
    print(f'Saved {name} ({length} bytes)', file=sys.stderr)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=pathlib.Path)
    p.add_argument('name', help='guest path, e.g. Home/Module.ZC')
    mode = p.add_mutually_exclusive_group(required=True)
    mode.add_argument('--serial', help='serial device or UTM PTY path')
    mode.add_argument('--emit', action='store_true', help='print pasteable lines for UTM Terminal 1')
    args = p.parse_args()
    try:
        valid_name(args.name)
        data = args.source.read_bytes()
        if not data or len(data) > MAX_FILE or b'\0' in data:
            raise ValueError('source must contain 1 byte to 4 MiB and no NUL bytes')
    except (OSError, UnicodeError, ValueError) as exc:
        p.error(str(exc))
    command = f'zrecv {args.name} {len(data)} {zlib.crc32(data):08x}'.encode()
    chunks = [(min(i + CHUNK, len(data)), data[i:i + CHUNK].hex().encode())
              for i in range(0, len(data), CHUNK)]
    if args.emit:
        print('Paste the first line at the guest prompt, then each hex line after its ack.',
              file=sys.stderr)
        print(command.decode())
        for _, chunk in chunks:
            print(chunk.decode())
        return
    fd = os.open(args.serial, os.O_RDWR | os.O_NOCTTY)
    old = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        attrs = termios.tcgetattr(fd)
        attrs[4] = attrs[5] = termios.B115200
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        termios.tcflush(fd, termios.TCIFLUSH)
        send(fd, command, chunks, args.name, len(data))
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, old)
        os.close(fd)


if __name__ == '__main__':
    main()
