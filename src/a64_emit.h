/*
 * Minimal AArch64 encoders for freestanding JIT smoke tests.
 * Shape inspired by Aiwnios c/arm64_asm.c — not a full backend.
 */
#pragma once

#include <stdint.h>

static inline uint32_t a64_ret(void) {
    return 0xD65F03C0u;
}

static inline uint32_t a64_blr(unsigned rn) {
    return 0xD63F0000u | ((rn & 31u) << 5);
}

static inline uint32_t a64_movz_x(unsigned rd, uint16_t imm16, unsigned shift) {
    unsigned hw = (shift >> 4) & 3u;
    return (1u << 31) | (2u << 29) | (0x25u << 23) | (hw << 21) |
           ((uint32_t)imm16 << 5) | (rd & 31u);
}

static inline uint32_t a64_movk_x(unsigned rd, uint16_t imm16, unsigned shift) {
    unsigned hw = (shift >> 4) & 3u;
    return (1u << 31) | (3u << 29) | (0x25u << 23) | (hw << 21) |
           ((uint32_t)imm16 << 5) | (rd & 31u);
}

static inline uint32_t a64_add_x_reg(unsigned rd, unsigned rn, unsigned rm) {
    return (1u << 31) | (0x0Bu << 24) | ((rm & 31u) << 16) | ((rn & 31u) << 5) |
           (rd & 31u);
}

static inline uint32_t a64_sub_x_reg(unsigned rd, unsigned rn, unsigned rm) {
    return (1u << 31) | (0x4Bu << 24) | ((rm & 31u) << 16) | ((rn & 31u) << 5) |
           (rd & 31u);
}

/* SUBS XZR, Xn, Xm — compare */
static inline uint32_t a64_cmp_x(unsigned rn, unsigned rm) {
    return 0xEB00001Fu | ((rm & 31u) << 16) | ((rn & 31u) << 5);
}

/* MUL Xd, Xn, Xm */
static inline uint32_t a64_mul_x(unsigned rd, unsigned rn, unsigned rm) {
    return 0x9B007C00u | ((rm & 31u) << 16) | ((rn & 31u) << 5) | (rd & 31u);
}

/* SDIV Xd, Xn, Xm */
static inline uint32_t a64_sdiv_x(unsigned rd, unsigned rn, unsigned rm) {
    return 0x9AC00C00u | ((rm & 31u) << 16) | ((rn & 31u) << 5) | (rd & 31u);
}

/* MSUB Xd, Xn, Xm, Xa  → Xd = Xa - Xn*Xm */
static inline uint32_t a64_msub_x(unsigned rd, unsigned rn, unsigned rm, unsigned ra) {
    return 0x9B008000u | ((rm & 31u) << 16) | ((ra & 31u) << 10) | ((rn & 31u) << 5) |
           (rd & 31u);
}

/* MOV Xd, Xn */
static inline uint32_t a64_mov_x(unsigned rd, unsigned rn) {
    return 0xAA0003E0u | ((rn & 31u) << 16) | (rd & 31u);
}

/* NEG Xd, Xm  (SUB Xd, XZR, Xm) */
static inline uint32_t a64_neg_x(unsigned rd, unsigned rm) {
    return a64_sub_x_reg(rd, 31, rm);
}

static inline uint32_t a64_add_x_imm(unsigned rd, unsigned rn, unsigned imm12) {
    return (1u << 31) | (0x11u << 24) | ((imm12 & 0xfffu) << 10) |
           ((rn & 31u) << 5) | (rd & 31u);
}

/* CSET Xd, EQ / NE / LT (signed) */
static inline uint32_t a64_cset_eq(unsigned rd) {
    return 0x9A9F17E0u | (rd & 31u);
}
static inline uint32_t a64_cset_ne(unsigned rd) {
    return 0x9A9F07E0u | (rd & 31u);
}
static inline uint32_t a64_cset_lt(unsigned rd) {
    return 0x9A9FA7E0u | (rd & 31u); /* CSINC XZR,XZR,GE → CSET LT */
}
static inline uint32_t a64_cset_ge(unsigned rd) {
    return 0x9A9FB7E0u | (rd & 31u);
}
static inline uint32_t a64_cset_le(unsigned rd) {
    return 0x9A9FC7E0u | (rd & 31u);
}
static inline uint32_t a64_cset_gt(unsigned rd) {
    return 0x9A9FD7E0u | (rd & 31u);
}

/* AND / ORR / EOR Xd, Xn, Xm */
static inline uint32_t a64_and_x(unsigned rd, unsigned rn, unsigned rm) {
    return 0x8A000000u | ((rm & 31u) << 16) | ((rn & 31u) << 5) | (rd & 31u);
}
static inline uint32_t a64_orr_x(unsigned rd, unsigned rn, unsigned rm) {
    return 0xAA000000u | ((rm & 31u) << 16) | ((rn & 31u) << 5) | (rd & 31u);
}
static inline uint32_t a64_eor_x(unsigned rd, unsigned rn, unsigned rm) {
    return 0xCA000000u | ((rm & 31u) << 16) | ((rn & 31u) << 5) | (rd & 31u);
}

/* LSLV / LSRV Xd, Xn, Xm */
static inline uint32_t a64_lslv_x(unsigned rd, unsigned rn, unsigned rm) {
    return 0x9AC02000u | ((rm & 31u) << 16) | ((rn & 31u) << 5) | (rd & 31u);
}
static inline uint32_t a64_lsrv_x(unsigned rd, unsigned rn, unsigned rm) {
    return 0x9AC02400u | ((rm & 31u) << 16) | ((rn & 31u) << 5) | (rd & 31u);
}

/* B imm26 (word offset, signed) */
static inline uint32_t a64_b(int32_t word_off) {
    return 0x14000000u | ((uint32_t)word_off & 0x03FFFFFFu);
}

/* CBZ / CBNZ Xt, imm19 */
static inline uint32_t a64_cbz(unsigned rt, int32_t word_off) {
    return 0xB4000000u | (((uint32_t)word_off & 0x7FFFFu) << 5) | (rt & 31u);
}
static inline uint32_t a64_cbnz(unsigned rt, int32_t word_off) {
    return 0xB5000000u | (((uint32_t)word_off & 0x7FFFFu) << 5) | (rt & 31u);
}

static inline uint32_t a64_stp_pre(unsigned rt, unsigned rt2, unsigned rn, int imm) {
    unsigned imm7 = (unsigned)((imm >> 3) & 0x7f);
    return 0xA9800000u | (imm7 << 15) | ((rt2 & 31u) << 10) | ((rn & 31u) << 5) |
           (rt & 31u);
}

static inline uint32_t a64_ldp_post(unsigned rt, unsigned rt2, unsigned rn, int imm) {
    unsigned imm7 = (unsigned)((imm >> 3) & 0x7f);
    return 0xA8C00000u | (imm7 << 15) | ((rt2 & 31u) << 10) | ((rn & 31u) << 5) |
           (rt & 31u);
}

static inline void a64_emit_imm64(uint32_t **p, unsigned rd, uint64_t imm) {
    *(*p)++ = a64_movz_x(rd, (uint16_t)(imm >> 0), 0);
    *(*p)++ = a64_movk_x(rd, (uint16_t)(imm >> 16), 16);
    *(*p)++ = a64_movk_x(rd, (uint16_t)(imm >> 32), 32);
    *(*p)++ = a64_movk_x(rd, (uint16_t)(imm >> 48), 48);
}

/* LDR Xt, [Xn] / STR Xt, [Xn]  (64-bit, imm0) */
static inline uint32_t a64_ldr_x(unsigned rt, unsigned rn) {
    return 0xF9400000u | ((rn & 31u) << 5) | (rt & 31u);
}
static inline uint32_t a64_str_x(unsigned rt, unsigned rn) {
    return 0xF9000000u | ((rn & 31u) << 5) | (rt & 31u);
}
/* LDRB Wt, [Xn] / STRB Wt, [Xn] */
static inline uint32_t a64_ldrb(unsigned rt, unsigned rn) {
    return 0x39400000u | ((rn & 31u) << 5) | (rt & 31u);
}
static inline uint32_t a64_strb(unsigned rt, unsigned rn) {
    return 0x39000000u | ((rn & 31u) << 5) | (rt & 31u);
}
/* LDR Wt, [Xn] zero-extends to Xt / STR Wt, [Xn] */
static inline uint32_t a64_ldr_w(unsigned rt, unsigned rn) {
    return 0xB9400000u | ((rn & 31u) << 5) | (rt & 31u);
}
static inline uint32_t a64_str_w(unsigned rt, unsigned rn) {
    return 0xB9000000u | ((rn & 31u) << 5) | (rt & 31u);
}
/* ADD Xd, Xn, Xm, LSL #imm6 */
static inline uint32_t a64_add_x_lsl(unsigned rd, unsigned rn, unsigned rm,
                                     unsigned imm6) {
    return (1u << 31) | (0x0Bu << 24) | ((rm & 31u) << 16) | ((imm6 & 63u) << 10) |
           ((rn & 31u) << 5) | (rd & 31u);
}

/* ---- Aiwnios arm64_asm.c–aligned extras (freestanding slice) ---- */

/* BL imm26 (PC-relative, word offset from this insn). */
static inline uint32_t a64_bl(int32_t word_off) {
    return 0x94000000u | ((uint32_t)word_off & 0x03FFFFFFu);
}

/* SUB Xd, Xn, #imm12 */
static inline uint32_t a64_sub_x_imm(unsigned rd, unsigned rn, unsigned imm12) {
    return (1u << 31) | (0x51u << 24) | ((imm12 & 0xfffu) << 10) |
           ((rn & 31u) << 5) | (rd & 31u);
}

/* LDR/STR Xt, [Xn, #imm12*8]  (unsigned offset, scale 8) */
static inline uint32_t a64_ldr_x_imm(unsigned rt, unsigned rn, unsigned imm12) {
    return 0xF9400000u | ((imm12 & 0xfffu) << 10) | ((rn & 31u) << 5) | (rt & 31u);
}
static inline uint32_t a64_str_x_imm(unsigned rt, unsigned rn, unsigned imm12) {
    return 0xF9000000u | ((imm12 & 0xfffu) << 10) | ((rn & 31u) << 5) | (rt & 31u);
}

/* ASRV Xd, Xn, Xm — arithmetic shift right by register */
static inline uint32_t a64_asrv_x(unsigned rd, unsigned rn, unsigned rm) {
    return 0x9AC02800u | ((rm & 31u) << 16) | ((rn & 31u) << 5) | (rd & 31u);
}

/* ---- F64 (Aiwnios arm64_asm Fp* encodings; type=double) ---- */
static inline uint32_t a64_fp_dataproc2(unsigned opcode, unsigned rd, unsigned rn,
                                        unsigned rm) {
    return (0xfu << 25) | (1u << 22) | (1u << 21) | ((rm & 31u) << 16) |
           ((opcode & 15u) << 12) | (2u << 10) | ((rn & 31u) << 5) | (rd & 31u);
}
static inline uint32_t a64_fadd_d(unsigned rd, unsigned rn, unsigned rm) {
    return a64_fp_dataproc2(2, rd, rn, rm);
}
static inline uint32_t a64_fsub_d(unsigned rd, unsigned rn, unsigned rm) {
    return a64_fp_dataproc2(3, rd, rn, rm);
}
static inline uint32_t a64_fmul_d(unsigned rd, unsigned rn, unsigned rm) {
    return a64_fp_dataproc2(0, rd, rn, rm);
}
static inline uint32_t a64_fdiv_d(unsigned rd, unsigned rn, unsigned rm) {
    return a64_fp_dataproc2(1, rd, rn, rm);
}
static inline uint32_t a64_fneg_d(unsigned rd, unsigned rn) {
    /* FpDataProc1Reg opcode=2 (fneg), type=double — Aiwnios ARM_fnegReg */
    return (0xfu << 25) | (1u << 22) | (1u << 21) | (2u << 15) | (1u << 14) |
           ((rn & 31u) << 5) | (rd & 31u);
}
static inline uint32_t a64_fcmp_d(unsigned rn, unsigned rm) {
    return (0xfu << 25) | (1u << 22) | (1u << 21) | ((rm & 31u) << 16) | (1u << 13) |
           ((rn & 31u) << 5);
}
/* FMOV Dd, Xn  /  FMOV Xd, Dn */
static inline uint32_t a64_fmov_d_x(unsigned dd, unsigned xn) {
    return (1u << 31) | (0xfu << 25) | (1u << 22) | (1u << 21) | (7u << 16) |
           ((xn & 31u) << 5) | (dd & 31u);
}
static inline uint32_t a64_fmov_x_d(unsigned xd, unsigned dn) {
    return (1u << 31) | (0xfu << 25) | (1u << 22) | (1u << 21) | (6u << 16) |
           ((dn & 31u) << 5) | (xd & 31u);
}
/* SCVTF Dd, Xn  /  FCVTZS Xd, Dn */
static inline uint32_t a64_scvtf_d_x(unsigned dd, unsigned xn) {
    return (1u << 31) | (0xfu << 25) | (1u << 22) | (1u << 21) | (2u << 16) |
           ((xn & 31u) << 5) | (dd & 31u);
}
static inline uint32_t a64_fcvtzs_x_d(unsigned xd, unsigned dn) {
    return (1u << 31) | (0xfu << 25) | (1u << 22) | (1u << 21) | (3u << 19) |
           ((dn & 31u) << 5) | (xd & 31u);
}

/* CSEL Xd, Xn, Xm, EQ */
static inline uint32_t a64_csel_eq_x(unsigned rd, unsigned rn, unsigned rm) {
    return 0x9A800000u | ((rm & 31u) << 16) | ((rn & 31u) << 5) | (rd & 31u);
}
