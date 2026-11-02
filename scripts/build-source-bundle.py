#!/usr/bin/env python3
"""Build deterministic read-only source module plus documented ARM64 overlays."""
import hashlib
import io
import json
from pathlib import Path
import sys
import tarfile
from arm64_source import translate_docfile, translate_debug_compat, extract_source_dates, extract_string_utilities, translate_budget_strings, translate_span_worker_cleanup, translate_keepaway_nearest_ball, translate_xcaliber_shot_return, translate_xcaliber_draw_visibility, translate_xcaliber_key_descriptors, translate_xcaliber_fire_trace, translate_xcaliber_escape_scan, translate_xcaliber_held_input, translate_tothefront_draw, translate_tothefront_init, translate_tothefront_player_pick, translate_tothefront_ai, translate_tothefront_sentinel, extract_keyboard_helpers, translate_math, translate_sysrep_processor, translate_pick3d_class_terminator, translate_classmeta_callback, translate_mandel_single_cpu, translate_lighttable_memory_prompt

from arm64_source import (extract_home_set, extract_home_startup, extract_home_startup_tasks, inject_home_startup_tasks, extract_version_scalars, extract_source_prompt, extract_source_training, extract_server_task, extract_user_task, translate_doctree, translate_registry, translate_docterm, translate_doced, translate_docfind, translate_docform, translate_docpopup, translate_docrecalc, translate_grbitmap,
                          translate_autocomplete_task, translate_grdc, translate_grscreen, translate_ktask, translate_mathode, translate_zsplash,
                          translate_winmgr, translate_win_focus_trace, translate_job_focus_trace, translate_sched_focus_trace, translate_filemgr, translate_home_once,
                          translate_grpalette, translate_message,
                          translate_stock_game_math_calls,
                          translate_empty_label_statement,
                          translate_wenceslas, translate_progress,
                          inject_sprite_elem_base_sizes,
                          inject_sprite_interpolation, inject_sprite_matrix_draw,
                          inject_sprite_wrappers)

root = Path(__file__).resolve().parent.parent
manifest = json.loads((root / 'upstream/pinned/UPSTREAM.json').read_text())
entries = {}
home_startup_tasks = None
for name, expected in manifest['files'].items():
    rel = name.removeprefix('src/')
    data = (root / 'upstream/pinned' / rel).read_bytes()
    if hashlib.sha256(data).hexdigest() != expected:
        raise SystemExit(f'Pinned source changed: {name}')
    if rel == 'HomeSys.ZC':
        entries['System/HomeStartUp.ZC'] = extract_home_startup(data)
        home_startup_tasks = extract_home_startup_tasks(data)
    if rel == 'System/Debug.ZC':
        try:
            kernel_debug = (root / 'upstream/pinned/Kernel/KDebug.ZC').read_bytes()
            data = translate_debug_compat(data, kernel_debug)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Debug overlay no longer applies: {exc}') from exc
    if rel == 'Apps/Budget/BgtStrs.ZC':
        data = translate_budget_strings(data)
    if rel == 'System/Math/Math.ZC':
        data = translate_math(data)
    if rel == 'System/Utils/SysRep.ZC':
        try:
            data = translate_sysrep_processor(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 SysRep overlay no longer applies: {exc}') from exc
    if rel in ('Compiler/Lex.ZC', 'Compiler/CompilerB.HH'):
        # The pinned signature stores an optional output length but the
        # implementation dereferences it. Preserve that API correctly on
        # the AArch64 source volume, including calls that omit the argument.
        if rel == 'Compiler/Lex.ZC':
            needle = b'U8 *LexFirstRemove(CCompCtrl *cc, U8 *marker, I64 _len=NULL)'
            replacement = b'U8 *LexFirstRemove(CCompCtrl *cc, U8 *marker, I64 *_len=NULL)'
        else:
            needle = b'public extern U8\t\t*LexFirstRemove(CCompCtrl *cc, U8 *marker, I64 _len=NULL);'
            replacement = b'public extern U8\t\t*LexFirstRemove(CCompCtrl *cc, U8 *marker, I64 *_len=NULL);'
        if data.count(needle) != 1:
            raise SystemExit('ARM64 LexFirstRemove pointer overlay no longer applies')
        data = data.replace(needle, replacement)
    if rel == 'Compiler/Lex.ZC':
        numeric_range = b'\t\t\tcase 128...255:'
        if data.count(numeric_range) != 1:
            raise SystemExit('ARM64 lexer byte-range overlay no longer applies')
        byte_cases = b'\n'.join(b'\t\t\tcase 0x%02X:' % value for value in range(0x80, 0x100))
        data = data.replace(numeric_range, byte_cases)
    if rel == 'Apps/Span/SpanMain.ZC':
        data = translate_span_worker_cleanup(data)
    if rel == 'Apps/KeepAway/KeepAway.ZC':
        try:
            data = translate_keepaway_nearest_ball(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 KeepAway overlay no longer applies: {exc}') from exc
    if rel == 'Apps/X-Caliber/X-Caliber.ZC':
        try:
            data = translate_xcaliber_shot_return(data)
            data = translate_xcaliber_draw_visibility(data)
            data = translate_xcaliber_key_descriptors(data)
            data = translate_xcaliber_fire_trace(data)
            data = translate_xcaliber_escape_scan(data)
            data = translate_xcaliber_held_input(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 X-Caliber overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/BomberGolf.ZC':
        try:
            data = translate_stock_game_math_calls(
                data, blink_count=3, full_tri_count=1)
            data = translate_empty_label_statement(data, b'bm_done')
        except ValueError as exc:
            raise SystemExit(f'ARM64 BomberGolf math overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/CastleFrankenstein.ZC':
        try:
            data = translate_stock_game_math_calls(data, tri_count=1)
            data = translate_empty_label_statement(data, b'fs_done')
        except ValueError as exc:
            raise SystemExit(f'ARM64 CastleFrankenstein math overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/DunGen.ZC':
        try:
            data = translate_stock_game_math_calls(data, blink_count=1, tri_count=2)
        except ValueError as exc:
            raise SystemExit(f'ARM64 DunGen math overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/Talons.ZC':
        try:
            data = translate_stock_game_math_calls(data, tri_count=1,
                                                   sign_count=2)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Talons math overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/Varoom.ZC':
        try:
            data = translate_stock_game_math_calls(data, log2_count=1)
            data = translate_empty_label_statement(data, b'vr_done')
        except ValueError as exc:
            raise SystemExit(f'ARM64 Varoom math overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/Wenceslas.ZC':
        try:
            data = translate_wenceslas(data)
            data = translate_empty_label_statement(data, b'ws_done')
        except ValueError as exc:
            raise SystemExit(f'ARM64 Wenceslas prompt overlay no longer applies: {exc}') from exc
    if rel == 'Demo/PhoneNumWords.ZC':
        # This lookup is indexed only for A-Z and does not need a string
        # terminator. Use an explicit initializer because the ARM64 compiler
        # rejects this stock string-to-array initializer form.
        needle = b'U8 char_map[26]="22233344455566677778889999";'
        replacement = (b"U8 char_map[26]={'2','2','2','3','3','3','4','4','4',"
                       b"'5','5','5','6','6','6','7','7','7','7','8','8','8',"
                       b"'9','9','9','9'};")
        if data.count(needle) != 1:
            raise SystemExit('ARM64 PhoneNumWords string extent overlay no longer applies')
        data = data.replace(needle, replacement)
    if rel == 'Demo/Progress.ZC':
        try:
            data = translate_progress(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Progress overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/RawHide.ZC':
        try:
            data = translate_stock_game_math_calls(data, clamp_u64_count=1)
        except ValueError as exc:
            raise SystemExit(f'ARM64 RawHide math overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/BlackDiamond.ZC':
        try:
            data = translate_empty_label_statement(data, b'bd_done')
        except ValueError as exc:
            raise SystemExit(f'ARM64 BlackDiamond label overlay no longer applies: {exc}') from exc
    if rel == 'Demo/ClassMeta.ZC':
        try:
            data = translate_classmeta_callback(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 ClassMeta overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Graphics/Pick3D.ZC':
        try:
            data = translate_pick3d_class_terminator(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Pick3D source overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Graphics/Mandel.ZC':
        try:
            data = translate_mandel_single_cpu(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 Mandel source overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Graphics/LightTable.ZC':
        try:
            data = translate_lighttable_memory_prompt(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 LightTable memory overlay no longer applies: {exc}') from exc
    if rel == 'Demo/Games/ZoneOut.ZC':
        try:
            data = translate_empty_label_statement(data, b'zo_done')
        except ValueError as exc:
            raise SystemExit(f'ARM64 ZoneOut label overlay no longer applies: {exc}') from exc
    if rel == 'Apps/ToTheFront/TTFFire.ZC':
        data = translate_tothefront_sentinel(data)
    if rel == 'Apps/ToTheFront/TTFDraw.ZC':
        data = translate_tothefront_draw(data)
    if rel == 'Apps/ToTheFront/TTFInit.ZC':
        data = translate_tothefront_init(data)
    if rel in ('Apps/ToTheFront/AIs/Human.ZC',
               'Apps/ToTheFront/AIs/SimpleAI.ZC'):
        try:
            data = translate_tothefront_ai(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 ToTheFront AI overlay no longer applies: {exc}') from exc
    if rel == 'Apps/ToTheFront/ToTheFront.ZC':
        try:
            data = translate_tothefront_player_pick(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 ToTheFront player picker overlay no longer applies: {exc}') from exc
    if rel == 'Kernel/BlkDev/DiskDirB.ZC':
        entries['System/SourceHome.ZC'] = extract_home_set(data)
    if rel == 'Kernel/SerialDev/Message.ZC':
        try:
            data = translate_message(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 keyboard message overlay no longer applies: {exc}') from exc
    if rel == 'Once.ZC':
        # Seed the same editable home startup file installed by upstream.
        # Disk/RAM and DVD boot queue this file in the first graphical
        # terminal. The ARM64 DVD overlay avoids the unported BIOS/AHCI
        # installer while preserving the live session.
        entries['Home/Once.ZC'] = translate_home_once(data)
    if rel == 'Kernel/KGlobals.ZC':
        entries['System/VersionScalars.ZC'] = extract_version_scalars(data)
    if rel == 'Kernel/StrB.ZC':
        entries['System/SourcePrompt.ZC'] = extract_source_prompt(data)
    if rel == 'System/Training.ZC':
        entries['System/SourceTraining.ZC'] = extract_source_training(data)
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
    elif rel == 'System/Gr/GrPalette.ZC':
        try:
            data = translate_grpalette(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 palette overlay no longer applies: {exc}') from exc
    elif rel == 'System/WinMgr.ZC':
        try:
            data = translate_winmgr(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 window-manager overlay no longer applies: {exc}') from exc
    elif rel == 'System/Win.ZC':
        try:
            data = translate_win_focus_trace(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 window focus trace no longer applies: {exc}') from exc
    elif rel == 'Kernel/Job.ZC':
        try:
            data = translate_job_focus_trace(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 job focus trace no longer applies: {exc}') from exc
    elif rel == 'Kernel/Sched.ZC':
        try:
            data = translate_sched_focus_trace(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 scheduler focus trace no longer applies: {exc}') from exc
    elif rel == 'System/BlkDev/FileMgr.ZC':
        try:
            data = translate_filemgr(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 FileMgr overlay no longer applies: {exc}') from exc
    elif rel == 'System/Math/MathODE.ZC':
        try:
            data = translate_mathode(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 MathODE overlay no longer applies: {exc}') from exc
    elif rel == 'System/ZSplash.ZC':
        try:
            data = translate_zsplash(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 ZSplash overlay no longer applies: {exc}') from exc
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
    elif rel == 'System/DolDoc/DocRecalc.ZC':
        try:
            data = translate_docrecalc(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 source overlay no longer applies: {exc}') from exc
    elif rel == 'System/DolDoc/DocTerm.ZC':
        data = translate_docterm(data)
    elif rel == 'System/AutoComplete/ACTask.ZC':
        try:
            data = translate_autocomplete_task(data)
        except ValueError as exc:
            raise SystemExit(f'ARM64 AutoComplete overlay no longer applies: {exc}') from exc
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
        rel = path.relative_to(root / 'upstream/bootstrap').as_posix()
        data = path.read_bytes()
        if rel == 'System/Gr/SpriteBridge.ZC':
            try:
                data = inject_sprite_elem_base_sizes(
                    data, (root/'upstream/pinned/System/Gr/SpriteNew.ZC').read_bytes())
                data = inject_sprite_interpolation(
                    data,
                    (root/'upstream/pinned/System/Gr/SpriteNew.ZC').read_bytes(),
                    (root/'upstream/pinned/System/Gr/GrSpritePlot.ZC').read_bytes())
                data = inject_sprite_matrix_draw(data)
                data = inject_sprite_wrappers(
                    data, (root/'upstream/pinned/System/Gr/GrSpritePlot.ZC').read_bytes())
            except ValueError as exc:
                raise SystemExit(f'ARM64 sprite helper overlay no longer applies: {exc}') from exc
        if rel == 'System/DolDoc/DocFile.ZC':
            data = translate_docfile(data)
        entries[rel] = data
entries['System/StringUtilities.ZC'] = extract_string_utilities(
    entries['Kernel/StrA.ZC'], entries['Kernel/StrB.ZC'])
entries['System/DateBridge.ZC'] = extract_source_dates(
    entries['Kernel/KDate.ZC'], entries['Kernel/StrScan.ZC'], entries['Kernel/StrPrint.ZC'])
entries['System/KeyboardBridge.ZC'] = extract_keyboard_helpers(
    entries['Kernel/SerialDev/Keyboard.ZC'], entries['Kernel/StrPrint.ZC'])
# MakeHome's stock optional `~/...` includes fall back to root-level defaults
# when a user's /Home override is absent. Keep those unchanged upstream files
# available to the source resolver for that fallback.
for name in ('HomeLocalize.ZC', 'HomeWrappers.ZC'):
    entries[name] = (root / 'upstream/pinned' / name).read_bytes()
# Factory account data is the same file installed by Budget/Install.ZC. The
# source volume infers directories from their files; seed its default home
# before the unchanged installer checks for it.
entries['Home/Budget/Accts.DD'] = entries['Apps/Budget/Accts.DD']
if home_startup_tasks is not None:
    gui_terminal_path = 'System/GuiTerminal.ZC'
    gui_terminal = entries.get(gui_terminal_path)
    if gui_terminal is None:
        raise SystemExit('cannot place pinned HomeSys.StartUpTasks adapter')
    try:
        entries[gui_terminal_path] = inject_home_startup_tasks(
            gui_terminal, home_startup_tasks)
    except ValueError as exc:
        raise SystemExit(str(exc)) from exc
for path in sorted((root / 'tests/compat').glob('*')):
    if path.suffix in ('.ZC', '.HH', '.DD', '.txt'):
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
