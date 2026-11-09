#!/usr/bin/env python3
"""Check for Psalmody's default-octave H-key C5 in a QEMU WAV capture."""
import struct
import sys
from pathlib import Path

data = Path(sys.argv[1]).read_bytes()
if data[:4] != b'RIFF' or data[8:12] != b'WAVE':
    raise SystemExit('invalid WAV header')
pos, fmt = 12, None
while pos + 8 <= len(data):
    tag, size = struct.unpack_from('<4sI', data, pos)
    pos += 8
    if tag == b'fmt ':
        if size < 16 or pos + size > len(data):
            raise SystemExit('invalid WAV format chunk')
        fmt = struct.unpack_from('<HHIIHH', data, pos)
    if tag == b'data':
        break
    pos += size + (size & 1)
else:
    raise SystemExit('missing PCM data')
if not fmt or fmt[0] != 1 or fmt[1] != 2 or fmt[5] != 16:
    raise SystemExit(f'expected 16-bit stereo PCM: {fmt}')
# QEMU may leave the data-size field zero when the test terminates it.
pcm = data[pos:]
if len(pcm) % 4:
    raise SystemExit('incomplete stereo frame')
samples = struct.unpack('<%dh' % (len(pcm) // 2), pcm)[::2]
rate = fmt[2]
window = rate // 20
matching = 0
for start in range(0, len(samples) - window + 1, window):
    chunk = samples[start:start + window]
    if sum(abs(sample) >= 1000 for sample in chunk) < window * 3 // 4:
        continue
    crossings = sum((a < 0 <= b) or (a >= 0 > b)
                    for a, b in zip(chunk, chunk[1:]))
    hz = crossings * rate / (2 * window)
    if 500 <= hz <= 550:
        matching += 1
if matching < 2:
    raise SystemExit(f'C5 not sustained: {matching} matching 50 ms windows')
print(f'PASS {sys.argv[1]}: C5 present in {matching} 50 ms windows')
