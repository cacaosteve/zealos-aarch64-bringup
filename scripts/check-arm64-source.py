#!/usr/bin/env python3
"""Verify the guarded AArch64 source overlay for pinned GrScreen.ZC."""
from pathlib import Path
import re
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
assert 'cell_dst = dst(U8 *);' in text
assert 'for (j = 0; j < FONT_HEIGHT; j++)' in text
assert '*cell_dst(U64 *) = c;' in text
assert 'cell_dst += w1;' in text
assert 'dst(U8 *) += 7 * w1;' in text
assert text.count('ZcDocUpdateTaskDocs(task);') == 1
assert not re.search(r'(?<!Zc)DocUpdateTaskDocs\(task\);', text)
assert text.count('ZcODEsUpdate(task);') == 1
assert not re.search(r'(?<!Zc)ODEsUpdate\(task\);', text)
assert text.count('dst(U8 *) += w2;') >= 2
assert 'MOV U64 [RSI], R13' not in text
assert 'ADD RSI, R12' not in text

try:
    translate_grscreen(source.replace(b'ADD', b'ADDX', 1))
except ValueError:
    pass
else:
    raise AssertionError('overlay accepted a changed x86 instruction sequence')

try:
    translate_grscreen(source.replace(b'DocUpdateTaskDocs(task);', b'OtherDocs(task);'))
except ValueError:
    pass
else:
    raise AssertionError('overlay accepted a changed task-doc call')

try:
    translate_grscreen(source.replace(b'ODEsUpdate(task);', b'OtherODE(task);'))
except ValueError:
    pass
else:
    raise AssertionError('overlay accepted a changed ODE update call')

print('PASS guarded ARM64 GrScreen source overlay', flush=True)
