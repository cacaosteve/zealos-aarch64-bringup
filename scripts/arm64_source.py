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


def translate_grscreen(source: bytes) -> bytes:
    """Apply the narrow AArch64 compatibility overlay to GrScreen.ZC.

    The source is pinned and remains byte-for-byte unchanged on disk. This
    guarded overlay is applied only to the AArch64 boot source bundle. It
    requires the exact text-background assembly and optional subsystem call
    sites so an upstream change cannot be silently mis-translated. The task-doc
    and ODE calls go through runtime bridges because DolDoc and MathODE may not
    have been loaded yet.
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
