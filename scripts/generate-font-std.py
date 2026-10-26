#!/usr/bin/env python3
"""Generate the host-side table for the pinned ZealOS standard font."""
import hashlib
import json
from pathlib import Path
import re
import sys


root = Path(__file__).resolve().parents[1]
std_path = root / "upstream/pinned/Kernel/FontStd.ZC"
aux_path = root / "upstream/pinned/Kernel/FontAux.ZC"
if len(sys.argv) != 3 or Path(sys.argv[1]).resolve() != std_path.resolve():
    raise SystemExit("usage: generate-font-std.py pinned/Kernel/FontStd.ZC OUTPUT")
manifest = json.loads((root / "upstream/pinned/UPSTREAM.json").read_text())


def load_font(path, source_name, table_name):
    expected = manifest["files"].get(source_name)
    source_bytes = path.read_bytes()
    if not expected or hashlib.sha256(source_bytes).hexdigest() != expected:
        raise SystemExit(f"pinned {path.name} hash mismatch")
    source = source_bytes.decode("utf-8")
    source = "\n".join(line.split("//", 1)[0] for line in source.splitlines())
    match = re.search(
        rf"\bsys_font_{table_name}\s*\[\s*256\s*\]\s*=\s*\{{(.*?)\}}\s*;",
        source,
        re.S,
    )
    if not match:
        raise SystemExit(f"could not find pinned sys_font_{table_name}[256]")
    body = match.group(1)
    tokens = re.findall(r"0[xX][0-9a-fA-F]+|\b[0-9]+\b", body)
    residue = re.sub(r"0[xX][0-9a-fA-F]+|\b[0-9]+\b|[\s,]", "", body)
    if residue or len(tokens) != 256:
        raise SystemExit(f"invalid pinned {table_name} font table ({len(tokens)} entries)")
    values = [int(token, 0) for token in tokens]
    if any(value < 0 or value > 0xFFFFFFFFFFFFFFFF for value in values):
        raise SystemExit(f"{table_name} font entry does not fit U64")
    return values


fonts = (
    ("std", load_font(std_path, "src/Kernel/FontStd.ZC", "std")),
    ("aux", load_font(aux_path, "src/Kernel/FontAux.ZC", "aux")),
)
lines = [
    "/* Generated from pinned ZealOS Kernel/FontStd.ZC and FontAux.ZC; do not edit. */",
]
for name, values in fonts:
    lines.append(f"static const uint64_t zeal_font_{name}[256] = {{")
    for offset in range(0, len(values), 4):
        lines.append("    " + ", ".join(f"UINT64_C(0x{v:016x})" for v in values[offset:offset + 4]) + ",")
    lines.append("};")
lines.append("")
output = Path(sys.argv[2])
output.parent.mkdir(parents=True, exist_ok=True)
rendered = "\n".join(lines)
if not output.exists() or output.read_text() != rendered:
    output.write_text(rendered)
