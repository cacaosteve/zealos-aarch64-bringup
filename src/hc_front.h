/*
 * Minimal HolyC subset → stack IR.
 *
 *   I64/U32/Bool/U0, if/else, while, do/while, for, switch/case/default, break, continue,
 *   return, &name, *expr, I64/U8 a[N], "str"[i], class / . / -> / Class*
 *   enum { A=1, B, C };  (optional tag name; auto-increment)
 *   I64 Foo(I64 a, I64 b){...} before use (≤8 fns, ≤5 args)
 *   File-scope I64/U32/U8/Bool/F64 globals (+ arrays) shared across fns (HC_*_ABS)
 *   F64 arith/cmp (HC_F*); literals via I2F (no C double under +nofp)
 *   F64 class members; F64 !=
 *   Array brace init: Type a[N] = { e0, e1, … }; (local + file-scope)
 *   Builtins: PutPixel/FillRect/Cls/GrLine, Print*, Mouse*, Key*, Abs, Rand, Sleep
 *   Locals/arrays/classes live in g_hc_mem so & works.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "hc_ir.h"

/* Kernel-provided host helpers (resolved at compile time). */
uint64_t hc_builtin_abs(uint64_t x);
uint64_t hc_builtin_min(uint64_t a, uint64_t b);
uint64_t hc_builtin_max(uint64_t a, uint64_t b);
uint64_t hc_builtin_sign(uint64_t x);
uint64_t hc_builtin_clamp(uint64_t v, uint64_t lo, uint64_t hi);
uint64_t hc_builtin_sqr(uint64_t x);
uint64_t hc_builtin_cos(uint64_t xbits); /* F64 bits in/out */
uint64_t hc_builtin_sin(uint64_t xbits);
uint64_t hc_builtin_sqrt(uint64_t xbits);
uint64_t hc_builtin_wrap1(uint64_t th_bits);
uint64_t hc_builtin_wrap2(uint64_t th_bits, uint64_t base_bits);
uint64_t hc_builtin_arg(uint64_t x_bits, uint64_t y_bits); /* Arg(x,y)=atan2(y,x) */
uint64_t hc_builtin_fabs(uint64_t xbits); /* F64 abs via sign-bit clear */
uint64_t hc_builtin_dcalias(void);        /* Lattice DCAlias → &g_hc_cdc */
uint64_t hc_builtin_refresh(void);        /* Lattice Refresh → call Fs->draw_it */
uint64_t hc_builtin_dcfill(uint64_t dc);  /* Lattice DCFill(dc) stub */
uint64_t hc_builtin_dcdel(uint64_t dc);   /* Lattice DCDel(dc) stub */
uint64_t hc_builtin_dcdepthbufalloc(uint64_t dc); /* Lattice DCDepthBufAlloc stub */
uint64_t hc_builtin_dcdepthbufreset(uint64_t dc); /* Lattice DCDepthBufReset stub */
uint64_t hc_builtin_popupcolor(uint64_t header);  /* Lattice PopUpColor Mid→YELLOW Edge→BLACK */
uint64_t hc_builtin_messageget(uint64_t p_arg1, uint64_t p_arg2, uint64_t mask);
uint64_t hc_builtin_msgquepush(uint64_t type, uint64_t a1, uint64_t a2); /* bring-up seed */
uint64_t hc_builtin_menupush(uint64_t s); /* Lattice MenuPush stub */
uint64_t hc_builtin_menupop(void);        /* Lattice MenuPop stub */
uint64_t hc_builtin_menuentryfind(uint64_t menu, uint64_t path); /* per-path stub */
/* Lattice shell/setup no-ops (SettingsPush, WinMax, DocClear, PutExcept, …). */
uint64_t hc_builtin_nop0(void);
uint64_t hc_builtin_fs_pix_width(void);   /* Fs->pix_width */
uint64_t hc_builtin_fs_pix_height(void);  /* Fs->pix_height */
/* Lattice Fs stub fields (assignable). */
extern uint64_t g_hc_fs_win_inhibit;
extern uint64_t g_hc_fs_draw_it;
extern uint64_t g_hc_fs_cur_menu;
uint64_t hc_builtin_putpixel(uint64_t x, uint64_t y, uint64_t c);
/* Bring-up GrPlot(x,y,c) or Lattice GrPlot(dc,x,y) — see hc_builtin_grplot. */
uint64_t hc_builtin_grplot(uint64_t a0, uint64_t a1, uint64_t a2);
/* Lattice: GrPlot3(dc,x,y,z) — ignore dc/z; 2D plot with default color. */
uint64_t hc_builtin_grplot3(uint64_t dc, uint64_t x, uint64_t y, uint64_t z);
/* Lattice: GrLine3(dc,x1,y1,z1,x2,y2,z2) — ignore dc/z; 2D line. */
uint64_t hc_builtin_grline3(uint64_t dc, uint64_t x1, uint64_t y1, uint64_t z1,
                            uint64_t x2, uint64_t y2, uint64_t z2);
uint64_t hc_builtin_grpeek(uint64_t x, uint64_t y);
uint64_t hc_builtin_grpeek_dc(uint64_t dc, uint64_t x, uint64_t y);
uint64_t hc_builtin_cls(uint64_t c);
uint64_t hc_builtin_fillrect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t c);
uint64_t hc_builtin_grline(uint64_t x0, uint64_t y0, uint64_t x1, uint64_t y1, uint64_t c);
uint64_t hc_builtin_grline_dc(uint64_t dc, uint64_t x1, uint64_t y1, uint64_t x2, uint64_t y2);
uint64_t hc_builtin_grline6(uint64_t x0, uint64_t y0, uint64_t x1, uint64_t y1, uint64_t c,
                            uint64_t step);
uint64_t hc_builtin_grhline(uint64_t x, uint64_t y, uint64_t w, uint64_t c);
uint64_t hc_builtin_grvline(uint64_t x, uint64_t y, uint64_t h, uint64_t c);
uint64_t hc_builtin_grcircle(uint64_t cx, uint64_t cy, uint64_t r, uint64_t c);
uint64_t hc_builtin_grfillcircle(uint64_t cx, uint64_t cy, uint64_t r, uint64_t c);
uint64_t hc_builtin_putchar(uint64_t ch);
uint64_t hc_builtin_print(uint64_t s);
uint64_t hc_builtin_printat(uint64_t col, uint64_t row, uint64_t s);
/* Lattice GrPrint(dc,x,y,fmt,…) — fmt literal for now; x/y pixels→cells. */
uint64_t hc_builtin_grprint(uint64_t dc, uint64_t x, uint64_t y, uint64_t fmt);
uint64_t hc_builtin_grprintn(uint64_t dc, uint64_t x, uint64_t y, uint64_t fmt, uint64_t argv,
                             uint64_t nargs);
void hc_fmt_f64_bits(char **dp, uint64_t bits, int prec);
uint64_t hc_builtin_printi64(uint64_t v);
uint64_t hc_builtin_strlen(uint64_t s);
uint64_t hc_builtin_strcmp(uint64_t a, uint64_t b);
uint64_t hc_builtin_str2i64(uint64_t s);
uint64_t hc_builtin_hashstr(uint64_t s);
uint64_t hc_builtin_strcpy(uint64_t dst, uint64_t src);
uint64_t hc_builtin_strcat(uint64_t dst, uint64_t src);
uint64_t hc_builtin_strnew(uint64_t src);
uint64_t hc_builtin_catprint(uint64_t dst, uint64_t fmt, uint64_t a0, uint64_t a1);
uint64_t hc_builtin_strprint(uint64_t dst, uint64_t fmt, uint64_t a0, uint64_t a1);
uint64_t hc_builtin_mstrprint(uint64_t fmt, uint64_t a0, uint64_t a1);
uint64_t hc_builtin_call0(uint64_t fp);
uint64_t hc_builtin_call1(uint64_t fp, uint64_t a0);
uint64_t hc_builtin_blkwrite(uint64_t buf, uint64_t blk, uint64_t count);
uint64_t hc_builtin_blkread(uint64_t buf, uint64_t blk, uint64_t count);
uint64_t hc_builtin_memcpy(uint64_t dst, uint64_t src, uint64_t n);
uint64_t hc_builtin_memset(uint64_t dst, uint64_t val, uint64_t n);
uint64_t hc_builtin_malloc(uint64_t n);
uint64_t hc_builtin_free(uint64_t p);
uint64_t hc_builtin_mousex(void);
uint64_t hc_builtin_mousey(void);
uint64_t hc_builtin_mousebtn(void);
uint64_t hc_builtin_mousedx(void);
uint64_t hc_builtin_mousedy(void);
uint64_t hc_builtin_fbw(void);
uint64_t hc_builtin_fbh(void);
uint64_t hc_builtin_rand(void);
uint64_t hc_builtin_sleep(uint64_t ms);
uint64_t hc_builtin_cnt(void);
uint64_t hc_builtin_cntfrq(void);
uint64_t hc_builtin_keyhit(void);
uint64_t hc_builtin_getkey(void);

#define HC_FRONT_ERR (-100)
#define HC_MAX_FNS   16 /* was 8 — shared RedSea/CFile lib + demos */
#define HC_MAX_LOOPS 8 /* was 4 — Lattice while + nested msg/key/cursor switches */
#define HC_MAX_CLASSES 8
#define HC_MAX_MEMBERS 16
#define HC_MAX_ENUMS 24
#define HC_TYPE_NONE 0xff

struct hc_fn {
    char name[24]; /* Lattice TurtleMicroMove = 15 chars */
    int nargs;
    uint32_t *code;
    uint8_t bc[8192]; /* was 4096 — Lattice NearLattice key compose + if(tt.w) */
    int bc_len;
};

struct hc_fn_table {
    struct hc_fn fns[HC_MAX_FNS];
    int n;
};

struct hc_member {
    char name[16];
    uint16_t off;   /* byte offset within class */
    uint8_t stride; /* 1 = U8, 8 = I64/F64, or nested class size */
    uint8_t type;   /* HC_TYPE_NONE = scalar, else nested class idx */
    uint8_t is_f64; /* 1 = F64 scalar member */
};

struct hc_class {
    char name[16];
    int nmembers;
    struct hc_member mem[HC_MAX_MEMBERS];
    int size; /* bytes, 8-aligned */
};

struct hc_enum_const {
    char name[16];
    int64_t val;
};

struct hc_front {
    const char *p;
    uint8_t *bc;
    size_t n;
    size_t cap;
    int err;
    int next_label;
    char names[HC_IR_MAX_NAMES][16];
    uint8_t base[HC_IR_MAX_NAMES];
    uint8_t count[HC_IR_MAX_NAMES];
    uint8_t stride[HC_IR_MAX_NAMES]; /* 8 = I64 element, 1 = U8 element */
    uint8_t type[HC_IR_MAX_NAMES];   /* HC_TYPE_NONE or class index */
    uint8_t is_ptr[HC_IR_MAX_NAMES]; /* 1 = Class* (one word holding addr) */
    uint8_t is_global[HC_IR_MAX_NAMES]; /* 1 = file-scope (HC_*_ABS) */
    uint8_t is_f64[HC_IR_MAX_NAMES];    /* 1 = F64 (IEEE bits in slot) */
    int expr_f64; /* 1 = last prim/expr produced F64 bits */
    int nlocals;
    int mem_used;   /* frame-local words (HC_ENTER) */
    int glob_used;  /* file-scope words at g_hc_mem[0..) */
    int str_top;    /* string literals grow down from HC_IR_MEM_WORDS */
    struct hc_fn_table *fns;
    struct hc_class classes[HC_MAX_CLASSES];
    int nclasses;
    struct hc_enum_const enums[HC_MAX_ENUMS];
    int nenums;
    char glab_names[HC_IR_MAX_LABELS][16];
    int glab_ids[HC_IR_MAX_LABELS];
    int nglab;
    int brk_tgt[HC_MAX_LOOPS];  /* break targets: loops + switches */
    int cont_tgt[HC_MAX_LOOPS]; /* continue targets: loops only */
    int nbrk;
    int ncont;
};

static inline void hcf_skip(struct hc_front *f) {
    for (;;) {
        while (*f->p == ' ' || *f->p == '\t' || *f->p == '\n' || *f->p == '\r') {
            f->p++;
        }
        if (f->p[0] == '/' && f->p[1] == '/') {
            while (*f->p && *f->p != '\n') {
                f->p++;
            }
            continue;
        }
        if (f->p[0] == '/' && f->p[1] == '*') {
            f->p += 2;
            while (*f->p && !(f->p[0] == '*' && f->p[1] == '/')) {
                f->p++;
            }
            if (*f->p) {
                f->p += 2;
            }
            continue;
        }
        break;
    }
}

static inline int hcf_emit(struct hc_front *f, uint8_t op) {
    if (f->n >= f->cap) {
        f->err = 1;
        return -1;
    }
    f->bc[f->n++] = op;
    return 0;
}

static inline int hcf_emit_u8(struct hc_front *f, uint8_t op, uint8_t a) {
    if (f->n + 2 > f->cap) {
        f->err = 1;
        return -1;
    }
    f->bc[f->n++] = op;
    f->bc[f->n++] = a;
    return 0;
}

static inline int hcf_emit_u8u8(struct hc_front *f, uint8_t op, uint8_t a, uint8_t b) {
    if (f->n + 3 > f->cap) {
        f->err = 1;
        return -1;
    }
    f->bc[f->n++] = op;
    f->bc[f->n++] = a;
    f->bc[f->n++] = b;
    return 0;
}

static inline int hcf_imm(struct hc_front *f, uint64_t v) {
    if (f->n + 9 > f->cap) {
        f->err = 1;
        return -1;
    }
    f->n += hc_pack_imm64(f->bc + f->n, v);
    f->expr_f64 = 0;
    return 0;
}

static inline int hcf_expect(struct hc_front *f, char c) {
    hcf_skip(f);
    if (*f->p != c) {
        f->err = 1;
        return -1;
    }
    f->p++;
    return 0;
}

static inline int hcf_new_label(struct hc_front *f) {
    if (f->next_label >= HC_IR_MAX_LABELS) {
        f->err = 1;
        return -1;
    }
    return f->next_label++;
}

static inline int hcf_ident_start(char c) {
    unsigned char u = (unsigned char)c;
    /* TempleOS Lattice: θ is 0xE9 (single-byte charset). */
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || u == 0xe9;
}
static inline int hcf_ident_char(char c) {
    return hcf_ident_start(c) || (c >= '0' && c <= '9');
}

static inline int hcf_kw(struct hc_front *f, const char *kw) {
    hcf_skip(f);
    const char *p = f->p;
    while (*kw && *p == *kw) {
        p++;
        kw++;
    }
    if (*kw || hcf_ident_char(*p)) {
        return 0;
    }
    f->p = p;
    return 1;
}

static inline int hcf_lookup(struct hc_front *f, const char *name, int len) {
    for (int i = 0; i < f->nlocals; i++) {
        int j = 0;
        while (j < len && f->names[i][j] && f->names[i][j] == name[j]) {
            j++;
        }
        if (j == len && f->names[i][j] == 0) {
            return i;
        }
    }
    return -1;
}

static inline int hcf_decl_n(struct hc_front *f, const char *name, int len, int nelem,
                             int stride) {
    int id = hcf_lookup(f, name, len);
    if (id >= 0) {
        return id;
    }
    int words;
    if (stride == 1) {
        words = (nelem + 7) / 8;
    } else if (stride == 4) {
        words = (nelem * 4 + 7) / 8;
    } else if (stride > 8) {
        words = ((stride + 7) / 8) * nelem;
    } else {
        words = nelem;
    }
    if (nelem < 1 || stride < 1 || f->nlocals >= HC_IR_MAX_NAMES || len >= 15 ||
        f->mem_used + words > HC_IR_MEM_WORDS) {
        f->err = 1;
        return -1;
    }
    id = f->nlocals++;
    for (int i = 0; i < len; i++) {
        f->names[id][i] = name[i];
    }
    f->names[id][len] = 0;
    f->base[id] = (uint8_t)f->mem_used;
    f->count[id] = (uint8_t)nelem;
    f->stride[id] = (uint8_t)stride;
    f->type[id] = HC_TYPE_NONE;
    f->is_ptr[id] = 0;
    f->is_global[id] = 0;
    f->is_f64[id] = 0;
    f->mem_used += words;
    return id;
}

/* File-scope global: absolute g_hc_mem slot (shared across functions). */
static inline int hcf_decl_global_n(struct hc_front *f, const char *name, int len, int nelem,
                                    int stride) {
    int id = hcf_lookup(f, name, len);
    if (id >= 0) {
        return id;
    }
    int words;
    if (stride == 1) {
        words = (nelem + 7) / 8;
    } else if (stride == 4) {
        words = (nelem * 4 + 7) / 8;
    } else if (stride > 8) {
        words = ((stride + 7) / 8) * nelem;
    } else {
        words = nelem;
    }
    if (nelem < 1 || stride < 1 || f->nlocals >= HC_IR_MAX_NAMES || len >= 15 ||
        f->glob_used + words > HC_IR_MAX_GLOBS ||
        f->glob_used + words > HC_IR_MEM_WORDS) {
        f->err = 1;
        return -1;
    }
    id = f->nlocals++;
    for (int i = 0; i < len; i++) {
        f->names[id][i] = name[i];
    }
    f->names[id][len] = 0;
    f->base[id] = (uint8_t)f->glob_used;
    f->count[id] = (uint8_t)nelem;
    f->stride[id] = (uint8_t)stride;
    f->type[id] = HC_TYPE_NONE;
    f->is_ptr[id] = 0;
    f->is_global[id] = 1;
    f->is_f64[id] = 0;
    f->glob_used += words;
    return id;
}

static inline int hcf_decl(struct hc_front *f, const char *name, int len) {
    return hcf_decl_n(f, name, len, 1, 8);
}

static inline int hcf_align_up(int v, int a) {
    return (v + a - 1) & ~(a - 1);
}

static inline int hcf_find_glab(struct hc_front *f, const char *name, int len) {
    for (int i = 0; i < f->nglab; i++) {
        int j = 0;
        while (j < len && f->glab_names[i][j] && f->glab_names[i][j] == name[j]) {
            j++;
        }
        if (j == len && f->glab_names[i][j] == 0) {
            return f->glab_ids[i];
        }
    }
    return -1;
}

static inline int hcf_get_glab(struct hc_front *f, const char *name, int len) {
    int id = hcf_find_glab(f, name, len);
    if (id >= 0) {
        return id;
    }
    if (f->nglab >= HC_IR_MAX_LABELS || len >= 15) {
        f->err = 1;
        return -1;
    }
    id = hcf_new_label(f);
    if (id < 0) {
        return -1;
    }
    for (int i = 0; i < len; i++) {
        f->glab_names[f->nglab][i] = name[i];
    }
    f->glab_names[f->nglab][len] = 0;
    f->glab_ids[f->nglab] = id;
    f->nglab++;
    return id;
}

static inline int hcf_find_class(struct hc_front *f, const char *name, int len) {
    for (int i = 0; i < f->nclasses; i++) {
        int j = 0;
        while (j < len && f->classes[i].name[j] && f->classes[i].name[j] == name[j]) {
            j++;
        }
        if (j == len && f->classes[i].name[j] == 0) {
            return i;
        }
    }
    return -1;
}

static inline int hcf_find_enum(struct hc_front *f, const char *name, int len) {
    for (int i = 0; i < f->nenums; i++) {
        int j = 0;
        while (j < len && f->enums[i].name[j] && f->enums[i].name[j] == name[j]) {
            j++;
        }
        if (j == len && f->enums[i].name[j] == 0) {
            return i;
        }
    }
    return -1;
}

static inline int hcf_add_enum(struct hc_front *f, const char *name, int len, int64_t val) {
    if (len < 1 || len >= 15 || f->nenums >= HC_MAX_ENUMS || hcf_find_enum(f, name, len) >= 0 ||
        hcf_lookup(f, name, len) >= 0) {
        f->err = 1;
        return -1;
    }
    struct hc_enum_const *e = &f->enums[f->nenums++];
    for (int i = 0; i < len; i++) {
        e->name[i] = name[i];
    }
    e->name[len] = 0;
    e->val = val;
    return 0;
}

/* Parse a char/hex/dec integer literal into *out (no IR emit). */
static inline int hcf_parse_int_lit(struct hc_front *f, uint64_t *out) {
    hcf_skip(f);
    if (*f->p == '\'') {
        f->p++;
        if (!*f->p) {
            f->err = 1;
            return -1;
        }
        char c = *f->p++;
        if (c == '\\' && *f->p) {
            char e = *f->p++;
            if (e == 'n') {
                c = '\n';
            } else if (e == 't') {
                c = '\t';
            } else if (e == '0') {
                c = 0;
            } else {
                c = e;
            }
        }
        if (*f->p != '\'') {
            f->err = 1;
            return -1;
        }
        f->p++;
        *out = (uint64_t)(uint8_t)c;
        return 0;
    }
    if (*f->p == '-') {
        f->p++;
        uint64_t v = 0;
        if (hcf_parse_int_lit(f, &v) < 0) {
            return -1;
        }
        *out = (uint64_t)(-(int64_t)v);
        return 0;
    }
    if (*f->p < '0' || *f->p > '9') {
        f->err = 1;
        return -1;
    }
    uint64_t v = 0;
    if (f->p[0] == '0' && (f->p[1] == 'x' || f->p[1] == 'X')) {
        f->p += 2;
        int digits = 0;
        while ((*f->p >= '0' && *f->p <= '9') || (*f->p >= 'a' && *f->p <= 'f') ||
               (*f->p >= 'A' && *f->p <= 'F')) {
            char c = *f->p++;
            digits++;
            v <<= 4;
            if (c >= '0' && c <= '9') {
                v |= (uint64_t)(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                v |= (uint64_t)(c - 'a' + 10);
            } else {
                v |= (uint64_t)(c - 'A' + 10);
            }
        }
        if (!digits) {
            f->err = 1;
            return -1;
        }
        *out = v;
        return 0;
    }
    while (*f->p >= '0' && *f->p <= '9') {
        v = v * 10 + (uint64_t)(*f->p - '0');
        f->p++;
    }
    *out = v;
    return 0;
}

static inline int hcf_find_member(struct hc_class *c, const char *name, int len) {
    for (int i = 0; i < c->nmembers; i++) {
        int j = 0;
        while (j < len && c->mem[i].name[j] && c->mem[i].name[j] == name[j]) {
            j++;
        }
        if (j == len && c->mem[i].name[j] == 0) {
            return i;
        }
    }
    return -1;
}

static inline int hcf_decl_class(struct hc_front *f, const char *name, int len, int ci,
                                 int as_ptr) {
    if (ci < 0 || ci >= f->nclasses) {
        f->err = 1;
        return -1;
    }
    int id;
    if (as_ptr) {
        id = hcf_decl_n(f, name, len, 1, 8);
    } else {
        int words = (f->classes[ci].size + 7) / 8;
        if (words < 1) {
            words = 1;
        }
        id = hcf_decl_n(f, name, len, words, 8);
    }
    if (id < 0) {
        return -1;
    }
    f->type[id] = (uint8_t)ci;
    f->is_ptr[id] = as_ptr ? 1 : 0;
    return id;
}

/* Parse switch case value: int / char / named const (CH_*, MESSAGE_*, enum). */
static inline int hcf_parse_case_val(struct hc_front *f, int *out) {
    hcf_skip(f);
    uint64_t lit = 0;
    const char *save = f->p;
    if (hcf_parse_int_lit(f, &lit) == 0) {
        *out = (int)(int64_t)lit;
        return 0;
    }
    f->p = save;
    f->err = 0;
    if (!hcf_ident_start(*f->p)) {
        f->err = 1;
        return -1;
    }
    const char *s = f->p;
    while (hcf_ident_char(*f->p)) {
        f->p++;
    }
    int len = (int)(f->p - s);
    int ei = hcf_find_enum(f, s, len);
    if (ei >= 0) {
        *out = (int)f->enums[ei].val;
        return 0;
    }
    {
        static const struct {
            const char *n;
            int len;
            int v;
        }                 k[] = {
            {"MESSAGE_NULL", 12, 0},
            {"MESSAGE_KEY_DOWN", 16, 2},
            {"MESSAGE_KEY_UP", 14, 3},
            {"MESSAGE_MS_MOVE", 15, 4},
            {"MESSAGE_MS_L_DOWN", 17, 5},
            {"MESSAGE_MS_L_UP", 15, 6},
            {"MESSAGE_MS_R_DOWN", 17, 7},
            {"MESSAGE_MS_R_UP", 15, 8},
            {"CH_SHIFT_ESC", 12, 0x1C},
            {"CH_ESC", 6, 0x1B},
            {"CH_SPACE", 8, 0x20},
            {"SC_CURSOR_UP", 12, 0x48},
            {"SC_CURSOR_DOWN", 14, 0x50},
            {"SC_CURSOR_LEFT", 14, 0x4B},
            {"SC_CURSOR_RIGHT", 15, 0x4D},
            {"STR_LEN", 7, 136},
        };
        for (int i = 0; i < (int)(sizeof k / sizeof k[0]); i++) {
            if (len == k[i].len) {
                int ok = 1;
                for (int j = 0; j < len; j++) {
                    if (s[j] != k[i].n[j]) {
                        ok = 0;
                        break;
                    }
                }
                if (ok) {
                    *out = k[i].v;
                    return 0;
                }
            }
        }
    }
    f->err = 1;
    return -1;
}

/* Lattice bare/call0 setup stubs. */
static inline int hcf_lattice_nop0(const char *s, int len) {
    static const struct {
        const char *n;
        int len;
    }                 nops[] = {
        {"SettingsPush", 12},
        {"SettingsPop", 11},
        {"AutoComplete", 12},
        {"WinBorder", 9},
        {"WinMax", 6},
        {"DocCursor", 9},
        {"DocClear", 8},
        {"PutExcept", 9},
    };
    for (int i = 0; i < (int)(sizeof nops / sizeof nops[0]); i++) {
        if (len == nops[i].len) {
            int ok = 1;
            for (int j = 0; j < len; j++) {
                if (s[j] != nops[i].n[j]) {
                    ok = 0;
                    break;
                }
            }
            if (ok) {
                return 1;
            }
        }
    }
    return 0;
}

static int hcf_parse_expr(struct hc_front *f);
static int hcf_parse_stmt(struct hc_front *f);
static int hcf_emit_var_addr(struct hc_front *f, int name_idx);
static int hcf_emit_var_ld(struct hc_front *f, int name_idx);
static int hcf_emit_var_st(struct hc_front *f, int name_idx);

/*
 * Parse `.m1.m2` or (if arrow) starting from a pointer value already on stack /
 * for `.` starting from object base. Leaves address of final scalar/nested field.
 * *out_stride / *out_type / *out_is_f64 describe the final field.
 */
static int hcf_parse_members_from(struct hc_front *f, int ci, int *out_stride, int *out_type,
                                  int *out_is_f64) {
    int cur = ci;
    for (;;) {
        hcf_skip(f);
        if (!hcf_ident_start(*f->p) || cur < 0 || cur >= f->nclasses) {
            f->err = 1;
            return -1;
        }
        const char *s = f->p;
        while (hcf_ident_char(*f->p)) {
            f->p++;
        }
        struct hc_class *c = &f->classes[cur];
        int mi = hcf_find_member(c, s, (int)(f->p - s));
        if (mi < 0) {
            f->err = 1;
            return -1;
        }
        if (hcf_imm(f, (uint64_t)c->mem[mi].off) < 0 || hcf_emit(f, HC_ADD) < 0) {
            return -1;
        }
        *out_stride = (int)c->mem[mi].stride;
        *out_type = (int)c->mem[mi].type;
        *out_is_f64 = (int)c->mem[mi].is_f64;
        hcf_skip(f);
        if (*f->p == '.') {
            if (c->mem[mi].type == HC_TYPE_NONE) {
                f->err = 1;
                return -1;
            }
            f->p++;
            cur = (int)c->mem[mi].type;
            continue;
        }
        return 0;
    }
}

/* name.m… — push &final; requires non-pointer class local. */
static int hcf_parse_dot_addr(struct hc_front *f, int name_idx, int *out_stride, int *out_type,
                              int *out_is_f64) {
    if (f->is_ptr[name_idx] || f->type[name_idx] == HC_TYPE_NONE ||
        f->type[name_idx] >= (uint8_t)f->nclasses) {
        f->err = 1;
        return -1;
    }
    if (hcf_emit_var_addr(f, name_idx) < 0) {
        return -1;
    }
    return hcf_parse_members_from(f, (int)f->type[name_idx], out_stride, out_type, out_is_f64);
}

/* name->m… — push &final; requires Class* local. */
static int hcf_parse_arrow_addr(struct hc_front *f, int name_idx, int *out_stride, int *out_type,
                                int *out_is_f64) {
    if (!f->is_ptr[name_idx] || f->type[name_idx] == HC_TYPE_NONE ||
        f->type[name_idx] >= (uint8_t)f->nclasses) {
        f->err = 1;
        return -1;
    }
    if (hcf_emit_var_ld(f, name_idx) < 0) {
        return -1;
    }
    return hcf_parse_members_from(f, (int)f->type[name_idx], out_stride, out_type, out_is_f64);
}

static inline struct hc_fn *hcf_find_fn(struct hc_front *f, const char *name, int len) {
    if (!f->fns) {
        return NULL;
    }
    for (int i = 0; i < f->fns->n; i++) {
        int j = 0;
        while (j < len && f->fns->fns[i].name[j] && f->fns->fns[i].name[j] == name[j]) {
            j++;
        }
        if (j == len && f->fns->fns[i].name[j] == 0) {
            return &f->fns->fns[i];
        }
    }
    return NULL;
}

static int hcf_emit_var_addr(struct hc_front *f, int name_idx) {
    uint8_t op = f->is_global[name_idx] ? HC_ABS_ADDR : HC_SLOT_ADDR;
    return hcf_emit_u8(f, op, f->base[name_idx]);
}

static int hcf_emit_var_ld(struct hc_front *f, int name_idx) {
    uint8_t op = f->is_global[name_idx] ? HC_LD_ABS : HC_LD_SLOT;
    return hcf_emit_u8(f, op, f->base[name_idx]);
}

static int hcf_emit_var_st(struct hc_front *f, int name_idx) {
    uint8_t op = f->is_global[name_idx] ? HC_ST_ABS : HC_ST_SLOT;
    return hcf_emit_u8(f, op, f->base[name_idx]);
}

static int hcf_pow10u(int n) {
    int v = 1;
    for (int i = 0; i < n && i < 9; i++) {
        v *= 10;
    }
    return v;
}

/* Emit F64 from unsigned decimal ip.frac (frac_digits). Leaves expr_f64=1. */
static int hcf_emit_f64_dec(struct hc_front *f, uint64_t ip, uint64_t frac, int frac_digits) {
    if (hcf_imm(f, ip) < 0 || hcf_emit(f, HC_I2F) < 0) {
        return -1;
    }
    if (frac_digits > 0) {
        if (hcf_imm(f, frac) < 0 || hcf_emit(f, HC_I2F) < 0 ||
            hcf_imm(f, (uint64_t)hcf_pow10u(frac_digits)) < 0 || hcf_emit(f, HC_I2F) < 0 ||
            hcf_emit(f, HC_FDIV) < 0 || hcf_emit(f, HC_FADD) < 0) {
            return -1;
        }
    }
    f->expr_f64 = 1;
    return 0;
}


static int hcf_parse_index_addr(struct hc_front *f, int name_idx) {
    if (f->is_ptr[name_idx]) {
        if (hcf_emit_var_ld(f, name_idx) < 0 || hcf_parse_expr(f) < 0 ||
            hcf_expect(f, ']') < 0 || hcf_imm(f, f->stride[name_idx]) < 0 ||
            hcf_emit(f, HC_MUL) < 0 || hcf_emit(f, HC_ADD) < 0) {
            return -1;
        }
        return 0;
    }
    if (hcf_emit_var_addr(f, name_idx) < 0 || hcf_parse_expr(f) < 0 ||
        hcf_expect(f, ']') < 0 || hcf_imm(f, f->stride[name_idx]) < 0 ||
        hcf_emit(f, HC_MUL) < 0 || hcf_emit(f, HC_ADD) < 0) {
        return -1;
    }
    return 0;
}

static int hcf_emit_load_at(struct hc_front *f, int name_idx) {
    int s = f->stride[name_idx];
    if (s == 1) {
        return hcf_emit(f, HC_LOAD8);
    }
    if (s == 4) {
        return hcf_emit(f, HC_LOAD32);
    }
    return hcf_emit(f, HC_LOAD);
}

static int hcf_emit_store_at(struct hc_front *f, int name_idx) {
    int s = f->stride[name_idx];
    if (s == 1) {
        return hcf_emit(f, HC_STORE8);
    }
    if (s == 4) {
        return hcf_emit(f, HC_STORE32);
    }
    return hcf_emit(f, HC_STORE);
}

/* a[N] = { e0, e1, … }; — remaining elements zeroed. */
static int hcf_parse_brace_init(struct hc_front *f, int id) {
    int n = (int)f->count[id];
    int stride = (int)f->stride[id];
    int i = 0;

    if (n < 1 || hcf_expect(f, '{') < 0) {
        return -1;
    }
    hcf_skip(f);
    if (*f->p != '}') {
        for (;;) {
            if (i >= n) {
                f->err = 1;
                return -1;
            }
            if (hcf_emit_var_addr(f, id) < 0 || hcf_imm(f, (uint64_t)i) < 0 ||
                hcf_imm(f, (uint64_t)stride) < 0 || hcf_emit(f, HC_MUL) < 0 ||
                hcf_emit(f, HC_ADD) < 0 || hcf_parse_expr(f) < 0 ||
                hcf_emit_store_at(f, id) < 0) {
                return -1;
            }
            i++;
            hcf_skip(f);
            if (*f->p == ',') {
                f->p++;
                hcf_skip(f);
                if (*f->p == '}') {
                    f->p++;
                    break;
                }
                continue;
            }
            if (*f->p == '}') {
                f->p++;
                break;
            }
            f->err = 1;
            return -1;
        }
    } else {
        f->p++;
    }
    while (i < n) {
        if (hcf_emit_var_addr(f, id) < 0 || hcf_imm(f, (uint64_t)i) < 0 ||
            hcf_imm(f, (uint64_t)stride) < 0 || hcf_emit(f, HC_MUL) < 0 ||
            hcf_emit(f, HC_ADD) < 0 || hcf_imm(f, 0) < 0 ||
            hcf_emit_store_at(f, id) < 0) {
            return -1;
        }
        i++;
    }
    return 0;
}

static int hcf_is_u32_scalar(struct hc_front *f, int id) {
    return id >= 0 && !f->is_ptr[id] && f->type[id] == HC_TYPE_NONE &&
           f->stride[id] == 4 && f->count[id] == 1;
}

static int hcf_trunc32(struct hc_front *f) {
    return hcf_imm(f, 0xffffffffu) < 0 || hcf_emit(f, HC_AND) < 0 ? -1 : 0;
}

static int hcf_ld_named(struct hc_front *f, int id) {
    if (hcf_emit_var_ld(f, id) < 0) {
        return -1;
    }
    if (hcf_is_u32_scalar(f, id)) {
        if (hcf_trunc32(f) < 0) {
            return -1;
        }
    }
    /* Must set every load — compare clears expr_f64; relying on prior lit leaks breaks
     * while/if bodies like x=x+0.5 after (x<=2.0). */
    f->expr_f64 = f->is_f64[id] ? 1 : 0;
    return 0;
}

static int hcf_st_named(struct hc_front *f, int id) {
    if (hcf_is_u32_scalar(f, id) && hcf_trunc32(f) < 0) {
        return -1;
    }
    return hcf_emit_var_st(f, id);
}

/* '=' → 0; '+=' → HC_ADD; etc. Leaves f->p past the operator. */
static int hcf_parse_assign_op(struct hc_front *f) {
    hcf_skip(f);
    if (f->p[0] == '+' && f->p[1] == '=') {
        f->p += 2;
        return HC_ADD;
    }
    if (f->p[0] == '-' && f->p[1] == '=') {
        f->p += 2;
        return HC_SUB;
    }
    if (f->p[0] == '*' && f->p[1] == '=') {
        f->p += 2;
        return HC_MUL;
    }
    if (*f->p == '=') {
        f->p++;
        return 0;
    }
    f->err = 1;
    return -1;
}

/* ptr already on stack. bin==0 → plain store; else DUP/load/rhs/op/store.
 * is_f64: remap +=/-=/*= to FADD/FSUB/FMUL; promote I64 RHS via I2F. */
static int hcf_finish_mem_assign(struct hc_front *f, int bin, int load_op, int store_op, int is_f64) {
    if (bin < 0) {
        return -1;
    }
    if (bin) {
        uint8_t op = (uint8_t)bin;
        if (is_f64) {
            if (bin == HC_ADD) {
                op = HC_FADD;
            } else if (bin == HC_SUB) {
                op = HC_FSUB;
            } else if (bin == HC_MUL) {
                op = HC_FMUL;
            } else {
                f->err = 1;
                return -1;
            }
        }
        if (hcf_emit(f, HC_DUP) < 0 || hcf_emit(f, (uint8_t)load_op) < 0 ||
            hcf_parse_expr(f) < 0) {
            return -1;
        }
        if (is_f64 && !f->expr_f64) {
            if (hcf_emit(f, HC_I2F) < 0) {
                return -1;
            }
            f->expr_f64 = 1;
        }
        if (hcf_emit(f, op) < 0 || hcf_emit(f, (uint8_t)store_op) < 0) {
            return -1;
        }
    } else {
        if (hcf_parse_expr(f) < 0) {
            return -1;
        }
        if (is_f64 && !f->expr_f64) {
            if (hcf_emit(f, HC_I2F) < 0) {
                return -1;
            }
            f->expr_f64 = 1;
        } else if (!is_f64 && f->expr_f64) {
            f->err = 1; /* F64→I64 store: use ToI64 */
            return -1;
        }
        if (hcf_emit(f, (uint8_t)store_op) < 0) {
            return -1;
        }
    }
    return hcf_expect(f, ';');
}

/* Lattice: tt.w++; t->idx--; — addr already on stack. */
static int hcf_finish_mem_incdec(struct hc_front *f, int load_op, int store_op, int is_f64,
                                 int is_inc) {
    if (hcf_emit(f, HC_DUP) < 0 || hcf_emit(f, (uint8_t)load_op) < 0) {
        return -1;
    }
    if (is_f64) {
        if (hcf_imm(f, 0x3ff0000000000000ULL) < 0 || /* 1.0 */
            hcf_emit(f, is_inc ? HC_FADD : HC_FSUB) < 0 ||
            hcf_emit(f, (uint8_t)store_op) < 0) {
            return -1;
        }
    } else if (hcf_imm(f, 1) < 0 || hcf_emit(f, is_inc ? HC_ADD : HC_SUB) < 0 ||
               hcf_emit(f, (uint8_t)store_op) < 0) {
        return -1;
    }
    return hcf_expect(f, ';');
}

static int hcf_parse_prim(struct hc_front *f) {
    hcf_skip(f);
    /* TempleOS Lattice π (0xE3) → same F64 as ASCII `pi`. */
    if ((unsigned char)*f->p == 0xe3) {
        f->p++;
        if (hcf_imm(f, 0x400921fb54442d18ULL) < 0) {
            return -1;
        }
        f->expr_f64 = 1;
        return 0;
    }
    /* UTF-8 π (U+03C0) for modern editors of Lattice-shaped sources. */
    if ((unsigned char)f->p[0] == 0xce && (unsigned char)f->p[1] == 0xb0) {
        f->p += 2;
        if (hcf_imm(f, 0x400921fb54442d18ULL) < 0) {
            return -1;
        }
        f->expr_f64 = 1;
        return 0;
    }
    if (*f->p == '(') {
        f->p++;
        if (hcf_parse_expr(f) < 0 || hcf_expect(f, ')') < 0) {
            return -1;
        }
        return 0;
    }
    if (hcf_kw(f, "sizeof")) {
        hcf_skip(f);
        int need_paren = 0;
        if (*f->p == '(') {
            f->p++;
            need_paren = 1;
            hcf_skip(f);
        }
        uint64_t sz = 0;
        if (hcf_kw(f, "I64") || hcf_kw(f, "i64") || hcf_kw(f, "Bool") || hcf_kw(f, "bool") ||
            hcf_kw(f, "F64") || hcf_kw(f, "f64")) {
            sz = 8;
        } else if (hcf_kw(f, "U32") || hcf_kw(f, "u32")) {
            sz = 4;
        } else if (hcf_kw(f, "U8") || hcf_kw(f, "u8")) {
            sz = 1;
        } else if (hcf_kw(f, "U0") || hcf_kw(f, "u0")) {
            sz = 0;
        } else if (hcf_ident_start(*f->p)) {
            const char *ts = f->p;
            while (hcf_ident_char(*f->p)) {
                f->p++;
            }
            int tlen = (int)(f->p - ts);
            int ci = hcf_find_class(f, ts, tlen);
            int id = hcf_lookup(f, ts, tlen);
            if (ci >= 0) {
                sz = (uint64_t)f->classes[ci].size;
            } else if (id >= 0) {
                if (f->is_ptr[id]) {
                    sz = 8;
                } else if (f->type[id] != HC_TYPE_NONE) {
                    sz = (uint64_t)f->classes[f->type[id]].size;
                } else {
                    sz = (uint64_t)f->stride[id] * (uint64_t)f->count[id];
                }
            } else {
                f->err = 1;
                return -1;
            }
        } else {
            f->err = 1;
            return -1;
        }
        if (need_paren && hcf_expect(f, ')') < 0) {
            return -1;
        }
        return hcf_imm(f, sz);
    }
    if (*f->p == '-') {
        f->p++;
        if (hcf_parse_prim(f) < 0) {
            return -1;
        }
        if (f->expr_f64) {
            return hcf_emit(f, HC_FNEG);
        }
        return hcf_emit(f, HC_NEG);
    }
    if (*f->p == '!') {
        f->p++;
        if (hcf_parse_prim(f) < 0) {
            return -1;
        }
        /* !x → x==0 */
        if (hcf_imm(f, 0) < 0 || hcf_emit(f, HC_EQ) < 0) {
            return -1;
        }
        return 0;
    }
    if (*f->p == '&') {
        f->p++;
        hcf_skip(f);
        if (!hcf_ident_start(*f->p)) {
            f->err = 1;
            return -1;
        }
        const char *s = f->p;
        while (hcf_ident_char(*f->p)) {
            f->p++;
        }
        int len = (int)(f->p - s);
        int id = hcf_lookup(f, s, len);
        if (id < 0) {
            /* &FnName → address of compiled HolyC fn (JIT code). */
            struct hc_fn *fn = hcf_find_fn(f, s, len);
            if (fn && fn->code) {
                return hcf_imm(f, (uint64_t)(uintptr_t)fn->code);
            }
            f->err = 1;
            return -1;
        }
        hcf_skip(f);
        if (*f->p == '[') {
            f->p++;
            return hcf_parse_index_addr(f, id);
        }
        if (*f->p == '.') {
            f->p++;
            int st = 8, ty = HC_TYPE_NONE, isf = 0;
            return hcf_parse_dot_addr(f, id, &st, &ty, &isf);
        }
        return hcf_emit_var_addr(f, id);
    }
    if (*f->p == '*') {
        f->p++;
        hcf_skip(f);
        if (hcf_ident_start(*f->p)) {
            const char *s = f->p;
            while (hcf_ident_char(*f->p)) {
                f->p++;
            }
            int len = (int)(f->p - s);
            int id = hcf_lookup(f, s, len);
            hcf_skip(f);
            if (id >= 0 && f->is_ptr[id] && *f->p != '.' && *f->p != '[' &&
                !(f->p[0] == '-' && f->p[1] == '>')) {
                if (hcf_ld_named(f, id) < 0) {
                    return -1;
                }
                return hcf_emit(f, f->stride[id] == 1 ? HC_LOAD8 : (f->stride[id] == 4 ? HC_LOAD32 : HC_LOAD));
            }
            f->p = s;
        }
        if (hcf_parse_prim(f) < 0) {
            return -1;
        }
        return hcf_emit(f, HC_LOAD);
    }
    if (*f->p == '\'') {
        f->p++;
        if (!*f->p) {
            f->err = 1;
            return -1;
        }
        char c = *f->p++;
        if (c == '\\' && *f->p) {
            char e = *f->p++;
            if (e == 'n') {
                c = '\n';
            } else if (e == 't') {
                c = '\t';
            } else if (e == '0') {
                c = 0;
            } else {
                c = e;
            }
        }
        if (*f->p != '\'') {
            f->err = 1;
            return -1;
        }
        f->p++;
        return hcf_imm(f, (uint64_t)(uint8_t)c);
    }
    if (*f->p >= '0' && *f->p <= '9') {
        uint64_t v = 0;
        if (f->p[0] == '0' && (f->p[1] == 'x' || f->p[1] == 'X')) {
            f->p += 2;
            int digits = 0;
            while ((*f->p >= '0' && *f->p <= '9') || (*f->p >= 'a' && *f->p <= 'f') ||
                   (*f->p >= 'A' && *f->p <= 'F')) {
                char c = *f->p++;
                digits++;
                v <<= 4;
                if (c >= '0' && c <= '9') {
                    v |= (uint64_t)(c - '0');
                } else if (c >= 'a' && c <= 'f') {
                    v |= (uint64_t)(c - 'a' + 10);
                } else {
                    v |= (uint64_t)(c - 'A' + 10);
                }
            }
            if (!digits) {
                f->err = 1;
                return -1;
            }
            return hcf_imm(f, v);
        }
        while (*f->p >= '0' && *f->p <= '9') {
            v = v * 10 + (uint64_t)(*f->p - '0');
            f->p++;
        }
        if (*f->p == '.') {
            f->p++;
            uint64_t frac = 0;
            int fdig = 0;
            while (*f->p >= '0' && *f->p <= '9' && fdig < 9) {
                frac = frac * 10 + (uint64_t)(*f->p - '0');
                f->p++;
                fdig++;
            }
            /* Skip excess frac digits (bring-up F64 lit precision cap is 9). */
            while (*f->p >= '0' && *f->p <= '9') {
                f->p++;
            }
            return hcf_emit_f64_dec(f, v, frac, fdig);
        }
        f->expr_f64 = 0;
        return hcf_imm(f, v);
    }
    if (*f->p == '"') {
        /* Adjacent string concat: "a" "b" → "ab" (Lattice MenuPush). */
        char buf[512]; /* was 241 — Lattice File/Play/Settings ~414 chars */
        int n = 0;
        for (;;) {
            f->p++; /* opening " */
            while (*f->p && *f->p != '"' && n < 511) {
                char c = *f->p++;
                if (c == '\\' && *f->p) {
                    char e = *f->p++;
                    if (e == 'n') {
                        c = '\n';
                    } else if (e == 't') {
                        c = '\t';
                    } else if (e == '0') {
                        c = 0;
                    } else {
                        c = e;
                    }
                }
                buf[n++] = c;
            }
            if (*f->p != '"') {
                f->err = 1;
                return -1;
            }
            f->p++;
            hcf_skip(f);
            if (*f->p != '"') {
                break;
            }
        }
        buf[n] = 0;
        int words = (n + 8) / 8;
        if (words < 1) {
            words = 1;
        }
        if (f->str_top < words || f->glob_used + f->mem_used + words > HC_IR_MEM_WORDS) {
            f->err = 1;
            return -1;
        }
        f->str_top -= words;
        int word = f->str_top;
        uint8_t *dst = (uint8_t *)&g_hc_mem[word];
        for (int i = 0; i <= n; i++) {
            dst[i] = (uint8_t)buf[i];
        }
        if (hcf_imm(f, (uint64_t)(uintptr_t)dst) < 0) {
            return -1;
        }
        hcf_skip(f);
        if (*f->p == '[') {
            f->p++;
            if (hcf_parse_expr(f) < 0 || hcf_expect(f, ']') < 0 || hcf_imm(f, 1) < 0 ||
                hcf_emit(f, HC_MUL) < 0 || hcf_emit(f, HC_ADD) < 0 || hcf_emit(f, HC_LOAD8) < 0) {
                return -1;
            }
        }
        return 0;
    }
    if (hcf_ident_start(*f->p)) {
        const char *s = f->p;
        while (hcf_ident_char(*f->p)) {
            f->p++;
        }
        int len = (int)(f->p - s);
        /* true/false / TRUE/FALSE as 1/0 before call/local lookup. */
        if (len == 4 && s[0] == 't' && s[1] == 'r' && s[2] == 'u' && s[3] == 'e') {
            return hcf_imm(f, 1);
        }
        if (len == 4 && s[0] == 'T' && s[1] == 'R' && s[2] == 'U' && s[3] == 'E') {
            return hcf_imm(f, 1);
        }
        if (len == 5 && s[0] == 'f' && s[1] == 'a' && s[2] == 'l' && s[3] == 's' && s[4] == 'e') {
            return hcf_imm(f, 0);
        }
        if (len == 5 && s[0] == 'F' && s[1] == 'A' && s[2] == 'L' && s[3] == 'S' && s[4] == 'E') {
            return hcf_imm(f, 0);
        }
        if (len == 4 && s[0] == 'N' && s[1] == 'U' && s[2] == 'L' && s[3] == 'L') {
            return hcf_imm(f, 0);
        }
        /* pi → IEEE F64 bits (Lattice ã stand-in for bring-up). */
        if (len == 2 && s[0] == 'p' && s[1] == 'i') {
            if (hcf_imm(f, 0x400921fb54442d18ULL) < 0) { /* M_PI */
                return -1;
            }
            f->expr_f64 = 1;
            return 0;
        }
        hcf_skip(f);
        if (*f->p == '(') {
            f->p++;
            int nargs = 0;
            /* GrPlot3/GrLine3/GrLine/GrPlot: F64 coords → I64 (ZealOS truncates at call). */
            int coerce_i64 = (len == 7 && s[0] == 'G' && s[1] == 'r' && s[2] == 'P' &&
                              s[3] == 'l' && s[4] == 'o' && s[5] == 't' && s[6] == '3') ||
                             (len == 6 && s[0] == 'G' && s[1] == 'r' && s[2] == 'P' &&
                              s[3] == 'l' && s[4] == 'o' && s[5] == 't') ||
                             (len == 7 && s[0] == 'G' && s[1] == 'r' && s[2] == 'L' &&
                              s[3] == 'i' && s[4] == 'n' && s[5] == 'e' && s[6] == '3') ||
                             (len == 6 && s[0] == 'G' && s[1] == 'r' && s[2] == 'L' &&
                              s[3] == 'i' && s[4] == 'n' && s[5] == 'e');
            hcf_skip(f);
            if (*f->p != ')') {
                for (;;) {
                    hcf_skip(f);
                    /* HolyC default/omitted args: MessageGet(,,mask) → NULL,NULL,mask */
                    if (*f->p == ',' || *f->p == ')') {
                        if (*f->p == ')') {
                            break;
                        }
                        if (hcf_imm(f, 0) < 0) {
                            return -1;
                        }
                        f->expr_f64 = 0;
                        nargs++;
                        f->p++; /* consume ',' */
                        continue;
                    }
                    if (hcf_parse_expr(f) < 0) {
                        return -1;
                    }
                    if (coerce_i64 && f->expr_f64) {
                        if (hcf_emit(f, HC_F2I) < 0) {
                            return -1;
                        }
                        f->expr_f64 = 0;
                    }
                    nargs++;
                    hcf_skip(f);
                    if (*f->p == ',') {
                        f->p++;
                        continue;
                    }
                    break;
                }
            }
            if (hcf_expect(f, ')') < 0) {
                f->err = 1;
                return -1;
            }
            /* GrPrint(dc,x,y,fmt,a…) may pass up to 5 format args (Lattice HUD). */
            {
                int is_grp_call = (len == 7 && s[0] == 'G' && s[1] == 'r' && s[2] == 'P' &&
                                   s[3] == 'r' && s[4] == 'i' && s[5] == 'n' && s[6] == 't');
                if (nargs > (is_grp_call ? 9 : 7)) {
                    f->err = 1;
                    return -1;
                }
            }
            if (f->n + 9 > f->cap) {
                f->err = 1;
                return -1;
            }
            if (hcf_lattice_nop0(s, len)) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_nop0);
                f->expr_f64 = 0;
                return 0;
            }
            /* Builtins (kernel-provided). */
            int is_toi = (len == 5 && s[0] == 'T' && s[1] == 'o' && s[2] == 'I' &&
                          s[3] == '6' && s[4] == '4');
            int is_tof = (len == 5 && s[0] == 'T' && s[1] == 'o' && s[2] == 'F' &&
                          s[3] == '6' && s[4] == '4');
            int is_cos = (len == 3 && s[0] == 'C' && s[1] == 'o' && s[2] == 's');
            int is_sin = (len == 3 && s[0] == 'S' && s[1] == 'i' && s[2] == 'n');
            int is_sqrt = (len == 4 && s[0] == 'S' && s[1] == 'q' && s[2] == 'r' && s[3] == 't');
            int is_wrap = (len == 4 && s[0] == 'W' && s[1] == 'r' && s[2] == 'a' && s[3] == 'p');
            int is_abs = (len == 3 && s[0] == 'A' && s[1] == 'b' && s[2] == 's');
            int is_abs64 = (len == 6 && s[0] == 'A' && s[1] == 'b' && s[2] == 's' &&
                            s[3] == 'I' && s[4] == '6' && s[5] == '4');
            int is_arg = (len == 3 && s[0] == 'A' && s[1] == 'r' && s[2] == 'g');
            int is_fabs = (len == 4 && s[0] == 'F' && s[1] == 'A' && s[2] == 'b' && s[3] == 's');
            int is_dca = (len == 7 && s[0] == 'D' && s[1] == 'C' && s[2] == 'A' &&
                          s[3] == 'l' && s[4] == 'i' && s[5] == 'a' && s[6] == 's');
            int is_ref = (len == 7 && s[0] == 'R' && s[1] == 'e' && s[2] == 'f' &&
                          s[3] == 'r' && s[4] == 'e' && s[5] == 's' && s[6] == 'h');
            int is_dcf = (len == 6 && s[0] == 'D' && s[1] == 'C' && s[2] == 'F' &&
                          s[3] == 'i' && s[4] == 'l' && s[5] == 'l');
            int is_dcd = (len == 5 && s[0] == 'D' && s[1] == 'C' && s[2] == 'D' &&
                          s[3] == 'e' && s[4] == 'l');
            int is_dcdba = (len == 15 && s[0] == 'D' && s[1] == 'C' && s[2] == 'D' &&
                            s[3] == 'e' && s[4] == 'p' && s[5] == 't' && s[6] == 'h' &&
                            s[7] == 'B' && s[8] == 'u' && s[9] == 'f' && s[10] == 'A' &&
                            s[11] == 'l' && s[12] == 'l' && s[13] == 'o' && s[14] == 'c');
            int is_dcdbr = (len == 15 && s[0] == 'D' && s[1] == 'C' && s[2] == 'D' &&
                            s[3] == 'e' && s[4] == 'p' && s[5] == 't' && s[6] == 'h' &&
                            s[7] == 'B' && s[8] == 'u' && s[9] == 'f' && s[10] == 'R' &&
                            s[11] == 'e' && s[12] == 's' && s[13] == 'e' && s[14] == 't');
            int is_puc = (len == 10 && s[0] == 'P' && s[1] == 'o' && s[2] == 'p' &&
                          s[3] == 'U' && s[4] == 'p' && s[5] == 'C' && s[6] == 'o' &&
                          s[7] == 'l' && s[8] == 'o' && s[9] == 'r');
            int is_mg = (len == 10 && s[0] == 'M' && s[1] == 'e' && s[2] == 's' &&
                         s[3] == 's' && s[4] == 'a' && s[5] == 'g' && s[6] == 'e' &&
                         s[7] == 'G' && s[8] == 'e' && s[9] == 't');
            int is_mqp = (len == 10 && s[0] == 'M' && s[1] == 's' && s[2] == 'g' &&
                          s[3] == 'Q' && s[4] == 'u' && s[5] == 'e' && s[6] == 'P' &&
                          s[7] == 'u' && s[8] == 's' && s[9] == 'h');
            int is_mpush = (len == 8 && s[0] == 'M' && s[1] == 'e' && s[2] == 'n' &&
                            s[3] == 'u' && s[4] == 'P' && s[5] == 'u' && s[6] == 's' &&
                            s[7] == 'h');
            int is_mpop = (len == 7 && s[0] == 'M' && s[1] == 'e' && s[2] == 'n' &&
                           s[3] == 'u' && s[4] == 'P' && s[5] == 'o' && s[6] == 'p');
            int is_mef = (len == 13 && s[0] == 'M' && s[1] == 'e' && s[2] == 'n' &&
                          s[3] == 'u' && s[4] == 'E' && s[5] == 'n' && s[6] == 't' &&
                          s[7] == 'r' && s[8] == 'y' && s[9] == 'F' && s[10] == 'i' &&
                          s[11] == 'n' && s[12] == 'd');
            int is_min = (len == 3 && s[0] == 'M' && s[1] == 'i' && s[2] == 'n');
            int is_max = (len == 3 && s[0] == 'M' && s[1] == 'a' && s[2] == 'x');
            int is_sgn = (len == 4 && s[0] == 'S' && s[1] == 'i' && s[2] == 'g' && s[3] == 'n');
            int is_clp = (len == 5 && s[0] == 'C' && s[1] == 'l' && s[2] == 'a' &&
                          s[3] == 'm' && s[4] == 'p');
            int is_sqr = (len == 3 && s[0] == 'S' && s[1] == 'q' && s[2] == 'r');
            int is_pp = (len == 8 && s[0] == 'P' && s[1] == 'u' && s[2] == 't' &&
                         s[3] == 'P' && s[4] == 'i' && s[5] == 'x' && s[6] == 'e' &&
                         s[7] == 'l');
            int is_gpl = (len == 6 && s[0] == 'G' && s[1] == 'r' && s[2] == 'P' &&
                          s[3] == 'l' && s[4] == 'o' && s[5] == 't');
            int is_gpl3 = (len == 7 && s[0] == 'G' && s[1] == 'r' && s[2] == 'P' &&
                           s[3] == 'l' && s[4] == 'o' && s[5] == 't' && s[6] == '3');
            int is_gpk = (len == 6 && s[0] == 'G' && s[1] == 'r' && s[2] == 'P' &&
                          s[3] == 'e' && s[4] == 'e' && s[5] == 'k');
            int is_cls = (len == 3 && s[0] == 'C' && s[1] == 'l' && s[2] == 's');
            int is_fr = (len == 8 && s[0] == 'F' && s[1] == 'i' && s[2] == 'l' &&
                         s[3] == 'l' && s[4] == 'R' && s[5] == 'e' && s[6] == 'c' &&
                         s[7] == 't');
            int is_gl = (len == 6 && s[0] == 'G' && s[1] == 'r' && s[2] == 'L' &&
                         s[3] == 'i' && s[4] == 'n' && s[5] == 'e');
            int is_gl3 = (len == 7 && s[0] == 'G' && s[1] == 'r' && s[2] == 'L' &&
                          s[3] == 'i' && s[4] == 'n' && s[5] == 'e' && s[6] == '3');
            int is_ghl = (len == 7 && s[0] == 'G' && s[1] == 'r' && s[2] == 'H' &&
                          s[3] == 'L' && s[4] == 'i' && s[5] == 'n' && s[6] == 'e');
            int is_gvl = (len == 7 && s[0] == 'G' && s[1] == 'r' && s[2] == 'V' &&
                          s[3] == 'L' && s[4] == 'i' && s[5] == 'n' && s[6] == 'e');
            int is_gc = (len == 8 && s[0] == 'G' && s[1] == 'r' && s[2] == 'C' &&
                         s[3] == 'i' && s[4] == 'r' && s[5] == 'c' && s[6] == 'l' &&
                         s[7] == 'e');
            int is_gfc = (len == 12 && s[0] == 'G' && s[1] == 'r' && s[2] == 'F' &&
                          s[3] == 'i' && s[4] == 'l' && s[5] == 'l' && s[6] == 'C' &&
                          s[7] == 'i' && s[8] == 'r' && s[9] == 'c' && s[10] == 'l' &&
                          s[11] == 'e');
            int is_pc = (len == 7 && s[0] == 'P' && s[1] == 'u' && s[2] == 't' &&
                         s[3] == 'C' && s[4] == 'h' && s[5] == 'a' && s[6] == 'r');
            int is_pr = (len == 5 && s[0] == 'P' && s[1] == 'r' && s[2] == 'i' &&
                         s[3] == 'n' && s[4] == 't');
            int is_pa = (len == 7 && s[0] == 'P' && s[1] == 'r' && s[2] == 'i' &&
                         s[3] == 'n' && s[4] == 't' && s[5] == 'A' && s[6] == 't');
            int is_grp = (len == 7 && s[0] == 'G' && s[1] == 'r' && s[2] == 'P' &&
                          s[3] == 'r' && s[4] == 'i' && s[5] == 'n' && s[6] == 't');
            int is_pi = (len == 8 && s[0] == 'P' && s[1] == 'r' && s[2] == 'i' &&
                         s[3] == 'n' && s[4] == 't' && s[5] == 'I' && s[6] == '6' &&
                         s[7] == '4');
            int is_sl = (len == 6 && s[0] == 'S' && s[1] == 't' && s[2] == 'r' &&
                         s[3] == 'L' && s[4] == 'e' && s[5] == 'n');
            int is_sc = (len == 6 && s[0] == 'S' && s[1] == 't' && s[2] == 'r' &&
                         s[3] == 'C' && s[4] == 'm' && s[5] == 'p');
            int is_scp = (len == 6 && s[0] == 'S' && s[1] == 't' && s[2] == 'r' &&
                          s[3] == 'C' && s[4] == 'p' && s[5] == 'y');
            int is_sct = (len == 6 && s[0] == 'S' && s[1] == 't' && s[2] == 'r' &&
                          s[3] == 'C' && s[4] == 'a' && s[5] == 't');
            int is_sn = (len == 6 && s[0] == 'S' && s[1] == 't' && s[2] == 'r' &&
                         s[3] == 'N' && s[4] == 'e' && s[5] == 'w');
            int is_cp = (len == 8 && s[0] == 'C' && s[1] == 'a' && s[2] == 't' &&
                         s[3] == 'P' && s[4] == 'r' && s[5] == 'i' && s[6] == 'n' &&
                         s[7] == 't');
            int is_spn = (len == 8 && s[0] == 'S' && s[1] == 't' && s[2] == 'r' &&
                          s[3] == 'P' && s[4] == 'r' && s[5] == 'i' && s[6] == 'n' &&
                          s[7] == 't');
            int is_msp = (len == 9 && s[0] == 'M' && s[1] == 'S' && s[2] == 't' &&
                          s[3] == 'r' && s[4] == 'P' && s[5] == 'r' && s[6] == 'i' &&
                          s[7] == 'n' && s[8] == 't');
            int is_s2i = (len == 7 && s[0] == 'S' && s[1] == 't' && s[2] == 'r' &&
                          s[3] == '2' && s[4] == 'I' && s[5] == '6' && s[6] == '4');
            int is_hs = (len == 7 && s[0] == 'H' && s[1] == 'a' && s[2] == 's' &&
                         s[3] == 'h' && s[4] == 'S' && s[5] == 't' && s[6] == 'r');
            int is_mc = (len == 6 && s[0] == 'M' && s[1] == 'e' && s[2] == 'm' &&
                         s[3] == 'C' && s[4] == 'p' && s[5] == 'y');
            int is_mcopy = (len == 7 && s[0] == 'M' && s[1] == 'e' && s[2] == 'm' &&
                            s[3] == 'C' && s[4] == 'o' && s[5] == 'p' && s[6] == 'y');
            int is_ms = (len == 6 && s[0] == 'M' && s[1] == 'e' && s[2] == 'm' &&
                         s[3] == 'S' && s[4] == 'e' && s[5] == 't');
            int is_ma = (len == 6 && s[0] == 'M' && s[1] == 'A' && s[2] == 'l' &&
                         s[3] == 'l' && s[4] == 'o' && s[5] == 'c');
            int is_free = (len == 4 && s[0] == 'F' && s[1] == 'r' && s[2] == 'e' &&
                           s[3] == 'e');
            int is_mx = (len == 6 && s[0] == 'M' && s[1] == 'o' && s[2] == 'u' &&
                         s[3] == 's' && s[4] == 'e' && s[5] == 'X');
            int is_my = (len == 6 && s[0] == 'M' && s[1] == 'o' && s[2] == 'u' &&
                         s[3] == 's' && s[4] == 'e' && s[5] == 'Y');
            int is_mb = (len == 8 && s[0] == 'M' && s[1] == 'o' && s[2] == 'u' &&
                         s[3] == 's' && s[4] == 'e' && s[5] == 'B' && s[6] == 't' &&
                         s[7] == 'n');
            int is_mdx = (len == 7 && s[0] == 'M' && s[1] == 'o' && s[2] == 'u' &&
                          s[3] == 's' && s[4] == 'e' && s[5] == 'D' && s[6] == 'X');
            int is_mdy = (len == 7 && s[0] == 'M' && s[1] == 'o' && s[2] == 'u' &&
                          s[3] == 's' && s[4] == 'e' && s[5] == 'D' && s[6] == 'Y');
            int is_fbw = (len == 3 && s[0] == 'F' && s[1] == 'b' && s[2] == 'W');
            int is_fbh = (len == 3 && s[0] == 'F' && s[1] == 'b' && s[2] == 'H');
            int is_rnd = (len == 4 && s[0] == 'R' && s[1] == 'a' && s[2] == 'n' && s[3] == 'd');
            int is_slp = (len == 5 && s[0] == 'S' && s[1] == 'l' && s[2] == 'e' &&
                          s[3] == 'e' && s[4] == 'p');
            int is_cnt = (len == 3 && s[0] == 'C' && s[1] == 'n' && s[2] == 't');
            int is_cf = (len == 6 && s[0] == 'C' && s[1] == 'n' && s[2] == 't' &&
                         s[3] == 'F' && s[4] == 'r' && s[5] == 'q');
            int is_kh = (len == 6 && s[0] == 'K' && s[1] == 'e' && s[2] == 'y' &&
                         s[3] == 'H' && s[4] == 'i' && s[5] == 't');
            int is_gk = (len == 6 && s[0] == 'G' && s[1] == 'e' && s[2] == 't' &&
                         s[3] == 'K' && s[4] == 'e' && s[5] == 'y');
            int is_c0 = (len == 5 && s[0] == 'C' && s[1] == 'a' && s[2] == 'l' &&
                         s[3] == 'l' && s[4] == '0');
            int is_c1 = (len == 5 && s[0] == 'C' && s[1] == 'a' && s[2] == 'l' &&
                         s[3] == 'l' && s[4] == '1');
            int is_bw = (len == 8 && s[0] == 'B' && s[1] == 'l' && s[2] == 'k' &&
                         s[3] == 'W' && s[4] == 'r' && s[5] == 'i' && s[6] == 't' &&
                         s[7] == 'e');
            int is_brd = (len == 7 && s[0] == 'B' && s[1] == 'l' && s[2] == 'k' &&
                          s[3] == 'R' && s[4] == 'e' && s[5] == 'a' && s[6] == 'd');
            if (is_toi) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                if (f->expr_f64) {
                    if (hcf_emit(f, HC_F2I) < 0) {
                        return -1;
                    }
                }
                f->expr_f64 = 0;
                return 0;
            }
            if (is_tof) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                if (!f->expr_f64) {
                    if (hcf_emit(f, HC_I2F) < 0) {
                        return -1;
                    }
                }
                f->expr_f64 = 1;
                return 0;
            }
            if (is_cos || is_sin) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                if (!f->expr_f64) {
                    if (hcf_emit(f, HC_I2F) < 0) {
                        return -1;
                    }
                }
                f->n += hc_pack_call1(f->bc + f->n,
                                      (uint64_t)(uintptr_t)(is_cos ? hc_builtin_cos : hc_builtin_sin));
                f->expr_f64 = 1;
                return 0;
            }
            if (is_sqrt) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                if (!f->expr_f64) {
                    if (hcf_emit(f, HC_I2F) < 0) {
                        return -1;
                    }
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_sqrt);
                f->expr_f64 = 1;
                return 0;
            }
            if (is_wrap) {
                if (nargs == 1) {
                    if (!f->expr_f64) {
                        if (hcf_emit(f, HC_I2F) < 0) {
                            return -1;
                        }
                    }
                    f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_wrap1);
                    f->expr_f64 = 1;
                    return 0;
                }
                if (nargs == 2) {
                    /* Both args should be F64; last expr_f64 is base. Promote if needed. */
                    if (!f->expr_f64) {
                        if (hcf_emit(f, HC_I2F) < 0) {
                            return -1;
                        }
                    }
                    f->n += hc_pack_call2(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_wrap2);
                    f->expr_f64 = 1;
                    return 0;
                }
                f->err = 1;
                return -1;
            }
            if (is_abs || is_abs64) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                /* Lattice TurtleMove: AbsI64(t->w + 1) — F64 arg truncates like HolyC. */
                if (f->expr_f64) {
                    if (hcf_emit(f, HC_F2I) < 0) {
                        return -1;
                    }
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_abs);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_arg) {
                if (nargs != 2) {
                    f->err = 1;
                    return -1;
                }
                /* Promote last arg (y) if needed; x should already be F64 from expr. */
                if (!f->expr_f64) {
                    if (hcf_emit(f, HC_I2F) < 0) {
                        return -1;
                    }
                }
                f->n += hc_pack_call2(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_arg);
                f->expr_f64 = 1;
                return 0;
            }
            if (is_fabs) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                if (!f->expr_f64) {
                    if (hcf_emit(f, HC_I2F) < 0) {
                        return -1;
                    }
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_fabs);
                f->expr_f64 = 1;
                return 0;
            }
            if (is_dca) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_dcalias);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_ref) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_refresh);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_dcf) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_dcfill);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_dcd) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_dcdel);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_dcdba) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_dcdepthbufalloc);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_dcdbr) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_dcdepthbufreset);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_puc) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_popupcolor);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_mg) {
                if (nargs != 3) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_messageget);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_mqp) {
                if (nargs != 3) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_msgquepush);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_mpush) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_menupush);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_mpop) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_menupop);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_mef) {
                if (nargs != 2) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call2(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_menuentryfind);
                f->expr_f64 = 0;
                return 0;
            }
            if (is_min) {
                if (nargs != 2) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call2(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_min);
                return 0;
            }
            if (is_max) {
                if (nargs != 2) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call2(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_max);
                return 0;
            }
            if (is_sgn) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_sign);
                return 0;
            }
            if (is_clp) {
                if (nargs != 3) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_clamp);
                return 0;
            }
            if (is_sqr) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_sqr);
                return 0;
            }
            if (is_pp) {
                if (nargs != 3) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_putpixel);
                return 0;
            }
            if (is_gpl) {
                if (nargs != 3) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grplot);
                return 0;
            }
            if (is_gpl3) {
                if (nargs != 4) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call4(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grplot3);
                return 0;
            }
            if (is_gpk) {
                if (nargs == 2) {
                    f->n += hc_pack_call2(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grpeek);
                    return 0;
                }
                if (nargs == 3) {
                    /* Lattice GrPeek(dc,x,y) — dc ignored. */
                    f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grpeek_dc);
                    return 0;
                }
                f->err = 1;
                return -1;
            }
            if (is_cls) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_cls);
                return 0;
            }
            if (is_fr) {
                if (nargs != 5) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call5(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_fillrect);
                return 0;
            }
            if (is_gl) {
                if (nargs == 5) {
                    /* ZealOS Lattice: GrLine(dc,x1,y1,x2,y2) — color from dc->color. */
                    f->n += hc_pack_call5(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grline_dc);
                    return 0;
                }
                if (nargs == 6) {
                    /* Bring-up: GrLine(x1,y1,x2,y2,color,step). */
                    f->n += hc_pack_call6(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grline6);
                    return 0;
                }
                f->err = 1;
                return -1;
            }
            if (is_gl3) {
                if (nargs != 7) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call7(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grline3);
                return 0;
            }
            if (is_ghl) {
                if (nargs != 4) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call4(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grhline);
                return 0;
            }
            if (is_gvl) {
                if (nargs != 4) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call4(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grvline);
                return 0;
            }
            if (is_gc) {
                if (nargs != 4) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call4(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grcircle);
                return 0;
            }
            if (is_gfc) {
                if (nargs != 4) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call4(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grfillcircle);
                return 0;
            }
            if (is_pc) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_putchar);
                return 0;
            }
            if (is_pr) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_print);
                return 0;
            }
            if (is_pa) {
                if (nargs != 3) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_printat);
                return 0;
            }
            if (is_grp) {
                /* GrPrint(dc,x,y,fmt[,a…]) — %f via argv spill when nargs>4. */
                if (nargs < 4 || nargs > 9) {
                    f->err = 1;
                    return -1;
                }
                if (nargs == 4) {
                    f->n += hc_pack_call4(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grprint);
                } else {
                    int nfmt = nargs - 4;
                    if (f->mem_used + nfmt > HC_IR_MEM_WORDS) {
                        f->err = 1;
                        return -1;
                    }
                    uint8_t base = (uint8_t)f->mem_used;
                    f->mem_used += nfmt;
                    /* Stack top = last fmt arg; store into contiguous slots. */
                    for (int i = nfmt - 1; i >= 0; i--) {
                        if (hcf_emit_u8(f, HC_ST_SLOT, (uint8_t)(base + i)) < 0) {
                            return -1;
                        }
                    }
                    if (hcf_emit_u8(f, HC_SLOT_ADDR, base) < 0 ||
                        hcf_imm(f, (uint64_t)nfmt) < 0) {
                        return -1;
                    }
                    f->n += hc_pack_call6(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_grprintn);
                }
                f->expr_f64 = 0;
                return 0;
            }
            if (is_pi) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_printi64);
                return 0;
            }
            if (is_sl) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_strlen);
                return 0;
            }
            if (is_sc) {
                if (nargs != 2) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call2(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_strcmp);
                return 0;
            }
            if (is_scp) {
                if (nargs != 2) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call2(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_strcpy);
                return 0;
            }
            if (is_sct) {
                if (nargs != 2) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call2(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_strcat);
                return 0;
            }
            if (is_sn) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_strnew);
                return 0;
            }
            if (is_cp) {
                if (nargs != 4) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call4(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_catprint);
                return 0;
            }
            if (is_spn) {
                if (nargs < 2 || nargs > 4) {
                    f->err = 1;
                    return -1;
                }
                while (nargs < 4) {
                    if (hcf_imm(f, 0) < 0) {
                        return -1;
                    }
                    nargs++;
                }
                f->n += hc_pack_call4(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_strprint);
                return 0;
            }
            if (is_msp) {
                if (nargs != 3) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_mstrprint);
                return 0;
            }
            if (is_s2i) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_str2i64);
                return 0;
            }
            if (is_hs) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_hashstr);
                return 0;
            }
            if (is_mc || is_mcopy) {
                if (nargs != 3) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_memcpy);
                return 0;
            }
            if (is_ms) {
                if (nargs != 3) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_memset);
                return 0;
            }
            if (is_ma) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_malloc);
                return 0;
            }
            if (is_free) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_free);
                return 0;
            }
            if (is_mx) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_mousex);
                return 0;
            }
            if (is_my) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_mousey);
                return 0;
            }
            if (is_mb) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_mousebtn);
                return 0;
            }
            if (is_mdx) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_mousedx);
                return 0;
            }
            if (is_mdy) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_mousedy);
                return 0;
            }
            if (is_fbw) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_fbw);
                return 0;
            }
            if (is_fbh) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_fbh);
                return 0;
            }
            if (is_rnd) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_rand);
                return 0;
            }
            if (is_slp) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_sleep);
                return 0;
            }
            if (is_cnt) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_cnt);
                return 0;
            }
            if (is_cf) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_cntfrq);
                return 0;
            }
            if (is_kh) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_keyhit);
                return 0;
            }
            if (is_gk) {
                if (nargs != 0) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_getkey);
                return 0;
            }
            if (is_c0) {
                if (nargs != 1) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_call0);
                return 0;
            }
            if (is_c1) {
                if (nargs != 2) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call2(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_call1);
                return 0;
            }
            if (is_bw) {
                if (nargs != 3) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_blkwrite);
                return 0;
            }
            if (is_brd) {
                if (nargs != 3) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_blkread);
                return 0;
            }
            struct hc_fn *fn = hcf_find_fn(f, s, len);
            if (!fn || !fn->code || nargs != fn->nargs) {
                f->err = 1;
                return -1;
            }
            if (nargs == 0) {
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)fn->code);
            } else if (nargs == 1) {
                f->n += hc_pack_call1(f->bc + f->n, (uint64_t)(uintptr_t)fn->code);
            } else if (nargs == 2) {
                f->n += hc_pack_call2(f->bc + f->n, (uint64_t)(uintptr_t)fn->code);
            } else if (nargs == 3) {
                f->n += hc_pack_call3(f->bc + f->n, (uint64_t)(uintptr_t)fn->code);
            } else if (nargs == 4) {
                f->n += hc_pack_call4(f->bc + f->n, (uint64_t)(uintptr_t)fn->code);
            } else if (nargs == 5) {
                f->n += hc_pack_call5(f->bc + f->n, (uint64_t)(uintptr_t)fn->code);
            } else if (nargs == 6) {
                f->n += hc_pack_call6(f->bc + f->n, (uint64_t)(uintptr_t)fn->code);
            } else {
                f->err = 1;
                return -1;
            }
            /* User fns return I64 for now; clear F64 arg leak so `f(0.1)==7` is I64 cmp. */
            f->expr_f64 = 0;
            return 0;
        }
        int id = hcf_lookup(f, s, len);
        if (id < 0) {
            int ei = hcf_find_enum(f, s, len);
            if (ei >= 0) {
                return hcf_imm(f, (uint64_t)f->enums[ei].val);
            }
            /* Lattice: bare SetMenu; / TurtleInit; — 0-arg user fn without (). */
            {
                struct hc_fn *bfn = hcf_find_fn(f, s, len);
                if (bfn && bfn->code && bfn->nargs == 0) {
                    if (f->n + 9 > f->cap) {
                        f->err = 1;
                        return -1;
                    }
                    f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)bfn->code);
                    f->expr_f64 = 0;
                    return 0;
                }
            }
            /* Bare DCAlias — HolyC default-arg call without (). */
            if (len == 7 && s[0] == 'D' && s[1] == 'C' && s[2] == 'A' &&
                s[3] == 'l' && s[4] == 'i' && s[5] == 'a' && s[6] == 's') {
                if (f->n + 9 > f->cap) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_dcalias);
                f->expr_f64 = 0;
                return 0;
            }
            /* Bare Refresh — call Fs->draw_it via host. */
            if (len == 7 && s[0] == 'R' && s[1] == 'e' && s[2] == 'f' &&
                s[3] == 'r' && s[4] == 'e' && s[5] == 's' && s[6] == 'h') {
                if (f->n + 9 > f->cap) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_refresh);
                f->expr_f64 = 0;
                return 0;
            }
            /* Bare MenuPop — Lattice MenuPop; */
            if (len == 7 && s[0] == 'M' && s[1] == 'e' && s[2] == 'n' && s[3] == 'u' &&
                s[4] == 'P' && s[5] == 'o' && s[6] == 'p') {
                if (f->n + 9 > f->cap) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_menupop);
                f->expr_f64 = 0;
                return 0;
            }
            /* Bare SettingsPush / WinMax / DocClear / PutExcept / … */
            if (hcf_lattice_nop0(s, len)) {
                if (f->n + 9 > f->cap) {
                    f->err = 1;
                    return -1;
                }
                f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_nop0);
                f->expr_f64 = 0;
                return 0;
            }
            /* Fs->pix_width / Fs->pix_height — Lattice TurtleInit centering. */
            if (len == 2 && s[0] == 'F' && s[1] == 's') {
                hcf_skip(f);
                if (!(f->p[0] == '-' && f->p[1] == '>')) {
                    f->err = 1;
                    return -1;
                }
                f->p += 2;
                hcf_skip(f);
                if (!hcf_ident_start(*f->p)) {
                    f->err = 1;
                    return -1;
                }
                const char *ms = f->p;
                while (hcf_ident_char(*f->p)) {
                    f->p++;
                }
                int mlen = (int)(f->p - ms);
                if (f->n + 9 > f->cap) {
                    f->err = 1;
                    return -1;
                }
                if (mlen == 9 && ms[0] == 'p' && ms[1] == 'i' && ms[2] == 'x' &&
                    ms[3] == '_' && ms[4] == 'w' && ms[5] == 'i' && ms[6] == 'd' &&
                    ms[7] == 't' && ms[8] == 'h') {
                    f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_fs_pix_width);
                    f->expr_f64 = 0;
                    return 0;
                }
                if (mlen == 10 && ms[0] == 'p' && ms[1] == 'i' && ms[2] == 'x' &&
                    ms[3] == '_' && ms[4] == 'h' && ms[5] == 'e' && ms[6] == 'i' &&
                    ms[7] == 'g' && ms[8] == 'h' && ms[9] == 't') {
                    f->n += hc_pack_call0(f->bc + f->n, (uint64_t)(uintptr_t)hc_builtin_fs_pix_height);
                    f->expr_f64 = 0;
                    return 0;
                }
                /* Fs->cur_menu / win_inhibit / draw_it — stub task fields. */
                if ((mlen == 8 && ms[0] == 'c' && ms[1] == 'u' && ms[2] == 'r' &&
                     ms[3] == '_' && ms[4] == 'm' && ms[5] == 'e' && ms[6] == 'n' &&
                     ms[7] == 'u') ||
                    (mlen == 11 && ms[0] == 'w' && ms[1] == 'i' && ms[2] == 'n' &&
                     ms[3] == '_' && ms[4] == 'i' && ms[5] == 'n' && ms[6] == 'h' &&
                     ms[7] == 'i' && ms[8] == 'b' && ms[9] == 'i' && ms[10] == 't') ||
                    (mlen == 7 && ms[0] == 'd' && ms[1] == 'r' && ms[2] == 'a' &&
                     ms[3] == 'w' && ms[4] == '_' && ms[5] == 'i' && ms[6] == 't')) {
                    uint64_t addr = (uint64_t)(uintptr_t)&g_hc_fs_cur_menu;
                    if (mlen == 11) {
                        addr = (uint64_t)(uintptr_t)&g_hc_fs_win_inhibit;
                    } else if (mlen == 7) {
                        addr = (uint64_t)(uintptr_t)&g_hc_fs_draw_it;
                    }
                    if (hcf_imm(f, addr) < 0 || hcf_emit(f, HC_LOAD) < 0) {
                        return -1;
                    }
                    f->expr_f64 = 0;
                    return 0;
                }
                f->err = 1;
                return -1;
            }
            /* ZealOS 16-color palette (KernelA.HH) — after enums so Color{RED=…} wins. */
            {
                static const struct {
                    const char *n;
                    int len;
                    uint64_t v;
                } cols[] = {
                    {"BLACK", 5, 0},   {"BLUE", 4, 1},    {"GREEN", 5, 2},   {"CYAN", 4, 3},
                    {"RED", 3, 4},     {"PURPLE", 6, 5},  {"BROWN", 5, 6},   {"LTGRAY", 6, 7},
                    {"DKGRAY", 6, 8},  {"LTBLUE", 6, 9},  {"LTGREEN", 7, 10},{"LTCYAN", 6, 11},
                    {"LTRED", 5, 12},  {"LTPURPLE", 8, 13},{"YELLOW", 6, 14}, {"WHITE", 5, 15},
                };
                for (int ci = 0; ci < (int)(sizeof cols / sizeof cols[0]); ci++) {
                    if (len == cols[ci].len) {
                        int ok = 1;
                        for (int k = 0; k < len; k++) {
                            if (s[k] != cols[ci].n[k]) {
                                ok = 0;
                                break;
                            }
                        }
                        if (ok) {
                            return hcf_imm(f, cols[ci].v);
                        }
                    }
                }
            }
            /* KernelA.HH MESSAGE_* — Lattice MessageGet masks. */
            {
                static const struct {
                    const char *n;
                    int len;
                    uint64_t v;
                }                 msgs[] = {
                    {"MESSAGE_NULL", 12, 0},
                    {"MESSAGE_KEY_DOWN", 16, 2},
                    {"MESSAGE_KEY_UP", 14, 3},
                    {"MESSAGE_MS_MOVE", 15, 4},
                    {"MESSAGE_MS_L_DOWN", 17, 5},
                    {"MESSAGE_MS_L_UP", 15, 6},
                    {"MESSAGE_MS_R_DOWN", 17, 7},
                    {"MESSAGE_MS_R_UP", 15, 8},
                };
                for (int mi = 0; mi < (int)(sizeof msgs / sizeof msgs[0]); mi++) {
                    if (len == msgs[mi].len) {
                        int ok = 1;
                        for (int k = 0; k < len; k++) {
                            if (s[k] != msgs[mi].n[k]) {
                                ok = 0;
                                break;
                            }
                        }
                        if (ok) {
                            return hcf_imm(f, msgs[mi].v);
                        }
                    }
                }
            }
            /* KernelA.HH WIF_* / WIG_* (Lattice Fs->win_inhibit). */
            {
                static const struct {
                    const char *n;
                    int len;
                    uint64_t v;
                } wins[] = {
                    {"WIF_SELF_FOCUS", 14, 0x0001},
                    {"WIF_SELF_GRAB_SCROLL", 20, 0x0200},
                    {"WIF_FOCUS_TASK_MENU", 19, 0x00020000},
                    {"WIG_TASK_DEFAULT", 16, 0x0052f3ff},
                    {"CH_SHIFT_ESC", 12, 0x1C},
                    {"CH_ESC", 6, 0x1B},
                    {"CH_SPACE", 8, 0x20},
                    {"SC_CURSOR_UP", 12, 0x48},
                    {"SC_CURSOR_DOWN", 14, 0x50},
                    {"SC_CURSOR_LEFT", 14, 0x4B},
                    {"SC_CURSOR_RIGHT", 15, 0x4D},
                    {"STR_LEN", 7, 136},
                };
                for (int wi = 0; wi < (int)(sizeof wins / sizeof wins[0]); wi++) {
                    if (len == wins[wi].len) {
                        int ok = 1;
                        for (int k = 0; k < len; k++) {
                            if (s[k] != wins[wi].n[k]) {
                                ok = 0;
                                break;
                            }
                        }
                        if (ok) {
                            return hcf_imm(f, wins[wi].v);
                        }
                    }
                }
            }
            f->err = 1;
            return -1;
        }
        hcf_skip(f);
        if (*f->p == '[') {
            f->p++;
            if (hcf_parse_index_addr(f, id) < 0 || hcf_emit_load_at(f, id) < 0) {
                return -1;
            }
            f->expr_f64 = f->is_f64[id] ? 1 : 0;
            return 0;
        }
        if (*f->p == '.') {
            f->p++;
            hcf_skip(f);
            /* HolyC integer views: arg2.u8[0] (Lattice scan codes). */
            if (f->type[id] == HC_TYPE_NONE && !f->is_ptr[id] && f->count[id] <= 1 &&
                hcf_ident_start(*f->p)) {
                const char *ms = f->p;
                while (hcf_ident_char(*f->p)) {
                    f->p++;
                }
                int mlen = (int)(f->p - ms);
                int bits = 0;
                int is_signed = 0;
                if (mlen == 2 && ms[0] == 'u' && ms[1] == '8') {
                    bits = 8;
                } else if (mlen == 2 && ms[0] == 'i' && ms[1] == '8') {
                    bits = 8;
                    is_signed = 1;
                } else if (mlen == 3 && ms[0] == 'u' && ms[1] == '1' && ms[2] == '6') {
                    bits = 16;
                } else if (mlen == 3 && ms[0] == 'i' && ms[1] == '1' && ms[2] == '6') {
                    bits = 16;
                    is_signed = 1;
                } else if (mlen == 3 && ms[0] == 'u' && ms[1] == '3' && ms[2] == '2') {
                    bits = 32;
                } else if (mlen == 3 && ms[0] == 'i' && ms[1] == '3' && ms[2] == '2') {
                    bits = 32;
                    is_signed = 1;
                }
                hcf_skip(f);
                if (bits && *f->p == '[') {
                    f->p++;
                    if (hcf_ld_named(f, id) < 0 || hcf_parse_expr(f) < 0 ||
                        hcf_expect(f, ']') < 0 || hcf_imm(f, (uint64_t)bits) < 0 ||
                        hcf_emit(f, HC_MUL) < 0 || hcf_emit(f, HC_SHR) < 0 ||
                        hcf_imm(f, bits == 8 ? 0xffULL : (bits == 16 ? 0xffffULL : 0xffffffffULL)) <
                            0 ||
                        hcf_emit(f, HC_AND) < 0) {
                        return -1;
                    }
                    if (is_signed) {
                        /* Sign-extend via (x ^ sign) - sign with sign = 1<<(bits-1). */
                        uint64_t sign = 1ULL << (bits - 1);
                        if (hcf_imm(f, sign) < 0 || hcf_emit(f, HC_XOR) < 0 ||
                            hcf_imm(f, sign) < 0 || hcf_emit(f, HC_SUB) < 0) {
                            return -1;
                        }
                    }
                    f->expr_f64 = 0;
                    return 0;
                }
                f->p = ms;
            }
            int st = 8, ty = HC_TYPE_NONE, isf = 0;
            if (hcf_parse_dot_addr(f, id, &st, &ty, &isf) < 0) {
                return -1;
            }
            if (ty != HC_TYPE_NONE) {
                return 0; /* nested object address */
            }
            if (hcf_emit(f, (st == 1 ? HC_LOAD8 : (st == 4 ? HC_LOAD32 : HC_LOAD))) < 0) {
                return -1;
            }
            f->expr_f64 = isf ? 1 : 0;
            return 0;
        }
        if (f->p[0] == '-' && f->p[1] == '>') {
            f->p += 2;
            int st = 8, ty = HC_TYPE_NONE, isf = 0;
            if (hcf_parse_arrow_addr(f, id, &st, &ty, &isf) < 0) {
                return -1;
            }
            if (ty != HC_TYPE_NONE) {
                return 0;
            }
            if (hcf_emit(f, (st == 1 ? HC_LOAD8 : (st == 4 ? HC_LOAD32 : HC_LOAD))) < 0) {
                return -1;
            }
            f->expr_f64 = isf ? 1 : 0;
            return 0;
        }
        if (f->is_ptr[id]) {
            return hcf_ld_named(f, id);
        }
        if (f->count[id] > 1 || f->type[id] != HC_TYPE_NONE) {
            return hcf_emit_var_addr(f, id);
        }
        return hcf_ld_named(f, id);
    }
    f->err = 1;
    return -1;
}

static int hcf_parse_term(struct hc_front *f) {
    if (hcf_parse_prim(f) < 0) {
        return -1;
    }
    for (;;) {
        hcf_skip(f);
        uint8_t op = 0;
        if (f->p[0] == '*' && f->p[1] != '=') {
            op = HC_MUL;
            f->p++;
        } else if (f->p[0] == '/' && f->p[1] != '=') {
            op = HC_DIV;
            f->p++;
        } else if (f->p[0] == '%' && f->p[1] != '=') {
            op = HC_MOD;
            f->p++;
        } else {
            break;
        }
        int left_f = f->expr_f64;
        if (hcf_parse_prim(f) < 0) {
            return -1;
        }
        if (left_f && f->expr_f64) {
            uint8_t fop = (op == HC_MUL) ? HC_FMUL : (op == HC_DIV) ? HC_FDIV : 0;
            if (!fop || hcf_emit(f, fop) < 0) {
                f->err = 1;
                return -1;
            }
            f->expr_f64 = 1;
        } else if (left_f || f->expr_f64) {
            /* Lattice: TURTLE_SIZE * Cos(...) — promote I64 side via I2F. */
            if (op != HC_MUL && op != HC_DIV) {
                f->err = 1;
                return -1;
            }
            if (!left_f) {
                if (hcf_emit(f, HC_SWAP) < 0 || hcf_emit(f, HC_I2F) < 0 ||
                    hcf_emit(f, HC_SWAP) < 0) {
                    return -1;
                }
            } else if (hcf_emit(f, HC_I2F) < 0) {
                return -1;
            }
            if (hcf_emit(f, op == HC_MUL ? HC_FMUL : HC_FDIV) < 0) {
                return -1;
            }
            f->expr_f64 = 1;
        } else if (hcf_emit(f, op) < 0) {
            return -1;
        } else {
            f->expr_f64 = 0;
        }
    }
    return 0;
}

static int hcf_parse_add(struct hc_front *f) {
    if (hcf_parse_term(f) < 0) {
        return -1;
    }
    for (;;) {
        hcf_skip(f);
        char c = *f->p;
        if (c != '+' && c != '-') {
            break;
        }
        f->p++;
        int left_f = f->expr_f64;
        if (hcf_parse_term(f) < 0) {
            return -1;
        }
        if (left_f && f->expr_f64) {
            if (hcf_emit(f, c == '+' ? HC_FADD : HC_FSUB) < 0) {
                return -1;
            }
            f->expr_f64 = 1;
        } else if (left_f || f->expr_f64) {
            /* Promote I64 side (Lattice w + Cos(...), etc.). */
            if (!left_f) {
                if (hcf_emit(f, HC_SWAP) < 0 || hcf_emit(f, HC_I2F) < 0 ||
                    hcf_emit(f, HC_SWAP) < 0) {
                    return -1;
                }
            } else if (hcf_emit(f, HC_I2F) < 0) {
                return -1;
            }
            if (hcf_emit(f, c == '+' ? HC_FADD : HC_FSUB) < 0) {
                return -1;
            }
            f->expr_f64 = 1;
        } else if (hcf_emit(f, c == '+' ? HC_ADD : HC_SUB) < 0) {
            return -1;
        } else {
            f->expr_f64 = 0;
        }
    }
    return 0;
}

/* C/HolyC: << >> bind tighter than & ^ | — Lattice MessageGet masks. */
static int hcf_parse_shift(struct hc_front *f) {
    if (hcf_parse_add(f) < 0) {
        return -1;
    }
    for (;;) {
        hcf_skip(f);
        uint8_t op = 0;
        if (f->p[0] == '<' && f->p[1] == '<') {
            op = HC_SHL;
            f->p += 2;
        } else if (f->p[0] == '>' && f->p[1] == '>') {
            op = HC_ASR;
            f->p += 2;
        } else {
            break;
        }
        if (f->expr_f64 || hcf_parse_add(f) < 0 || f->expr_f64 || hcf_emit(f, op) < 0) {
            f->err = 1;
            return -1;
        }
        f->expr_f64 = 0;
    }
    return 0;
}

static int hcf_parse_band(struct hc_front *f) {
    if (hcf_parse_shift(f) < 0) {
        return -1;
    }
    for (;;) {
        hcf_skip(f);
        if (!(f->p[0] == '&' && f->p[1] != '&')) {
            break;
        }
        f->p++;
        if (f->expr_f64 || hcf_parse_shift(f) < 0 || f->expr_f64 || hcf_emit(f, HC_AND) < 0) {
            f->err = 1;
            return -1;
        }
        f->expr_f64 = 0;
    }
    return 0;
}

static int hcf_parse_bxor(struct hc_front *f) {
    if (hcf_parse_band(f) < 0) {
        return -1;
    }
    for (;;) {
        hcf_skip(f);
        if (*f->p != '^') {
            break;
        }
        f->p++;
        if (f->expr_f64 || hcf_parse_band(f) < 0 || f->expr_f64 || hcf_emit(f, HC_XOR) < 0) {
            f->err = 1;
            return -1;
        }
        f->expr_f64 = 0;
    }
    return 0;
}

static int hcf_parse_bor(struct hc_front *f) {
    if (hcf_parse_bxor(f) < 0) {
        return -1;
    }
    for (;;) {
        hcf_skip(f);
        if (!(f->p[0] == '|' && f->p[1] != '|')) {
            break;
        }
        f->p++;
        if (f->expr_f64 || hcf_parse_bxor(f) < 0 || f->expr_f64 || hcf_emit(f, HC_OR) < 0) {
            f->err = 1;
            return -1;
        }
        f->expr_f64 = 0;
    }
    return 0;
}

static int hcf_parse_cmp(struct hc_front *f) {
    if (hcf_parse_bor(f) < 0) {
        return -1;
    }
    hcf_skip(f);
    uint8_t cmp = 0;
    if (f->p[0] == '=' && f->p[1] == '=') {
        cmp = HC_EQ;
        f->p += 2;
    } else if (f->p[0] == '!' && f->p[1] == '=') {
        cmp = HC_NE;
        f->p += 2;
    } else if (f->p[0] == '<' && f->p[1] == '=') {
        cmp = HC_LE;
        f->p += 2;
    } else if (f->p[0] == '>' && f->p[1] == '=') {
        cmp = HC_GE;
        f->p += 2;
    } else if (f->p[0] == '<' && f->p[1] != '<') {
        cmp = HC_LT;
        f->p++;
    } else if (f->p[0] == '>' && f->p[1] != '>') {
        cmp = HC_GT;
        f->p++;
    } else {
        return 0;
    }
    int left_f = f->expr_f64;
    if (hcf_parse_bor(f) < 0) {
        return -1;
    }
    if (left_f && f->expr_f64) {
        uint8_t fcmp = 0;
        if (cmp == HC_EQ) {
            fcmp = HC_FEQ;
        } else if (cmp == HC_NE) {
            fcmp = HC_FNE;
        } else if (cmp == HC_LT) {
            fcmp = HC_FLT;
        } else if (cmp == HC_LE) {
            fcmp = HC_FLE;
        } else if (cmp == HC_GT) {
            fcmp = HC_FGT;
        } else if (cmp == HC_GE) {
            fcmp = HC_FGE;
        } else {
            f->err = 1;
            return -1;
        }
        if (hcf_emit(f, fcmp) < 0) {
            return -1;
        }
        f->expr_f64 = 0; /* compare yields I64 */
        return 0;
    }
    if (left_f || f->expr_f64) {
        /* Lattice: if (w < 0) / if (y2 >= 0) — promote I64 side via I2F. */
        uint8_t fcmp = 0;
        if (cmp == HC_EQ) {
            fcmp = HC_FEQ;
        } else if (cmp == HC_NE) {
            fcmp = HC_FNE;
        } else if (cmp == HC_LT) {
            fcmp = HC_FLT;
        } else if (cmp == HC_LE) {
            fcmp = HC_FLE;
        } else if (cmp == HC_GT) {
            fcmp = HC_FGT;
        } else if (cmp == HC_GE) {
            fcmp = HC_FGE;
        } else {
            f->err = 1;
            return -1;
        }
        if (!left_f) {
            if (hcf_emit(f, HC_SWAP) < 0 || hcf_emit(f, HC_I2F) < 0 ||
                hcf_emit(f, HC_SWAP) < 0) {
                return -1;
            }
        } else if (hcf_emit(f, HC_I2F) < 0) {
            return -1;
        }
        if (hcf_emit(f, fcmp) < 0) {
            return -1;
        }
        f->expr_f64 = 0;
        return 0;
    }
    f->expr_f64 = 0;
    return hcf_emit(f, cmp) < 0 ? -1 : 0;
}

static int hcf_parse_land(struct hc_front *f) {
    if (hcf_parse_cmp(f) < 0) {
        return -1;
    }
    for (;;) {
        hcf_skip(f);
        if (!(f->p[0] == '&' && f->p[1] == '&')) {
            break;
        }
        f->p += 2;
        if (f->mem_used >= HC_IR_MEM_WORDS) {
            f->err = 1;
            return -1;
        }
        uint8_t tmp = (uint8_t)f->mem_used++;
        int l_false = hcf_new_label(f), l_end = hcf_new_label(f);
        if (l_false < 0 || l_end < 0) {
            return -1;
        }
        /* JZ pops; both arms ST_SLOT so merge keeps IR sp balanced. */
        if (hcf_emit_u8(f, HC_JZ, (uint8_t)l_false) < 0 || hcf_parse_cmp(f) < 0 ||
            hcf_emit_u8(f, HC_JZ, (uint8_t)l_false) < 0 || hcf_imm(f, 1) < 0 ||
            hcf_emit_u8(f, HC_ST_SLOT, tmp) < 0 || hcf_emit_u8(f, HC_JMP, (uint8_t)l_end) < 0 ||
            hcf_emit_u8(f, HC_LABEL, (uint8_t)l_false) < 0 || hcf_imm(f, 0) < 0 ||
            hcf_emit_u8(f, HC_ST_SLOT, tmp) < 0 || hcf_emit_u8(f, HC_LABEL, (uint8_t)l_end) < 0 ||
            hcf_emit_u8(f, HC_LD_SLOT, tmp) < 0) {
            return -1;
        }
    }
    return 0;
}

static int hcf_parse_lor(struct hc_front *f) {
    if (hcf_parse_land(f) < 0) {
        return -1;
    }
    for (;;) {
        hcf_skip(f);
        if (!(f->p[0] == '|' && f->p[1] == '|')) {
            break;
        }
        f->p += 2;
        if (f->mem_used >= HC_IR_MEM_WORDS) {
            f->err = 1;
            return -1;
        }
        uint8_t tmp = (uint8_t)f->mem_used++;
        int l_true = hcf_new_label(f), l_end = hcf_new_label(f);
        if (l_true < 0 || l_end < 0) {
            return -1;
        }
        if (hcf_emit_u8(f, HC_JNZ, (uint8_t)l_true) < 0 || hcf_parse_land(f) < 0 ||
            hcf_emit_u8(f, HC_JNZ, (uint8_t)l_true) < 0 || hcf_imm(f, 0) < 0 ||
            hcf_emit_u8(f, HC_ST_SLOT, tmp) < 0 || hcf_emit_u8(f, HC_JMP, (uint8_t)l_end) < 0 ||
            hcf_emit_u8(f, HC_LABEL, (uint8_t)l_true) < 0 || hcf_imm(f, 1) < 0 ||
            hcf_emit_u8(f, HC_ST_SLOT, tmp) < 0 || hcf_emit_u8(f, HC_LABEL, (uint8_t)l_end) < 0 ||
            hcf_emit_u8(f, HC_LD_SLOT, tmp) < 0) {
            return -1;
        }
    }
    return 0;
}

static int hcf_parse_expr(struct hc_front *f) {
    /* Assignment-as-expression: `if (x = MenuEntryFind(...))` (Lattice). */
    hcf_skip(f);
    const char *save = f->p;
    if (hcf_ident_start(*f->p)) {
        const char *s = f->p;
        while (hcf_ident_char(*f->p)) {
            f->p++;
        }
        int len = (int)(f->p - s);
        int id = hcf_lookup(f, s, len);
        hcf_skip(f);
        if (id >= 0 && *f->p == '=' && f->p[1] != '=') {
            f->p++;
            if (hcf_parse_expr(f) < 0) {
                return -1;
            }
            if (f->is_f64[id] && !f->expr_f64) {
                if (hcf_emit(f, HC_I2F) < 0) {
                    return -1;
                }
                f->expr_f64 = 1;
            } else if (!f->is_f64[id] && f->expr_f64) {
                f->err = 1;
                return -1;
            }
            /* Leave RHS on stack after store. */
            if (hcf_emit(f, HC_DUP) < 0 || hcf_st_named(f, id) < 0) {
                return -1;
            }
            return 0;
        }
        /* Lattice: if (p->m = e) / if (o.m = e). Probe may emit; rewind if not `=`. */
        if (id >= 0 && ((f->p[0] == '-' && f->p[1] == '>') || *f->p == '.')) {
            size_t save_n = f->n;
            int save_mem = f->mem_used;
            int st = 8, ty = HC_TYPE_NONE, isf = 0;
            int is_arrow = (f->p[0] == '-');
            int ok = 0;
            if (is_arrow) {
                f->p += 2;
                ok = (hcf_parse_arrow_addr(f, id, &st, &ty, &isf) == 0);
            } else {
                f->p++;
                ok = (hcf_parse_dot_addr(f, id, &st, &ty, &isf) == 0);
            }
            hcf_skip(f);
            if (ok && ty == HC_TYPE_NONE && *f->p == '=' && f->p[1] != '=') {
                f->p++;
                if (hcf_parse_expr(f) < 0) {
                    return -1;
                }
                if (isf && !f->expr_f64) {
                    if (hcf_emit(f, HC_I2F) < 0) {
                        return -1;
                    }
                    f->expr_f64 = 1;
                } else if (!isf && f->expr_f64) {
                    f->err = 1;
                    return -1;
                }
                /* addr, val → keep val: ST tmp; LD tmp; STORE; LD tmp */
                if (f->mem_used >= HC_IR_MEM_WORDS) {
                    f->err = 1;
                    return -1;
                }
                uint8_t tmp = (uint8_t)f->mem_used++;
                int sop = (st == 1 ? HC_STORE8 : (st == 4 ? HC_STORE32 : HC_STORE));
                if (hcf_emit_u8(f, HC_ST_SLOT, tmp) < 0 || hcf_emit_u8(f, HC_LD_SLOT, tmp) < 0 ||
                    hcf_emit(f, (uint8_t)sop) < 0 || hcf_emit_u8(f, HC_LD_SLOT, tmp) < 0) {
                    return -1;
                }
                f->expr_f64 = isf ? 1 : 0;
                return 0;
            }
            f->n = save_n;
            f->mem_used = save_mem;
            f->err = 0;
            f->p = save;
        } else {
            f->p = save;
        }
    }
    if (hcf_parse_lor(f) < 0) {
        return -1;
    }
    hcf_skip(f);
    if (*f->p != '?') {
        return 0;
    }
    f->p++;
    /* Both arms leave a value; store through a temp so IR sp stays balanced. */
    if (f->mem_used >= HC_IR_MEM_WORDS) {
        f->err = 1;
        return -1;
    }
    uint8_t tmp = (uint8_t)f->mem_used++;
    int l_else = hcf_new_label(f), l_end = hcf_new_label(f);
    if (l_else < 0 || l_end < 0) {
        return -1;
    }
    if (hcf_emit_u8(f, HC_JZ, (uint8_t)l_else) < 0 || hcf_parse_expr(f) < 0 ||
        hcf_emit_u8(f, HC_ST_SLOT, tmp) < 0 || hcf_emit_u8(f, HC_JMP, (uint8_t)l_end) < 0 ||
        hcf_emit_u8(f, HC_LABEL, (uint8_t)l_else) < 0 || hcf_expect(f, ':') < 0 ||
        hcf_parse_expr(f) < 0 || hcf_emit_u8(f, HC_ST_SLOT, tmp) < 0 ||
        hcf_emit_u8(f, HC_LABEL, (uint8_t)l_end) < 0 || hcf_emit_u8(f, HC_LD_SLOT, tmp) < 0) {
        return -1;
    }
    return 0;
}

static int hcf_parse_block(struct hc_front *f) {
    if (hcf_expect(f, '{') < 0) {
        return -1;
    }
    for (;;) {
        hcf_skip(f);
        if (*f->p == '}') {
            f->p++;
            return 0;
        }
        if (!*f->p || hcf_parse_stmt(f) < 0) {
            return -1;
        }
    }
}

static int hcf_parse_stmt(struct hc_front *f) {
    hcf_skip(f);
    if (*f->p == '{') {
        return hcf_parse_block(f);
    }
    /* Optional const/volatile — storage still mutable in this subset. */
    for (;;) {
        if (hcf_kw(f, "const") || hcf_kw(f, "volatile")) {
            hcf_skip(f);
            continue;
        }
        break;
    }
    /* Lattice try { … } catch PutExcept; — no exceptions yet; parse as stmts. */
    if (hcf_kw(f, "try") || hcf_kw(f, "catch")) {
        return hcf_parse_stmt(f);
    }
    if (hcf_kw(f, "return")) {
        hcf_skip(f);
        if (*f->p == ';') {
            f->p++;
            return (hcf_imm(f, 0) < 0 || hcf_emit(f, HC_RET) < 0) ? -1 : 0;
        }
        if (hcf_parse_expr(f) < 0 || hcf_emit(f, HC_RET) < 0) {
            return -1;
        }
        return hcf_expect(f, ';');
    }
    if (hcf_kw(f, "if")) {
        int l_else = hcf_new_label(f), l_end = hcf_new_label(f);
        if (l_else < 0 || l_end < 0) {
            return -1;
        }
        if (hcf_expect(f, '(') < 0 || hcf_parse_expr(f) < 0 || hcf_expect(f, ')') < 0) {
            return -1;
        }
        /* Lattice `if (tt.w)` — F64 nonzero → I64 for JZ. */
        if (f->expr_f64) {
            if (hcf_emit(f, HC_F2I) < 0) {
                return -1;
            }
            f->expr_f64 = 0;
        }
        if (hcf_emit_u8(f, HC_JZ, (uint8_t)l_else) < 0 || hcf_parse_stmt(f) < 0) {
            return -1;
        }
        if (hcf_emit_u8(f, HC_JMP, (uint8_t)l_end) < 0 ||
            hcf_emit_u8(f, HC_LABEL, (uint8_t)l_else) < 0) {
            return -1;
        }
        if (hcf_kw(f, "else") && hcf_parse_stmt(f) < 0) {
            return -1;
        }
        return hcf_emit_u8(f, HC_LABEL, (uint8_t)l_end);
    }
    if (hcf_kw(f, "while")) {
        int l_top = hcf_new_label(f), l_end = hcf_new_label(f);
        if (l_top < 0 || l_end < 0) {
            return -1;
        }
        if (hcf_emit_u8(f, HC_LABEL, (uint8_t)l_top) < 0 || hcf_expect(f, '(') < 0 ||
            hcf_parse_expr(f) < 0 || hcf_expect(f, ')') < 0) {
            return -1;
        }
        if (hcf_emit_u8(f, HC_JZ, (uint8_t)l_end) < 0) {
            return -1;
        }
        if (f->nbrk >= HC_MAX_LOOPS || f->ncont >= HC_MAX_LOOPS) {
            f->err = 1;
            return -1;
        }
        f->brk_tgt[f->nbrk++] = l_end;
        f->cont_tgt[f->ncont++] = l_top;
        if (hcf_parse_stmt(f) < 0 || hcf_emit_u8(f, HC_JMP, (uint8_t)l_top) < 0) {
            return -1;
        }
        f->nbrk--;
        f->ncont--;
        return hcf_emit_u8(f, HC_LABEL, (uint8_t)l_end);
    }
    if (hcf_kw(f, "do")) {
        int l_top = hcf_new_label(f), l_cond = hcf_new_label(f), l_end = hcf_new_label(f);
        if (l_top < 0 || l_cond < 0 || l_end < 0) {
            return -1;
        }
        if (f->nbrk >= HC_MAX_LOOPS || f->ncont >= HC_MAX_LOOPS) {
            f->err = 1;
            return -1;
        }
        f->brk_tgt[f->nbrk++] = l_end;
        f->cont_tgt[f->ncont++] = l_cond;
        if (hcf_emit_u8(f, HC_LABEL, (uint8_t)l_top) < 0 || hcf_parse_stmt(f) < 0) {
            return -1;
        }
        if (!hcf_kw(f, "while") || hcf_expect(f, '(') < 0) {
            f->err = 1;
            return -1;
        }
        if (hcf_emit_u8(f, HC_LABEL, (uint8_t)l_cond) < 0 || hcf_parse_expr(f) < 0 ||
            hcf_expect(f, ')') < 0 || hcf_expect(f, ';') < 0) {
            return -1;
        }
        if (hcf_emit_u8(f, HC_JNZ, (uint8_t)l_top) < 0) {
            return -1;
        }
        f->nbrk--;
        f->ncont--;
        return hcf_emit_u8(f, HC_LABEL, (uint8_t)l_end);
    }
    if (hcf_kw(f, "for")) {
        /* for (init; cond; step) body → init; L: cond; JZ end; body; step; JMP L; end: */
        int l_top = hcf_new_label(f), l_end = hcf_new_label(f);
        if (l_top < 0 || l_end < 0 || hcf_expect(f, '(') < 0) {
            return -1;
        }
        hcf_skip(f);
        if (*f->p != ';') {
            /* One init statement (decl / assign / *p=); parse_stmt consumes ';'. */
            if (hcf_parse_stmt(f) < 0) {
                return -1;
            }
        } else {
            f->p++;
        }
        if (hcf_emit_u8(f, HC_LABEL, (uint8_t)l_top) < 0) {
            return -1;
        }
        hcf_skip(f);
        if (*f->p != ';') {
            if (hcf_parse_expr(f) < 0) {
                return -1;
            }
        } else if (hcf_imm(f, 1) < 0) {
            return -1;
        }
        if (hcf_expect(f, ';') < 0 || hcf_emit_u8(f, HC_JZ, (uint8_t)l_end) < 0) {
            return -1;
        }
        /* Step is deferred: emit body first, then step, via a forward JMP over step. */
        {
            int l_body = hcf_new_label(f), l_step = hcf_new_label(f);
            if (l_body < 0 || l_step < 0) {
                return -1;
            }
            if (hcf_emit_u8(f, HC_JMP, (uint8_t)l_body) < 0 ||
                hcf_emit_u8(f, HC_LABEL, (uint8_t)l_step) < 0) {
                return -1;
            }
            hcf_skip(f);
            if (*f->p != ')') {
                if (hcf_ident_start(*f->p)) {
                    const char *s = f->p;
                    while (hcf_ident_char(*f->p)) {
                        f->p++;
                    }
                    int len = (int)(f->p - s);
                    hcf_skip(f);
                    {
                        int id = hcf_lookup(f, s, len);
                        /* Lattice: for (i = 0; i < l; i++) / i-- */
                        if (id >= 0 && f->p[0] == '+' && f->p[1] == '+') {
                            f->p += 2;
                            if (hcf_ld_named(f, id) < 0 || hcf_imm(f, 1) < 0 ||
                                hcf_emit(f, HC_ADD) < 0 || hcf_st_named(f, id) < 0) {
                                return -1;
                            }
                        } else if (id >= 0 && f->p[0] == '-' && f->p[1] == '-') {
                            f->p += 2;
                            if (hcf_ld_named(f, id) < 0 || hcf_imm(f, 1) < 0 ||
                                hcf_emit(f, HC_SUB) < 0 || hcf_st_named(f, id) < 0) {
                                return -1;
                            }
                        } else {
                        int bin = hcf_parse_assign_op(f);
                        if (id < 0 || bin < 0) {
                            f->err = 0;
                            f->p = s;
                            if (hcf_parse_expr(f) < 0 || hcf_emit(f, HC_DROP) < 0) {
                                return -1;
                            }
                        } else if (bin) {
                            if (hcf_ld_named(f, id) < 0 || hcf_parse_expr(f) < 0) {
                                return -1;
                            }
                            {
                                uint8_t op = (uint8_t)bin;
                                if (f->is_f64[id]) {
                                    if (bin == HC_ADD) {
                                        op = HC_FADD;
                                    } else if (bin == HC_SUB) {
                                        op = HC_FSUB;
                                    } else if (bin == HC_MUL) {
                                        op = HC_FMUL;
                                    } else {
                                        f->err = 1;
                                        return -1;
                                    }
                                }
                                if (hcf_emit(f, op) < 0 || hcf_st_named(f, id) < 0) {
                                    return -1;
                                }
                            }
                        } else {
                            if (hcf_parse_expr(f) < 0 || hcf_st_named(f, id) < 0) {
                                return -1;
                            }
                        }
                        }
                    }
                } else if (*f->p == '*') {
                    f->p++;
                    if (hcf_parse_prim(f) < 0 || hcf_expect(f, '=') < 0 ||
                        hcf_parse_expr(f) < 0 || hcf_emit(f, HC_STORE) < 0) {
                        return -1;
                    }
                } else if (hcf_parse_expr(f) < 0 || hcf_emit(f, HC_DROP) < 0) {
                    return -1;
                }
            }
            if (hcf_emit_u8(f, HC_JMP, (uint8_t)l_top) < 0 || hcf_expect(f, ')') < 0 ||
                hcf_emit_u8(f, HC_LABEL, (uint8_t)l_body) < 0) {
                return -1;
            }
            if (f->nbrk >= HC_MAX_LOOPS || f->ncont >= HC_MAX_LOOPS) {
                f->err = 1;
                return -1;
            }
            f->brk_tgt[f->nbrk++] = l_end;
            f->cont_tgt[f->ncont++] = l_step;
            if (hcf_parse_stmt(f) < 0 || hcf_emit_u8(f, HC_JMP, (uint8_t)l_step) < 0) {
                return -1;
            }
            f->nbrk--;
            f->ncont--;
        }
        return hcf_emit_u8(f, HC_LABEL, (uint8_t)l_end);
    }
    if (hcf_kw(f, "switch")) {
        int l_disp = hcf_new_label(f), l_end = hcf_new_label(f);
        int case_lab[32], case_lo[32], case_hi[32], ncases = 0, l_def = -1; /* was 16 — Lattice key switch */
        if (l_disp < 0 || l_end < 0 || hcf_expect(f, '(') < 0 || hcf_parse_expr(f) < 0 ||
            hcf_expect(f, ')') < 0) {
            return -1;
        }
        if (f->mem_used >= HC_IR_MEM_WORDS) {
            f->err = 1;
            return -1;
        }
        {
            int tmp = f->mem_used++;
            if (hcf_emit_u8(f, HC_ST_SLOT, (uint8_t)tmp) < 0 ||
                hcf_emit_u8(f, HC_JMP, (uint8_t)l_disp) < 0 || hcf_expect(f, '{') < 0) {
                return -1;
            }
            if (f->nbrk >= HC_MAX_LOOPS) {
                f->err = 1;
                return -1;
            }
            f->brk_tgt[f->nbrk++] = l_end;
            for (;;) {
                hcf_skip(f);
                if (*f->p == '}') {
                    f->p++;
                    break;
                }
                if (hcf_kw(f, "case")) {
                    int lo = 0, hi = 0;
                    if (hcf_parse_case_val(f, &lo) < 0 || ncases >= 32) {
                        f->err = 1;
                        return -1;
                    }
                    hi = lo;
                    hcf_skip(f);
                    if (f->p[0] == '.' && f->p[1] == '.' && f->p[2] == '.') {
                        f->p += 3;
                        if (hcf_parse_case_val(f, &hi) < 0) {
                            return -1;
                        }
                    }
                    if (hcf_expect(f, ':') < 0) {
                        return -1;
                    }
                    case_lo[ncases] = lo;
                    case_hi[ncases] = hi;
                    case_lab[ncases] = hcf_new_label(f);
                    if (case_lab[ncases] < 0 ||
                        hcf_emit_u8(f, HC_LABEL, (uint8_t)case_lab[ncases]) < 0) {
                        return -1;
                    }
                    ncases++;
                    continue;
                }
                if (hcf_kw(f, "default")) {
                    if (l_def >= 0 || hcf_expect(f, ':') < 0) {
                        f->err = 1;
                        return -1;
                    }
                    l_def = hcf_new_label(f);
                    if (l_def < 0 || hcf_emit_u8(f, HC_LABEL, (uint8_t)l_def) < 0) {
                        return -1;
                    }
                    continue;
                }
                if (hcf_parse_stmt(f) < 0) {
                    return -1;
                }
            }
            f->nbrk--;
            if (hcf_emit_u8(f, HC_JMP, (uint8_t)l_end) < 0 ||
                hcf_emit_u8(f, HC_LABEL, (uint8_t)l_disp) < 0) {
                return -1;
            }
            for (int i = 0; i < ncases; i++) {
                if (case_lo[i] == case_hi[i]) {
                    if (hcf_emit_u8(f, HC_LD_SLOT, (uint8_t)tmp) < 0 ||
                        hcf_imm(f, (uint64_t)(int64_t)case_lo[i]) < 0 ||
                        hcf_emit(f, HC_EQ) < 0 ||
                        hcf_emit_u8(f, HC_JNZ, (uint8_t)case_lab[i]) < 0) {
                        return -1;
                    }
                } else {
                    /* lo <= tmp && tmp <= hi */
                    if (hcf_emit_u8(f, HC_LD_SLOT, (uint8_t)tmp) < 0 ||
                        hcf_imm(f, (uint64_t)(int64_t)case_lo[i]) < 0 ||
                        hcf_emit(f, HC_GE) < 0 ||
                        hcf_emit_u8(f, HC_LD_SLOT, (uint8_t)tmp) < 0 ||
                        hcf_imm(f, (uint64_t)(int64_t)case_hi[i]) < 0 ||
                        hcf_emit(f, HC_LE) < 0 || hcf_emit(f, HC_AND) < 0 ||
                        hcf_emit_u8(f, HC_JNZ, (uint8_t)case_lab[i]) < 0) {
                        return -1;
                    }
                }
            }
            if (l_def >= 0) {
                if (hcf_emit_u8(f, HC_JMP, (uint8_t)l_def) < 0) {
                    return -1;
                }
            }
            return hcf_emit_u8(f, HC_LABEL, (uint8_t)l_end);
        }
    }
    if (hcf_kw(f, "goto")) {
        hcf_skip(f);
        if (!hcf_ident_start(*f->p)) {
            f->err = 1;
            return -1;
        }
        const char *s = f->p;
        while (hcf_ident_char(*f->p)) {
            f->p++;
        }
        int lab = hcf_get_glab(f, s, (int)(f->p - s));
        if (lab < 0 || hcf_emit_u8(f, HC_JMP, (uint8_t)lab) < 0) {
            return -1;
        }
        return hcf_expect(f, ';');
    }
    if (hcf_kw(f, "break")) {
        if (f->nbrk < 1 || hcf_emit_u8(f, HC_JMP, (uint8_t)f->brk_tgt[f->nbrk - 1]) < 0) {
            f->err = 1;
            return -1;
        }
        return hcf_expect(f, ';');
    }
    if (hcf_kw(f, "continue")) {
        if (f->ncont < 1 || hcf_emit_u8(f, HC_JMP, (uint8_t)f->cont_tgt[f->ncont - 1]) < 0) {
            f->err = 1;
            return -1;
        }
        return hcf_expect(f, ';');
    }
    if (hcf_kw(f, "enum")) {
        hcf_skip(f);
        /* Optional tag name (ignored; constants are global to the compile unit). */
        if (hcf_ident_start(*f->p)) {
            while (hcf_ident_char(*f->p)) {
                f->p++;
            }
            hcf_skip(f);
        }
        if (hcf_expect(f, '{') < 0) {
            return -1;
        }
        int64_t next = 0;
        for (;;) {
            hcf_skip(f);
            if (*f->p == '}') {
                f->p++;
                break;
            }
            if (!hcf_ident_start(*f->p)) {
                f->err = 1;
                return -1;
            }
            const char *ns = f->p;
            while (hcf_ident_char(*f->p)) {
                f->p++;
            }
            int nlen = (int)(f->p - ns);
            hcf_skip(f);
            int64_t val = next;
            if (*f->p == '=') {
                f->p++;
                uint64_t lit = 0;
                if (hcf_parse_int_lit(f, &lit) < 0) {
                    return -1;
                }
                val = (int64_t)lit;
                hcf_skip(f);
            }
            if (hcf_add_enum(f, ns, nlen, val) < 0) {
                return -1;
            }
            next = val + 1;
            hcf_skip(f);
            if (*f->p == ',') {
                f->p++;
                continue;
            }
            if (*f->p == '}') {
                f->p++;
                break;
            }
            f->err = 1;
            return -1;
        }
        hcf_skip(f);
        if (*f->p == ';') {
            f->p++;
        }
        return 0;
    }
    {
        int is_union = 0;
        if (hcf_kw(f, "union")) {
            is_union = 1;
        } else if (!hcf_kw(f, "class")) {
            is_union = -1;
        }
        if (is_union >= 0) {
        if (f->nclasses >= HC_MAX_CLASSES) {
            f->err = 1;
            return -1;
        }
        hcf_skip(f);
        if (!hcf_ident_start(*f->p)) {
            f->err = 1;
            return -1;
        }
        const char *ns = f->p;
        while (hcf_ident_char(*f->p)) {
            f->p++;
        }
        int nlen = (int)(f->p - ns);
        if (nlen >= 15 || hcf_find_class(f, ns, nlen) >= 0) {
            f->err = 1;
            return -1;
        }
        struct hc_class *c = &f->classes[f->nclasses];
        for (int i = 0; i < nlen; i++) {
            c->name[i] = ns[i];
        }
        c->name[nlen] = 0;
        c->nmembers = 0;
        c->size = 0;
        if (hcf_expect(f, '{') < 0) {
            return -1;
        }
        for (;;) {
            hcf_skip(f);
            if (*f->p == '}') {
                f->p++;
                break;
            }
            int mstride = 0;
            int mtype = HC_TYPE_NONE;
            int m_f64 = 0;
            if (hcf_kw(f, "U8") || hcf_kw(f, "u8")) {
                mstride = 1;
            } else if (hcf_kw(f, "U32") || hcf_kw(f, "u32")) {
                mstride = 4;
            } else if (hcf_kw(f, "F64") || hcf_kw(f, "f64")) {
                mstride = 8;
                m_f64 = 1;
            } else if (hcf_kw(f, "I64") || hcf_kw(f, "i64") || hcf_kw(f, "Bool") ||
                       hcf_kw(f, "bool")) {
                mstride = 8;
            } else if (hcf_kw(f, "CColorROPU16")) {
                /* Lattice color members — word-sized for BLACK/YELLOW assigns. */
                mstride = 8;
            } else if (hcf_ident_start(*f->p)) {
                const char *ts = f->p;
                while (hcf_ident_char(*f->p)) {
                    f->p++;
                }
                int tlen = (int)(f->p - ts);
                int tci = hcf_find_class(f, ts, tlen);
                if (tci < 0) {
                    f->err = 1;
                    return -1;
                }
                mtype = tci;
                mstride = f->classes[tci].size;
            } else {
                f->err = 1;
                return -1;
            }
            hcf_skip(f);
            /* Lattice: F64 x, y, z; — same type, comma-separated names. */
            for (;;) {
                if (!hcf_ident_start(*f->p) || c->nmembers >= HC_MAX_MEMBERS) {
                    f->err = 1;
                    return -1;
                }
                const char *ms = f->p;
                while (hcf_ident_char(*f->p)) {
                    f->p++;
                }
                int mlen = (int)(f->p - ms);
                if (mlen >= 15 || hcf_find_member(c, ms, mlen) >= 0) {
                    f->err = 1;
                    return -1;
                }
                int align = (mtype != HC_TYPE_NONE) ? 8 : mstride;
                int off = is_union ? 0 : hcf_align_up(c->size, align);
                struct hc_member *m = &c->mem[c->nmembers++];
                for (int i = 0; i < mlen; i++) {
                    m->name[i] = ms[i];
                }
                m->name[mlen] = 0;
                m->off = (uint16_t)off;
                m->stride = (uint8_t)((mtype != HC_TYPE_NONE) ? 8 : mstride);
                m->type = (uint8_t)mtype;
                m->is_f64 = (uint8_t)((mtype != HC_TYPE_NONE) ? 0 : m_f64);
                if (is_union) {
                    if (mstride > c->size) {
                        c->size = mstride;
                    }
                } else {
                    c->size = off + mstride;
                }
                hcf_skip(f);
                if (*f->p == ',') {
                    f->p++;
                    hcf_skip(f);
                    continue;
                }
                if (hcf_expect(f, ';') < 0) {
                    return -1;
                }
                break;
            }
        }
        c->size = hcf_align_up(c->size, 8);
        if (c->size < 8) {
            c->size = 8;
        }
        f->nclasses++;
        hcf_skip(f);
        /* Lattice: `} tt;` / `} a, b;` — file-scope instances of this class. */
        if (hcf_ident_start(*f->p)) {
            int ci = f->nclasses - 1;
            int words = (f->classes[ci].size + 7) / 8;
            if (words < 1) {
                words = 1;
            }
            for (;;) {
                const char *gs = f->p;
                while (hcf_ident_char(*f->p)) {
                    f->p++;
                }
                int glen = (int)(f->p - gs);
                int gid = hcf_decl_global_n(f, gs, glen, words, 8);
                if (gid < 0) {
                    return -1;
                }
                f->type[gid] = (uint8_t)ci;
                f->is_ptr[gid] = 0;
                hcf_skip(f);
                if (*f->p == ',') {
                    f->p++;
                    hcf_skip(f);
                    if (!hcf_ident_start(*f->p)) {
                        f->err = 1;
                        return -1;
                    }
                    continue;
                }
                if (hcf_expect(f, ';') < 0) {
                    return -1;
                }
                break;
            }
            return 0;
        }
        if (*f->p == ';') {
            f->p++;
        }
        return 0;
        }
    }
    if (*f->p == '*') {
        f->p++;
        hcf_skip(f);
        uint8_t st_op = HC_STORE;
        if (hcf_ident_start(*f->p)) {
            const char *s = f->p;
            while (hcf_ident_char(*f->p)) {
                f->p++;
            }
            int len = (int)(f->p - s);
            int id = hcf_lookup(f, s, len);
            if (id >= 0 && f->is_ptr[id]) {
                if (hcf_ld_named(f, id) < 0 || hcf_expect(f, '=') < 0 ||
                    hcf_parse_expr(f) < 0 ||
                    hcf_emit(f, f->stride[id] == 1 ? HC_STORE8 : (f->stride[id] == 4 ? HC_STORE32 : HC_STORE)) < 0) {
                    return -1;
                }
                return hcf_expect(f, ';');
            }
            f->p = s;
        }
        if (hcf_parse_prim(f) < 0 || hcf_expect(f, '=') < 0 || hcf_parse_expr(f) < 0 ||
            hcf_emit(f, st_op) < 0) {
            return -1;
        }
        return hcf_expect(f, ';');
    }
    {
        int stride = 8;
        int is_decl = 0;
        if (hcf_kw(f, "U8") || hcf_kw(f, "u8")) {
            stride = 1;
            is_decl = 1;
        } else if (hcf_kw(f, "U32") || hcf_kw(f, "u32")) {
            stride = 4;
            is_decl = 1;
        } else if (hcf_kw(f, "F64") || hcf_kw(f, "f64")) {
            stride = 8;
            is_decl = 2; /* F64 */
        } else if (hcf_kw(f, "I64") || hcf_kw(f, "i64") || hcf_kw(f, "Bool") ||
                   hcf_kw(f, "bool") || hcf_kw(f, "U0") || hcf_kw(f, "u0")) {
            is_decl = 1;
        }
        if (is_decl) {
            for (;;) {
                hcf_skip(f);
                int as_iptr = 0;
                if (*f->p == '*') {
                    f->p++;
                    as_iptr = 1;
                    hcf_skip(f);
                }
                if (!hcf_ident_start(*f->p)) {
                    f->err = 1;
                    return -1;
                }
                const char *s = f->p;
                while (hcf_ident_char(*f->p)) {
                    f->p++;
                }
                int nlen = (int)(f->p - s);
                hcf_skip(f);
                int nelem = 1;
                if (!as_iptr && *f->p == '[') {
                    f->p++;
                    hcf_skip(f);
                    int nsz = 0;
                    if (hcf_parse_case_val(f, &nsz) < 0 || nsz < 1 || nsz > 255 ||
                        hcf_expect(f, ']') < 0) {
                        f->err = 1;
                        return -1;
                    }
                    nelem = nsz;
                }
                int pointee = stride;
                if (as_iptr) {
                    /* I64 *p / U8 *p — pointer word; stride[id] keeps pointee width. */
                    nelem = 1;
                }
                int id = hcf_decl_n(f, s, nlen, nelem, as_iptr ? 8 : stride);
                if (id < 0) {
                    return -1;
                }
                if (is_decl == 2) {
                    f->is_f64[id] = 1;
                }
                if (as_iptr) {
                    f->is_ptr[id] = 1;
                    f->stride[id] = (uint8_t)pointee;
                }
                hcf_skip(f);
                if (*f->p == '=') {
                    f->p++;
                    hcf_skip(f);
                    if (*f->p == '{') {
                        if (hcf_parse_brace_init(f, id) < 0) {
                            return -1;
                        }
                    } else if (nelem == 1) {
                        if (hcf_parse_expr(f) < 0 || hcf_st_named(f, id) < 0) {
                            return -1;
                        }
                    } else {
                        f->err = 1;
                        return -1;
                    }
                } else if (nelem == 1) {
                    if (hcf_imm(f, 0) < 0 || hcf_st_named(f, id) < 0) {
                        return -1;
                    }
                }
                hcf_skip(f);
                if (*f->p == ',') {
                    f->p++;
                    continue;
                }
                break;
            }
            return hcf_expect(f, ';');
        }
    }
    if (hcf_ident_start(*f->p)) {
        const char *s = f->p;
        while (hcf_ident_char(*f->p)) {
            f->p++;
        }
        int len = (int)(f->p - s);
        hcf_skip(f);
        if (*f->p == ':') {
            f->p++;
            int lab = hcf_get_glab(f, s, len);
            if (lab < 0 || hcf_emit_u8(f, HC_LABEL, (uint8_t)lab) < 0) {
                return -1;
            }
            return 0;
        }
        int ci = hcf_find_class(f, s, len);
        if (ci >= 0) {
            int as_ptr = 0;
            if (*f->p == '*') {
                f->p++;
                as_ptr = 1;
                hcf_skip(f);
            }
            if (!hcf_ident_start(*f->p)) {
                f->err = 1;
                return -1;
            }
            const char *vs = f->p;
            while (hcf_ident_char(*f->p)) {
                f->p++;
            }
            int id = hcf_decl_class(f, vs, (int)(f->p - vs), ci, as_ptr);
            if (id < 0) {
                return -1;
            }
            hcf_skip(f);
            if (as_ptr && *f->p == '=') {
                f->p++;
                if (hcf_parse_expr(f) < 0 || hcf_st_named(f, id) < 0) {
                    return -1;
                }
            } else if (as_ptr) {
                if (hcf_imm(f, 0) < 0 || hcf_st_named(f, id) < 0) {
                    return -1;
                }
            }
            return hcf_expect(f, ';');
        }
        hcf_skip(f);
        if (*f->p == '(') {
            f->p = s;
            if (hcf_parse_expr(f) < 0 || hcf_emit(f, HC_DROP) < 0) {
                return -1;
            }
            return hcf_expect(f, ';');
        }
        int id = hcf_lookup(f, s, len);
        if (id < 0) {
            /* Fs->win_inhibit = …;  Fs->draw_it = …; */
            if (len == 2 && s[0] == 'F' && s[1] == 's' && f->p[0] == '-' && f->p[1] == '>') {
                f->p += 2;
                hcf_skip(f);
                if (!hcf_ident_start(*f->p)) {
                    f->err = 1;
                    return -1;
                }
                const char *ms = f->p;
                while (hcf_ident_char(*f->p)) {
                    f->p++;
                }
                int mlen = (int)(f->p - ms);
                uint64_t addr = 0;
                if (mlen == 11 && ms[0] == 'w' && ms[1] == 'i' && ms[2] == 'n' &&
                    ms[3] == '_' && ms[4] == 'i' && ms[5] == 'n' && ms[6] == 'h' &&
                    ms[7] == 'i' && ms[8] == 'b' && ms[9] == 'i' && ms[10] == 't') {
                    addr = (uint64_t)(uintptr_t)&g_hc_fs_win_inhibit;
                } else if (mlen == 7 && ms[0] == 'd' && ms[1] == 'r' && ms[2] == 'a' &&
                           ms[3] == 'w' && ms[4] == '_' && ms[5] == 'i' && ms[6] == 't') {
                    addr = (uint64_t)(uintptr_t)&g_hc_fs_draw_it;
                } else if (mlen == 8 && ms[0] == 'c' && ms[1] == 'u' && ms[2] == 'r' &&
                           ms[3] == '_' && ms[4] == 'm' && ms[5] == 'e' && ms[6] == 'n' &&
                           ms[7] == 'u') {
                    addr = (uint64_t)(uintptr_t)&g_hc_fs_cur_menu;
                } else {
                    f->err = 1;
                    return -1;
                }
                hcf_skip(f);
                if (*f->p != '=' || f->p[1] == '=') {
                    f->err = 1;
                    return -1;
                }
                f->p++;
                if (hcf_imm(f, addr) < 0 || hcf_parse_expr(f) < 0 || hcf_emit(f, HC_STORE) < 0) {
                    return -1;
                }
                return hcf_expect(f, ';');
            }
            /* Bare call/ident stmt: MenuPop; DCAlias; user fns without (). */
            f->p = s;
            if (hcf_parse_expr(f) < 0 || hcf_emit(f, HC_DROP) < 0) {
                return -1;
            }
            return hcf_expect(f, ';');
        }
        if (f->p[0] == '+' && f->p[1] == '+') {
            f->p += 2;
            if (hcf_ld_named(f, id) < 0 || hcf_imm(f, 1) < 0 ||
                hcf_emit(f, HC_ADD) < 0 || hcf_st_named(f, id) < 0) {
                return -1;
            }
            return hcf_expect(f, ';');
        }
        if (f->p[0] == '-' && f->p[1] == '-') {
            f->p += 2;
            if (hcf_ld_named(f, id) < 0 || hcf_imm(f, 1) < 0 ||
                hcf_emit(f, HC_SUB) < 0 || hcf_st_named(f, id) < 0) {
                return -1;
            }
            return hcf_expect(f, ';');
        }
        if (*f->p == '[') {
            f->p++;
            int bin;
            if (hcf_parse_index_addr(f, id) < 0 || (bin = hcf_parse_assign_op(f)) < 0) {
                return -1;
            }
            int ld = f->stride[id] == 1 ? HC_LOAD8 : (f->stride[id] == 4 ? HC_LOAD32 : HC_LOAD);
            int st = f->stride[id] == 1 ? HC_STORE8 : (f->stride[id] == 4 ? HC_STORE32 : HC_STORE);
            return hcf_finish_mem_assign(f, bin, ld, st, f->is_f64[id]);
        }
        if (*f->p == '.') {
            f->p++;
            int st = 8, ty = HC_TYPE_NONE, isf = 0;
            if (hcf_parse_dot_addr(f, id, &st, &ty, &isf) < 0 || ty != HC_TYPE_NONE) {
                f->err = 1;
                return -1;
            }
            hcf_skip(f);
            int ld = (st == 1 ? HC_LOAD8 : (st == 4 ? HC_LOAD32 : HC_LOAD));
            int sop = (st == 1 ? HC_STORE8 : (st == 4 ? HC_STORE32 : HC_STORE));
            if (f->p[0] == '+' && f->p[1] == '+') {
                f->p += 2;
                return hcf_finish_mem_incdec(f, ld, sop, isf, 1);
            }
            if (f->p[0] == '-' && f->p[1] == '-') {
                f->p += 2;
                return hcf_finish_mem_incdec(f, ld, sop, isf, 0);
            }
            {
                int bin = hcf_parse_assign_op(f);
                if (bin < 0) {
                    f->err = 1;
                    return -1;
                }
                return hcf_finish_mem_assign(f, bin, ld, sop, isf);
            }
        }
        if (f->p[0] == '-' && f->p[1] == '>') {
            f->p += 2;
            int st = 8, ty = HC_TYPE_NONE, isf = 0;
            if (hcf_parse_arrow_addr(f, id, &st, &ty, &isf) < 0 || ty != HC_TYPE_NONE) {
                f->err = 1;
                return -1;
            }
            hcf_skip(f);
            int ld = (st == 1 ? HC_LOAD8 : (st == 4 ? HC_LOAD32 : HC_LOAD));
            int sop = (st == 1 ? HC_STORE8 : (st == 4 ? HC_STORE32 : HC_STORE));
            if (f->p[0] == '+' && f->p[1] == '+') {
                f->p += 2;
                return hcf_finish_mem_incdec(f, ld, sop, isf, 1);
            }
            if (f->p[0] == '-' && f->p[1] == '-') {
                f->p += 2;
                return hcf_finish_mem_incdec(f, ld, sop, isf, 0);
            }
            {
                int bin = hcf_parse_assign_op(f);
                if (bin < 0) {
                    f->err = 1;
                    return -1;
                }
                return hcf_finish_mem_assign(f, bin, ld, sop, isf);
            }
        }
        {
            int bin = hcf_parse_assign_op(f);
            if (bin < 0) {
                return -1;
            }
            if (bin) {
                if (hcf_ld_named(f, id) < 0 || hcf_parse_expr(f) < 0) {
                    return -1;
                }
                {
                    uint8_t op = (uint8_t)bin;
                    if (f->is_f64[id]) {
                        if (!f->expr_f64) {
                            if (hcf_emit(f, HC_I2F) < 0) {
                                return -1;
                            }
                            f->expr_f64 = 1;
                        }
                        if (bin == HC_ADD) {
                            op = HC_FADD;
                        } else if (bin == HC_SUB) {
                            op = HC_FSUB;
                        } else if (bin == HC_MUL) {
                            op = HC_FMUL;
                        } else {
                            f->err = 1;
                            return -1;
                        }
                    }
                    if (hcf_emit(f, op) < 0 || hcf_st_named(f, id) < 0) {
                        return -1;
                    }
                }
            } else if (hcf_parse_expr(f) < 0) {
                return -1;
            } else {
                if (f->is_f64[id] && !f->expr_f64) {
                    if (hcf_emit(f, HC_I2F) < 0) {
                        return -1;
                    }
                    f->expr_f64 = 1;
                } else if (!f->is_f64[id] && f->expr_f64) {
                    f->err = 1;
                    return -1;
                }
                if (hcf_st_named(f, id) < 0) {
                    return -1;
                }
            }
            return hcf_expect(f, ';');
        }
    }
    f->err = 1;
    return -1;
}

typedef int (*hc_jit_into_fn)(const uint8_t *bc, size_t n, uint32_t *out, size_t cap);

static inline int hc_front_compile_ex(const char *src, uint8_t *bc, size_t cap,
                                      struct hc_fn_table *tbl, uint32_t fn_code[][16384],
                                      hc_jit_into_fn jit_into) {
    struct hc_front f;
    f.p = src;
    f.bc = bc;
    f.n = 0;
    f.cap = cap;
    f.err = 0;
    f.next_label = 0;
    f.nlocals = 0;
    f.mem_used = 0;
    f.glob_used = 0;
    f.str_top = HC_IR_MEM_WORDS;
    f.fns = tbl;
    f.nclasses = 0;
    f.nenums = 0;
    f.nglab = 0;
    f.nbrk = 0;
    f.ncont = 0;
    g_hc_glob_words = 0;
    for (int i = 0; i < HC_IR_MAX_NAMES; i++) {
        f.names[i][0] = 0;
        f.base[i] = 0;
        f.count[i] = 0;
        f.stride[i] = 8;
        f.type[i] = HC_TYPE_NONE;
        f.is_ptr[i] = 0;
        f.is_global[i] = 0;
        f.is_f64[i] = 0;
    }
    f.expr_f64 = 0;
    /* Builtin CDC { I64 color; I64 depth_buf; } + CMenuEntry { I64 checked; }. */
    {
        struct hc_class *c = &f.classes[0];
        c->name[0] = 'C';
        c->name[1] = 'D';
        c->name[2] = 'C';
        c->name[3] = 0;
        c->nmembers = 2;
        c->size = 16;
        c->mem[0].name[0] = 'c';
        c->mem[0].name[1] = 'o';
        c->mem[0].name[2] = 'l';
        c->mem[0].name[3] = 'o';
        c->mem[0].name[4] = 'r';
        c->mem[0].name[5] = 0;
        c->mem[0].off = 0;
        c->mem[0].stride = 8;
        c->mem[0].type = HC_TYPE_NONE;
        c->mem[0].is_f64 = 0;
        c->mem[1].name[0] = 'd';
        c->mem[1].name[1] = 'e';
        c->mem[1].name[2] = 'p';
        c->mem[1].name[3] = 't';
        c->mem[1].name[4] = 'h';
        c->mem[1].name[5] = '_';
        c->mem[1].name[6] = 'b';
        c->mem[1].name[7] = 'u';
        c->mem[1].name[8] = 'f';
        c->mem[1].name[9] = 0;
        c->mem[1].off = 8;
        c->mem[1].stride = 8;
        c->mem[1].type = HC_TYPE_NONE;
        c->mem[1].is_f64 = 0;

        c = &f.classes[1];
        c->name[0] = 'C';
        c->name[1] = 'M';
        c->name[2] = 'e';
        c->name[3] = 'n';
        c->name[4] = 'u';
        c->name[5] = 'E';
        c->name[6] = 'n';
        c->name[7] = 't';
        c->name[8] = 'r';
        c->name[9] = 'y';
        c->name[10] = 0;
        c->nmembers = 1;
        c->size = 8;
        c->mem[0].name[0] = 'c';
        c->mem[0].name[1] = 'h';
        c->mem[0].name[2] = 'e';
        c->mem[0].name[3] = 'c';
        c->mem[0].name[4] = 'k';
        c->mem[0].name[5] = 'e';
        c->mem[0].name[6] = 'd';
        c->mem[0].name[7] = 0;
        c->mem[0].off = 0;
        c->mem[0].stride = 8;
        c->mem[0].type = HC_TYPE_NONE;
        c->mem[0].is_f64 = 0;
        f.nclasses = 2;
    }
    if (tbl) {
        tbl->n = 0;
    }
    if (hcf_emit_u8(&f, HC_ENTER, 0) < 0) {
        return HC_FRONT_ERR;
    }

    while (*f.p) {
        hcf_skip(&f);
        if (!*f.p) {
            break;
        }
        const char *save = f.p;
        int typed = 0;
        int gstride = 8;
        int is_u0 = 0;
        int is_f64t = 0;
        if (hcf_kw(&f, "U0") || hcf_kw(&f, "u0")) {
            typed = 1;
            is_u0 = 1;
            gstride = 8;
        } else if (hcf_kw(&f, "I64") || hcf_kw(&f, "i64") || hcf_kw(&f, "Bool") ||
                   hcf_kw(&f, "bool")) {
            typed = 1;
            gstride = 8;
        } else if (hcf_kw(&f, "U32") || hcf_kw(&f, "u32")) {
            typed = 1;
            gstride = 4;
        } else if (hcf_kw(&f, "U8") || hcf_kw(&f, "u8")) {
            typed = 1;
            gstride = 1;
        } else if (hcf_kw(&f, "F64") || hcf_kw(&f, "f64")) {
            typed = 1;
            gstride = 8;
            is_f64t = 1;
        }
        if (typed) {
            hcf_skip(&f);
            if (hcf_ident_start(*f.p)) {
                const char *ns = f.p;
                while (hcf_ident_char(*f.p)) {
                    f.p++;
                }
                int nlen = (int)(f.p - ns);
                hcf_skip(&f);
                if (*f.p == '(') {
                    if (!tbl || !jit_into || tbl->n >= HC_MAX_FNS || nlen >= 23) {
                        return HC_FRONT_ERR;
                    }
                    struct hc_fn *fn = &tbl->fns[tbl->n];
                    for (int i = 0; i < nlen; i++) {
                        fn->name[i] = ns[i];
                    }
                    fn->name[nlen] = 0;
                    fn->nargs = 0;
                    fn->code = NULL;
                    f.p++;
                    struct hc_front fb;
                    fb.p = f.p;
                    fb.bc = fn->bc;
                    fb.n = 0;
                    fb.cap = sizeof(fn->bc);
                    fb.err = 0;
                    fb.next_label = 0;
                    fb.nlocals = 0;
                    fb.mem_used = 0;
                    fb.glob_used = f.glob_used;
                    fb.str_top = f.str_top;
                    fb.fns = tbl;
                    /* Functions see classes/enums/globals declared above them. */
                    fb.nclasses = f.nclasses;
                    for (int ci = 0; ci < f.nclasses; ci++) {
                        struct hc_class *dst = &fb.classes[ci];
                        struct hc_class *src = &f.classes[ci];
                        for (int k = 0; k < 16; k++) {
                            dst->name[k] = src->name[k];
                        }
                        dst->nmembers = src->nmembers;
                        dst->size = src->size;
                        for (int mi = 0; mi < HC_MAX_MEMBERS; mi++) {
                            for (int k = 0; k < 16; k++) {
                                dst->mem[mi].name[k] = src->mem[mi].name[k];
                            }
                            dst->mem[mi].off = src->mem[mi].off;
                            dst->mem[mi].stride = src->mem[mi].stride;
                            dst->mem[mi].type = src->mem[mi].type;
                            dst->mem[mi].is_f64 = src->mem[mi].is_f64;
                        }
                    }
                    fb.nenums = f.nenums;
                    for (int ei = 0; ei < f.nenums; ei++) {
                        for (int k = 0; k < 16; k++) {
                            fb.enums[ei].name[k] = f.enums[ei].name[k];
                        }
                        fb.enums[ei].val = f.enums[ei].val;
                    }
                    fb.nglab = 0;
                    fb.nbrk = 0;
                    fb.ncont = 0;
                    for (int i = 0; i < HC_IR_MAX_NAMES; i++) {
                        fb.names[i][0] = 0;
                        fb.base[i] = 0;
                        fb.count[i] = 0;
                        fb.stride[i] = 8;
                        fb.type[i] = HC_TYPE_NONE;
                        fb.is_ptr[i] = 0;
                        fb.is_global[i] = 0;
                        fb.is_f64[i] = 0;
                    }
                    fb.expr_f64 = 0;
                    /* Import file-scope globals into the function name table. */
                    for (int gi = 0; gi < f.nlocals; gi++) {
                        if (!f.is_global[gi]) {
                            continue;
                        }
                        if (fb.nlocals >= HC_IR_MAX_NAMES) {
                            return HC_FRONT_ERR;
                        }
                        int id = fb.nlocals++;
                        for (int k = 0; k < 16; k++) {
                            fb.names[id][k] = f.names[gi][k];
                        }
                        fb.base[id] = f.base[gi];
                        fb.count[id] = f.count[gi];
                        fb.stride[id] = f.stride[gi];
                        fb.type[id] = f.type[gi];
                        fb.is_ptr[id] = f.is_ptr[gi];
                        fb.is_global[id] = 1;
                        fb.is_f64[id] = f.is_f64[gi];
                    }
                    /* Frame placeholder; patched after params+body know mem_used. */
                    if (hcf_emit_u8(&fb, HC_ENTER, 0) < 0) {
                        return HC_FRONT_ERR;
                    }
                    hcf_skip(&fb);
                    if (*fb.p != ')') {
                        for (;;) {
                            int pstride = 8;
                            int is_f64p = 0;
                            int is_ptrp = 0;
                            int pci = -1;
                            int pointee = 8;
                            if (hcf_kw(&fb, "F64") || hcf_kw(&fb, "f64")) {
                                is_f64p = 1;
                                pstride = 8;
                            } else if (hcf_kw(&fb, "U32") || hcf_kw(&fb, "u32")) {
                                pstride = 4;
                                pointee = 4;
                            } else if (hcf_kw(&fb, "U8") || hcf_kw(&fb, "u8")) {
                                pstride = 1;
                                pointee = 1;
                            } else if (hcf_kw(&fb, "I64") || hcf_kw(&fb, "i64") ||
                                       hcf_kw(&fb, "Bool") || hcf_kw(&fb, "bool") ||
                                       hcf_kw(&fb, "CColorROPU16")) {
                                pstride = 8;
                                pointee = 8;
                            } else if (hcf_ident_start(*fb.p)) {
                                const char *cts = fb.p;
                                while (hcf_ident_char(*fb.p)) {
                                    fb.p++;
                                }
                                pci = hcf_find_class(&fb, cts, (int)(fb.p - cts));
                                pstride = 8;
                                is_ptrp = 1; /* Class / opaque forward → pointer slot */
                                if (pci < 0) {
                                    /* Opaque `CTask *` without class body — still a word ptr. */
                                    hcf_skip(&fb);
                                    if (*fb.p != '*') {
                                        return HC_FRONT_ERR;
                                    }
                                }
                            } else {
                                return HC_FRONT_ERR;
                            }
                            hcf_skip(&fb);
                            if (*fb.p == '*') {
                                fb.p++;
                                is_ptrp = 1;
                                hcf_skip(&fb);
                            } else if (pci >= 0) {
                                /* `Turtle t` by value not supported in params yet. */
                                return HC_FRONT_ERR;
                            }
                            int id;
                            if (!hcf_ident_start(*fb.p)) {
                                /* Anonymous param: `CTask *` / `I64` — still takes a slot. */
                                char an[4];
                                an[0] = '_';
                                an[1] = 'a';
                                an[2] = (char)('0' + fn->nargs);
                                an[3] = 0;
                                if (pci >= 0) {
                                    id = hcf_decl_class(&fb, an, 3, pci, 1);
                                } else {
                                    id = hcf_decl_n(&fb, an, 3, 1, is_ptrp ? 8 : pstride);
                                }
                            } else {
                                const char *ps = fb.p;
                                while (hcf_ident_char(*fb.p)) {
                                    fb.p++;
                                }
                                int plen = (int)(fb.p - ps);
                                if (pci >= 0) {
                                    id = hcf_decl_class(&fb, ps, plen, pci, 1);
                                } else {
                                    id = hcf_decl_n(&fb, ps, plen, 1, is_ptrp ? 8 : pstride);
                                }
                            }
                            if (id < 0 || fn->nargs >= 7) {
                                return HC_FRONT_ERR;
                            }
                            /* F64 params: bits in xN like builtins. Mark is_f64 so loads
                             * participate in F64 arith. */
                            if (is_f64p) {
                                fb.is_f64[id] = 1;
                            }
                            if (is_ptrp && pci < 0) {
                                fb.is_ptr[id] = 1;
                                fb.stride[id] = (uint8_t)pointee;
                            }
                            if (hcf_emit_u8u8(&fb, HC_INARG, fb.base[id], (uint8_t)fn->nargs) < 0) {
                                return HC_FRONT_ERR;
                            }
                            fn->nargs++;
                            hcf_skip(&fb);
                            if (*fb.p == ',') {
                                fb.p++;
                                continue;
                            }
                            break;
                        }
                    }
                    if (hcf_expect(&fb, ')') < 0 || hcf_parse_block(&fb) < 0 || fb.err) {
                        return HC_FRONT_ERR;
                    }
                    if (fb.mem_used > 255) {
                        return HC_FRONT_ERR;
                    }
                    fb.bc[1] = (uint8_t)fb.mem_used; /* patch HC_ENTER nwords */
                    fn->bc[fb.n++] = HC_END;
                    fn->bc_len = (int)fb.n;
                    f.p = fb.p;
                    int bytes = jit_into(fn->bc, (size_t)fn->bc_len, fn_code[tbl->n], 16384);
                    if (bytes < 0) {
                        return HC_FRONT_ERR;
                    }
                    fn->code = fn_code[tbl->n];
                    tbl->n++;
                    f.str_top = fb.str_top;
                    continue;
                }
                /* File-scope global: Type name; / Type name = expr; / Type name[N];
                 * Also: Type a=1,b=2,c=3; */
                if (is_u0) {
                    /* U0 name; is not a global — fall through. */
                    f.p = save;
                } else {
                    for (;;) {
                        int nelem = 1;
                        const char *ns2 = ns;
                        int nlen2 = nlen;
                        if (*f.p == '[') {
                            f.p++;
                            uint64_t nlit = 0;
                            if (hcf_parse_int_lit(&f, &nlit) < 0 || nlit < 1 || nlit > 255 ||
                                hcf_expect(&f, ']') < 0) {
                                return HC_FRONT_ERR;
                            }
                            nelem = (int)nlit;
                        }
                        int gid = hcf_decl_global_n(&f, ns2, nlen2, nelem, gstride);
                        if (gid < 0) {
                            return HC_FRONT_ERR;
                        }
                        if (is_f64t) {
                            f.is_f64[gid] = 1;
                        }
                        hcf_skip(&f);
                        if (*f.p == '=') {
                            f.p++;
                            hcf_skip(&f);
                            if (*f.p == '{') {
                                if (hcf_parse_brace_init(&f, gid) < 0) {
                                    return HC_FRONT_ERR;
                                }
                            } else if (nelem == 1) {
                                if (hcf_parse_expr(&f) < 0 || hcf_st_named(&f, gid) < 0) {
                                    return HC_FRONT_ERR;
                                }
                            } else {
                                return HC_FRONT_ERR;
                            }
                            hcf_skip(&f);
                        }
                        if (*f.p == ',') {
                            f.p++;
                            hcf_skip(&f);
                            if (!hcf_ident_start(*f.p)) {
                                return HC_FRONT_ERR;
                            }
                            ns = f.p;
                            while (hcf_ident_char(*f.p)) {
                                f.p++;
                            }
                            nlen = (int)(f.p - ns);
                            hcf_skip(&f);
                            continue;
                        }
                        if (hcf_expect(&f, ';') < 0) {
                            return HC_FRONT_ERR;
                        }
                        break;
                    }
                    continue;
                }
            }
            f.p = save;
        } else {
            f.p = save;
        }
        if (hcf_parse_stmt(&f) < 0 || f.err) {
            return HC_FRONT_ERR;
        }
    }
    if (f.mem_used > 255) {
        return HC_FRONT_ERR;
    }
    bc[1] = (uint8_t)f.mem_used; /* patch HC_ENTER nwords */
    g_hc_glob_words = (uint64_t)f.glob_used;
    if (f.n + 1 > cap) {
        return HC_FRONT_ERR;
    }
    bc[f.n++] = HC_END;
    return (int)f.n;
}

static inline int hc_front_compile(const char *src, uint8_t *bc, size_t cap) {
    return hc_front_compile_ex(src, bc, cap, NULL, NULL, NULL);
}
