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
    rsp_data_store = "\t*task->rsp = data;"
    rsp_data_store_arm64 = "\tMemCopy(task->rsp(U8 *), &data, sizeof(U8 *));"
    rsp_exit_store = "\t*task->rsp = &Exit;"
    rsp_exit_store_arm64 = "\tMemCopy(task->rsp(U8 *), &Exit, sizeof(U8 *));"
    if text.count(rsp_data_store) != 1 or text.count(rsp_exit_store) != 1:
        raise ValueError("expected the pinned Spawn stack-word stores")
    text = text.replace(rsp_data_store, rsp_data_store_arm64, 1)
    text = text.replace(rsp_exit_store, rsp_exit_store_arm64, 1)
    no_warn = "\tno_warn dummy;"
    if text.count(no_warn) != 2:
        raise ValueError("expected the pinned KTask dummy-argument suppressions")
    # This is a compiler warning directive expressed as a pseudo-variable in
    # upstream ZealC. The compatibility parser does not implement it; removing
    # it has no generated-code effect.
    text = text.replace(no_warn, "\t// ARM64 compiler: no_warn is metadata only.", 2)
    extstr_decl = "extern I64 ZcCallExtStr(U8 *name);\n"
    if text.count('CallExtStr("ServerStartUp");') != 1 or \
            text.count('CallExtStr("UserStartUp");') != 1:
        raise ValueError("expected the pinned KTask startup hooks")
    text = extstr_decl + text
    text = text.replace('CallExtStr("ServerStartUp");',
                        'ZcCallExtStr("ServerStartUp");', 1)
    text = text.replace('CallExtStr("UserStartUp");',
                        'ZcCallExtStr("UserStartUp");', 1)
    spawn_signature = "CTask *Spawn(U0 (*fp_start_addr)(U8 *data),"
    if text.count(spawn_signature) != 1:
        raise ValueError("expected the pinned Spawn signature")
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
