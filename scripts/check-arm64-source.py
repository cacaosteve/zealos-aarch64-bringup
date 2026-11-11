#!/usr/bin/env python3
"""Verify guarded AArch64 overlays for pinned ZealOS sources."""
from pathlib import Path
import re
import struct
import sys
from arm64_source import extract_keyboard_helpers
from arm64_source import translate_docputs_exception_cleanup
from arm64_source import extract_string_utilities
from arm64_source import (extract_sprite_elem_base_sizes, inject_sprite_elem_base_sizes,
                          extract_sprite_interpolation, inject_sprite_interpolation,
                          extract_sprite_matrix_draw, inject_sprite_matrix_draw,
                          inject_sprite_wrappers)

from arm64_source import (extract_home_set, extract_home_startup, extract_home_startup_tasks, inject_home_startup_tasks, extract_server_task, extract_user_task, extract_ktask_stack_grow, extract_version_scalars, extract_source_prompt, extract_source_training, translate_autocomplete_task, translate_doctree, translate_registry, translate_docterm, translate_doced, translate_docfind, translate_docform, translate_docpopup, translate_docrecalc, translate_grbitmap, translate_tothefront_draw, translate_tothefront_init, translate_tothefront_sentinel, translate_math,
translate_tothefront_player_pick, translate_tothefront_ai, translate_filemgr, translate_grdc, translate_grpalette, translate_message, translate_grscreen, translate_ktask, translate_winmgr, translate_win_focus_trace, translate_job_focus_trace, translate_sched_focus_trace, translate_zsplash, translate_home_once, translate_span_worker_cleanup, translate_keepaway_nearest_ball, translate_stock_game_math_calls, translate_empty_label_statement, translate_wenceslas, translate_progress, translate_xcaliber_shot_return, translate_xcaliber_draw_visibility, translate_xcaliber_key_descriptors, translate_xcaliber_fire_trace, translate_xcaliber_escape_scan, translate_xcaliber_held_input)
from arm64_source import (translate_pick3d_class_terminator,
                          translate_classmeta_callback)
from arm64_source import translate_blackdiamond_math_dependency
from arm64_source import translate_polezeros_math_dependency
from arm64_source import (translate_mathode, translate_conversion_math,
                          translate_f32_math, translate_vec3_math,
                          translate_vec4_math, translate_mat4_math,
                          translate_ndarray_math, translate_math_test_dependencies)
from arm64_source import translate_debug_compat
from arm64_source import (translate_mandel_single_cpu,
                          translate_make_tos_dependency,
                          translate_interrupt_demo_svc, translate_andnotmod_benchmark,
                          translate_cpurep_arm64,
                          translate_pci_interrupts_gic_note,
                          translate_multicore_interrupts_cooperative,
                          translate_multicore_radix_single_cpu,
                          translate_multicore_loadtest_jit,
                          translate_multicore_lock_single_cpu,
                          translate_ring3_svc)
from arm64_source import (translate_tos_config_boot_memory,
                          translate_jukepuppet_song_mask)
from arm64_source import translate_elephant_walk_default_dc
from arm64_source import translate_fonted_draw_callback, translate_wallpaper_compiler_stat

root = Path(__file__).resolve().parent.parent
cpurep_source = (root / 'upstream/pinned/System/Utils/CPURep.ZC').read_bytes()
cpurep_overlay = translate_cpurep_arm64(cpurep_source)
assert b'SysRep(SMBIOSt_PROCESSOR)' not in cpurep_overlay
assert b'SMBIOS processor details are unavailable on this AArch64 platform.' in cpurep_overlay
assert b'AArch64 %03X active cores' in cpurep_overlay
try:
    translate_cpurep_arm64(cpurep_source.replace(b'SysRep(', b'Wrong(', 1))
except ValueError:
    pass
else:
    raise AssertionError('CPURep overlay must reject a changed SMBIOS call site')
print('PASS ARM64 CPURep avoids the x86 SMBIOS dependency and reports core/timer facts',
      flush=True)

string_utilities = extract_string_utilities(
    (root / 'upstream/pinned/Kernel/StrA.ZC').read_bytes(),
    (root / 'upstream/pinned/Kernel/StrB.ZC').read_bytes())
assert re.search(rb'U32\s+char_bmp_word\[16\]', string_utilities)
assert re.search(rb'U32\s+char_bmp_hex_numeric\[16\]', string_utilities)
assert re.search(rb'U32\s+char_bmp_white_space\[16\]', string_utilities)
print('PASS ARM64 string utility overlay exports pinned word/hex/whitespace bitmaps',
      flush=True)

fonted_source = (root / 'upstream/pinned/Demo/Graphics/FontEd.ZC').read_bytes()
fonted_overlay = translate_fonted_draw_callback(fonted_source)
assert b'U0\t\t(*old_draw_it)(CTask *task, CDC *dc) = Fs->draw_it;' in fonted_overlay
assert b'U8\t\t old_draw_it = Fs->draw_it;' not in fonted_overlay
try:
    translate_fonted_draw_callback(fonted_source.replace(b'old_draw_it', b'old_draw_fn', 1))
except ValueError:
    pass
else:
    raise AssertionError('FontEd overlay accepted changed callback save declaration')
print('PASS guarded FontEd ARM64 callback-width overlay', flush=True)

wallpaper_source = (root / 'upstream/pinned/System/WallPaper.ZC').read_bytes()
wallpaper_overlay = translate_wallpaper_compiler_stat(wallpaper_source)
assert wallpaper_overlay.count(b'"N/A"') == 2
assert b'cmp.compiled_lines' not in wallpaper_overlay
assert wallpaper_overlay.count(b'%12ts%12td%12td') == 2
assert b'WallPaperTaskMemAlloced(task)' in wallpaper_overlay
assert b'WallPaperTaskMemUsed(task)' in wallpaper_overlay
assert b'= TaskMemAlloced(task)' not in wallpaper_overlay
assert b'= TaskMemUsed(task)' not in wallpaper_overlay
assert b'return WallPaperTaskMemUsed(task);' in wallpaper_overlay
assert b'CSMBIOSSystemInfo' not in wallpaper_overlay
assert b'sys_bootloader_id' not in wallpaper_overlay
assert b"wall->host_ch = '?';" in wallpaper_overlay
assert b"wall->bootm_ch = 'U';" in wallpaper_overlay
assert b"wall->bootl_ch = 'L';" in wallpaper_overlay
assert b"wall->boots_ch = '?';" in wallpaper_overlay
try:
    translate_wallpaper_compiler_stat(wallpaper_source.replace(
        b'cmp.compiled_lines', b'compiler.lines', 1))
except ValueError:
    pass
else:
    raise AssertionError('Wallpaper overlay accepted changed compiler counter source')
print('PASS guarded ARM64 wallpaper with unavailable compiler counter shown as N/A', flush=True)

debug_overlay = translate_debug_compat(
    (root / 'upstream/pinned/System/Debug.ZC').read_bytes(),
    (root / 'upstream/pinned/Kernel/KDebug.ZC').read_bytes())
assert b'U0 Dump(U8 *addr, I64 count=0x80, Bool show_offset=TRUE)' in debug_overlay
assert b'ptr += j;' in debug_overlay and b'count -= j;' in debug_overlay
assert b'UpdateRegVarImg' not in debug_overlay
print('PASS ARM64 portable debugger overlay includes upstream Dump without x86 register helpers',
      flush=True)
classmeta_source = (root / 'upstream/pinned/Demo/ClassMeta.ZC').read_bytes()
classmeta_overlay = translate_classmeta_callback(classmeta_source)
assert b'output_fun &RankOut' not in classmeta_overlay

assert b'percentile 54.20;' in classmeta_overlay
assert b'" percentile: %.2f", field.percentile;' in classmeta_overlay
assert b'RankOut(*q);' in classmeta_overlay
try:
    translate_classmeta_callback(classmeta_source.replace(b'&RankOut', b'&Other', 1))
except ValueError:
    pass
else:
    raise AssertionError('ClassMeta overlay accepted changed callback source')
print('PASS guarded ClassMeta metadata and rank callback overlay', flush=True)
pick3d_source = (root / 'upstream/pinned/Demo/Graphics/Pick3D.ZC').read_bytes()
pick3d_overlay = translate_pick3d_class_terminator(pick3d_source)
assert pick3d_overlay.count(b'*dr; //Rounding error might eventually screw this up\n};\n\nclass PObj') == 1
try:
    translate_pick3d_class_terminator(pick3d_source.replace(b'\t*dr;', b'\t*delta;', 1))
except ValueError:
    pass
else:
    raise AssertionError('Pick3D overlay accepted changed pinned source')
print('PASS guarded Pick3D PCSprite class terminator overlay', flush=True)

mandel_source = (root / 'upstream/pinned/Demo/Graphics/Mandel.ZC').read_bytes()
mandel_overlay = translate_mandel_single_cpu(mandel_source)
assert b'I64 numCores = mp_count - 1 + (mp_count <= 1);' in mandel_overlay
assert b'if (mp_count > 1)' in mandel_overlay
assert b'JobQueue(&MandelFrame, i, i+1);' in mandel_overlay
assert b'MandelFrame(0);' in mandel_overlay
try:
    translate_mandel_single_cpu(mandel_source.replace(
        b'JobQueue(&MandelFrame, i, i+1);', b'JobQueue(&MandelFrame, i, i);', 1))
except ValueError:
    pass
else:
    raise AssertionError('Mandel overlay accepted changed worker dispatch')
print('PASS guarded Mandel single-CPU fallback preserves multi-CPU dispatch', flush=True)

elephant_source = (root / 'upstream/pinned/Demo/Games/ElephantWalk.ZC').read_bytes()
elephant_overlay = translate_elephant_walk_default_dc(elephant_source)
assert b'DCSymmetrySet(gr.dc, x, y, x, y - 1);' in elephant_overlay
try:
    translate_elephant_walk_default_dc(elephant_source.replace(
        b'DCSymmetrySet(, x, y, x, y - 1);',
        b'DCSymmetrySet(gr.dc, x, y, x, y - 1);', 1))
except ValueError:
    pass
else:
    raise AssertionError('ElephantWalk overlay accepted a changed symmetry call')
print('PASS guarded ElephantWalk active-CDC symmetry overlay', flush=True)

blackdiamond_source = (root / 'upstream/pinned/Demo/Games/BlackDiamond.ZC').read_bytes()
blackdiamond_overlay = translate_blackdiamond_math_dependency(blackdiamond_source)
blackdiamond_draw = b'U0 DrawIt(CTask *, CDC *dc)\n{\n\tI64  i;'
blackdiamond_expected = (b'#include "/System/Math/Math"\n' +
                         blackdiamond_source.replace(
                             blackdiamond_draw,
                             b'U0 DrawIt(CTask *, CDC *dc)\n{\n'
                             b'\tDCFill(dc, WHITE);\n\tI64  i;', 1))
assert blackdiamond_overlay == blackdiamond_expected
try:
    translate_blackdiamond_math_dependency(blackdiamond_source.replace(
        b'Cosh(', b'Cos(', 1))
except ValueError:
    pass
else:
    raise AssertionError('BlackDiamond overlay accepted changed math dependency')
print('PASS guarded BlackDiamond stock math dependency overlay', flush=True)

polezeros_source = (root / 'upstream/pinned/Demo/Graphics/PoleZeros.ZC').read_bytes()
polezeros_overlay = translate_polezeros_math_dependency(polezeros_source)
assert polezeros_overlay.startswith(b'#include "/System/Math/Math"\n')
assert polezeros_overlay.count(b'CPoly(') == 2
try:
    translate_polezeros_math_dependency(polezeros_source.replace(
        b'CPoly(', b'ComplexPoly(', 1))
except ValueError:
    pass
else:
    raise AssertionError('PoleZeros overlay accepted changed complex-math dependency')
print('PASS guarded PoleZeros shared complex-math dependency overlay', flush=True)

math_source = (root / 'upstream/pinned/System/Math/Math.ZC').read_bytes()
math_overlay = translate_math(math_source)
assert math_overlay.count(b'return Arg(1.0, s / c);') == 2
assert b'return ATan(s / c);' not in math_overlay
assert (b'public F64 Saw(F64 t, F64 period=1.0)' in math_overlay and
        b'if (!period)' in math_overlay and
        b'if (t < 0.0)' in math_overlay and
        b'return t / period;' in math_overlay)
assert b'public Bool Arm64Blink(F64 hz=2.5)' in math_overlay
assert b'(tS * hz) % 2.0 < 1.0' in math_overlay
assert b'public U64 Arm64ClampU64(U64 num, U64 lo, U64 hi)' in math_overlay
assert b'public F64 Arm64Tri(F64 t, F64 period)' in math_overlay
assert b'public F64 Arm64FullTri(F64 t, F64 period)' in math_overlay
assert b'public F64 Arm64Log2(F64 value)' in math_overlay
assert b'public F64 Arm64Sign(F64 value)' in math_overlay

make_tos_source = (root/'upstream/pinned/Demo/AcctExample/TOS/MakeTOS.ZC').read_bytes()
make_tos_overlay = translate_make_tos_dependency(make_tos_source)
assert b'#include "TOSExtern"' in make_tos_overlay
assert b'#include "TOSExt"' not in make_tos_overlay
try:
    translate_make_tos_dependency(make_tos_source.replace(
        b'#include "TOSExt"', b'#include "OtherExt"', 1))
except ValueError:
    pass
else:
    raise AssertionError('MakeTOS overlay accepted a changed include')
print('PASS guarded MakeTOS extension dependency correction')
tos_config_source = (root/'upstream/pinned/Demo/AcctExample/TOS/TOSConfig.ZC').read_bytes()
tos_config_overlay = translate_tos_config_boot_memory(tos_config_source)
assert b'CZXE *zxe = mem_boot_base' not in tos_config_overlay
assert b'switch (INS_REG_MACHINE_NUM)' not in tos_config_overlay
assert b'switch (0)' in tos_config_overlay
assert b'DefinePrint("TOS_DEBUG_DISTRO", "0");' in tos_config_overlay
assert b'DefinePrint("TOS_DEBUG_DISTRO_END", "0");' in tos_config_overlay
assert b'DefinePrint("CONFIG_DEBUG_DISTRO_FILE"' in tos_config_overlay
try:
    translate_tos_config_boot_memory(tos_config_source.replace(
        b'mem_boot_base', b'boot_image_base', 1))
except ValueError:
    pass
else:
    raise AssertionError('TOSConfig overlay accepted changed boot-memory source')
print('PASS guarded ARM64 TOSConfig disabled-debug-distro overlay')
interrupt_demo_source = (root/'upstream/pinned/Demo/Lectures/InterruptDemo.ZC').read_bytes()
interrupt_demo_overlay = translate_interrupt_demo_svc(interrupt_demo_source)
assert b'\nasm {' not in interrupt_demo_overlay
assert b'_extern _ARM64_SVC_BENCH U0 Arm64SvcBench();' in interrupt_demo_overlay
assert b'_extern _ARM64_SVC_BENCH_COUNT U64 Arm64SvcBenchCount();' in interrupt_demo_overlay
assert b'Arm64SvcBench;' in interrupt_demo_overlay
assert b'Arm64SvcBenchCount' in interrupt_demo_overlay
assert b'AArch64 SVC timer ticks:' in interrupt_demo_overlay
assert b'IntEntrySet' not in interrupt_demo_overlay
try:
    translate_interrupt_demo_svc(interrupt_demo_source.replace(
        b'I_F_UNARY_MINUS', b'I_F_NEGATE', 1))
except ValueError:
    pass
else:
    raise AssertionError('InterruptDemo overlay accepted a changed benchmark')
print('PASS guarded AArch64 InterruptDemo SVC benchmark overlay')
andnotmod_source = (root/'upstream/pinned/Demo/Lectures/AndNotMod.ZC').read_bytes()
andnotmod_overlay = translate_andnotmod_benchmark(andnotmod_source)
assert b'#define SAMPLE_SIZE \t\t1000000' in andnotmod_overlay
assert b'Remainder Version #1 Timer Ticks' in andnotmod_overlay
assert b'Remainder Version #2 Timer Ticks' in andnotmod_overlay
assert b'#define SAMPLE_SIZE \t\t100000000' not in andnotmod_overlay
try:
    translate_andnotmod_benchmark(andnotmod_source.replace(
        b'#define SAMPLE_SIZE \t\t100000000',
        b'#define SAMPLE_SIZE \t\t10000000', 1))
except ValueError:
    pass
else:
    raise AssertionError('AndNotMod overlay accepted changed sample size')
print('PASS guarded ARM64 AndNotMod timing and bounded TCG sample overlay')
pcie_source = (root/'upstream/pinned/Demo/Lectures/PCIInterrupts.ZC').read_bytes()
pcie_overlay = translate_pci_interrupts_gic_note(pcie_source)
assert b'IOAPIC' in pcie_overlay and b'GIC' in pcie_overlay
assert b'LAPIC_EOI' not in pcie_overlay and b'IOAPIC_REG' not in pcie_overlay
try:
    translate_pci_interrupts_gic_note(pcie_source.replace(
        b'#assert I_USER == 0x40', b'#assert I_USER == 0x41', 1))
except ValueError:
    pass
else:
    raise AssertionError('PCIInterrupts overlay accepted a changed source')
print('PASS guarded PCIInterrupts AArch64 GIC capability note')
multicore_interrupt_source = (root/'upstream/pinned/Demo/MultiCore/Interrupts.ZC').read_bytes()
multicore_interrupt_overlay = translate_multicore_interrupts_cooperative(
    multicore_interrupt_source)
assert b'RoundRobinTaskA' in multicore_interrupt_overlay
assert b'Yield;' in multicore_interrupt_overlay
assert b'MPInt(' not in multicore_interrupt_overlay
assert b'Guest logical CPUs: %d' in multicore_interrupt_overlay
try:
    translate_multicore_interrupts_cooperative(multicore_interrupt_source.replace(
        b'MPInt(int_on, 1);', b'MPInt(int_on, 2);', 1))
except ValueError:
    pass
else:
    raise AssertionError('MultiCore/Interrupts overlay accepted a changed source')
print('PASS guarded cooperative AArch64 MultiCore/Interrupts overlay')
multicore_radix_source = (root/'upstream/pinned/Demo/MultiCore/MPRadix.ZC').read_bytes()
multicore_radix_overlay = translate_multicore_radix_single_cpu(multicore_radix_source)
assert b'One guest CPU: running the radix stage with one worker.' in multicore_radix_overlay
assert b'b[0] = MAlloc(NUM * sizeof(I32));' in multicore_radix_overlay
assert b'MemCopy(b[0], arg1, NUM * sizeof(I32));' in multicore_radix_overlay
assert b'if (my_mp_count > 1)' in multicore_radix_overlay
assert b'throw(\'MultCore\')' not in multicore_radix_overlay
try:
    translate_multicore_radix_single_cpu(multicore_radix_source.replace(
        b'if (my_mp_count < 2)', b'if (my_mp_count < 3)', 1))
except ValueError:
    pass
else:
    raise AssertionError('MPRadix overlay accepted a changed source')
print('PASS guarded single-worker ARM64 MPRadix fallback')
loadtest_source = (root/'upstream/pinned/Demo/MultiCore/LoadTest.ZC').read_bytes()
loadtest_overlay = translate_multicore_loadtest_jit(loadtest_source)
assert b'ExeFile2("::/Demo/Print.ZC", CCF_JUST_LOAD);' in loadtest_overlay
assert (b'ExeFile2("::/Demo/Print.ZC", CCF_JUST_LOAD);\n'
        b'\t\tYield;') in loadtest_overlay
for job in (1, 2, 3):
    assert loadtest_overlay.count(b'Spawn(&Job%d,,, 0);' % job) == 1
assert b'Comp("Compiler")' not in loadtest_overlay
try:
    translate_multicore_loadtest_jit(loadtest_source.replace(
        b'Comp("Compiler");', b'Comp("Kernel");', 1))
except ValueError:
    pass
else:
    raise AssertionError('LoadTest overlay accepted a changed AOT compile call')
print('PASS guarded ARM64 LoadTest uses JIT and cooperative CPU-0 workers')
lock_source = (root/'upstream/pinned/Demo/MultiCore/Lock.ZC').read_bytes()
lock_overlay = translate_multicore_lock_single_cpu(lock_source)
assert lock_overlay.count(b'JobQueue(&MPUnlockedInc, NULL, 0, 0);') == 2
assert lock_overlay.count(b'JobQueue(&MPLockedInc, NULL, 0, 0);') == 2
assert b'COUNT \t0x1000000' in lock_overlay
try:
    translate_multicore_lock_single_cpu(lock_source.replace(
        b'JobQueue(&MPLockedInc, NULL, 1, 0);',
        b'JobQueue(&MPLockedInc, NULL, 2, 0);', 1))
except ValueError:
    pass
else:
    raise AssertionError('Lock overlay accepted a changed job target')
print('PASS guarded ARM64 Lock maps stock worker CPUs onto CPU 0')
ring3_source = (root/'upstream/pinned/Demo/Lectures/Ring3.ZC').read_bytes()
ring3_overlay = translate_ring3_svc(ring3_source)
assert b'asm {' not in ring3_overlay
assert b'_extern _ARM64_SVC_BENCH U0 Arm64SvcBench();' in ring3_overlay
assert b'_extern _ARM64_SVC_BENCH_COUNT U64 Arm64SvcBenchCount();' in ring3_overlay
assert b'Arm64SvcBench;' in ring3_overlay
assert b'EL0 user tasks are not implemented' in ring3_overlay
assert b'R3_START' not in ring3_overlay and b'INT_TO_RING0' not in ring3_overlay
try:
    translate_ring3_svc(ring3_source.replace(
        b'INT_TO_RING0::', b'INT_TO_PRIVILEGE::', 1))
except ValueError:
    pass
else:
    raise AssertionError('Ring3 overlay accepted changed pinned source')
print('PASS guarded AArch64 Ring3 SVC adaptation')
assert translate_stock_game_math_calls(
    b'x=Tri(t,1);y=FullTri(t,1);z=Log2(x);q=Sign(y);', tri_count=1,
    full_tri_count=1, log2_count=1, sign_count=1) == (
        b'x=Arm64Tri(t,1);y=Arm64FullTri(t,1);z=Arm64Log2(x);q=Arm64Sign(y);')
assert translate_stock_game_math_calls(b'if (Blink(5)) A();', blink_count=1) == b'if (Arm64Blink(5)) A();'
assert translate_stock_game_math_calls(b'x=ClampU64(x,0,9);', clamp_u64_count=1) == b'x=Arm64ClampU64(x,0,9);'
try:
    translate_stock_game_math_calls(b'Blink(5);', blink_count=2)
except ValueError:
    pass
else:
    raise AssertionError('stock math overlay accepted changed Blink call count')
assert translate_empty_label_statement(b'bd_done:\n\t}', b'bd_done') == b'bd_done:\n\t\t;\n\t}'
assert translate_empty_label_statement(
    b'vr_done: // exit label\n\t}', b'vr_done') == (
        b'vr_done: // exit label\n\t\t;\n\t}')
wenceslas_source = (root / 'upstream/pinned/Demo/Games/Wenceslas.ZC').read_bytes()
wenceslas_overlay = translate_wenceslas(wenceslas_source)
assert b'Sire, the night is darker now' in wenceslas_overlay
assert wenceslas_overlay.count(b'PopUpOk("') > wenceslas_source.count(b'PopUpOk("')
assert max(len(chunk) for chunk in re.findall(rb'PopUpOk\("(.*?)"\);', wenceslas_overlay, re.S)) < 144
assert b'if (sc.u8[0] == SC_CURSOR_RIGHT)' in wenceslas_overlay
assert b'switch (sc.u8[0])' not in wenceslas_overlay
assert all(code in wenceslas_overlay for code in (
    b'SC_CURSOR_LEFT', b'SC_CURSOR_DOWN', b'SC_CURSOR_UP'))
assert b'StepNew(Fs, king_x, king_y, king_' in wenceslas_overlay
assert b'#define ARM64_STEP_CAP 256' in wenceslas_overlay
assert b'#define ARM64_KING_STEP_CAP 128' in wenceslas_overlay
assert b'MAlloc(sizeof(Step), task)' not in wenceslas_overlay
assert b'QueueInsert(tmps, king_step_head.last)' not in wenceslas_overlay
assert b'QueueInsert(tmps, step_head.last)' not in wenceslas_overlay
progress_source = (root / 'upstream/pinned/Demo/Progress.ZC').read_bytes()
progress_overlay = translate_progress(progress_source)
assert b'asm {' not in progress_overlay and b'Yield;' in progress_overlay
assert b'AsmProgressDemo;' in progress_overlay
try:
    translate_math(math_source.replace(b'return ATan(s / c);', b'return ATan(s / c) + 0;', 1))
except ValueError:
    pass
else:
    raise AssertionError('Math overlay accepted changed ASin/ACos call sites')
try:
    translate_math(math_source + b'\npublic F64 Saw(F64 x) { return x; }\n')
except ValueError:
    pass
else:
    raise AssertionError('Math overlay accepted an upstream Saw definition')
keepaway_source = (root / 'upstream/pinned/Apps/KeepAway/KeepAway.ZC').read_bytes()
keepaway_overlay = translate_keepaway_nearest_ball(keepaway_source)
assert keepaway_overlay != keepaway_source
assert (b'if (o != last_owner && (!nearest_ball[o->team] || '
        b'o->get_ball_dt < nearest_ball[o->team]->get_ball_dt))') in keepaway_overlay
assert b'if (o != last_owner && !nearest_ball[o->team] ||' not in keepaway_overlay
try:
    translate_keepaway_nearest_ball(keepaway_source.replace(
        b'if (o != last_owner && !nearest_ball[o->team] ||',
        b'if (o != last_owner && nearest_ball[o->team] ||', 1))
except ValueError:
    pass
else:
    raise AssertionError('KeepAway overlay accepted changed nearest-player selection')
jukepuppet_source = (root/'upstream/pinned/Apps/Psalmody/JukePuppet.ZC').read_bytes()
jukepuppet_overlay = translate_jukepuppet_song_mask(jukepuppet_source)
assert b'FilesFind("*.ZC", FUF_RECURSE | FUF_JUST_FILES)' in jukepuppet_overlay
assert b'FUF_JUST_TXT' not in jukepuppet_overlay
assert (b'MessagePost(juke_task->popup_task, MESSAGE_KEY_DOWN_UP, CH_ESC, 0);\n'
        b'\tTaskWait(juke_task->popup_task);\n\tSleep(500);') in jukepuppet_overlay
try:
    translate_jukepuppet_song_mask(jukepuppet_source.replace(
        b'FUF_JUST_TXT', b'FUF_JUST_DD', 1))
except ValueError:
    pass
else:
    raise AssertionError('JukePuppet overlay accepted a changed text-only song filter')
try:
    translate_jukepuppet_song_mask(jukepuppet_source.replace(
        b'MessagePost(juke_task->popup_task, MESSAGE_KEY_DOWN_UP, CH_SPACE, 0);',
        b'MessagePost(juke_task->popup_task, MESSAGE_KEY_DOWN_UP, CH_ENTER, 0);', 1))
except ValueError:
    pass
else:
    raise AssertionError('JukePuppet overlay accepted a changed song-start gesture')
print('PASS guarded JukePuppet song discovery and Space/Escape run gesture', flush=True)
xcaliber_source = (root / 'upstream/pinned/Apps/X-Caliber/X-Caliber.ZC').read_bytes()
xcaliber_overlay = translate_xcaliber_shot_return(xcaliber_source)
assert xcaliber_overlay != xcaliber_source
shot_new = xcaliber_overlay.split(b'Shot *ShotNew', 1)[1].split(b'\n}\n', 1)[0]
assert b'QueueInsert(tmps, shot_head.last);\n\treturn tmps;' in shot_new
try:
    translate_xcaliber_shot_return(xcaliber_source.replace(
        b'QueueInsert(tmps, shot_head.last);',
        b'QueueInsert(tmps, other_head.last);', 1))
except ValueError:
    pass
else:
    raise AssertionError('X-Caliber overlay accepted changed ShotNew queue insertion')
xcaliber_draw_overlay = translate_xcaliber_draw_visibility(xcaliber_source)
draw_it = xcaliber_draw_overlay.split(b'U0 DrawIt(', 1)[1].split(b'\n}', 1)[0]
assert b'if (!ode)\n\t\treturn;' in draw_it
assert b'if (ode != task->last_ode)' not in draw_it
try:
    translate_xcaliber_draw_visibility(xcaliber_source.replace(
        b'if (ode != task->last_ode)', b'if (ode == task->last_ode)', 1))
except ValueError:
    pass
else:
    raise AssertionError('X-Caliber draw overlay accepted changed ODE visibility guard')
xcaliber_input_overlay = translate_xcaliber_key_descriptors(xcaliber_draw_overlay)
assert (b'Fs->win_inhibit = WIG_TASK_DEFAULT - WIF_SELF_FOCUS - '
        b'WIF_SELF_BORDER - WIF_FOCUS_TASK_MENU - WIF_SELF_CTRLS;') in xcaliber_input_overlay
assert (b'\t\tInit;\n\t\tFs->win_inhibit = 0x0050E2FA;\n'
        b'\t\tQueueInsert(ode, Fs->last_ode);') in xcaliber_input_overlay
try:
    translate_xcaliber_key_descriptors(
        xcaliber_draw_overlay.replace(b'WIF_SELF_CTRLS;', b'WIF_SELF_CTRLS - WIF_SELF_KEY_DESC;', 1))
except ValueError:
    pass
else:
    raise AssertionError('X-Caliber overlay accepted changed window input policy')
xcaliber_input_overlay = translate_xcaliber_fire_trace(xcaliber_input_overlay)
assert xcaliber_input_overlay.count(b'ZcXCaliberFireTrace(') == 3
assert b'ZcXCaliberFireTrace(0, level, QueueCount(&shot_head), 0, 0);' in xcaliber_input_overlay
assert b'ToI64(shot_head.last->p.x), ToI64(shot_head.last->p.y)' in xcaliber_input_overlay
xcaliber_escape_overlay = translate_xcaliber_escape_scan(xcaliber_input_overlay)
assert xcaliber_escape_overlay.count(b'case SC_ESC:\n\t\t\t\t\t\tch = CH_ESC;') == 2
xcaliber_input_overlay = translate_xcaliber_held_input(xcaliber_escape_overlay)
assert b'\t\t\tYield; //Keep focused input and other tasks responsive.\n' not in xcaliber_input_overlay
assert (b'if (Bt(kbd.down_bitmap, SC_CURSOR_LEFT))\n'
        b'\t\t\t\t\thuman_t_left = THRUST_MAX;\n'
        b'\t\t\t\telse\n\t\t\t\t\thuman_t_left = 0;') in xcaliber_input_overlay
assert b'if (Bt(kbd.down_bitmap, SC_ESC))\n\t\t\t\tch = CH_ESC;' in xcaliber_input_overlay
try:
    translate_xcaliber_escape_scan(xcaliber_input_overlay.replace(
        b'case SC_CURSOR_RIGHT:', b'case SC_CURSOR_LEFT:', 1))
except ValueError:
    pass
else:
    raise AssertionError('X-Caliber Escape overlay accepted changed key switches')

mathode_source = (root / 'upstream/pinned/System/Math/MathODE.ZC').read_bytes()
mathode_overlay = translate_mathode(mathode_source)
assert (b'monotonic_timeout_ns = ZcMonotonicNs() + '
        b'(timeout_time - tS) * 1000000000.0;') in mathode_overlay
assert b'monotonic_timeout_ns = timeout_time * 1000000000.0;' not in mathode_overlay
assert b'if (ZcMonotonicNs() > monotonic_timeout_ns)' in mathode_overlay
assert (b'ode_steps = 0;\n'
        b'\t\t\t\t\t\twhile (ode->t < t_desired)' in mathode_overlay)
assert b'if (++ode_steps >= 16 && ode->t < t_desired)' in mathode_overlay
try:
    translate_mathode(mathode_source.replace(
        b'timeout_time = start_time +', b'timeout_time = other_epoch +', 1))
except ValueError:
    pass
else:
    raise AssertionError('MathODE overlay accepted changed guest timeout calculation')
print('PASS guarded MathODE monotonic deadline and bounded RK-step overlay', flush=True)

tothefront_players = (root / 'upstream/pinned/Apps/ToTheFront/ToTheFront.ZC').read_bytes()
tothefront_player_overlay = translate_tothefront_player_pick(tothefront_players)
assert tothefront_player_overlay.count(b'ToTheFrontPlayerPick("AIs",') == 2
assert b'\tPlayerPick("AIs",' not in tothefront_player_overlay
assert b'FilesFind(mask, FUF_JUST_FILES)' in tothefront_player_overlay
assert b'LE=%d' in tothefront_player_overlay
assert b'I64 ttf_ai_load_slot;' in tothefront_player_overlay
assert b'ttf_ai_load_slot = player;' in tothefront_player_overlay
assert b'ExeFile(entry->full_name);' in tothefront_player_overlay
assert b'HashFind("PlayerIndirect"' not in tothefront_player_overlay
assert b'if (selected >= 0)' in tothefront_player_overlay
for ai_name in ('Human.ZC', 'SimpleAI.ZC'):
    ai_source = (root / 'upstream/pinned/Apps/ToTheFront/AIs' / ai_name).read_bytes()
    ai_overlay = translate_tothefront_ai(ai_source)
    if ai_name == 'Human.ZC':
        expected_ai_source = ai_source.replace(
            b'for (i = 0; i > UNITS_NUM; i++)',
            b'for (i = 0; i < UNITS_NUM; i++)', 1)
        expected_ai_source = expected_ai_source.replace(
            b'!Bt(&target->vis, 0)', b'!Bt(&target->vis[cur_player], 0)', 1)
        assert ai_overlay.startswith(expected_ai_source)
    else:
        assert ai_overlay.startswith(ai_source)
    for callback, slot in ((b'PlayerIndirect', b'player_indirect'),
                           (b'PlayerMove', b'player_move'),
                           (b'PlayerDirect', b'player_direct')):
        registration = (slot + b'[ttf_ai_load_slot] = &' + callback + b';')
        assert ai_overlay.count(registration) == 1
    if ai_name == 'Human.ZC':
        assert b'for (i = 0; i > UNITS_NUM; i++)' in ai_source
        assert b'for (i = 0; i < UNITS_NUM; i++)' in ai_overlay
        assert ai_overlay.count(b'for (i = 0; i < UNITS_NUM; i++)') == 1
        assert ai_source.count(b'!Bt(&target->vis, 0)') == 1
        assert ai_overlay.count(b'!Bt(&target->vis[cur_player], 0)') == 1
    else:
        assert b'for (i = 0; i > UNITS_NUM; i++)' not in ai_overlay
try:
    translate_tothefront_player_pick(tothefront_players.replace(
        b'PlayerPick("AIs", 1);', b'PlayerPick("Bots", 1);', 1))
except ValueError:
    pass
else:
    raise AssertionError('ToTheFront picker overlay accepted a changed call site')
span_source = (root / 'upstream/pinned/Apps/Span/SpanMain.ZC').read_bytes()
span_overlay = translate_span_worker_cleanup(span_source)
assert span_overlay != span_source
assert b'span_worker_cleanup_diag |= Kill(span_animate_task, TRUE) << 5;' in span_overlay
assert span_overlay.count(b'CTask\t\t*span_song_task = NULL, *span_animate_task = NULL;') == 1
assert span_overlay.count(b'Kill(span_animate_task, TRUE) << 5;') == 1
assert span_overlay.count(b'Kill(span_song_task, TRUE) << 6;') == 1
assert b'Fs->animate_task = NULL;' in span_overlay
assert b'Fs->song_task = NULL;' in span_overlay
try:
    translate_span_worker_cleanup(span_source.replace(
        b'Fs->animate_task = Spawn(&AnimateTask, &a, "Animate",, Fs);',
        b'Fs->other_task = Spawn(&AnimateTask, &a, "Animate",, Fs);', 1))
except ValueError:
    pass
else:
    raise AssertionError('Span cleanup overlay accepted changed worker spawn')
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
sprite_bridge_with_wrappers = inject_sprite_wrappers(
    sprite_bridge_with_matrix, sprite_plot)
assert b'public U0 Sprite3B(' in sprite_bridge_with_wrappers
assert b'public U0 SpriteExtents(' in sprite_bridge_with_wrappers
assert b'public CDC *Sprite2DC(U8 *elems)' in sprite_bridge_with_wrappers
assert b'public U8 *SpriteTransform(U8 *elems, I64 *matrix)' in sprite_bridge_with_wrappers
assert b'public U0 DCExtentsInit(CDC *dc=NULL)' in sprite_bridge_with_wrappers
assert b'dc->flags |= DCF_RECORD_EXTENTS;' in sprite_bridge_with_wrappers
try:
    inject_sprite_wrappers(sprite_bridge_with_wrappers, sprite_plot)
except ValueError:
    pass
else:
    raise AssertionError('sprite wrapper injection accepted duplicate APIs')
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
menu_focus_start = task_bridge.index('public U0 BootstrapMenuFocusRestore(')
menu_focus_end = task_bridge.index('\nU0 BootstrapMouseGlobalsInit()', menu_focus_start)
menu_focus = task_bridge[menu_focus_start:menu_focus_end]
assert ('if (clear)\n    {\n        bootstrap_menu_focus_task = NULL;\n'
        '        return;\n    }\n'
        '    if (TaskValidate(bootstrap_menu_focus_task))\n'
        '    {\n'
        '        ZcFocusTrace("MenuRestore", sys_focus_task, bootstrap_menu_focus_task);\n'
        '        sys_focus_task = bootstrap_menu_focus_task;') in menu_focus
assert 'I64 best_score, best_score0, best_score1;' in task_bridge
for sound_compat in ('public U0 Play(U8 *st, U8 *words=NULL)', 'U0 SoundTaskEndCB()',
                     'Bool FileOcc(U8 *needle, U8 *filename, U8 *flags="")'):
    assert sound_compat in task_bridge
assert 'U0 SoundTaskEndCB()\n{\n    Sound;\n    Exit;\n}' in task_bridge
assert 'U0 Sweep(I64 frequency, I64 duration, I64 end_frequency)' not in task_bridge
sound_source = (root / 'upstream/pinned/System/Sound.ZC').read_bytes()
assert sound_source.count(b'#define SE_NOISE\t\t0\n') == 1
assert b'public CTask *Noise(I64 mS, F64 min_ona, F64 max_ona)' in sound_source
assert b'public CTask *Sweep(I64 mS, F64 ona1, F64 ona2)' in sound_source
print('PASS pinned asynchronous sound effect entry points and task cleanup')
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
assert 'if (active_gr && !active_gr->fp_wall_paper)' in task_bridge
assert 'BootstrapEnsureWinPaper(active_gr);' in task_bridge
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
    ('System/Math/Conversion.ZC', translate_conversion_math, b'asm\n{', b'legacy block\n{'),
    ('System/Math/F32.ZC', translate_f32_math, b'asm\n{', b'legacy block\n{'),
    ('System/Math/Vec3.ZC', translate_vec3_math, b'asm\n{', b'legacy block\n{'),
    ('System/Math/Vec4.ZC', translate_vec4_math, b'asm\n{', b'legacy block\n{'),
    ('System/Math/Mat4.ZC', translate_mat4_math, b'asm\n{', b'legacy block\n{'),
    ('System/Math/NDArray.ZC', translate_ndarray_math,
     b'switch (type)\n\t{\n\t\tcase NDARRAY_I32:\n\t\t\tFree(arr->I32Buf);',
     b'switch (other_type)\n\t{\n\t\tcase NDARRAY_I32:\n\t\t\tFree(arr->I32Buf);'),
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
conversion = translate_conversion_math(
    (root / 'upstream/pinned/System/Math/Conversion.ZC').read_bytes()).decode('utf-8')
assert 'asm\n{' not in conversion
for name in ('F64ToF32', 'F32ToF64', 'F32RadToDeg', 'F32DegToRad'):
    assert f'extern ' in conversion and name in conversion
f32_math = translate_f32_math(
    (root / 'upstream/pinned/System/Math/F32.ZC').read_bytes()).decode('utf-8')
assert 'asm\n{' not in f32_math
assert 'extern F32 CosF32(F32 theta);' in f32_math
vec3_math = translate_vec3_math(
    (root / 'upstream/pinned/System/Math/Vec3.ZC').read_bytes()).decode()
assert 'asm\n{' not in vec3_math
assert 'U0 Vector3DPrint(CVector3D *v)' in vec3_math
for name in ('Vector3DInit', 'Vector3DCopy', 'Vector3DIsEqual', 'Vector3DAdd',
             'Vector3DAddS', 'Vector3DSub', 'Vector3DSubS', 'Vector3DMul',
             'Vector3DMulS', 'Vector3DDiv', 'Vector3DDivS', 'Vector3DMin',
             'Vector3DMax', 'Vector3DNegate', 'Vector3DNormalize',
             'Vector3DDot', 'Vector3DCross'):
    assert name in vec3_math
vec4_math = translate_vec4_math(
    (root / 'upstream/pinned/System/Math/Vec4.ZC').read_bytes()).decode('utf-8')
assert 'asm\n{' not in vec4_math
assert 'U0 Vector4DPrint(CVector4D *v)' in vec4_math
for name in ('Vector4DInit', 'Vector4DCopy', 'Vector4DIsEqual', 'Vector4DAdd',
             'Vector4DAddS', 'Vector4DSub', 'Vector4DSubS', 'Vector4DMul',
             'Vector4DMulS', 'Vector4DDiv', 'Vector4DDivS', 'Vector4DMin',
             'Vector4DMax', 'Vector4DNegate', 'Vector4DDot'):
    assert f'extern ' in vec4_math and name in vec4_math
mat4_math = translate_mat4_math(
    (root / 'upstream/pinned/System/Math/Mat4.ZC').read_bytes()).decode('utf-8')
assert 'asm\n{' not in mat4_math
assert 'U0 Matrix4DPrint(CMatrix4D *m)' in mat4_math
for name in ('Matrix4DMulVector4D', 'Matrix4DMul', 'Matrix4DTranspose'):
    assert 'extern ' in mat4_math and name in mat4_math
assert 'extern U0 Matrix4DTranslationSet(' in mat4_math
assert 'extern U0 Matrix4DScaleSet(' in mat4_math
ndarray_math = translate_ndarray_math(
    (root / 'upstream/pinned/System/Math/NDArray.ZC').read_bytes()).decode('utf-8')
assert ndarray_math.startswith('#include "Types"\n')
array_free = ndarray_math[ndarray_math.index('U0 ArrayFree('):]
assert 'switch (arr->type)' in array_free
assert 'arr->I64Buf = MAllocAligned(sizeof(I64) * arr->size, 16);' in ndarray_math
assert 'arr->F32Buf = MAllocAligned(sizeof(F32) * arr->size, 16);' in ndarray_math
assert 'arr->F64Buf = CAllocAligned(sizeof(F64) * arr->size, 16);' in ndarray_math
assert 'arr->F64Buf = MAllocAligned(sizeof(F64) * arr->size, 16);' in ndarray_math
math_test_dependencies = {
    'TestVec.ZC': ('Types', 'Conversion', 'F32', 'Vec4', 'Vec3'),
    'TestMat4.ZC': ('Types', 'Conversion', 'F32', 'Vec4', 'Vec3', 'Mat4'),
    'TestF32.ZC': ('Types', 'Conversion', 'F32', 'Math'),
}
for name, dependencies in math_test_dependencies.items():
    pinned_math_test = (root / 'upstream/pinned/System/Math/Tests' / name).read_bytes()
    translated_math_test = translate_math_test_dependencies(pinned_math_test, name)
    assert translated_math_test.startswith(
        ''.join(f'#include "/System/Math/{dep}"\n' for dep in dependencies).encode())
    assert translated_math_test.endswith(pinned_math_test)
    try:
        translate_math_test_dependencies(pinned_math_test.replace(b'Cd(__DIR__);;', b'Cd("/");;', 1), name)
    except ValueError:
        pass
    else:
        raise AssertionError('Math test overlay accepted changed source boundary: ' + name)
    for dependency in dependencies:
        try:
            translate_math_test_dependencies(
                pinned_math_test.replace(b'Cd(__DIR__);;',
                    f'#include "/System/Math/{dependency}"\nCd(__DIR__);;'.encode(), 1), name)
        except ValueError:
            pass
        else:
            raise AssertionError('Math test overlay accepted duplicate include: ' + name)
version_scalars = extract_version_scalars(
    (root / 'upstream/pinned/Kernel/KGlobals.ZC').read_bytes()).decode('utf-8')
version_match = re.search(r'sys_os_version = ([0-9.]+);\nU64\s+'
                          r'sys_os_version_sub = ([0-9]+);', version_scalars)
assert version_match
for name in ('sys_os_version_str', 'sys_os_version_full', 'sys_os_version_nice'):
    assert (f'U8 *{name} = "ZealOS ARM64 V{version_match[1]}.'
            f'{version_match[2]}' in version_scalars)
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
assert b'ZcTaskLexDefineText(doc_e->define_str, win_task)' in docrecalc_overlay
assert b'StrNew(compiler_define, mem_task)' in docrecalc_overlay
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
win_source = (root / 'upstream/pinned/System/Win.ZC').read_bytes()
win_focus_trace = translate_win_focus_trace(win_source).decode('utf-8')
assert win_focus_trace.count('ZcFocusTrace("WinRefocus", sys_focus_task, task);') == 1
assert win_focus_trace.count('ZcFocusTrace("WinToTop", sys_focus_task, task);') == 1
assert win_focus_trace.count('ZcFocusTrace("WinFocus", sys_focus_task, task);') == 1
job_source = (root / 'upstream/pinned/Kernel/Job.ZC').read_bytes()
job_focus_trace = translate_job_focus_trace(job_source).decode('utf-8')
assert job_focus_trace.count('ZcFocusTrace("JobRunOne", sys_focus_task, master);') == 1
sched_source = (root / 'upstream/pinned/Kernel/Sched.ZC').read_bytes()
sched_focus_trace = translate_sched_focus_trace(sched_source).decode('utf-8')
assert sched_focus_trace.count('ZcFocusTrace("TaskFocusNextClear", sys_focus_task, NULL);') == 1
assert sched_focus_trace.count('ZcFocusTrace("TaskFocusNextSelect", sys_focus_task, task);') == 1
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
stack_grow_bridge = extract_ktask_stack_grow(ktask_overlay.encode('utf-8')).decode('utf-8')
assert 'extern I64 ZcCallNOnStack(' in stack_grow_bridge
assert 'CTaskStack *TaskStackNew(' in stack_grow_bridge
assert 'argpop I64 CallStackGrow(' in stack_grow_bridge
assert 'CallStackGrow: entered' not in stack_grow_bridge
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
assert b'GrRect(dc, x - 5, y - 3, 10, 6);' in tothefront_draw_overlay
assert b'GrLine(dc, x, y, x + 9 * Cos(f), y + 9 * Sin(f));' in tothefront_draw_overlay
assert b'Sprite3ZB(dc, x, y, 0, tmpu->img, f)' not in tothefront_draw_overlay
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

docpopup_source = (root / 'upstream/pinned/System/DolDoc/DocPopUp.ZC').read_bytes()
docpopup_overlay = translate_docpopup(docpopup_source)
assert b'$WW,0$' not in docpopup_overlay
assert b'BootstrapGuiTerminalExeDocFile(\\"%Q\\");' in docpopup_overlay
assert b'res = Sys("#include \\"%s\\";", name2);' in docpopup_overlay
try:
    translate_docpopup(docpopup_source.replace(b'$WW,0$', b'$WW,1$'))
except ValueError:
    pass
else:
    raise AssertionError('DocPopUp overlay accepted changed source metadata')
print('PASS guarded DocPopUp overlay retains text and packed document include paths')

docputs_source = (root / 'upstream/pinned/System/DolDoc/DocPutS.ZC').read_bytes()
docputs_overlay = translate_docputs_exception_cleanup(docputs_source)
partial = docputs_overlay.split(b'public U0 DocPrintPartial(', 1)[1].split(b'\nBool KDDocPutS(', 1)[0]
assert b'catch\n\t{' in partial and b'throw(except_ch);' in partial
assert b'if (unlock)\n\t\t\tDocUnlock(doc);\n\t\tFree(buf);' in partial
assert partial.index(b'try\n\t{') < partial.index(b'(*doc->user_put_s)')
assert b'Free(buf);\n\t\tbuf = NULL;' in partial
assert b'if (!(doc->flags & DOCF_DONT_SWAP_OUT))\n\t\tYield;' in partial
try:
    translate_docputs_exception_cleanup(docputs_source.replace(b'unlock = DocLock(doc);', b'unlock = OtherLock(doc);'))
except ValueError:
    pass
else:
    raise AssertionError('DocPrintPartial overlay accepted changed locking')
print('PASS guarded DocPrintPartial cleanup preserves its conditional yield')
