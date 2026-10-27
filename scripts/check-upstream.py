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
bridge = (root/'upstream/bootstrap/System/TaskBridge.ZC').read_text()
for name, next_name in [('ExeFile', 'RunFile'), ('RunFile', 'ExePutS2'),
                        ('ExeFile2', 'RunFile2'), ('RunFile2', 'StreamExePrint')]:
    start = compiler_source.index('I64 '+name+'(')
    end = compiler_source.index('\nI64 '+next_name+'(', start)
    wrapper = compiler_source[start:end].strip()
    if name in ('ExeFile', 'ExeFile2'):
        call = 'res = '+('ExePutS2' if name.endswith('2') else 'ExePutS')+'(st, name, ccf_flags);'
        wrapper = wrapper.replace('\t'+call,
            '\ttry\n\t{\n\t\t'+call+'\n\t}\n\tcatch\n\t{\n\t\tFree(st);\n\t\tFree(name2);\n\t}')
    if wrapper not in bridge:
        raise SystemExit(name+' wrapper drifted from pinned CMain + cleanup catch')
compiler_misc = (zeal/'Compiler/CMisc.ZC').read_text()
option_end = compiler_misc.index('\nasm {')
if compiler_misc[:option_end].strip() not in (root/'upstream/bootstrap/System/TaskBridge.ZC').read_text():
    raise SystemExit('Option / OptionGet bodies drifted from pinned CMisc source')
str_source = (zeal/'Kernel/StrA.ZC').read_text()
first_remove = str_source.split('U8 *StrFirstRemove(', 1)[1].split('\nU8 *StrLastOcc(', 1)[0]
if 'U8 *StrFirstRemove(' + first_remove.strip() not in bridge:
    raise SystemExit('StrFirstRemove body drifted from pinned StrA source')
str_b_source = (zeal/'Kernel/StrB.ZC').read_text()
press_key = str_b_source.split('I64 PressAKey()', 1)[1].split('\nBool AreYouSure()', 1)[0]
if 'I64 PressAKey()' + press_key.rstrip() not in bridge:
    raise SystemExit('PressAKey body drifted from pinned StrB source')
print('PASS pinned Aiwnios originals + documented patch; unchanged ZealOS files; Win progress, CMain cleanup wrappers, Option, StrFirstRemove and PressAKey fixtures')

color_picker = (root/'upstream/bootstrap/System/DolDoc/ColorPicker.ZC').read_bytes()
polar_bridge = (root/'upstream/bootstrap/System/Math/PolarBridge.ZC').read_bytes()
for name, begin, end, target in [
    ('Kernel/KDataTypes.ZC', b'U0 QueueDel(', b'CQueue *QueueCopy(',
     (root/'upstream/bootstrap/Kernel/QueueBridge.ZC').read_bytes()),
    ('Kernel/KDataTypes.ZC', b'I64 QueueCount(', b'I64 QueueSize(',
     (root/'upstream/bootstrap/Kernel/QueueBridge.ZC').read_bytes()),
    ('System/Math/Math.ZC', b'public U0 R2P(', b'public U0 P2R(', polar_bridge),
    ('System/DolDoc/DocGr.ZC', b'public I64 PopUpColor(', b'public I64 PopUpColorLighting(', color_picker),
]:
    source = (zeal/name).read_bytes()
    start = source.index(begin)
    body = source[start:source.index(end, start)].rstrip()
    if body not in target:
        raise SystemExit(f'Portable source fixture drifted: {begin!r} in {name}')
print('PASS unchanged QueueDel, QueueCount, R2P and PopUpColor bodies')
