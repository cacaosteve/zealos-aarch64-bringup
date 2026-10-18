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
win_source = (zeal/'System/Win.ZC').read_text()
win_fixture = (root/'tests/compat/WinProgressBars.ZC').read_text()
start = win_source.index('U0 DrawProgressBars(CDC *dc)')
brace = win_source.index('{', start)
depth = 0
for index in range(brace, len(win_source)):
    depth += (win_source[index] == '{') - (win_source[index] == '}')
    if depth == 0:
        end = index + 1
        break
else:
    raise SystemExit('Could not find upstream DrawProgressBars body')
if win_source[start:end] not in win_fixture:
    raise SystemExit('WinProgressBars compatibility fixture drifted from pinned source')
compiler_source = (zeal/'Compiler/CMain.ZC').read_text()
start = compiler_source.index('I64 ExeFile(U8 *name, I64 ccf_flags=0)')
end = compiler_source.index('\nI64 RunFile(', start)
if compiler_source[start:end].strip() not in (root/'upstream/bootstrap/System/TaskBridge.ZC').read_text():
    raise SystemExit('ExeFile wrapper drifted from pinned CMain source')
start = end + 1
end = compiler_source.index('\nI64 ExePutS2(', start)
if compiler_source[start:end].strip() not in (root/'upstream/bootstrap/System/TaskBridge.ZC').read_text():
    raise SystemExit('RunFile wrapper drifted from pinned CMain source')
compiler_misc = (zeal/'Compiler/CMisc.ZC').read_text()
option_end = compiler_misc.index('\nasm {')
if compiler_misc[:option_end].strip() not in (root/'upstream/bootstrap/System/TaskBridge.ZC').read_text():
    raise SystemExit('Option / OptionGet bodies drifted from pinned CMisc source')
print('PASS pinned Aiwnios originals + documented patch; unchanged ZealOS files; Win progress, ExeFile, RunFile and Option fixtures')
