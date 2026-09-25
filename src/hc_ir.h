/*
 * Tiny HolyC-shaped stack IR → aarch64 (Aiwnios IC_* lineage, freestanding).
 *
 * Supports arithmetic, locals (x20–x23), labels/branches, and host calls.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "a64_emit.h"

enum {
    HC_END = 0,
    HC_IMM64 = 1,
    HC_ADD = 2,
    HC_SUB = 3,
    HC_MUL = 4,
    HC_CALL0 = 5,
    HC_RET = 6,
    HC_EQ = 7,       /* like IC_EQ — push a==b */
    HC_LT = 8,       /* signed < */
    HC_NEG = 9,      /* unary - */
    HC_DUP = 10,
    HC_DROP = 11,
    HC_LABEL = 12,   /* u8 id 0..15 */
    HC_JMP = 13,     /* u8 id */
    HC_JZ = 14,      /* u8 id — pop, branch if zero */
    HC_JNZ = 15,     /* u8 id */
    HC_ST_LOCAL = 16, /* u8 slot 0..3 — pop */
    HC_LD_LOCAL = 17, /* u8 slot — push */
    HC_SLOT_ADDR = 18, /* u8 slot — push &g_hc_mem[slot] */
    HC_LOAD = 19,      /* pop ptr, push *(u64*)ptr */
    HC_STORE = 20,     /* pop val, pop ptr, *ptr = val */
    HC_AND = 21,
    HC_OR = 22,
    HC_XOR = 23,
    HC_SHL = 24,
    HC_SHR = 25,       /* logical */
    HC_NE = 26,
    HC_LE = 27,
    HC_GT = 28,
    HC_GE = 29,
    HC_CALL1 = 30,     /* u64 fn; pop arg→x0; call; push x0 */
    HC_CALL2 = 31,     /* u64 fn; pop→x1, pop→x0; call; push x0 */
    HC_LD_SLOT = 32,   /* u8 slot — push g_hc_mem[slot] */
    HC_ST_SLOT = 33,   /* u8 slot — pop → g_hc_mem[slot] */
    HC_INARG = 34,     /* u8 slot, u8 regno (0..6) — xN → g_hc_mem[slot] */
    HC_LOAD8 = 35,     /* pop ptr, push *(u8*)ptr zero-ext */
    HC_STORE8 = 36,    /* pop val, pop ptr, *(u8*)ptr = val */
    HC_DIV = 37,       /* signed / */
    HC_MOD = 38,       /* signed % */
    HC_ASR = 39,       /* arithmetic >> */
    HC_CALL3 = 40,     /* u64 fn; pop→x2,x1,x0; call; push x0 */
    HC_CALL4 = 41,     /* pop→x3..x0 */
    HC_CALL5 = 42,     /* pop→x4..x0 */
    HC_LOAD32 = 43,    /* pop ptr, push *(u32*)ptr zero-ext */
    HC_STORE32 = 44,   /* pop val, pop ptr, *(u32*)ptr = val */
    HC_ENTER = 45,     /* u8 nwords — push frame; fp = alloc; alloc += n */
    HC_LEAVE = 46,     /* pop frame */
    HC_CALL6 = 47,     /* pop→x5..x0 */
    HC_LD_ABS = 48,    /* u8 slot — push g_hc_mem[slot] (file-scope global) */
    HC_ST_ABS = 49,    /* u8 slot — pop → g_hc_mem[slot] */
    HC_ABS_ADDR = 50,  /* u8 slot — push &g_hc_mem[slot] (no fp) */
    HC_I2F = 51,       /* pop i64 → push f64 bits (SCVTF) */
    HC_F2I = 52,       /* pop f64 bits → push i64 (FCVTZS) */
    HC_FADD = 53,
    HC_FSUB = 54,
    HC_FMUL = 55,
    HC_FDIV = 56,
    HC_FNEG = 57,      /* unary */
    HC_FEQ = 58,       /* pop b,a → push (a==b) as i64 */
    HC_FLT = 59,       /* signed < */
    HC_FLE = 60,
    HC_FGT = 61,
    HC_FGE = 62,
    HC_FNE = 63,       /* != */
    HC_CALL7 = 64,     /* pop→x6..x0 (GrLine3) */
    HC_SWAP = 65,      /* swap top two stack slots (I64↔F64 promote) */
};

#define HC_IR_MAX_DEPTH  16
#define HC_IR_MAX_LABELS 128 /* was 64 — Lattice nested key/msg switches */
#define HC_IR_MAX_LOCALS 4   /* register locals x20–x23 (ST_LOCAL/LD_LOCAL) */
#define HC_IR_MEM_WORDS  256 /* was 128 — Lattice MenuPush strings+angles leave <16 words for DrawIt frames */
#define HC_IR_MAX_NAMES  24  /* was 8 — upstream demos need more locals */
#define HC_IR_MAX_GLOBS  48  /* was 32 — Lattice angles[35] */
#define HC_IR_ERR_DEPTH  (-1)
#define HC_IR_ERR_OP     (-2)
#define HC_IR_ERR_SPACE  (-3)
#define HC_IR_ERR_TRUNC  (-4)
#define HC_IR_ERR_LABEL  (-5)
#define HC_IR_ERR_LOCAL  (-6)

/* Memory-backed slots for pointer/array IR (defined by kernel). */
extern uint64_t g_hc_mem[HC_IR_MEM_WORDS];
extern uint64_t g_hc_fp;    /* current frame base (word index) */
extern uint64_t g_hc_alloc; /* bump allocator (word index) */
extern uint64_t g_hc_glob_words; /* file-scope globals occupy [0, g_hc_glob_words) */
enum { HC_PATCH_B = 1, HC_PATCH_CBZ = 2, HC_PATCH_CBNZ = 3 };

struct hc_patch {
    uint8_t kind;
    uint8_t label;
    uint32_t *site;
};

static inline unsigned hc_local_reg(uint8_t slot) {
    return 20u + slot; /* x20..x23 */
}

/* Emit xRd = &g_hc_mem[g_hc_fp + slot] (clobbers x17; rd must not be 17). */
static inline int hc_emit_slot_addr(uint32_t **pp, uint32_t *pend, unsigned rd,
                                    uint8_t slot) {
    uint32_t *p = *pp;
    if (p + 12 > pend || rd == 17) {
        return -1;
    }
    a64_emit_imm64(&p, 17, (uint64_t)(uintptr_t)&g_hc_fp);
    *p++ = a64_ldr_x(17, 17);
    if (slot) {
        *p++ = a64_add_x_imm(17, 17, slot);
    }
    a64_emit_imm64(&p, rd, (uint64_t)(uintptr_t)&g_hc_mem[0]);
    *p++ = a64_add_x_lsl(rd, rd, 17, 3);
    *pp = p;
    return 0;
}

/* Emit xRd = &g_hc_mem[slot] (absolute; file-scope globals). */
static inline int hc_emit_abs_addr(uint32_t **pp, uint32_t *pend, unsigned rd,
                                   uint8_t slot) {
    uint32_t *p = *pp;
    if (p + 8 > pend) {
        return -1;
    }
    a64_emit_imm64(&p, rd, (uint64_t)(uintptr_t)&g_hc_mem[slot]);
    *pp = p;
    return 0;
}

static inline int hc_emit_enter(uint32_t **pp, uint32_t *pend, uint8_t nwords) {
    uint32_t *p = *pp;
    if (p + 16 > pend) {
        return -1;
    }
    /* push old fp; fp = alloc; alloc += nwords */
    a64_emit_imm64(&p, 16, (uint64_t)(uintptr_t)&g_hc_fp);
    *p++ = a64_ldr_x(17, 16);
    *p++ = a64_stp_pre(17, 31, 31, -16);
    a64_emit_imm64(&p, 18, (uint64_t)(uintptr_t)&g_hc_alloc);
    *p++ = a64_ldr_x(17, 18);
    *p++ = a64_str_x(17, 16);
    if (nwords) {
        *p++ = a64_add_x_imm(17, 17, nwords);
    }
    *p++ = a64_str_x(17, 18);
    *pp = p;
    return 0;
}

static inline int hc_emit_leave(uint32_t **pp, uint32_t *pend) {
    uint32_t *p = *pp;
    if (p + 12 > pend) {
        return -1;
    }
    /* alloc = fp; fp = pop */
    a64_emit_imm64(&p, 16, (uint64_t)(uintptr_t)&g_hc_fp);
    a64_emit_imm64(&p, 18, (uint64_t)(uintptr_t)&g_hc_alloc);
    *p++ = a64_ldr_x(17, 16);
    *p++ = a64_str_x(17, 18);
    *p++ = a64_ldp_post(17, 31, 31, 16);
    *p++ = a64_str_x(17, 16);
    *pp = p;
    return 0;
}

static inline int hc_ir_compile(const uint8_t *bc, size_t bc_len,
                                uint32_t *out, size_t out_cap_insns) {
    uint32_t *p = out;
    uint32_t *pend = out + out_cap_insns;
    size_t i = 0;
    int sp = 0;
    int framed = 0;
    int64_t label_at[HC_IR_MAX_LABELS];
    struct hc_patch patches[256]; /* was 64 — Lattice nested switches + if */
    int npatch = 0;

    for (int L = 0; L < HC_IR_MAX_LABELS; L++) {
        label_at[L] = -1;
    }

    if (p + 3 > pend) {
        return HC_IR_ERR_SPACE;
    }
    /* Callee-saved: locals live in x20–x23; must preserve across JIT return. */
    *p++ = a64_stp_pre(20, 21, 31, -16);
    *p++ = a64_stp_pre(22, 23, 31, -16);
    *p++ = a64_stp_pre(30, 31, 31, -16);

#define EMIT(insn)                                                             \
    do {                                                                       \
        if (p >= pend)                                                         \
            return HC_IR_ERR_SPACE;                                            \
        *p++ = (insn);                                                         \
    } while (0)

#define EMIT_BR(kind_, lab_expr)                                               \
    do {                                                                       \
        uint8_t _lab = (uint8_t)(lab_expr);                                    \
        if (p >= pend)                                                         \
            return HC_IR_ERR_SPACE;                                            \
        if (_lab >= HC_IR_MAX_LABELS)                                          \
            return HC_IR_ERR_LABEL;                                            \
        if (npatch >= (int)(sizeof(patches) / sizeof(patches[0])))             \
            return HC_IR_ERR_SPACE;                                            \
        patches[npatch].kind = (uint8_t)(kind_);                               \
        patches[npatch].label = _lab;                                          \
        patches[npatch].site = p;                                              \
        npatch++;                                                              \
        *p++ = 0x14000000u;                                                    \
    } while (0)

    while (i < bc_len) {
        uint8_t op = bc[i++];
        if (op == HC_END) {
            break;
        }
        switch (op) {
        case HC_IMM64: {
            if (i + 8 > bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            if (sp >= HC_IR_MAX_DEPTH) {
                return HC_IR_ERR_DEPTH;
            }
            uint64_t imm = 0;
            for (int b = 0; b < 8; b++) {
                imm |= (uint64_t)bc[i++] << (8 * b);
            }
            if (p + 4 > pend) {
                return HC_IR_ERR_SPACE;
            }
            a64_emit_imm64(&p, 9u + (unsigned)sp, imm);
            sp++;
            break;
        }
        case HC_ADD:
        case HC_SUB:
        case HC_MUL:
        case HC_DIV:
        case HC_MOD:
        case HC_AND:
        case HC_OR:
        case HC_XOR:
        case HC_SHL:
        case HC_SHR:
        case HC_ASR: {
            if (sp < 2) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned rb = 9u + (unsigned)(sp - 1);
            unsigned ra = 9u + (unsigned)(sp - 2);
            if (op == HC_ADD) {
                EMIT(a64_add_x_reg(ra, ra, rb));
            } else if (op == HC_SUB) {
                EMIT(a64_sub_x_reg(ra, ra, rb));
            } else if (op == HC_MUL) {
                EMIT(a64_mul_x(ra, ra, rb));
            } else if (op == HC_DIV) {
                EMIT(a64_sdiv_x(ra, ra, rb));
            } else if (op == HC_MOD) {
                EMIT(a64_sdiv_x(8, ra, rb));
                EMIT(a64_msub_x(ra, 8, rb, ra));
            } else if (op == HC_AND) {
                EMIT(a64_and_x(ra, ra, rb));
            } else if (op == HC_OR) {
                EMIT(a64_orr_x(ra, ra, rb));
            } else if (op == HC_XOR) {
                EMIT(a64_eor_x(ra, ra, rb));
            } else if (op == HC_SHL) {
                EMIT(a64_lslv_x(ra, ra, rb));
            } else if (op == HC_ASR) {
                EMIT(a64_asrv_x(ra, ra, rb));
            } else {
                EMIT(a64_lsrv_x(ra, ra, rb));
            }
            sp--;
            break;
        }
        case HC_EQ:
        case HC_NE:
        case HC_LT:
        case HC_LE:
        case HC_GT:
        case HC_GE: {
            if (sp < 2) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned rb = 9u + (unsigned)(sp - 1);
            unsigned ra = 9u + (unsigned)(sp - 2);
            EMIT(a64_cmp_x(ra, rb));
            if (op == HC_EQ) {
                EMIT(a64_cset_eq(ra));
            } else if (op == HC_NE) {
                EMIT(a64_cset_ne(ra));
            } else if (op == HC_LT) {
                EMIT(a64_cset_lt(ra));
            } else if (op == HC_LE) {
                EMIT(a64_cset_le(ra));
            } else if (op == HC_GT) {
                EMIT(a64_cset_gt(ra));
            } else {
                EMIT(a64_cset_ge(ra));
            }
            sp--;
            break;
        }
        case HC_NEG: {
            if (sp < 1) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned r = 9u + (unsigned)(sp - 1);
            EMIT(a64_neg_x(r, r));
            break;
        }
        case HC_DUP: {
            if (sp < 1 || sp >= HC_IR_MAX_DEPTH) {
                return HC_IR_ERR_DEPTH;
            }
            EMIT(a64_mov_x(9u + (unsigned)sp, 9u + (unsigned)(sp - 1)));
            sp++;
            break;
        }
        case HC_SWAP: {
            if (sp < 2) {
                return HC_IR_ERR_DEPTH;
            }
            EMIT(a64_mov_x(16, 9u + (unsigned)(sp - 2)));
            EMIT(a64_mov_x(9u + (unsigned)(sp - 2), 9u + (unsigned)(sp - 1)));
            EMIT(a64_mov_x(9u + (unsigned)(sp - 1), 16));
            break;
        }
        case HC_DROP: {
            if (sp < 1) {
                return HC_IR_ERR_DEPTH;
            }
            sp--;
            break;
        }
        case HC_LABEL: {
            if (i >= bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            uint8_t id = bc[i++];
            if (id >= HC_IR_MAX_LABELS || label_at[id] >= 0) {
                return HC_IR_ERR_LABEL;
            }
            label_at[id] = (int64_t)(p - out);
            break;
        }
        case HC_JMP: {
            if (i >= bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            EMIT_BR(HC_PATCH_B, bc[i++]);
            break;
        }
        case HC_JZ:
        case HC_JNZ: {
            if (i >= bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            if (sp < 1) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned r = 9u + (unsigned)(sp - 1);
            sp--;
            /* move into x16 so CBZ target reg is stable */
            EMIT(a64_mov_x(16, r));
            EMIT_BR(op == HC_JZ ? HC_PATCH_CBZ : HC_PATCH_CBNZ, bc[i++]);
            /* patch site is the branch; rewrite placeholder as cbz x16 */
            patches[npatch - 1].site[0] =
                (op == HC_JZ) ? a64_cbz(16, 0) : a64_cbnz(16, 0);
            break;
        }
        case HC_ST_LOCAL: {
            if (i >= bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            uint8_t slot = bc[i++];
            if (slot >= HC_IR_MAX_LOCALS || sp < 1) {
                return HC_IR_ERR_LOCAL;
            }
            EMIT(a64_mov_x(hc_local_reg(slot), 9u + (unsigned)(sp - 1)));
            sp--;
            break;
        }
        case HC_LD_LOCAL: {
            if (i >= bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            uint8_t slot = bc[i++];
            if (slot >= HC_IR_MAX_LOCALS || sp >= HC_IR_MAX_DEPTH) {
                return HC_IR_ERR_LOCAL;
            }
            EMIT(a64_mov_x(9u + (unsigned)sp, hc_local_reg(slot)));
            sp++;
            break;
        }
        case HC_SLOT_ADDR: {
            if (i >= bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            uint8_t slot = bc[i++];
            if (slot >= HC_IR_MEM_WORDS || sp >= HC_IR_MAX_DEPTH) {
                return HC_IR_ERR_LOCAL;
            }
            if (hc_emit_slot_addr(&p, pend, 9u + (unsigned)sp, slot) < 0) {
                return HC_IR_ERR_SPACE;
            }
            sp++;
            break;
        }
        case HC_LOAD: {
            if (sp < 1) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned r = 9u + (unsigned)(sp - 1);
            EMIT(a64_ldr_x(r, r));
            break;
        }
        case HC_STORE: {
            if (sp < 2) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned rval = 9u + (unsigned)(sp - 1);
            unsigned rptr = 9u + (unsigned)(sp - 2);
            EMIT(a64_str_x(rval, rptr));
            sp -= 2;
            break;
        }
        case HC_LOAD8: {
            if (sp < 1) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned r = 9u + (unsigned)(sp - 1);
            EMIT(a64_ldrb(r, r));
            break;
        }
        case HC_STORE8: {
            if (sp < 2) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned rval = 9u + (unsigned)(sp - 1);
            unsigned rptr = 9u + (unsigned)(sp - 2);
            EMIT(a64_strb(rval, rptr));
            sp -= 2;
            break;
        }
        case HC_LOAD32: {
            if (sp < 1) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned r = 9u + (unsigned)(sp - 1);
            EMIT(a64_ldr_w(r, r));
            break;
        }
        case HC_STORE32: {
            if (sp < 2) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned rval = 9u + (unsigned)(sp - 1);
            unsigned rptr = 9u + (unsigned)(sp - 2);
            EMIT(a64_str_w(rval, rptr));
            sp -= 2;
            break;
        }
        case HC_ENTER: {
            if (i >= bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            uint8_t nwords = bc[i++];
            if (hc_emit_enter(&p, pend, nwords) < 0) {
                return HC_IR_ERR_SPACE;
            }
            framed = 1;
            break;
        }
        case HC_LEAVE: {
            if (hc_emit_leave(&p, pend) < 0) {
                return HC_IR_ERR_SPACE;
            }
            framed = 0;
            break;
        }
        case HC_CALL0:
        case HC_CALL1:
        case HC_CALL2:
        case HC_CALL3:
        case HC_CALL4:
        case HC_CALL5:
        case HC_CALL6:
        case HC_CALL7: {
            if (i + 8 > bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            uint64_t fn = 0;
            for (int b = 0; b < 8; b++) {
                fn |= (uint64_t)bc[i++] << (8 * b);
            }
            int argc = 0;
            if (op == HC_CALL7) {
                argc = 7;
            } else if (op == HC_CALL6) {
                argc = 6;
            } else if (op == HC_CALL5) {
                argc = 5;
            } else if (op == HC_CALL4) {
                argc = 4;
            } else if (op == HC_CALL3) {
                argc = 3;
            } else if (op == HC_CALL2) {
                argc = 2;
            } else if (op == HC_CALL1) {
                argc = 1;
            }
            if (sp < argc) {
                return HC_IR_ERR_DEPTH;
            }
            for (int a = 0; a < argc; a++) {
                EMIT(a64_mov_x((unsigned)(argc - 1 - a),
                               9u + (unsigned)(sp - 1 - a)));
            }
            sp -= argc;
            int need = sp;
            /* Save IR stack (low→high) so restore order matches. */
            for (int s = 0; s < need; s++) {
                EMIT(a64_stp_pre(9u + (unsigned)s, 31, 31, -16));
            }
            EMIT(a64_stp_pre(20, 21, 31, -16));
            EMIT(a64_stp_pre(22, 23, 31, -16));
            if (p + 4 > pend) {
                return HC_IR_ERR_SPACE;
            }
            a64_emit_imm64(&p, 16, fn);
            EMIT(a64_blr(16));
            EMIT(a64_ldp_post(22, 23, 31, 16));
            EMIT(a64_ldp_post(20, 21, 31, 16));
            if (sp >= HC_IR_MAX_DEPTH) {
                return HC_IR_ERR_DEPTH;
            }
            /* Pop high→low (reverse of push). */
            for (int s = need - 1; s >= 0; s--) {
                EMIT(a64_ldp_post(9u + (unsigned)s, 31, 31, 16));
            }
            EMIT(a64_mov_x(9u + (unsigned)sp, 0));
            sp++;
            break;
        }
        case HC_LD_SLOT: {
            if (i >= bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            uint8_t slot = bc[i++];
            if (slot >= HC_IR_MEM_WORDS || sp >= HC_IR_MAX_DEPTH) {
                return HC_IR_ERR_LOCAL;
            }
            unsigned rd = 9u + (unsigned)sp;
            if (hc_emit_slot_addr(&p, pend, rd, slot) < 0) {
                return HC_IR_ERR_SPACE;
            }
            EMIT(a64_ldr_x(rd, rd));
            sp++;
            break;
        }
        case HC_ST_SLOT: {
            if (i >= bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            uint8_t slot = bc[i++];
            if (slot >= HC_IR_MEM_WORDS || sp < 1) {
                return HC_IR_ERR_LOCAL;
            }
            unsigned rv = 9u + (unsigned)(sp - 1);
            if (hc_emit_slot_addr(&p, pend, 16, slot) < 0) {
                return HC_IR_ERR_SPACE;
            }
            EMIT(a64_str_x(rv, 16));
            sp--;
            break;
        }
        case HC_LD_ABS: {
            if (i >= bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            uint8_t slot = bc[i++];
            if (slot >= HC_IR_MEM_WORDS || sp >= HC_IR_MAX_DEPTH) {
                return HC_IR_ERR_LOCAL;
            }
            unsigned rd = 9u + (unsigned)sp;
            if (hc_emit_abs_addr(&p, pend, rd, slot) < 0) {
                return HC_IR_ERR_SPACE;
            }
            EMIT(a64_ldr_x(rd, rd));
            sp++;
            break;
        }
        case HC_ST_ABS: {
            if (i >= bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            uint8_t slot = bc[i++];
            if (slot >= HC_IR_MEM_WORDS || sp < 1) {
                return HC_IR_ERR_LOCAL;
            }
            unsigned rv = 9u + (unsigned)(sp - 1);
            if (hc_emit_abs_addr(&p, pend, 16, slot) < 0) {
                return HC_IR_ERR_SPACE;
            }
            EMIT(a64_str_x(rv, 16));
            sp--;
            break;
        }
        case HC_ABS_ADDR: {
            if (i >= bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            uint8_t slot = bc[i++];
            if (slot >= HC_IR_MEM_WORDS || sp >= HC_IR_MAX_DEPTH) {
                return HC_IR_ERR_LOCAL;
            }
            if (hc_emit_abs_addr(&p, pend, 9u + (unsigned)sp, slot) < 0) {
                return HC_IR_ERR_SPACE;
            }
            sp++;
            break;
        }
        case HC_I2F: {
            if (sp < 1) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned r = 9u + (unsigned)(sp - 1);
            EMIT(a64_scvtf_d_x(0, r));
            EMIT(a64_fmov_x_d(r, 0));
            break;
        }
        case HC_F2I: {
            if (sp < 1) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned r = 9u + (unsigned)(sp - 1);
            EMIT(a64_fmov_d_x(0, r));
            EMIT(a64_fcvtzs_x_d(r, 0));
            break;
        }
        case HC_FNEG: {
            if (sp < 1) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned r = 9u + (unsigned)(sp - 1);
            EMIT(a64_fmov_d_x(0, r));
            EMIT(a64_fneg_d(0, 0));
            EMIT(a64_fmov_x_d(r, 0));
            break;
        }
        case HC_FADD:
        case HC_FSUB:
        case HC_FMUL:
        case HC_FDIV: {
            if (sp < 2) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned rb = 9u + (unsigned)(sp - 1);
            unsigned ra = 9u + (unsigned)(sp - 2);
            EMIT(a64_fmov_d_x(0, ra));
            EMIT(a64_fmov_d_x(1, rb));
            if (op == HC_FADD) {
                EMIT(a64_fadd_d(0, 0, 1));
            } else if (op == HC_FSUB) {
                EMIT(a64_fsub_d(0, 0, 1));
            } else if (op == HC_FMUL) {
                EMIT(a64_fmul_d(0, 0, 1));
            } else {
                EMIT(a64_fdiv_d(0, 0, 1));
            }
            EMIT(a64_fmov_x_d(ra, 0));
            sp--;
            break;
        }
        case HC_FEQ:
        case HC_FLT:
        case HC_FLE:
        case HC_FGT:
        case HC_FGE:
        case HC_FNE: {
            if (sp < 2) {
                return HC_IR_ERR_DEPTH;
            }
            unsigned rb = 9u + (unsigned)(sp - 1);
            unsigned ra = 9u + (unsigned)(sp - 2);
            EMIT(a64_fmov_d_x(0, ra));
            EMIT(a64_fmov_d_x(1, rb));
            EMIT(a64_fcmp_d(0, 1));
            if (op == HC_FEQ) {
                EMIT(a64_cset_eq(ra));
            } else if (op == HC_FLT) {
                EMIT(a64_cset_lt(ra));
            } else if (op == HC_FLE) {
                EMIT(a64_cset_le(ra));
            } else if (op == HC_FGT) {
                EMIT(a64_cset_gt(ra));
            } else if (op == HC_FGE) {
                EMIT(a64_cset_ge(ra));
            } else {
                EMIT(a64_cset_ne(ra));
            }
            sp--;
            break;
        }
        case HC_INARG: {
            if (i + 2 > bc_len) {
                return HC_IR_ERR_TRUNC;
            }
            uint8_t slot = bc[i++];
            uint8_t regno = bc[i++];
            if (slot >= HC_IR_MEM_WORDS || regno > 6) {
                return HC_IR_ERR_LOCAL;
            }
            if (hc_emit_slot_addr(&p, pend, 16, slot) < 0) {
                return HC_IR_ERR_SPACE;
            }
            EMIT(a64_str_x(regno, 16));
            break;
        }
        case HC_RET: {
            if (framed) {
                if (hc_emit_leave(&p, pend) < 0) {
                    return HC_IR_ERR_SPACE;
                }
            }
            if (sp < 1) {
                EMIT(a64_movz_x(0, 0, 0));
            } else {
                EMIT(a64_mov_x(0, 9u + (unsigned)(sp - 1)));
            }
            EMIT(a64_ldp_post(30, 31, 31, 16));
            EMIT(a64_ldp_post(22, 23, 31, 16));
            EMIT(a64_ldp_post(20, 21, 31, 16));
            EMIT(a64_ret());
            /* Keep scanning so labels after RET (branch targets) are recorded. */
            sp = 0;
            break;
        }
        default:
            return HC_IR_ERR_OP;
        }
    }
    if (framed) {
        if (hc_emit_leave(&p, pend) < 0) {
            return HC_IR_ERR_SPACE;
        }
    }
    EMIT(a64_movz_x(0, 0, 0));
    EMIT(a64_ldp_post(30, 31, 31, 16));
    EMIT(a64_ldp_post(22, 23, 31, 16));
    EMIT(a64_ldp_post(20, 21, 31, 16));
    EMIT(a64_ret());    for (int k = 0; k < npatch; k++) {
        int64_t target = label_at[patches[k].label];
        if (target < 0) {
            return HC_IR_ERR_LABEL;
        }
        int32_t off = (int32_t)(target - (patches[k].site - out));
        switch (patches[k].kind) {
        case HC_PATCH_B:
            patches[k].site[0] = a64_b(off);
            break;
        case HC_PATCH_CBZ:
            patches[k].site[0] = a64_cbz(16, off);
            break;
        case HC_PATCH_CBNZ:
            patches[k].site[0] = a64_cbnz(16, off);
            break;
        default:
            return HC_IR_ERR_OP;
        }
    }
    return (int)((p - out) * 4);
#undef EMIT
#undef EMIT_BR
}

static inline size_t hc_pack_imm64(uint8_t *dst, uint64_t v) {
    dst[0] = HC_IMM64;
    for (int b = 0; b < 8; b++) {
        dst[1 + b] = (uint8_t)(v >> (8 * b));
    }
    return 9;
}

static inline size_t hc_pack_call0(uint8_t *dst, uint64_t fn) {
    dst[0] = HC_CALL0;
    for (int b = 0; b < 8; b++) {
        dst[1 + b] = (uint8_t)(fn >> (8 * b));
    }
    return 9;
}

static inline size_t hc_pack_call1(uint8_t *dst, uint64_t fn) {
    dst[0] = HC_CALL1;
    for (int b = 0; b < 8; b++) {
        dst[1 + b] = (uint8_t)(fn >> (8 * b));
    }
    return 9;
}

static inline size_t hc_pack_call2(uint8_t *dst, uint64_t fn) {
    dst[0] = HC_CALL2;
    for (int b = 0; b < 8; b++) {
        dst[1 + b] = (uint8_t)(fn >> (8 * b));
    }
    return 9;
}

static inline size_t hc_pack_call3(uint8_t *dst, uint64_t fn) {
    dst[0] = HC_CALL3;
    for (int b = 0; b < 8; b++) {
        dst[1 + b] = (uint8_t)(fn >> (8 * b));
    }
    return 9;
}

static inline size_t hc_pack_call4(uint8_t *dst, uint64_t fn) {
    dst[0] = HC_CALL4;
    for (int b = 0; b < 8; b++) {
        dst[1 + b] = (uint8_t)(fn >> (8 * b));
    }
    return 9;
}

static inline size_t hc_pack_call5(uint8_t *dst, uint64_t fn) {
    dst[0] = HC_CALL5;
    for (int b = 0; b < 8; b++) {
        dst[1 + b] = (uint8_t)(fn >> (8 * b));
    }
    return 9;
}

static inline size_t hc_pack_call6(uint8_t *dst, uint64_t fn) {
    dst[0] = HC_CALL6;
    for (int b = 0; b < 8; b++) {
        dst[1 + b] = (uint8_t)(fn >> (8 * b));
    }
    return 9;
}

static inline size_t hc_pack_call7(uint8_t *dst, uint64_t fn) {
    dst[0] = HC_CALL7;
    for (int b = 0; b < 8; b++) {
        dst[1 + b] = (uint8_t)(fn >> (8 * b));
    }
    return 9;
}

static inline size_t hc_pack_u8op(uint8_t *dst, uint8_t op, uint8_t a) {
    dst[0] = op;
    dst[1] = a;
    return 2;
}
