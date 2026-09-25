#!/usr/bin/env python3
"""Embed a .ZC file as a C string constant for kernel smokes/shell."""
import pathlib, sys

src, dst, symbol = sys.argv[1], sys.argv[2], sys.argv[3]
raw = pathlib.Path(src).read_bytes()
text = raw.decode("latin-1")
out = []
out.append(f"/* auto-generated from {src} — do not edit */")
out.append("#pragma once")
out.append(f"static const char {symbol}[] =")

def esc_line(line: str) -> str:
    buf = []
    for ch in line:
        o = ord(ch)
        if ch == "\\":
            buf.append("\\\\")
        elif ch == '"':
            buf.append('\\"')
        elif ch == "\t":
            buf.append("\\t")
        elif o < 0x20 or o >= 0x7F:
            # Octal: exactly 3 digits, avoids C \\x greediness before hex digits.
            buf.append(f"\\{o:03o}")
        else:
            buf.append(ch)
    return "".join(buf)

parts = text.split("\n")
for i, line in enumerate(parts):
    el = esc_line(line)
    if i + 1 < len(parts):
        out.append(f'    "{el}\\n"')
    else:
        out.append(f'    "{el}";')
pathlib.Path(dst).write_text("\n".join(out) + "\n")
print(f"wrote {dst} ({len(raw)} bytes source)")
