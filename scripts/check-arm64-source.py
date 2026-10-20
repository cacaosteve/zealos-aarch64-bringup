#!/usr/bin/env python3
"""Verify guarded AArch64 overlays for pinned ZealOS sources."""
from pathlib import Path
import re
import struct
import sys

from arm64_source import (extract_home_set, extract_home_startup, extract_home_startup_tasks, inject_home_startup_tasks, extract_server_task, extract_user_task, extract_version_scalars, extract_source_prompt, extract_source_training, translate_autocomplete_task, translate_doctree, translate_registry, translate_docterm, translate_doced, translate_docfind, translate_docform, translate_docpopup, translate_grbitmap,
                          translate_grdc, translate_grpalette, translate_grscreen, translate_ktask, translate_winmgr, translate_zsplash, translate_home_once)

root = Path(__file__).resolve().parent.parent
for path, transform, original, changed in (
    ('Kernel/StrB.ZC', extract_source_prompt, b'Bool YorN()', b'Bool OtherPrompt()'),
    ('System/Training.ZC', extract_source_training, b'i = RandU16;', b'i = 0;'),
    ('System/DolDoc/DocTree.ZC', translate_doctree, b'LexAttachDoc(cc,, doc);', b'LexAttachDoc(cc,, other);'),
    ('System/Registry.ZC', translate_registry, b'if (!sys_registry_doc)', b'if (sys_registry_doc)'),
    ('Kernel/KGlobals.ZC', extract_version_scalars, b'sys_os_version_sub', b'other_version_sub'),
):
    source = (root / 'upstream/pinned' / path).read_bytes()
    translated = transform(source)
    if path == 'System/DolDoc/DocTree.ZC':
        text = translated.decode('utf-8')
        start = text.index('public I64 DocTreeExe(')
        end = text.index('\npublic I64 DocTreeFExe(', start)
        doc_tree_exe = text[start:end]
        assert doc_tree_exe.count('DocDel(doc2);') == 1
        assert doc_tree_exe.index('catch') < doc_tree_exe.index('DocDel(doc2);')
    try:
        transform(source.replace(original, changed, 1))
    except ValueError:
        pass
    else:
        raise AssertionError('overlay accepted changed dependency: ' + path)
docterm = (root / 'upstream/pinned/System/DolDoc/DocTerm.ZC').read_bytes()
assert b'SCX=15$$", STR_LEN - 1);' in translate_docterm(docterm)
try:
    translate_docterm(docterm.replace(b'LEN=STR_LEN-1', b'LEN=other'))
except ValueError:
    pass
else:
    raise AssertionError('DocTerm overlay accepted a changed title length')
autocomplete_task = (root / 'upstream/pinned/System/AutoComplete/ACTask.ZC').read_bytes()
autocomplete_overlay = translate_autocomplete_task(autocomplete_task)
assert b'if (Fs->win_right <= left)' in autocomplete_overlay
assert b'if (Fs->win_bottom <= top)' in autocomplete_overlay
assert b'DocRecalc(DocPut);' in autocomplete_overlay
assert autocomplete_overlay.count(b'DocPrint(DocPut,') == 8
assert b'"$$RED$$Word:%s$$FG$$\\n", ac.cur_word;' not in autocomplete_overlay
assert b'"%s\\n", st;' not in autocomplete_overlay
try:
    translate_autocomplete_task(autocomplete_task.replace(
        b'WinHorz(left, Fs->win_right);', b'WinHorz(left, other);', 1))
except ValueError:
    pass
else:
    raise AssertionError('AutoComplete overlay accepted changed popup geometry')
try:
    translate_autocomplete_task(autocomplete_task.replace(
        b'ACPutChoices(doc, doc_e, focus_task, ToBool(sc != last_sc));',
        b'ACPutChoices(doc, doc_e, focus_task, FALSE);', 1))
except ValueError:
    pass
else:
    raise AssertionError('AutoComplete overlay accepted changed choice update')
doced = (root / 'upstream/pinned/System/DolDoc/DocEd.ZC').read_bytes()
assert b'", STR_LEN - 1);' in translate_doced(doced)
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

dc_source = (root / 'upstream/pinned/System/Gr/GrDC.ZC').read_bytes()
dc_overlay = translate_grdc(dc_source)
assert dc_overlay != dc_source
assert b'res->width_internal\t= (width + 7) & ~7;' in dc_overlay
assert b'res->width_internal\t= width & ~7;' not in dc_overlay
try:
    translate_grdc(dc_source.replace(b'width & ~7;', b'width & ~15;', 1))
except ValueError:
    pass
else:
    raise AssertionError('GrDC overlay accepted a changed row-stride calculation')

palette_source = (root / 'upstream/pinned/System/Gr/GrPalette.ZC').read_bytes()
palette_overlay = translate_grpalette(palette_source)
assert palette_overlay != palette_source
assert b'ZcFbGraphPaletteSet(bgr24);' in palette_overlay
try:
    translate_grpalette(palette_source.replace(
        b'MemCopy(&gr_palette, bgr24, sizeof(CBGR24) * COLORS_NUM);',
        b'MemCopy(&other_palette, bgr24, sizeof(CBGR24) * COLORS_NUM);', 1))
except ValueError:
    pass
else:
    raise AssertionError('GrPalette overlay accepted a changed palette setter')

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
assert ('if (task == Fs && 0 <= mouse.pos.y < FONT_HEIGHT && mouse_hard.installed)\n'
        '\t\t\t\t{\n'
        '\t\t\t\t\twinmgr.show_menu = TRUE;\n'
        '\t\t\t\t\tBootstrapMenuFocusRemember;\n'
        '\t\t\t\t\tBootstrapCtrlInputPoll(TRUE);\n'
        '\t\t\t\t\tBootstrapMenuFocusRestore;\n'
        '\t\t\t\t\tif (mouse.lb)\n') in winmgr_text
assert ('while (mouse.lb)\n'
        '\t\t\t\t\t\t{\n'
        '\t\t\t\t\t\t\tBootstrapCtrlInputPoll(TRUE);\n'
        '\t\t\t\t\t\t\tBootstrapMenuFocusRestore;\n'
        '\t\t\t\t\t\t\tWinMgrSleep(TRUE);\n'
        '\t\t\t\t\t\t\tBootstrapMenuFocusRestore;\n'
        '\t\t\t\t\t\t}') in winmgr_text
assert ('BootstrapMenuFocusRestore;\n'
        '\t\t\t\t\t\tif (sys_cur_submenu_entry)\n'
        '\t\t\t\t\t\t\tTaskMessage(sys_focus_task, 0,') in winmgr_text
assert '\t\t\t\t\t\tBootstrapMenuFocusRestore(TRUE);\n' in winmgr_text

try:
    translate_winmgr(winmgr_source.replace(b'GrUpdateScreen;', b'GrUpdateScreen32;', 1))
except ValueError:
    pass
else:
    raise AssertionError('overlay accepted a changed window-manager refresh call')

try:
    translate_winmgr(winmgr_source.replace(
        b'\t\t\t\t\tif (mouse.lb && !old_mouse.lb)\n',
        b'\t\t\t\t\tif (mouse.lb)\n', 1))
except ValueError:
    pass
else:
    raise AssertionError('overlay accepted a changed top-menu press check')

try:
    translate_winmgr(winmgr_source.replace(
        b'\t\t\t\t\t\twhile (mouse.lb)\n'
        b'\t\t\t\t\t\t\tWinMgrSleep(TRUE);',
        b'\t\t\t\t\t\twhile (mouse.lb)\n'
        b'\t\t\t\t\t\t\tWinMgrSleep;', 1))
except ValueError:
    pass
else:
    raise AssertionError('overlay accepted a changed top-menu mouse wait loop')

ktask_source = (root / 'upstream/pinned/Kernel/KTask.ZC').read_bytes()
user_slice = extract_user_task(ktask_source)
for anchor in (b'UserTaskCont()', b'I64 ExeCmdLine(CCompCtrl *cc)',
               b'QueueInsert(cc, Fs->last_cc);', b'cont = Bt(&cc->flags, CCf_PROMPT);',
               b'res = Call(machine_code);', b'Fs->answer_type\t= type;',
               b'ZcUserStatement2Bin(cc, &type)', b'ZcUserCompCtrlDel(cc);',
               b'\tcatch\n\t{\n\t\tFs->title_src = old_title_src;'):
    assert anchor in user_slice, anchor
assert user_slice.count(b'Free(old_task_title);') == 2
assert b'ServerTaskCont' not in user_slice
assert b'U0 UserCmdLine(I64 dummy=0)' in user_slice
assert b'ZcCallExtStr("UserStartUp");' in user_slice
assert b'CTask *User(U8 *format=NULL, ...)' in user_slice
assert b'if (!task)\n\t\treturn NULL;' in user_slice
home_source = (root / 'upstream/pinned/HomeSys.ZC').read_bytes()
once_source = (root / 'upstream/pinned/Once.ZC').read_bytes()
once_overlay = translate_home_once(once_source)
assert b'ARM64 live session is ready.' in once_overlay
assert b'The original disk installer has not been ported yet.' in once_overlay
assert b'RunFile("::/Misc/OSInstall",, TRUE);' not in once_overlay
assert b'if (FileFind("::/Misc/Tour"))' in once_overlay
try:
    translate_home_once(once_source.replace(b'RunFile("::/Misc/OSInstall",, TRUE);',
                                             b'RunFile("other",, TRUE);'))
except ValueError:
    pass
else:
    raise AssertionError('Once DVD overlay accepted a changed installer call')
disk_dir = (root / 'upstream/pinned/Kernel/BlkDev/DiskDirB.ZC').read_bytes()
home_set = extract_home_set(disk_dir)
original = disk_dir.split(b'\nBool Cd(', 1)[0]
home_guard = (b"\tif (!dirname || *dirname != '/')\n\t{\n"
              b"\t\tFree(dirname);\n\t\tthrow('HomePath');\n\t}\n")
assert original.replace(b'\tdirname = DirNameAbs(dirname);\n',
                        b'\tdirname = DirNameAbs(dirname);\n' + home_guard) in home_set
try:
    extract_home_set(disk_dir.replace(b'SysStrNew(dirname)', b'StrNew(dirname)', 1))
except ValueError:
    pass
else:
    raise AssertionError('HomeSet extraction accepted a changed allocation owner')
home_hooks = extract_home_startup(home_source)
home_tasks = extract_home_startup_tasks(home_source)
assert b'StartUpTasks;' not in home_hooks
for name in (b'UserStartUp', b'ServerStartUp'):
    assert b'U0 BootstrapHome' + name + b'()' in home_hooks
assert b'Type("::/Doc/Start.DD");' in home_hooks
assert b'I64 BootstrapHomeStartUpTasks()' in home_tasks
assert b'CTask *user = bootstrap_gui_terminal_task;' in home_tasks
assert b'bootstrap_gui_terminal_home_second = User();' in home_tasks
assert b'BootstrapGuiTerminalTileHomeWindows(user, bootstrap_gui_terminal_home_second);' in home_tasks
assert b'Boot Time:%%7.3fs' in home_tasks
assert b'#include \\"Once\\";' in home_tasks
assert b'ACInit(\\"/*;!*/Bible.TXT;!*/Clementine.TXT;!*/PCIDevices.DD;!*/HTML/*;!*/ChangeLog.DD\\")' in home_tasks
assert b'BootstrapGuiTerminalExeDocFile' in home_tasks
assert b'return 0x2a;' in home_tasks
gui_terminal_bootstrap = (root / 'upstream/bootstrap/System/GuiTerminal.ZC').read_bytes()
injected_gui_terminal = inject_home_startup_tasks(gui_terminal_bootstrap, home_tasks)
assert (injected_gui_terminal.index(b'I64 BootstrapHomeStartUpTasks()') <
        injected_gui_terminal.index(b'I64 BootstrapGuiTerminalHomeStart()'))
assert b'CTask *bootstrap_gui_terminal_task;' in injected_gui_terminal
try:
    inject_home_startup_tasks(gui_terminal_bootstrap.replace(
        b'I64 BootstrapGuiTerminalHomeStart()', b'I64 OtherStart()', 1), home_tasks)
except ValueError:
    pass
else:
    raise AssertionError('HomeSys task injection accepted a missing GUI insertion point')
try:
    extract_home_startup(home_source.replace(b'Type("::/Doc/Start.DD");', b'Type("other");'))
except ValueError:
    pass
else:
    raise AssertionError('HomeSys extraction accepted a changed startup document')
try:
    extract_home_startup(home_source.replace(b'WinTileVert;', b'WinTileGrid;', 1))
except ValueError:
    pass
else:
    raise AssertionError('HomeSys startup extraction accepted changed window tiling')
gui_terminal_source = gui_terminal_bootstrap.decode('utf-8')
assert 'I64 BootstrapGuiTerminalHomeStart()' in gui_terminal_source
assert 'BootstrapHomeStartUpTasks() != 0x2a' in gui_terminal_source
assert 'The DVD Once overlay opens a live session' in gui_terminal_source
assert 'sys_boot_src.u16[0] == BOOT_SRC_DVD' not in gui_terminal_source
assert 'bootstrap_gui_terminal_home_second = User();' not in gui_terminal_source
assert 'XTalk(user, "Cd; #include \\"Once\\";\\n");' not in gui_terminal_source
assert 'U0 ServerStartUp()' in gui_terminal_source
assert 'BootstrapHomeServerStartUp;' in gui_terminal_source
runtime_source = (root / 'src/zc/runtime.c').read_text()
assert 'zc_call("BootstrapGuiTerminalHomeStart", &result)' in runtime_source
kernel_source = (root / 'src/kernel_stub.c').read_text()
assert 'if (zc_start_home_startup() != 0)' in kernel_source
try:
    extract_user_task(ktask_source.replace(b'LexStatement2Bin(cc, &type)', b'OtherCompiler(cc)', 1))
except ValueError:
    pass
else:
    raise AssertionError('user extraction accepted a changed compiler call')
server_slice = extract_server_task(ktask_source)
assert (ktask_source.split(b'U0 ServerTaskCont()\n')[1].split(b'\nU0 UserTaskCont()\n')[0]
        .replace(b'JobsHandler(old_flags)', b'ZcServerJobsHandler(old_flags)', 1)
        in server_slice)
assert b'UserTaskCont' not in server_slice
assert b'return ZcSystemTaskStart(&ServerTaskCont);' in server_slice
assert b'U0 BootstrapServerTaskStart()' in server_slice
assert b'ZcCallExtStr("ServerStartUp");' in server_slice
assert b'ServerTaskCont;' in server_slice
try:
    extract_server_task(ktask_source.replace(b'JobsHandler(old_flags)', b'JobsHandler(0)', 1))
except ValueError:
    pass
else:
    raise AssertionError('server extraction accepted a changed dispatch loop')
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

zsplash_source = (root / 'upstream/pinned/System/ZSplash.ZC').read_bytes()
zsplash_overlay = translate_zsplash(zsplash_source)
zsplash_payload = zsplash_source.split(b'\0', 1)[1].decode('utf-8').encode('cp437')
assert b'ZSplash;' in zsplash_overlay
assert b'$SP,"<1>",BI=1$' in zsplash_overlay
assert b'Sprite3(dc, 0, 0, 0, $IB,"<1>",BI=1$);' in zsplash_overlay
assert zsplash_overlay.endswith(b'\0' + zsplash_payload)
assert struct.unpack_from('<IIII', zsplash_payload) == (1, 0, len(zsplash_payload) - 16, 2)
assert zsplash_payload[16] == 24
assert struct.unpack_from('<II', zsplash_payload, 17) == (20, 35)
assert struct.unpack_from('<iii', zsplash_payload, 25) == (-50, -50, -20)
assert b'Sleep(counts.timer & 0x7F);' in zsplash_overlay
assert b'Sleep((Now + counts.timer) & 0x7F);' not in zsplash_overlay
assert b'MStrPrint("%s", "ZealOS ARM64");' in zsplash_overlay
assert b'DD_OS_NAME_VERSION' not in zsplash_overlay
assert b'no_warn' not in zsplash_overlay
assert b'AutoComplete(' not in zsplash_overlay
assert b'MouseHard' not in zsplash_overlay and b'DrawStdMouse' not in zsplash_overlay
assert b'DocPrint(' not in zsplash_overlay
assert b'CTask *splash_task = Fs;' in zsplash_overlay
assert b'sys_focus_task = splash_task;' in zsplash_overlay
assert b'sys_focus_task = old_focus;' in zsplash_overlay
assert b'WinToTop(splash_task);' in zsplash_overlay
assert b'Simplicity Perfected' in zsplash_overlay
assert b'Kill(bg_task)' not in zsplash_overlay and b'Kill(splash_task)' not in zsplash_overlay
gui_terminal = (root / 'upstream/bootstrap/System/GuiTerminal.ZC').read_bytes()
assert b'BootstrapGuiTerminalExeDocFile(' in gui_terminal and b'::/System/ZSplash.ZC' not in gui_terminal
assert b'BootstrapHomeStartUpTasks() != 0x2a' in gui_terminal
try:
    translate_zsplash(zsplash_source.replace(b'Sprite3(dc,', b'Sprite4(dc,', 1))
except ValueError:
    pass
else:
    raise AssertionError('ZSplash overlay accepted changed inline sprite call')
print('PASS guarded upstream animated ZSplash sprite with native pointer ownership')

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
