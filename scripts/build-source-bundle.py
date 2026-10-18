#!/usr/bin/env python3
"""Build deterministic read-only source module plus documented ARM64 overlays."""
import hashlib
import io
import json
from pathlib import Path
import sys
import tarfile

from arm64_source import (extract_home_set, extract_home_startup, extract_version_scalars, extract_server_task, extract_user_task, translate_doctree, translate_registry, translate_docterm, translate_doced, translate_docfind, translate_docform, translate_docpopup, translate_grbitmap,
                          translate_grdc, translate_grscreen, translate_ktask, translate_mathode,
                          translate_winmgr)

root = Path(__file__).resolve().parent.parent
manifest = json.loads((root / 'upstream/pinned/UPSTREAM.json').read_text())
entries = {}
for name, expected in manifest['files'].items():
    rel = name.removeprefix('src/')
    data = (root / 'upstream/pinned' / rel).read_bytes()
    if hashlib.sha256(data).hexdigest() != expected:
        raise SystemExit(f'Pinned source changed: {name}')
    if rel == 'HomeSys.ZC':
        entries['System/HomeStartUp.ZC'] = extract_home_startup(data)
    if rel == 'Kernel/BlkDev/DiskDirB.ZC':
        entries['System/SourceHome.ZC'] = extract_home_set(data)
    if rel == 'Once.ZC':
        # Seed the same editable home startup file installed by upstream.
        # It is available for the upcoming registry bring-up, not auto-run.
        entries['Home/Once.ZC'] = data
    if rel == 'Kernel/KGlobals.ZC':
        entries['System/VersionScalars.ZC'] = extract_version_scalars(data)
    if rel == 'System/Gr/GrScreen.ZC':
        try:
            data = translate_grscreen(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 source overlay no longer applies: {exc}') from exc
    elif rel == 'System/Gr/GrDC.ZC':
        try:
            data = translate_grdc(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 source overlay no longer applies: {exc}') from exc
    elif rel == 'System/Gr/GrBitMap.ZC':
        try:
            data = translate_grbitmap(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 source overlay no longer applies: {exc}') from exc
    elif rel == 'System/WinMgr.ZC':
        try:
            data = translate_winmgr(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 window-manager overlay no longer applies: {exc}') from exc
    elif rel == 'System/Math/MathODE.ZC':
        try:
            data = translate_mathode(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 MathODE overlay no longer applies: {exc}') from exc
    elif rel == 'System/DolDoc/DocPopUp.ZC':
        data = translate_docpopup(data)
    elif rel == 'System/DolDoc/DocForm.ZC':
        try:
            data = translate_docform(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 source overlay no longer applies: {exc}') from exc
    elif rel == 'System/DolDoc/DocFind.ZC':
        try:
            data = translate_docfind(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 source overlay no longer applies: {exc}') from exc
    elif rel == 'System/DolDoc/DocTerm.ZC':
        data = translate_docterm(data)
    elif rel == 'System/DolDoc/DocEd.ZC':
        data = translate_doced(data)
    elif rel == 'System/DolDoc/DocTree.ZC':
        data = translate_doctree(data)
    elif rel == 'System/Registry.ZC':
        data = translate_registry(data)
    elif rel == 'Kernel/KTask.ZC':
        entries['System/ServerTask.ZC'] = extract_server_task(data)
        entries['System/UserTask.ZC'] = extract_user_task(data)
        try:
            data = translate_ktask(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 KTask overlay no longer applies: {exc}') from exc
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
entries['PORTS/ARM64/GrScreen.md'] = (root / 'upstream/arm64/GrScreen.md').read_bytes()
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
