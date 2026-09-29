#!/usr/bin/env python3
"""Build a deterministic, read-only source module with pinned upstream bytes."""
import hashlib
import io
import json
from pathlib import Path
import sys
import tarfile

root = Path(__file__).resolve().parent.parent
manifest = json.loads((root / 'upstream/pinned/UPSTREAM.json').read_text())
entries = {}
for name, expected in manifest['files'].items():
    rel = name.removeprefix('src/')
    data = (root / 'upstream/pinned' / rel).read_bytes()
    if hashlib.sha256(data).hexdigest() != expected:
        raise SystemExit(f'Pinned source changed: {name}')
    entries[rel] = data
for path in sorted((root / 'upstream/bootstrap').rglob('*')):
    if path.is_file():
        entries[path.relative_to(root / 'upstream/bootstrap').as_posix()] = path.read_bytes()
for path in sorted((root / 'tests/compat').glob('*')):
    if path.suffix in ('.ZC', '.HH'):
        entries['Tests/' + path.name] = path.read_bytes()
entries['LICENSE/ZealOS.txt'] = (root / 'upstream/pinned/LICENSE').read_bytes()
entries['LICENSE/Aiwnios.txt'] = (root / 'third_party/aiwnios/LICENSE').read_bytes()
entries['UPSTREAM.json'] = (root / 'upstream/pinned/UPSTREAM.json').read_bytes()
output = Path(sys.argv[1])
output.parent.mkdir(parents=True, exist_ok=True)
with tarfile.open(output, 'w', format=tarfile.USTAR_FORMAT) as bundle:
    for name, data in sorted(entries.items()):
        info = tarfile.TarInfo(name)
        info.size = len(data)
        info.mode = 0o444
        info.mtime = info.uid = info.gid = 0
        bundle.addfile(info, io.BytesIO(data))
print(f'Source bundle: {len(entries)} files, ZealOS {manifest["commit"][:12]}')
