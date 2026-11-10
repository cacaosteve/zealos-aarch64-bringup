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


def translate_docputs_exception_cleanup(source: bytes) -> bytes:
    """Release partial-output scratch memory and its lock on exceptions."""
    start = b'public U0 DocPrintPartial('
    end = b'\nBool KDDocPutS('
    if source.count(start) != 1 or source.count(end) != 1:
        raise ValueError('DocPrintPartial function boundaries changed')
    first = source.index(start)
    last = source.index(end, first)
    body = source[first:last]
    callback_and_lock = (
        b'\tif (doc->user_put_s && (*doc->user_put_s)(doc, doc->user_put_data, buf))\n'
        b'\t{\n\t\tFree(buf);\n\t\treturn;\n\t}\n'
        b'\tunlock = DocLock(doc);\n')
    finish = (b'\tif (unlock)\n\t\tDocUnlock(doc);\n'
              b'\tif (!(doc->flags & DOCF_DONT_SWAP_OUT))\n\t\tYield;\n'
              b'\tFree(buf);\n}')
    if body.count(callback_and_lock) != 1 or body.count(finish) != 1:
        raise ValueError('DocPrintPartial lock or cleanup sequence changed')
    body = body.replace(callback_and_lock,
        b'\tunlock = FALSE;\n\ttry\n\t{\n'
        b'\tif (doc->user_put_s && (*doc->user_put_s)(doc, doc->user_put_data, buf))\n'
        b'\t{\n\t\tFree(buf);\n\t\tbuf = NULL;\n\t}\n'
        b'\telse\n\t{\n\tunlock = DocLock(doc);\n', 1)
    body = body.replace(finish,
        b'\tif (unlock)\n\t\tDocUnlock(doc);\n'
        b'\tunlock = FALSE;\n'
        b'\tif (!(doc->flags & DOCF_DONT_SWAP_OUT))\n\t\tYield;\n'
        b'\t}\n\t}\n\tcatch\n\t{\n'
        b'\t\tI64 except_ch = Fs->except_ch;\n'
        b'\t\tif (unlock)\n\t\t\tDocUnlock(doc);\n'
        b'\t\tFree(buf);\n\t\tthrow(except_ch);\n\t}\n'
        b'\tif (buf) Free(buf);\n}', 1)
    return source[:first] + body + source[last:]


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
    if b'I64 sprite_elem_base_sizes[SPT_TYPES_NUM] =' in bridge:
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


def translate_compile_demo(source: bytes) -> bytes:
    """Run each CompileDemo expression through the native AArch64 compiler.

    The pinned Compiler/LexExpression2Bin ABI uses the x86 compiler's
    CCompCtrl layout. The ARM64 runtime instead evaluates one source
    expression through its native Aiwnios parser/JIT, preserving the demo's
    supplied expression stream, side effects, per-expression result output,
    and final value.
    """
    old = b'''F64 CompileDemo(U8 *st)
{
	I64			 type;
	U8			*machine_code;
	CCompCtrl	*cc = CompCtrlNew(st, CCF_DONT_FREE_BUF);
	F64			 res = 0;

	Lex(cc);	//Gotta get it started
	"Compile \\"%s\\"\\n", st;
	do
	{
		if (machine_code = LexExpression2Bin(cc, &type))
		{
			if (type != RT_F64)
				res = ToF64(Call(machine_code));
			else
				res = Call(machine_code)(F64);
			Free(machine_code);
		}
		"res=%9.4f\\n", res;
		if (cc->token == ';')
			Lex(cc);
	}
	while (cc->token != TK_EOF);	//end of file?

	CompCtrlDel(cc);

	return res;
}'''
    new = b'''F64 CompileDemo(U8 *st)
{
	U8 *copy = StrNew(st), *p = copy, *start;
	F64 res = 0;
	Bool more;

	"Compile \\"%s\\"\\n", st;
	do
	{
		start = p;
		while (*p && *p != ';')
			p++;
		more = *p == ';';
		if (more)
			*p = 0;
		while (*start == ' ' || *start == '\\t' || *start == '\\r' || *start == '\\n')
			start++;
		if (*start)
		{
			res = ZcEvalExpression(start);
			"res=%9.4f\\n", res;
		}
		if (more)
			p++;
	}
	while (more);
	Free(copy);
	return res;
}'''
    if source.count(old) != 1:
        raise ValueError('pinned CompileDemo function changed')
    return source.replace(old, new, 1)


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
        throw('SizNull');
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
                throw('SizTerm');
            return total + sprite_elem_base_sizes[SPT_END];
        }
        if (type >= SPT_TYPES_NUM)
            throw('SizType');
        size = SpriteElemSizeRaw(record_bytes, type);
        if (size < 1 || size > 16777216 - total ||
                (available && total + size + sprite_elem_base_sizes[SPT_END] > available))
            throw('SizRecBd');
        total += size;
        record_bytes += size;
        record = record_bytes - offset(CSprite.start);
    }
    throw('SizLimit');
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
        elem_size0 = SpriteElemSizeRaw(p0, type0);
        elem_size1 = SpriteElemSizeRaw(p1, type1);
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
	{
		// Some stock animations change sprite record layouts between frames.
		// Interpolate compatible streams; for incompatible frames, render the
		// nearer complete frame instead of failing the entire window draw.
		if (t < 0.5)
		{
			i = size0;
			res = MAlloc(i);
			MemCopy(res, elems0, i);
		}
		else
		{
			i = size1;
			res = MAlloc(i);
			MemCopy(res, elems1, i);
		}
		return res;
	}
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
        (old_step0, b'bytes0 += SpriteElemSizeRaw(bytes0, tmpg0->type & SPG_TYPE_MASK); tmpg0 = bytes0 - offset(CSprite.start);'),
        (old_step1, b'bytes1 += SpriteElemSizeRaw(bytes1, tmpg1->type & SPG_TYPE_MASK); tmpg1 = bytes1 - offset(CSprite.start);'),
        (old_step_result, b'bytesr += SpriteElemSizeRaw(bytesr, tmpgr->type & SPG_TYPE_MASK); tmpgr = bytesr - offset(CSprite.start);'),
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


def translate_mandel_single_cpu(source: bytes) -> bytes:
    """Keep the stock Mandel renderer usable when the guest has one CPU.

    Upstream reserves CPU 0 for its UI and dispatches scanlines to CPUs
    1..mp_count-1. A single-CPU ARM64 guest therefore has no workers and
    divides by zero. Preserve the stock multi-CPU path, but run the same frame
    worker synchronously on CPU 0 when no worker CPUs exist.
    """
    if source.count(b'I64 numCores = mp_count - 1;') != 1:
        raise ValueError('pinned Mandel worker count changed')
    if source.count(b'I64 rowsPerCore = height / numCores;') != 1:
        raise ValueError('pinned Mandel row partition changed')
    dispatch = b'''      for (i=0; i<numCores; i++) {
        JobQueue(&MandelFrame, i, i+1);
      }'''
    if source.count(dispatch) != 1:
        raise ValueError('pinned Mandel frame dispatch changed')
    source = source.replace(
        b'I64 numCores = mp_count - 1;',
        # ZealC does not support C's conditional operator. mp_count is always
        # at least one, so add one when it is the single-CPU case.
        b'I64 numCores = mp_count - 1 + (mp_count <= 1);')
    source = source.replace(
        dispatch,
        b'''      if (mp_count > 1) {
        for (i=0; i<numCores; i++) {
          JobQueue(&MandelFrame, i, i+1);
        }
      } else {
        MandelFrame(0);
      }''')
    return source


def translate_elephant_walk_default_dc(source: bytes) -> bytes:
    """Pass the active CDC explicitly through the stock symmetry helper."""
    old = b'DCSymmetrySet(, x, y, x, y - 1);'
    new = b'DCSymmetrySet(gr.dc, x, y, x, y - 1);'
    if source.count(old) != 1:
        raise ValueError('pinned ElephantWalk symmetry call changed')
    return source.replace(old, new, 1)


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
    return b'''\n#define SPRITE3_MATRIX_LIMIT (1024 * SPRITE_INTERPOLATION_SCALE)

U0 BootstrapSpriteDCTransform(CDC *dc, I64 *x, I64 *y, I64 *z)
{//Match the default CDC transform when a static framebuffer CDC has no callback.
    I64 x0 = *x, y0 = *y, z0 = *z;
    *x = ((dc->r[0] * x0 + dc->r[1] * y0 + dc->r[2] * z0 + dc->r[3]) >> 32) + dc->x;
    *y = ((dc->r[4] * x0 + dc->r[5] * y0 + dc->r[6] * z0 + dc->r[7]) >> 32) + dc->y;
    *z = ((dc->r[8] * x0 + dc->r[9] * y0 + dc->r[10] * z0 + dc->r[11]) >> 32) + dc->z;
}

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
    I64 composed[16], local[16], identity[16], *old_matrix, old_flags, old_x, old_y, old_z;
    U0 (*old_transform)(CDC *, I64 *, I64 *, I64 *);
    I64 i, sprite_size;

    if (!dc)
        throw('MatDC0');
    if (dc->dc_signature != DCS_SIGNATURE_VAL)
        throw('MatDCSig');
    if (!elems)
        throw('MatElem');
    if (!matrix)
        throw('MatNull');
    if (x < -1073741824 || x > 1073741824)
    {
        Print("Sprite3Mat4x4B coords=%d,%d,%d\\n", x, y, z);
        throw('MatX');
    }
    if (y < -1073741824 || y > 1073741824)
        throw('MatY');
    if (z < -1073741824 || z > 1073741824)
        throw('MatZ');
    if (!Sprite3MatrixBounded(matrix))
        throw('MatBound');
    old_matrix = dc->r;
    old_transform = dc->transform;
    if (old_matrix && !Sprite3MatrixBounded(old_matrix))
        throw('MatPar');
    sprite_size = SpriteSize(elems);
    if (sprite_size < 1)
        throw('MatSize');

    old_flags = dc->flags;
    old_x = dc->x;
    old_y = dc->y;
    old_z = dc->z;
    MemCopy(local, matrix, sizeof(local));
    local[3] += x << 32;
    local[7] += y << 32;
    local[11] += z << 32;
    local[15] = SPRITE_INTERPOLATION_SCALE;
    if (old_matrix)
        BootstrapMat4x4Mul(composed, old_matrix, local);
    else
    {
        Mat4x4IdentEqu(identity);
        BootstrapMat4x4Mul(composed, identity, local);
    }
    dc->r = composed;
    if (!old_transform)
        dc->transform = &BootstrapSpriteDCTransform;
    dc->flags |= DCF_TRANSFORMATION;
    try
        Sprite3(dc, 0, 0, 0, elems);
    catch
    {
        dc->r = old_matrix;
        dc->transform = old_transform;
        dc->flags = old_flags;
        dc->x = old_x;
        dc->y = old_y;
        dc->z = old_z;
        throw;
    }
    dc->r = old_matrix;
    dc->transform = old_transform;
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


def inject_sprite_wrappers(bridge: bytes, sprite_plot: bytes) -> bytes:
    """Expose upstream Sprite3 translation and extent wrappers at boot."""
    if (b'public U0 Sprite3B(' in bridge or
            b'public U0 SpriteExtents(' in bridge or
            b'public U0 DCExtentsInit(' in bridge or
            b'public CDC *Sprite2DC(' in bridge or
            b'public U8 *SpriteTransform(' in bridge):
        raise ValueError('ARM64 sprite bridge already declares source wrappers')
    sprite3b = _extract_between(
        sprite_plot, b'public U0 Sprite3B(',
        b'\npublic U0 Sprite3Mat4x4B(', 'Sprite3B')
    extents = _extract_between(
        sprite_plot, b'public U0 SpriteExtents(',
        b'\npublic CDC *Sprite2DC(', 'SpriteExtents')
    sprite_to_dc = _extract_between(
        sprite_plot, b'public CDC *Sprite2DC(',
        b'\npublic U8 *SpriteInterpolate(', 'Sprite2DC')
    if (b'Sprite3(dc, 0, 0, 0, elems);' not in sprite3b or
            b'DCExtentsInit(dc);' not in extents or
            b'Sprite3(dc, I32_MAX / 2' not in extents):
        raise ValueError('pinned sprite wrapper implementation changed')
    packed_extents = extents.replace(b'SpriteExtents(', b'SpriteExtentsPacked(', 1)
    packed_extents = packed_extents.replace(b'Sprite3(dc,', b'Sprite3Packed(dc,', 1)
    # GrDC isn't part of the boot source bundle. Keep this upstream helper's
    # small CDC state initialization available to SpriteExtents.
    dc_extents_init = b'''public U0 DCExtentsInit(CDC *dc=NULL)
{
    if (!dc)
        dc = gr.dc;
    dc->flags |= DCF_RECORD_EXTENTS;
    dc->min_x = I64_MAX;
    dc->max_x = I64_MIN;
    dc->min_y = I64_MAX;
    dc->max_y = I64_MIN;
}
'''
    sprite_transform = b'''public U8 *SpriteTransform(U8 *elems, I64 *matrix)
{//Bounded mesh sprite transform for the stock Talons meshes.
    U8 *res, *cursor;
    I64 size, type, i, x, y, z;
    CSprite *record;
    CD3I32 *point;

    if (!elems || !matrix || !Sprite3MatrixBounded(matrix))
        throw('Graphics');
    size = SpriteSize(elems);
    if (size < sizeof(CSpriteBase) || size > 16777216)
        throw('Graphics');
    res = MAlloc(size);
    if (!res)
        throw('Graphics');
    MemCopy(res, elems, size);
    cursor = res;
    while (TRUE)
    {
        record = cursor - offset(CSprite.start);
        type = record->type & SPG_TYPE_MASK;
        if (!type)
            break;
        if (type == SPT_MESH || type == SPT_SHIFTABLE_MESH)
        {
            if (type == SPT_MESH)
            {
                point = &record->mu.u;
                i = record->mu.vertex_count;
            }
            else
            {
                point = &record->pmu.u;
                i = record->pmu.vertex_count;
                x = record->pmu.x;
                y = record->pmu.y;
                z = record->pmu.z;
                Mat4x4MulXYZ(matrix, &x, &y, &z);
                if (x < I32_MIN || x > I32_MAX || y < I32_MIN ||
                        y > I32_MAX || z < I32_MIN || z > I32_MAX)
                    throw('Graphics');
                record->pmu.x = x;
                record->pmu.y = y;
                record->pmu.z = z;
            }
            if (i < 0 || i > 1048576)
                throw('Graphics');
            while (i--)
            {
                x = point->x;
                y = point->y;
                z = point->z;
                Mat4x4MulXYZ(matrix, &x, &y, &z);
                if (x < I32_MIN || x > I32_MAX || y < I32_MIN ||
                        y > I32_MAX || z < I32_MIN || z > I32_MAX)
                    throw('Graphics');
                point->x = x;
                point->y = y;
                point->z = z;
                point++;
            }
        }
        else if (type != SPT_COLOR && type != SPT_DITHER_COLOR &&
                type != SPT_THICK)
            throw('Graphics');
        i = SpriteElemSizeRaw(cursor, type);
        if (i < 1 || cursor - res > size - i)
            throw('Graphics');
        cursor += i;
    }
    return res;
}
'''
    return (bridge + b'\n\n' + dc_extents_init + b'\n' + sprite3b +
            b'\n\n' + extents + b'\n\n' + packed_extents + b'\n\n' +
            sprite_to_dc + b'\n\n' +
            sprite_transform + b'\n')


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
                          'ZcCallExtStr("UserStartUp");\n'
                          '\tZcCallExtStr("BootstrapGuiTerminalAttachUser");', 1)
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
    version = re.search(r'[0-9.]+', match[1]).group(0)
    version_sub = re.search(r'[0-9]+', match[2]).group(0)
    version_text = f'ZealOS ARM64 V{version}.{version_sub}'
    return ('// Version values from pinned Kernel/KGlobals.ZC.\nF64 ' +
            match[1] + '\nU64 ' + match[2] +
            '\n// ARM64 has no boot-time KMain formatter; keep its public version strings available.\n'
            f'U8 *sys_os_version_str = "{version_text}";\n'
            f'U8 *sys_os_version_full = "{version_text}\\n";\n'
            f'U8 *sys_os_version_nice = "{version_text}";\n').encode()


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


def translate_span_worker_cleanup(source: bytes) -> bytes:
    # Debug instrumentation for worker teardown in the compatibility probe.
    """Join Span's app-owned workers before its stack-backed animation state ends."""
    replacements = (
        (b'F64 SpanTime()\n',
         b'I64 span_worker_cleanup_diag;\n'
         b'F64 SpanTime()\n'),
        (b'\tCCtrl\t\t*bt_run, *bt_mode;\n',
         b'\tCCtrl\t\t*bt_run, *bt_mode;\n'
         b'\tCTask\t\t*span_song_task = NULL, *span_animate_task = NULL;\n'),
        (b'\tFs->song_task = Spawn(&SongTask, NULL, "Song",, Fs);',
         b'\tspan_song_task = Fs->song_task = Spawn(&SongTask, NULL, "Song",, Fs);'),
        (b'\tFs->animate_task = Spawn(&AnimateTask, &a, "Animate",, Fs);',
         b'\tspan_animate_task = Fs->animate_task = Spawn(&AnimateTask, &a, "Animate",, Fs);'),
        (b'\tDocClear;\n\tSettingsPop;\n',
         b'\t// Stop app workers before their stack data and graphics state go away.\n'
         b'\tspan_worker_cleanup_diag = 1;\n'
         b'\tif (Fs->animate_task == span_animate_task)\n'
         b'\t\tspan_worker_cleanup_diag |= 1 << 1;\n'
         b'\tif (Fs->song_task == span_song_task)\n'
         b'\t\tspan_worker_cleanup_diag |= 1 << 2;\n'
         b'\tif (TaskValidate(span_animate_task))\n'
         b'\t\tspan_worker_cleanup_diag |= 1 << 3;\n'
         b'\tif (TaskValidate(span_song_task))\n'
         b'\t\tspan_worker_cleanup_diag |= 1 << 4;\n'
         b'\tif (TaskValidate(span_animate_task))\n'
         b'\t{\n'
         b'\t\tspan_worker_cleanup_diag |= Kill(span_animate_task, TRUE) << 5;\n'
         b'\t}\n'
         b'\tif (TaskValidate(span_song_task))\n'
         b'\t{\n'
         b'\t\tspan_worker_cleanup_diag |= Kill(span_song_task, TRUE) << 6;\n'
         b'\t}\n'
         b'\tif (TaskValidate(span_animate_task))\n'
         b'\t\tspan_worker_cleanup_diag |= 1 << 7;\n'
         b'\tif (TaskValidate(span_song_task))\n'
         b'\t\tspan_worker_cleanup_diag |= 1 << 8;\n'
         b'\tif (Fs->animate_task == span_animate_task)\n'
         b'\t\tFs->animate_task = NULL;\n'
         b'\tif (Fs->song_task == span_song_task)\n'
         b'\t\tFs->song_task = NULL;\n'
         b'\tDocClear;\n\tSettingsPop;\n'),
    )
    text = source
    for old, new in replacements:
        if text.count(old) != 1:
            raise ValueError('pinned Span worker lifecycle changed')
        text = text.replace(old, new, 1)
    return text


def translate_keepaway_nearest_ball(source: bytes) -> bytes:
    """Guard KeepAway's nearest-player read when the owner is first in its team."""
    old = (b'if (o != last_owner && !nearest_ball[o->team] || '
           b'o->get_ball_dt < nearest_ball[o->team]->get_ball_dt)')
    new = (b'if (o != last_owner && (!nearest_ball[o->team] || '
           b'o->get_ball_dt < nearest_ball[o->team]->get_ball_dt))')
    if source.count(old) != 1:
        raise ValueError('pinned KeepAway nearest-player selection changed')
    return source.replace(old, new, 1)


def translate_xcaliber_shot_return(source: bytes) -> bytes:
    """Return the newly queued shot to X-Caliber callers that dereference it."""
    old = (b'\tQueueInsert(&tmps->p, ode->last_mass);\n'
           b'\tQueueInsert(tmps, shot_head.last);\n}')
    new = (b'\tQueueInsert(&tmps->p, ode->last_mass);\n'
           b'\tQueueInsert(tmps, shot_head.last);\n'
           b'\treturn tmps;\n}')
    if source.count(old) != 1:
        raise ValueError('pinned X-Caliber ShotNew queue insertion changed')
    return source.replace(old, new, 1)


def translate_xcaliber_draw_visibility(source: bytes) -> bytes:
    """Do not hide X-Caliber's whole scene behind a stale task ODE cursor."""
    old = (b'\tif (ode != task->last_ode)\n'
           b'\t\treturn;\n')
    new = (b'\tif (!ode)\n'
           b'\t\treturn;\n')
    if source.count(old) != 1:
        raise ValueError('pinned X-Caliber draw visibility guard changed')
    return source.replace(old, new, 1)


def translate_xcaliber_key_descriptors(source: bytes) -> bytes:
    """Keep X-Caliber scan-code input enabled after its one-time help popup."""
    old = (b'Fs->win_inhibit = WIG_TASK_DEFAULT - WIF_SELF_FOCUS - '
           b'WIF_SELF_BORDER - WIF_FOCUS_TASK_MENU - WIF_SELF_CTRLS;')
    anchor = b'\t\tInit;\n\t\tQueueInsert(ode, Fs->last_ode);'
    if source.count(old) != 1 or source.count(anchor) != 1:
        raise ValueError('pinned X-Caliber window input policy changed')
    # InitLevel's first OneTimePopUp returns control with the default input
    # inhibit restored on the task. Set the desired pinned policy after Init
    # has completed so the modal popup cannot overwrite it.
    return source.replace(anchor,
                          b'\t\tInit;\n\t\tFs->win_inhibit = 0x0050E2FA;\n'
                          b'\t\tQueueInsert(ode, Fs->last_ode);', 1)


def translate_xcaliber_fire_trace(source: bytes) -> bytes:
    """Trace the Space firing gate and shot queue around the pinned fire call."""
    old = (b'\t\t\t\t\t\t\tcase CH_SPACE:\n'
           b'\t\t\t\t\t\t\t\tif (level < 4)\n'
           b'\t\t\t\t\t\t\t\t\tHumanFireGunBegin;\n'
           b'\t\t\t\t\t\t\t\telse\n'
           b'\t\t\t\t\t\t\t\t\tHumanFireLaserBegin;\n'
           b'\t\t\t\t\t\t\t\tbreak;')
    new = (b'\t\t\t\t\t\t\tcase CH_SPACE:\n'
           b'\t\t\t\t\t\t\t\tZcXCaliberFireTrace(0, level, QueueCount(&shot_head), 0, 0);\n'
           b'\t\t\t\t\t\t\t\tif (level < 4)\n'
           b'\t\t\t\t\t\t\t\t\tHumanFireGunBegin;\n'
           b'\t\t\t\t\t\t\t\telse\n'
           b'\t\t\t\t\t\t\t\t\tHumanFireLaserBegin;\n'
           b'\t\t\t\t\t\t\t\tif (QueueCount(&shot_head))\n'
           b'\t\t\t\t\t\t\t\t\tZcXCaliberFireTrace(1, level, QueueCount(&shot_head), '\
           b'ToI64(shot_head.last->p.x), ToI64(shot_head.last->p.y));\n'
           b'\t\t\t\t\t\t\t\telse\n'
           b'\t\t\t\t\t\t\t\t\tZcXCaliberFireTrace(1, level, QueueCount(&shot_head), 0, 0);\n'
           b'\t\t\t\t\t\t\t\tbreak;')
    if source.count(old) != 1:
        raise ValueError('pinned X-Caliber Space firing case changed')
    return source.replace(old, new, 1)


def translate_xcaliber_escape_scan(source: bytes) -> bytes:
    """Treat the raw Escape scan code as the exit key in X-Caliber's loop."""
    anchor = b'\t\t\t\t\tcase SC_CURSOR_RIGHT:'
    replacement = (b'\t\t\t\t\tcase SC_ESC:\n'
                   b'\t\t\t\t\t\tch = CH_ESC;\n'
                   b'\t\t\t\t\t\tbreak;\n\n' + anchor)
    if source.count(anchor) != 2:
        raise ValueError('pinned X-Caliber key-down/up scan-code switches changed')
    return source.replace(anchor, replacement)


def translate_xcaliber_held_input(source: bytes) -> bytes:
    """Poll held controls each frame when no key transition is queued."""
    old = (b'\t\t\twhile (!game_over && !show_level_message &&\n'
           b'\t\t\t\t\t(message_code = MessageScan(&arg1, &arg2, 1 << MESSAGE_KEY_DOWN | 1 << MESSAGE_KEY_UP)))')
    if source.count(old) != 1:
        raise ValueError('pinned X-Caliber input loop changed')
    anchor = (b'\t\t\t}\n\t\t\tAI;\n'
              b'\t\t\tSolarFlares;\n'
              b'\t\t\tExpireShots;\n'
              b'\t\t\tExpireSplats;\n'
              b'\t\t\tExpireMissiles;\n'
              b'\t\t\tCheckDamage;\n'
              b'\t\t\tRefresh; //messages are only qued by winmgr')
    replacement = (b'\t\t\t}\n'
                   b'\t\t\tZcXCaliberFrameTrace(0, Fs);\n'
                   b'\t\t\tif (!Bt(kbd.down_bitmap, SC_CTRL) &&\n'
                   b'\t\t\t\t!Bt(kbd.down_bitmap, SC_SHIFT))\n'
                   b'\t\t\t{\n'
                   b'\t\t\t\tif (Bt(kbd.down_bitmap, SC_CURSOR_LEFT))\n'
                   b'\t\t\t\t\thuman_t_left = THRUST_MAX;\n'
                   b'\t\t\t\telse\n'
                   b'\t\t\t\t\thuman_t_left = 0;\n'
                   b'\t\t\t\tif (Bt(kbd.down_bitmap, SC_CURSOR_RIGHT))\n'
                   b'\t\t\t\t\thuman_t_right = THRUST_MAX;\n'
                   b'\t\t\t\telse\n'
                   b'\t\t\t\t\thuman_t_right = 0;\n'
                   b'\t\t\t}\n'
                   b'\t\t\tif (human && human->p[0].state)\n'
                   b'\t\t\t\tZcXCaliberInputTrace(Fs, human_t_left, human_t_right, '
                   b'Bt(kbd.down_bitmap, SC_CURSOR_LEFT), '
                   b'Bt(kbd.down_bitmap, SC_CURSOR_RIGHT), '
                   b'human->s[3].flags, human->s[5].flags, human->exploding, '
                   b'human->p[0].state->x, human->p[0].state->y);\n'
                   b'\t\t\tif (Bt(kbd.down_bitmap, SC_ESC))\n'
                   b'\t\t\t\tch = CH_ESC;\n'
                   b'\t\t\tAI;\n'
                   b'\t\t\tZcXCaliberFrameTrace(1, Fs);\n'
                   b'\t\t\tSolarFlares;\n'
                   b'\t\t\tZcXCaliberFrameTrace(2, Fs);\n'
                   b'\t\t\tExpireShots;\n'
                   b'\t\t\tZcXCaliberFrameTrace(3, Fs);\n'
                   b'\t\t\tExpireSplats;\n'
                   b'\t\t\tZcXCaliberFrameTrace(4, Fs);\n'
                   b'\t\t\tExpireMissiles;\n'
                   b'\t\t\tZcXCaliberFrameTrace(5, Fs);\n'
                   b'\t\t\tCheckDamage;\n'
                   b'\t\t\tZcXCaliberFrameTrace(6, Fs);\n'
                   b'\t\t\tRefresh; //messages are only qued by winmgr\n'
                   b'\t\t\tZcXCaliberFrameTrace(7, Fs);')
    if source.count(anchor) != 1:
        raise ValueError('pinned X-Caliber frame update point changed')
    traced = source
    for code in (b'MESSAGE_KEY_DOWN', b'MESSAGE_KEY_UP'):
        old_event = (b'case ' + code + b':\n'
                     b'\t\t\t\t\t\tch = arg1;\n'
                     b'\t\t\t\t\t\tsc = arg2;')
        new_event = (b'case ' + code + b':\n'
                     b'\t\t\t\t\t\tch = arg1;\n'
                     b'\t\t\t\t\t\tsc = arg2;\n'
                     b'\t\t\t\t\t\tZcXCaliberMessageTrace(message_code, ch, sc, Fs);')
        if traced.count(old_event) != 1:
            raise ValueError('pinned X-Caliber key event cases changed')
        traced = traced.replace(old_event, new_event, 1)
    return traced.replace(anchor, replacement, 1)


def translate_tothefront_sentinel(source: bytes) -> bytes:
    """Compare the ToTheFront queue cursor to its sentinel object's address."""
    old = b'while (tmpi != *indirect_head)'
    if source.count(old) != 1:
        raise ValueError('pinned ToTheFront indirect-fire queue check changed')
    return source.replace(old, b'while (tmpi != &indirect_head)', 1)


def translate_tothefront_draw(source: bytes) -> bytes:
    """Keep ToTheFront tank rendering usable without the failing 3D sprite path."""
    replacements = (
        (b'Saw(4 * tS, 2.0)', b'ToTheFrontSaw(4 * tS, 2.0)'),
        (b'Saw(4 * tS + 1.0, 2.0)', b'ToTheFrontSaw(4 * tS + 1.0, 2.0)'),
        (b'Blink(5)', b'ToTheFrontBlink(5)'),
        (b'Blink(10)', b'ToTheFrontBlink(10)'),
    )
    text = source
    for old, new in replacements:
        if text.count(old) != 1:
            raise ValueError('pinned ToTheFront Saw call changed')
        text = text.replace(old, new, 1)
    unit_branch = (
        (b'\tif (tmpu->infantry)\n'
         b'\t\tSprite3(dc, x, y, 0, tmpu->img);\n'
         b'\telse\n'
         b'\t\tSprite3ZB(dc, x, y, 0, tmpu->img, f);',
         b'\tif (tmpu->infantry)\n'
         b'\t\tSprite3(dc, x, y, 0, tmpu->img);\n'
         b'\telse\n'
         b'\t{\n'
         b'\t\tI64 old_color = dc->color;\n'
         b'\t\tGrRect(dc, x - 5, y - 3, 10, 6);\n'
         b'\t\tdc->color = BLACK;\n'
         b'\t\tGrLine(dc, x, y, x + 9 * Cos(f), y + 9 * Sin(f));\n'
         b'\t\tdc->color = old_color;\n'
         b'\t}'),
    )
    for old, new in unit_branch:
        if text.count(old) != 1:
            raise ValueError('pinned ToTheFront unit sprite path changed')
        text = text.replace(old, new, 1)
    helper = (
        b'F64 ToTheFrontSaw(F64 t, F64 period)\n'
        b'{\n'
        b'    if (!period)\n'
        b'        return 0.0;\n'
        b'    if (t >= 0.0)\n'
        b'        return t % period / period;\n'
        b'    return 1.0 + t % period / period;\n'
        b'}\n\n'
        b'Bool ToTheFrontBlink(F64 hz)\n'
        b'{\n'
        b'    return (tS * hz) % 2.0 < 1.0;\n'
        b'}\n\n'
    )
    return helper + text


def translate_tothefront_init(source: bytes) -> bytes:
    """Compute byte counts for app arrays whose bounds are runtime globals."""
    replacements = (
        (b'MemSet(terrain, PLAINS, sizeof(terrain));',
         b'MemSet(terrain, PLAINS, map_rows * map_cols * sizeof(U8));'),
        (b'MemSet(roads, FALSE, sizeof(roads));',
         b'MemSet(roads, FALSE, map_rows * map_cols * sizeof(Bool));'),
        (b'MemSet(rivers, FALSE, sizeof(rivers));',
         b'MemSet(rivers, FALSE, map_rows * map_cols * sizeof(Bool));'),
        (b'MemSet(vis_map, FALSE, sizeof(vis_map));',
         b'MemSet(vis_map, FALSE, map_rows * map_cols * sizeof(Bool));'),
    )
    text = source
    for old, new in replacements:
        if text.count(old) != 1:
            raise ValueError('pinned ToTheFront runtime-sized map initialization changed')
        text = text.replace(old, new, 1)
    return text


def translate_tothefront_player_pick(source: bytes) -> bytes:
    """Use positive menu indices for ToTheFront's dynamically loaded AIs.

    The stock picker stores a CDirEntry pointer in a signed DolDoc link
    expression. AArch64 heap pointers are negative as I64 values, colliding
    with the popup's negative cancel result. Preserve the AI directory and
    callback contract while using small, nonnegative selection indices.
    """
    calls = (b'PlayerPick("AIs", 0);', b'PlayerPick("AIs", 1);')
    if any(source.count(call) != 1 for call in calls):
        raise ValueError('pinned ToTheFront AI picker calls changed')
    anchor = b'U0 ToTheFront()'
    if source.count(anchor) != 1:
        raise ValueError('pinned ToTheFront entry point changed')
    helper = b'''I64 ttf_ai_load_slot;

U0 ToTheFrontPlayerPick(U8 *ai_dir, I64 player)
{
\tCDirEntry *ais, *entry;
\tCDoc *doc;
\tU8 *mask = MStrPrint("%s/*.ZC", ai_dir);
\tI64 i, selected;

\tais = FilesFind(mask, FUF_JUST_FILES);
\tFree(mask);
\tif (!ais)
\t\treturn;
\tdoc = DocNew;
\tDocPrint(doc, "Player %d Type\\n\\n", player + 1);
\tfor (entry = ais, i = 0; entry; entry = entry->next, i++)
\t\tDocPrint(doc, "$$MU,\\\"%s\\\",LE=%d$$\\n", entry->name, i);
\tDocPrint(doc, "\\n$$MU,\\\"CANCEL\\\",LE=DOCM_CANCEL$$\\n");
\tselected = PopUpMenu(doc);
\tDocDel(doc);
\tif (selected >= 0)
\t{
\t\tfor (entry = ais, i = 0; entry && i < selected; entry = entry->next, i++);
\t\tif (entry)
\t\t{
\t\t\tttf_ai_load_slot = player;
\t\t\tExeFile(entry->full_name);
\t\t\tPrint("ToTheFront AI slot %d=%X,%X,%X\\n", player,
\t\t\t\tplayer_indirect[player], player_move[player], player_direct[player]);
\t\t}
\t}
\tDirTreeDel(ais);
}

'''
    text = source
    for call in calls:
        text = text.replace(call, call.replace(b'PlayerPick',
                                               b'ToTheFrontPlayerPick'), 1)
    text = text.replace(anchor, helper + anchor, 1)
    return text


def translate_tothefront_ai(source: bytes) -> bytes:
    """Register an AI's callback addresses before its temporary hash table ends."""
    callbacks = (b'PlayerIndirect', b'PlayerMove', b'PlayerDirect')
    for callback in callbacks:
        if source.count(b'U0 ' + callback + b'()') != 1:
            raise ValueError('pinned ToTheFront AI callback changed: ' +
                             callback.decode('ascii'))
    # The stock Human indirect-fire phase counted artillery with a loop whose
    # condition could never be true. That silently skipped every artillery
    # order phase. Correct the source typo in the ARM64 bundle overlay.
    indirect_loop = b'for (i = 0; i > UNITS_NUM; i++)'
    if indirect_loop in source:
        if source.count(indirect_loop) != 1 or b'U0 PlayerIndirect()' not in source:
            raise ValueError('pinned ToTheFront indirect-fire loop changed')
        source = source.replace(indirect_loop,
                                b'for (i = 0; i < UNITS_NUM; i++)', 1)
        # Visibility is tracked separately for each player. The original
        # Human direct-fire check always reads player zero's flag, so player
        # two cannot fire at targets only visible to that player.
        visibility = b'!Bt(&target->vis, 0)'
        if source.count(visibility) != 1:
            raise ValueError('pinned ToTheFront direct-fire visibility check changed')
        source = source.replace(visibility,
                                b'!Bt(&target->vis[cur_player], 0)', 1)
    registration = (
        b'\nplayer_indirect[ttf_ai_load_slot] = &PlayerIndirect;\n'
        b'player_move[ttf_ai_load_slot] = &PlayerMove;\n'
        b'player_direct[ttf_ai_load_slot] = &PlayerDirect;\n')
    if any(line in source for line in registration.splitlines() if line):
        raise ValueError('ToTheFront AI already has an ARM64 registration overlay')
    return source + registration


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


def translate_home_localize(source: bytes) -> bytes:
    """Drop the legacy PS/2 typematic write from the ARM64 home settings."""
    old = b'KbdTypeMatic(0);'
    if source.count(old) != 1:
        raise ValueError('pinned HomeLocalize typematic call changed')
    return source.replace(
        old,
        b'// USB keyboards provide repeat; there is no PS/2 typematic controller.\n'
        b'// KbdTypeMatic(0);', 1)


def extract_string_utilities(str_a: bytes, str_b: bytes) -> bytes:
    """Keep pinned string transforms and character bitmaps used by source apps."""
    begin, end = b'I64 Spaces2Tabs(U8 *dst, U8 *src)\n', b'\nU8 *StrFirstOcc('
    if str_a.count(begin) != 1 or str_a.count(end) != 1:
        raise ValueError('pinned string utility boundaries changed')
    inplace = begin + str_a.split(begin, 1)[1].split(end, 1)[0]
    bitmaps = []
    for name in (b'char_bmp_alpha', b'char_bmp_alpha_numeric',
                 b'char_bmp_alpha_numeric_no_at', b'char_bmp_word',
                 b'char_bmp_filename', b'char_bmp_dec_numeric',
                 b'char_bmp_hex_numeric'):
        pattern = rb'(?m)^\s*' + name + rb'\[16\]\s*=\s*\{[^}]*\}'
        found = re.findall(pattern, str_a)
        if len(found) != 1:
            raise ValueError(f'pinned {name.decode()} bitmap changed')
        bitmaps.append(found[0])
    end = b'\nU0 GetOutOfDollar()'
    if not str_b.startswith(b'U8 *Tabs2Spaces(') or str_b.count(end) != 1:
        raise ValueError('pinned allocating string utility boundaries changed')
    allocating = str_b.split(end, 1)[0]
    return (b'// Original StrA/StrB string transforms and whitespace bitmap.\n'
            b'U32 char_bmp_white_space[16] = {0x80002600, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};\n'
            + b'U32 ' + b';\nU32 '.join(bitmaps) + b';\n'
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


def translate_debug_compat(source: bytes, kernel_debug_source: bytes) -> bytes:
    """Apply narrow ARM64 ZealC compatibility fixes to stock Debug.ZC."""
    text = source.decode('utf-8')
    translated, count = re.subn(r'(?<![.>\w])offset\b', 'base_offset', text)
    if count != 8:
        raise ValueError(f'Debug.ZC reserved-word overlay matched {count}, expected 8')
    text = translated
    # The remaining stock debugger contains x86 disassembly/register helpers
    # that are not part of the portable ClassRep service needed by apps.
    helper_boundary = '\nU0 UpdateRegVarImg('
    if text.count(helper_boundary) != 1:
        raise ValueError('Debug.ZC portable ClassRep boundary changed')
    portable_text = text.split(helper_boundary, 1)[0]
    # Keep the original renderer and its dynamic/static field formatting, but
    # obtain class data through the compiler's ABI rather than interpreting
    # Aiwnios CHashClass/CMemberLst as the different ZealOS structures.
    rep_end = portable_text.index('\npublic U0 ClassRep(')
    rep = portable_text[:rep_end]
    rep_replacements = {
        '\tCMemberList *ml;': '\tCArmClassRepField arm_field;\n\tI64 arm_index = 0, arm_size;',
        '\tCHashClass\t*tmpc, *tmpc2;\n': '',
        'if (!(tmpc = HashFind(class_name, Fs->hash_table, types)))':
            'if ((arm_size = ZcClassRepSize(class_name, types)) < 0)',
        '_d(U8 *) + tmpc->size': '_d(U8 *) + arm_size',
        '\tif (tmpc->base_class && !ClassRep2(doc, _d, tmpc->base_class->str, depth, max_depth, dynamic, types, base_offset))\n\t\treturn FALSE;\n': '',
        '\tml = tmpc->member_list_and_root;\n\twhile (ml)':
            '\twhile (ZcClassRepField(class_name, types, arm_index, &arm_field))',
        '\t\ttmpc2 = ml->member_class;\n': '',
        'ml->offset': 'arm_field.offset',
        'stars = tmpc2->ptr_stars_count;': 'stars = arm_field.pointer;',
        '\t\ttmpc2 = OptClassFwd(tmpc2);\n\t\ttmpc2 -= tmpc2->ptr_stars_count;\n': '',
        'tmpc2->type & HTT_INTERNAL_TYPE': 'arm_field.is_internal',
        'tmpc2->raw_type': 'arm_field.raw_type',
        'ml->dim.total_count': 'arm_field.elements',
        'ml->str': 'arm_field.name',
        'tmpc2->str': 'arm_field.type_name',
        '\t\tml = ml->next;\n': '',
    }
    for old, new in rep_replacements.items():
        if old not in rep:
            raise ValueError('pinned ClassRep reflection changed: '+old)
        rep = rep.replace(old, new)
    rep = rep.replace('\t\tptr = _d(U8 *) + arm_field.offset;',
        '\t\tarm_index++;\n\t\tptr = _d(U8 *) + arm_field.offset;', 1)
    portable_debug = (rep + portable_text[rep_end:]).rstrip().encode('utf-8') + b'\n'

    # The portable debugger overlay is loaded at graphical startup. Pull the
    # architecture-neutral Dump routine and stack bounds helper from KDebug,
    # then provide ARM64 stack/frame-pointer reports. The full KDebug module
    # contains x86 register and disassembly helpers that cannot be loaded here.
    kernel_text = kernel_debug_source.decode('utf-8')
    dump_start = 'U0 Dump(U8 *addr, I64 count=0x80, Bool show_offset=TRUE)'
    dump_end = '\nU0 DumpMem('
    if kernel_text.count(dump_start) != 1 or kernel_text.count(dump_end) != 1:
        raise ValueError('KDebug.ZC portable Dump extraction boundary changed')
    dump_body = kernel_text[kernel_text.index(dump_start):kernel_text.index(dump_end)]
    stack_reports = r'''
U0 StackRep(CTask *task=NULL)
{//AArch64 stack report using the task's saved stack pointer.
    if (!task)
        task = Fs;
    if (!TaskValidate(task) || !task->stack)
        return;
    if (!task->rsp ||
            task->rsp(U8 *) < (&task->stack->stack_base)(U8 *) ||
            task->rsp(U8 *) >= (&task->stack->stack_base)(U8 *) + task->stack->stack_size)
        return;
    if (!IsRaw)
        "$$LTCYAN$$";
    "AArch64 StackRep, saved SP=%016X\n", task->rsp;
    if (!IsRaw)
        "$$FG$$";
    Dump(task->rsp(U8 *), 0x100, FALSE);
}

U0 CallerRep(U8 **rbp=NULL, CTask *task=NULL)
{//AArch64 caller report using the task's saved frame pointer.
    if (!task)
        task = Fs;
    if (!TaskValidate(task) || !task->stack)
        return;
    if (!rbp)
    {
        if (task == Fs)
            rbp = RBPGet;
        else
            rbp = task->rbp;
    }
    if (!rbp || rbp(U8 *) < (&task->stack->stack_base)(U8 *) ||
            rbp(U8 *) + 16 > (&task->stack->stack_base)(U8 *) + task->stack->stack_size)
        return;
    "AArch64 CallerRep, saved FP=%016X SP=%016X\n", rbp, task->rsp;
    if (!IsRaw)
        "$$LTCYAN$$Frame record and return address:\n$$FG$$";
    Dump(rbp(U8 *), 0x40, FALSE);
}
'''
    return (portable_debug + dump_body.rstrip().encode('utf-8') + b'\n' +
            stack_reports.encode('utf-8'))


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
    timing = ("\tTimeCal;\n"
              "\tif ((t_delta = (t = tS) - winmgr.last_refresh_tS) > 0.01)\n"
              "\t\twinmgr.fps = Max(1.0 / t_delta, 1);\n"
              "\telse\n"
              "\t\twinmgr.fps = 99;\n"
              "\twinmgr.last_refresh_tS = t;\n")
    if text.count(timing) != 1:
        raise ValueError(
            "expected exactly one WinMgrSleep frame timing update; "
            f"found {text.count(timing)}"
        )
    text = text.replace(timing, "\tBootstrapWinMgrTimingUpdate;\n", 1)
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


def translate_lighttable_memory_prompt(source: bytes) -> bytes:
    """Remove LightTable's BIOS-only memory report from its ARM64 path.

    LightTable allocates its refract/reflect bitmaps from the system heap, so
    the x86 BIOS memory map printed by MemBIOSRep is unrelated to the storage
    it uses. Keep the demo's own heap-size prompt and correct its description.
    """
    text = source.decode("utf-8")
    call = "\tMemBIOSRep;\n"
    old_prompt = (
        '"You can choose up to the largest\\n"\n'
        '\t\t\t\t\t"contiguous chunk of physical memory.\\n\\n"\n'
    )
    new_prompt = (
        '"Choose the total bitmap memory in bytes;\\n"\n'
        '\t\t\t\t\t"it is allocated from the system heap.\\n\\n"\n'
    )
    if text.count(call) != 1 or text.count(old_prompt) != 1:
        raise ValueError("pinned LightTable BIOS memory prompt changed")
    return text.replace(call, "\t\"ARM64 bitmap memory is allocated from the system heap.\\n\";\n", 1).replace(
        old_prompt, new_prompt, 1).encode("utf-8")


def translate_win_focus_trace(source: bytes) -> bytes:
    """Trace upstream window-manager focus assignments during input routing."""
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("Win.ZC is not UTF-8") from exc
    plain = "\tsys_focus_task = task;\n"
    if text.count(plain) != 2:
        raise ValueError("expected the two guarded Win.ZC focus assignments")
    text = text.replace(
        plain,
        '\tZcFocusTrace("WinRefocus", sys_focus_task, task);\n' + plain,
        1,
    )
    text = text.replace(
        plain,
        '\tZcFocusTrace("WinToTop", sys_focus_task, task);\n' + plain,
        1,
    )
    inline = "\tWinToTop(sys_focus_task = task);"
    if text.count(inline) != 1:
        raise ValueError("expected the WinFocus assignment and raise call")
    text = text.replace(
        inline,
        '\tZcFocusTrace("WinFocus", sys_focus_task, task);\n'
        '\tWinToTop(sys_focus_task = task);',
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
    """Adapt define lookup and release depth buffers before recursive trimming.

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
    local = '\tCHashDefineStr\t*tmph;'
    lookup = ('\t\t\t\t\tif (tmph = HashFind(doc_e->define_str, win_task->hash_table, HTT_DEFINE_STR))\n'
              '\t\t\t\t\t\tdoc_e->tag = StrNew(tmph->data, mem_task);')
    if text.count(local) != 1 or text.count(lookup) != 1:
        raise ValueError('expected the pinned DocRecalc define lookup')
    text = text.replace(local, local + '\n\tU8 *compiler_define;', 1)
    text = text.replace(lookup,
        ('\t\t\t\t\tif (tmph = ZcGuestHashFindLocal(doc_e->define_str, win_task->hash_table, HTT_DEFINE_STR, 1))\n'
         '\t\t\t\t\t\tdoc_e->tag = StrNew(tmph->data, mem_task);\n'
         '\t\t\t\t\telse if (compiler_define = ZcTaskLexDefineText(doc_e->define_str, win_task))\n'
         '\t\t\t\t\t\tdoc_e->tag = StrNew(compiler_define, mem_task);'), 1)
    text = ('extern U8 *ZcTaskLexDefineText(U8 *name, CTask *task);\n'
            'extern CHash *ZcGuestHashFindLocal(U8 *name, CHashTable *table, I64 mask, I64 instance=1);\n' + text)
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
    """Adapt the pinned ODE source to the ARM64 cooperative runtime.

    The bit-test expression workaround keeps the pinned source compiling.
    The monotonic deadline and RK-step cap keep ODE work bounded when a stiff
    solve needs extremely many tiny steps before the guest's cooperative clock
    can advance. The guarded replacements keep the upstream mirror pristine.
    """
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("MathODE.ZC is not UTF-8") from exc
    signature = "U0 ODEMPTask(CMathODE *ode)\n{"
    call = "\t\twhile (!Bt(&ode->mp_not_done_flags, Gs->num))"
    reset = "\t\tLBtr(&ode->mp_not_done_flags, Gs->num);"
    update_signature = "U0 ODEsUpdate(CTask *task)\n{"
    update_declarations = "\tI64\t\t\t i;\n\tF64\t\t\t d, start_time, timeout_time, t_desired, t_initial, interpolation;\n"
    timeout_calculation = "\t\ttimeout_time = start_time + (task->last_ode_time / d + 0.1) / (winmgr.last_ode_time / d + 0.1) * ode_alloced_factor * d;\n"
    integration_loop = "\t\t\t\t\t\twhile (ode->t < t_desired)\n\t\t\t\t\t\t{\n"
    if (text.count(signature) != 1 or text.count(call) != 1 or
            text.count(reset) != 1 or text.count(update_signature) != 1 or
            text.count(update_declarations) != 1 or
            text.count(timeout_calculation) != 1 or
            text.count(integration_loop) != 1):
        raise ValueError("expected the pinned ODE task/update boundaries")
    if "ZcXCaliberFrameTrace" in text:
        raise ValueError("MathODE source unexpectedly contains X-Caliber tracing")
    text = text.replace(signature, signature + "\n\tU8 *mp_flags = &ode->mp_not_done_flags;", 1)
    text = text.replace(call, "\t\twhile (!Bt(mp_flags, Gs->num))", 1)
    text = text.replace(reset, "\t\tLBtr(mp_flags, Gs->num);", 1)
    text = text.replace(update_declarations,
        update_declarations + "\tI64\t\t\t monotonic_timeout_ns, ode_steps;\n", 1)
    text = text.replace(timeout_calculation,
        timeout_calculation +
        # `timeout_time` is in the guest's uptime epoch, while
        # ZcMonotonicNs() is in the host process epoch. Convert only the
        # remaining guest interval before building a host monotonic deadline.
        "\t\tmonotonic_timeout_ns = ZcMonotonicNs() + (timeout_time - tS) * 1000000000.0;\n", 1)
    text = text.replace(integration_loop, integration_loop +
        "\t\t\t\t\t\t\tZcXCaliberOdeTrace(task, timeout_time, t_desired, ode->t, winmgr.fps, d);\n" +
        "\t\t\t\t\t\t\tif (ZcMonotonicNs() > monotonic_timeout_ns)\n"
        "\t\t\t\t\t\t\t{\n"
        "\t\t\t\t\t\t\t\tode->base_t += t_desired - ode->t; //Slip\n"
        "\t\t\t\t\t\t\t\tgoto ode_done;\n"
        "\t\t\t\t\t\t\t}\n", 1)
    step = "\t\t\t\t\t\t\tODERK5OneStep(ode);"
    if text.count(step) != 1:
        raise ValueError("expected one MathODE integration step site")
    step_counter_reset = (
        "\t\t\t\t\t\tODEState2Internal(ode);\n"
        "\t\t\t\t\t\tMemCopy(ode->initial_state, ode->state_internal, ode->n_internal * sizeof(F64));\n"
        "\t\t\t\t\t\twhile (ode->t < t_desired)")
    if text.count(step_counter_reset) != 1:
        raise ValueError("expected one MathODE integration-loop setup")
    text = text.replace(step_counter_reset,
        "\t\t\t\t\t\tODEState2Internal(ode);\n"
        "\t\t\t\t\t\tMemCopy(ode->initial_state, ode->state_internal, ode->n_internal * sizeof(F64));\n"
        "\t\t\t\t\t\tode_steps = 0;\n"
        "\t\t\t\t\t\twhile (ode->t < t_desired)", 1)
    text = text.replace(step, step + "\n"
        "\t\t\t\t\t\t\tif (++ode_steps >= 16 && ode->t < t_desired)\n"
        "\t\t\t\t\t\t\t{\n"
        "\t\t\t\t\t\t\t\tode->base_t += t_desired - ode->t; //Slip\n"
        "\t\t\t\t\t\t\t\tgoto ode_done;\n"
        "\t\t\t\t\t\t\t}", 1)
    derivative = "\t\t\t\t\t\t\tODECallDerivative(ode, ode->t, ode->state_internal, ode->DstateDt);"
    step = "\t\t\t\t\t\t\tODERK5OneStep(ode);"
    if text.count(derivative) != 1 or text.count(step) != 1:
        raise ValueError("expected one MathODE derivative/step site")
    text = text.replace(derivative,
        "\t\t\t\t\t\t\tZcXCaliberFrameTrace(22, task);\n" +
        derivative + "\n\t\t\t\t\t\t\tZcXCaliberFrameTrace(23, task);", 1)
    text = text.replace(step,
        "\t\t\t\t\t\t\tZcXCaliberFrameTrace(24, task);\n" +
        step + "\n\t\t\t\t\t\t\tZcXCaliberFrameTrace(25, task);", 1)
    text = ("extern U0 ZcXCaliberFrameTrace(I64 stage, CTask *task);\n"
            "extern U0 ZcXCaliberInputTrace(CTask *task, F64 t_left, F64 t_right, I64 key_left, I64 key_right, I64 left_flags, I64 right_flags, I64 exploding, F64 ship_x, F64 ship_y);\n"
            "extern U0 ZcXCaliberOdeTrace(CTask *task, F64 timeout_time, F64 t_desired, F64 ode_time, F64 fps, F64 frame_d);\n"
            "extern U0 ZcXCaliberFireTrace(I64 phase, I64 level, I64 shot_count, I64 x, I64 y);\n\n" + text)
    return text.encode("utf-8")


def translate_math(source: bytes) -> bytes:
    """Use ZealOS Arg in place of the unavailable IC_ATAN backend intrinsic.

    The source Math.ZC formulas call unary ATan. Arg(1, y) has the same
    principal-value result for finite y and is already implemented by the
    ARM64 runtime. Add the shared sawtooth helper used by stock demos; the
    pinned Math.ZC source tree does not include it. Keep the pinned source
    unchanged and guard all adapted call sites.
    """
    old = b'return ATan(s / c);'
    if source.count(old) != 2:
        raise ValueError('expected the two pinned ASin/ACos ATan call sites')
    if b'public F64 Saw(' in source:
        raise ValueError('pinned Math.ZC unexpectedly defines Saw')
    translated = source.replace(old, b'return Arg(1.0, s / c);')
    return translated + (
        b'\n\npublic F64 Saw(F64 t, F64 period=1.0)\n'
        b'{\n'
        b'\tif (!period)\n'
        b'\t\treturn 0.0;\n'
        b'\tt %= period;\n'
        b'\tif (t < 0.0)\n'
        b'\t\tt += period;\n'
        b'\treturn t / period;\n'
        b'}\n'
        b'\npublic F64 Arm64Tri(F64 t, F64 period)\n'
        b'{\n'
        b'\tif (!period)\n'
        b'\t\treturn 0.0;\n'
        b'\tt = 2.0 * (Abs(t) % period) / period;\n'
        b'\tif (t <= 1.0)\n'
        b'\t\treturn t;\n'
        b'\treturn 2.0 - t;\n'
        b'}\n'
        b'\npublic F64 Arm64FullTri(F64 t, F64 period)\n'
        b'{\n'
        b'\tif (!period)\n'
        b'\t\treturn 0.0;\n'
        b'\tt = 4.0 * (t % period) / period;\n'
        b'\tif (t <= -1.0)\n'
        b'\t{\n'
        b'\t\tif (t <= -3.0)\n'
        b'\t\t\treturn t + 4.0;\n'
        b'\t\treturn -2.0 - t;\n'
        b'\t}\n'
        b'\tif (t <= 1.0)\n'
        b'\t\treturn t;\n'
        b'\tif (t <= 3.0)\n'
        b'\t\treturn 2.0 - t;\n'
        b'\treturn t - 4.0;\n'
        b'}\n'
        b'\npublic F64 Arm64Log2(F64 value)\n'
        b'{\n'
        b'\tF64 fraction = 0.0, bit = 0.5;\n'
        b'\tI64 exponent = 0, i;\n'
        b'\n'
        b'\tif (!(value > 0.0))\n'
        b'\t\treturn -1e300;\n'
        b'\twhile (value >= 2.0 && exponent < 1023)\n'
        b'\t{\n'
        b'\t\tvalue /= 2.0;\n'
        b'\t\texponent++;\n'
        b'\t}\n'
        b'\twhile (value < 1.0 && exponent > -1074)\n'
        b'\t{\n'
        b'\t\tvalue *= 2.0;\n'
        b'\t\texponent--;\n'
        b'\t}\n'
        b'\tfor (i = 0; i < 53; i++)\n'
        b'\t{\n'
        b'\t\tvalue *= value;\n'
        b'\t\tif (value >= 2.0)\n'
        b'\t\t{\n'
        b'\t\t\tvalue /= 2.0;\n'
        b'\t\t\tfraction += bit;\n'
        b'\t\t}\n'
        b'\t\tbit *= 0.5;\n'
        b'\t}\n'
        b'\treturn exponent + fraction;\n'
        b'}\n'
        b'\npublic F64 Arm64Sign(F64 value)\n'
        b'{\n'
        b'\tif (value > 0.0)\n'
        b'\t\treturn 1.0;\n'
        b'\tif (value < 0.0)\n'
        b'\t\treturn -1.0;\n'
        b'\treturn 0.0;\n'
        b'}\n'
        b'\npublic Bool Arm64Blink(F64 hz=2.5)\n'
        b'{\n'
        b'\treturn hz > 0.0 && (tS * hz) % 2.0 < 1.0;\n'
        b'}\n'
        b'\npublic U64 Arm64ClampU64(U64 num, U64 lo, U64 hi)\n'
        b'{\n'
        b'\tif (num < lo)\n'
        b'\t\treturn lo;\n'
        b'\tif (num > hi)\n'
        b'\t\treturn hi;\n'
        b'\treturn num;\n'
        b'}\n')


def translate_conversion_math(source: bytes) -> bytes:
    """Route the four legacy x86/SSE conversion imports through the ARM64 ABI bridge."""
    text = source.decode('utf-8')
    if text.count('asm\n{') != 4:
        raise ValueError('pinned Conversion.ZC no longer has four x86 assembly blocks')
    labels = ('_F64_TO_F32', '_F32_TO_F64', '_F32_RAD_TO_DEG',
              '_F32_DEG_TO_RAD')
    for label in labels:
        if text.count(label) != 2:
            raise ValueError('pinned Conversion.ZC import label changed: ' + label)
    text, count = re.subn(r'asm\n\{.*?\n\}\n', '', text, flags=re.S)
    if count != 4:
        raise ValueError('failed to remove all pinned x86 Conversion.ZC blocks')
    for label in labels:
        text = text.replace('_extern ' + label + ' ', 'extern ', 1)
    if '_extern ' in text or 'asm\n{' in text:
        raise ValueError('legacy conversion assembly/import annotation remains')
    return ('// ARM64 replaces the pinned x86/SSE bodies with runtime ABI imports.\n' +
            text).encode('utf-8')


def translate_f32_math(source: bytes) -> bytes:
    """Replace the pinned x86/SSE CosF32 body with its ARM64 runtime import."""
    text = source.decode('utf-8')
    if (text.count('asm\n{') != 1 or
            text.count('_extern _F32_COS F32 CosF32(F32 theta);') != 1 or
            text.count('_F32_COS::') != 1):
        raise ValueError('pinned F32 source shape changed')
    start = text.index('asm\n{')
    end = text.index('\n}\n', start) + 2
    text = text[:start] + '// ARM64 runtime import replaces the pinned x86/SSE cosine body.\n' + text[end:]
    text = text.replace('_extern _F32_COS F32 CosF32(F32 theta);',
                        'extern F32 CosF32(F32 theta);', 1)
    if '_extern ' in text or 'asm\n{' in text:
        raise ValueError('legacy F32 assembly/import annotation remains')
    return text.encode('utf-8')


def translate_vec4_math(source: bytes) -> bytes:
    """Replace pinned x86/SSE vector code with guarded ARM64 ABI imports."""
    text = source.decode('utf-8')
    symbols = (
        '_VECTOR_4D_INIT', '_VECTOR_4D_COPY', '_VECTOR_4D_IS_EQUAL',
        '_VECTOR_4D_ADD', '_VECTOR_4D_ADDS', '_VECTOR_4D_SUB',
        '_VECTOR_4D_SUBS', '_VECTOR_4D_MUL', '_VECTOR_4D_MULS',
        '_VECTOR_4D_DIV', '_VECTOR_4D_DIVS', '_VECTOR_4D_MIN',
        '_VECTOR_4D_MAX', '_VECTOR_4D_NEGATE', '_VECTOR_4D_DOT',
    )
    names = (
        'Vector4DInit', 'Vector4DCopy', 'Vector4DIsEqual', 'Vector4DAdd',
        'Vector4DAddS', 'Vector4DSub', 'Vector4DSubS', 'Vector4DMul',
        'Vector4DMulS', 'Vector4DDiv', 'Vector4DDivS', 'Vector4DMin',
        'Vector4DMax', 'Vector4DNegate', 'Vector4DDot',
    )
    if (text.count('asm\n{') != 16 or
            any(text.count(symbol + '::') != 1 for symbol in symbols) or
            any(text.count('_extern ' + symbol + ' ') != 1 for symbol in symbols) or
            text.count('U0 Vector4DPrint(CVector4D *v)') != 1):
        raise ValueError('pinned Vec4 source shape changed')
    print_start = text.index('U0 Vector4DPrint(CVector4D *v)')
    print_end = text.index('\n\nasm\n{', print_start)
    text = (text[:print_start] +
            'U0 Vector4DPrint(CVector4D *v)\n{\n'
            '\tPrint("%n\\t%n\\t%n\\t%n\\n\\n",\n'
            '\t\tF32ToF64(v->x), F32ToF64(v->y),\n'
            '\t\tF32ToF64(v->z), F32ToF64(v->w));\n'
            '}\n' + text[print_end:])
    text, count = re.subn(r'(?ms)^asm\s*\{\n.*?^\}',
                          '/* ARM64 vector operation is bound by the runtime. */',
                          text)
    if count != 15:
        raise ValueError(f'expected 15 Vec4 assembly blocks, found {count}')
    for symbol in symbols:
        text = text.replace('_extern ' + symbol + ' ', 'extern ', 1)
    if '_extern ' in text or 'asm\n{' in text:
        raise ValueError('legacy Vec4 assembly/import annotation remains')
    for name in names:
        if text.count('extern ') == 0 or text.count(name + '(') != 1:
            raise ValueError(f'Vec4 runtime import {name} missing or duplicated')
    return text.encode('utf-8')


def translate_vec3_math(source: bytes) -> bytes:
    """Replace pinned x86/SSE Vec3 bodies with ARM64 runtime imports."""
    text = source.decode('utf-8')
    symbols = (
        '_VECTOR_3D_INIT', '_VECTOR_3D_IS_EQUAL', '_VECTOR_3D_NORMALIZE',
        '_VECTOR_3D_DOT', '_VECTOR_3D_CROSS',
    )
    names = (
        'Vector3DInit', 'Vector3DIsEqual', 'Vector3DNormalize',
        'Vector3DDot', 'Vector3DCross',
    )
    imported = (
        'Vector3DInit', 'Vector3DCopy', 'Vector3DIsEqual', 'Vector3DAdd',
        'Vector3DAddS', 'Vector3DSub', 'Vector3DSubS', 'Vector3DMul',
        'Vector3DMulS', 'Vector3DDiv', 'Vector3DDivS', 'Vector3DMin',
        'Vector3DMax', 'Vector3DNegate', 'Vector3DNormalize',
        'Vector3DDot', 'Vector3DCross',
    )
    if (text.count('asm\n{') != 6 or
            any(text.count(symbol + '::') != 1 for symbol in symbols) or
            any(text.count('_extern ' + symbol + ' ') != 1 for symbol in symbols) or
            text.count('U0 Vector3DPrint(CVector3D *v)') != 1):
        raise ValueError('pinned Vec3 source shape changed')
    print_start = text.index('U0 Vector3DPrint(CVector3D *v)')
    print_end = text.index('\n\n/**', print_start)
    text = (text[:print_start] +
            'U0 Vector3DPrint(CVector3D *v)\n{\n'
            '\tPrint("%n\\t%n\\t%n\\n\\n", F32ToF64(v->x),\n'
            '\t\tF32ToF64(v->y), F32ToF64(v->z));\n'
            '}\n' + text[print_end:])
    text, count = re.subn(r'(?ms)^asm\s*\{\n.*?^\}',
                          '/* ARM64 vector operation is bound by the runtime. */',
                          text)
    if count != 5:
        raise ValueError(f'expected 5 Vec3 assembly blocks, found {count}')
    for symbol in symbols:
        text = text.replace('_extern ' + symbol + ' ', 'extern ', 1)
    # The remaining operations share the same 128-bit behavior as Vec4.
    for symbol in ('_VECTOR_4D_COPY', '_VECTOR_4D_ADD', '_VECTOR_4D_ADDS',
                   '_VECTOR_4D_SUB', '_VECTOR_4D_SUBS', '_VECTOR_4D_MUL',
                   '_VECTOR_4D_MULS', '_VECTOR_4D_DIV', '_VECTOR_4D_DIVS',
                   '_VECTOR_4D_MIN', '_VECTOR_4D_MAX', '_VECTOR_4D_NEGATE'):
        text = text.replace('_extern ' + symbol + ' ', 'extern ', 1)
    if '_extern ' in text or 'asm\n{' in text:
        raise ValueError('legacy Vec3 assembly/import annotation remains')
    for name in imported:
        if text.count(name + '(') != 1:
            raise ValueError(f'Vec3 runtime import {name} missing or duplicated')
    return text.encode('utf-8')


def translate_mat4_math(source: bytes) -> bytes:
    """Replace pinned x86/SSE Mat4 bodies with native runtime imports."""
    text = source.decode('utf-8')
    symbols = ('_MATRIX_4D_MUL_VECTOR_4D', '_MATRIX_4D_MUL',
               '_MATRIX_4D_TRANSPOSE')
    names = ('Matrix4DMulVector4D', 'Matrix4DMul', 'Matrix4DTranspose')
    if (text.count('asm\n{') != 4 or
            any(text.count(symbol + '::') != 1 for symbol in symbols) or
            any(text.count('_extern ' + symbol + ' ') != 1 for symbol in symbols) or
            text.count('U0 Matrix4DPrint(CMatrix4D *m)') != 1 or
            text.count('U0 Matrix4DTranslationSet(F32 x, F32 y, F32 z, CMatrix4D *dest)') != 1 or
            text.count('U0 Matrix4DScaleSet(F32 x, F32 y, F32 z, CMatrix4D *dest)') != 1):
        raise ValueError('pinned Mat4 source shape changed')
    print_start = text.index('U0 Matrix4DPrint(CMatrix4D *m)')
    print_end = text.index('\n\n/**', print_start)
    text = (text[:print_start] +
            'U0 Matrix4DPrint(CMatrix4D *m)\n{\n'
            '\tPrint("%n\\t%n\\t%n\\t%n\\n",\n'
            '\t\tF32ToF64(m->e[0]), F32ToF64(m->e[4]),\n'
            '\t\tF32ToF64(m->e[8]), F32ToF64(m->e[12]));\n'
            '\tPrint("%n\\t%n\\t%n\\t%n\\n",\n'
            '\t\tF32ToF64(m->e[1]), F32ToF64(m->e[5]),\n'
            '\t\tF32ToF64(m->e[9]), F32ToF64(m->e[13]));\n'
            '\tPrint("%n\\t%n\\t%n\\t%n\\n",\n'
            '\t\tF32ToF64(m->e[2]), F32ToF64(m->e[6]),\n'
            '\t\tF32ToF64(m->e[10]), F32ToF64(m->e[14]));\n'
            '\tPrint("%n\\t%n\\t%n\\t%n\\n\\n",\n'
            '\t\tF32ToF64(m->e[3]), F32ToF64(m->e[7]),\n'
            '\t\tF32ToF64(m->e[11]), F32ToF64(m->e[15]));\n'
            '}\n' + text[print_end:])
    for function, signature in (
            ('Matrix4DTranslationSet',
             'extern U0 Matrix4DTranslationSet(F32 x, F32 y, F32 z, CMatrix4D *dest);'),
            ('Matrix4DScaleSet',
             'extern U0 Matrix4DScaleSet(F32 x, F32 y, F32 z, CMatrix4D *dest);')):
        start = text.index('U0 ' + function + '(')
        end = text.index('\n}', start) + 2
        text = text[:start] + signature + text[end:]
    text, count = re.subn(r'(?ms)^asm\s*\{\n.*?^\}',
                          '/* ARM64 matrix operation is bound by the runtime. */',
                          text)
    if count != 3:
        raise ValueError(f'expected 3 Mat4 assembly blocks, found {count}')
    for symbol in symbols:
        text = text.replace('_extern ' + symbol + ' ', 'extern ', 1)
    if '_extern ' in text or 'asm\n{' in text:
        raise ValueError('legacy Mat4 assembly/import annotation remains')
    for name in names:
        if text.count(name + '(') != 1:
            raise ValueError(f'Mat4 runtime import {name} missing or duplicated')
    return text.encode('utf-8')


def translate_stock_game_math_calls(source: bytes, *, blink_count: int = 0,
                                    clamp_u64_count: int = 0, tri_count: int = 0,
                                    full_tri_count: int = 0,
                                    log2_count: int = 0,
                                    sign_count: int = 0) -> bytes:
    """Route stock game calls through ARM64 source helpers, not kernel externs."""
    for name, count in ((b'Blink(', blink_count),
                        (b'ClampU64(', clamp_u64_count),
                        (b'Log2(', log2_count),
                        (b'Sign(', sign_count)):
        if source.count(name) != count:
            raise ValueError(f'pinned stock game {name.decode()} call count changed')
    direct_tri_count = source.count(b'Tri(') - source.count(b'FullTri(')
    if direct_tri_count != tri_count or source.count(b'FullTri(') != full_tri_count:
        raise ValueError('pinned stock game Tri/FullTri call counts changed')
    return (source.replace(b'Tri(', b'Arm64Tri(')
            .replace(b'FullArm64Tri(', b'Arm64FullTri(')
            .replace(b'Log2(', b'Arm64Log2(')
            .replace(b'Sign(', b'Arm64Sign(')
            .replace(b'Blink(', b'Arm64Blink(')
            .replace(b'ClampU64(', b'Arm64ClampU64('))


def translate_blackdiamond_math_dependency(source: bytes) -> bytes:
    """Load stock math and paint BlackDiamond's expected light canvas."""
    include = b'#include "/System/Math/Math"\n'
    draw_it = b'U0 DrawIt(CTask *, CDC *dc)\n{\n\tI64  i;'
    draw_with_canvas = (b'U0 DrawIt(CTask *, CDC *dc)\n{\n'
                        b'\tDCFill(dc, WHITE);\n\tI64  i;')
    if (source.count(b'Cosh(') != 1 or source.count(include) or
            source.count(draw_it) != 1):
        raise ValueError('pinned BlackDiamond math dependency shape changed')
    return include + source.replace(draw_it, draw_with_canvas, 1)


def translate_polezeros_math_dependency(source: bytes) -> bytes:
    """Load the shared complex-math definitions used by PoleZeros."""
    include = b'#include "/System/Math/Math"\n'
    if (source.count(b'CPoly(') != 2 or source.count(b'Complex zeros[ZEROS_NUM]') != 1 or
            include in source):
        raise ValueError('pinned PoleZeros math dependency shape changed')
    return include + source


def translate_wenceslas(source: bytes) -> bytes:
    """Apply AArch64 Wenceslas prompt and extended-key compatibility fixes."""
    starts = []
    offset = 0
    marker = b'PopUpOk("'
    while True:
        index = source.find(marker, offset)
        if index < 0:
            break
        starts.append(index)
        offset = index + len(marker)
    if len(starts) != 3:
        raise ValueError('expected the three pinned Wenceslas lyric prompts')

    chunks = []
    cursor = 0
    for start in starts:
        if start < cursor:
            continue
        finish = source.find(b'");', start + len(marker))
        if finish < 0:
            raise ValueError('pinned Wenceslas prompt terminator changed')
        payload = source[start + len(marker):finish]
        if len(payload) <= 143:
            continue

        lines = payload.splitlines(keepends=True)
        pages = []
        page = b''
        for line in lines:
            if len(line) > 143:
                raise ValueError('pinned Wenceslas lyric line exceeds STR_LEN')
            if page and len(page) + len(line) > 143:
                pages.append(page)
                page = b''
            page += line
        if page:
            pages.append(page)
        replacement = b'\n'.join(marker + page + b'");' for page in pages)
        chunks.append((start, finish + 3, replacement))
        cursor = finish + 3

    if len(chunks) != 2:
        raise ValueError('expected exactly two oversized Wenceslas prompts')
    for start, finish, replacement in reversed(chunks):
        source = source[:start] + replacement + source[finish:]

    # The pinned game shares allocator-backed linked queues between its
    # keyboard task and animation task. On AArch64, inserting a king trail
    # record from the keyboard task wedges the guest. Keep the same trail
    # data and lifetimes in bounded static rings instead; this avoids
    # cross-task allocator and linked-queue mutation while retaining the
    # footprints and peasant-follow behavior.
    queue_decl = b'} step_head, king_step_head;'
    ring_decl = (b'} step_head, king_step_head;\n'
                 b'\n'
                 b'#define ARM64_STEP_CAP 256\n'
                 b'#define ARM64_KING_STEP_CAP 128\n'
                 b'Step arm64_steps[ARM64_STEP_CAP], arm64_king_steps[ARM64_KING_STEP_CAP];\n'
                 b'U16 arm64_step_head, arm64_step_count, arm64_king_step_head, arm64_king_step_count;\n'
                 b'\n'
                 b'U0 StepRingPush(Step *ring, U16 cap, U16 *head, U16 *count, I64 x, I64 y)\n'
                 b'{\n'
                 b'\tU16 idx;\n'
                 b'\tif (*count < cap)\n'
                 b'\t{\n'
                 b'\t\tidx = (*head + *count) % cap;\n'
                 b'\t\t(*count)++;\n'
                 b'\t}\n'
                 b'\telse\n'
                 b'\t{\n'
                 b'\t\tidx = *head;\n'
                 b'\t\t*head = (*head + 1) % cap;\n'
                 b'\t}\n'
                 b'\tring[idx].x = x;\n'
                 b'\tring[idx].y = y;\n'
                 b'\tring[idx].t0 = tS;\n'
                 b'}\n')
    if source.count(queue_decl) != 1:
        raise ValueError('expected one Wenceslas trail queue declaration')
    source = source.replace(queue_decl, ring_decl, 1)

    # Draw only recent generic footprints, retaining their dark/light age
    # transition and four-pixel marker thickness.
    draw_start = b'\ttmpst = step_head.next;\n\twhile (tmpst != &step_head)\n\t{'
    draw_end = b'\n\tDCDepthBufAlloc(dc);'
    if source.count(draw_start) != 1 or source.count(draw_end) != 1:
        raise ValueError('Wenceslas footprint drawing boundary changed')
    start = source.index(draw_start)
    finish = source.index(draw_end, start)
    draw_ring = (b'\tfor (i = 0; i < arm64_step_count; i++)\n'
                 b'\t{\n'
                 b'\t\ttmpst = &arm64_steps[(arm64_step_head + i) % ARM64_STEP_CAP];\n'
                 b'\t\tif (tS - tmpst->t0 < 5.0)\n'
                 b'\t\t{\n'
                 b'\t\t\tif (tS - tmpst->t0 < 2.0)\n'
                 b'\t\t\t\tdc->color = DKGRAY;\n'
                 b'\t\t\telse\n'
                 b'\t\t\t\tdc->color = LTGRAY;\n'
                 b'\t\t\tdc->thick = 4;\n'
                 b'\t\t\tGrPlot3(dc, tmpst->x, tmpst->y, 0);\n'
                 b'\t\t}\n'
                 b'\t}')
    source = source[:start] + draw_ring + source[finish:]

    # Replace the entire allocator-backed insertion routine with ring writes.
    step_start = b'U0 StepNew(CTask *task, I64 x, I64 y, F64 '
    step_finish = b'\nU0 Init()'
    if source.count(step_start) != 1 or source.count(step_finish) != 1:
        raise ValueError('Wenceslas StepNew boundary changed')
    start = source.index(step_start)
    finish = source.index(step_finish, start)
    step_ring = '''U0 StepNew(CTask *, I64 x, I64 y, F64 Θ, Bool left_right, Bool king)
{
\tif (king)
\t\tStepRingPush(arm64_king_steps, ARM64_KING_STEP_CAP,
\t\t\t\t&arm64_king_step_head, &arm64_king_step_count, x, y);

\tif (left_right)
\t{
\t\tStepRingPush(arm64_steps, ARM64_STEP_CAP, &arm64_step_head, &arm64_step_count,
\t\t\t\tx - 3.5 * Sin(Θ) + 2.0 * Cos(Θ), y + 3.5 * Cos(Θ) + 2.0 * Sin(Θ));
\t\tStepRingPush(arm64_steps, ARM64_STEP_CAP, &arm64_step_head, &arm64_step_count,
\t\t\t\tx - 3.5 * Sin(Θ) + 5.0 * Cos(Θ), y + 3.5 * Cos(Θ) + 5.0 * Sin(Θ));
\t}
\telse
\t{
\t\tStepRingPush(arm64_steps, ARM64_STEP_CAP, &arm64_step_head, &arm64_step_count,
\t\t\t\tx + 3.5 * Sin(Θ), y - 3.5 * Cos(Θ));
\t\tStepRingPush(arm64_steps, ARM64_STEP_CAP, &arm64_step_head, &arm64_step_count,
\t\t\t\tx + 3.5 * Sin(Θ) + 3.0 * Cos(Θ), y - 3.5 * Cos(Θ) + 3.0 * Sin(Θ));
\t}
}
'''.encode('utf-8')
    source = source[:start] + step_ring + source[finish:]

    follow_start = b'\tStep *tmps = king_step_head.next;\n\tF64\t  d, best_d = F64_MAX;\n\n\twhile (tmps != &king_step_head)\n\t{'
    follow_end = b'\n}\n\nU0 AnimateTask'
    if source.count(follow_start) != 1 or source.count(follow_end) != 1:
        raise ValueError('Wenceslas follower queue boundary changed')
    start = source.index(follow_start)
    finish = source.index(follow_end, start)
    follow_ring = (b'\tStep *tmps;\n\tI64 i;\n\tF64 d, best_d = F64_MAX;\n\n'
                   b'\tfor (i = 0; i < arm64_king_step_count; i++)\n'
                   b'\t{\n'
                   b'\t\ttmps = &arm64_king_steps[(arm64_king_step_head + i) % ARM64_KING_STEP_CAP];\n'
                   b'\t\tif (tS - tmps->t0 < 3.0 &&\n'
                   b'\t\t\t\t(d = Sqr(tmps->x - tmpp->x) + Sqr(tmps->y - tmpp->y)) && d < 15 * 15)\n'
                   b'\t\t{\n'
                   b'\t\t\td += 1000 * Sqr(tS - tmps->t0);\n'
                   b'\t\t\tif (d < best_d)\n'
                   b'\t\t\t{\n'
                   b'\t\t\t\tbest_d = d;\n'
                   b'\t\t\t\ttmpp->\xce\x98 = Arg(tmps->x - tmpp->x, tmps->y - tmpp->y);\n'
                   b'\t\t\t}\n'
                   b'\t\t}\n'
                   b'\t}')
    source = source[:start] + follow_ring + source[finish:]

    init_queues = b'\tQueueInit(&step_head);\n\tQueueInit(&king_step_head);'
    reset_rings = (b'\tarm64_step_head = arm64_step_count = 0;\n'
                   b'\tarm64_king_step_head = arm64_king_step_count = 0;')
    if source.count(init_queues) != 1:
        raise ValueError('Wenceslas trail queue initialization changed')
    source = source.replace(init_queues, reset_rings, 1)

    cleanup_queues = b'\tQueueDel(&step_head, TRUE);\n\tQueueDel(&king_step_head, TRUE);'
    if source.count(cleanup_queues) != 1:
        raise ValueError('Wenceslas trail queue cleanup changed')
    source = source.replace(cleanup_queues, b'', 1)

    # The animation task used to prune both linked queues. Ring readers now
    # filter records by age, so there is no shared-list mutation to perform.
    prune_start = b'\t\ttmps = step_head.next;\n\t\twhile (tmps != &step_head)\n\t\t{'
    prune_finish = b'\n\t\tnot_stopped_count = 0;'
    if source.count(prune_start) != 1 or source.count(prune_finish) != 1:
        raise ValueError('Wenceslas trail pruning boundary changed')
    start = source.index(prune_start)
    finish = source.index(prune_finish, start)
    source = source[:start] + source[finish:]

    # The pinned nested start:/end: cursor sub-switch wedges this AArch64
    # guest after a cursor event. Lower the complete input switch to explicit
    # conditions while retaining the original key actions.
    declaration = b'U0 Wenceslas()\n{\n\tI64 sc;'
    if source.count(declaration) != 1:
        raise ValueError('expected one Wenceslas input function declaration')
    source = source.replace(declaration,
                            b'U0 Wenceslas()\n{\n\tI64 sc, ch;', 1)
    switch_start = b'\t\t\tswitch (KeyGet(&sc))\n'
    switch_end = b'\n\t\t\t}\n\t\t}\nws_done:'
    if source.count(switch_start) != 1 or source.count(switch_end) != 1:
        raise ValueError('Wenceslas input switch boundary changed')
    start = source.index(switch_start)
    finish = source.index(switch_end, start) + len(b'\n\t\t\t}')
    replacement = '''\t\t\tch = KeyGet(&sc);
\t\t\tif (ch == '\\n')
\t\t\t{
\t\t\t\tCleanUp;
\t\t\t\tInit;
\t\t\t}
\t\t\telse if (ch == CH_ESC || ch == CH_SHIFT_ESC)
\t\t\t\tgoto ws_done;
\t\t\telse if (ch == CH_SPACE)
\t\t\t\tBurnTrees;
\t\t\telse if (!ch)
\t\t\t{
\t\t\t\tif (sc.u8[0] == SC_CURSOR_RIGHT)
\t\t\t\t{
\t\t\t\t\tif (king_x + KING_STEP < Fs->pix_width - BORDER)
\t\t\t\t\t\tking_x += KING_STEP;
\t\t\t\t\tking_Θ = 0;
\t\t\t\t}
\t\t\t\telse if (sc.u8[0] == SC_CURSOR_LEFT)
\t\t\t\t{
\t\t\t\t\tif (king_x - KING_STEP >= BORDER)
\t\t\t\t\t\tking_x -= KING_STEP;
\t\t\t\t\tking_Θ = π;
\t\t\t\t}
\t\t\t\telse if (sc.u8[0] == SC_CURSOR_DOWN)
\t\t\t\t{
\t\t\t\t\tif (king_y + KING_STEP < Fs->pix_height - BORDER)
\t\t\t\t\t\tking_y += KING_STEP;
\t\t\t\t\tking_Θ = -π / 2;
\t\t\t\t}
\t\t\t\telse if (sc.u8[0] == SC_CURSOR_UP)
\t\t\t\t{
\t\t\t\t\tif (king_y - KING_STEP >= BORDER)
\t\t\t\t\t\tking_y -= KING_STEP;
\t\t\t\t\tking_Θ = π / 2;
\t\t\t\t}
\t\t\t\tif (sc.u8[0] == SC_CURSOR_RIGHT || sc.u8[0] == SC_CURSOR_LEFT ||
\t\t\t\t\t\tsc.u8[0] == SC_CURSOR_DOWN || sc.u8[0] == SC_CURSOR_UP)
\t\t\t\t{
\t\t\t\t\tking_mS = 0;
\t\t\t\t\tking_phase = (king_phase + 2) & 2;
\t\t\t\t\tStepNew(Fs, king_x, king_y, king_Θ, king_phase & 2, TRUE);
\t\t\t\t\tking_timeout = tS + 0.5;
\t\t\t\t}
\t\t\t}'''.encode('utf-8')
    source = source[:start] + replacement + source[finish:]
    return source


def translate_job_focus_trace(source: bytes) -> bytes:
    """Trace focus restore and wait for synchronous popup completion."""
    old = (b'\t\t\tif (Bt(&flags, JOBf_FOCUS_MASTER) && '
           b'!Bt(&master->win_inhibit, WIf_SELF_FOCUS))\n'
           b'\t\t\t\tsys_focus_task = master;')
    new = (b'\t\t\tif (Bt(&flags, JOBf_FOCUS_MASTER) && '
           b'!Bt(&master->win_inhibit, WIf_SELF_FOCUS))\n'
           b'\t\t\t{\n'
           b'\t\t\t\tZcFocusTrace("JobRunOne", sys_focus_task, master);\n'
           b'\t\t\t\tsys_focus_task = master;\n'
           b'\t\t\t}')
    if source.count(old) != 1:
        raise ValueError('pinned JobRunOne focus restore shape changed')
    source = source.replace(old, new)
    popup = (b'\t\tJobResScan(tmpc, &res);\n'
             b'\t\tFs->popup_task = NULL;\n'
             b'\t\tKill(task);')
    if source.count(popup) != 1:
        raise ValueError('pinned synchronous PopUp completion shape changed')
    return source.replace(popup,
        b'\t\tres = JobResGet(tmpc);\n'
        b'\t\tFs->popup_task = NULL;\n'
        b'\t\tKill(task);', 1)


def translate_sched_focus_trace(source: bytes) -> bytes:
    """Trace the pinned cooperative focus-ring walk without altering it."""
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("Sched.ZC is not UTF-8") from exc
    clear = "\tsys_focus_task = NULL;\n"
    select = "\t\t\tsys_focus_task = task;\n"
    if text.count(clear) != 1 or text.count(select) != 1:
        raise ValueError("expected the pinned TaskFocusNext focus assignments")
    text = text.replace(
        clear,
        '\tZcFocusTrace("TaskFocusNextClear", sys_focus_task, NULL);\n' + clear,
        1,
    )
    text = text.replace(
        select,
        '\t\t\tZcFocusTrace("TaskFocusNextSelect", sys_focus_task, task);\n' + select,
        1,
    )
    return text.encode("utf-8")


def translate_ndarray_math(source: bytes) -> bytes:
    """Make the stock NDArray module self-contained and repair its allocator paths.

    The pinned module is normally loaded after Types.ZC, but isolated ARM64
    source probes and direct use need its CArray definition. The original
    F64 clear path allocates half the required storage, the non-clearing I64
    and F32 paths still zero-fill, and ArrayFree switches on an undeclared
    identifier instead of the array's recorded element type.
    """
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("NDArray.ZC is not UTF-8") from exc
    if '#include "Types"' in text or '#include "/System/Math/Types"' in text:
        raise ValueError("NDArray.ZC already includes its Types dependency")
    replacements = (
        ("arr->I64Buf = CAllocAligned(sizeof(I64) * arr->size, 16);\n\t\t\tbreak;",
         "arr->I64Buf = MAllocAligned(sizeof(I64) * arr->size, 16);\n\t\t\tbreak;"),
        ("\t\t\telse\n\t\t\t\tarr->F32Buf = CAllocAligned(sizeof(F32) * arr->size, 16);",
         "\t\t\telse\n\t\t\t\tarr->F32Buf = MAllocAligned(sizeof(F32) * arr->size, 16);"),
        ("arr->F64Buf = CAllocAligned(sizeof(I32) * arr->size, 16);",
         "arr->F64Buf = CAllocAligned(sizeof(F64) * arr->size, 16);"),
        ("arr->F64Buf = MAllocAligned(sizeof(I64) * arr->size, 16);",
         "arr->F64Buf = MAllocAligned(sizeof(F64) * arr->size, 16);"),
        ("\tswitch (type)\n\t{\n\t\tcase NDARRAY_I32:\n\t\t\tFree(arr->I32Buf);",
         "\tswitch (arr->type)\n\t{\n\t\tcase NDARRAY_I32:\n\t\t\tFree(arr->I32Buf);"),
    )
    for old, new in replacements:
        if text.count(old) != 1:
            raise ValueError("NDArray.ZC allocator/free source no longer matches expected stock code")
        text = text.replace(old, new, 1)
    return ('#include "Types"\n' + text).encode("utf-8")


def translate_math_test_dependencies(source: bytes, module: str) -> bytes:
    """Give the stock Math test scripts their explicit support-module imports."""
    dependencies = {
        "TestVec.ZC": ("Types", "Conversion", "F32", "Vec4", "Vec3"),
        "TestMat4.ZC": ("Types", "Conversion", "F32", "Vec4", "Vec3", "Mat4"),
        "TestF32.ZC": ("Types", "Conversion", "F32", "Math"),
    }
    name = module.rsplit("/", 1)[-1]
    if name not in dependencies:
        raise ValueError("unknown pinned Math test module")
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError(f"{name} is not UTF-8") from exc
    for dependency in dependencies[name]:
        if f'#include "/System/Math/{dependency}"' in text:
            raise ValueError(f"{name} already includes {dependency}")
    boundary = re.match(r"\A\s*Cd\(__DIR__\);;", text)
    if not boundary:
        raise ValueError(f"{name} no longer starts at the expected source boundary")
    includes = "".join(f'#include "/System/Math/{dep}"\n'
                        for dep in dependencies[name])
    return (text[:boundary.start()] + includes + text[boundary.start():]).encode("utf-8")


def translate_sysrep_processor(source: bytes) -> bytes:
    """Report firmware CPU family on ARM64 without inventing x86 CPUID data."""
    declaration = b'\t\t\t\t\tCRAXRBXRCXRDX regs;\n'
    cpuid = b'\t\t\t\t\tCPUId(1, &regs);\n'
    if source.count(declaration) != 1 or source.count(cpuid) != 1:
        raise ValueError('pinned SysRep processor register probe changed')
    source = source.replace(declaration, b'', 1).replace(cpuid, b'', 1)

    family_start = b'\t\t\t\t\t"Family\\t\\t\\t:$$BLACK$$";\n'
    speed_start = b'\t\t\t\t\t"Current Speed\\t:$$BLACK$$";\n'
    if source.count(family_start) != 1 or source.count(speed_start) != 1:
        raise ValueError('pinned SysRep processor family fields changed')
    first = source.index(family_start)
    last = source.index(speed_start, first)
    family = (b'\t\t\t\t\t"CPU Family\\t\\t:$$BLACK$$%Z$$FG$$\\n", '
              b'p_info->family, "ST_SMBIOS_PROCESSOR_FAMILIES";\n')
    source = source[:first] + family + source[last:]

    cpuid_flags = b'\t\t\t\t\tFlagsStrPrint(buffer, Define("ST_CPUID_1_EDX_FLAGS")'
    flags_start = source.find(cpuid_flags)
    if flags_start < 0:
        raise ValueError('pinned SysRep CPUID feature report changed')
    flags_end = source.find(b'\t\t\t\t\tFree(str);\n', flags_start)
    if flags_end < 0:
        raise ValueError('pinned SysRep CPUID report terminator changed')
    flags_end += len(b'\t\t\t\t\tFree(str);\n')
    source = source[:flags_start] + source[flags_end:]
    return source


def translate_cpurep_arm64(source: bytes) -> bytes:
    """Report ARM64 scheduler/timer facts without requiring x86 SMBIOS data."""
    old = b'\tSysRep(SMBIOSt_PROCESSOR);'
    new = (b'\t"SMBIOS processor details are unavailable on this AArch64 platform.\\n";')
    if source.count(old) != 1:
        raise ValueError('pinned CPURep SMBIOS call changed')
    source = source.replace(old, new, 1)
    old = b'"%03X Active Cores at %6h?nHz\\n", mp_count, ToF64(counts.time_stamp_freq);'
    new = (b'"AArch64 %03X active cores at %6h?nHz\\n", '
           b'mp_count, ToF64(counts.time_stamp_freq);')
    if source.count(old) != 1:
        raise ValueError('pinned CPURep active-core report changed')
    return source.replace(old, new, 1)


def translate_progress(source: bytes) -> bytes:
    """Replace Progress's x86-only countdown with an ARM64-friendly loop.

    The original assembly yields on every counter tick. A C translation of all
    12 million iterations is prohibitively slow in the current interpreter,
    even when yielding periodically. Preserve the full counter range and
    visible countdown while advancing it in coarse steps, yielding between
    updates so other cooperative tasks still run.
    """
    asm = (
        b'asm {\n_PROGRESS_DEMO::\n'
        b'\t\t\t\tMOV \t\tRCX, 12000000\n'
        b'@@05: \t\t\tMOV \t\tU64 [&progress1], RCX\n'
        b'\t\t\t\tCALL\t\t&Yield\n'
        b'\t\t\t\tLOOP\t\t@@05\n'
        b'\t\t\t\tMOV \t\tU64 [&progress1], RCX\n'
        b'\t\t\t\tRET\n}'
    )
    external = b'_extern _PROGRESS_DEMO U0 AsmProgressDemo();'
    if source.count(asm) != 1 or source.count(external) != 1:
        raise ValueError('pinned Progress countdown assembly changed')
    source = source.replace(asm,
        b'U0 AsmProgressDemo()\n{\n'
        b'\tI64 i;\n'
        b'\tfor (i = 12000000; i > 0; i -= 16384)\n'
        b'\t{\n'
        b'\t\tprogress1 = i;\n'
        b'\t\tYield;\n'
        b'\t}\n'
        b'\tprogress1 = 0;\n'
        b'}', 1)
    return source.replace(external, b'', 1)


def translate_empty_label_statement(source: bytes, label: bytes) -> bytes:
    """Satisfy the native parser where a stock goto label precedes `}`."""
    pattern = re.compile(rb'(?m)^' + re.escape(label) + rb':([^\n]*)\n(\t})')
    matches = list(pattern.finditer(source))
    if len(matches) != 1:
        raise ValueError(f'pinned empty label {label.decode()} changed')
    match = matches[0]
    replacement = label + b':' + match.group(1) + b'\n\t\t;\n\t}'
    return source[:match.start()] + replacement + source[match.end():]


def translate_docpopup(source: bytes) -> bytes:
    """Keep System Include's packed DolDoc resources available to the lexer.

    Remove the source-only word-wrap directive and use document compilation
    for source files that have a binary section after their text terminator.
    Plain text retains the existing include path and its include context.
    """
    directive = b'return $WW,0$PopUp2("YES", 1, "NO", 0, header, footer) > 0;'
    if source.count(directive) != 1 or source.count(b'$WW,0$') != 1:
        raise ValueError("expected the single pinned PopUpNoYes word-wrap directive")
    source = source.replace(directive, directive.replace(b'$WW,0$', b''), 1)
    include = b'\tif (okay)\n\t\tres = Sys("#include \\"%s\\";", name2);'
    if source.count(include) != 1:
        raise ValueError("expected the pinned SysFile include")
    document_include = b'''\
	if (okay)
	{
		I64 size, text_size = 0;
		U8 *bytes = FileRead(name2, &size);
		while (bytes && text_size < size && bytes[text_size])
			text_size++;
		Bool packed = bytes && text_size + 1 < size;
		Free(bytes);
		if (packed)
			res = Sys("BootstrapGuiTerminalExeDocFile(\\"%Q\\");", name2);
		else
			res = Sys("#include \\"%s\\";", name2);
	}'''
    return source.replace(include, document_include, 1)


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
        'DocPrint(doc, "%s", format)': 'DocPutS(doc, format)',
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
    replacement = """// ARM64: preserve the stock threshold check and grow only near the
// current task stack's low boundary. The trampoline also gives the target a
// fresh call frame when the current stack already has enough room.
argpop I64 CallStackGrow(I64 stack_size_threshold, I64 stack_size,
                         /*argpop*/I64 (*fp_addr)(...), ...)
{
    CTaskStack *tmp, *old_stack;
    U8 *stack_base, *stack_top;
    I64 result, unused;
    if (stack_size <= 0 || !fp_addr)
        return -1;
    old_stack = Fs->stack;
    if (old_stack) {
        stack_base = &old_stack->stack_base;
        unused = RSPGet()(U8 *) - stack_base;
        if (unused >= stack_size_threshold) {
            // Keep a little room for the bridge's own call frames below this SP.
            stack_top = RSPGet()(U8 *) - 0x200;
            return ZcCallNOnStack(stack_base, stack_top, fp_addr, argc, argv);
        }
    }
    tmp = TaskStackNew(stack_size, Fs);
    if (!tmp)
        return -1;
    tmp->next_stack = old_stack;
    if (old_stack)
        old_stack->stack_ptr = RSPGet;
    Fs->stack = tmp;
    result = ZcCallNOnStack(&tmp->stack_base,
                            (&tmp->stack_base)(U8 *) + tmp->stack_size,
                            fp_addr, argc, argv);
    Fs->stack = old_stack;
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


def extract_ktask_stack_grow(source: bytes) -> bytes:
    """Expose the transformed stock stack-growth API in the ARM64 task bridge.

    The host-fiber runtime does not load all of Kernel/KTask.ZC, so the stock
    KernelC.HH declaration otherwise resolves CallStackGrow to the unresolved
    external stub. Keep the implementation sourced from the pinned KTask
    translation while adding only its two required functions to TaskBridge.
    """
    try:
        text = source.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError("KTask.ZC is not UTF-8") from exc
    start = text.find("CTaskStack *TaskStackNew(")
    grow = text.find("argpop I64 CallStackGrow(", start)
    end = text.find("\nI64 TaskInit(", grow)
    if start < 0 or grow <= start or end <= grow:
        raise ValueError("expected TaskStackNew and CallStackGrow before TaskInit")
    stack_new = text[start:grow].strip()
    grow_fn = text[grow:end].strip()
    if (stack_new.count("CTaskStack *TaskStackNew(") != 1 or
            grow_fn.count("argpop I64 CallStackGrow(") != 1 or
            "ZcCallNOnStack(" not in grow_fn or
            "CallStackGrow: entered" in grow_fn):
        raise ValueError("unexpected transformed KTask stack-growth functions")
    return ("extern I64 ZcCallNOnStack(U8 *stack_low, U8 *stack_top, "
            "I64 (*fp_addr)(...), I64 argc, I64 *argv);\n\n" +
            stack_new + "\n\n" + grow_fn + "\n").encode("utf-8")


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
    task_ode = "\t\tODEsUpdate(task);"
    screen_tasks = "\tGrUpdateTasks;"
    if text.count(task_ode) != 1 or text.count(screen_tasks) != 1:
        raise ValueError("expected exact GrScreen task and screen update sites")
    # Temporary diagnostic bridge used by the X-Caliber UTM hang probe. The
    # extern is bound before the app runs; these calls are inert unless the
    # X-Caliber trace budget has been armed by its input probe.
    text = text.replace("\ttry\n" + task_ode,
                        "\ttry\n\t{\n"
                        "\t\tZcXCaliberFrameTrace(20, task);\n"
                        "\t\tODEsUpdate(task);\n"
                        "\t\tZcXCaliberFrameTrace(21, task);\n"
                        "\t}\n", 1)
    text = text.replace(screen_tasks,
                        "\tZcXCaliberFrameTrace(10, sys_focus_task);\n"
                        "\tGrUpdateTasks;\n"
                        "\tZcXCaliberFrameTrace(11, sys_focus_task);", 1)
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
    matches = list(_TEXT_BG_X86.finditer(text))
    if len(matches) != 1:
        raise ValueError("GrScreen text-background assembly changed during overlay")
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


def extract_god_timer_pick(source: bytes) -> bytes:
    """Expose the original timer picker without loading the God FIFO and Bible."""
    start = b'U8 *TimeStampCB('
    end = b'public U0 GodBitsInsert('
    if source.count(start) != 1 or source.count(end) != 1:
        raise ValueError('expected the pinned God timer picker')
    return source[source.index(start):source.index(end)]


def translate_palette_editor(source: bytes) -> bytes:
    """Publish live edits and let SettingsPop restore the editor window."""
    edits = (
        (b'\t\t\t\t\tgr_palette[selected_color] = Str2I64(manual_value, 16);',
         b'\t\t\t\t\tgr_palette[selected_color] = Str2I64(manual_value, 16);\n'
         b'\t\t\t\t\tGrPaletteSet(gr_palette);'),
        (b'\t\t\t\t\tgr_palette[selected_color] = ColorGod;',
         b'\t\t\t\t\tgr_palette[selected_color] = ColorGod;\n'
         b'\t\t\t\t\tSelectColor(selected_color, c);\n'
         b'\t\t\t\t\tGrPaletteSet(gr_palette);'),
        (b'\t\t\t\t\tPaletteGod;',
         b'\t\t\t\t\tPaletteGod;\n'
         b'\t\t\t\t\tSelectColor(selected_color, c);\n'
         b'\t\t\t\t\tGrPaletteSet(gr_palette);'),
        (b'I64 /*arg1, arg2,*/ sc = 0,\n'
         b'\t\toriginal_left, original_right, original_top, original_bottom;',
         b'I64 sc = 0;'),
        (b'\toriginal_left\t= c->win_task->win_left;\n'
         b'\toriginal_right\t= c->win_task->win_right;\n'
         b'\toriginal_top\t= c->win_task->win_top;\n'
         b'\toriginal_bottom\t= c->win_task->win_bottom;\n',
         b''),
        (b'\tc->win_task->win_left\t= original_left;\n'
         b'\tc->win_task->win_right\t= original_right;\n'
         b'\tc->win_task->win_top \t= original_top;\n'
         b'\tc->win_task->win_bottom = original_bottom;\n',
         b''),
        (b'\tMenuPop;\n\tSettingsPop;',
         b'\tSliderDel(c);\n\tMenuPop;\n\tSettingsPop;'),
        (b'\n\tSliderDel(c);\n\tDocBottom;',
         b'\n\tDocBottom;'),
    )
    for old, new in edits:
        if source.count(old) != 1:
            raise ValueError('expected pinned Palette Editor block: ' + repr(old[:64]))
        source = source.replace(old, new, 1)
    return source


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
        "\t\t\t\tZcXCaliberMessageTrace(res, arg1, arg2, task);\n"
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
    text = text.replace(old, new, 1)
    scan_anchor = "\tif (!task)\n\t\ttask = Fs;\n\told_flags = RFlagsGet;"
    scan_trace = ("\tif (!task)\n\t\ttask = Fs;\n"
                  "\tZcXCaliberMessageScanTrace(task);\n"
                  "\told_flags = RFlagsGet;")
    if text.count(scan_anchor) != 1:
        raise ValueError("expected unchanged SerialDev/Message.ZC MessageScan entry")
    return text.replace(scan_anchor, scan_trace, 1).encode("utf-8")


def translate_classmeta_callback(source: bytes) -> bytes:
    # Adapt expression metadata, then use the checked ARM64 reflection ABI.
    old_meta = b'I64 rank\t\tprint_str "%1d" default_val 6/2 output_fun &RankOut;'
    # The native runtime cannot call a guest function pointer embedded in
    # compiler metadata, so preserve the display behavior in DumpStruct below.
    new_meta = b'I64 rank\t\tprint_str "%1d" default_val (6/2);'
    old_percentile = b'\tI64 age \t\t\tprint_str "%2d" default_val 38 percentile 54.20;'
    # Aiwnios's generic expression parser treats following metadata names as
    # identifiers. Keep percentile as this field's sole metadata key; the
    # renderer below preserves the stock display annotation for Test2Struct.age.
    new_percentile = b'\tI64 age \t\t\tpercentile 54.20;'
    if source.count(old_meta) != 1 or source.count(old_percentile) != 1:
        raise ValueError('pinned ClassMeta metadata declarations changed')
    source = source.replace(old_meta, new_meta, 1).replace(old_percentile, new_percentile, 1)
    start_marker = b'U0 DumpStruct(U8 *_d, U8 *class_name=lastclass)'
    end_marker = b'\n\nTest1Struct t1;'
    if source.count(start_marker) != 1 or source.count(end_marker) != 1:
        raise ValueError('pinned ClassMeta DumpStruct boundaries changed')
    start = source.index(start_marker)
    end = source.index(end_marker, start)
    old = source[start:end]
    required = (b'MemberMetaData("default_val", ml)',
                b'MemberMetaData("percentile", ml)(F64)',
                b'MemberMetaData("output_fun", ml)')
    if any(token not in old for token in required):
        raise ValueError('pinned ClassMeta metadata behavior changed')
    replacement = b'''U0 DumpStruct(U8 *_d, U8 *class_name=lastclass)
{//Use the ARM64 reflection ABI; native compiler reflection structs differ.
    CArmClassMetaField field;
    I64 i, *q;

    for (i = 0; ZcClassMetaField(class_name, i, &field); i++)
    {
        "%s:", field.name;
        q = _d + field.offset;
        if (field.print_str)
            "" field.print_str, *q;
        else
            "%d", *q;
        if (field.has_default_val)
            " default:%d", field.default_val;
        else if (!StrCompare(class_name, "Test2Struct") &&
                !StrCompare(field.name, "age"))
            " default:%d", 38;
        if (field.has_percentile)
            " percentile: %.2f", field.percentile;
        if (!StrCompare(field.name, "rank") &&
                !StrCompare(class_name, "Test1Struct"))
            RankOut(*q);
        '\n';
    }
}'''
    return source[:start] + replacement + source[end:]


def translate_pick3d_class_terminator(source: bytes) -> bytes:
    """Repair the missing class terminator in the pinned Pick3D demo."""
    old = (b'\tI64 \t*r, \n'
           b'\t\t\t*dr; //Rounding error might eventually screw this up\n'
           b'}\n\nclass PObj')
    new = (b'\tI64 \t*r, \n'
           b'\t\t\t*dr; //Rounding error might eventually screw this up\n'
           b'};\n\nclass PObj')
    if source.count(old) != 1:
        raise ValueError('pinned Pick3D sprite class terminator changed')
    return source.replace(old, new, 1)


def translate_home_wrappers(source: bytes) -> bytes:
    """Provide the stock F2/FD2 wrappers required by the TOS distro loader."""
    old = (b'public I64 R(U8 *needle_str, U8 *replace_text = NULL, '
           b'U8 *fu_flags = "+l-i")')
    new = (b'#define FILEMASK_OFF_LIMITS "!*/Misc/PCIDevices.DD*;!*/Misc/Bible.TXT*;"\\\n'
           b'\t\t\t\t\t\t\t"!*/Books?/*;!*/Sup1/Sup1Words/*;!*/Sup1/Sup1Bin/*;"\\\n'
           b'\t\t\t\t\t\t\t"!*/God/Vocab.DD*"\n'
           b'public I64 F2(U8 *needle_str, U8 *fu_flags=NULL)\n'
           b'{\n\treturn Find(needle_str, "/*;" FILEMASK_OFF_LIMITS, fu_flags);\n}\n\n'
           b'public I64 FD2(U8 *needle_str, U8 *fu_flags=NULL)\n'
           b'{\n\treturn Find(needle_str, "*;" FILEMASK_OFF_LIMITS, fu_flags);\n}\n\n'
           + old)
    if source.count(old) != 1 or b'public I64 F2(' in source:
        raise ValueError('pinned root HomeWrappers shape changed')
    return source.replace(old, new, 1)


def translate_make_tos_dependency(source: bytes) -> bytes:
    """Load MakeTOS's extension and ScreenCast dependencies on ARM64."""
    old = b'#include "TOSExt"'
    new = (b'#define KERNEL_ZXE "Kernel.ZXE"\n'
           b'#define BOOT_DIR "/Boot"\n'
           b'#define BOOT_DIR_KERNEL_ZXE BOOT_DIR "/" KERNEL_ZXE\n'
           b'#define TOS_HDS "CD"\n'
           b'#ifndef INS_REG_PERSONAL_INITIALS\n'
           b'#define INS_REG_PERSONAL_INITIALS "TAD"\n'
           b'#endif\n'
           b'#ifndef INS_REG_MACHINE_NUM\n'
           b'#define INS_REG_MACHINE_NUM 0\n'
           b'#endif\n'
           b'#include "/HomeWrappers"\n'
           b'#include "TOSExtern"\n'
           b'#include "/Apps/Psalmody/JukePuppet"\n'
           b'#include "/System/BlkDev/DiskCheck"\n'
           b'#include "/System/BlkDev/ZDiskA"\n'
           b'#include "/System/BlkDev/ZDiskB"\n'
           b'#include "/System/Utils/DocUtils"\n'
           b'#include "/System/Utils/StrUtils"\n'
           b'#include "/System/Gr/ScreenCast"')
    if source.count(old) != 1:
        raise ValueError('pinned MakeTOS extension include changed')
    return source.replace(old, new, 1)


def translate_make_home_dependency(source: bytes) -> bytes:
    """Load MakeHome's kernel/compiler and disk helpers before MakeUtils."""
    old = (b'#include "/System/Boot/MakeBoot"\n'
           b'#include "/System/Utils/MakeUtils"\n'
           b'#include "~/HomeWrappers"\n'
           b'MapFileLoad("::/Kernel/Kernel");\n'
           b'MapFileLoad("::/Compiler/Compiler");')
    new = (b'#include "/System/Boot/MakeBoot"\n'
           b'#include "/System/BlkDev/DiskCheck"\n'
           b'#include "/System/BlkDev/ZDiskA"\n'
           b'#include "/System/Memory"\n'
           b'#include "::/Compiler/CompilerA.HH"\n'
           b'#include "::/Compiler/CompilerB.HH"\n'
           # ARM64's kernel APIs are already registered by the native source
           # runtime. Only the compiler's public declarations replace its
           # legacy x86 CZXE .MAP export table here.
           b'#include "/System/God/GodBible"\n'
           b'#include "/System/Utils/MakeUtils"\n'
           b'#include "~/HomeWrappers"')
    if source.count(old) != 1:
        raise ValueError('pinned MakeHome boot/utils includes changed')
    return source.replace(old, new, 1)


def translate_legacy_kernel_header(source: bytes) -> bytes:
    """Drop x86 kernel-map labels used as extern declaration annotations.

    The pinned KernelB/KernelC headers put fixed-address map labels between
    ``_extern`` and the declared type (for example ``_extern SYS_CODE_BP
    CBlkPool *sys_code_bp``). ARM64 boots from an ELF image and has no such
    x86 map labels, so these tokens are not types in the source compiler.
    Convert those map-annotated declarations to ordinary extern declarations
    so source modules can parse the legacy API names against ARM64 bindings.
    """
    pattern = re.compile(rb'_extern\s+(?:SYS|MEM|C32)_[A-Z0-9_]+(\s+)')
    translated, count = pattern.subn(rb'extern\1', source)
    if count == 0:
        raise ValueError('no legacy kernel-map annotations found')
    return translated


def translate_cartesian_plot_color(source: bytes) -> bytes:
    """Keep the Cartesian curve visible on the ARM64 terminal's black canvas."""
    old = b'\t\t\t\tdc->color = BLACK;'
    new = (b'\t\t\t\t// The ARM64 graphical terminal clears this surface to black.\n'
           b'\t\t\t\t// Keep the plotted curve visible against that background.\n'
           b'\t\t\t\tdc->color = WHITE;')
    if source.count(old) != 1:
        raise ValueError('pinned Cartesian curve color assignment changed')
    return source.replace(old, new, 1)


def translate_low_pass_filter_sampling(source: bytes) -> bytes:
    """Bound the randomized graph's synchronous work on the ARM64 desktop.

    The upstream 0..0.1 pixel step draws tens of thousands of subpixel lines
    per curve. That monopolizes the single-core ARM64 guest and prevents input
    and window-manager tasks from running. Keep the random jitter while using
    approximately pixel-sized steps, so the demo stays interactive.
    """
    old = b'dt = Rand / 10;'
    new = b'dt = 1.0 + Rand / 10;'
    if source.count(old) != 2:
        raise ValueError('pinned LowPassFilter randomized curve loops changed')
    return source.replace(old, new)


def translate_symmetry_message_masks(source: bytes) -> bytes:
    """Use explicit event-mask unions for Symmetry's pointer and key waits.

    The pinned source chains shifts and additions, relying on upstream
    operator parsing to produce a union of event bits. The ARM64 source
    compiler follows C precedence, where addition binds before shifts, so
    those expressions create the wrong mask and the demo silently discards
    pointer events. Keep the upstream file pinned and normalize its three
    masks in the generated source bundle.
    """
    replacements = (
        (b'1 << MESSAGE_KEY_DOWN + 1 << MESSAGE_MS_L_DOWN + 1 << MESSAGE_MS_R_DOWN',
         b'(1 << MESSAGE_KEY_DOWN) | (1 << MESSAGE_MS_L_DOWN) | (1 << MESSAGE_MS_R_DOWN)'),
        (b'1 << MESSAGE_MS_R_UP + 1<< MESSAGE_MS_MOVE',
         b'(1 << MESSAGE_MS_R_UP) | (1 << MESSAGE_MS_MOVE)'),
        (b'1 << MESSAGE_MS_L_UP + 1 << MESSAGE_MS_MOVE',
         b'(1 << MESSAGE_MS_L_UP) | (1 << MESSAGE_MS_MOVE)'),
    )
    for old, new in replacements:
        if source.count(old) != 1:
            raise ValueError('pinned Symmetry message mask changed')
        source = source.replace(old, new, 1)
    return source


def translate_fonted_draw_callback(source: bytes) -> bytes:
    """Preserve FontEd's task draw callback across its temporary DocWrite.

    The upstream local was declared U8, truncating the 64-bit callback
    pointer. After saving from FontEd, the window manager could jump to the
    truncated address while redrawing the task. Use the callback's real type.
    """
    old = b'\tU8\t\t old_draw_it = Fs->draw_it;'
    new = b'\tU0\t\t(*old_draw_it)(CTask *task, CDC *dc) = Fs->draw_it;'
    if source.count(old) != 1:
        raise ValueError('pinned FontEd callback save declaration changed')
    return source.replace(old, new, 1)


def translate_tos_config_boot_memory(source: bytes) -> bytes:
    """Keep the stock TOS config module loadable without the x86 boot-image map.

    The optional debug distro initializer derives a RAM-disk range from the
    x86 ``mem_boot_base`` image layout. The ARM64 boot path has no equivalent
    address map in this user-space source runtime, and MAKE_DEBUG is disabled
    in the stock TOS distro. Preserve the initializer names and generated
    config symbols while reserving an empty range for that disabled feature.
    """
    old = b'''U0 TOSDebugDistro1()
{
	CZXE *zxe = mem_boot_base - sizeof(CZXE);

	zxe(I64) += zxe->file_size - 1;
	DefinePrint("TOS_DEBUG_DISTRO", "0x%X", CeilI64(SYS_KERNEL_END  + SLOP, BLK_SIZE));
	DefinePrint("TOS_DEBUG_DISTRO_END", "0x%X",
			FloorI64((BOOT_RAM_LIMIT - (BOOT_STACK_SIZE + DVD_BOOT_LOADER_SIZE)) >> 4 << 4 -
			(zxe(I64) - SYS_KERNEL_END) - SLOP, BLK_SIZE));
	DefinePrint("CONFIG_DEBUG_DISTRO_FILE", "\\\"/Tmp/DebugDistro.BIN\\\"");

} TOSDebugDistro1;'''
    new = b'''U0 TOSDebugDistro1()
{
	// The legacy debug distro occupies x86 boot-image RAM; ARM64 has no
	// compatible mem_boot_base mapping. MAKE_DEBUG remains disabled here.
	DefinePrint("TOS_DEBUG_DISTRO", "0");
	DefinePrint("TOS_DEBUG_DISTRO_END", "0");
	DefinePrint("CONFIG_DEBUG_DISTRO_FILE", "\\\"/Tmp/DebugDistro.BIN\\\"");

} TOSDebugDistro1;'''
    if source.count(old) != 1:
        raise ValueError('pinned TOS debug distro boot-memory initializer changed')
    source = source.replace(old, new, 1)
    marker = b'#define SLOP\t\tBLK_SIZE'
    replacement = (marker + b'\n'
                   b'// Compile-time defaults for the disabled x86 debug-distro path.\n'
                   b'#define TOS_DEBUG_DISTRO\t\t0\n'
                   b'#define TOS_DEBUG_DISTRO_END\t0\n'
                   b'// TOSDistro expands these while compiling its nested include;\n'
                   b'// the runtime DefinePrint calls in TOSInit are too late there.\n'
                   b'#define CONFIG_RAM_DRIVES\t"B\\nScale2Mem(2048,0x100000,4*1024*1024*1024)\\n"\n'
                   b'#define CONFIG_HARD_DRIVES\t""\n'
                   b'#define CONFIG_DVD_DRIVES\t""\n'
                   b'#define CONFIG_DISK_CACHE\t"Scale2Mem(0x80000,0x8000000)\\n"')
    if source.count(marker) != 1:
        raise ValueError('pinned TOSConfig SLOP definition changed')
    source = source.replace(marker, replacement, 1)
    machine_switch = b'switch (INS_REG_MACHINE_NUM)'
    if source.count(machine_switch) != 1:
        raise ValueError('pinned TOSConfig machine selector changed')
    # The host ARM64 profile is not the legacy TAD installation target. Use
    # TOSConfig's portable default drive layout instead of Registry.ZC state.
    return source.replace(machine_switch, b'switch (0)', 1)


def translate_memrep_arm64(source: bytes) -> bytes:
    """Replace legacy x86/compiler memory-report fields with ARM64 equivalents."""
    old = (b'\tCZXE\t\t*zxe = mem_boot_base - sizeof(CZXE);\n\n'
           b'\tzxe(I64) += zxe->file_size - 1;\n\n'
           b'\t"$$BLACK$$Low Memory\\t:00100000$$FG$$\\n$$ID,2$$";\n'
           b'\t"Kernel\\t:%08X-%08X\\n", mem_boot_base - sizeof(CZXE), zxe;\n')
    new = (b'\t"Kernel image\\t:Limine-loaded AArch64 ELF (no CZXE header)\\n";\n'
           b'\t"$$ID,2$$";\n')
    if source.count(old) != 1:
        raise ValueError('pinned MemRep CZXE boot-image report changed')
    source = source.replace(old, new, 1)
    compiler_control = b'\tCCompCtrl\t*cc;'
    if source.count(compiler_control) != 1:
        raise ValueError('pinned MemRep compiler-control local changed')
    source = source.replace(compiler_control, b'', 1)
    compiler_loop = (b'\tcc = task->next_cc;\n'
                     b'\twhile (cc != &task->next_cc)\n'
                     b'\t{\n'
                     b'\t\t"CompCtrl\\t:%010X\\n", CompCtrlSize(cc);\n'
                     b'\t\tcc = cc->next;\n'
                     b'\t}\n')
    if source.count(compiler_loop) != 1:
        raise ValueError('pinned MemRep compiler-control report loop changed')
    return source.replace(
        compiler_loop,
        b'\t// ARM64 uses the host-native compiler; its internal control blocks\n'
        b'\t// are not ZealOS CCompCtrl allocations and are not reported here.\n',
        1)


def translate_profiler_idle_sentinel(source: bytes) -> bytes:
    """Give the legacy profiler a stable idle-sample token on ARM64."""
    marker = b'#define PF_ARRAY_COUNT 0x100000'
    replacement = (b'// No x86 SYS_IDLE_PT assembly label exists on AArch64.\n'
                   b'#define SYS_IDLE_PT 0\n' + marker)
    if source.count(marker) != 1:
        raise ValueError('pinned Profiler sample-array definition changed')
    if b'#define SYS_IDLE_PT' in source:
        raise ValueError('pinned Profiler already defines SYS_IDLE_PT')
    return source.replace(marker, replacement, 1)


def translate_interrupt_demo_svc(source: bytes) -> bytes:
    """Port the x86 INT-vs-call benchmark to the guest's AArch64 SVC path."""
    required = (
        b'#define I_F_UNARY_MINUS I_USER',
        b'IntEntrySet(I_F_UNARY_MINUS, F_UNARY_MINUS_INT, IDTET_TRAP)',
        b'#define SAMPLE_SIZE \t\t1000000',
        b'_extern _F_UM_INT  F64 UnaryMinusInt(F64 d);',
        b'F_UNARY_MINUS_INT::',
    )
    if any(source.count(marker) != 1 for marker in required):
        raise ValueError('pinned InterruptDemo benchmark structure changed')
    return b'''// AArch64 equivalent of the pinned x86 software-interrupt benchmark.
// Arm64SvcBench executes SVC #0x42 in the guest and returns through the EL1
// exception vector. TSCGet reads the guest generic timer, so the values below
// are timer ticks per operation, not CPU cycles.
_extern _ARM64_SVC_BENCH U0 Arm64SvcBench();
_extern _ARM64_SVC_BENCH_COUNT U64 Arm64SvcBenchCount();

#define SAMPLE_SIZE 10000

F64 UnaryMinusInt(F64 d)
{
    Arm64SvcBench;
    return -d;
}

F64 UnaryMinusCall(F64 d)
{
    return -d;
}

U0 TimeIns()
{
    U64 start, end, hits_before, hits_after;
    I64 i;

    hits_before = Arm64SvcBenchCount;
    start = TSCGet;
    for (i = 0; i < SAMPLE_SIZE; i++)
        UnaryMinusInt(3.141592653589793);
    end = TSCGet;
    hits_after = Arm64SvcBenchCount;
    "AArch64 SVC timer ticks: %10.5f\\n", ToF64(end - start) / SAMPLE_SIZE;
    "SVC count: %d\\n", hits_after - hits_before;

    start = TSCGet;
    for (i = 0; i < SAMPLE_SIZE; i++)
        UnaryMinusCall(3.141592653589793);
    end = TSCGet;
    "Direct call timer ticks: %10.5f\\n", ToF64(end - start) / SAMPLE_SIZE;
}

TimeIns;
'''


def translate_andnotmod_benchmark(source: bytes) -> bytes:
    """Scale the stock timing sample for usable execution under AArch64 TCG.

    The pinned 100-million-iteration remainder pass does not reach its first
    result in a five-minute UTM/QEMU run. One million iterations still gives a
    useful averaged timing comparison while letting the demo return promptly.
    TSCGet is the guest generic timer on this target, so label the reported
    measurements as timer ticks instead of x86 CPU cycles.
    """
    old = b'#define SAMPLE_SIZE \t\t100000000'
    if source.count(old) != 1:
        raise ValueError('pinned AndNotMod sample-size definition changed')
    source = source.replace(old,
                            b'// AArch64 TCG keeps the comparison interactive.\n'
                            b'#define SAMPLE_SIZE \t\t1000000', 1)
    replacements = (
        (b'Remainder Version #1 Cycles',
         b'Remainder Version #1 Timer Ticks'),
        (b'Remainder Version #2 Cycles',
         b'Remainder Version #2 Timer Ticks'),
    )
    for old_label, new_label in replacements:
        # Each label appears in the displayed output format and its sample
        # output comment; keep both copies consistent with the target timer.
        if source.count(old_label) != 2:
            raise ValueError(f'pinned AndNotMod output label changed: {old_label!r}')
        source = source.replace(old_label, new_label)
    return source


def translate_wallpaper_stats(source: bytes) -> bytes:
    """Adapt Wallpaper statistics to the ARM64 compiler/task model.

    Upstream reads ``cmp.compiled_lines`` from its compiler project. The
    AArch64 native compiler does not publish that project global in the
    system namespace, so render an explicit N/A. Its task-memory helpers
    live in System/Memory.ZC, which is not loaded into this source context;
    calculate those same counters from each task's public heap controls.
    """
    if source.count(b'cmp.compiled_lines') != 2:
        raise ValueError('expected the two upstream wallpaper compiler counters')
    if source.count(b'%12td%12td%12td') != 2:
        raise ValueError('expected the two upstream wallpaper counter formats')
    if source.count(b'TaskMemAlloced(task)') != 1 or \
            source.count(b'TaskMemUsed(task)') != 1:
        raise ValueError('expected the two upstream wallpaper task-memory counters')
    if source.count(b'U0 WallPaper(CTask *_task)') != 1:
        raise ValueError('expected the upstream Wallpaper entry point')
    source = source.replace(b'%12td%12td%12td', b'%12ts%12td%12td')
    source = source.replace(b'cmp.compiled_lines', b'"N/A"')
    source = source.replace(b'TaskMemAlloced(task)',
                            b'WallPaperTaskMemAlloced(task)')
    source = source.replace(b'TaskMemUsed(task)',
                            b'WallPaperTaskMemUsed(task)')
    helpers = b'''\
I64 WallPaperTaskMemUsed(CTask *task)
{
    I64 res = task->code_heap->used_u8s;
    if (task->code_heap != task->data_heap)
        res += task->data_heap->used_u8s;
    return res;
}

I64 WallPaperTaskMemAlloced(CTask *task)
{
    // The ARM64 host uses a shared arena and retains no per-task free pages;
    // task-owned capacity therefore equals its currently live allocations.
    return WallPaperTaskMemUsed(task);
}

'''
    return source.replace(b'U0 WallPaper(CTask *_task)', helpers +
                          b'U0 WallPaper(CTask *_task)', 1)


def translate_wallpaper_desktop(source: bytes) -> bytes:
    """Use packed sprites and stable hit boxes for the desktop launchers."""
    text, separator, sprites = translate_wallpaper_stats(source).partition(b'\0')
    if not separator or text.count(b'Sprite3(dc,') != 6 or text.count(b'SpriteExtents(') != 6:
        raise ValueError('expected six desktop sprite draws and extent calls')
    text = text.replace(b'Sprite3(dc,', b'Sprite3Packed(dc,')
    text = text.replace(b'SpriteExtents(', b'SpriteExtentsPacked(')
    offset = b'Sprite3Packed(dc, c->left-8, c->top-8, 0,'
    if text.count(offset) != 3:
        raise ValueError('expected three offset desktop sprites')
    text = text.replace(offset, b'Sprite3Packed(dc, c->left, c->top, 0,')
    wallpaper = b'U0 WallPaper(CTask *_task)\n{'
    stats_click = b'Sys("WallPaperToggle;");'
    if text.count(wallpaper) != 1 or text.count(stats_click) != 1:
        raise ValueError('expected the desktop stats renderer and launcher')
    text = text.replace(wallpaper,
                        b'Bool wallpaper_desktop_stats = TRUE;\n\n' + wallpaper +
                        b'\n\tif (!wallpaper_desktop_stats) return;', 1)
    text = text.replace(stats_click,
                        b'wallpaper_desktop_stats = !wallpaper_desktop_stats;', 1)
    netlog_click = b'\tif(down)\n\t\tSys("NetLogToggle;");'
    if text.count(netlog_click) != 1:
        raise ValueError('expected the desktop NetLog launcher')
    text = text.replace(netlog_click, b'''\
\tif (down)
\t\tSys("if (ZcCallExtStr(\\"BootstrapNetLogToggle\\") == -1) { \
ExeFile2(\\"/Home/Net/Utilities/NetLog.ZC\\"); ZcCallExtStr(\\"BootstrapNetLogToggle\\"); }");''', 1)
    telnet_click = b'PopUp("ExeFile2(\\"~/Net/Programs/Telnet/Telnet.ZC\\");");'
    if text.count(telnet_click) != 1:
        raise ValueError('expected the pinned desktop Telnet launcher')
    text = text.replace(telnet_click, b'''\
TaskExe(sys_task, NULL,
    "if (ZcCallExtStr(\\"BootstrapDesktopTelnet\\") == -1) "
    "{ ExeFile2(\\"/Home/Net/Load.ZC\\"); ZcCallExtStr(\\"BootstrapDesktopTelnet\\"); }",
    1 << JOBf_FREE_ON_COMPLETE);''', 1)

    # The ARM64 System task has full-screen logical bounds even when hidden.
    # Lay out the left column from its own geometry and keep each draw inside
    # the corresponding hit box. The pinned draw callbacks changed only part
    # of each box after the screen coordinates had already been derived.
    for name, sprite in ((b'Home', b'2'), (b'Telnet', b'3'), (b'Palette', b'4')):
        draw = rb'U0 Draw' + name + rb'Icon\(CDC \*dc, CCtrl \*c\)\n\{.*?\n\}'
        updated_draw = (b'U0 Draw' + name + b'Icon(CDC *dc, CCtrl *c)\n{\n'
                        b'\tI64 *origin = c->state;\n'
                        b'\tSprite3Packed(dc, c->left - origin[0], c->top - origin[1], -1, '
                        b'$IB,"<' + sprite + b'>",BI=' + sprite + b'$);\n}')
        text, count = re.subn(draw, lambda _: updated_draw, text, flags=re.S)
        start = b'CCtrl *' + name + b'IconNew()'
        if count != 1 or text.count(start) != 1:
            raise ValueError(f'desktop {name.decode()} icon callback changed')
        begin = text.index(start)
        end = text.index(b'\n}', begin) + 2
        block = text[begin:end].replace(start, start[:-1] + b'I64 top)')
        extents = re.compile(rb'\tSpriteExtentsPacked\(.*?\n\tQueueInsert', re.S)
        placement = (b'\tSpriteExtentsPacked($IB,"<' + sprite + b'>",BI=' + sprite +
                     b'$, &min_x, &max_x, &min_y, &max_y);\n'
                     b'\tc->left = FONT_WIDTH;\n'
                     b'\tc->right = c->left + max_x - min_x;\n'
                     b'\tc->top = top;\n'
                     b'\tc->bottom = c->top + max_y - min_y;\n'
                     b'\tI64 *origin = SysCAlloc(2 * sizeof(I64));\n'
                     b'\torigin[0] = min_x;\n'
                     b'\torigin[1] = min_y;\n'
                     b'\tc->state = origin;\n\n'
                     b'\tQueueInsert')
        block, count = extents.subn(lambda _: placement, block)
        if count != 1:
            raise ValueError(f'desktop {name.decode()} icon geometry changed')
        text = text[:begin] + block + text[end:]

    launchers = b'    \tHomeIconNew;\n    \tTelnetIconNew;\n\t\tPaletteIconNew;'
    if text.count(launchers) != 1:
        raise ValueError('expected the pinned desktop launcher initialization')
    text = text.replace(launchers,
                        b'\t\tCCtrl *home = HomeIconNew(3 * FONT_HEIGHT);\n'
                        b'\t\tCCtrl *telnet = TelnetIconNew(home->bottom + FONT_HEIGHT);\n'
                        b'\t\tPaletteIconNew(telnet->bottom + FONT_HEIGHT);', 1)
    return text + separator + sprites


def translate_wallpaper_compiler_stat(source: bytes) -> bytes:
    """Adapt the standard wallpaper's statistics and host identification."""
    source = translate_wallpaper_stats(source)
    metadata_start = b'\tCSMBIOSSystemInfo\t*sys_info = SMBIOSStructGet(SMBIOSt_SYSTEM);'
    metadata_end = b'\t// # of chars in resolution string in top bar'
    if source.count(metadata_start) != 1 or source.count(metadata_end) != 1:
        raise ValueError('expected the upstream Wallpaper platform-identification block')
    metadata_begin = source.index(metadata_start)
    metadata_finish = source.index(metadata_end, metadata_begin)
    metadata = b'''\
    // The AArch64 build has a fixed Limine UEFI boot path and no SMBIOS
    // service. Keep unavailable host and boot-device details explicit.
    wall->host_ch = '?';
    wall->bootm_ch = 'U';
    wall->bootl_ch = 'L';
    wall->boots_ch = '?';

'''
    source = source[:metadata_begin] + metadata + source[metadata_finish:]
    return source


def translate_pci_interrupts_gic_note(source: bytes) -> bytes:
    """Replace the x86 IOAPIC/LAPIC demo with an honest ARM64 capability view.

    The current guest has no guest-level GIC device-interrupt registration
    API.  Keep this source runnable and explicit about that gap rather than
    compiling x86 MMIO writes against unrelated AArch64 addresses.
    """
    required = (
        b'#assert I_USER == 0x40',
        b'U8  *da = dev.uncached_alias + IOAPIC_REG;',
        b'Main;\n',
    )
    if any(source.count(marker) != 1 for marker in required) or \
            source.count(b'LAPIC_EOI') != 4:
        raise ValueError('pinned PCIInterrupts source structure changed')
    return b'''// AArch64 adaptation of the pinned x86 PCI interrupt-routing demo.
// The pinned example programs IOAPIC redirection entries and acknowledges
// LAPIC vectors. ARM64 uses a GIC, and this guest does not yet expose a
// guest-level GIC device-interrupt registration/routing API.
U0 Main()
{
    "PCI interrupt demo: x86 IOAPIC/LAPIC routing is unavailable on AArch64.\\n"
    "The ARM64 kernel handles its GIC timer interrupt; guest device IRQ\\n"
    "registration and PCI INTx routing remain unimplemented.\\n"
    "No x86 MMIO addresses were accessed.\\n";
}

Main;
'''


def translate_multicore_interrupts_cooperative(source: bytes) -> bytes:
    """Adapt the x86 IPI demo into a real cooperative ARM64 task-ring demo.

    The host currently exposes one logical guest CPU and does not implement
    MPInt/LAPIC.  Demonstrate the supported ZealOS round-robin Yield path and
    state the SMP limitation in the program output.
    """
    required = (
        b'*(dev.uncached_alias + LAPIC_EOI)(U32 *) = 0; //signal end of interrupt',
        b'MPInt(int_on, 1);',
        b'IntTest;\n',
    )
    if any(source.count(marker) != 1 for marker in required):
        raise ValueError('pinned MultiCore/Interrupts source structure changed')
    return b'''// AArch64 adaptation of the pinned x86 multicore IPI demonstration.
// This guest currently exposes one logical CPU and no LAPIC/MPInt API.
// Show the supported cooperative ZealOS task ring: a task runs until Yield.
I64 round_robin_order[4], round_robin_count;

U0 RoundRobinTaskA(U8 *data=NULL)
{
    round_robin_order[round_robin_count++] = 'A';
    Yield;
    round_robin_order[round_robin_count++] = 'C';
}

U0 RoundRobinTaskB(U8 *data=NULL)
{
    round_robin_order[round_robin_count++] = 'B';
    Yield;
    round_robin_order[round_robin_count++] = 'D';
}

U0 IntTest()
{
    I64 i;

    round_robin_count = 0;
    Spawn(&RoundRobinTaskA, NULL, "RoundRobinA",, Fs);
    Spawn(&RoundRobinTaskB, NULL, "RoundRobinB",, Fs);
    for (i = 0; i < 4; i++)
        Yield;

    "AArch64 cooperative round-robin task demo\\n"
    "Guest logical CPUs: %d\\n", mp_count;
    "The x86 LAPIC IPI and interrupt-driven sound path are unavailable.\\n"
    "Task order after explicit Yield calls: ";
    for (i = 0; i < round_robin_count; i++)
        "%c", round_robin_order[i];
    "\\n";
}

IntTest;
'''


def translate_multicore_radix_single_cpu(source: bytes) -> bytes:
    """Keep the pinned radix-sort demo complete with one guest CPU.

    The one-CPU fallback puts the input in one bucket, then runs the same
    worker, Yield, and final bucket-copy path as a parallel run. Multi-CPU
    guests retain the stock radix distribution and per-core workers.
    """
    block = b'''\tfor (i = 0; i < my_mp_count; i++)
\t{
\t\t//$WW,0$We must do full size, just in case.
\t\t//There will be uneven split between cores
\t\t//depending on the distribution of rand numbers.
\t\tb[i] = MAlloc(NUM * sizeof(I32));
\t\tbn[i] = 0;
\t}

\tif (my_mp_count < 2)
\t\tthrow('MultCore');
'''
    if source.count(block) != 1:
        raise ValueError('pinned MPRadix single-core guard changed')
    data = source.replace(
        block,
        b'''\tif (my_mp_count < 2)
\t{
\t\t"One guest CPU: running the radix stage with one worker.\\n";
\t\tb[0] = MAlloc(NUM * sizeof(I32));
\t\tbn[0] = NUM;
\t\tMemCopy(b[0], arg1, NUM * sizeof(I32));
\t}
\telse
\t{
\t\tfor (i = 0; i < my_mp_count; i++)
\t\t{
\t\t//$WW,0$We must do full size, just in case.
\t\t//There will be uneven split between cores
\t\t//depending on the distribution of rand numbers.
\t\tb[i] = MAlloc(NUM * sizeof(I32));
\t\tbn[i] = 0;
\t\t}
\t}
''')
    distribution = b'''\tfor (i = 0; i < NUM; i++)
\t{
\t\tj = arg1[i] >> k1 + k2; //This is a preliminary radix sort.
\t\tb[j][bn[j]++] = arg1[i];
\t}
'''
    if data.count(distribution) != 1:
        raise ValueError('pinned MPRadix radix-distribution loop changed')
    return data.replace(
        distribution,
        b'''\tif (my_mp_count > 1)
\t{
\t\tfor (i = 0; i < NUM; i++)
\t\t{
\t\t\tj = arg1[i] >> k1 + k2; //This is a preliminary radix sort.
\t\t\tb[j][bn[j]++] = arg1[i];
\t\t}
\t}
''')


def translate_ring3_svc(source: bytes) -> bytes:
    """Replace the x86 ring-transition demo with an honest AArch64 SVC demo.

    The current ARM64 host does not run guest ZC in EL0, so this demonstrates
    the supported exception transition instead of pretending to switch to an
    unprivileged execution level.
    """
    required = (
        b'INT_TO_RING0::',
        b'CLI //This causes general protection fault #13',
        b'R3_CALLBACK:',
        b'Ring3Demo;\n',
    )
    if any(source.count(marker) != 1 for marker in required):
        raise ValueError('pinned Ring3 demo structure changed')
    return b'''// AArch64 adaptation of the pinned x86 Ring3 demonstration.
// This guest executes ZC through the host runtime and has no EL0 task mode.
// Demonstrate the supported synchronous exception path with an SVC and make
// the platform limitation explicit instead of attempting x86 IRET/GDT setup.
_extern _ARM64_SVC_BENCH U0 Arm64SvcBench();
_extern _ARM64_SVC_BENCH_COUNT U64 Arm64SvcBenchCount();

U0 Ring3Demo()
{
    U64 before, after;

    before = Arm64SvcBenchCount;
    "AArch64 privilege transition demo\\n"
    "EL0 user tasks are not implemented by this ARM64 guest runtime.\\n"
    "Testing the supported EL1 SVC exception and return path.\\n";
    Arm64SvcBench;
    after = Arm64SvcBenchCount;
    "SVC exception count: %d\\n", after - before;
}

Ring3Demo;
'''


def translate_multicore_loadtest_jit(source: bytes) -> bytes:
    """Run LoadTest workers on the ARM64 bridge's cooperative CPU-0 ring.

    The pinned worker calls Comp("Compiler"), which asks the x86 ZealOS AOT
    compiler to produce Compiler.ZXE. The ARM64 guest currently executes ZC
    through its native JIT and has no AOT ZXE backend. Preserve the worker's
    repeated compiler-load stress using a small stock source that can be
    compiled by the supported JIT path. The task bridge currently schedules
    only on CPU 0, so move these workers there; explicit Yield preserves
    cooperative round-robin progress and lets each worker observe app_done.
    """
    old_compile = b'\t\tComp("Compiler");'
    new_compile = (b'\t\tExeFile2("::/Demo/Print.ZC", CCF_JUST_LOAD);\n'
                   b'\t\tYield;')
    old_cpu = b'Spawn(&Job%d,,, 1);'
    if (source.count(old_compile) != 1 or
            sum(source.count(old_cpu % job) for job in (1, 2, 3)) != 3 or
            source.count(b'while (!app_done)') < 3):
        raise ValueError('pinned LoadTest compiler worker changed')
    translated = source.replace(old_compile, new_compile)
    for job in (1, 2, 3):
        translated = translated.replace(old_cpu % job,
                                        b'Spawn(&Job%d,,, 0);' % job)
    return translated


def translate_multicore_lock_single_cpu(source: bytes) -> bytes:
    """Route the stock Lock jobs onto the bridge's only cooperative CPU.

    This makes the demo runnable for single-CPU acceptance, but cannot create
    concurrent read-modify-write contention. SMP correctness remains a separate
    acceptance requirement.
    """
    unlocked = b'JobQueue(&MPUnlockedInc, NULL, 1, 0);'
    locked = b'JobQueue(&MPLockedInc, NULL, 1, 0);'
    if source.count(unlocked) != 1 or source.count(locked) != 1:
        raise ValueError('pinned Lock job calls changed')
    return source.replace(unlocked, unlocked.replace(b', 1,', b', 0,')).replace(
        locked, locked.replace(b', 1,', b', 0,'))
