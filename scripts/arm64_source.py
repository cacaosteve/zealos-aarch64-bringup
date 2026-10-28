"""Narrow source overlays needed to compile pinned ZealOS on AArch64."""
from __future__ import annotations

import re
import struct


_TEXT_BG_X86 = re.compile(
    r"(?m)^(?P<indent>[ \t]*)MOV[ \t]+U64[ \t]+\[RSI\],[ \t]*R13[ \t]*\n"
    r"(?:[ \t]*ADD[ \t]+RSI,[ \t]*R12[ \t]*\n"
    r"[ \t]*MOV[ \t]+U64[ \t]+\[RSI\],[ \t]*R13[ \t]*\n){6}"
    r"[ \t]*ADD[ \t]+RSI,[ \t]*R12[ \t]*\n"
    r"[ \t]*MOV[ \t]+U64[ \t]+\[RSI\],[ \t]*R13[ \t]*(?:\n|$)"
)

_KTASK_GROW_BEGIN = "#exe {Option(OPTf_NO_REG_VAR, ON);};"
_KTASK_GROW_END = "#exe {Option(OPTf_NO_REG_VAR, OFF);};"


def extract_sprite_elem_base_sizes(source: bytes) -> bytes:
    """Extract the exact pinned ZealOS packed sprite size declaration."""
    text = source.decode('utf-8')
    start = text.find('I64 sprite_elem_base_sizes[SPT_TYPES_NUM] = {')
    if start < 0:
        raise ValueError('pinned SpriteNew.ZC has no sprite base-size table')
    brace = text.find('{', start)
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == '{':
            depth += 1
        elif text[index] == '}':
            depth -= 1
            if depth == 0:
                end = text.find(';', index)
                if end < 0:
                    break
                declaration = text[start:end + 1]
                if declaration.count('//SPT_') != 30:
                    raise ValueError('pinned sprite base-size table entry count changed')
                return declaration.encode('utf-8')
    raise ValueError('pinned sprite base-size table is unterminated')


def inject_sprite_elem_base_sizes(bridge: bytes, source: bytes) -> bytes:
    """Prepend pinned sprite record sizes to the ARM64 sprite bridge."""
    if b'sprite_elem_base_sizes' in bridge:
        raise ValueError('ARM64 sprite bridge already declares the pinned size table')
    declaration = extract_sprite_elem_base_sizes(source)
    return declaration + b'\n\n' + bridge


def _extract_between(source: bytes, start: bytes, end: bytes, label: str) -> bytes:
    if source.count(start) != 1:
        raise ValueError(f'pinned {label} function boundaries changed')
    first = source.index(start)
    last = source.find(end, first)
    if last < 0:
        raise ValueError(f'pinned {label} function end changed')
    return source[first:last].rstrip()


def extract_sprite_interpolation(sprite_new: bytes, sprite_plot: bytes) -> bytes:
    """Generate bounded sprite sizing plus pinned upstream interpolation.

    SpriteSize lives in SpriteNew.ZC; interpolation is the upstream
    GrSpritePlot.ZC implementation. The generated walker uses the existing
    SpriteElemSize bridge, caps records and total bytes, and the interpolation
    wrapper validates matching stream shape and the documented t interval
    before entering the upstream field interpolation switch.
    """
    sprite_size = _extract_between(
        sprite_new, b'public I64 SpriteSize(U8 *elems)\n',
        b'\nI64 SpriteTypeMask(', 'SpriteSize')
    if (b'tmpg(U8 *) += SpriteElemSize(tmpg);' not in sprite_size or
            b'sprite_elem_base_sizes[SPT_END]' not in sprite_size):
        raise ValueError('pinned SpriteSize traversal changed')
    interpolation = _extract_between(
        sprite_plot, b'public U8 *SpriteInterpolate(F64 t, U8 *elems0, U8 *elems1)\n',
        b'\n#help_index', 'SpriteInterpolate')
    required = (
        b'if (t < 0.5)',
        b'case SPT_BITMAP:',
        b'case SPT_MESH:',
        b'case SPT_SHIFTABLE_MESH:',
        b'return res;',
    )
    if any(token not in interpolation for token in required):
        raise ValueError('pinned SpriteInterpolate implementation changed')
    if interpolation.count(b'GR_SCALE') != 3:
        raise ValueError('pinned SpriteInterpolate scale references changed')

    bounded_size = b'''// Bounded ARM64 walker derived from the pinned SpriteNew.ZC SpriteSize.
public I64 SpriteSize(U8 *elems)
{
    U8 *record_bytes;
    CSprite *record;
    I64 records = 0, total = 0, size, type, available;

    if (!elems)
        throw('Graphics');
    record_bytes = elems;
    record = record_bytes - offset(CSprite.start);
    available = ZcManagedAllocationSize(elems);
    while (records < 65536 && total < 16777216)
    {
        records++;
        type = record->type & SPG_TYPE_MASK;
        if (!type)
        {
            if (available && total + sprite_elem_base_sizes[SPT_END] > available)
                throw('Graphics');
            return total + sprite_elem_base_sizes[SPT_END];
        }
        if (type >= SPT_TYPES_NUM)
            throw('Graphics');
        size = SpriteElemSize(record_bytes, type);
        if (size < 1 || size > 16777216 - total ||
                (available && total + size + sprite_elem_base_sizes[SPT_END] > available))
            throw('Graphics');
        total += size;
        record_bytes += size;
        record = record_bytes - offset(CSprite.start);
    }
    throw('Graphics');
}

Bool SpriteInterpolationStreamsCompatible(U8 *elems0, U8 *elems1,
        I64 size0, I64 size1)
{
    U8 *p0 = elems0, *p1 = elems1;
    I64 byte_offset = 0, records = 0, type0, type1, elem_size0, elem_size1;
    I32 count0, count1;

    if (size0 != size1 || size0 < 1 || size0 > 16777216)
        return FALSE;
    while (records < 65536 && byte_offset < size0)
    {
        records++;
        type0 = p0[0] & SPG_TYPE_MASK;
        type1 = p1[0] & SPG_TYPE_MASK;
        if (type0 != type1)
            return FALSE;
        if (!type0)
            return byte_offset == size0 - 1;
        if (type0 >= SPT_TYPES_NUM)
            return FALSE;
        elem_size0 = SpriteElemSize(p0, type0);
        elem_size1 = SpriteElemSize(p1, type1);
        if (elem_size0 < 1 || elem_size0 != elem_size1 ||
                elem_size0 > 16777216 || byte_offset + elem_size0 >= size0)
            return FALSE;
        if (type0 == SPT_POLYLINE || type0 == SPT_POLYPT ||
                type0 == SPT_BSPLINE2 || type0 == SPT_BSPLINE3 ||
                type0 == SPT_BSPLINE2_CLOSED || type0 == SPT_BSPLINE3_CLOSED)
        {
            MemCopy(&count0, p0 + 4, sizeof(I32));
            MemCopy(&count1, p1 + 4, sizeof(I32));
            if (count0 != count1)
                return FALSE;
        }
        else if (type0 == SPT_BITMAP)
        {
            if (MemCompare(p0 + 9, p1 + 9, 8))
                return FALSE;
        }
        else if (type0 == SPT_MESH)
        {
            if (MemCompare(p0 + 1, p1 + 1, 8))
                return FALSE;
        }
        else if (type0 == SPT_SHIFTABLE_MESH)
        {
            if (MemCompare(p0 + 13, p1 + 13, 8))
                return FALSE;
        }
        byte_offset += elem_size0;
        p0 += elem_size0;
        p1 += elem_size1;
    }
    return FALSE;
}

'''
    # Retain the original interpolation switch but validate its inputs and
    # replace the upstream size call with the ARM64 bridge signature.
    old_declaration = b'I64\t\t i, t1 = GR_SCALE * t, t0 = GR_SCALE - t1;'
    if interpolation.count(old_declaration) != 1:
        raise ValueError('pinned SpriteInterpolate local declaration changed')
    interpolation = interpolation.replace(
        old_declaration,
        b'I64\t\t i, t1, t0, size0, size1;', 1)
    for source_size in (b'elems0', b'elems1'):
        old_allocation = b'i = SpriteSize(' + source_size + b'), res = MAlloc(i);'
        if interpolation.count(old_allocation) != 1:
            raise ValueError('pinned SpriteInterpolate allocation setup changed')
        interpolation = interpolation.replace(
            old_allocation,
            b'i = SpriteSize(' + source_size + b');\n\t\tres = MAlloc(i);', 1)
    guard = b'''\n\tif (!elems0 || !elems1 || t != t || t < 0.0 || t > 1.0)
		throw('Graphics');
	size0 = SpriteSize(elems0);
	size1 = SpriteSize(elems1);
	if (!SpriteInterpolationStreamsCompatible(elems0, elems1, size0, size1))
		throw('Graphics');
	t1 = GR_SCALE * t;
	t0 = GR_SCALE - t1;
'''
    if interpolation.count(b'\n\tif (t < 0.5)') != 1:
        raise ValueError('pinned SpriteInterpolate selection point changed')
    interpolation = interpolation.replace(b'\n\tif (t < 0.5)', guard + b'\n\tif (t < 0.5)', 1)
    old_type_check = b'if (tmpg0->type & SPG_TYPE_MASK != tmpg1->type & SPG_TYPE_MASK)'
    if interpolation.count(old_type_check) != 1:
        raise ValueError('pinned SpriteInterpolate type check changed')
    interpolation = interpolation.replace(
        old_type_check,
        b'if ((tmpg0->type & SPG_TYPE_MASK) != (tmpg1->type & SPG_TYPE_MASK))', 1)
    old_step0 = b'tmpg0(U8 *) += SpriteElemSize(tmpg0);'
    old_step1 = b'tmpg1(U8 *) += SpriteElemSize(tmpg1);'
    old_step_result = b'tmpgr(U8 *) += SpriteElemSize(tmpgr);'
    for old, new in (
        (old_step0, b'bytes0 += SpriteElemSize(bytes0, tmpg0->type & SPG_TYPE_MASK); tmpg0 = bytes0 - offset(CSprite.start);'),
        (old_step1, b'bytes1 += SpriteElemSize(bytes1, tmpg1->type & SPG_TYPE_MASK); tmpg1 = bytes1 - offset(CSprite.start);'),
        (old_step_result, b'bytesr += SpriteElemSize(bytesr, tmpgr->type & SPG_TYPE_MASK); tmpgr = bytesr - offset(CSprite.start);'),
    ):
        if interpolation.count(old) != 1:
            raise ValueError('pinned SpriteInterpolate record walk changed')
        interpolation = interpolation.replace(old, new, 1)
    interpolation = interpolation.replace(
        b'CSprite\t*tmpg0 = elems0 - offset(CSprite.start), *tmpg1 = elems1 - offset(CSprite.start), *tmpgr;',
        b'CSprite\t*tmpg0, *tmpg1, *tmpgr;\n\tU8\t*bytes0 = elems0, *bytes1 = elems1, *bytesr;', 1)
    if interpolation.count(b'U8\t*bytes0 = elems0, *bytes1 = elems1, *bytesr;') != 1:
        raise ValueError('pinned SpriteInterpolate sprite-pointer declaration changed')
    interpolation = interpolation.replace(
        b'\ttmpgr = res - offset(CSprite.start);',
        b'\tbytesr = res;\n\ttmpg0 = bytes0 - offset(CSprite.start);\n'
        b'\ttmpg1 = bytes1 - offset(CSprite.start);\n'
        b'\ttmpgr = bytesr - offset(CSprite.start);', 1)
    if interpolation.count(b'\tbytesr = res;') != 1:
        raise ValueError('pinned SpriteInterpolate result pointer setup changed')
    if interpolation.count(b'GR_SCALE') != 3:
        raise ValueError('pinned SpriteInterpolate scale references changed')
    interpolation = interpolation.replace(b'GR_SCALE', b'SPRITE_INTERPOLATION_SCALE')
    return (bounded_size + b'#define SPRITE_INTERPOLATION_SCALE (1<<32)\n' +
            interpolation + b'\n')


def inject_sprite_interpolation(bridge: bytes, sprite_new: bytes,
                                sprite_plot: bytes) -> bytes:
    """Append size/interpolation APIs before helpers that depend on SpriteSize."""
    if b'public U8 *SpriteInterpolate(' in bridge or b'public I64 SpriteSize(' in bridge:
        raise ValueError('ARM64 sprite bridge already declares sprite interpolation helpers')
    helper_start = bridge.find(b'public CDocEntry *DocSprite(')
    doc_helpers = b''
    if helper_start >= 0:
        doc_helpers = bridge[helper_start:].rstrip()
        if (doc_helpers.count(b'public CDocEntry *DocSprite(') != 1 or
                doc_helpers.count(b'public CDocEntry *Sprite(') != 1):
            raise ValueError('ARM64 document sprite helpers changed')
        bridge = bridge[:helper_start].rstrip()
    generated = extract_sprite_interpolation(sprite_new, sprite_plot)
    if doc_helpers:
        return bridge + b'\n' + generated + b'\n' + doc_helpers + b'\n'
    return bridge + b'\n' + generated


def extract_sprite_matrix_draw() -> bytes:
    """Generate bounded matrix-based mesh drawing and axis-rotation adapters."""
    return b'''\n#define SPRITE3_MATRIX_LIMIT (64 * SPRITE_INTERPOLATION_SCALE)

Bool Sprite3MatrixBounded(I64 *matrix)
{
    I64 i;
    if (!matrix)
        return FALSE;
    for (i = 0; i < 16; i++)
        if (matrix[i] < -SPRITE3_MATRIX_LIMIT ||
                matrix[i] > SPRITE3_MATRIX_LIMIT)
            return FALSE;
    return TRUE;
}

public U0 Sprite3Mat4x4B(CDC *dc=gr.dc, I64 x, I64 y, I64 z,
        U8 *elems, I64 *matrix)
{//Bounded upstream matrix-composition wrapper for the ARM64 sprite rasterizer.
    I64 composed[16], local[16], *old_matrix, old_flags, old_x, old_y, old_z;
    I64 i, sprite_size;

    if (!dc || dc->dc_signature != DCS_SIGNATURE_VAL || !dc->r ||
            !elems || !matrix || x < -1073741824 || x > 1073741824 ||
            y < -1073741824 || y > 1073741824 ||
            z < -1073741824 || z > 1073741824 ||
            !Sprite3MatrixBounded(matrix) || !Sprite3MatrixBounded(dc->r))
        throw('Graphics');
    sprite_size = SpriteSize(elems);
    if (sprite_size < 1)
        throw('Graphics');

    old_matrix = dc->r;
    old_flags = dc->flags;
    old_x = dc->x;
    old_y = dc->y;
    old_z = dc->z;
    MemCopy(local, matrix, sizeof(local));
    local[3] += x << 32;
    local[7] += y << 32;
    local[11] += z << 32;
    local[15] = SPRITE_INTERPOLATION_SCALE;
    BootstrapMat4x4Mul(composed, old_matrix, local);
    dc->r = composed;
    dc->flags |= DCF_TRANSFORMATION;
    try
        Sprite3(dc, 0, 0, 0, elems);
    catch
    {
        dc->r = old_matrix;
        dc->flags = old_flags;
        dc->x = old_x;
        dc->y = old_y;
        dc->z = old_z;
        throw;
    }
    dc->r = old_matrix;
    dc->flags = old_flags;
    dc->x = old_x;
    dc->y = old_y;
    dc->z = old_z;
}

public U0 Sprite3XB(CDC *dc=gr.dc, I64 x, I64 y, I64 z,
        U8 *elems, F64 angle=0)
{
    I64 matrix[16];
    Mat4x4IdentEqu(matrix);
    Mat4x4RotX(matrix, angle);
    Sprite3Mat4x4B(dc, x, y, z, elems, matrix);
}

public U0 Sprite3YB(CDC *dc=gr.dc, I64 x, I64 y, I64 z,
        U8 *elems, F64 angle=0)
{
    I64 matrix[16];
    Mat4x4IdentEqu(matrix);
    Mat4x4RotY(matrix, angle);
    Sprite3Mat4x4B(dc, x, y, z, elems, matrix);
}

public U0 Sprite3ZB(CDC *dc=gr.dc, I64 x, I64 y, I64 z,
        U8 *elems, F64 angle=0)
{
    I64 matrix[16];
    Mat4x4IdentEqu(matrix);
    Mat4x4RotZ(matrix, angle);
    Sprite3Mat4x4B(dc, x, y, z, elems, matrix);
}
'''


def inject_sprite_matrix_draw(bridge: bytes) -> bytes:
    """Append the bounded Sprite3 matrix API and upstream axis wrappers."""
    if b'public U0 Sprite3Mat4x4B(' in bridge:
        raise ValueError('ARM64 sprite bridge already declares matrix drawing')
    return bridge + extract_sprite_matrix_draw()


def extract_user_task(source: bytes) -> bytes:
    """Retain KTask's execution/input loops with a native compiler ABI.

    Aiwnios IR and lexer objects do not have the pinned compiler layout. Only
    the compiler entry points are redirected; execution, answers and the outer
    try/catch loop remain source code. Explicit abort cleanup also reclaims a
    partially compiled statement after a guest exception.
    """
    text = source.decode("utf-8")
    boundaries = ("I64 ExeCmdLine(CCompCtrl *cc)\n", "\nU0 ServerTaskCont()\n",
                  "U0 UserTaskCont()\n", "\nU0 ServerCmdLine(I64 dummy=0)\n")
    if any(text.count(s) != 1 for s in boundaries):
        raise ValueError("expected the pinned execution/user loop boundaries")
    exe = text[text.index(boundaries[0]):text.index(boundaries[1])]
    user = text[text.index(boundaries[2]):text.index(boundaries[3])]
    replacements = {
        "LexStatement2Bin(cc, &type)": "ZcUserStatement2Bin(cc, &type)",
        "Free(machine_code);": "ZcUserCodeFree(cc, machine_code);",
        "cc->lex_include_stack->flags & LFSF_DOC":
            "cc->lex_include_stack && cc->lex_include_stack->flags & LFSF_DOC",
        "(ptr = cc->lex_include_stack->line_start) && *ptr":
            "cc->lex_include_stack && (ptr = cc->lex_include_stack->line_start) && *ptr",
    }
    for old, new in replacements.items():
        if exe.count(old) != 1:
            raise ValueError(f"pinned ExeCmdLine changed: {old}")
        exe = exe.replace(old, new, 1)
    # The upstream function predates this port's recoverable compiler boundary.
    # Restore the title and free its saved copy even if Call or parsing throws.
    begin = "\twhile (cc->token &&"
    end = "\tif (Fs->title_src != TTS_LOCKED_CONST)\n\t{\n\t\tFs->title_src = old_title_src;"
    if exe.count(begin) != 1 or exe.count(end) != 1:
        raise ValueError("pinned ExeCmdLine title cleanup changed")
    exe = exe.replace(begin, "\ttry\n\t{\n" + begin, 1)
    exe = exe.replace(end, "\t}\n\tcatch\n\t{\n"
        "\t\tFs->title_src = old_title_src;\n"
        "\t\tStrCopy(Fs->task_title, old_task_title);\n"
        "\t\tFree(old_task_title);\n\t}\n" + end, 1)
    for old, new in {
        "CompCtrlNew(,": "ZcUserCompCtrlNew(,",
        "Lex(cc);": "ZcUserLex(cc);",
        "CompCtrlDel(cc);": "ZcUserCompCtrlDel(cc);",
        "\t\t\tPutExcept;": "\t\t\tZcUserCompCtrlDel(cc);\n\t\t\tZcUserPutExcept;",
    }.items():
        if user.count(old) != 1:
            raise ValueError(f"pinned UserTaskCont changed: {old}")
        user = user.replace(old, new, 1)
    header = """// Original Kernel/KTask.ZC loops, using the ARM64 lexer/JIT ABI.
extern CCompCtrl *ZcUserCompCtrlNew(U8 *buf=NULL, I64 flags=0);
extern U0 ZcUserCompCtrlDel(CCompCtrl *cc);
extern I64 ZcUserLex(CCompCtrl *cc);
extern U8 *ZcUserStatement2Bin(CCompCtrl *cc, I64 *type);
extern U0 ZcUserCodeFree(CCompCtrl *cc, U8 *code);
extern I64 ZcUserPromptSet(U8 *(*reader)(Bool continuation));
extern U0 ZcUserOutputSet(U0 (*writer)(U8 *text));
extern U0 ZcUserPutExcept();
extern Bool ZcUserInputComplete(U8 *text);
"""
    # Normal user entry is separate from the compiler loop: it sets the window
    # policy, calls the late-bound home hook and checks that it made a window.
    start, end = "U0 UserCmdLine(I64 dummy=0)\n", "\nU0 TaskDel(CTask *task)\n"
    if text.count(start) != 1 or text.count(end) != 1:
        raise ValueError("expected the pinned UserCmdLine/User boundaries")
    entry = text[text.index(start):text.index(end)]
    for old in ('\tno_warn dummy;', 'CallExtStr("UserStartUp");',
                'CTask\t*task = Spawn(&UserCmdLine,, "Terminal");'):
        if entry.count(old) != 1:
            raise ValueError(f"pinned user entry changed: {old}")
    entry = entry.replace('\tno_warn dummy;', '\tif (dummy) {}', 1)
    entry = entry.replace('CallExtStr("UserStartUp");',
                          'ZcCallExtStr("UserStartUp");', 1)
    # This port has bounded task slots. Fail before TaskWait(NULL) would wait
    # for the caller itself when all application slots are in use.
    entry = entry.replace('CTask\t*task = Spawn(&UserCmdLine,, "Terminal");',
        'CTask\t*task = Spawn(&UserCmdLine,, "Terminal");\n'
        '\n\tif (!task)\n\t\treturn NULL;', 1)
    return (header + '\nextern I64 ZcCallExtStr(U8 *name);\n' +
            exe + "\n" + user + "\n" + entry).encode("utf-8")


def extract_home_startup(source: bytes) -> bytes:
    """Extract the pinned HomeSys per-task hooks."""
    return _extract_home_startup_parts(source)[0]


def extract_home_startup_tasks(source: bytes) -> bytes:
    """Translate the pinned HomeSys desktop-startup body for ARM64."""
    return _extract_home_startup_parts(source)[1]


def inject_home_startup_tasks(gui_terminal: bytes, startup_tasks: bytes) -> bytes:
    """Place the derived startup body after its GUI globals and helpers."""
    anchor = b'I64 BootstrapGuiTerminalHomeStart()\n'
    if gui_terminal.count(anchor) != 1:
        raise ValueError('expected the GUI HomeSys startup insertion point')
    return gui_terminal.replace(anchor, startup_tasks + b'\n' + anchor, 1)


def translate_home_once(source: bytes) -> bytes:
    """Keep the live DVD session usable without offering the legacy installer.

    Keep the pinned DVD branch's live-session behavior while exposing the
    bounded ARM64 installer command to the user. Preserve the upstream
    ``Once.ZC`` source everywhere else.
    """
    text = source.decode('utf-8')
    start = '\t\t\t"\\nIf you answer \'No\' you can play with\\n"\n'
    end = '\t\t\tif (FileFind("::/Misc/Tour"))\n'
    if text.count(start) != 1 or text.count(end) != 1:
        raise ValueError('pinned Once DVD installer prompt changed')
    first = text.index(start)
    last = text.index(end, first)
    dvd_prompt = text[first:last]
    if (dvd_prompt.count('Install onto hard drive') != 1 or
            dvd_prompt.count('RunFile("::/Misc/OSInstall",, TRUE);') != 1):
        raise ValueError('pinned Once DVD installer flow changed')
    replacement = ('\t\t\t"\\nARM64 live session is ready.\\n"\n'
                   '\t\t\t"Install to a 128 MiB UTM/QEMU VirtIO disk: zinstall, then zinstall ERASE.\\n";\n')
    return (text[:first] + replacement + text[last:]).encode('utf-8')


def _extract_home_startup_parts(source: bytes) -> tuple[bytes, bytes]:
    """Keep HomeSys hooks and its pinned desktop-startup body.

    The ARM64 adapter reuses the first terminal it already brought up, then
    translates upstream-only output/tiling calls into terminal-scoped bridges.
    This keeps the startup sequence tied to the pinned HomeSys implementation.
    The DVD path reaches the live graphical session; its generated
    ``Home/Once.ZC`` points to the bounded UTM/QEMU ``zinstall`` command and
    never invokes the legacy BIOS/AHCI installer.
    """
    text = source.decode('utf-8')
    boundary = '\nU0 StartUpTasks()\n'
    task_end = '\nStartUpTasks;'
    if text.count(boundary) != 1 or text.count(task_end) != 1:
        raise ValueError('expected the pinned HomeSys startup boundary')
    task_start = text.index(boundary) + 1
    task_end_at = text.index(task_end, task_start)
    hooks = text[:task_start - 1]
    startup = text[task_start:task_end_at]
    for name in ('UserStartUp', 'ServerStartUp'):
        signature = 'U0 ' + name + '()'
        if hooks.count(signature) != 1:
            raise ValueError('pinned HomeSys hook changed: ' + name)
        hooks = hooks.replace(signature, 'U0 BootstrapHome' + name + '()', 1)
    if hooks.count('Type("::/Doc/Start.DD");') != 1 or hooks.count('\tDir;') != 1:
        raise ValueError('pinned HomeSys user document/directory setup changed')
    anchors = (
        'U0 StartUpTasks()\n',
        '\tCTask *user = User;\n',
        '\tif (sys_boot_src.u16[0] != BOOT_SRC_DVD)\n\t\tUser;\n',
        '\tWinToTop(user);\n',
        '\tWinTileVert;\n',
        '\t"Boot Time:%7.3fs\\n", tS;\n',
        '\tXTalk(user, "Cd; #include \\"Once\\";\\n");\n',
        '\tSilent; //no output to screen\n',
        '\tACInit("/*;!*/Bible.TXT;!*/Clementine.TXT;!*/PCIDevices.DD;!*/HTML/*;!*/ChangeLog.DD");\n',
        '\tSilent(OFF); //no output to screen\n',
        '\tif (sys_boot_src.u16[0] != BOOT_SRC_RAM)\n'
        '\t\tPopUp("ExeFile2(\\"::/System/ZSplash.ZC\\");");\n',
    )
    if any(startup.count(anchor) != 1 for anchor in anchors):
        raise ValueError('pinned HomeSys StartUpTasks body changed')
    replacements = (
        ('U0 StartUpTasks()', 'I64 BootstrapHomeStartUpTasks()'),
        ('\tCTask *user = User;\n',
         '\tCTask *user = bootstrap_gui_terminal_task;\n'
         '\tif (!TaskValidate(user)) return 1;\n'),
        ('\tif (sys_boot_src.u16[0] != BOOT_SRC_DVD)\n\t\tUser;\n',
         '\tif (sys_boot_src.u16[0] != BOOT_SRC_DVD)\n'
         '\t{\n'
         '\t\tbootstrap_gui_terminal_home_second = User();\n'
         '\t\tif (!TaskValidate(bootstrap_gui_terminal_home_second)) return 2;\n'
         '\t}\n'),
        ('\tWinTileVert;\n',
         '\tBootstrapGuiTerminalTileHomeWindows(user, bootstrap_gui_terminal_home_second);\n'),
        ('\t"Boot Time:%7.3fs\\n", tS;\n',
         '\tXTalk(user, "\\"Boot Time:%%7.3fs\\n\\", tS;\\n");\n'),
        ('\tSilent; //no output to screen\n'
         '\tACInit("/*;!*/Bible.TXT;!*/Clementine.TXT;!*/PCIDevices.DD;!*/HTML/*;!*/ChangeLog.DD");\n'
         '\tSilent(OFF); //no output to screen\n',
         '\tXTalk(user, "Silent; Cd(\\"/\\"); ACInit(\\"/*;!*/Bible.TXT;!*/Clementine.TXT;!*/PCIDevices.DD;!*/HTML/*;!*/ChangeLog.DD\\"); Cd(\\"/Home\\"); Silent(OFF);\\n");\n'),
        ('\tif (sys_boot_src.u16[0] != BOOT_SRC_RAM)\n'
         '\t\tPopUp("ExeFile2(\\"::/System/ZSplash.ZC\\");");\n',
         '\tif (sys_boot_src.u16[0] != BOOT_SRC_RAM)\n'
         '\t\tXTalk(user, "PopUp(\\"BootstrapGuiTerminalExeDocFile(\\\\\\"::/System/ZSplash.ZC\\\\\\");\\");\\n");\n'),
        ('\n}', '\n\treturn 0x2a;\n}'),
    )
    for old, new in replacements:
        if startup.count(old) != 1:
            raise ValueError('HomeSys startup translation anchor changed: ' + old[:48])
        startup = startup.replace(old, new, 1)
    hook_file = ('// Pinned HomeSys per-task hooks.\n' + hooks + '\n').encode('utf-8')
    startup_body = ('// Guarded ARM64 translation of pinned HomeSys.StartUpTasks.\n' +
                    startup + '\n').encode('utf-8')
    return hook_file, startup_body


def translate_zsplash(source: bytes) -> bytes:
    """Run the pinned animated splash with ARM64-owned pointer and task state.

    Keep its embedded sprite and renderer intact. The ARM64 task already owns
    native USB pointer input, so drop the x86 mouse-driver handoff and restore
    keyboard focus locally instead of killing the parent task. Preserve its
    short startup jitter with the loaded timer counter because this bootstrap
    does not load the full date/time helper. Keep a task-owned splash depth
    buffer across animation frames, detach it from the transient CDC alias
    before that alias is deleted, and release it when the splash is dismissed.
    Restore the task's prior draw callback when the splash is dismissed.
    """
    if source.count(b'\0') < 1:
        raise ValueError('expected ZSplash inline sprite payload terminator')
    raw, payload = source.split(b'\0', 1)
    if not raw.endswith(b'ZSplash;') or not payload:
        raise ValueError('expected ZSplash launch statement and sprite payload')
    # The pinned source stores this CDocBin as UTF-8 text whose non-ASCII
    # codepoints are the original CP437 byte values. Restore those single-byte
    # sprite bytes before the bootstrap compiler embeds the binary. Passing
    # the UTF-8 bytes through literally changes negative mesh coordinates into
    # unrelated multi-byte values and leaves the splash labels without their
    # rotating mesh.
    try:
        payload = payload.decode('utf-8').encode('cp437')
    except (UnicodeDecodeError, UnicodeEncodeError) as exc:
        raise ValueError('pinned ZSplash binary is not UTF-8 encoded CP437 data') from exc
    if len(payload) < 26:
        raise ValueError('pinned ZSplash binary is truncated')
    bin_num, bin_flags, bin_size, bin_use_count = struct.unpack_from('<IIII', payload)
    if (bin_num != 1 or bin_flags != 0 or bin_size != len(payload) - 16 or
            bin_use_count < 1 or payload[16] != 24):
        raise ValueError('pinned ZSplash CDocBin or mesh header changed')
    vertex_count, tri_count = struct.unpack_from('<II', payload, 17)
    if (vertex_count < 1 or vertex_count > 1024 or tri_count > 2048 or
            9 + vertex_count * 12 + tri_count * 16 + 1 != bin_size):
        raise ValueError('pinned ZSplash packed mesh payload changed')
    text = raw.decode('utf-8')
    anchors = (
        '$SP,"<1>",BI=1$',
        'Sprite3(dc, 0, 0, 0, $IB,"<1>",BI=1$);',
        'Sleep((Now + counts.timer) & 0x7F);',
        'MStrPrint("%S", "DD_OS_NAME_VERSION");',
        'U0 ZSplash()\n',
        'while (!CharScan)',
        'DCDepthBufAlloc(dc);',
        'dc->r = old_r;',
        'Sprite3(dc, 0, 0, 0, $IB,"<1>",BI=1$);',
        'CTask\t*bg_task = User;',
        'Fs->draw_it = &DrawIt;',
        'WinBorder(, bg_task);',
        'WinMax(bg_task);',
        'gr.fp_draw_mouse = &DrawNoMouse;',
        'MouseHardEnable(FALSE);',
        'gr.fp_draw_mouse = &DrawStdMouse;',
        'MouseHardEnable(TRUE);',
        'Spawn(&MouseHardDriverInstall);',
        'Kill(bg_task);',
        'no_warn dc, x, y;',
        'AutoComplete(OFF);',
        'AutoComplete(ON);',
    )
    if any(text.count(anchor) != 1 for anchor in anchors):
        raise ValueError('pinned ZSplash sprite, renderer, pointer, or task-exit code changed')
    if 'DD_OS_NAME_VERSION' not in text or 'Simplicity Perfected' not in text:
        raise ValueError('pinned ZSplash title or motto changed')
    text = text.replace('Sleep((Now + counts.timer) & 0x7F);',
                        'Sleep(counts.timer & 0x7F);', 1)
    text = text.replace('MStrPrint("%S", "DD_OS_NAME_VERSION");',
                        'MStrPrint("%s", "ZealOS ARM64");', 1)
    text = text.replace('DCDepthBufAlloc(dc);',
        'if (!bootstrap_zsplash_depth_buf)\n'
        '\t\tbootstrap_zsplash_depth_buf = MAlloc(dc->width_internal * dc->height * sizeof(I32), dc->mem_task);\n'
        '\tdc->depth_buf = bootstrap_zsplash_depth_buf;\n'
        '\tDCDepthBufReset(dc);', 1)
    text = text.replace('U0 DrawIt(CTask *,CDC *dc)\n',
                        'I32 *bootstrap_zsplash_depth_buf;\n\nU0 DrawIt(CTask *,CDC *dc)\n', 1)
    text = text.replace('dc->r = old_r;',
                        'dc->r = old_r;\n\tdc->depth_buf = NULL;', 1)
    text = text.replace('no_warn dc, x, y;',
                        '// Native ARM64 cursor rendering stays enabled.', 1)
    text = text.replace('AutoComplete(OFF);',
                        '// Keep global autocomplete task state unchanged.', 1)
    text = text.replace('AutoComplete(ON);',
                        '// Keep global autocomplete task state unchanged.', 1)
    text = text.replace('CTask\t*bg_task = User;',
        'CTask *splash_task = Fs;\n'
        '\tCTask *old_focus = sys_focus_task;\n'
        '\tU0 (*old_draw_it)(CTask *task, CDC *dc) = Fs->draw_it;', 1)
    text = text.replace('WinToTop(bg_task);',
                        'WinToTop(splash_task);\n\tsys_focus_task = splash_task;', 1)
    text = text.replace('WinBorder(, bg_task);', 'WinBorder(, splash_task);', 1)
    text = text.replace('WinMax(bg_task);', 'WinMax(splash_task);', 1)
    text = text.replace('bg_task->', 'splash_task->')
    text = text.replace('gr.fp_draw_mouse = &DrawNoMouse;', '// Native USB pointer remains enabled.', 1)
    text = text.replace('MouseHardEnable(FALSE);', '', 1)
    text = text.replace('gr.fp_draw_mouse = &DrawStdMouse;', '', 1)
    text = text.replace('MouseHardEnable(TRUE);', '', 1)
    text = text.replace('Spawn(&MouseHardDriverInstall);', '', 1)
    loop_end = '\t\tΦ += .03;\n\t\tWinToTop;\n\t}\n'
    if text.count(loop_end) != 1:
        raise ValueError('pinned ZSplash animation loop boundary changed')
    text = text.replace(loop_end,
        loop_end +
        '\tif (bootstrap_zsplash_depth_buf)\n'
        '\t{\n'
        '\t\tFree(bootstrap_zsplash_depth_buf);\n'
        '\t\tbootstrap_zsplash_depth_buf = NULL;\n'
        '\t}\n'
        '\tFs->draw_it = old_draw_it;\n', 1)
    text = text.replace('Kill(bg_task);',
                        'if (TaskValidate(old_focus))\n\t\tsys_focus_task = old_focus;', 1)
    result = ('// ARM64 restores the packed pinned sprite bytes and uses the native USB pointer.\n' + text).encode('utf-8') + b'\0' + payload
    if (b'$SP,"<1>",BI=1$' not in result or
            b'Sprite3(dc, 0, 0, 0, $IB,"<1>",BI=1$);' not in result or
            b'bootstrap_zsplash_depth_buf = MAlloc(dc->width_internal * dc->height * sizeof(I32), dc->mem_task);' not in result or
            b'dc->depth_buf = bootstrap_zsplash_depth_buf;' not in result or
            b'Free(bootstrap_zsplash_depth_buf);' not in result or
            b'Fs->draw_it = old_draw_it;' not in result or
            b'MStrPrint("%s", "ZealOS ARM64");' not in result or
            b'bg_task' in result):
        raise ValueError('ARM64 ZSplash translation lost its embedded sprite tokens')
    return result


def extract_home_set(source: bytes) -> bytes:
    """Retain DiskDirB's shared, system-owned home path, without disk drivers."""
    begin, end = b'U0 HomeSet(U8 *dirname)\n', b'\nBool Cd('
    if not source.startswith(begin) or source.count(end) != 1:
        raise ValueError('expected the pinned HomeSet/Cd boundaries')
    body = source[:source.index(end)]
    anchor = b'\tdirname = DirNameAbs(dirname);\n'
    if body.count(anchor) != 1 or body.count(b'blkdev.home_dir = SysStrNew(dirname);') != 1:
        raise ValueError('pinned HomeSet allocation changed')
    # The bootstrap path boundary rejects unsupported drive syntax. Preserve
    # the old home when canonicalization fails instead of storing NULL.
    guard = (b"\tif (!dirname || *dirname != '/')\n\t{\n"
             b"\t\tFree(dirname);\n\t\tthrow('HomePath');\n\t}\n")
    return (b'// Original HomeSet with the source-store path failure boundary.\n' +
            body.replace(anchor, anchor + guard, 1))


def extract_version_scalars(source: bytes) -> bytes:
    text = source.decode('utf-8')
    match = re.search(r'F64\s+\*pow10_I64,\s+(sys_os_version = [0-9.]+;)\n'
                      r'U64\s+(sys_os_version_sub = [0-9]+;)', text)
    if not match:
        raise ValueError('pinned kernel version scalars changed')
    return ('// Version values from pinned Kernel/KGlobals.ZC.\nF64 ' +
            match[1] + '\nU64 ' + match[2] + '\n').encode()


def extract_source_prompt(source: bytes) -> bytes:
    begin, end = b'Bool YorN()\n', b'\nI64 PressAKey()\n'
    if source.count(begin) != 1 or source.count(end) != 1:
        raise ValueError('pinned YorN boundaries changed')
    body = begin + source.split(begin, 1)[1].split(end, 1)[0]
    return b'// Original keyboard-driven confirmation prompt.\n' + body + b'\n'


def translate_budget_strings(source: bytes) -> bytes:
    """Correct the pinned reader's two-byte comma literal at the file boundary."""
    old = b"if (*ptr == ', ')"
    if source.count(old) != 1:
        raise ValueError('pinned Budget separator check changed')
    return source.replace(old, b"if (*ptr == ',')", 1)


def extract_keyboard_helpers(source: bytes, printer: bytes) -> bytes:
    """Keep source key translation/naming, replacing only x86 DU8 table syntax."""
    import ast
    import re
    tables = []
    names = ('NORMAL_KEY_SCAN_DECODE_TABLE', 'SHIFT_KEY_SCAN_DECODE_TABLE',
             'CTRL_KEY_SCAN_DECODE_TABLE')
    assembly = source.split(b'\nU0 KbdCmdSend(', 1)[0].decode('utf-8')
    constants = {'CH_ESC': 27, 'CH_SHIFT_ESC': 28, 'CH_BACKSPACE': 8, 'CH_SPACE': 32}
    constants.update({'CH_CTRL'+chr(65+i): i+1 for i in range(26)})
    for i, name in enumerate(names):
        marker = name + '::'
        if assembly.count(marker) != 1:
            raise ValueError('pinned keyboard table changed: '+name)
        body = assembly.split(marker, 1)[1]
        body = body.split(names[i+1]+'::', 1)[0] if i < 2 else body.split('}', 1)[0]
        body = body.replace('DU8', '')
        values = []
        tokens = re.findall(r'''"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|CH_[A-Z_]+|\b0\b''', body)
        for token in tokens:
            if token in constants:
                values.append(constants[token])
            elif token == '0':
                values.append(0)
            else:
                value = ast.literal_eval(token.replace('\\`', '`')).replace('$$', '$')
                values.extend(value.encode('ascii'))
        if len(values) != 0x50:
            raise ValueError('pinned keyboard table length changed: '+name)
        tables.append(('U8 '+name+'[0x50] = {'+
                       ','.join('0x%02x'%v for v in values)+'};\n').encode())
    bodies = []
    for begin, end in [(b'I64 Char2ScanCode(', b'\nU8 scan_code_map['),
                       (b'U8 *Char2KeyName(', b'\nU0 KbdBuildSC(')]:
        if source.count(begin) != 1 or source.count(end) != 1:
            raise ValueError('pinned keyboard helper boundaries changed')
        bodies.append(begin+source.split(begin, 1)[1].split(end, 1)[0])
    # The original no-prefix control-character case reads an uninitialized
    # buffer; initialize it and reject out-of-range bitmap indexes.
    old = b'\tif (ch <= CH_SPACE)'
    if bodies[1].count(old) != 1:
        raise ValueError('pinned key-name buffer boundary changed')
    bodies[1] = bodies[1].replace(old, b'\t*buf = 0;\n\tif (ch < 0 || ch > 255) return StrNew("");\n'+old, 1)
    begin, end = b'U8 *CatPrint(', b'\nU0 Print('
    if printer.count(begin) != 1 or printer.count(end) != 1:
        raise ValueError('pinned CatPrint boundaries changed')
    cat_print = begin+printer.split(begin, 1)[1].split(end, 1)[0]
    return (b'// Portable helpers from pinned SerialDev/Keyboard.ZC; no PS/2 I/O.\n'
            + cat_print+b'\n'+b''.join(tables)+b'\n'+b'\n'.join(bodies)+b'\n')


def extract_string_utilities(str_a: bytes, str_b: bytes) -> bytes:
    """Keep the pinned allocating string transforms used by date/application input."""
    begin, end = b'I64 Spaces2Tabs(U8 *dst, U8 *src)\n', b'\nU8 *StrFirstOcc('
    if str_a.count(begin) != 1 or str_a.count(end) != 1:
        raise ValueError('pinned string utility boundaries changed')
    inplace = begin + str_a.split(begin, 1)[1].split(end, 1)[0]
    end = b'\nU0 GetOutOfDollar()'
    if not str_b.startswith(b'U8 *Tabs2Spaces(') or str_b.count(end) != 1:
        raise ValueError('pinned allocating string utility boundaries changed')
    allocating = str_b.split(end, 1)[0]
    return (b'// Original StrA/StrB string transforms and whitespace bitmap.\n'
            b'U32 char_bmp_white_space[16] = {0x80002600, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};\n'
            + inplace + b'\n' + allocating + b'\n')


def extract_source_dates(calendar: bytes, scan: bytes, printer: bytes) -> bytes:
    """Retain source calendar/parse/print routines; replace the CMOS boundary."""
    boundary = b'U8 CMOSRegRead(I64 register)\n'
    if calendar.count(boundary) != 1:
        raise ValueError('pinned calendar/CMOS boundary changed')
    body = calendar.split(boundary, 1)[0]
    # ModU64 is an x86 compiler intrinsic, not a callable kernel function.
    if body.count(b'ModU64(') != 4:
        raise ValueError('pinned date division dependency changed')
    body = body.replace(b'ModU64(', b'ArmDateDivMod(')
    # December must advance to January. Keep the source calendar otherwise,
    # with bounds checked before reading the month table's next element.
    old = b'\t\tds.mon = 0;\n\t\tds.year++;'
    if body.count(old) != 1:
        raise ValueError('pinned December carry changed')
    body = body.replace(old, b'\t\tds.mon = 1;\n\t\tds.year++;', 1)
    for table in (b'month_start_days', b'month_start_days_leap'):
        old = b'date >= '+table+b'[k + 1] && k < 11'
        if body.count(old) != 1:
            raise ValueError('pinned month search changed')
        body = body.replace(old, b'k < 11 && date >= '+table+b'[k + 1]', 1)
    old = b'\ti1 = YearStartDate(_ds->year);'
    if body.count(old) != 1:
        raise ValueError('pinned Struct2Date boundary changed')
    body = body.replace(old, b"\tif (!_ds || !(_ds->mon >= 1 && _ds->mon <= 12)) throw('Date');\n"+old, 1)
    begin, end = b'CDate Str2Date(U8 *_src)\n', b'\nU8 *StrScan('
    if scan.count(begin) != 1 or scan.count(end) != 1:
        raise ValueError('pinned Str2Date boundaries changed')
    parser = begin + scan.split(begin, 1)[1].split(end, 1)[0]
    # The month guard below can throw on malformed text. Reclaim the two
    # original parser buffers on that path as well as on success.
    start = b'\tMemSet(&ds, 0, sizeof(CDateStruct));'
    finish = b'\tFree(src);\n\tFree(v);'
    if parser.count(start) != 1 or parser.count(finish) != 1:
        raise ValueError('pinned date parser cleanup changed')
    parser = parser.replace(start, b'\ttry\n\t{\n'+start, 1)
    parser = parser.replace(finish,
        b'\t}\n\tcatch\n\t{\n\t\tFree(src);\n\t\tFree(v);\n\t}\n'+finish, 1)
    float_begin = b'F64 Str2F64(U8 *src, U8 **_end_ptr=NULL)\n'
    if scan.count(float_begin) != 1:
        raise ValueError('pinned floating point scanner changed')
    float_scan = float_begin + scan.split(float_begin, 1)[1].split(begin, 1)[0]
    scanner_begin = b'U8 *StrScan(U8 *src, U8 *format, ...)\n'
    if scan.count(scanner_begin) != 1:
        raise ValueError('pinned format scanner changed')
    scanner = scanner_begin + scan.split(scanner_begin, 1)[1]
    scan_switch = b'\t\t\t\tswitch (ch)\n'
    scan_free = b'\t\t\t\tFree(buf);'
    if scanner.count(scan_switch) != 1 or scanner.count(scan_free) != 1:
        raise ValueError('pinned format scanner cleanup changed')
    scanner = scanner.replace(scan_switch, b'\t\t\t\ttry\n\t\t\t\t{\n'+scan_switch, 1)
    scanner = scanner.replace(scan_free,
        b'\t\t\t\t}\n\t\t\t\tcatch\n\t\t\t\t{\n\t\t\t\t\tFree(buf);\n\t\t\t\t}\n'+scan_free, 1)
    begin, end = b'U8 *MPrintTime(CDate cdt)\n', b'\nU8 *MPrintQ('
    if printer.count(begin) != 1 or printer.count(end) != 1:
        raise ValueError('pinned date formatter boundaries changed')
    formatting = begin + printer.split(begin, 1)[1].split(end, 1)[0]
    prefix = b'''// Original KDate/StrScan/StrPrint calendar services, with an ARM64 clock.
CDate local_time_offset = 0; // UTC by default; callers may configure local time.
U64 ArmDateDivMod(U64 *value, U64 divisor)
{
    U64 remainder = *value % divisor;
    *value /= divisor;
    return remainder;
}
'''
    clock = b'''
CDate Now()
{//Limine's boot epoch is UTC; the generic counter supplies elapsed time.
    I64 ns = ZcUnixTimeNs();
    CDate result;
    result.date = YearStartDate(1970) + ns / 86400000000000;
    result.time = ToF64(ns % 86400000000000) * (4294967296.0 / 86400000000000.0);
    return result;
}
U0 NowDateTimeStruct(CDateStruct *ds)
{
    Date2Struct(ds, Now + local_time_offset);
}
'''
    return prefix + body + clock + parser + b'\n' + float_scan + b'\n' + scanner + b'\n' + formatting + b'\n'


def extract_source_training(source: bytes) -> bytes:
    begin = b'public U0 TipOfDay(U8 *tip_file = "::/Doc/Tips.DD")\n'
    if source.count(begin) != 1:
        raise ValueError('pinned TipOfDay signature changed')
    tip = begin + source.split(begin, 1)[1]
    anchor = b'\t"$$WW,1$$\\n";\n\twhile (TRUE)'
    if tip.count(anchor) != 1 or tip.count(b'I64\t\t\t i = RandU16;') != 1:
        raise ValueError('pinned TipOfDay selection loop changed')
    # The original circular scan chooses RandU16 modulo the tip count. Count
    # first to avoid 65535 complete-document scans and reject an empty/missing
    # tip file instead of spinning forever without yielding the desktop.
    guard = (b'\tI64 tip_count = 0;\n'
             b'\twhile (doc_e != doc)\n\t{\n'
             b"\t\tif (doc_e->type_u8 == DOCT_TEXT && *doc_e->tag == '*') tip_count++;\n"
             b'\t\tdoc_e = doc_e->next;\n\t}\n'
             b"\tif (!tip_count) { DocDel(doc2); DocDel(doc); throw('NoTips'); }\n"
             b'\ti %= tip_count;\n\tdoc_e = doc->head.next;\n')
    return (b'// Original TipOfDay with bounded equivalent selection and empty-file recovery.\n' +
            tip.replace(anchor, guard + anchor, 1))


def translate_docfile(source: bytes) -> bytes:
    """Normalize serialized binary resources before the unchanged DocLoad loop."""
    text = source.decode('utf-8')
    start = '\tCDocBin\t*tmpb;\n\n\tdoc->find_replace->filter_lines = 0;'
    if text.count(start) != 1:
        raise ValueError('pinned DocLoad local declarations changed')
    # Validate before locking: a catchable malformed resource must leave the
    # caller's document unlocked and usable for a subsequent load.
    lock = '\tBool\t unlock = DocLock(doc);'
    if text.count(lock) != 1:
        raise ValueError('DocLoad lock boundary changed')
    text = text.replace(lock, '\tBool\t unlock;', 1)
    text = text.replace(start, '\tCDocBin\t*tmpb;\n'
        '\tU8 *arm_owned = ZcDocInputBytes(src2, size, &size);\n'
        '\tif (arm_owned) src2 = arm_owned;\n'
        '\tunlock = DocLock(doc);\n\n'
        '\tdoc->find_replace->filter_lines = 0;', 1)
    end = '\tDocTop(doc); //Calls $LK,"DocRecalc",A="MN:DocRecalc"$().  DOCT_CURSOR will be set.\n'
    if text.count(end) != 1:
        raise ValueError('pinned DocLoad cleanup boundary changed')
    text = text.replace(end, end+'\tFree(arm_owned);\n', 1)
    return text.encode('utf-8')


def translate_doctree(source: bytes) -> bytes:
    text = source.decode('utf-8')
    for old, new, count in (
        ('DriveIsWritable(*doc->filename.name)', 'ZcSourceWritable(doc->filename.name)', 2),
        ('CompCtrlNew(, ccf_flags | CCF_DONT_FREE_BUF)',
         'ZcUserCompCtrlNew(, ccf_flags | CCF_DONT_FREE_BUF)', 1),
        ('LexAttachDoc(cc,, doc);', 'ArmLexAttachDoc(cc, doc);', 1),
        ('\t\tLex(cc);', '\t\tZcUserLex(cc);', 1),
        ('\tQueueRemove(cc);\n\tif (okay)\n\t\tCompCtrlDel(cc); //TODO: can crash',
         '\tif (cc)\n\t{\n\t\tQueueRemove(cc);\n\t\tZcUserCompCtrlDel(cc);\n\t}', 1),
    ):
        if text.count(old) != count:
            raise ValueError('pinned DocTree dependency changed: ' + old)
        text = text.replace(old, new)
    # Every exception leaves the private compiler controller and document
    # lock usable, including non-Compiler exceptions that keep propagating.
    anchor = '\t\t\tres = 0;\n\t\t}\n\t}\n\tif (cc)'
    replacement = ('\t\t\tres = 0;\n\t\t}\n'
        '\t\tZcUserCompCtrlDel(cc);\n\t\tcc = NULL;\n'
        '\t\tif (unlock_doc) DocUnlock(doc);\n\t\tunlock_doc = FALSE;\n\t}\n\tif (cc)')
    if text.count(anchor) != 1:
        raise ValueError('pinned ExeDoc catch changed')
    text = text.replace(anchor, replacement, 1)
    # Only the snapshot copy needs to stay locked during user code. A child
    # killed while that code yields must not strand the System registry lock.
    anchor = '\t\tdoc2=DocCopy(doc, tree_branch, end_indent);'
    if text.count(anchor) != 1:
        raise ValueError('pinned DocTreeExe copy changed')
    text = text.replace(anchor, anchor + '\n\t\tif (unlock_doc) DocUnlock(doc);\n'
                        '\t\tunlock_doc = FALSE;', 1)
    anchor = '\t\tres=ExeDoc(doc2);'
    if text.count(anchor) != 1:
        raise ValueError('pinned DocTreeExe body changed')
    text = text.replace(anchor, '\t\ttry\n\t\t{\n\t\t\tres=ExeDoc(doc2);\n\t\t}\n\t\tcatch\n\t\t{\n'
        '\t\t\tif (unlock_doc) DocUnlock(doc);\n\t\t}', 1)
    return text.encode()


def translate_registry(source: bytes) -> bytes:
    text = source.decode('utf-8')
    old = 'DriveIsWritable(*sys_registry_doc->filename.name)'
    if text.count(old) != 1:
        raise ValueError('pinned Registry writable guard changed')
    text = text.replace(old, 'ZcSourceWritable(sys_registry_doc->filename.name)')
    anchor = '\tif (!sys_registry_doc)\n\t{\n'
    if text.count(anchor) != 1:
        raise ValueError('pinned registry cache ownership changed')
    text = text.replace(anchor, anchor + '\t\tif (Fs != sys_task)\n\t\t{\n'
        '\t\t\tSys("RegCache;");\n\t\t\treturn FALSE;\n\t\t}\n', 1)
    return text.encode()


def translate_docterm(source: bytes) -> bytes:
    """Evaluate the title length in ZealC before the bounded document lexer."""
    old = 'LEN=STR_LEN-1,A=\\"%%s...\\",SCX=15$$");'
    new = 'LEN=%d,A=\\"%%s...\\",SCX=15$$", STR_LEN - 1);'
    text = source.decode('utf-8')
    if text.count(old) != 1:
        raise ValueError('pinned DocBorderNew title length changed')
    return text.replace(old, new, 1).encode('utf-8')


def translate_autocomplete_task(source: bytes) -> bytes:
    """Render AutoComplete suggestions into the popup's DolDoc document."""
    text = source.decode('utf-8')
    old = '\tWinHorz(left, Fs->win_right);\n\tWinVert(top,  Fs->win_bottom);'
    new = ('\t// Spawned ARM64 tasks may not yet have TaskInit window bounds.\n'
           '\t// Keep the anchored completion popup large enough to render.\n'
           '\tif (Fs->win_right <= left)\n'
           '\t\tFs->win_right = TEXT_COLS - 2;\n'
           '\tif (Fs->win_bottom <= top)\n'
           '\t\tFs->win_bottom = TEXT_ROWS - 2;\n' + old)
    if text.count(old) != 1:
        raise ValueError('pinned AutoComplete popup geometry changed')
    text = text.replace(old, new, 1)

    # Aiwnios lowers a bare string statement to the host Print binding. In this
    # ARM64 port that binding is a task-text output path, so it bypasses
    # AutoComplete's DolDoc and leaves the popup document empty. Use the normal
    # DolDoc printer explicitly for every suggestion row instead.
    print_calls = (
        ('"$$RED$$Word:%s$$FG$$\\n", ac.cur_word;',
         'DocPrint(DocPut, "$$RED$$Word:%s$$FG$$\\n", ac.cur_word);'),
        ('"$$GREEN$$F%02d$$FG$$$$HL$$ ", i + 1;',
         'DocPrint(DocPut, "$$GREEN$$F%02d$$FG$$$$HL$$ ", i + 1);'),
        ('"$$TX+UL+L+PU,\\"%$$Q\\",A=\\"%s\\"$$$$IV,0$$\\n", st, tmph->src_link;',
         'DocPrint(DocPut, "$$TX+UL+L+PU,\\"%$$Q\\",A=\\"%s\\"$$$$IV,0$$\\n", st, tmph->src_link);'),
        ('"%s\\n", st;', 'DocPrint(DocPut, "%s\\n", st);'),
        ('"$$HL,0$$";', 'DocPrint(DocPut, "$$HL,0$$");'),
    )
    for old, new in print_calls:
        if text.count(old) != 1:
            raise ValueError(f'pinned AutoComplete popup output changed: {old}')
        text = text.replace(old, new, 1)
    old = '\t\t\t\t\t\t"$$IV$$";'
    new = '\t\t\t\t\t\tDocPrint(DocPut, "$$IV$$");'
    if text.count(old) != 1:
        raise ValueError('pinned AutoComplete public-link marker changed')
    text = text.replace(old, new, 1)
    old = "'\\n';"
    new = 'DocPrint(DocPut, "\\n");'
    if text.count(old) != 1:
        raise ValueError('pinned AutoComplete dictionary row break changed')
    text = text.replace(old, new, 1)
    old = '"$$GREEN$$\'%d\'$$FG$$ $$BLACK$$%-23ts$$FG$$", i, ptr;'
    new = 'DocPrint(DocPut, "$$GREEN$$\'%d\'$$FG$$ $$BLACK$$%-23ts$$FG$$", i, ptr);'
    if text.count(old) != 1:
        raise ValueError('pinned AutoComplete dictionary row changed')
    text = text.replace(old, new, 1)

    old = '\t\t\tACPutChoices(doc, doc_e, focus_task, ToBool(sc != last_sc));'
    new = (old + '\n'
           '\t\t\t// Recalc size-to-content after inserting the suggestions.\n'
           '\t\t\tDocRecalc(DocPut);')
    if text.count(old) != 1:
        raise ValueError('pinned AutoComplete choice-update call changed')
    return text.replace(old, new, 1).encode('utf-8')


def translate_doced(source: bytes) -> bytes:
    """Use the same evaluated title length in the editor's border document."""
    old = 'LEN=STR_LEN-1,A=\\"%%s\\"$$");'
    new = 'LEN=%d,A=\\"%%s\\"$$", STR_LEN - 1);'
    text = source.decode('utf-8')
    if text.count(old) != 1:
        raise ValueError('pinned DocEd title length changed')
    return text.replace(old, new, 1).encode('utf-8')


def extract_server_task(source: bytes) -> bytes:
    """Load the original server loop before KTask's compiler-dependent loop.

    Keep the control flow intact; route JobsHandler through the native port's
    compiler/unwind boundary, as required by the existing source-job bridge.
    """
    text = source.decode("utf-8")
    begin = "U0 ServerTaskCont()\n"
    end = "\nU0 UserTaskCont()\n"
    if text.count(begin) != 1 or text.count(end) != 1:
        raise ValueError("expected the pinned server/user loop boundaries")
    body = text[text.index(begin):text.index(end)]
    for anchor in ("FlushMessages;", "JobsHandler(old_flags)",
                   "LBts(&Fs->task_flags, TASKf_IDLE);",
                   "LBts(&Fs->task_flags, TASKf_AWAITING_MESSAGE);",
                   "Yield;", "RFlagsSet(old_flags);"):
        if anchor not in body:
            raise ValueError(f"pinned ServerTaskCont changed: {anchor}")
    body = body.replace("JobsHandler(old_flags)", "ZcServerJobsHandler(old_flags)", 1)
    return ("// Kernel/KTask.ZC server loop with the ARM64 job-dispatch boundary.\n"
            "extern I64 ZcServerJobsHandler(I64 flags);\n"
            "extern I64 ZcSystemTaskStart(U0 (*entry)());\n"
            "extern I64 ZcCallExtStr(U8 *name);\n" + body +
            "\nI64 BootstrapSystemTaskStart()\n"
            "{\n"
            "    I64 result = ZcSystemTaskStart(&ServerTaskCont);\n"
            "    if (result == 42) LBts(&sys_run_level, RLf_SYSTEM_SERVER);\n"
            "    return result;\n"
            "}\n"
            "U0 BootstrapServerTaskStart()\n"
            "{\n    Fs->win_inhibit = WIG_USER_TASK_DEFAULT;\n"
            "    ZcCallExtStr(\"ServerStartUp\");\n"
            "    ServerTaskCont;\n}\n").encode("utf-8")


def translate_winmgr(source: bytes) -> bytes:
    """Adapt the source manager redraw and USB mouse menu wait loop.

    The upstream manager otherwise calls GrUpdateScreen from WinMgrSleep,
    bypassing the host's staged framebuffer transaction. Poll native USB input
    in the manager task immediately before its top-menu held-button check, and
    during the held selection loop so clicks, drags and release stay live.
    Require exact anchors for all changes.
    """
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("WinMgr.ZC is not UTF-8") from exc
    call = "\t\tGrUpdateScreen;"
    if text.count(call) != 1:
        raise ValueError(
            "expected exactly one WinMgrSleep screen refresh call; "
            f"found {text.count(call)}"
        )
    if "ZcTaskFrameRun" in text:
        raise ValueError("WinMgr.ZC already contains the frame-run hook")
    text = text.replace(call, "\t\tZcTaskFrameRun;", 1)
    menu_press = "\t\t\t\t\tif (mouse.lb && !old_mouse.lb)\n"
    if text.count(menu_press) != 1:
        raise ValueError(
            "expected exactly one edge-triggered top-menu press check; "
            f"found {text.count(menu_press)}"
        )
    text = text.replace(
        menu_press,
        "\t\t\t\t\tBootstrapCtrlInputPoll(TRUE);\n"
        "\t\t\t\t\tBootstrapMenuFocusRestore;\n"
        "\t\t\t\t\tif (mouse.lb)\n",
        1,
    )
    menu_show_poll = ("\t\t\t\t\twinmgr.show_menu = TRUE;\n"
                      "\t\t\t\t\tBootstrapCtrlInputPoll(TRUE);\n")
    if text.count(menu_show_poll) != 1:
        raise ValueError(
            "expected exactly one top-menu show/poll sequence; "
            f"found {text.count(menu_show_poll)}"
        )
    text = text.replace(
        menu_show_poll,
        "\t\t\t\t\twinmgr.show_menu = TRUE;\n"
        "\t\t\t\t\tBootstrapMenuFocusRemember;\n"
        "\t\t\t\t\tBootstrapCtrlInputPoll(TRUE);\n",
        1,
    )
    menu_wait = ("\t\t\t\t\t\twhile (mouse.lb)\n"
                 "\t\t\t\t\t\t\tWinMgrSleep(TRUE);")
    if text.count(menu_wait) != 1:
        raise ValueError(
            "expected exactly one top-menu mouse wait loop; "
            f"found {text.count(menu_wait)}"
        )
    text = text.replace(
        menu_wait,
        "\t\t\t\t\t\twhile (mouse.lb)\n"
        "\t\t\t\t\t\t{\n"
        "\t\t\t\t\t\t\tBootstrapCtrlInputPoll(TRUE);\n"
        "\t\t\t\t\t\t\tBootstrapMenuFocusRestore;\n"
        "\t\t\t\t\t\t\tWinMgrSleep(TRUE);\n"
        "\t\t\t\t\t\t\tBootstrapMenuFocusRestore;\n"
        "\t\t\t\t\t\t}",
        1,
    )
    menu_dispatch = ("\t\t\t\t\t\tif (sys_cur_submenu_entry)\n"
                     "\t\t\t\t\t\t\tTaskMessage(sys_focus_task, 0,")
    if text.count(menu_dispatch) != 1:
        raise ValueError(
            "expected exactly one top-menu action dispatch; "
            f"found {text.count(menu_dispatch)}"
        )
    text = text.replace(
        menu_dispatch,
        "\t\t\t\t\t\tBootstrapMenuFocusRestore;\n"
        + menu_dispatch,
        1,
    )
    menu_done = "\t\t\t\t\t\twinmgr.show_menu = FALSE;\n"
    if text.count(menu_done) != 1:
        raise ValueError(
            "expected exactly one top-menu close after selection; "
            f"found {text.count(menu_done)}"
        )
    text = text.replace(
        menu_done,
        menu_done + "\t\t\t\t\t\tBootstrapMenuFocusRestore(TRUE);\n",
        1,
    )
    return text.encode("utf-8")


def translate_filemgr(source: bytes) -> bytes:
    """Add the merged source volume to the unchanged ZealOS FileMgr tree.

    The bring-up filesystem has a persistent source namespace alongside
    optional block drives. FilesFind returns a flat listing there, so the
    bootstrap builds the CDirEntry tree consumed by FileMgr. Guard unmounted
    drive slots before following their block-device pointers.
    """
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("FileMgr.ZC is not UTF-8") from exc
    marker = "U0 FMRebuildDoc(CDoc **_doc, CDirEntry **_head, I64 mode)\n"
    if text.count(marker) != 1:
        raise ValueError("expected exactly one FMRebuildDoc definition")
    if "FMRebuildDocSourceVolume" in text:
        raise ValueError("FileMgr.ZC already contains the source-volume adapter")
    for old, new in (
        ('tmpde->user_data = DocPrint(doc, "$$TR,\\\"%s\\\",U=0x%X$$", tmpde->name, tmpde);',
         'tmpde->user_data = DocPrint(doc, "$$TR,\\\"%s\\\"$$", tmpde->name);\n'
         '\t\t\ttmpde->user_data(CDocEntry *)->user_data = tmpde;'),
        ('tmpde->user_data = DocPrint(doc, "$$MU,\\\"%s\\\",U=0x%X$$", tmpde->name, tmpde);',
         'tmpde->user_data = DocPrint(doc, "$$MU,\\\"%s\\\"$$", tmpde->name);\n'
         '\t\t\ttmpde->user_data(CDocEntry *)->user_data = tmpde;'),
        ('tmpde->user_data = DocPrint(doc, "$$TR,\\\"%s\\\",U=0x%X$$", tmpde->full_name, tmpde);',
         'tmpde->user_data = DocPrint(doc, "$$TR,\\\"%s\\\"$$", tmpde->full_name);\n'
         '\ttmpde->user_data(CDocEntry *)->user_data = tmpde;'),
    ):
        if text.count(old) != 1:
            raise ValueError("expected exactly one FileMgr DocEntry pointer format")
        text = text.replace(old, new, 1)
    space_open = (
        "\t\t\t\t\tcase CH_SPACE:\n"
        "\t\t\t\t\t\tif (doc->cur_entry->type_u8 == DOCT_MENU_VAL)\n"
        "\t\t\t\t\t\t{\n"
        "\t\t\t\t\t\t\tDocUnlock(doc);\n"
        "\t\t\t\t\t\t\tEd(doc->cur_entry->user_data(CDirEntry *)->full_name);\n"
        "\t\t\t\t\t\t\tDocLock(doc);\n"
        "\t\t\t\t\t\t}\n"
        "\t\t\t\t\t\telse\n"
        "\t\t\t\t\t\t\tgoto fm_regular_key;\n"
        "\t\t\t\t\t\tbreak;"
    )
    safe_space_open = (
        "\t\t\t\t\tcase CH_SPACE:\n"
        "\t\t\t\t\t\tif (doc->cur_entry->type_u8 == DOCT_MENU_VAL &&\n"
        "\t\t\t\t\t\t\t(tmpde = doc->cur_entry->user_data) && tmpde->full_name)\n"
        "\t\t\t\t\t\t{\n"
        "\t\t\t\t\t\t\tDocUnlock(doc);\n"
        "\t\t\t\t\t\t\tBootstrapGuiTerminalEditFile(tmpde->full_name);\n"
        "\t\t\t\t\t\t\tDocLock(doc);\n"
        "\t\t\t\t\t\t}\n"
        "\t\t\t\t\t\telse\n"
        "\t\t\t\t\t\t\tgoto fm_regular_key;\n"
        "\t\t\t\t\t\tbreak;"
    )
    if text.count(space_open) != 1:
        raise ValueError("expected exactly one FileMgr Space-to-open branch")
    text = text.replace(space_open, safe_space_open, 1)
    click_open = "\t\t\t\t\t\t\tEd(doc_e->user_data(CDirEntry *)->full_name);\n"
    if text.count(click_open) != 1:
        raise ValueError("expected exactly one FileMgr click-to-open branch")
    text = text.replace(
        click_open,
        "\t\t\t\t\t\t\tBootstrapGuiTerminalEditFile(\n"
        "\t\t\t\t\t\t\t\tdoc_e->user_data(CDirEntry *)->full_name);\n",
        1,
    )
    helper = (
        "extern Bool BootstrapGuiTerminalEditFile(U8 *path);\n"
        "extern CDirEntry *BootstrapSourceFilesTree();\n"
        "CDirEntry *FMRebuildDocSourceVolume(CDoc *doc, CDirEntry **_head)\n"
        "{\n"
        "\tCDirEntry *root = BootstrapSourceFilesTree();\n"
        "\tif (!root)\n"
        "\t\treturn NULL;\n"
        "\troot->user_data = DocPrint(doc, \"$$TR,\\\"Source /\\\"$$\");\n"
        "\troot->user_data(CDocEntry *)->user_data = root;\n"
        "\troot->next = *_head;\n"
        "\t*_head = root;\n"
        "\tDocPrint(doc, \"\\n$$ID,+2$$\");\n"
        "\tDocBottom(doc);\n"
        "\tDirFileDoc(doc, root->sub);\n"
        "\tDocPrint(doc, \"$$ID,-2$$\");\n"
        "\treturn root;\n"
        "}\n\n"
    )
    text = text.replace(marker, helper + marker, 1)
    drive_loop = "\tfor (i = 0; i < DRIVES_NUM; i++)\n"
    if text.count(drive_loop) != 1:
        raise ValueError("expected exactly one FileMgr drive-list loop")
    text = text.replace(drive_loop, "\tif (blkdev.drvs)\n\t\tfor (i = 0; i < DRIVES_NUM; i++)\n", 1)
    drive = "\t\tdrive = &blkdev.drvs[i];\n"
    if text.count(drive) != 1:
        raise ValueError("expected exactly one FileMgr drive-list assignment")
    text = text.replace(drive, drive + "\t\tif (drive->bd)\n\t\t{\n", 1)
    clip_reset = "\tDocReset(doc2, TRUE);\n"
    if text.count(clip_reset) != 1:
        raise ValueError("expected exactly one FileMgr clipboard reset")
    text = text.replace(clip_reset, "\tif (doc2)\n\t\tDocReset(doc2, TRUE);\n", 1)
    source_root = (
        "\t\telse if (drive->fs_type == FSt_REDSEA || drive->fs_type == FSt_FAT32)\n"
        "\t\t\tFMRebuildDocDrive(Drive2Letter(drive), doc, _head, TRUE);\n"
        "\t}\n"
        "\tDocTop(doc);"
    )
    if text.count(source_root) != 1:
        raise ValueError("expected exactly one end of FileMgr drive-list loop")
    text = text.replace(
        source_root,
        source_root.replace(
            "\n\t}\n\tDocTop(doc);",
            "\n\t\t}\n\t}\n\tFMRebuildDocSourceVolume(doc, _head);\n\tDocTop(doc);"),
        1,
    )
    return text.encode("utf-8")


def translate_grdc(source: bytes) -> bytes:
    """Allocate every requested pixel, with rows rounded up to eight bytes.

    The pinned DCNew rounds down instead. Odd-width brush drawing can then
    write beyond the last row and corrupt the following heap allocation.
    """
    old = (b'//\tres->width_internal\t= (width + 7) & ~7;\n'
           b'\tres->width_internal\t= width & ~7;')
    if source.count(old) != 1:
        raise ValueError("expected the pinned DCNew row-stride calculation")
    return source.replace(old, b'\tres->width_internal\t= (width + 7) & ~7;', 1)


def translate_docrecalc(source: bytes) -> bytes:
    """Release per-frame depth buffers before recursive document trimming.

    The pinned full-refresh path recursively recalculates after deleting old
    entries. A mesh sprite allocates a screen-sized depth buffer in each
    frame, so retaining those locals until recursion unwinds can exhaust the
    bounded guest heap. Keep the upstream redraw and trim order, but release
    each completed frame's CDC and depth buffer before descending.
    """
    try:
        text = source.decode('utf-8')
    except UnicodeDecodeError as exc:
        raise ValueError('DocRecalc.ZC is not UTF-8') from exc
    anchors = (
        ('\t\t\tDocDelToEntry(doc, next_clear_found, clear_holds);\n'
         '\t\t\tDocRecalc(doc, recalc_flags);'),
        ('\t\t\tDocDelToNum(doc, i);\n'
         '\t\t\tDocRecalc(doc, recalc_flags);'),
    )
    if any(text.count(anchor) != 1 for anchor in anchors):
        raise ValueError('expected both pinned recursive DocRecalc trim paths')
    cleanup = ('\t\t\tDCDel(dc);\n'
               '\t\t\tdc = NULL;\n'
               '\t\t\tFree(depth_buf);\n'
               '\t\t\tdepth_buf = NULL;\n')
    for anchor, trim in zip(anchors, ('DocDelToEntry', 'DocDelToNum')):
        before, recurse = anchor.split('\n', 1)
        text = text.replace(anchor, before + '\n' + cleanup + recurse, 1)
    return text.encode('utf-8')


def translate_grbitmap(source: bytes) -> bytes:
    """Apply ARM-safe bitmap blitting and extract regular CDC rectangles.

    Pinned DCExt creates a result CDC sized to the requested source rectangle,
    then calls GrBlot at (-x1, -y1). GrBlot's negative-origin clipping is only
    valid for a larger/screen destination; for an ordinary tightly-sized CDC
    the path can address before the result buffer. Keep screen-bitmap handling
    on the upstream path and copy rows directly for regular CDCs. The packed
    no-transparency GrBlot path also combines unaligned source words with
    pre/post-incremented pointers in one expression; on AArch64 this loses
    source bytes. Keep its fast path for aligned blits and use an equivalent
    pixel-safe ROP path for unaligned destinations.
    """
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("GrBitMap.ZC is not UTF-8") from exc
    signature = "public CDC *DCExt(CDC *dc=gr.dc, I64 x1, I64 y1, I64 x2, I64 y2, CTask *task=NULL)"
    start = text.find(signature)
    if start < 0 or text.find(signature, start + len(signature)) >= 0:
        raise ValueError("expected exactly one pinned DCExt definition")
    end_anchor = "\npublic CDC *DCDiff(CDC *base, CDC *update)"
    if text.count(end_anchor) != 1:
        raise ValueError("expected the pinned DCExt/DCDiff boundary")
    end = text.index(end_anchor, start)
    body = text[start:end]
    if (body.count("\tCTask *win_task;") != 1 or
            body.count("\tGrBlot(res, -x1, -y1, dc);") != 1 or
            body.count("res = DCNew(x2 - x1 + 1, y2 -y1 + 1, task);") != 1):
        raise ValueError("pinned DCExt extraction sequence changed")
    body = body.replace("\tCTask *win_task;",
                        "\tCTask *win_task;\n"
                        "\tI64 row, width, copy_x1, copy_y1, copy_x2, copy_y2;", 1)
    body = body.replace(
        "\tres = DCNew(x2 - x1 + 1, y2 -y1 + 1, task);",
        "\twidth = x2 - x1 + 1;\n"
        "\tres = DCNew((width + 7) & ~7, y2 - y1 + 1, task);\n"
        "\tres->width = width;",
        1,
    )
    body = body.replace(
        "\tGrBlot(res, -x1, -y1, dc);",
        """\tif (dc->flags & DCF_SCREEN_BITMAP)
		GrBlot(res, -x1, -y1, dc);
		else
		{
			copy_x1 = x1;
			if (copy_x1 < 0)
				copy_x1 = 0;
			copy_y1 = y1;
			if (copy_y1 < 0)
				copy_y1 = 0;
			copy_x2 = x2;
			if (copy_x2 >= dc->width)
				copy_x2 = dc->width - 1;
			copy_y2 = y2;
			if (copy_y2 >= dc->height)
				copy_y2 = dc->height - 1;
			if (copy_x1 <= copy_x2 && copy_y1 <= copy_y2)
			{
				width = copy_x2 - copy_x1 + 1;
				for (row = 0; row <= copy_y2 - copy_y1; row++)
					MemCopy(res->body + (copy_y1 + row - y1) *
							res->width_internal + copy_x1 - x1,
						dc->body + (copy_y1 + row) *
							dc->width_internal + copy_x1,
						width);
			}
		}""",
        1,
    )
    text = text[:start] + body + text[end:]
    blot_signature = "public I64 GrBlot(CDC *dc=gr.dc, I64 x, I64 y, CDC *img)"
    blot_end_anchor = "\n#help_index \"Graphics/Device Contexts\"\n\nU8 *GrBitMap4ToBitMap8"
    if text.count(blot_signature) != 1 or text.count(blot_end_anchor) != 1:
        raise ValueError("expected the pinned GrBlot definition boundaries")
    blot_start = text.index(blot_signature)
    blot_end = text.index(blot_end_anchor, blot_start)
    blot = text[blot_start:blot_end]
    packed_copy = "if (img->flags & DCF_NO_TRANSPARENTS)"
    if blot.count(packed_copy) != 3:
        raise ValueError("expected the three packed no-transparency ROP paths")
    draw_guard = "\t\tif (dc->flags & DCF_DONT_DRAW)\n\t\t\treturn 1;"
    if blot.count(draw_guard) != 1:
        raise ValueError("expected the pinned GrBlot no-draw return")
    safe_unaligned = """\t\tif ((img->flags & DCF_NO_TRANSPARENTS) && (-x & 7) &&
				(dc->color.c0.rop == ROPB_COLLISION || dc->color.c0.rop == ROPB_MONO ||
				 dc->color.c0.rop == ROPB_EQU || dc->color.c0.rop == ROPB_XOR))
		{
			old_color = dc->color;
			db = dc->depth_buf;
			dc->depth_buf = NULL;
			dc->color &= ~ROPF_DITHER;
			color = dc->color;
			if (dc->flags & DCF_SCREEN_BITMAP)
				win_z_num = win_task->win_z_num;
			k = h1 * img->width_internal;
			for (j = h1; j < h2; j++)
			{
				for (i = w1; i < w2; i++)
				{
					color_mask = TRUE;
					if (dc->flags & DCF_SCREEN_BITMAP && !(dc->flags & DCF_ON_TOP))
					{
						win_z_buf_ptr = gr.win_z_buf(U8 *) +
								((y + j) / FONT_HEIGHT * TEXT_COLS + (x + i) / FONT_WIDTH) * sizeof(U16);
						if (win_z_num < *win_z_buf_ptr)
							color_mask = FALSE;
					}
					if (color_mask)
					{
						c = img->body[k + i];
						switch (color.c0.rop)
						{
							case ROPB_COLLISION:
								if (dc->body[(y + j) * dc->width_internal + x + i] != TRANSPARENT &&
										dc->body[(y + j) * dc->width_internal + x + i] != dc->bkcolor.c0.color)
									dc->collision_count++;
								break;
							case ROPB_MONO:
								dc->color.c0.color = color.c0.color & c;
								GrPlot0(dc, x + i, y + j);
								break;
							case ROPB_EQU:
							case ROPB_XOR:
								dc->color.c0.color = c;
								GrPlot0(dc, x + i, y + j);
								break;
						}
					}
				}
				k += img->width_internal;
			}
			dc->depth_buf = db;
			dc->color = old_color;
			return 1;
		}
"""
    blot = blot.replace(draw_guard, draw_guard + "\n" + safe_unaligned.rstrip("\n"), 1)
    collision_start_anchor = "\t\t\tcase ROPB_COLLISION: //TODO: Might want to check win_z_buf\n"
    collision_end_anchor = "\n\t\t\tcase ROPB_MONO:"
    if blot.count(collision_start_anchor) != 1:
        raise ValueError("expected the pinned GrBlot collision z-buffer TODO")
    collision_start = blot.index(collision_start_anchor)
    collision_end = blot.index(collision_end_anchor, collision_start)
    collision_case = """\t\t\tcase ROPB_COLLISION:
				color = dc->bkcolor.c0.color;
				k = h1 * img->width_internal;
				k1 = (h1 + y) * dc->width_internal + x;
				for (j = h1; j < h2; j++)
				{
					for (i = w1; i < w2; i++)
					{
						if (img->body[k + i] != TRANSPARENT &&
								(!(dc->flags & DCF_SCREEN_BITMAP) ||
								 dc->flags & DCF_ON_TOP || !gr.win_z_buf ||
								 win_task->win_z_num >= gr.win_z_buf[
										(y + j) / FONT_HEIGHT * TEXT_COLS +
										(x + i) / FONT_WIDTH]))
						{
							c = dc->body[k1 + i];
							if (c != TRANSPARENT && c != color)
								dc->collision_count++;
						}
					}
					k += img->width_internal;
					k1 += dc->width_internal;
				}
				break;
"""
    blot = blot[:collision_start] + collision_case + blot[collision_end:]
    text = text[:blot_start] + blot + text[blot_end:]
    return text.encode("utf-8")


def translate_mathode(source: bytes) -> bytes:
    """Work around a parser failure on inline address-of-member call args.

    The expression is equivalent and the guarded replacement keeps the
    upstream mirror pristine while compiling the arm64 source volume.
    """
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("MathODE.ZC is not UTF-8") from exc
    signature = "U0 ODEMPTask(CMathODE *ode)\n{"
    call = "\t\twhile (!Bt(&ode->mp_not_done_flags, Gs->num))"
    reset = "\t\tLBtr(&ode->mp_not_done_flags, Gs->num);"
    if (text.count(signature) != 1 or text.count(call) != 1 or
            text.count(reset) != 1):
        raise ValueError("expected the pinned ODEMPTask bit-test expression")
    text = text.replace(signature, signature + "\n\tU8 *mp_flags = &ode->mp_not_done_flags;", 1)
    text = text.replace(call, "\t\twhile (!Bt(mp_flags, Gs->num))", 1)
    text = text.replace(reset, "\t\tLBtr(mp_flags, Gs->num);", 1)
    return text.encode("utf-8")


def translate_docpopup(source: bytes) -> bytes:
    """Remove DocPopUp's one inline DolDoc word-wrap directive.

    It controls source-document display, not program evaluation. The native
    text lexer has no raw DolDoc mode; retain every executable token.
    """
    directive = b'return $WW,0$PopUp2("YES", 1, "NO", 0, header, footer) > 0;'
    if source.count(directive) != 1 or source.count(b'$WW,0$') != 1:
        raise ValueError("expected the single pinned PopUpNoYes word-wrap directive")
    return source.replace(directive, directive.replace(b'$WW,0$', b''), 1)


def translate_docform(source: bytes) -> bytes:
    """Fix the bit-index argument in the pinned form's DEFAULT_LEN test.

    ZealOS's LBtr primitive takes a flag word and a numeric bit index. This
    source passes the address of the index constant, which aborts the current
    compiler. The neighboring DEFAULT_RAW_TYPE call already passes the index.
    Keep the mirror untouched and require the single expected call site.
    """
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("DocForm.ZC is not UTF-8") from exc
    bad = "LBtr(&doc_e->de_flags, &DOCEf_DEFAULT_LEN)"
    good = "LBtr(&doc_e->de_flags, DOCEf_DEFAULT_LEN)"
    if text.count(bad) != 1 or text.count(good):
        raise ValueError("expected the pinned DocForm DEFAULT_LEN bit test")
    text = text.replace(bad, good, 1)
    start = text.index('public Bool DocForm(')
    end = text.index('\nU0 DocMenuEndTaskCB()', start)
    form = text[start:end]
    replacements = {
        'CMemberList\t*ml;': 'CArmDocFormField arm_field;\n\tI64 arm_index = 0;',
        '\tCHashClass\t*tmpc, *tmpc2;\n': '',
        'if (!(tmpc = HashFind(class_name, Fs->hash_table, HTT_CLASS)))\n\t\treturn FALSE;':
            'if (!ZcDocFormField(class_name, 0, NULL))\n\t\treturn FALSE;',
        '\tml = tmpc->member_list_and_root;\n\twhile (ml)':
            '\twhile (ZcDocFormField(class_name, arm_index++, &arm_field))',
        'MemberMetaData("format", ml)': 'arm_field.format',
        '\t\t\ttmpc2 = ml->member_class;\n': '',
        '!tmpc2->ptr_stars_count': '!arm_field.pointer',
        '\t\t\t\ttmpc2 = OptClassFwd(tmpc2);\n\t\t\t\ttmpc2 -= tmpc2->ptr_stars_count;\n': '',
        'tmpc2->type & HTT_INTERNAL_TYPE': 'arm_field.is_internal',
        'ml->dim.next': 'arm_field.is_array',
        'ml->dim.total_count': 'arm_field.elements',
        'tmpc2->raw_type': 'arm_field.raw_type',
        'ml->offset': 'arm_field.offset',
        'MemberMetaData("data", ml)': 'arm_field.data',
        '\t\tml = ml->next;\n': '',
    }
    for old, new in replacements.items():
        if old not in form:
            raise ValueError('pinned DocForm reflection changed: '+old)
        form = form.replace(old, new)
    return (text[:start] + form + text[end:]).encode("utf-8")


def translate_docfind(source: bytes) -> bytes:
    """Adapt DocFind's runtime external and unsupported array sizeof syntax."""
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("DocFind.ZC is not UTF-8") from exc
    old = "StrIMatch(needle, doc_e->tag)"
    new = "BootstrapStrIMatch(needle, doc_e->tag)"
    if text.count(old) != 1 or text.count(new):
        raise ValueError("expected the pinned DocFind StrIMatch call")
    text = text.replace(old, new, 1)
    for field in ("find_text", "replace_text"):
        old = f"sizeof(CEdFindText.{field})"
        if text.count(old) != 1:
            raise ValueError(f"expected the pinned DocFind {field} array size")
        text = text.replace(old, "STR_LEN", 1)
    return text.encode("utf-8")


def translate_ktask(source: bytes) -> bytes:
    """Replace KTask's x86 CallStackGrow with the tested AAPCS64 trampoline.

    The original function uses lex-ahead #exe options and two x86 stack
    assembly blocks. Its portable replacement allocates a temporary task-owned
    stack and forwards the complete variadic integer argument vector through
    ZcCallNOnStack. TaskInit retains its source queue/layout work while the
    fixed-area FPU image and heap-owned string/allocation edges use explicit
    ARM64 runtime hooks.
    """
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("KTask.ZC is not UTF-8") from exc
    if text.count(_KTASK_GROW_BEGIN) != 1 or text.count(_KTASK_GROW_END) != 1:
        raise ValueError("expected the pinned CallStackGrow #exe option pair")
    start = text.index(_KTASK_GROW_BEGIN)
    finish = text.index(_KTASK_GROW_END, start) + len(_KTASK_GROW_END)
    block = text[start:finish]
    signature = (
        "argpop I64 CallStackGrow(I64 stack_size_threshold, I64 stack_size, "
        "/*argpop*/I64 (*fp_addr)(...), ...)"
    )
    if block.count(signature) != 1 or block.count("asm {") != 2:
        raise ValueError("CallStackGrow signature or x86 assembly blocks changed")
    for anchor in (r"(?m)^\s*LEAVE\s*(?://.*)?$",
                   r"(?m)^\s*JMP\s+RBX\b",
                   r"(?m)^\s*IMPORT\s+_FREE;",
                   r"(?m)^\s*MOV\s+RSP,\s*U64 &rsp2\[RBP\]",
                   r"(?m)^\s*CALL\s+U64 &fp_addr\[RBP\]"):
        if not re.search(anchor, block):
            raise ValueError(f"CallStackGrow assembly anchor changed: {anchor!r}")
    replacement = """// ARM64: run the variadic target on a temporary task-owned stack.
argpop I64 CallStackGrow(I64 stack_size_threshold, I64 stack_size,
                         /*argpop*/I64 (*fp_addr)(...), ...)
{
    CTaskStack *tmp;
    I64 result;
    if (stack_size <= 0 || !fp_addr)
        return -1;
    tmp = TaskStackNew(stack_size, Fs);
    if (!tmp)
        return -1;
    result = ZcCallNOnStack(&tmp->stack_base,
                            (&tmp->stack_base)(U8 *) + tmp->stack_size,
                            fp_addr, argc, argv);
    Free(tmp);
    return result;
}"""
    text = text[:start] + replacement + text[finish:]

    # TaskInit still owns the task ABI and queue setup, but its x86 fixed-area
    # FPU image and implicit kernel heap helpers are not available in the
    # host-fiber runtime. Keep those two platform edges explicit so the real
    # source initializer can be exercised against a disposable task record.
    fpu_alloc = "task->fpu_mmx = MAllocAligned(sizeof(CFPU), 0x10, task);"
    fpu_alloc_arm = "task->fpu_mmx = MAlloc(sizeof(CFPU), task);"
    if text.count(fpu_alloc) != 1 or text.count(fpu_alloc_arm):
        raise ValueError("expected the pinned TaskInit aligned FPU allocation")
    text = text.replace(fpu_alloc, fpu_alloc_arm, 1)
    fpu_copy = (
        "\tMemCopy(task->fpu_mmx, SYS_FIXED_AREA + "
        "offset(CSysFixedArea.init_fpu_mmx), sizeof(CFPU));"
    )
    fpu_init = "\tZcTaskFpuInit(task->fpu_mmx);"
    if text.count(fpu_copy) != 1 or text.count(fpu_init):
        raise ValueError("expected the pinned TaskInit fixed-area FPU copy")
    text = text.replace(fpu_copy, fpu_init, 1)
    for old, new in (
        ("task->cur_dir\t= StrNew(blkdev.home_dir + 2, task);",
         "task->cur_dir\t= ZcTaskStrNew(blkdev.home_dir + 2, task);"),
        ("task->cur_dir\t= StrNew(\"/Home\", task);",
         "task->cur_dir\t= ZcTaskStrNew(\"/Home\", task);"),
        ("Seed(, task);",
         "task->rand_seed = TSCGet;\n\t\tLBtr(&task->task_flags, TASKf_NONTIMER_RAND);"),
    ):
        if text.count(old) != 1 or text.count(new):
            raise ValueError(f"expected one pinned TaskInit platform call: {old!r}")
        text = text.replace(old, new, 1)
    no_warn = "\tno_warn dummy;"
    if text.count(no_warn) != 2:
        raise ValueError("expected the pinned KTask dummy-argument suppressions")
    # This is a compiler warning directive expressed as a pseudo-variable in
    # upstream ZealC. The compatibility parser does not implement it; removing
    # it has no generated-code effect.
    text = text.replace(no_warn, "\t// ARM64 compiler: no_warn is metadata only.", 2)
    extstr_decl = (
        "extern I64 ZcCallExtStr(U8 *name);\n"
        "extern U0 ZcTaskFpuInit(U8 *fpu);\n"
        "extern U8 *ZcTaskStrNew(U8 *text, CTask *task);\n"
        "extern Bool ZcTaskValidate(CTask *task);\n"
        "extern Bool ZcTaskKill(CTask *task, Bool wait=TRUE, Bool just_break=FALSE);\n"
        "extern CTask *ZcSpawn(U0 (*fp_start_addr)(U8 *data), U8 *data=NULL, "
        "U8 *task_name=NULL, I64 target_cpu=-1, CTask *parent=NULL, "
        "I64 stack_size=0, I64 flags=1 << JOBf_ADD_TO_QUE);\n"
    )
    if text.count('CallExtStr("ServerStartUp");') != 1 or \
            text.count('CallExtStr("UserStartUp");') != 1:
        raise ValueError("expected the pinned KTask startup hooks")
    text = extstr_decl + text
    text = text.replace('CallExtStr("ServerStartUp");',
                        'ZcCallExtStr("ServerStartUp");', 1)
    text = text.replace('CallExtStr("UserStartUp");',
                        'ZcCallExtStr("UserStartUp");', 1)
    validate_signature = "Bool TaskValidate(CTask *task)"
    if text.count(validate_signature) != 1:
        raise ValueError("expected the pinned TaskValidate signature")
    validate_start = text.index(validate_signature)
    validate_end_anchor = "\nI64 BirthWait(CTask **_task, I64 task_num=-1)"
    if text.count(validate_end_anchor) != 1:
        raise ValueError("expected the pinned TaskValidate function boundary")
    validate_end = text.index(validate_end_anchor, validate_start)
    validate_body = text[validate_start:validate_end]
    if (validate_body.count("0 < task <= I32_MAX") != 1 or
            validate_body.count("task->task_signature != TASK_SIGNATURE_VAL") != 1):
        raise ValueError("pinned TaskValidate pointer/signature checks changed")
    text = text[:validate_start] + """Bool TaskValidate(CTask *task)
{
    return ZcTaskValidate(task);
}
""" + text[validate_end:]
    kill_signature = "Bool Kill(CTask *task, Bool wait=TRUE, Bool just_break=FALSE)"
    if text.count(kill_signature) != 1:
        raise ValueError("expected the pinned Kill signature")
    kill_start = text.index(kill_signature)
    kill_end_anchor = "\nBool Suspend(CTask *task=NULL, Bool state=TRUE)"
    if text.count(kill_end_anchor) != 1:
        raise ValueError("expected the pinned Kill function boundary")
    kill_end = text.index(kill_end_anchor, kill_start)
    kill_body = text[kill_start:kill_end]
    if (kill_body.count("LBts(&task->task_flags, TASKf_KILL_TASK);") != 1 or
            kill_body.count("while (TaskValidate(task) && Bt(") != 1):
        raise ValueError("pinned Kill flag/wait path changed")
    text = text[:kill_start] + """Bool Kill(CTask *task, Bool wait=TRUE, Bool just_break=FALSE)
{
    if (task == sys_winmgr_task)
        return FALSE;
    return ZcTaskKill(task, wait, just_break);
}
""" + text[kill_end:]
    spawn_signature = "CTask *Spawn(U0 (*fp_start_addr)(U8 *data),"
    if text.count(spawn_signature) != 1:
        raise ValueError("expected the pinned Spawn signature")
    spawn_start = text.index(spawn_signature)
    spawn_end_anchor = "\nU0 TaskDerivedValsUpdate(CTask *task=NULL, Bool update_z_buf=TRUE)"
    if text.count(spawn_end_anchor) != 1:
        raise ValueError("expected the pinned Spawn function boundary")
    spawn_end = text.index(spawn_end_anchor, spawn_start)
    spawn_body = text[spawn_start:spawn_end]
    if (spawn_body.count("TaskInit(task, stack_size);") != 1 or
            spawn_body.count("TaskQueueInsChild(task);") != 1 or
            spawn_body.count("*task->rsp = data;") != 1 or
            spawn_body.count("*task->rsp = &Exit;") != 1):
        raise ValueError("pinned Spawn initialization or queue path changed")
    replacement_spawn = """CTask *Spawn(U0 (*fp_start_addr)(U8 *data), U8 *data=NULL,
             U8 *task_name=NULL, I64 target_cpu=-1, CTask *parent=NULL,
             I64 stack_size=0, I64 flags=1 << JOBf_ADD_TO_QUE)
{
    return ZcSpawn(fp_start_addr, data, task_name, target_cpu, parent,
                  stack_size, flags);
}
"""
    text = text[:spawn_start] + replacement_spawn + text[spawn_end:]
    return text.encode("utf-8")


def translate_grscreen(source: bytes) -> bytes:
    """Apply the narrow AArch64 compatibility overlay to GrScreen.ZC.

    The source is pinned and remains byte-for-byte unchanged on disk. This
    guarded overlay is applied only to the AArch64 boot source bundle. It
    requires the exact text-background assembly and optional subsystem call
    sites so an upstream change cannot be silently mis-translated. The task-doc
    and ODE calls go through runtime bridges because these optional modules may
    not have been loaded when the source window manager first starts.
    """
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("GrScreen.ZC is not UTF-8") from exc
    matches = list(_TEXT_BG_X86.finditer(text))
    if len(matches) != 1:
        raise ValueError(
            "expected exactly one GrScreen text-background x86 store loop; "
            f"found {len(matches)}"
        )
    doc_call = "DocUpdateTaskDocs(task);"
    if text.count(doc_call) != 1:
        raise ValueError("expected exactly one GrUpdateTaskWin task-doc call")
    ode_call = "ODEsUpdate(task);"
    if text.count(ode_call) != 1:
        raise ValueError("expected exactly one GrUpdateTaskODEs integrator call")
    rop_call = "GrRopEquU8NoClipping("
    if text.count(rop_call) != 2:
        raise ValueError("expected exactly two GrScreen glyph rasterizer calls")
    indent = matches[0].group("indent")
    replacement = (
        f"{indent}cell_dst = dst(U8 *);\n"
        f"{indent}for (j = 0; j < FONT_HEIGHT; j++)\n"
        f"{indent}{{\n"
        f"{indent}\t*cell_dst(U64 *) = c;\n"
        f"{indent}\tcell_dst += w1;\n"
        f"{indent}}}\n"
        # The original source's following dst += w2 remains in place. Position
        # dst at scanline 7 here so that w2 returns it to the next cell on row 0.
        f"{indent}dst(U8 *) += 7 * w1;\n"
    )
    function = """U8 *ZcGrTextBGStore(U8 *dst, I64 stride, U64 color)
{
\tU8 *row_dst = dst;
\tI64 row;
\tfor (row = 0; row < 8; row++)
\t{
\t\t*row_dst(U64 *) = color;
\t\tif (row < 7)
\t\t\trow_dst += stride;
\t}
\treturn row_dst;
}

"""
    marker = "U0 GrUpdateTextBG()"
    if text.count(marker) != 1:
        raise ValueError("expected exactly one GrUpdateTextBG definition")
    replaced = (text[:matches[0].start()] + replacement +
                text[matches[0].end():])
    function_marker = "\tU8\t\t\t*dst2 = dst;"
    if replaced.count(function_marker) != 1:
        raise ValueError("expected exactly one GrUpdateTextBG destination setup")
    replaced = replaced.replace(
        function_marker,
        "\tU8\t\t\t*dst2 = dst, *cell_dst;",
        1,
    )
    replaced = replaced.replace(marker, function + marker, 1)
    replaced = replaced.replace(doc_call, "ZcDocUpdateTaskDocs(task);", 1)
    replaced = replaced.replace(ode_call, "ZcODEsUpdate(task);", 1)
    replaced = replaced.replace(
        rop_call, "ZcGrRopEquU8NoClipping(text.font,", 2,
    )
    return replaced.encode("utf-8")


def translate_grpalette(source: bytes) -> bytes:
    """Mirror upstream palette changes into the ARM64 framebuffer bridge."""
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("GrPalette.ZC is not UTF-8") from exc
    setter = (
        "public U0 GrPaletteSet(CBGR24 *bgr24)\n"
        "{//16 colors\n"
        "\tMemCopy(&gr_palette, bgr24, sizeof(CBGR24) * COLORS_NUM);\n"
        "}"
    )
    if text.count(setter) != 1:
        raise ValueError("expected exactly one pinned GrPaletteSet body")
    text = text.replace(
        setter,
        setter[:-1] + "\tZcFbGraphPaletteSet(bgr24);\n}",
        1,
    )
    return text.encode("utf-8")


def translate_message(source: bytes) -> bytes:
    """Route decoded Ctrl-Alt guest messages through registered key callbacks."""
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("Message.ZC is not UTF-8") from exc
    marker = "U0 InputFilterTask()"
    if text.count(marker) != 1 or "ZcCtrlAltDispatch" in text:
        raise ValueError("expected unchanged SerialDev/Message.ZC input filter")
    old = (
        "\t\t\t\tif (_arg1)\n"
        "\t\t\t\t\t*_arg1 = tmpc->aux1;\n"
        "\t\t\t\tif (_arg2)\n"
        "\t\t\t\t\t*_arg2 = tmpc->aux2;\n"
        "\t\t\t\tJobDel(tmpc);\n"
        "\t\t\t\tif ((res != MESSAGE_KEY_DOWN || !(tmpc->aux2 & SCF_KEY_DESC) || !Bt(&task->win_inhibit, WIf_SELF_KEY_DESC)) &&\n"
    )
    new = (
        "\t\t\t\tI64 arg1 = tmpc->aux1, arg2 = tmpc->aux2;\n"
        "\t\t\t\tif (_arg1)\n"
        "\t\t\t\t\t*_arg1 = arg1;\n"
        "\t\t\t\tif (_arg2)\n"
        "\t\t\t\t\t*_arg2 = arg2;\n"
        "\t\t\t\tJobDel(tmpc);\n"
        "\t\t\t\tif (!ZcCtrlAltDispatch(res, arg1, arg2, task) &&\n"
        "\t\t\t\t\t(res != MESSAGE_KEY_DOWN || !(arg2 & SCF_KEY_DESC) || !Bt(&task->win_inhibit, WIf_SELF_KEY_DESC)) &&\n"
    )
    if text.count(old) != 1:
        raise ValueError("expected unchanged SerialDev/Message.ZC message dequeue")
    return text.replace(old, new, 1).encode("utf-8")
