#!/usr/bin/env python3
"""Verify the guarded AArch64 source overlay for pinned GrScreen.ZC."""
from pathlib import Path
import sys

from arm64_source import translate_grscreen

root = Path(__file__).resolve().parent.parent
source = (root / 'upstream/pinned/System/Gr/GrScreen.ZC').read_bytes()
translated = translate_grscreen(source)
text = translated.decode('utf-8')
assert translated != source
assert text.count('*row_dst(U64 *) = color;') == 1
assert 'U8 *ZcGrTextBGStore(U8 *dst, I64 stride, U64 color)' in text
assert 'for (row = 0; row < 8; row++)' in text
assert 'if (row < 7)' in text
assert 'row_dst += stride;' in text
assert 'dst = ZcGrTextBGStore(dst(U8 *), w1, c);' in text
assert 'dst(U8 *) += w2;' in text
assert 'MOV U64 [RSI], R13' not in text
assert 'ADD RSI, R12' not in text

try:
    translate_grscreen(source.replace(b'ADD', b'ADDX', 1))
except ValueError:
    pass
else:
    raise AssertionError('overlay accepted a changed x86 instruction sequence')

print('PASS guarded ARM64 GrScreen source overlay', flush=True)
