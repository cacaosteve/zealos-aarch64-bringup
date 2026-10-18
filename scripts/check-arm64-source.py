#!/usr/bin/env python3
"""Verify guarded AArch64 overlays for pinned ZealOS sources."""
from pathlib import Path
import re
import sys

from arm64_source import (translate_docfind, translate_docform, translate_docpopup, translate_grbitmap,
                          translate_grscreen, translate_ktask, translate_winmgr)

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

bitmap_source = (root / 'upstream/pinned/System/Gr/GrBitMap.ZC').read_bytes()
bitmap_overlay = translate_grbitmap(bitmap_source)
bitmap_text = bitmap_overlay.decode('utf-8')
assert bitmap_overlay != bitmap_source
ext_start = bitmap_text.index('public CDC *DCExt(')
ext_end = bitmap_text.index('\npublic CDC *DCDiff(', ext_start)
ext_text = bitmap_text[ext_start:ext_end]
assert 'if (dc->flags & DCF_SCREEN_BITMAP)\n\t\tGrBlot(res, -x1, -y1, dc);' in ext_text
assert 'for (row = 0; row < res->height; row++)' in ext_text
assert 'DCNew((width + 7) & ~7, y2 - y1 + 1, task)' in ext_text
assert 'res->width = width;' in ext_text
assert 'dc->body + (y1 + row) * dc->width_internal + x1' in ext_text
assert 'res->width);' in ext_text

try:
    translate_grbitmap(bitmap_source.replace(b'GrBlot(res, -x1, -y1, dc);',
                                             b'GrBlot(res, -x1, -y1, image);', 1))
except ValueError:
    pass
else:
    raise AssertionError('GrBitMap overlay accepted a changed DCExt extraction')

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

winmgr_source = (root / 'upstream/pinned/System/WinMgr.ZC').read_bytes()
winmgr_translated = translate_winmgr(winmgr_source)
winmgr_text = winmgr_translated.decode('utf-8')
assert winmgr_translated != winmgr_source
assert winmgr_text.count('ZcTaskFrameRun;') == 1
assert '\t\tGrUpdateScreen;' not in winmgr_text

try:
    translate_winmgr(winmgr_source.replace(b'GrUpdateScreen;', b'GrUpdateScreen32;', 1))
except ValueError:
    pass
else:
    raise AssertionError('overlay accepted a changed window-manager refresh call')

ktask_source = (root / 'upstream/pinned/Kernel/KTask.ZC').read_bytes()
ktask_overlay = translate_ktask(ktask_source).decode('utf-8')
assert ktask_overlay != ktask_source.decode('utf-8')
assert 'result = ZcCallNOnStack(&tmp->stack_base,' in ktask_overlay
assert 'fp_addr, argc, argv);' in ktask_overlay
assert 'asm {' not in ktask_overlay
assert '#exe {Option(OPTf_NO_REG_VAR, ON);};' not in ktask_overlay
assert '#exe {Option(OPTf_NO_REG_VAR, OFF);};' not in ktask_overlay
assert 'return ZcSpawn(fp_start_addr, data, task_name, target_cpu, parent,' in ktask_overlay
assert 'stack_size, flags);' in ktask_overlay
assert 'return ZcTaskValidate(task);' in ktask_overlay
assert 'return ZcTaskKill(task, wait, just_break);' in ktask_overlay
assert 'if (task == sys_winmgr_task)' in ktask_overlay
assert '0 < task <= I32_MAX' not in ktask_overlay
assert 'TaskQueueInsChild(task);' not in ktask_overlay
assert 'MemCopy(task->rsp(U8 *), &data, sizeof(U8 *));' not in ktask_overlay
assert ktask_overlay.count('no_warn dummy;') == 0
assert ktask_overlay.count('no_warn is metadata only') == 2
assert ktask_overlay.startswith('extern I64 ZcCallExtStr(U8 *name);\n')
assert ktask_overlay.count('ZcCallExtStr("ServerStartUp");') == 1
assert ktask_overlay.count('ZcCallExtStr("UserStartUp");') == 1
assert 'extern Bool ZcTaskValidate(CTask *task);' in ktask_overlay
assert 'extern Bool ZcTaskKill(CTask *task,' in ktask_overlay
assert 'extern CTask *ZcSpawn(' in ktask_overlay

try:
    translate_ktask(ktask_source.replace(
        b'\tTaskQueueInsChild(task);', b'\tTaskQueueIns(task);', 1))
except ValueError:
    pass
else:
    raise AssertionError('KTask overlay accepted a changed Spawn queue path')

try:
    translate_ktask(ktask_source.replace(
        b'0 < task <= I32_MAX', b'0 <= task <= I32_MAX', 1))
except ValueError:
    pass
else:
    raise AssertionError('KTask overlay accepted a changed TaskValidate range check')

try:
    translate_ktask(ktask_source.replace(
        b'LBts(&task->task_flags, TASKf_KILL_TASK);',
        b'LBtr(&task->task_flags, TASKf_KILL_TASK);', 1))
except ValueError:
    pass
else:
    raise AssertionError('KTask overlay accepted a changed Kill flag path')

try:
    translate_ktask(ktask_source.replace(
        b'\tno_warn dummy;', b'\tno_warn unused;', 1))
except ValueError:
    pass
else:
    raise AssertionError('KTask overlay accepted a changed warning suppression')

try:
    translate_ktask(ktask_source.replace(b'LEAVE', b'ENTER', 1))
except ValueError:
    pass
else:
    raise AssertionError('KTask overlay accepted a changed CallStackGrow assembly')

docform_source = (root / 'upstream/pinned/System/DolDoc/DocForm.ZC').read_bytes()
docform_overlay = translate_docform(docform_source)
assert docform_overlay != docform_source
assert docform_overlay.count(b'LBtr(&doc_e->de_flags, DOCEf_DEFAULT_LEN)') == 1
assert b'LBtr(&doc_e->de_flags, &DOCEf_DEFAULT_LEN)' not in docform_overlay
try:
    translate_docform(docform_source.replace(
        b'LBtr(&doc_e->de_flags, &DOCEf_DEFAULT_LEN)',
        b'LBtr(&doc_e->de_flags, DOCEf_DEFAULT_LEN)', 1))
except ValueError:
    pass
else:
    raise AssertionError('DocForm overlay accepted a changed bit-index expression')

docfind_source = (root / 'upstream/pinned/System/DolDoc/DocFind.ZC').read_bytes()
docfind_overlay = translate_docfind(docfind_source)
assert docfind_overlay != docfind_source
assert docfind_overlay.count(b'BootstrapStrIMatch(needle, doc_e->tag)') == 1
assert b'= StrIMatch(needle, doc_e->tag)' not in docfind_overlay
assert docfind_overlay.count(b'STR_LEN') == docfind_source.count(b'STR_LEN') + 2
assert b'sizeof(CEdFindText.find_text)' not in docfind_overlay
assert b'sizeof(CEdFindText.replace_text)' not in docfind_overlay
try:
    translate_docfind(docfind_source.replace(
        b'StrIMatch(needle, doc_e->tag)',
        b'BootstrapStrIMatch(needle, doc_e->tag)', 1))
except ValueError:
    pass
else:
    raise AssertionError('DocFind overlay accepted a changed call site')

menu_source = (root / 'upstream/pinned/System/Menu.ZC').read_bytes()
menu_overlay = (root / 'upstream/bootstrap/System/Menu.ZC').read_bytes()
menu_expected = menu_source
for old, new in (
        (b'LexExcept(cc, "Missing \')\' at ");',
         b'return NULL; // ARM64 bootstrap: reject malformed menu syntax safely.'),
        (b'LexExcept(cc, "Missing \';\' at");',
         b'return NULL; // ARM64 bootstrap: reject malformed menu syntax safely.'),
        (b'LexExcept(cc, "Missing \'}\' at ");',
         b'return NULL; // ARM64 bootstrap: reject malformed menu syntax safely.'),
        (b'LexExcept(cc, "Expecting \'{\' at ");',
         b'return NULL; // ARM64 bootstrap: reject malformed menu syntax safely.'),
        (b'res = MaxI64(res, StrLen(tmpse->name));',
         b'if (res < StrLen(tmpse->name))\n            res = StrLen(tmpse->name);')):
    if menu_expected.count(old) != 1:
        raise AssertionError(f'menu overlay anchor changed: {old!r}')
    menu_expected = menu_expected.replace(old, new, 1)
assert menu_overlay == menu_expected
runtime = (root / 'src/zc/runtime.c').read_text()
assert runtime.index('"/System/Menu.ZC"') < runtime.index('"/System/Win.ZC"')

print('PASS guarded ARM64 source overlays', flush=True)

# Only raw source-display metadata is removed; program tokens remain pinned.
docpopup_source = (root / 'upstream/pinned/System/DolDoc/DocPopUp.ZC').read_bytes()
assert translate_docpopup(docpopup_source) == docpopup_source.replace(b'$WW,0$', b'')
try:
    translate_docpopup(docpopup_source.replace(b'$WW,0$', b'$WW,1$'))
except ValueError:
    pass
else:
    raise AssertionError('DocPopUp overlay accepted changed source metadata')
print('PASS guarded DocPopUp display-metadata overlay')
