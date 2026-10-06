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


def translate_grscreen(source: bytes) -> bytes:
    """Replace GrUpdateTextBG's x86 register loop with equivalent HolyC.

    The source is pinned and remains byte-for-byte unchanged on disk. This
    guarded overlay is applied only to the AArch64 boot source bundle. It
    intentionally requires the exact eight-store/seven-stride sequence so an
    upstream change cannot be silently mis-translated.
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
    indent = matches[0].group("indent")
    replacement = (
        f"{indent}*dst(U64 *) = c;\n"
        f"{indent}dst(U8 *) += w1;\n"
        f"{indent}*dst(U64 *) = c;\n"
        f"{indent}dst(U8 *) += w1;\n"
        f"{indent}*dst(U64 *) = c;\n"
        f"{indent}dst(U8 *) += w1;\n"
        f"{indent}*dst(U64 *) = c;\n"
        f"{indent}dst(U8 *) += w1;\n"
        f"{indent}*dst(U64 *) = c;\n"
        f"{indent}dst(U8 *) += w1;\n"
        f"{indent}*dst(U64 *) = c;\n"
        f"{indent}dst(U8 *) += w1;\n"
        f"{indent}*dst(U64 *) = c;\n"
        f"{indent}dst(U8 *) += w1;\n"
        f"{indent}*dst(U64 *) = c;\n"
        f"{indent}dst(U8 *) += w2;\n"
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
    return replaced.replace(marker, function + marker, 1).encode("utf-8")
