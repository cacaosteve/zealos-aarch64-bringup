#!/usr/bin/env python3
"""Check note order in a QEMU WAV capture, not real-time playback latency."""
import struct
import sys
from pathlib import Path

path = Path(sys.argv[1])
data = path.read_bytes()
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
pcm = data[pos:]
if len(pcm) % 4:
    raise SystemExit('incomplete stereo frame')
samples = struct.unpack('<%dh' % (len(pcm) // 2), pcm)[::2]
rate = fmt[2]
window = rate // 20
labels = []
for start in range(0, len(samples) - window + 1, window):
    chunk = samples[start:start + window]
    # Count both transitions of a square wave; ignore quiet windows.
    if max(map(abs, chunk)) < 1000:
        continue
    crossings = sum((a < 0 <= b) or (a >= 0 > b)
                    for a, b in zip(chunk, chunk[1:]))
    hz = crossings * rate / (2 * window)
    label = ('A' if 350 <= hz <= 520 else
             'B' if 780 <= hz <= 980 else
             'C' if 570 <= hz <= 750 else '?')
    labels.append(label)

# Require at least two 50 ms windows at each expected pitch in order.
index = 0
for label in ('A', 'B', 'C'):
    while index < len(labels) and labels[index] != label:
        index += 1
    if index + 1 >= len(labels) or labels[index + 1] != label:
        raise SystemExit(f'missing sustained {label} note in order: {"".join(labels)}')
    index += 2
print(f'PASS {path}: A4, A5 and E5 present in order; windows={"".join(labels)}')
