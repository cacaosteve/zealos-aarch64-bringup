#!/usr/bin/env python3
"""Verify original source hashes after reversing the documented port patch."""
import hashlib, json, pathlib, shutil, subprocess, tempfile
root = pathlib.Path(__file__).resolve().parents[1]
vendor = root/'third_party/aiwnios'
manifest = json.loads((vendor/'UPSTREAM.json').read_text())
with tempfile.TemporaryDirectory(prefix='zeal-source-check-') as tmp:
    tmp = pathlib.Path(tmp)
    for name in manifest['files']:
        dest = tmp/name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(vendor/name, dest)
    subprocess.run(['patch', '-R', '-p1', '-t', '-s', '-i', str(vendor/'PORT.patch')], cwd=tmp, check=True)
    for name, expected in manifest['files'].items():
        actual = hashlib.sha256((tmp/name).read_bytes()).hexdigest()
        if actual != expected: raise SystemExit(f'Aiwnios original hash mismatch: {name}')
zeal = root/'upstream/pinned'
manifest = json.loads((zeal/'UPSTREAM.json').read_text())
for name, expected in manifest['files'].items():
    if hashlib.sha256((zeal/name.removeprefix('src/')).read_bytes()).hexdigest() != expected:
        raise SystemExit(f'ZealOS original hash mismatch: {name}')
print('PASS pinned Aiwnios originals + documented patch; unchanged ZealOS source files')
