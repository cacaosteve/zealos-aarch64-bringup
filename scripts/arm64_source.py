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
