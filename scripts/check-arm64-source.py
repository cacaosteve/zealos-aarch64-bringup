#!/usr/bin/env python3
"""Verify guarded AArch64 overlays for pinned ZealOS sources."""
from pathlib import Path
import re
import struct
import sys
from arm64_source import extract_keyboard_helpers
from arm64_source import (extract_sprite_elem_base_sizes, inject_sprite_elem_base_sizes,
                          extract_sprite_interpolation, inject_sprite_interpolation,
                          extract_sprite_matrix_draw, inject_sprite_matrix_draw)

from arm64_source import (extract_home_set, extract_home_startup, extract_home_startup_tasks, inject_home_startup_tasks, extract_server_task, extract_user_task, extract_version_scalars, extract_source_prompt, extract_source_training, translate_autocomplete_task, translate_doctree, translate_registry, translate_docterm, translate_doced, translate_docfind, translate_docform, translate_docpopup, translate_docrecalc, translate_grbitmap, translate_tothefront_draw, translate_tothefront_init, translate_tothefront_sentinel,
                          translate_filemgr, translate_grdc, translate_grpalette, translate_message, translate_grscreen, translate_ktask, translate_winmgr, translate_zsplash, translate_home_once)

root = Path(__file__).resolve().parent.parent
sprite_new = (root/'upstream/pinned/System/Gr/SpriteNew.ZC').read_bytes()
sprite_bridge = (root/'upstream/bootstrap/System/Gr/SpriteBridge.ZC').read_bytes()
assert b'Fs ? Fs->task_name : NULL' not in sprite_bridge
assert b'doc ? doc->flags : 0' not in sprite_bridge
assert b'entry ? entry->type : -1' not in sprite_bridge
sprite_size_declaration = extract_sprite_elem_base_sizes(sprite_new)
assert sprite_size_declaration.startswith(
    b'I64 sprite_elem_base_sizes[SPT_TYPES_NUM] = {')
assert sprite_size_declaration.count(b'//SPT_') == 30
sprite_bridge_with_sizes = inject_sprite_elem_base_sizes(
    sprite_bridge, sprite_new)
assert sprite_bridge_with_sizes.startswith(sprite_size_declaration + b'\n\n')
assert sprite_bridge_with_sizes.endswith(sprite_bridge)
assert sprite_bridge_with_sizes.count(b'I64 sprite_elem_base_sizes[') == 1
sprite_plot = (root/'upstream/pinned/System/Gr/GrSpritePlot.ZC').read_bytes()
sprite_helpers = extract_sprite_interpolation(sprite_new, sprite_plot)
assert b'public I64 SpriteSize(U8 *elems)' in sprite_helpers
assert b'public U8 *SpriteInterpolate(F64 t, U8 *elems0, U8 *elems1)' in sprite_helpers
assert b'records < 65536 && total < 16777216' in sprite_helpers
assert b't < 0.0 || t > 1.0' in sprite_helpers
assert b'SpriteInterpolationStreamsCompatible' in sprite_helpers
sprite_bridge_with_interpolation = inject_sprite_interpolation(
    sprite_bridge_with_sizes, sprite_new, sprite_plot)
assert sprite_helpers in sprite_bridge_with_interpolation
assert sprite_bridge_with_interpolation.count(b'public U8 *SpriteInterpolate(') == 1
assert sprite_bridge_with_interpolation.index(b'public I64 SpriteSize(') < \
        sprite_bridge_with_interpolation.index(b'public CDocEntry *DocSprite(')
sprite_matrix_helpers = extract_sprite_matrix_draw()
assert b'public U0 Sprite3Mat4x4B(' in sprite_matrix_helpers
assert b'Sprite3MatrixBounded(matrix)' in sprite_matrix_helpers
assert b'dc->r = old_matrix;' in sprite_matrix_helpers
sprite_bridge_with_matrix = inject_sprite_matrix_draw(sprite_bridge_with_interpolation)
assert sprite_bridge_with_matrix.endswith(sprite_matrix_helpers)
for symbol in (b'Sprite3XB', b'Sprite3YB', b'Sprite3ZB'):
    assert sprite_matrix_helpers.count(b'public U0 ' + symbol + b'(') == 1
try:
    inject_sprite_matrix_draw(sprite_bridge_with_matrix)
except ValueError:
    pass
else:
    raise AssertionError('sprite matrix injection accepted duplicate API')
for changed_new, changed_plot in (
    (sprite_new.replace(b'public I64 SpriteSize(U8 *elems)',
                        b'public I64 SpriteExtent(U8 *elems)', 1), sprite_plot),
    (sprite_new, sprite_plot.replace(b'public U8 *SpriteInterpolate(',
                                     b'public U8 *SpriteMix(', 1)),
    (sprite_new, sprite_plot.replace(b'case SPT_BITMAP:', b'case SPT_IMAGE:')),
):
    try:
        extract_sprite_interpolation(changed_new, changed_plot)
    except ValueError:
        pass
    else:
        raise AssertionError('sprite helper extraction accepted changed upstream input')
keyboard = (root / 'upstream/pinned/Kernel/SerialDev/Keyboard.ZC').read_bytes()
printer = (root / 'upstream/pinned/Kernel/StrPrint.ZC').read_bytes()
keyboard_bridge = extract_keyboard_helpers(keyboard, printer)
assert b'asm {' not in keyboard_bridge
assert b'U0 KbdCmdSend' not in keyboard_bridge
assert b'CH_SHIFT_ESC' in keyboard_bridge
assert b'U8 *CatPrint(' in keyboard_bridge
assert b'*buf = 0;\n\tif (ch < 0 || ch > 255)' in keyboard_bridge
for changed_keyboard, changed_printer in [
    (keyboard.replace(b'I64 Char2ScanCode(', b'I64 OtherScanCode(', 1), printer),
    (keyboard.replace(b'NORMAL_KEY_SCAN_DECODE_TABLE::', b'OTHER_KEY_SCAN_DECODE_TABLE::', 1), printer),
    (keyboard, printer.replace(b'U8 *CatPrint(', b'U8 *OtherPrint(', 1)),
]:
    try:
        extract_keyboard_helpers(changed_keyboard, changed_printer)
    except ValueError:
        pass
    else:
        raise AssertionError('keyboard helper extraction accepted changed dependencies')
task_bridge = (root / 'upstream/bootstrap/System/TaskBridge.ZC').read_text()
assert 'I64 best_score, best_score0, best_score1;' in task_bridge
for sound_compat in ('U0 Play(U8 *music=NULL, ...)', 'U0 SoundTaskEndCB()',
                     'U0 Sweep(I64 frequency, I64 duration, I64 end_frequency)',
                     'Bool FileOcc(U8 *needle, U8 *filename, U8 *flags="")'):
    assert sound_compat in task_bridge
full_compositor_start = task_bridge.index('Bool BootstrapTryUpstreamGrUpdateScreen()')
full_compositor_end = task_bridge.index('\nU0 BootstrapGrUpdateTasks()', full_compositor_start)
full_compositor = task_bridge[full_compositor_start:full_compositor_end]
assert ('ZcFbShellTextCompose(gr.text_base, TEXT_COLS);\n'
        '    bootstrap_gr_text_updates++;') in full_compositor
pixel_visible_start = task_bridge.index('Bool BootstrapWinPixelVisible(')
pixel_visible_end = task_bridge.index('\nBool TextChar(', pixel_visible_start)
pixel_visible = task_bridge[pixel_visible_start:pixel_visible_end]
assert 'bootstrap_win_pixel_z_buf[y * 800 + x] <= task->win_z_num' in pixel_visible
assert 'x >= 800 || y < 0 || y >= 600' in pixel_visible
pixel_function_start = task_bridge.index('Bool BootstrapGraphPixelAt(')
pixel_function_end = task_bridge.find('\n}', pixel_function_start) + 2
assert 'BootstrapWinPixelVisible(task,' in task_bridge[
        pixel_function_start:pixel_function_end]
rect_function_start = task_bridge.rindex('Bool GrRect(')
rect_function_end = task_bridge.find('\n}', rect_function_start) + 2
assert 'BootstrapGraphPixelAt(dc,' in task_bridge[
        rect_function_start:rect_function_end]
win_z_start = task_bridge.index('U0 WinZBufUpdate()')
win_z_end = task_bridge.index('\nBool BootstrapWinIsUncovered(', win_z_start)
win_z_update = task_bridge[win_z_start:win_z_end]
assert 'MemSet(bootstrap_win_pixel_z_buf, 0, 800 * 600 * sizeof(U16));' in win_z_update
assert 'bootstrap_win_pixel_z_buf[pixel_index++] = z;' in win_z_update
assert 'ZcFbSetWinPixelZBuffer(bootstrap_win_pixel_z_buf);' in win_z_update
wallpaper_start = task_bridge.index('I64 BootstrapWallpaperBg(')
wallpaper_end = task_bridge.index('\nU0 BootstrapBindGrTaskTables(', wallpaper_start)
wallpaper = task_bridge[wallpaper_start:wallpaper_end]
assert 'U32 BootstrapWallpaperCell(U32 cell, I64 x, I64 y)' in wallpaper
assert 'if (!z_buf[index + x])' in wallpaper
assert 'cells[index + x] = BootstrapWallpaperCell(' in wallpaper
assert 'active_gr->fp_wall_paper != &BootstrapWinPaper' in wallpaper
assert 'active_gr->fp_wall_paper = &BootstrapWinPaper;' in task_bridge
assert task_bridge.count('BootstrapWallpaperApply();') >= 3
assert 'BootstrapWallpaperChecks() != 0x2a' in (root / 'tests/compat/GrScreenFinalUpdate.ZC').read_text()
upstream_grscreen = (root / 'upstream/pinned/System/Gr/GrScreen.ZC').read_text()
assert 'if (gr.fp_wall_paper)\n\t\t\t\t(*gr.fp_wall_paper)(task);' in upstream_grscreen
kernel_stub = (root / 'src/kernel_stub.c').read_text()
assert 'void zeal_fb_set_win_pixel_z_buffer(const uint16_t *z_buf)' in kernel_stub
assert 'g_fb_win_pixel_z_buf[(size_t)y * 800u + x] <= task_z' in kernel_stub
runtime = (root / 'src/zc/runtime.c').read_text()
assert '{"ZcFbSetWinPixelZBuffer",' in runtime
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

docrecalc_source = (root / 'upstream/pinned/System/DolDoc/DocRecalc.ZC').read_bytes()
docrecalc_overlay = translate_docrecalc(docrecalc_source)
assert docrecalc_overlay != docrecalc_source
assert docrecalc_overlay.count(b'DCDel(dc);') == docrecalc_source.count(b'DCDel(dc);') + 2
assert docrecalc_overlay.count(b'Free(depth_buf);') == docrecalc_source.count(b'Free(depth_buf);') + 2
entry_trim = docrecalc_overlay.index(b'DocDelToEntry')
entry_cleanup = docrecalc_overlay.index(b'DCDel(dc);', entry_trim)
entry_recalc = docrecalc_overlay.index(b'DocRecalc(doc, recalc_flags);', entry_trim)
assert entry_cleanup < entry_recalc
count_trim = docrecalc_overlay.index(b'DocDelToNum')
count_cleanup = docrecalc_overlay.index(b'DCDel(dc);', count_trim)
count_recalc = docrecalc_overlay.index(b'DocRecalc(doc, recalc_flags);', count_trim)
assert count_cleanup < count_recalc
try:
    translate_docrecalc(docrecalc_source.replace(
        b'DocDelToNum(doc, i);', b'DocDelToNum(doc, i - 1);', 1))
except ValueError:
    pass
else:
    raise AssertionError('DocRecalc overlay accepted a changed trim path')
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
assert text.count('ZcGrRopEquU8NoClipping(text.font,') == 2
assert not re.search(r'(?<!Zc)GrRopEquU8NoClipping\(', text)
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

message_source = (root / 'upstream/pinned/Kernel/SerialDev/Message.ZC').read_bytes()
message_overlay = translate_message(message_source)
assert message_overlay != message_source
assert b'ZcCtrlAltDispatch(res, arg1, arg2, task)' in message_overlay
assert b'JobDel(tmpc);\n\t\t\t\tif (!ZcCtrlAltDispatch' in message_overlay
try:
    translate_message(message_source.replace(
        b'if ((res != MESSAGE_KEY_DOWN ||',
        b'if ((res != MESSAGE_NULL ||', 1))
except ValueError:
    pass
else:
    raise AssertionError('Message overlay accepted a changed pinned message dequeue')

bitmap_source = (root / 'upstream/pinned/System/Gr/GrBitMap.ZC').read_bytes()
bitmap_overlay = translate_grbitmap(bitmap_source)
bitmap_text = bitmap_overlay.decode('utf-8')
assert bitmap_overlay != bitmap_source
ext_start = bitmap_text.index('public CDC *DCExt(')
ext_end = bitmap_text.index('\npublic CDC *DCDiff(', ext_start)
ext_text = bitmap_text[ext_start:ext_end]
assert 'if (dc->flags & DCF_SCREEN_BITMAP)\n\t\tGrBlot(res, -x1, -y1, dc);' in ext_text
assert 'copy_x1 = x1;' in ext_text and 'if (copy_x1 < 0)' in ext_text
assert 'copy_y1 = y1;' in ext_text and 'if (copy_y1 < 0)' in ext_text
assert 'if (copy_x2 >= dc->width)' in ext_text
assert 'if (copy_y2 >= dc->height)' in ext_text
assert 'if (copy_x1 <= copy_x2 && copy_y1 <= copy_y2)' in ext_text
assert 'for (row = 0; row <= copy_y2 - copy_y1; row++)' in ext_text
assert 'DCNew((width + 7) & ~7, y2 - y1 + 1, task)' in ext_text
assert 'res->width = width;' in ext_text
assert 'res->body + (copy_y1 + row - y1) *' in ext_text
assert 'dc->body + (copy_y1 + row) *' in ext_text
assert 'width = copy_x2 - copy_x1 + 1;' in ext_text
blot_start = bitmap_text.index('public I64 GrBlot(')
blot_end = bitmap_text.index('\n#help_index "Graphics/Device Contexts"\n\nU8 *GrBitMap4ToBitMap8', blot_start)
blot_text = bitmap_text[blot_start:blot_end]
assert blot_text.count('if (img->flags & DCF_NO_TRANSPARENTS)') == 3
assert 'if ((img->flags & DCF_NO_TRANSPARENTS) && (-x & 7)' in blot_text
assert 'dc->color.c0.color = color.c0.color & c;' in blot_text
assert 'if (win_z_num < *win_z_buf_ptr)' in blot_text
assert 'case ROPB_COLLISION: //TODO: Might want to check win_z_buf' not in blot_text
assert 'win_task->win_z_num >= gr.win_z_buf[' in blot_text

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

filemgr_source = (root / 'upstream/pinned/System/BlkDev/FileMgr.ZC').read_bytes()
filemgr_translated = translate_filemgr(filemgr_source)
filemgr_text = filemgr_translated.decode('utf-8')
assert filemgr_translated != filemgr_source
assert 'FMRebuildDocSourceVolume(doc, _head);' in filemgr_text
assert 'BootstrapSourceFilesTree();' in filemgr_text
assert 'extern Bool BootstrapGuiTerminalEditFile(U8 *path);' in filemgr_text
assert '\tif (blkdev.drvs)\n\t\tfor (i = 0; i < DRIVES_NUM; i++)' in filemgr_text
assert '\t\tif (drive->bd)\n\t\t{' in filemgr_text
assert '\tif (doc2)\n\t\tDocReset(doc2, TRUE);' in filemgr_text
assert filemgr_text.count(
    'tmpde->user_data(CDocEntry *)->user_data = tmpde;') == 3
assert 'root->user_data(CDocEntry *)->user_data = root;' in filemgr_text
assert 'U=0x%X' not in filemgr_text
assert ('(tmpde = doc->cur_entry->user_data) && tmpde->full_name' in
        filemgr_text)
assert ('BootstrapGuiTerminalEditFile(tmpde->full_name);' in
        filemgr_text)
assert '\t\t\t\t\t\t\tPutKey(ch, sc);' in filemgr_text
assert 'bootstrap_filemgr_key_events' not in filemgr_text
assert ('BootstrapGuiTerminalEditFile(\n'
        '\t\t\t\t\t\t\t\tdoc_e->user_data(CDirEntry *)->full_name);' in
        filemgr_text)
assert 'Ed(tmpde->full_name);' not in filemgr_text
assert 'Ed(doc_e->user_data(CDirEntry *)->full_name);' not in filemgr_text
try:
    translate_filemgr(filemgr_source.replace(
        b'drive = &blkdev.drvs[i];', b'drive = &blkdev.drvs[j];', 1))
except ValueError:
    pass
else:
    raise AssertionError('FileMgr overlay accepted a changed drive lookup')
try:
    translate_filemgr(filemgr_source.replace(
        b'\t\t\tFMRebuildDocDrive(Drive2Letter(drive), doc, _head, TRUE);',
        b'\t\t\tFMRebuildDocDrive(Drive2Letter(drive), doc, _head, FALSE);', 1))
except ValueError:
    pass
else:
    raise AssertionError('FileMgr overlay accepted a changed drive-tree rebuild')

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
assert b'Install to a 128 MiB UTM/QEMU VirtIO disk: zinstall, then zinstall ERASE.' in once_overlay
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
assert b'I64 result = ZcSystemTaskStart(&ServerTaskCont);' in server_slice
assert b'if (result == 42) LBts(&sys_run_level, RLf_SYSTEM_SERVER);' in server_slice
assert b'return result;' in server_slice
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
assert 'task->fpu_mmx = MAlloc(sizeof(CFPU), task);' in ktask_overlay
assert 'ZcTaskFpuInit(task->fpu_mmx);' in ktask_overlay
assert 'ZcTaskStrNew(blkdev.home_dir + 2, task)' in ktask_overlay
assert 'ZcTaskStrNew("/Home", task)' in ktask_overlay
assert 'task->rand_seed = TSCGet;' in ktask_overlay
assert 'LBtr(&task->task_flags, TASKf_NONTIMER_RAND);' in ktask_overlay
assert 'MAllocAligned(sizeof(CFPU)' not in ktask_overlay
assert 'SYS_FIXED_AREA + offset(CSysFixedArea.init_fpu_mmx)' not in ktask_overlay
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
assert (b'if (!ZcDocFormField(class_name, 0, NULL))\n'
        b'\t\treturn FALSE;\n\tdoc = DocNew;') in docform_overlay
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

tothefront_fire = (root/'upstream/pinned/Apps/ToTheFront/TTFFire.ZC').read_bytes()
tothefront_overlay = translate_tothefront_sentinel(tothefront_fire)
assert b'while (tmpi != &indirect_head)' in tothefront_overlay
assert b'while (tmpi != *indirect_head)' not in tothefront_overlay
try:
    translate_tothefront_sentinel(tothefront_fire.replace(
        b'while (tmpi != *indirect_head)', b'while (tmpi != *other_head)'))
except ValueError:
    pass
else:
    raise AssertionError('ToTheFront queue overlay accepted changed pinned source')
print('PASS guarded ToTheFront sentinel-address source overlay', flush=True)

tothefront_draw = (root/'upstream/pinned/Apps/ToTheFront/TTFDraw.ZC').read_bytes()
tothefront_draw_overlay = translate_tothefront_draw(tothefront_draw)
assert b'F64 ToTheFrontSaw(F64 t, F64 period)' in tothefront_draw_overlay
assert b'Bool ToTheFrontBlink(F64 hz)' in tothefront_draw_overlay
assert b'ToTheFrontSaw(4 * tS, 2.0)' in tothefront_draw_overlay
assert b'ToTheFrontSaw(4 * tS + 1.0, 2.0)' in tothefront_draw_overlay
assert b'ToTheFrontBlink(5)' in tothefront_draw_overlay
assert b'ToTheFrontBlink(10)' in tothefront_draw_overlay
assert b'    Saw(4 * tS' not in tothefront_draw_overlay
try:
    translate_tothefront_draw(tothefront_draw.replace(
        b'Saw(4 * tS, 2.0)', b'Saw(5 * tS, 2.0)'))
except ValueError:
    pass
else:
    raise AssertionError('ToTheFront drawing overlay accepted changed pinned source')
print('PASS guarded ToTheFront pure waveform helper overlay', flush=True)

tothefront_init = (root/'upstream/pinned/Apps/ToTheFront/TTFInit.ZC').read_bytes()
tothefront_init_overlay = translate_tothefront_init(tothefront_init)
assert b'MemSet(terrain, PLAINS, map_rows * map_cols * sizeof(U8));' in tothefront_init_overlay
assert tothefront_init_overlay.count(b'map_rows * map_cols * sizeof(Bool)') == 3
try:
    translate_tothefront_init(tothefront_init.replace(
        b'MemSet(terrain, PLAINS, sizeof(terrain));',
        b'MemSet(terrain, PLAINS, sizeof(other));', 1))
except ValueError:
    pass
else:
    raise AssertionError('ToTheFront map overlay accepted changed pinned source')
print('PASS guarded ToTheFront runtime-sized map initialization overlay', flush=True)

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
assert b'U0 (*old_draw_it)(CTask *task, CDC *dc) = Fs->draw_it;' in zsplash_overlay
assert b'I32 *bootstrap_zsplash_depth_buf;' in zsplash_overlay
assert b'bootstrap_zsplash_depth_buf = MAlloc(dc->width_internal * dc->height * sizeof(I32), dc->mem_task);' in zsplash_overlay
assert b'dc->depth_buf = bootstrap_zsplash_depth_buf;' in zsplash_overlay
assert b'dc->depth_buf = NULL;' in zsplash_overlay
assert b'Free(bootstrap_zsplash_depth_buf);' in zsplash_overlay
assert zsplash_overlay.index(b'Sprite3(dc, 0, 0, 0, $IB,"<1>",BI=1$);') < \
       zsplash_overlay.index(b'Free(bootstrap_zsplash_depth_buf);')
assert zsplash_overlay.count(b'Free(bootstrap_zsplash_depth_buf);') == 1
zsplash_loop_end = '\t\tΦ += .03;\n\t\tWinToTop;\n\t}\n'.encode('utf-8')
assert zsplash_overlay.index(zsplash_loop_end) < \
       zsplash_overlay.index(b'Free(bootstrap_zsplash_depth_buf);')
assert zsplash_overlay.index(b'while (!CharScan)') < \
       zsplash_overlay.index(b'Fs->draw_it = old_draw_it;')
assert b'Simplicity Perfected' in zsplash_overlay
assert b'Kill(bg_task)' not in zsplash_overlay and b'Kill(splash_task)' not in zsplash_overlay
gui_terminal = (root / 'upstream/bootstrap/System/GuiTerminal.ZC').read_bytes()
assert b'BootstrapGuiTerminalExeDocFile(' in gui_terminal and b'::/System/ZSplash.ZC' not in gui_terminal
assert b'BootstrapHomeStartUpTasks() != 0x2a' in gui_terminal
for menu_item in (b'Zoom In', b'Zoom Out', b'Reset View', b'Pan Left',
                  b'Pan Right', b'Pan Up', b'Pan Down', b'Paint Demo'):
    assert menu_item in gui_terminal
assert b'BootstrapGuiTerminalZoomPanChecks()' in gui_terminal
assert b'active_gr->sx = ClampI64(active_gr->sx, 0, max_x);' in gui_terminal
assert b'active_gr->sy = ClampI64(active_gr->sy, 0, max_y);' in gui_terminal
assert b'if (12 <= action <= 18)' in gui_terminal
assert b'if (action == 19)' in gui_terminal
assert b'BootstrapGuiTerminalMenuAction(19)' in gui_terminal
print('PASS zoom and bounded viewport pan controls in the boot OS menu')
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
