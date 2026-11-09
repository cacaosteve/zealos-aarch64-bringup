#!/usr/bin/env python3
"""Check captured QEMU WAV PCM for actual sound-effect samples.

QEMU's streaming WAV backend can leave the RIFF and data lengths at zero,
so use the file length after locating its data chunk.
"""
import argparse
import struct
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--minimum-seconds', type=float, default=0.1,
                    help='minimum duration of nonzero stereo PCM')
parser.add_argument('captures', nargs='+', type=Path)
args = parser.parse_args()

for path in args.captures:
    data = path.read_bytes()
    if data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        raise SystemExit(f'{path}: invalid WAV header')
    pos = 12
    fmt = None
    while pos + 8 <= len(data):
        tag, size = struct.unpack_from('<4sI', data, pos)
        pos += 8
        if tag == b'fmt ':
            if size < 16 or pos + size > len(data):
                raise SystemExit(f'{path}: invalid fmt chunk')
            fmt = struct.unpack_from('<HHIIHH', data, pos)
        if tag == b'data':
            break
        pos += size + (size & 1)
    else:
        raise SystemExit(f'{path}: no PCM data')
    if not fmt or fmt[0] != 1 or fmt[1] != 2 or fmt[5] != 16:
        raise SystemExit(f'{path}: expected 16-bit stereo PCM, got {fmt}')
    pcm = data[pos:]
    if len(pcm) % 4:
        raise SystemExit(f'{path}: incomplete stereo frame')
    samples = struct.unpack('<%dh' % (len(pcm) // 2), pcm)
    nonzero = sum(value != 0 for value in samples)
    peak = max((abs(value) for value in samples), default=0)
    if (nonzero < int(fmt[2] * 2 * args.minimum_seconds) or peak < 1000 or
            not any(v < 0 for v in samples) or not any(v > 0 for v in samples)):
        raise SystemExit(f'{path}: insufficient audible PCM ({nonzero=}, {peak=})')
    print(f'PASS {path}: {nonzero} nonzero samples at {fmt[2]} Hz, peak {peak}')
