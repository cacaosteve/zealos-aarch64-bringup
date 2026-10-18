"""Narrow source overlays needed to compile pinned ZealOS on AArch64."""
from __future__ import annotations

import re


_TEXT_BG_X86 = re.compile(
    r"(?m)^(?P<indent>[ \t]*)MOV[ \t]+U64[ \t]+\[RSI\],[ \t]*R13[ \t]*\n"
    r"(?:[ \t]*ADD[ \t]+RSI,[ \t]*R12[ \t]*\n"
    r"[ \t]*MOV[ \t]+U64[ \t]+\[RSI\],[ \t]*R13[ \t]*\n){6}"
    r"[ \t]*ADD[ \t]+RSI,[ \t]*R12[ \t]*\n"
    r"[ \t]*MOV[ \t]+U64[ \t]+\[RSI\],[ \t]*R13[ \t]*(?:\n|$)"
)

_KTASK_GROW_BEGIN = "#exe {Option(OPTf_NO_REG_VAR, ON);};"
_KTASK_GROW_END = "#exe {Option(OPTf_NO_REG_VAR, OFF);};"


def translate_winmgr(source: bytes) -> bytes:
    """Route the source manager's periodic redraw through atomic scanout.

    The upstream manager otherwise calls GrUpdateScreen from WinMgrSleep,
    bypassing the host's staged framebuffer transaction. Require the exact
    single call site so source updates cannot silently change its semantics.
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
    return text.encode("utf-8")


def translate_grbitmap(source: bytes) -> bytes:
    """Extract regular CDC rectangles without negative-destination blitting.

    Pinned DCExt creates a result CDC sized to the requested source rectangle,
    then calls GrBlot at (-x1, -y1). GrBlot's negative-origin clipping is only
    valid for a larger/screen destination; for an ordinary tightly-sized CDC
    the path can address before the result buffer. Keep screen-bitmap handling
    on the upstream path and copy rows directly for regular CDCs.
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
                        "\tCTask *win_task;\n\tI64 row, width;", 1)
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
		for (row = 0; row < res->height; row++)
			MemCopy(res->body + row * res->width_internal,
					dc->body + (y1 + row) * dc->width_internal + x1,
					res->width);""",
        1,
    )
    return (text[:start] + body + text[end:]).encode("utf-8")


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
    return text.replace(bad, good, 1).encode("utf-8")


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
    ZcCallNOnStack. The surrounding pinned task source remains unchanged.
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
    no_warn = "\tno_warn dummy;"
    if text.count(no_warn) != 2:
        raise ValueError("expected the pinned KTask dummy-argument suppressions")
    # This is a compiler warning directive expressed as a pseudo-variable in
    # upstream ZealC. The compatibility parser does not implement it; removing
    # it has no generated-code effect.
    text = text.replace(no_warn, "\t// ARM64 compiler: no_warn is metadata only.", 2)
    extstr_decl = (
        "extern I64 ZcCallExtStr(U8 *name);\n"
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
    return replaced.encode("utf-8")
