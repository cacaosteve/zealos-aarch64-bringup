/*
 * Exception + GICv3 + CNTP timer bring-up for QEMU virt @ EL1.
 */
#include <stdint.h>
#include <stddef.h>
#include "handoff.h"
#include "a64_emit.h"
#include "hc_ir.h"
#include "hc_front.h"
#include "fb_font.h"
#include "mmio_map.h"
#include "plat_pi4.h"
#include "virtio_kbd.h"
#include "virtio_tablet.h"
#include "virtio_blk.h"
#include "disk_layout.h"
#include "netofdots_zc.h"
#include "lines_zc.h"
#include "minigr_zc.h"
#include "memsort_zc.h"
#include "peekplot_zc.h"
#include "offbmp_zc.h"
#include "heapstr_zc.h"
#include "catfmt_zc.h"
#include "heapque_zc.h"
#include "jobque_zc.h"
#include "jobrun_zc.h"
#include "taskspawn_zc.h"
#include "popup_zc.h"
#include "doclite_zc.h"
#include "doclib_zc.h"
#include "notes_zc.h"
#include "globshare_zc.h"
#include "life_zc.h"
#include "cartlite_zc.h"
#include "vec2lite_zc.h"
#include "angleslite_zc.h"
#include "coslite_zc.h"
#include "sqrtlite_zc.h"
#include "arglite_zc.h"
#include "commalite_zc.h"
#include "plot3lite_zc.h"
#include "tospilite_zc.h"
#include "colorlite_zc.h"
#include "turtlelite_zc.h"
#include "filllite_zc.h"
#include "initlite_zc.h"
#include "deflite_zc.h"
#include "printlite_zc.h"
#include "msglite_zc.h"
#include "menulite_zc.h"
#include "findlite_zc.h"
#include "fslite_zc.h"
#include "setuplite_zc.h"
#include "ttlite_zc.h"
#include "buflite_zc.h"
#include "inclite_zc.h"
#include "dclite_zc.h"
#include "linedclite_zc.h"
#include "grflite_zc.h"
#include "movelite_zc.h"
#include "checkedlite_zc.h"
#include "cmplite_zc.h"
#include "forinclite_zc.h"
#include "microlite_zc.h"
#include "movestacklite_zc.h"
#include "endlite_zc.h"
#include "drawitlite_zc.h"
#include "latticelite_zc.h"
#include "looplite_zc.h"
#include "demolite_zc.h"
#include "eventlite_zc.h"
#include "playlite_zc.h"
#include "inputlite_zc.h"
#include "rightlite_zc.h"
#include "cursorlite_zc.h"
#include "uplite_zc.h"
#include "ticklite_zc.h"
#include "framelite_zc.h"
#include "plotdclite_zc.h"
#include "abortlite_zc.h"
#include "aimmovelite_zc.h"
#include "idlelite_zc.h"
#include "layerlite_zc.h"
#include "endslite_zc.h"
#include "speedlite_zc.h"
#include "midlite_zc.h"
#include "livelite_zc.h"
#include "accellite_zc.h"
#include "restartlite_zc.h"
#include "widthlite_zc.h"
#include "bothcolorlite_zc.h"
#include "menufulllite_zc.h"
#include "menubiglite_zc.h"
#include "trylite_zc.h"
#include "stepcountlite_zc.h"
#include "anglesfulllite_zc.h"
#include "braceangleslite_zc.h"
#include "bracepilite_zc.h"
#include "setmenulite_zc.h"
#include "nearlatticelite_zc.h"
#include "f64iflite_zc.h"
#include "wraplatticelite_zc.h"
#include "menulooplite_zc.h"
#include "idxalllite_zc.h"
#include "disklat_zc.h"
#include "stocklat_zc.h"
#include "depthbuflite_zc.h"
#include "depthrstlite_zc.h"
#include "depthplotlite_zc.h"
#include "depthlinelite_zc.h"
#include "ramblk_zc.h"
#include "namefile_zc.h"
#include "dirlook_zc.h"
#include "dirdel_zc.h"
#include "fopen_zc.h"
#include "fwrite_zc.h"
#include "multiblk_zc.h"
#include "redsea_zc.h"
#include "rsroot_zc.h"
#include "rsfile_zc.h"
#include "rsalloc_zc.h"
#include "rsfree_zc.h"
#include "rsmulti_zc.h"
#include "rscfile_zc.h"
#include "rscwrite_zc.h"
#include "rscseek_zc.h"
#include "rsclib_zc.h"
#include "rspersist_zc.h"
#include "runzc_zc.h"

#define PL011_DR      0x00
#define PL011_FR      0x18
#define PL011_CR      0x30
#define PL011_FR_TXFF (1u << 5)
#define PL011_FR_BUSY (1u << 3)
#define PL011_UARTEN  (1u << 0)
#define PL011_TXE     (1u << 8)
#define PL011_RXE     (1u << 9)

/* QEMU virt GICv3 */
#define GICD_BASE_PHYS 0x08000000ULL
#define GICR_BASE_PHYS 0x080A0000ULL
#define GICR_STRIDE    0x20000ULL
#define GICD_CTLR      0x0000
#define GICR_WAKER     0x0014
#define GICR_TYPER     0x0008
#define GICR_IGROUPR0  0x10080
#define GICR_ISENABLER0 0x10100
#define GICR_ICENABLER0 0x10180
#define GICR_ICPENDR0  0x10280
#define GICR_IPRIORITYR 0x10400
#define GICR_ICFGR1    0x10C04

#define TIMER_PPI 30 /* non-secure EL1 physical timer */

/*
 * ICC_*_EL1 (op0=3, op1=0, CRn=12, CRm=12):
 *   op2=0 IAR1, 1 EOIR1, 2 HPPIR1, 3 BPR1,
 *   op2=4 CTLR, 5 SRE, 6 IGRPEN0, 7 IGRPEN1
 */
#define MRS_ICC(op2, out) \
    __asm__ volatile("mrs %0, S3_0_C12_C12_" #op2 : "=r"(out))
#define MSR_ICC(op2, in) \
    __asm__ volatile("msr S3_0_C12_C12_" #op2 ", %0" ::"r"(in))

extern char exception_vectors[];

static volatile uint8_t *g_uart;
static volatile uint32_t g_timer_irqs;
static volatile uint32_t g_svc_hits;
/* 1 = use GICv2 MMIO CPU interface (Pi); 0 = GICv3 system regs (virt). */
static volatile uint32_t g_gic_v2;
/* When set, non-SVC sync advances ELR by 4 and returns (probe CNTP under UTM/HVF). */
static volatile uint32_t g_sync_recover;
static volatile uint32_t g_sync_skipped;
static volatile uint32_t g_uart_probe;
static volatile uint32_t g_uart_dead;
static volatile uint64_t g_probe_esr;
static volatile uint64_t g_probe_far;
static uint64_t g_hhdm;
static uint64_t g_timer_ticks;

uint64_t g_hc_mem[HC_IR_MEM_WORDS];
uint64_t g_hc_fp;
uint64_t g_hc_alloc;
uint64_t g_hc_glob_words;

/* Guard so a bad slot write cannot clobber the framebuffer pointer. */
static uint64_t g_fb_guard[2];

/* Framebuffer text cursor */
static volatile uint8_t *g_fb;
static uint64_t g_fb_w, g_fb_h, g_fb_pitch;
static uint32_t g_fb_bpp;
static uint32_t g_fb_cx, g_fb_cy;
static uint32_t g_fb_cols, g_fb_rows;

/* BCM2711 PL011 wants 32-bit MMIO; QEMU virt accepts it too. */
static void uart_write(volatile uint8_t *uart, char c) {
    unsigned spin = 0;
    volatile uint32_t *r;
    if (!uart || g_uart_dead) {
        return;
    }
    r = (volatile uint32_t *)(uintptr_t)uart;
    while ((r[PL011_FR / 4] & PL011_FR_TXFF) && spin++ < 100000u) {
        if (g_uart_dead) {
            return;
        }
    }
    if (g_uart_dead) {
        return;
    }
    r[PL011_DR / 4] = (uint32_t)(uint8_t)c;
}

/* Enable TX without touching baud (keep UEFI/firmware divisor). */
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
static void pl011_enable_tx(volatile uint8_t *uart) {
    volatile uint32_t *r;
    uint32_t cr;
    if (!uart) {
        return;
    }
    r = (volatile uint32_t *)(uintptr_t)uart;
    cr = r[PL011_CR / 4];
    cr |= PL011_UARTEN | PL011_TXE | PL011_RXE;
    r[PL011_CR / 4] = cr;
}

static void uart_flush(volatile uint8_t *uart) {
    unsigned spin = 0;
    volatile uint32_t *r;
    if (!uart) {
        return;
    }
    r = (volatile uint32_t *)(uintptr_t)uart;
    while ((r[PL011_FR / 4] & PL011_FR_BUSY) && spin++ < 100000u) {
    }
}
#endif

static void uart_puts(volatile uint8_t *uart, const char *s) {
    if (!uart || !s) {
        return;
    }
    while (*s) {
        if (*s == '\n') {
            uart_write(uart, '\r');
        }
        uart_write(uart, *s++);
    }
}

static void uart_put_u64_hex(volatile uint8_t *uart, uint64_t v) {
    static const char hex[] = "0123456789abcdef";
    if (!uart) {
        return;
    }
    uart_puts(uart, "0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_write(uart, hex[(v >> i) & 0xf]);
    }
}

static void uart_put_u32_dec(volatile uint8_t *uart, uint32_t v) {
    char buf[11];
    int n = 0;
    if (!uart) {
        return;
    }
    if (v == 0) {
        uart_write(uart, '0');
        return;
    }
    while (v && n < 10) {
        buf[n++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (n--) {
        uart_write(uart, buf[n]);
    }
}

static void fb_putpixel(uint32_t x, uint32_t y, uint32_t bgr) {
    if (!g_fb || x >= g_fb_w || y >= g_fb_h) {
        return;
    }
    uint32_t pxb = (g_fb_bpp + 7) / 8;
    volatile uint8_t *p = g_fb + (uint64_t)y * g_fb_pitch + (uint64_t)x * pxb;
    if (pxb >= 3) {
        p[0] = (uint8_t)(bgr & 0xff);
        p[1] = (uint8_t)((bgr >> 8) & 0xff);
        p[2] = (uint8_t)((bgr >> 16) & 0xff);
        if (pxb >= 4) {
            p[3] = 0;
        }
    }
}

/* ZealOS GrPeek: pixel color, or -1 if off-screen. */
static int64_t fb_peek(int32_t x, int32_t y) {
    if (!g_fb || x < 0 || y < 0 || (uint32_t)x >= g_fb_w || (uint32_t)y >= g_fb_h) {
        return -1;
    }
    uint32_t pxb = (g_fb_bpp + 7) / 8;
    volatile uint8_t *p = g_fb + (uint64_t)y * g_fb_pitch + (uint64_t)x * pxb;
    if (pxb < 3) {
        return -1;
    }
    return (int64_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16));
}

/* Fast scanline fill for 32bpp; falls back to putpixel otherwise. */
static void fb_fillrect(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h, uint32_t bgr) {
    if (!g_fb || w == 0 || h == 0) {
        return;
    }
    if (x0 >= g_fb_w || y0 >= g_fb_h) {
        return;
    }
    if (x0 + w > g_fb_w) {
        w = (uint32_t)(g_fb_w - x0);
    }
    if (y0 + h > g_fb_h) {
        h = (uint32_t)(g_fb_h - y0);
    }
    uint32_t pxb = (g_fb_bpp + 7) / 8;
    if (pxb == 4) {
        uint32_t pix = bgr & 0x00ffffffu;
        for (uint32_t y = 0; y < h; y++) {
            volatile uint32_t *row =
                (volatile uint32_t *)(g_fb + (uint64_t)(y0 + y) * g_fb_pitch + (uint64_t)x0 * 4);
            for (uint32_t x = 0; x < w; x++) {
                row[x] = pix;
            }
        }
        return;
    }
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            fb_putpixel(x0 + x, y0 + y, bgr);
        }
    }
}

static void fb_clear(uint32_t bgr) {
    if (!g_fb) {
        return;
    }
    fb_fillrect(0, 0, (uint32_t)g_fb_w, (uint32_t)g_fb_h, bgr);
    g_fb_cx = 0;
    g_fb_cy = 0;
}

static void fb_draw_char(uint32_t col, uint32_t row, char ch, uint32_t fg) {
    if (!g_fb) {
        return;
    }
    /* TempleOS Lattice HUD uses 0xE3=π and 0xE9=θ (M184); ASCII font is 0x20..0x7F. */
    static const uint8_t glyph_pi[8] = {
        0x00, 0x7e, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x00
    };
    static const uint8_t glyph_theta[8] = {
        0x3c, 0x66, 0x60, 0x7c, 0x60, 0x66, 0x3c, 0x00
    };
    unsigned char u = (unsigned char)ch;
    const uint8_t *glyph;
    if (u == 0xe3) {
        glyph = glyph_pi;
    } else if (u == 0xe9) {
        glyph = glyph_theta;
    } else if (u < 32 || u > 127) {
        u = (unsigned char)'?';
        glyph = g_font8[u - 32];
    } else {
        glyph = g_font8[u - 32];
    }
    uint32_t x0 = col * 8;
    uint32_t y0 = row * 8;
    for (uint32_t r = 0; r < 8; r++) {
        uint8_t bits = glyph[r];
        for (uint32_t c = 0; c < 8; c++) {
            if (bits & (0x80u >> c)) {
                fb_putpixel(x0 + c, y0 + r, fg);
            }
        }
    }
}

static void fb_putc(char ch) {
    if (!g_fb) {
        return;
    }
    if (ch == '\n') {
        g_fb_cx = 0;
        if (++g_fb_cy >= g_fb_rows) {
            g_fb_cy = 0;
        }
        return;
    }
    if (ch == '\r') {
        g_fb_cx = 0;
        return;
    }
    fb_draw_char(g_fb_cx, g_fb_cy, ch, 0x00E0E0E0u);
    if (++g_fb_cx >= g_fb_cols) {
        g_fb_cx = 0;
        if (++g_fb_cy >= g_fb_rows) {
            g_fb_cy = 0;
        }
    }
}

static void con_write(char c) {
    if (g_uart) {
        if (c == '\n') {
            uart_write(g_uart, '\r');
        }
        uart_write(g_uart, c);
    }
    fb_putc(c);
}

static void con_puts(const char *s) {
    while (*s) {
        con_write(*s++);
    }
}

static void fb_init_ex(const struct zeal_handoff *h, int clear) {
    if (!h->fb_addr_phys || !h->fb_width || !h->fb_pitch || h->fb_bpp < 24) {
        g_fb = NULL;
        return;
    }
    g_fb = (volatile uint8_t *)(h->hhdm_offset + h->fb_addr_phys);
    g_fb_w = h->fb_width;
    g_fb_h = h->fb_height;
    g_fb_pitch = h->fb_pitch;
    g_fb_bpp = h->fb_bpp;
    g_fb_cols = (uint32_t)(g_fb_w / 8);
    g_fb_rows = (uint32_t)(g_fb_h / 8);
    if (clear) {
        fb_clear(0x00101820u);
    } else {
        /* Keep firmware/Limine splash; start text near bottom. */
        g_fb_cx = 0;
        g_fb_cy = g_fb_rows > 4 ? g_fb_rows - 4 : 0;
    }
}

static void fb_init(const struct zeal_handoff *h) {
    fb_init_ex(h, 1);
}

static inline void *mmio(uint64_t phys) {
    return (void *)(uintptr_t)(g_hhdm + phys);
}

static void gic_init(void) {
    volatile uint32_t *gicd = mmio(GICD_BASE_PHYS);

    uint64_t mpidr;
    __asm__ volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    uint32_t want_aff = (uint32_t)((mpidr & 0xff)
        | (((mpidr >> 8) & 0xff) << 8)
        | (((mpidr >> 16) & 0xff) << 16)
        | (((mpidr >> 32) & 0xff) << 24));

    volatile uint32_t *gicr = NULL;
    for (uint64_t i = 0; i < 8; i++) {
        volatile uint32_t *rd = mmio(GICR_BASE_PHYS + i * GICR_STRIDE);
        uint64_t typer = ((uint64_t)rd[(GICR_TYPER / 4) + 1] << 32)
            | rd[GICR_TYPER / 4];
        if ((uint32_t)(typer >> 32) == want_aff) {
            gicr = rd;
            break;
        }
        if ((typer >> 4) & 1u) {
            break;
        }
    }
    if (!gicr) {
        gicr = mmio(GICR_BASE_PHYS + (mpidr & 0xff) * GICR_STRIDE);
    }

    uint32_t waker = gicr[GICR_WAKER / 4] & ~(1u << 1);
    gicr[GICR_WAKER / 4] = waker;
    __asm__ volatile("dsb sy");
    while (gicr[GICR_WAKER / 4] & (1u << 2)) {
    }

    /* Level-triggered PPI 30 */
    uint32_t icfgr1 = gicr[GICR_ICFGR1 / 4];
    icfgr1 &= ~(3u << ((TIMER_PPI - 16) * 2));
    gicr[GICR_ICFGR1 / 4] = icfgr1;

    gicr[GICR_ICENABLER0 / 4] = (1u << TIMER_PPI);
    gicr[GICR_ICPENDR0 / 4] = (1u << TIMER_PPI);
    __asm__ volatile("dsb sy");

    gicr[GICR_IGROUPR0 / 4] |= (1u << TIMER_PPI);
    ((volatile uint8_t *)((uintptr_t)gicr + GICR_IPRIORITYR))[TIMER_PPI] = 0x80;
    __asm__ volatile("dsb sy");
    gicr[GICR_ISENABLER0 / 4] = (1u << TIMER_PPI);
    __asm__ volatile("dsb sy");

    /* ARE_NS | EnableGrp1NS */
    gicd[GICD_CTLR / 4] = (1u << 4) | (1u << 1);
    __asm__ volatile("dsb sy");

    uint64_t sre;
    MRS_ICC(5, sre);
    sre |= 0x7;
    MSR_ICC(5, sre);
    __asm__ volatile("isb");

    __asm__ volatile("msr S3_0_C4_C6_0, %0" ::"r"(0xffull)); /* PMR */
    MSR_ICC(3, 0ull); /* BPR1 */
    MSR_ICC(7, 1ull); /* IGRPEN1 — was wrongly written as 0 before */
    __asm__ volatile("isb");

    g_gic_v2 = 0;

    if (g_uart) {
        uint64_t grp1, hppir;
        MRS_ICC(7, grp1);
        MRS_ICC(2, hppir);
        uart_puts(g_uart, "GICR ISEN=");
        uart_put_u64_hex(g_uart, gicr[GICR_ISENABLER0 / 4]);
        uart_puts(g_uart, " IGRPEN1=");
        uart_put_u64_hex(g_uart, grp1);
        uart_puts(g_uart, " HPPIR1=");
        uart_put_u64_hex(g_uart, hppir);
        uart_puts(g_uart, "\n");
    }
}

/* GIC-400 (GICv2) on Pi4 — map Device if raw HHDM aborts (FF range). */
#define GICV2_GICD_CTLR       0x000
#define GICV2_GICD_ISENABLER  0x100
#define GICV2_GICD_ICENABLER  0x180
#define GICV2_GICD_ICPENDR    0x280
#define GICV2_GICD_IPRIORITY  0x400
#define GICV2_GICD_ICFGR      0xC00
#define GICV2_GICC_CTLR       0x00
#define GICV2_GICC_PMR        0x04
#define GICV2_GICC_BPR        0x08
#define GICV2_GICC_IAR        0x0C
#define GICV2_GICC_EOIR       0x10

static uint64_t g_gicd_phys = PI4_GICD_PHYS;
static uint64_t g_gicc_phys = PI4_GICC_PHYS;

static int gicv2_probe_read(uint64_t hhdm, uint64_t gicd_phys) {
    volatile uint32_t *gicd = (volatile uint32_t *)(uintptr_t)(hhdm + gicd_phys);
    g_uart_probe = 1;
    g_sync_recover = 1;
    g_uart_dead = 0;
    (void)gicd[GICV2_GICD_CTLR / 4];
    __asm__ volatile("dsb sy");
    g_sync_recover = 0;
    g_uart_probe = 0;
    return g_uart_dead ? -1 : 0;
}

static int gicv2_init(uint64_t hhdm) {
    volatile uint32_t *gicd;
    volatile uint32_t *gicc;
    unsigned ppi = (unsigned)PI4_CNTP_PPI;

    g_gicd_phys = PI4_GICD_PHYS;
    g_gicc_phys = PI4_GICC_PHYS;

    if (gicv2_probe_read(hhdm, g_gicd_phys) != 0) {
        /* Punch Device 2MiB covering GICD+GICC (same as UART fix path). */
        if (!g_pt_pool_phys || mmio_map_2m(hhdm, PI4_GICD_PHYS) != 0) {
            return -1;
        }
        if (gicv2_probe_read(hhdm, g_gicd_phys) != 0) {
            /* Legacy ARM-local alias used by some firmware builds. */
            g_gicd_phys = 0x40041000ull;
            g_gicc_phys = 0x40042000ull;
            if (mmio_map_2m(hhdm, g_gicd_phys) != 0 ||
                gicv2_probe_read(hhdm, g_gicd_phys) != 0) {
                return -2;
            }
        }
    }

    gicd = (volatile uint32_t *)(uintptr_t)(hhdm + g_gicd_phys);
    gicc = (volatile uint32_t *)(uintptr_t)(hhdm + g_gicc_phys);

    /* Disable distributor while programming. */
    gicd[GICV2_GICD_CTLR / 4] = 0;
    __asm__ volatile("dsb sy");

    /* PPI 30: clear pending, priority, level-sensitive, enable. */
    gicd[(GICV2_GICD_ICPENDR / 4) + (ppi / 32)] = (1u << (ppi % 32));
    ((volatile uint8_t *)((uintptr_t)gicd + GICV2_GICD_IPRIORITY))[ppi] = 0xa0;
    {
        unsigned idx = ppi / 16;
        unsigned shift = (ppi % 16) * 2;
        uint32_t cfg = gicd[(GICV2_GICD_ICFGR / 4) + idx];
        cfg &= ~(3u << shift); /* level */
        gicd[(GICV2_GICD_ICFGR / 4) + idx] = cfg;
    }
    gicd[(GICV2_GICD_ICENABLER / 4) + (ppi / 32)] = (1u << (ppi % 32));
    __asm__ volatile("dsb sy");
    gicd[(GICV2_GICD_ISENABLER / 4) + (ppi / 32)] = (1u << (ppi % 32));
    __asm__ volatile("dsb sy");

    gicd[GICV2_GICD_CTLR / 4] = 1; /* EnableGrp0 */
    __asm__ volatile("dsb sy");

    gicc[GICV2_GICC_PMR / 4] = 0xff;
    gicc[GICV2_GICC_BPR / 4] = 0;
    gicc[GICV2_GICC_CTLR / 4] = 1; /* EnableGrp0 */
    __asm__ volatile("dsb sy");
    __asm__ volatile("isb");

    g_gic_v2 = 1;
    return 0;
}

static void timer_arm(void) {
    __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(0ull));
    __asm__ volatile("isb");
    __asm__ volatile("msr cntp_tval_el0, %0" ::"r"(g_timer_ticks));
    __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(1ull));
    __asm__ volatile("isb");
}

static void vectors_install(void) {
    __asm__ volatile(
        "mov x0, sp\n"
        "msr spsel, #1\n"
        "mov sp, x0\n"
        "isb\n"
        :
        :
        : "x0", "memory");
    /* PC-relative — works even before GOT relocs (belt and suspenders). */
    uint64_t vbar;
    __asm__ volatile("adr %0, exception_vectors" : "=r"(vbar));
    __asm__ volatile("msr vbar_el1, %0" ::"r"(vbar));
    __asm__ volatile("isb");
}

void exc_handle_fatal(uint64_t unused) {
    (void)unused;
    if (g_uart) {
        uart_puts(g_uart, "FATAL exception\n");
    }
    for (;;) {
        __asm__ volatile("wfi");
    }
}

/* Digits-only hex — safe if .rodata/statics are unhealthy mid-fault. */
static void uart_put_hex_raw(volatile uint8_t *uart, uint64_t v) {
    uart_write(uart, '0');
    uart_write(uart, 'x');
    for (int i = 60; i >= 0; i -= 4) {
        unsigned n = (unsigned)((v >> i) & 0xf);
        uart_write(uart, (char)(n < 10 ? '0' + n : 'a' + (n - 10)));
    }
}

void exc_handle_sync(uint64_t *frame) {
    (void)frame;
    uint64_t esr, elr, far;
    __asm__ volatile("mrs %0, esr_el1" : "=r"(esr));
    __asm__ volatile("mrs %0, elr_el1" : "=r"(elr));
    __asm__ volatile("mrs %0, far_el1" : "=r"(far));
    uint64_t ec = (esr >> 26) & 0x3f;
    if (ec == 0x15) {
        g_svc_hits++;
        if (g_uart) {
            uart_puts(g_uart, "sync: SVC ok (hits=");
            uart_put_u32_dec(g_uart, g_svc_hits);
            uart_puts(g_uart, ")\n");
        }
        return;
    }
    /* UTM/HVF may trap CNTP_* (EL1PCEN) while CNTFRQ still works. Skip insn.
     * PI-DIAG also uses this around first UART DR touches. */
    if (g_sync_recover) {
        g_sync_skipped++;
        if (g_uart_probe) {
            uint64_t esr, far;
            __asm__ volatile("mrs %0, esr_el1" : "=r"(esr));
            __asm__ volatile("mrs %0, far_el1" : "=r"(far));
            g_probe_esr = esr;
            g_probe_far = far;
            g_uart_dead = 1;
        }
        elr += 4;
        __asm__ volatile("msr elr_el1, %0" ::"r"(elr));
        return;
    }
    if (g_uart) {
        uart_puts(g_uart, "sync: ESR=");
        uart_put_hex_raw(g_uart, esr);
        uart_puts(g_uart, " EC=");
        uart_put_hex_raw(g_uart, ec);
        uart_puts(g_uart, " ELR=");
        uart_put_hex_raw(g_uart, elr);
        uart_puts(g_uart, " FAR=");
        uart_put_hex_raw(g_uart, far);
        uart_puts(g_uart, "\n");
    }
    for (;;) {
        __asm__ volatile("wfi");
    }
}

void exc_handle_irq(uint64_t *frame) {
    (void)frame;
    uint32_t id;

    if (g_gic_v2) {
        volatile uint32_t *gicc =
            (volatile uint32_t *)(uintptr_t)(g_hhdm + g_gicc_phys);
        id = gicc[GICV2_GICC_IAR / 4] & 0x3ffu;
    } else {
        uint64_t iar;
        MRS_ICC(0, iar);
        id = (uint32_t)(iar & 0x3ff);
    }

    if (id == TIMER_PPI || id == (uint32_t)PI4_CNTP_PPI) {
        g_timer_irqs++;
        if (g_uart) {
            uart_puts(g_uart, "irq: CNTP tick ");
            uart_put_u32_dec(g_uart, g_timer_irqs);
            uart_puts(g_uart, "\n");
        }
        timer_arm();
    } else if (g_uart && id < 1020) {
        uart_puts(g_uart, "irq: id=");
        uart_put_u32_dec(g_uart, id);
        uart_puts(g_uart, "\n");
    }

    if (id < 1020) {
        if (g_gic_v2) {
            volatile uint32_t *gicc =
                (volatile uint32_t *)(uintptr_t)(g_hhdm + g_gicc_phys);
            gicc[GICV2_GICC_EOIR / 4] = id;
        } else {
            uint64_t iar = id;
            MSR_ICC(1, iar);
        }
    }
}

static void enable_irq(void) {
    __asm__ volatile("msr daifclr, #2");
    __asm__ volatile("isb");
}

static void shell_run(void);

/* Limine-mapped RWX scratch for emit-and-run (HolyC/JIT bridge). */
__attribute__((section(".jit"), aligned(64)))
static uint32_t g_jit_buf[8192]; /* was 1536 — Lattice angles[35] brace init */
__attribute__((section(".jit"), aligned(64)))
static uint32_t g_fn_code[HC_MAX_FNS][16384]; /* was 8192 — Lattice NearLattice + if(tt.w) */

static struct hc_fn_table g_fn_tbl;
static void jit_icache_flush(void *addr, size_t size) {
    uintptr_t start = (uintptr_t)addr & ~63ull;
    uintptr_t end = ((uintptr_t)addr + size + 63) & ~63ull;
    for (uintptr_t p = start; p < end; p += 64) {
        __asm__ volatile("dc cvau, %0" ::"r"(p) : "memory");
    }
    __asm__ volatile("dsb ish");
    for (uintptr_t p = start; p < end; p += 64) {
        __asm__ volatile("ic ivau, %0" ::"r"(p) : "memory");
    }
    __asm__ volatile("dsb ish");
    __asm__ volatile("isb");
}

/* Called from JITed / IR-compiled code. */
static void jit_host_puts(void) {
    con_puts("hc: host callback OK\n");
}

static uint64_t hc_host_marker(void) {
    return 0x48434F4BULL; /* "HCOK" */
}

static uint64_t hc_host_inc(uint64_t x) {
    return x + 1;
}

uint64_t hc_builtin_abs(uint64_t x) {
    int64_t v = (int64_t)x;
    return (uint64_t)(v < 0 ? -v : v);
}

/* F64 abs: clear sign bit (no FP ops; kernel stays +nofp). */
uint64_t hc_builtin_fabs(uint64_t xbits) {
    return xbits & 0x7fffffffffffffffULL;
}

/* Lattice DCFill(dc) / DCDel(dc) stubs — CDC body deferred beyond color. */
/* Minimal CDC for Lattice: color @0; depth_buf @8 (DCDepthBufAlloc). */
struct hc_cdc {
    uint64_t color;
    uint64_t depth_buf;
};
static struct hc_cdc g_hc_cdc;
/* Sparse z-buffer (tiny mcmodel — full FB BSS is too large). */
#define HC_DEPTH_MAP 2048
struct hc_depth_cell {
    uint16_t x;
    uint16_t y;
    int32_t z;
};
static struct hc_depth_cell g_hc_depth_map[HC_DEPTH_MAP];
static int g_hc_depth_on;

/* PopUpColor state (M148+); painted on FB for latticeplay (M160/M164/M165). */
static int g_hc_popup_i;
static int g_hc_popup_live;
static uint8_t g_hc_popup_mid = 14;  /* YELLOW — matches TurtleInit */
static uint8_t g_hc_popup_edge = 0;  /* BLACK */
static void hc_popup_paint_swatch(int which, uint64_t c) {
    static const uint32_t pal[16] = {
        0x00000000u, 0x000000AAu, 0x0000AA00u, 0x0000AAAAu, 0x00AA0000u, 0x00AA00AAu,
        0x00AA5500u, 0x00AAAAAAu, 0x00555555u, 0x005555FFu, 0x0055FF55u, 0x0055FFFFu,
        0x00FF5555u, 0x00FF55FFu, 0x00FFFF55u, 0x00FFFFFFu,
    };
    const char *lab = which ? "Edge" : "Mid";
    uint32_t x0 = which ? 72u : 8u;
    uint32_t y0 = (uint32_t)g_fb_h > 40u ? (uint32_t)g_fb_h - 28u : 8u;
    uint32_t col = x0 / 8u;
    uint32_t row = (y0 >= 8u ? y0 - 8u : 0u) / 8u;
    int k;
    if (!g_fb) {
        return;
    }
    fb_fillrect(x0, y0, 56u, 20u, pal[(int)c & 15]);
    fb_fillrect(x0, y0, 56u, 1u, 0x00E0E0E0u);
    fb_fillrect(x0, y0 + 19u, 56u, 1u, 0x00E0E0E0u);
    fb_fillrect(x0, y0, 1u, 20u, 0x00E0E0E0u);
    fb_fillrect(x0 + 55u, y0, 1u, 20u, 0x00E0E0E0u);
    for (k = 0; lab[k]; k++) {
        fb_draw_char(col + (uint32_t)k, row, lab[k], 0x00E0E0E0u);
    }
}

/* M165–M169: repaint Mid/Edge + controls hints so Cls/Refresh keep chrome. */
static void hc_popup_paint_live(void) {
    static const char hint[] = "Esc=exit Enter=restart Space=step c=color +/-=w";
    static const char hint2[] = "L-click=place R-drag=aim e=ends";
    static const char hint3[] = "arrows=di/speed 0-9=layer";
    uint32_t i;
    if (!g_hc_popup_live) {
        return;
    }
    hc_popup_paint_swatch(0, g_hc_popup_mid);
    hc_popup_paint_swatch(1, g_hc_popup_edge);
    /* Bands match GrPrint(dc, 0, 16/24/32, …) — idle DrawIt must not bury the cues. */
    if (g_fb) {
        fb_fillrect(0, 16u, 64u * 8u, 24u, 0);
        for (i = 0; hint[i]; i++) {
            fb_draw_char(i, 2u, hint[i], 0x00E0E0E0u);
        }
        for (i = 0; hint2[i]; i++) {
            fb_draw_char(i, 3u, hint2[i], 0x00E0E0E0u);
        }
        for (i = 0; hint3[i]; i++) {
            fb_draw_char(i, 4u, hint3[i], 0x00E0E0E0u);
        }
    }
}

static void hc_depth_map_clear(void) {
    int i;
    for (i = 0; i < HC_DEPTH_MAP; i++) {
        g_hc_depth_map[i].x = 0xFFFFu;
        g_hc_depth_map[i].y = 0xFFFFu;
        g_hc_depth_map[i].z = (int32_t)0x7fffffff;
    }
}

uint64_t hc_builtin_dcfill(uint64_t dc) {
    (void)dc;
    /* Lattice DCFill — clear virt FB so DiskLat/NearLattice leave a clean surface. */
    if (g_fb) {
        fb_clear(0);
    }
    /* M174: Restart must not keep stale z cells from prior strokes. */
    if (g_hc_depth_on) {
        hc_depth_map_clear();
    }
    /* M164: latticeplay Restart TurtleInit → YELLOW/BLACK; sync picker + swatches. */
    if (g_hc_popup_live) {
        g_hc_popup_i = 0;
        g_hc_popup_mid = 14;
        g_hc_popup_edge = 0;
        hc_popup_paint_live();
    }
    return 0;
}

uint64_t hc_builtin_dcdel(uint64_t dc) {
    (void)dc;
    return 0;
}

uint64_t hc_builtin_dcdepthbufreset(uint64_t dc) {
    struct hc_cdc *d;

    if (!dc) {
        return 0;
    }
    d = (struct hc_cdc *)(uintptr_t)dc;
    if (!d->depth_buf) {
        return 0;
    }
    hc_depth_map_clear();
    return d->depth_buf;
}

uint64_t hc_builtin_dcdepthbufalloc(uint64_t dc) {
    struct hc_cdc *d;

    if (!dc) {
        return 0;
    }
    d = (struct hc_cdc *)(uintptr_t)dc;
    g_hc_depth_on = 1;
    /* Non-null sentinel — HolyC reads dc->depth_buf; map lives in g_hc_depth_map. */
    d->depth_buf = (uint64_t)(uintptr_t)g_hc_depth_map;
    return hc_builtin_dcdepthbufreset(dc);
}

/* Return 1 if plot allowed (closer z); 0 if occluded. */
static int hc_depth_try(int32_t xi, int32_t yi, int32_t zi) {
    uint32_t h;
    int i;

    if (!g_hc_depth_on || xi < 0 || yi < 0 || xi > 0xFFFE || yi > 0xFFFE) {
        return 1;
    }
    h = (uint32_t)xi * 2654435761u ^ (uint32_t)yi * 40503u;
    for (i = 0; i < HC_DEPTH_MAP; i++) {
        uint32_t idx = (h + (uint32_t)i) % (uint32_t)HC_DEPTH_MAP;
        struct hc_depth_cell *c = &g_hc_depth_map[idx];
        if (c->x == 0xFFFFu) {
            c->x = (uint16_t)xi;
            c->y = (uint16_t)yi;
            c->z = zi;
            return 1;
        }
        if (c->x == (uint16_t)xi && c->y == (uint16_t)yi) {
            if (zi > c->z) {
                return 0;
            }
            c->z = zi;
            return 1;
        }
    }
    /* Map full — allow plot (bring-up). */
    return 1;
}

/* Non-interactive Lattice color picker (M148/M150/M160/M161/M164/M165).
 * HolyC string args to PopUpColor are not yet reliable C pointers here —
 * use Mid/Edge call-pair order (reset each hc_run_src). UART RX clears RSR.
 * M160: paint FB swatches. M161: latticeplay cycles Mid/Edge pairs after the first.
 * M164: DCFill (Restart) resets live picker + default Mid/Edge swatches.
 * M165: track Mid/Edge so Cls/Refresh keep swatches from entry through idle. */
uint64_t hc_builtin_popupcolor(uint64_t header) {
    (void)header;
    {
        /* Pair 0 stays YELLOW/BLACK for smokes + first latticeplay 'c'. */
        static const uint8_t mids[] = {14, 4, 9, 10, 12, 11, 13, 2, 5, 15};
        static const uint8_t edges[] = {0, 15, 0, 15, 0, 0, 0, 15, 0, 0};
        int i = g_hc_popup_i++;
        uint64_t c;
        if (g_hc_popup_live) {
            int pair = (i / 2) % (int)(sizeof mids / sizeof mids[0]);
            c = (i & 1) ? (uint64_t)edges[pair] : (uint64_t)mids[pair];
        } else {
            c = (i & 1) ? 0 : 14; /* Edge BLACK, Mid YELLOW */
        }
        if (i & 1) {
            g_hc_popup_edge = (uint8_t)(c & 15);
        } else {
            g_hc_popup_mid = (uint8_t)(c & 15);
        }
        hc_popup_paint_swatch(i & 1, c);
        return c;
    }
}

/* Scripted MessageGet queue for Lattice while(TRUE) event-loop smokes. */
#define HC_MSG_QUE_MAX 16
#define HC_MSG_BIT(t) (1ULL << (uint64_t)(t))
static uint64_t g_hc_msg_type[HC_MSG_QUE_MAX];
static uint64_t g_hc_msg_a1[HC_MSG_QUE_MAX];
static uint64_t g_hc_msg_a2[HC_MSG_QUE_MAX];
static int g_hc_msg_head;
static int g_hc_msg_tail;
/* Virtio edge/move state for MessageGet when the scripted queue is empty. */
static int g_hc_msg_btn_prev;
static int g_hc_msg_rbtn_prev;
static int g_hc_msg_have_xy;
static uint32_t g_hc_msg_last_x, g_hc_msg_last_y;

static void hc_msg_que_reset(void) {
    g_hc_msg_head = 0;
    g_hc_msg_tail = 0;
    g_hc_msg_btn_prev = 0;
    g_hc_msg_rbtn_prev = 0;
    g_hc_msg_have_xy = 0;
}

/* Bring-up: MsgQuePush(type, arg1, arg2) seeds the queue for LoopLite. */
uint64_t hc_builtin_msgquepush(uint64_t type, uint64_t a1, uint64_t a2) {
    int next = (g_hc_msg_tail + 1) % HC_MSG_QUE_MAX;
    if (next == g_hc_msg_head) {
        return 0; /* full */
    }
    g_hc_msg_type[g_hc_msg_tail] = type;
    g_hc_msg_a1[g_hc_msg_tail] = a1;
    g_hc_msg_a2[g_hc_msg_tail] = a2;
    g_hc_msg_tail = next;
    return 1;
}

static void hc_msg_store_outs(uint64_t p_arg1, uint64_t p_arg2, uint64_t a1, uint64_t a2) {
    if (p_arg1) {
        *(volatile uint64_t *)(uintptr_t)p_arg1 = a1;
    }
    if (p_arg2) {
        *(volatile uint64_t *)(uintptr_t)p_arg2 = a2;
    }
}

static int uart_getc_nb(void); /* Pi MessageGet fallback; defined with PL011 RX */

/* Pop scripted event (mask-filtered), else virtio (QEMU/UTM) or UART (Pi). */
uint64_t hc_builtin_messageget(uint64_t p_arg1, uint64_t p_arg2, uint64_t mask) {
    while (g_hc_msg_head != g_hc_msg_tail) {
        uint64_t t = g_hc_msg_type[g_hc_msg_head];
        uint64_t a1 = g_hc_msg_a1[g_hc_msg_head];
        uint64_t a2 = g_hc_msg_a2[g_hc_msg_head];
        g_hc_msg_head = (g_hc_msg_head + 1) % HC_MSG_QUE_MAX;
        if (mask & HC_MSG_BIT(t)) {
            hc_msg_store_outs(p_arg1, p_arg2, a1, a2);
            return t;
        }
        /* Drop events outside mask (scripted smokes push only wanted types). */
    }

    /* KEY_DOWN/KEY_UP: virtio typed msgs when present; UART ASCII KEY_DOWN only. */
    if ((mask & (HC_MSG_BIT(2) | HC_MSG_BIT(3))) && virtio_kbd_ready()) {
        uint8_t t = 0;
        uint64_t a1 = 0, a2 = 0;
        while (virtio_kbd_msg_nb(&t, &a1, &a2)) {
            if (mask & HC_MSG_BIT(t)) {
                hc_msg_store_outs(p_arg1, p_arg2, a1, a2);
                return t;
            }
            /* Drop events outside mask (same as scripted queue). */
        }
    } else if ((mask & HC_MSG_BIT(2)) && !virtio_kbd_ready()) {
        int c = uart_getc_nb();
        if (c >= 0) {
            /* Serial Enter is often CR; Lattice Restart is '\n'. */
            if (c == '\r') {
                c = '\n';
            }
            hc_msg_store_outs(p_arg1, p_arg2, (uint64_t)(int64_t)c, 0);
            return 2;
        }
    }
    if (virtio_tablet_ready() && g_fb) {
        uint32_t x = 0, y = 0;
        int btns = virtio_tablet_buttons();
        (void)virtio_tablet_xy((uint32_t)g_fb_w, (uint32_t)g_fb_h, &x, &y);
        int btn = btns & 1;
        int rbtn = (btns & 2) ? 1 : 0;
        int prev = g_hc_msg_btn_prev;
        int rprev = g_hc_msg_rbtn_prev;
        g_hc_msg_btn_prev = btn;
        g_hc_msg_rbtn_prev = rbtn;
        if ((mask & HC_MSG_BIT(5)) && btn && !prev) { /* MS_L_DOWN */
            hc_msg_store_outs(p_arg1, p_arg2, x, y);
            return 5;
        }
        if ((mask & HC_MSG_BIT(6)) && !btn && prev) { /* MS_L_UP */
            hc_msg_store_outs(p_arg1, p_arg2, x, y);
            return 6;
        }
        if ((mask & HC_MSG_BIT(7)) && rbtn && !rprev) { /* MS_R_DOWN */
            hc_msg_store_outs(p_arg1, p_arg2, x, y);
            return 7;
        }
        if ((mask & HC_MSG_BIT(8)) && !rbtn && rprev) { /* MS_R_UP */
            hc_msg_store_outs(p_arg1, p_arg2, x, y);
            return 8;
        }
        if ((mask & HC_MSG_BIT(4)) && g_hc_msg_have_xy &&
            (x != g_hc_msg_last_x || y != g_hc_msg_last_y)) { /* MS_MOVE */
            g_hc_msg_last_x = x;
            g_hc_msg_last_y = y;
            hc_msg_store_outs(p_arg1, p_arg2, x, y);
            return 4;
        }
        g_hc_msg_last_x = x;
        g_hc_msg_last_y = y;
        g_hc_msg_have_xy = 1;
    }

    hc_msg_store_outs(p_arg1, p_arg2, 0, 0);
    return 0; /* MESSAGE_NULL */
}

uint64_t hc_builtin_menupush(uint64_t s) {
    (void)s;
    return 0;
}

uint64_t hc_builtin_menupop(void) {
    return 0;
}

uint64_t hc_builtin_nop0(void) {
    return 0;
}

uint64_t g_hc_fs_win_inhibit;
uint64_t g_hc_fs_draw_it;
uint64_t g_hc_fs_cur_menu;

/* Lattice CMenuEntry stubs — MenuEntryFind returns one entry per path. */
struct hc_menu_entry {
    uint64_t checked;
};
#define HC_MENU_PATH_MAX 32
#define HC_MENU_ENTRIES  12 /* Layer0..9 + Ends + fallback */
struct hc_menu_slot {
    char path[HC_MENU_PATH_MAX];
    struct hc_menu_entry ent;
};
static struct hc_menu_slot g_hc_menu_slots[HC_MENU_ENTRIES];
static int g_hc_menu_nslots;

static int hc_menu_streq(const char *a, const char *b) {
    if (!a || !b) {
        return 0;
    }
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static void hc_menu_path_copy(char *dst, const char *src, int cap) {
    int i = 0;
    if (!src || cap < 1) {
        if (cap > 0) {
            dst[0] = 0;
        }
        return;
    }
    while (src[i] && i < cap - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

uint64_t hc_builtin_menuentryfind(uint64_t menu, uint64_t path) {
    (void)menu;
    const char *p = (const char *)(uintptr_t)path;
    if (!p) {
        p = "";
    }
    for (int i = 0; i < g_hc_menu_nslots; i++) {
        if (hc_menu_streq(g_hc_menu_slots[i].path, p)) {
            return (uint64_t)(uintptr_t)&g_hc_menu_slots[i].ent;
        }
    }
    if (g_hc_menu_nslots >= HC_MENU_ENTRIES) {
        /* Reuse last slot for overflow paths. */
        hc_menu_path_copy(g_hc_menu_slots[HC_MENU_ENTRIES - 1].path, p, HC_MENU_PATH_MAX);
        return (uint64_t)(uintptr_t)&g_hc_menu_slots[HC_MENU_ENTRIES - 1].ent;
    }
    hc_menu_path_copy(g_hc_menu_slots[g_hc_menu_nslots].path, p, HC_MENU_PATH_MAX);
    g_hc_menu_slots[g_hc_menu_nslots].ent.checked = 0;
    g_hc_menu_nslots++;
    return (uint64_t)(uintptr_t)&g_hc_menu_slots[g_hc_menu_nslots - 1].ent;
}

static uint32_t hc_dc_rgb(uint64_t dc) {
    static const uint32_t pal[16] = {
        0x00000000u, 0x000000AAu, 0x0000AA00u, 0x0000AAAAu, 0x00AA0000u, 0x00AA00AAu,
        0x00AA5500u, 0x00AAAAAAu, 0x00555555u, 0x005555FFu, 0x0055FF55u, 0x0055FFFFu,
        0x00FF5555u, 0x00FF55FFu, 0x00FFFF55u, 0x00FFFFFFu,
    };
    if (!dc) {
        return 0x00FFFF00u; /* Plot3Lite bring-up yellow when dc==0 */
    }
    uint64_t c = *(volatile uint64_t *)(uintptr_t)dc;
    if (c < 16) {
        return pal[(int)c];
    }
    return (uint32_t)c;
}

/* Lattice DCAlias(dc=NULL,task=NULL) → shared CDC stub. */
uint64_t hc_builtin_dcalias(void) {
    return (uint64_t)(uintptr_t)&g_hc_cdc;
}

/* Lattice Refresh — WinMgr-shaped call of Fs->draw_it(task, dc). */
uint64_t hc_builtin_refresh(void) {
    if (!g_hc_fs_draw_it) {
        return 0;
    }
    typedef void (*hc_draw_it_fn)(uint64_t task, uint64_t dc);
    ((hc_draw_it_fn)(uintptr_t)g_hc_fs_draw_it)(0, (uint64_t)(uintptr_t)&g_hc_cdc);
    /* M165: DrawIt/TurtleMove can cover bottom swatches — keep them on top. */
    hc_popup_paint_live();
    return 1;
}

uint64_t hc_builtin_fs_pix_width(void) {
    return g_fb_w;
}

uint64_t hc_builtin_fs_pix_height(void) {
    return g_fb_h;
}

uint64_t hc_builtin_min(uint64_t a, uint64_t b) {
    return (int64_t)a < (int64_t)b ? a : b;
}

uint64_t hc_builtin_max(uint64_t a, uint64_t b) {
    return (int64_t)a > (int64_t)b ? a : b;
}

uint64_t hc_builtin_sign(uint64_t x) {
    int64_t v = (int64_t)x;
    if (v < 0) {
        return (uint64_t)(int64_t)-1;
    }
    if (v > 0) {
        return 1;
    }
    return 0;
}

uint64_t hc_builtin_clamp(uint64_t v, uint64_t lo, uint64_t hi) {
    int64_t x = (int64_t)v, a = (int64_t)lo, b = (int64_t)hi;
    if (x < a) {
        return (uint64_t)a;
    }
    if (x > b) {
        return (uint64_t)b;
    }
    return (uint64_t)x;
}

uint64_t hc_builtin_sqr(uint64_t x) {
    int64_t v = (int64_t)x;
    return (uint64_t)(v * v);
}

uint64_t hc_builtin_putpixel(uint64_t x, uint64_t y, uint64_t c) {
    fb_putpixel((uint32_t)x, (uint32_t)y, (uint32_t)c);
    return 0;
}

/* ZealOS GrPlot(dc,x,y) or bring-up GrPlot(x,y,color).
 * Heuristic: a0 > 0xFFFF → CDC pointer (Lattice); else (x,y,color). */
uint64_t hc_builtin_grplot(uint64_t a0, uint64_t a1, uint64_t a2) {
    uint64_t x, y, c;
    if (a0 > 0xFFFFull) {
        x = a1;
        y = a2;
        c = hc_dc_rgb(a0);
    } else {
        x = a0;
        y = a1;
        c = a2;
    }
    int32_t xi = (int32_t)x, yi = (int32_t)y;
    if (xi < 0 || yi < 0 || (uint32_t)xi >= g_fb_w || (uint32_t)yi >= g_fb_h) {
        return 0;
    }
    fb_putpixel((uint32_t)xi, (uint32_t)yi, (uint32_t)c);
    return 1;
}

/* Lattice GrPlot3(dc,x,y,z): 2D project; z-test when depth_buf set. */
uint64_t hc_builtin_grplot3(uint64_t dc, uint64_t x, uint64_t y, uint64_t z) {
    int32_t xi = (int32_t)x, yi = (int32_t)y, zi = (int32_t)z;
    if (xi < 0 || yi < 0 || (uint32_t)xi >= g_fb_w || (uint32_t)yi >= g_fb_h) {
        return 0;
    }
    if (dc) {
        struct hc_cdc *d = (struct hc_cdc *)(uintptr_t)dc;
        if (d->depth_buf && !hc_depth_try(xi, yi, zi)) {
            return 0;
        }
    }
    fb_putpixel((uint32_t)xi, (uint32_t)yi, hc_dc_rgb(dc));
    return 1;
}

/* Lattice GrLine3(dc,x1,y1,z1,x2,y2,z2): 2D Bresenham + optional z-test. */
uint64_t hc_builtin_grline3(uint64_t dc, uint64_t x1, uint64_t y1, uint64_t z1,
                            uint64_t x2, uint64_t y2, uint64_t z2) {
    int32_t xa = (int32_t)x1, ya = (int32_t)y1, xb = (int32_t)x2, yb = (int32_t)y2;
    int32_t za = (int32_t)z1, zb = (int32_t)z2;
    int32_t dx = xa < xb ? xb - xa : xa - xb;
    int32_t sx = xa < xb ? 1 : -1;
    int32_t dy = ya < yb ? ya - yb : yb - ya;
    int32_t sy = ya < yb ? 1 : -1;
    int32_t err = dx + dy;
    int32_t steps;
    int32_t i;
    uint32_t col = hc_dc_rgb(dc);
    int use_depth = 0;
    int plotted = 0;

    if (dc) {
        struct hc_cdc *d = (struct hc_cdc *)(uintptr_t)dc;
        use_depth = d->depth_buf != 0;
    }
    steps = dx > -dy ? dx : -dy;
    if (steps < 1) {
        steps = 1;
    }
    i = 0;
    for (;;) {
        int32_t zi = za + (int32_t)(((int64_t)(zb - za) * i) / steps);
        if (xa >= 0 && ya >= 0 && (uint32_t)xa < g_fb_w && (uint32_t)ya < g_fb_h) {
            if (!use_depth || hc_depth_try(xa, ya, zi)) {
                fb_putpixel((uint32_t)xa, (uint32_t)ya, col);
                plotted = 1;
            }
        }
        if (xa == xb && ya == yb) {
            break;
        }
        {
            int32_t e2 = err << 1;
            if (e2 >= dy) {
                err += dy;
                xa += sx;
            }
            if (e2 <= dx) {
                err += dx;
                ya += sy;
            }
        }
        i++;
    }
    return plotted ? 1 : 0;
}

/* ZealOS GrPeek(dc,x,y) — ignore dc; color or -1. */
uint64_t hc_builtin_grpeek_dc(uint64_t dc, uint64_t x, uint64_t y) {
    (void)dc;
    return (uint64_t)fb_peek((int32_t)x, (int32_t)y);
}

/* Bring-up GrPeek(x,y); color or -1. */
uint64_t hc_builtin_grpeek(uint64_t x, uint64_t y) {
    return (uint64_t)fb_peek((int32_t)x, (int32_t)y);
}

uint64_t hc_builtin_cls(uint64_t c) {
    fb_clear((uint32_t)c);
    /* M175: latticeplay entry Cls — same stale-z clear as DCFill. */
    if (g_hc_depth_on) {
        hc_depth_map_clear();
    }
    /* M165: latticeplay entry Cls — show Mid/Edge before first 'c'. */
    hc_popup_paint_live();
    return 0;
}

uint64_t hc_builtin_fillrect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t c) {
    fb_fillrect((uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h, (uint32_t)c);
    return 0;
}

uint64_t hc_builtin_grhline(uint64_t x, uint64_t y, uint64_t w, uint64_t c) {
    fb_fillrect((uint32_t)x, (uint32_t)y, (uint32_t)w, 1, (uint32_t)c);
    return 0;
}

uint64_t hc_builtin_grvline(uint64_t x, uint64_t y, uint64_t h, uint64_t c) {
    fb_fillrect((uint32_t)x, (uint32_t)y, 1, (uint32_t)h, (uint32_t)c);
    return 0;
}

uint64_t hc_builtin_grcircle(uint64_t cx, uint64_t cy, uint64_t r, uint64_t c) {
    int32_t x0 = (int32_t)cx, y0 = (int32_t)cy, rad = (int32_t)r;
    if (rad < 0) {
        return 0;
    }
    uint32_t col = (uint32_t)c;
    int32_t x = rad, y = 0, err = 1 - rad;
    while (x >= y) {
        fb_putpixel((uint32_t)(x0 + x), (uint32_t)(y0 + y), col);
        fb_putpixel((uint32_t)(x0 + y), (uint32_t)(y0 + x), col);
        fb_putpixel((uint32_t)(x0 - y), (uint32_t)(y0 + x), col);
        fb_putpixel((uint32_t)(x0 - x), (uint32_t)(y0 + y), col);
        fb_putpixel((uint32_t)(x0 - x), (uint32_t)(y0 - y), col);
        fb_putpixel((uint32_t)(x0 - y), (uint32_t)(y0 - x), col);
        fb_putpixel((uint32_t)(x0 + y), (uint32_t)(y0 - x), col);
        fb_putpixel((uint32_t)(x0 + x), (uint32_t)(y0 - y), col);
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
    return 0;
}

uint64_t hc_builtin_grfillcircle(uint64_t cx, uint64_t cy, uint64_t r, uint64_t c) {
    int32_t x0 = (int32_t)cx, y0 = (int32_t)cy, rad = (int32_t)r;
    if (rad < 0) {
        return 0;
    }
    uint32_t col = (uint32_t)c;
    for (int32_t dy = -rad; dy <= rad; dy++) {
        int32_t dx = 0;
        while (dx * dx + dy * dy <= rad * rad) {
            dx++;
        }
        dx--;
        if (dx >= 0) {
            fb_fillrect((uint32_t)(x0 - dx), (uint32_t)(y0 + dy), (uint32_t)(2 * dx + 1), 1, col);
        }
    }
    return 0;
}

uint64_t hc_builtin_grline(uint64_t x0, uint64_t y0, uint64_t x1, uint64_t y1, uint64_t c) {
    return hc_builtin_grline6(x0, y0, x1, y1, c, 1);
}

/* ZealOS Lattice: GrLine(dc,x1,y1,x2,y2) — color from CDC. */
uint64_t hc_builtin_grline_dc(uint64_t dc, uint64_t x1, uint64_t y1, uint64_t x2, uint64_t y2) {
    return hc_builtin_grline(x1, y1, x2, y2, hc_dc_rgb(dc));
}

/* ZealOS GrLine(..., step): plot every `step` pixels along the line (start=0). */
uint64_t hc_builtin_grline6(uint64_t x0, uint64_t y0, uint64_t x1, uint64_t y1, uint64_t c,
                            uint64_t step) {
    int32_t xa = (int32_t)x0, ya = (int32_t)y0, xb = (int32_t)x1, yb = (int32_t)y1;
    int32_t dx = xa < xb ? xb - xa : xa - xb;
    int32_t sx = xa < xb ? 1 : -1;
    int32_t dy = ya < yb ? ya - yb : yb - ya; /* always ≤ 0 */
    int32_t sy = ya < yb ? 1 : -1;
    int32_t err = dx + dy;
    uint32_t col = (uint32_t)c;
    int64_t st = (int64_t)step;
    if (st < 1) {
        st = 1;
    }
    int64_t n = 0;
    for (;;) {
        if ((n % st) == 0) {
            fb_putpixel((uint32_t)xa, (uint32_t)ya, col);
        }
        if (xa == xb && ya == yb) {
            break;
        }
        int32_t e2 = err << 1;
        if (e2 >= dy) {
            err += dy;
            xa += sx;
        }
        if (e2 <= dx) {
            err += dx;
            ya += sy;
        }
        n++;
    }
    return 0;
}

uint64_t hc_builtin_putchar(uint64_t ch) {
    con_write((char)(ch & 0xff));
    return ch & 0xff;
}

uint64_t hc_builtin_print(uint64_t s) {
    const char *p = (const char *)(uintptr_t)s;
    if (!p) {
        return 0;
    }
    uint64_t n = 0;
    while (*p) {
        con_write(*p++);
        n++;
    }
    return n;
}

uint64_t hc_builtin_printat(uint64_t col, uint64_t row, uint64_t s) {
    const char *p = (const char *)(uintptr_t)s;
    if (!p || !g_fb) {
        return 0;
    }
    uint32_t c0 = (uint32_t)col, r0 = (uint32_t)row;
    uint64_t n = 0;
    while (*p) {
        char ch = *p++;
        if (ch == '\n') {
            c0 = (uint32_t)col;
            r0++;
            continue;
        }
        if (c0 < g_fb_cols && r0 < g_fb_rows) {
            fb_draw_char(c0, r0, ch, 0x00E0E0E0u);
        }
        c0++;
        n++;
    }
    return n;
}

/* Lattice: GrPrint(dc, x, y, fmt) — x/y pixels → 8px cells.
 * Wipe a fixed HUD band first so shorter Refresh lines do not leave digit soup. */
uint64_t hc_builtin_grprint(uint64_t dc, uint64_t x, uint64_t y, uint64_t fmt) {
    (void)dc;
    if (g_fb) {
        fb_fillrect((uint32_t)x, (uint32_t)y, 64u * 8u, 8u, 0);
    }
    return hc_builtin_printat(x / 8, y / 8, fmt);
}

uint64_t hc_builtin_printi64(uint64_t v) {
    int64_t x = (int64_t)v;
    char buf[24];
    int n = 0;
    int extra = 0;
    uint64_t u;
    if (x < 0) {
        con_write('-');
        extra = 1;
        u = (x == (int64_t)((uint64_t)1 << 63)) ? ((uint64_t)1 << 63) : (uint64_t)(-x);
    } else {
        u = (uint64_t)x;
    }
    if (u == 0) {
        con_write('0');
        return (uint64_t)(extra + 1);
    }
    while (u && n < 20) {
        buf[n++] = (char)('0' + (u % 10));
        u /= 10;
    }
    int digits = n;
    while (n--) {
        con_write(buf[n]);
    }
    return (uint64_t)(extra + digits);
}

uint64_t hc_builtin_strlen(uint64_t s) {
    const char *p = (const char *)(uintptr_t)s;
    uint64_t n = 0;
    if (!p) {
        return 0;
    }
    while (*p++) {
        n++;
    }
    return n;
}

uint64_t hc_builtin_strcmp(uint64_t a, uint64_t b) {
    const char *pa = (const char *)(uintptr_t)a;
    const char *pb = (const char *)(uintptr_t)b;
    if (!pa) {
        pa = "";
    }
    if (!pb) {
        pb = "";
    }
    while (*pa && *pa == *pb) {
        pa++;
        pb++;
    }
    return (uint64_t)(int64_t)((int)(unsigned char)*pa - (int)(unsigned char)*pb);
}

uint64_t hc_builtin_strcpy(uint64_t dst, uint64_t src) {
    char *d = (char *)(uintptr_t)dst;
    const char *s = (const char *)(uintptr_t)src;
    if (!d) {
        return 0;
    }
    if (!s) {
        s = "";
    }
    char *out = d;
    while ((*d++ = *s++)) {
    }
    return (uint64_t)(uintptr_t)out;
}

uint64_t hc_builtin_strcat(uint64_t dst, uint64_t src) {
    char *d = (char *)(uintptr_t)dst;
    const char *s = (const char *)(uintptr_t)src;
    if (!d) {
        return 0;
    }
    if (!s) {
        s = "";
    }
    char *out = d;
    while (*d) {
        d++;
    }
    while ((*d++ = *s++)) {
    }
    return (uint64_t)(uintptr_t)out;
}

uint64_t hc_builtin_str2i64(uint64_t s) {
    const char *p = (const char *)(uintptr_t)s;
    int64_t v = 0;
    int neg = 0;
    if (!p) {
        return 0;
    }
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p == '-') {
        neg = 1;
        p++;
    } else if (*p == '+') {
        p++;
    }
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
        while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) {
            char c = *p++;
            v <<= 4;
            if (c >= '0' && c <= '9') {
                v |= c - '0';
            } else if (c >= 'a' && c <= 'f') {
                v |= c - 'a' + 10;
            } else {
                v |= c - 'A' + 10;
            }
        }
    } else {
        while (*p >= '0' && *p <= '9') {
            v = v * 10 + (*p - '0');
            p++;
        }
    }
    return (uint64_t)(neg ? -v : v);
}

uint64_t hc_builtin_hashstr(uint64_t s) {
    const unsigned char *p = (const unsigned char *)(uintptr_t)s;
    uint64_t h = 5381;
    if (!p) {
        return h;
    }
    while (*p) {
        h = ((h << 5) + h) + *p++;
    }
    return h;
}

uint64_t hc_builtin_memcpy(uint64_t dst, uint64_t src, uint64_t n) {
    uint8_t *d = (uint8_t *)(uintptr_t)dst;
    const uint8_t *s = (const uint8_t *)(uintptr_t)src;
    if (!d || !s) {
        return dst;
    }
    for (uint64_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

uint64_t hc_builtin_memset(uint64_t dst, uint64_t val, uint64_t n) {
    uint8_t *d = (uint8_t *)(uintptr_t)dst;
    uint8_t v = (uint8_t)(val & 0xff);
    if (!d) {
        return dst;
    }
    for (uint64_t i = 0; i < n; i++) {
        d[i] = v;
    }
    return dst;
}

/* ZealOS-shaped bump heap (LIFO Free of the last chunk; Free(0) resets). */
#define HC_HEAP_BYTES (64u * 1024u)
static uint8_t g_hc_heap[HC_HEAP_BYTES] __attribute__((aligned(16)));
static uint64_t g_hc_heap_top;
static uint64_t g_hc_heap_last;

static void hc_heap_reset(void) {
    g_hc_heap_top = 0;
    g_hc_heap_last = 0;
    hc_msg_que_reset();
    g_hc_menu_nslots = 0;
    g_hc_cdc.color = 0;
    g_hc_cdc.depth_buf = 0;
    g_hc_depth_on = 0;
}

uint64_t hc_builtin_malloc(uint64_t n) {
    if (n == 0) {
        n = 1;
    }
    n = (n + 15u) & ~15u;
    if (g_hc_heap_top + n > HC_HEAP_BYTES) {
        return 0;
    }
    g_hc_heap_last = g_hc_heap_top;
    uint64_t p = (uint64_t)(uintptr_t)(g_hc_heap + g_hc_heap_top);
    g_hc_heap_top += n;
    return p;
}

uint64_t hc_builtin_free(uint64_t p) {
    if (p == 0) {
        hc_heap_reset();
        return 0;
    }
    if (p == (uint64_t)(uintptr_t)(g_hc_heap + g_hc_heap_last)) {
        g_hc_heap_top = g_hc_heap_last;
    }
    return 0;
}

uint64_t hc_builtin_strnew(uint64_t src) {
    const char *s = (const char *)(uintptr_t)src;
    uint64_t n = hc_builtin_strlen(src) + 1;
    uint64_t p = hc_builtin_malloc(n);
    if (!p) {
        return 0;
    }
    if (!s) {
        *(char *)(uintptr_t)p = 0;
        return p;
    }
    hc_builtin_memcpy(p, src, n);
    return p;
}

/* Append decimal / hex of v at *dp (advances). */
static void hc_fmt_i64(char **dp, int64_t v, int hex) {
    char tmp[24];
    int n = 0;
    uint64_t u;
    if (hex) {
        u = (uint64_t)v;
        if (u == 0) {
            tmp[n++] = '0';
        } else {
            while (u) {
                int d = (int)(u & 15u);
                tmp[n++] = (char)(d < 10 ? '0' + d : 'A' + (d - 10));
                u >>= 4;
            }
        }
    } else {
        if (v < 0) {
            *(*dp)++ = '-';
            u = (uint64_t)(-(v + 1)) + 1u;
        } else {
            u = (uint64_t)v;
        }
        if (u == 0) {
            tmp[n++] = '0';
        } else {
            while (u) {
                tmp[n++] = (char)('0' + (u % 10u));
                u /= 10u;
            }
        }
    }
    while (n > 0) {
        *(*dp)++ = tmp[--n];
    }
}

/* Format fmt with up to 5 args (F64-bits/%f or I64/%d) then GrPrint. */
uint64_t hc_builtin_grprintn(uint64_t dc, uint64_t x, uint64_t y, uint64_t fmt, uint64_t argv,
                             uint64_t nargs) {
    char buf[192];
    const char *f = (const char *)(uintptr_t)fmt;
    char *d = buf;
    char *end = buf + sizeof(buf) - 1;
    uint64_t *args = (uint64_t *)(uintptr_t)argv;
    int ai = 0;
    int narg = (int)nargs;
    if (!f) {
        f = "";
    }
    if (narg < 0) {
        narg = 0;
    }
    if (narg > 5) {
        narg = 5;
    }
    while (*f && d < end) {
        if (*f != '%') {
            *d++ = *f++;
            continue;
        }
        f++;
        if (*f == '%') {
            *d++ = '%';
            f++;
            continue;
        }
        if (ai >= narg) {
            break;
        }
        /* Optional width.prec before conversion (Lattice %5.1f). */
        int prec = 1;
        while (*f >= '0' && *f <= '9') {
            f++;
        }
        if (*f == '.') {
            f++;
            prec = 0;
            while (*f >= '0' && *f <= '9') {
                prec = prec * 10 + (*f - '0');
                f++;
            }
        }
        uint64_t av = args[ai++];
        if (*f == 'f' || *f == 'F') {
            f++;
            hc_fmt_f64_bits(&d, av, prec);
        } else if (*f == 'd') {
            f++;
            hc_fmt_i64(&d, (int64_t)av, 0);
        } else if (*f == 'X' || *f == 'x') {
            f++;
            hc_fmt_i64(&d, (int64_t)av, 1);
        } else if (*f == 'c') {
            f++;
            *d++ = (char)(av & 0xff);
        } else if (*f == 's') {
            const char *s = (const char *)(uintptr_t)av;
            f++;
            if (!s) {
                s = "";
            }
            while (*s && d < end) {
                *d++ = *s++;
            }
        } else {
            *d++ = '%';
            if (*f) {
                *d++ = *f++;
            }
        }
    }
    *d = 0;
    return hc_builtin_grprint(dc, x, y, (uint64_t)(uintptr_t)buf);
}

/* ZealOS-shaped CatPrint / StrPrint / MStrPrint — %d %s %c %X %% only; ≤2 args. */
uint64_t hc_builtin_catprint(uint64_t dst, uint64_t fmt, uint64_t a0, uint64_t a1) {
    char *d = (char *)(uintptr_t)dst;
    const char *f = (const char *)(uintptr_t)fmt;
    if (!d) {
        return 0;
    }
    if (!f) {
        f = "";
    }
    while (*d) {
        d++;
    }
    uint64_t args[2] = {a0, a1};
    int ai = 0;
    while (*f) {
        if (*f != '%') {
            *d++ = *f++;
            continue;
        }
        f++;
        if (*f == '%') {
            *d++ = '%';
            f++;
            continue;
        }
        if (ai >= 2) {
            break;
        }
        uint64_t av = args[ai++];
        if (*f == 'd') {
            hc_fmt_i64(&d, (int64_t)av, 0);
            f++;
        } else if (*f == 'X' || *f == 'x') {
            hc_fmt_i64(&d, (int64_t)av, 1);
            f++;
        } else if (*f == 'c') {
            *d++ = (char)(av & 0xff);
            f++;
        } else if (*f == 's') {
            const char *s = (const char *)(uintptr_t)av;
            if (!s) {
                s = "";
            }
            while (*s) {
                *d++ = *s++;
            }
            f++;
        } else {
            *d++ = '%';
            if (*f) {
                *d++ = *f++;
            }
        }
    }
    *d = 0;
    return dst;
}

uint64_t hc_builtin_strprint(uint64_t dst, uint64_t fmt, uint64_t a0, uint64_t a1) {
    char *d = (char *)(uintptr_t)dst;
    if (d) {
        *d = 0;
    }
    return hc_builtin_catprint(dst, fmt, a0, a1);
}

uint64_t hc_builtin_mstrprint(uint64_t fmt, uint64_t a0, uint64_t a1) {
    uint64_t p = hc_builtin_malloc(128);
    if (!p) {
        return 0;
    }
    *(char *)(uintptr_t)p = 0;
    hc_builtin_catprint(p, fmt, a0, a1);
    return p;
}

/* Indirect HolyC/JIT calls (PopUp / Spawn fp_start_addr). */
uint64_t hc_builtin_call0(uint64_t fp) {
    if (!fp) {
        return 0;
    }
    return ((uint64_t(*)(void))(uintptr_t)fp)();
}

uint64_t hc_builtin_call1(uint64_t fp, uint64_t a0) {
    if (!fp) {
        return 0;
    }
    return ((uint64_t(*)(uint64_t))(uintptr_t)fp)(a0);
}

/* 128 × 512-byte RAM disk (fallback; matches PCI/UTM RedSea 64KiB). When virtio-blk is ready, Blk* hit the image. */
#define HC_RAM_BLK_SIZE 512u
#define HC_RAM_N_BLKS   128u
static uint8_t g_ram_dsk[HC_RAM_N_BLKS * HC_RAM_BLK_SIZE] __attribute__((aligned(16)));

uint64_t hc_builtin_blkwrite(uint64_t buf, uint64_t blk, uint64_t count) {
    uint8_t *src = (uint8_t *)(uintptr_t)buf;
    if (!src || count == 0) {
        return 0;
    }
    if (virtio_blk_ready()) {
        if (blk + count > virtio_blk_capacity()) {
            return 0;
        }
        return (uint64_t)virtio_blk_write(src, blk, count);
    }
    if (blk + count > HC_RAM_N_BLKS) {
        return 0;
    }
    hc_builtin_memcpy((uint64_t)(uintptr_t)(g_ram_dsk + blk * HC_RAM_BLK_SIZE), buf,
                      count * HC_RAM_BLK_SIZE);
    return 1;
}

uint64_t hc_builtin_blkread(uint64_t buf, uint64_t blk, uint64_t count) {
    uint8_t *dst = (uint8_t *)(uintptr_t)buf;
    if (!dst || count == 0) {
        return 0;
    }
    if (virtio_blk_ready()) {
        if (blk + count > virtio_blk_capacity()) {
            return 0;
        }
        return (uint64_t)virtio_blk_read(dst, blk, count);
    }
    if (blk + count > HC_RAM_N_BLKS) {
        return 0;
    }
    hc_builtin_memcpy(buf, (uint64_t)(uintptr_t)(g_ram_dsk + blk * HC_RAM_BLK_SIZE),
                      count * HC_RAM_BLK_SIZE);
    return 1;
}

uint64_t hc_builtin_mousex(void) {
    uint32_t x = 0, y = 0;
    if (virtio_tablet_ready() && g_fb) {
        (void)virtio_tablet_xy((uint32_t)g_fb_w, (uint32_t)g_fb_h, &x, &y);
    }
    return x;
}

uint64_t hc_builtin_mousey(void) {
    uint32_t x = 0, y = 0;
    if (virtio_tablet_ready() && g_fb) {
        (void)virtio_tablet_xy((uint32_t)g_fb_w, (uint32_t)g_fb_h, &x, &y);
    }
    return y;
}

uint64_t hc_builtin_mousebtn(void) {
    if (virtio_tablet_ready()) {
        virtio_tablet_poll();
    }
    return (uint64_t)virtio_tablet_buttons();
}

/* Tablet abs deltas (call MouseDX then MouseDY for a paired sample). */
static int32_t g_mouse_last_x = -1, g_mouse_last_y = -1;
static int32_t g_mouse_dx, g_mouse_dy;
static int g_mouse_have_dy;

static void mouse_delta_refresh(void) {
    uint32_t x = 0, y = 0;
    if (virtio_tablet_ready() && g_fb) {
        (void)virtio_tablet_xy((uint32_t)g_fb_w, (uint32_t)g_fb_h, &x, &y);
    }
    if (g_mouse_last_x < 0) {
        g_mouse_dx = 0;
        g_mouse_dy = 0;
    } else {
        g_mouse_dx = (int32_t)x - g_mouse_last_x;
        g_mouse_dy = (int32_t)y - g_mouse_last_y;
    }
    g_mouse_last_x = (int32_t)x;
    g_mouse_last_y = (int32_t)y;
    g_mouse_have_dy = 1;
}

uint64_t hc_builtin_mousedx(void) {
    mouse_delta_refresh();
    return (uint64_t)(int64_t)g_mouse_dx;
}

uint64_t hc_builtin_mousedy(void) {
    if (!g_mouse_have_dy) {
        mouse_delta_refresh();
    }
    g_mouse_have_dy = 0;
    return (uint64_t)(int64_t)g_mouse_dy;
}

uint64_t hc_builtin_fbw(void) {
    return g_fb_w;
}

uint64_t hc_builtin_fbh(void) {
    return g_fb_h;
}

static uint64_t g_hc_rand = 0x9e3779b97f4a7c15ull;

uint64_t hc_builtin_rand(void) {
    /* xorshift64* */
    uint64_t x = g_hc_rand;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    g_hc_rand = x;
    return x * 0x2545F4914F6CDD1Dull;
}

/* Peek only — do not update MessageGet edge/xy state (else edges are lost). */
static int hc_input_pending_for_sleep(void) {
    if (g_hc_msg_head != g_hc_msg_tail) {
        return 1;
    }
    if (virtio_kbd_ready()) {
        if (virtio_kbd_msg_pending()) {
            return 1;
        }
    } else if (g_uart) {
        volatile uint32_t *r = (volatile uint32_t *)(uintptr_t)g_uart;
        /* PL011_FR_RXFE — defined later with uart_getc_nb; peek without consume. */
        if (!(r[PL011_FR / 4] & (1u << 4))) {
            return 1;
        }
    }
    if (virtio_tablet_ready() && g_fb) {
        uint32_t x = 0, y = 0;
        int btns = virtio_tablet_buttons();
        (void)virtio_tablet_xy((uint32_t)g_fb_w, (uint32_t)g_fb_h, &x, &y);
        int btn = btns & 1;
        int rbtn = (btns & 2) ? 1 : 0;
        if (btn != g_hc_msg_btn_prev || rbtn != g_hc_msg_rbtn_prev) {
            return 1;
        }
        if (g_hc_msg_have_xy && (x != g_hc_msg_last_x || y != g_hc_msg_last_y)) {
            return 1;
        }
    }
    return 0;
}

uint64_t hc_builtin_sleep(uint64_t ms) {
    uint64_t frq = 0, t0 = 0, now = 0, next_poll = 0;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frq));
    if (frq == 0) {
        frq = 62500000;
    }
    if (ms > 5000) {
        ms = 5000;
    }
    if (ms == 0) {
        return 0;
    }
    uint64_t ticks = (frq / 1000ull) * ms;
    uint64_t poll_every = frq / 1000ull; /* ~1 ms between input peeks */
    if (poll_every == 0) {
        poll_every = 1;
    }
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
    next_poll = t0;
    do {
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        if (now >= next_poll) {
            if (hc_input_pending_for_sleep()) {
                break;
            }
            next_poll = now + poll_every;
        }
    } while (now - t0 < ticks);
    return ms;
}

uint64_t hc_builtin_cnt(void) {
    uint64_t t = 0;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t));
    return t;
}

uint64_t hc_builtin_cntfrq(void) {
    uint64_t f = 0;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(f));
    if (f == 0) {
        f = 62500000;
    }
    return f;
}

static int hc_run_bc(const uint8_t *bc, size_t n, uint64_t *out) {
    g_hc_fp = 0;
    /* Frames start past file-scope globals. */
    g_hc_alloc = g_hc_glob_words;
    for (uint64_t i = 0; i < g_hc_glob_words && i < HC_IR_MEM_WORDS; i++) {
        g_hc_mem[i] = 0;
    }
    int bytes = hc_ir_compile(bc, n, g_jit_buf, sizeof(g_jit_buf) / 4);
    if (bytes < 0) {
        return bytes;
    }
    jit_icache_flush(g_jit_buf, (size_t)bytes);
    *out = ((uint64_t(*)(void))(void *)g_jit_buf)();
    return 0;
}

static int hc_jit_into(const uint8_t *bc, size_t n, uint32_t *out, size_t cap) {
    int bytes = hc_ir_compile(bc, n, out, cap);
    if (bytes < 0) {
        return bytes;
    }
    jit_icache_flush(out, (size_t)bytes);
    return bytes;
}

static int hc_run_src(const char *src, uint64_t *out);
static int hc_run_src_ex(const char *src, uint64_t *out, int popup_live);

/* Forward decl: RedSea load used by #include expansion (defined below). */
static int rs_load_file(const char *name, char *dst, size_t cap, size_t *out_len);
static size_t str_len(const char *s);

static char g_hc_src_exp[16384];
static char g_hc_zc_src[16384]; /* M163: DiskLat.ZC grew past 8K */
static char g_hc_src_mid[16384];

#define HC_MAX_MACROS 24
#define HC_MACRO_NAME 32
#define HC_MACRO_BODY 96

struct hc_macro {
    char name[HC_MACRO_NAME];
    char body[HC_MACRO_BODY];
    int nlen;
    int blen;
};

static struct hc_macro g_hc_macros[HC_MAX_MACROS];
static int g_hc_nmacros;

static int hc_mac_ident_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

static int hc_mac_ident_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/* If p is a #define line, record macro and return ptr past the line; else NULL. */
static const char *hc_parse_define(const char *p) {
    const char *q = p;
    while (*q == ' ' || *q == '\t') {
        q++;
    }
    if (*q != '#') {
        return NULL;
    }
    q++;
    while (*q == ' ' || *q == '\t') {
        q++;
    }
    if (!(q[0] == 'd' && q[1] == 'e' && q[2] == 'f' && q[3] == 'i' && q[4] == 'n' &&
          q[5] == 'e')) {
        return NULL;
    }
    q += 6;
    while (*q == ' ' || *q == '\t') {
        q++;
    }
    if (!hc_mac_ident_start(*q) || g_hc_nmacros >= HC_MAX_MACROS) {
        return NULL;
    }
    const char *ns = q;
    while (hc_mac_ident_char(*q)) {
        q++;
    }
    int nlen = (int)(q - ns);
    if (nlen <= 0 || nlen >= HC_MACRO_NAME) {
        return NULL;
    }
    while (*q == ' ' || *q == '\t') {
        q++;
    }
    const char *bs = q;
    while (*q && *q != '\n') {
        q++;
    }
    const char *be = q;
    while (be > bs && (be[-1] == ' ' || be[-1] == '\t' || be[-1] == '\r')) {
        be--;
    }
    int blen = (int)(be - bs);
    if (blen <= 0 || blen >= HC_MACRO_BODY) {
        return NULL;
    }
    struct hc_macro *m = &g_hc_macros[g_hc_nmacros++];
    for (int i = 0; i < nlen; i++) {
        m->name[i] = ns[i];
    }
    m->name[nlen] = 0;
    m->nlen = nlen;
    for (int i = 0; i < blen; i++) {
        m->body[i] = bs[i];
    }
    m->body[blen] = 0;
    m->blen = blen;
    if (*q == '\n') {
        q++;
    }
    return q;
}

static int hc_find_macro(const char *s, int len) {
    for (int i = 0; i < g_hc_nmacros; i++) {
        if (g_hc_macros[i].nlen == len) {
            int j;
            for (j = 0; j < len; j++) {
                if (g_hc_macros[i].name[j] != s[j]) {
                    break;
                }
            }
            if (j == len) {
                return i;
            }
        }
    }
    return -1;
}

/* Object-like #define expansion (Lattice TURTLE_SIZE / ANGLES). */
static int hc_expand_macros(const char *src, char *dst, size_t cap) {
    char tmp_a[16384];
    char tmp_b[16384];
    char *outbuf = tmp_a;
    char *inbuf = tmp_b;
    size_t slen = 0;
    while (src[slen]) {
        slen++;
        if (slen + 1 >= sizeof(tmp_a)) {
            return -1;
        }
    }
    for (size_t i = 0; i <= slen; i++) {
        inbuf[i] = src[i];
    }
    for (int pass = 0; pass < 8; pass++) {
        size_t n = 0;
        const char *p = inbuf;
        int changed = 0;
        int in_str = 0;
        while (*p) {
            if (*p == '"') {
                in_str = !in_str;
                if (n + 1 >= sizeof(tmp_a)) {
                    return -1;
                }
                outbuf[n++] = *p++;
                continue;
            }
            if (!in_str && hc_mac_ident_start(*p)) {
                const char *s = p;
                while (hc_mac_ident_char(*p)) {
                    p++;
                }
                int len = (int)(p - s);
                int mi = hc_find_macro(s, len);
                if (mi >= 0) {
                    struct hc_macro *m = &g_hc_macros[mi];
                    if (n + (size_t)m->blen + 1 >= sizeof(tmp_a)) {
                        return -1;
                    }
                    for (int i = 0; i < m->blen; i++) {
                        outbuf[n++] = m->body[i];
                    }
                    changed = 1;
                    continue;
                }
                if (n + (size_t)len + 1 >= sizeof(tmp_a)) {
                    return -1;
                }
                for (int i = 0; i < len; i++) {
                    outbuf[n++] = s[i];
                }
                continue;
            }
            if (n + 1 >= sizeof(tmp_a)) {
                return -1;
            }
            outbuf[n++] = *p++;
        }
        outbuf[n] = 0;
        if (!changed) {
            if (n + 1 > cap) {
                return -1;
            }
            for (size_t i = 0; i <= n; i++) {
                dst[i] = outbuf[i];
            }
            return 0;
        }
        /* swap for next pass */
        char *swap = inbuf;
        inbuf = outbuf;
        outbuf = swap;
    }
    return -1;
}

/* If p is an #include line, copy name and return ptr past the line; else NULL. */
static const char *hc_parse_include(const char *p, char *name, size_t nmax) {
    const char *q = p;
    char end;
    size_t i = 0;

    while (*q == ' ' || *q == '\t') {
        q++;
    }
    if (*q != '#') {
        return NULL;
    }
    q++;
    while (*q == ' ' || *q == '\t') {
        q++;
    }
    if (!(q[0] == 'i' && q[1] == 'n' && q[2] == 'c' && q[3] == 'l' && q[4] == 'u' &&
          q[5] == 'd' && q[6] == 'e')) {
        return NULL;
    }
    q += 7;
    while (*q == ' ' || *q == '\t') {
        q++;
    }
    if (*q == '"') {
        end = '"';
    } else if (*q == '<') {
        end = '>';
    } else {
        return NULL;
    }
    q++;
    while (*q && *q != end && *q != '\n' && i + 1 < nmax) {
        name[i++] = *q++;
    }
    if (*q != end || i == 0) {
        return NULL;
    }
    name[i] = 0;
    q++;
    while (*q && *q != '\n') {
        q++;
    }
    if (*q == '\n') {
        q++;
    }
    return q;
}

static int hc_expand_includes_r(const char *src, char *dst, size_t cap, size_t *out_n, int depth) {
    const char *p = src;
    char iname[40];
    char ibuf[4096];

    if (!src || !dst || !out_n || depth > 4) {
        return -1;
    }
    while (*p) {
        if (p == src || p[-1] == '\n') {
            const char *dafter = hc_parse_define(p);
            if (dafter) {
                p = dafter;
                continue;
            }
            const char *after = hc_parse_include(p, iname, sizeof(iname));
            if (after) {
                size_t n = 0;
                size_t sub = 0;
                if (rs_load_file(iname, ibuf, sizeof(ibuf), &n) != 0) {
                    return -2;
                }
                if (hc_expand_includes_r(ibuf, dst + *out_n, cap - *out_n, &sub, depth + 1) != 0) {
                    return -3;
                }
                *out_n += sub;
                if (*out_n + 1 >= cap) {
                    return -4;
                }
                p = after;
                continue;
            }
        }
        if (*out_n + 1 >= cap) {
            return -4;
        }
        dst[(*out_n)++] = *p++;
    }
    dst[*out_n] = 0;
    return 0;
}

static int hc_expand_includes(const char *src, char *dst, size_t cap) {
    size_t n = 0;
    if (!src || !dst || cap < 2) {
        return -1;
    }
    g_hc_nmacros = 0;
    if (hc_expand_includes_r(src, g_hc_src_mid, sizeof(g_hc_src_mid), &n, 0) != 0) {
        return -1;
    }
    if (hc_expand_macros(g_hc_src_mid, dst, cap) != 0) {
        return -1;
    }
    return 0;
}

static int hc_run_src(const char *src, uint64_t *out) {
    return hc_run_src_ex(src, out, 0);
}

static int hc_run_src_ex(const char *src, uint64_t *out, int popup_live) {
    uint8_t bc[8192]; /* was 4096 — Lattice angles[35] brace init */
    int rc;
    g_hc_popup_i = 0;
    g_hc_popup_mid = 14;
    g_hc_popup_edge = 0;
    g_hc_popup_live = popup_live ? 1 : 0;
    g_hc_fs_draw_it = 0; /* M153: no stale Fs->draw_it across demos */
    hc_heap_reset();
    if (hc_expand_includes(src, g_hc_src_exp, sizeof(g_hc_src_exp)) != 0) {
        g_hc_popup_live = 0;
        return -50;
    }
    int n = hc_front_compile_ex(g_hc_src_exp, bc, sizeof(bc), &g_fn_tbl, g_fn_code, hc_jit_into);
    if (n < 0) {
        g_hc_popup_live = 0;
        return n;
    }
    rc = hc_run_bc(bc, (size_t)n, out);
    g_hc_fs_draw_it = 0;
    g_hc_popup_live = 0;
    return rc;
}

/* M155/M159: DiskLat → LatticePlay — drop MsgQuePush scripts for live UTM.
 * smoke_esc: inject one CH_ESC after Cls so check-serial still terminates.
 * live (!smoke_esc): inject a one-line controls hint after Cls. */
static int hc_lattice_play_src(const char *in, char *out, size_t cap, int smoke_esc) {
    size_t o = 0;
    const char *p = in;
    int saw_cls = 0;
    if (!in || !out || cap < 8) {
        return -1;
    }
    while (*p) {
        if (p[0] == 'M' && p[1] == 's' && p[2] == 'g' && p[3] == 'Q' && p[4] == 'u' &&
            p[5] == 'e' && p[6] == 'P' && p[7] == 'u' && p[8] == 's' && p[9] == 'h') {
            p += 10;
            while (*p && *p != ';') {
                p++;
            }
            if (*p == ';') {
                p++;
            }
            while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
                p++;
            }
            continue;
        }
        if (o + 1 >= cap) {
            return -1;
        }
        out[o++] = *p++;
        if (!saw_cls && o >= 7 && out[o - 7] == 'C' && out[o - 6] == 'l' && out[o - 5] == 's' &&
            out[o - 4] == '(' && out[o - 3] == '0' && out[o - 2] == ')' && out[o - 1] == ';') {
            const char *inj;
            size_t el;
            if (smoke_esc) {
                inj = "\n\t\tMsgQuePush(MESSAGE_KEY_DOWN, CH_ESC, 0);";
            } else {
        inj = "\n\t\tGrPrint(dc, 0, 16, \"Esc=exit Enter=restart Space=step c=color +/-=w\");"
              "\n\t\tGrPrint(dc, 0, 24, \"L-click=place R-drag=aim e=ends\");"
              "\n\t\tGrPrint(dc, 0, 32, \"arrows=di/speed 0-9=layer\");";
            }
            el = 0;
            while (inj[el]) {
                el++;
            }
            if (o + el >= cap) {
                return -1;
            }
            for (size_t i = 0; i < el; i++) {
                out[o++] = inj[i];
            }
            saw_cls = 1;
        }
    }
    if (o >= cap) {
        return -1;
    }
    out[o] = 0;
    return 0;
}

/* I64 Abs(I64 x) { if (x < 0) return -x; return x; } */
static int hc_build_abs(uint8_t *bc, size_t cap, int64_t x) {
    size_t n = 0;
    if (cap < 48) {
        return -1;
    }
    n += hc_pack_imm64(bc + n, (uint64_t)x);
    n += hc_pack_u8op(bc + n, HC_ST_LOCAL, 0);
    n += hc_pack_u8op(bc + n, HC_LD_LOCAL, 0);
    n += hc_pack_imm64(bc + n, 0);
    bc[n++] = HC_LT;
    n += hc_pack_u8op(bc + n, HC_JNZ, 1); /* x < 0 → L1 */
    n += hc_pack_u8op(bc + n, HC_LD_LOCAL, 0);
    bc[n++] = HC_RET;
    n += hc_pack_u8op(bc + n, HC_LABEL, 1);
    n += hc_pack_u8op(bc + n, HC_LD_LOCAL, 0);
    bc[n++] = HC_NEG;
    bc[n++] = HC_RET;
    bc[n++] = HC_END;
    return (int)n;
}

/* ---- expression → IR (shell) ---- */
struct hc_expr {
    const char *p;
    uint8_t *bc;
    size_t n;
    size_t cap;
    int err;
};

static void expr_skip(struct hc_expr *e) {
    while (*e->p == ' ' || *e->p == '\t') {
        e->p++;
    }
}

static int expr_emit(struct hc_expr *e, uint8_t op) {
    if (e->n >= e->cap) {
        e->err = 1;
        return -1;
    }
    e->bc[e->n++] = op;
    return 0;
}

static int expr_imm(struct hc_expr *e, uint64_t v) {
    if (e->n + 9 > e->cap) {
        e->err = 1;
        return -1;
    }
    e->n += hc_pack_imm64(e->bc + e->n, v);
    return 0;
}

static int expr_parse_add(struct hc_expr *e);

static int expr_parse_prim(struct hc_expr *e) {
    expr_skip(e);
    if (*e->p == '(') {
        e->p++;
        if (expr_parse_add(e) < 0) {
            return -1;
        }
        expr_skip(e);
        if (*e->p != ')') {
            e->err = 1;
            return -1;
        }
        e->p++;
        return 0;
    }
    if (*e->p == '-') {
        e->p++;
        if (expr_parse_prim(e) < 0) {
            return -1;
        }
        return expr_emit(e, HC_NEG);
    }
    if (*e->p >= '0' && *e->p <= '9') {
        uint64_t v = 0;
        while (*e->p >= '0' && *e->p <= '9') {
            v = v * 10 + (uint64_t)(*e->p - '0');
            e->p++;
        }
        return expr_imm(e, v);
    }
    e->err = 1;
    return -1;
}

static int expr_parse_mul(struct hc_expr *e) {
    if (expr_parse_prim(e) < 0) {
        return -1;
    }
    for (;;) {
        expr_skip(e);
        char c = *e->p;
        if (c != '*') {
            break;
        }
        e->p++;
        if (expr_parse_prim(e) < 0) {
            return -1;
        }
        if (expr_emit(e, HC_MUL) < 0) {
            return -1;
        }
    }
    return 0;
}

static int expr_parse_add(struct hc_expr *e) {
    if (expr_parse_mul(e) < 0) {
        return -1;
    }
    for (;;) {
        expr_skip(e);
        char c = *e->p;
        if (c != '+' && c != '-') {
            break;
        }
        e->p++;
        if (expr_parse_mul(e) < 0) {
            return -1;
        }
        if (expr_emit(e, c == '+' ? HC_ADD : HC_SUB) < 0) {
            return -1;
        }
    }
    return 0;
}

static int expr_compile_run(const char *s, uint64_t *out) {
    uint8_t bc[128];
    struct hc_expr e = { .p = s, .bc = bc, .n = 0, .cap = sizeof(bc), .err = 0 };
    if (expr_parse_add(&e) < 0 || e.err) {
        return -1;
    }
    expr_skip(&e);
    if (*e.p != '\0') {
        return -1;
    }
    if (e.n + 2 > e.cap) {
        return -1;
    }
    bc[e.n++] = HC_RET;
    bc[e.n++] = HC_END;
    return hc_run_bc(bc, e.n, out);
}

static int streq(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static int str_startswith(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

static size_t str_len(const char *s) {
    size_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

static char str_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + 32);
    }
    return c;
}

/* Case-insensitive equality (RedSea names / runzc). */
static int str_ieq(const char *a, const char *b) {
    if (!a || !b) {
        return 0;
    }
    while (*a && *b) {
        if (str_lower(*a) != str_lower(*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return !*a && !*b;
}

static const char *skip_ws(const char *s) {
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    return s;
}

/* Read boot + root dir; returns 0 and fills buffers. */
static int rs_read_root(uint8_t *br, uint8_t *dir, uint64_t *out_root, uint64_t *out_sects) {
    const int64_t *w;
    uint64_t root, sects = 16;
    unsigned i;

    for (i = 0; i < 512; i++) {
        br[i] = 0;
        dir[i] = 0;
    }
    if (!hc_builtin_blkread((uint64_t)(uintptr_t)br, 0, 1)) {
        return -1;
    }
    if (br[3] != 0x88 || br[510] != 0x55 || br[511] != 0xAA) {
        return -2;
    }
    w = (const int64_t *)(br + 8);
    root = (uint64_t)w[2];
    if (w[1] > 0 && (uint64_t)w[1] <= 128) {
        sects = (uint64_t)w[1];
    }
    if (!root) {
        return -3;
    }
    if (!hc_builtin_blkread((uint64_t)(uintptr_t)dir, root, 1)) {
        return -4;
    }
    if (out_root) {
        *out_root = root;
    }
    if (out_sects) {
        *out_sects = sects;
    }
    return 0;
}

/* Case-insensitive find; returns slot or -1. */
static int rs_find_slot(uint8_t *dir, const char *name, uint64_t *out_blk, uint64_t *out_len) {
    unsigned i;
    const int64_t *w;

    for (i = 0; i < 8; i++) {
        uint8_t *e = dir + i * 64;
        if (!e[2]) {
            continue;
        }
        if (str_ieq((const char *)(e + 2), name)) {
            w = (const int64_t *)(e + 40);
            if (out_blk) {
                *out_blk = (uint64_t)w[0];
            }
            if (out_len) {
                *out_len = (uint64_t)w[1];
            }
            return (int)i;
        }
    }
    return -1;
}

/* Contiguous RedSea clusters: load root file into dst (case-insensitive). */
static int rs_load_file(const char *name, char *dst, size_t cap, size_t *out_len) {
    uint8_t br[512];
    uint8_t dir[512];
    uint8_t data[512];
    uint64_t root = 0;
    uint64_t fblk = 0;
    uint64_t flen = 0;
    uint64_t off;
    uint64_t blk;
    unsigned i;

    if (!name || !dst || cap < 2) {
        return -1;
    }
    if (rs_read_root(br, dir, &root, NULL) != 0) {
        return -2;
    }
    if (rs_find_slot(dir, name, &fblk, &flen) < 0) {
        return -6;
    }
    if (!fblk || flen == 0 || flen + 1 > cap) {
        return -6;
    }
    off = 0;
    blk = fblk;
    while (off < flen) {
        uint64_t chunk = flen - off;
        if (chunk > 512) {
            chunk = 512;
        }
        for (i = 0; i < 512; i++) {
            data[i] = 0;
        }
        if (!hc_builtin_blkread((uint64_t)(uintptr_t)data, blk, 1)) {
            return -7;
        }
        for (i = 0; i < (unsigned)chunk; i++) {
            dst[off + i] = (char)data[i];
        }
        off += chunk;
        blk++;
    }
    dst[flen] = 0;
    if (out_len) {
        *out_len = (size_t)flen;
    }
    return 0;
}

/* Host RSFmt — boot+bitmap; free data clus 2..sects-1 (M145). */
static int rs_fmt_host(uint64_t sects, int64_t uid) {
    uint8_t br[512];
    uint8_t map[512];
    uint8_t z[512];
    int64_t *w;
    unsigned i;

    if (sects < 4 || sects > 128) {
        return -1;
    }
    for (i = 0; i < 512; i++) {
        br[i] = 0;
        map[i] = 0;
        z[i] = 0;
    }
    br[3] = 0x88;
    w = (int64_t *)(br + 8);
    w[0] = 0;
    w[1] = (int64_t)sects;
    w[2] = 2; /* root clus */
    w[3] = 1; /* bitmap sects */
    w[4] = uid;
    br[510] = 0x55;
    br[511] = 0xAA;
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)br, 0, 1)) {
        return -2;
    }
    map[0] = 0x07; /* clus 0 boot, 1 bitmap, 2 root reserved */
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)map, 1, 1)) {
        return -3;
    }
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)z, 2, 1)) {
        return -4;
    }
    for (i = 3; i < (unsigned)sects; i++) {
        if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)z, i, 1)) {
            return -5;
        }
    }
    return 0;
}

/* Write/overwrite a contiguous multi-clus RedSea root file. */
static int rs_put_file(const char *name, const char *src, size_t len) {
    uint8_t br[512];
    uint8_t map[512];
    uint8_t dir[512];
    uint8_t data[512];
    uint64_t root = 0;
    uint64_t sects = 16;
    uint64_t nclus;
    uint64_t fblk = 0;
    int64_t slot = -1;
    int existing;
    unsigned i, j;
    int64_t *w;

    if (!name || !src || len == 0 || str_len(name) > 37) {
        return -1;
    }
    nclus = (len + 511) / 512;
    if (nclus < 1 || nclus > 120) {
        return -1;
    }
    if (rs_read_root(br, dir, &root, &sects) != 0) {
        return -2;
    }
    existing = rs_find_slot(dir, name, &fblk, NULL);
    for (i = 0; i < 8; i++) {
        if (!dir[i * 64 + 2] && slot < 0) {
            slot = (int64_t)i;
        }
    }
    if (existing < 0) {
        if (slot < 0) {
            return -6;
        }
        for (i = 0; i < 512; i++) {
            map[i] = 0;
        }
        if (!hc_builtin_blkread((uint64_t)(uintptr_t)map, 1, 1)) {
            return -7;
        }
        {
            int found = 0;
            fblk = 0;
            for (i = 0; i + (unsigned)nclus <= (unsigned)sects; i++) {
                int ok = 1;
                for (j = 0; j < (unsigned)nclus; j++) {
                    unsigned b = i + j;
                    unsigned m = 1u << (b & 7);
                    if (map[b >> 3] & m) {
                        ok = 0;
                        break;
                    }
                }
                if (ok) {
                    for (j = 0; j < (unsigned)nclus; j++) {
                        unsigned b = i + j;
                        unsigned m = 1u << (b & 7);
                        map[b >> 3] = (uint8_t)(map[b >> 3] | m);
                    }
                    fblk = i;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                return -8;
            }
        }
        if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)map, 1, 1)) {
            return -9;
        }
        {
            uint8_t *e = dir + (unsigned)slot * 64;
            size_t nlen = str_len(name);
            for (i = 0; i < 64; i++) {
                e[i] = 0;
            }
            e[0] = 0x20;
            e[1] = 0x04;
            for (i = 0; i <= nlen; i++) {
                e[2 + i] = (uint8_t)name[i];
            }
            w = (int64_t *)(e + 40);
            w[0] = (int64_t)fblk;
            w[1] = (int64_t)len;
            w[2] = 0;
        }
        if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)dir, root, 1)) {
            return -10;
        }
    } else {
        uint8_t *e = dir + (unsigned)existing * 64;
        w = (int64_t *)(e + 40);
        w[1] = (int64_t)len;
        if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)dir, root, 1)) {
            return -11;
        }
    }

    for (j = 0; j < (unsigned)nclus; j++) {
        size_t off = (size_t)j * 512;
        size_t chunk = len - off;
        if (chunk > 512) {
            chunk = 512;
        }
        for (i = 0; i < 512; i++) {
            data[i] = 0;
        }
        for (i = 0; i < (unsigned)chunk; i++) {
            data[i] = (uint8_t)src[off + i];
        }
        if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)data, fblk + j, 1)) {
            return -12;
        }
    }
    return 0;
}

static int rs_file_exists(const char *name, uint64_t *out_len) {
    uint8_t br[512];
    uint8_t dir[512];
    uint64_t root = 0;
    uint64_t flen = 0;

    if (!name || rs_read_root(br, dir, &root, NULL) != 0) {
        return 0;
    }
    if (rs_find_slot(dir, name, NULL, &flen) < 0) {
        return 0;
    }
    if (out_len) {
        *out_len = flen;
    }
    return 1;
}

static int rs_put_file_if_absent(const char *name, const char *src, size_t len) {
    uint64_t elen = 0;
    if (rs_file_exists(name, &elen) && elen > 0) {
        return 0;
    }
    return rs_put_file(name, src, len);
}

/* Free contiguous clusters then clear dir slot (M37). */
static int rs_del_file(const char *name) {
    uint8_t br[512];
    uint8_t map[512];
    uint8_t dir[512];
    uint64_t root = 0;
    uint64_t fblk = 0;
    uint64_t flen = 0;
    uint64_t nclus;
    int slot;
    unsigned i, j;

    if (!name || str_ieq(name, ".") || str_ieq(name, "..")) {
        return -1;
    }
    if (rs_read_root(br, dir, &root, NULL) != 0) {
        return -2;
    }
    slot = rs_find_slot(dir, name, &fblk, &flen);
    if (slot < 0 || !fblk) {
        return -3;
    }
    nclus = (flen + 511) / 512;
    if (nclus < 1) {
        nclus = 1;
    }
    for (i = 0; i < 512; i++) {
        map[i] = 0;
    }
    if (!hc_builtin_blkread((uint64_t)(uintptr_t)map, 1, 1)) {
        return -4;
    }
    for (j = 0; j < (unsigned)nclus; j++) {
        unsigned b = (unsigned)(fblk + j);
        unsigned m = 1u << (b & 7);
        map[b >> 3] = (uint8_t)(map[b >> 3] & (uint8_t)~m);
    }
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)map, 1, 1)) {
        return -5;
    }
    {
        uint8_t *e = dir + (unsigned)slot * 64;
        for (i = 0; i < 64; i++) {
            e[i] = 0;
        }
    }
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)dir, root, 1)) {
        return -6;
    }
    return 0;
}

/* Rename root entry (case-insensitive find; new name stored as given). */
static int rs_rename_file(const char *oldn, const char *newn) {
    uint8_t br[512];
    uint8_t dir[512];
    uint64_t root = 0;
    int slot;
    size_t nlen;
    unsigned i;

    if (!oldn || !newn || str_ieq(oldn, ".") || str_ieq(oldn, "..")) {
        return -1;
    }
    nlen = str_len(newn);
    if (nlen == 0 || nlen > 37) {
        return -1;
    }
    if (rs_read_root(br, dir, &root, NULL) != 0) {
        return -2;
    }
    slot = rs_find_slot(dir, oldn, NULL, NULL);
    if (slot < 0) {
        return -3;
    }
    if (!str_ieq(oldn, newn) && rs_find_slot(dir, newn, NULL, NULL) >= 0) {
        return -4; /* target exists */
    }
    {
        uint8_t *e = dir + (unsigned)slot * 64;
        for (i = 0; i < 38; i++) {
            e[2 + i] = 0;
        }
        for (i = 0; i <= nlen; i++) {
            e[2 + i] = (uint8_t)newn[i];
        }
    }
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)dir, root, 1)) {
        return -5;
    }
    return 0;
}

static int rs_dir_print(void) {
    uint8_t br[512];
    uint8_t dir[512];
    uint64_t root = 0;
    unsigned i, n = 0;
    const int64_t *w;

    if (rs_read_root(br, dir, &root, NULL) != 0) {
        return -1;
    }
    con_puts("rsdir:\n");
    for (i = 0; i < 8; i++) {
        uint8_t *e = dir + i * 64;
        if (!e[2]) {
            continue;
        }
        w = (const int64_t *)(e + 40);
        con_puts("  ");
        con_puts((const char *)(e + 2));
        con_puts("  size=");
        uart_put_u64_hex(g_uart, (uint64_t)w[1]);
        con_puts("  blk=");
        uart_put_u64_hex(g_uart, (uint64_t)w[0]);
        con_puts("\n");
        n++;
    }
    con_puts("rsdir n=");
    uart_put_u32_dec(g_uart, n);
    con_puts("\n");
    return (int)n;
}

/* Disk catalog: seed without per-file ifs in the runzc path (M37+). */
struct rs_catalog_ent {
    const char *name;
    const char *src;
    int graphic;
};

/* M38 multi-file demo (also on disk via catalog). */
static const char ADDLIB_ZC[] = "I64 Add(I64 a, I64 b)\n{\n\treturn a + b;\n}\n";
static const char USEADD_ZC[] = "#include \"AddLib.ZC\"\n\nreturn Add(20, 22);\n";

/* Freeze catalog: #include demos + Notes (fits 8 root slots with . Keep Hi). */
static const struct rs_catalog_ent g_rs_catalog[] = {
    { "AddLib.ZC", ADDLIB_ZC, 0 },
    { "UseAdd.ZC", USEADD_ZC, 0 },
    { "DocLib.ZC", DOCLIB_ZC, 0 },
    { "Notes.ZC", NOTES_ZC, 1 },
    { "MemSort.ZC", MEMSORT_ZC, 0 },
};

static const struct rs_catalog_ent *rs_catalog_lookup(const char *name) {
    unsigned i;
    for (i = 0; i < sizeof(g_rs_catalog) / sizeof(g_rs_catalog[0]); i++) {
        if (str_ieq(name, g_rs_catalog[i].name)) {
            return &g_rs_catalog[i];
        }
    }
    return NULL;
}

static int rs_name_kept(const char *name) {
    if (!name || !*name) {
        return 1;
    }
    if (str_ieq(name, ".") || str_ieq(name, "..") || str_ieq(name, "Keep") ||
        str_ieq(name, "Hi.ZC")) {
        return 1;
    }
    return rs_catalog_lookup(name) != NULL;
}

/* Delete one root file that is not part of the freeze catalog (old demos). */
static int rs_evict_one_stale(void) {
    uint8_t br[512];
    uint8_t dir[512];
    uint64_t root = 0;
    unsigned i;

    if (rs_read_root(br, dir, &root, NULL) != 0) {
        return -1;
    }
    for (i = 0; i < 8; i++) {
        uint8_t *e = dir + i * 64;
        if (!e[2]) {
            continue;
        }
        if (!rs_name_kept((const char *)(e + 2))) {
            return rs_del_file((const char *)(e + 2));
        }
    }
    return -1;
}

/* Best-effort seed: evict stale (non-catalog) files if the root is full.
 * RAM smoke and PCI RedSea are different volumes — PCI may still hold
 * NetOfDots/Lines from older milestones. */
static int rs_seed_catalog(void) {
    uint8_t br[512];
    uint8_t dir[512];
    unsigned i;

    if (rs_read_root(br, dir, NULL, NULL) != 0) {
        return -1;
    }
    for (i = 0; i < sizeof(g_rs_catalog) / sizeof(g_rs_catalog[0]); i++) {
        const struct rs_catalog_ent *e = &g_rs_catalog[i];
        uint64_t elen = 0;
        int tries;

        if (rs_file_exists(e->name, &elen) && elen > 0) {
            continue;
        }
        for (tries = 0; tries < 4; tries++) {
            if (rs_put_file(e->name, e->src, str_len(e->src)) == 0) {
                break;
            }
            if (rs_evict_one_stale() != 0) {
                break;
            }
        }
    }
    return 0;
}

/* Ensure one catalog file is on disk (for runzc / #include deps). */
static int rs_ensure_catalog_file(const char *name) {
    const struct rs_catalog_ent *e = rs_catalog_lookup(name);
    uint64_t elen = 0;
    int tries;

    if (!e) {
        return 0;
    }
    if (rs_file_exists(e->name, &elen) && elen > 0) {
        return 0;
    }
    for (tries = 0; tries < 4; tries++) {
        if (rs_put_file(e->name, e->src, str_len(e->src)) == 0) {
            return 0;
        }
        if (rs_evict_one_stale() != 0) {
            return -1;
        }
    }
    return -1;
}

static int rs_is_graphic_zc(const char *name) {
    const struct rs_catalog_ent *e = rs_catalog_lookup(name);
    return e && e->graphic;
}

#define PL011_FR_RXFE (1u << 4)
#define PL011_RSR     0x04

static int uart_getc_nb(void) {
    volatile uint32_t *r;
    int c;
    if (!g_uart) {
        return -1;
    }
    r = (volatile uint32_t *)(uintptr_t)g_uart;
    if (r[PL011_FR / 4] & PL011_FR_RXFE) {
        return -1;
    }
    c = (int)(r[PL011_DR / 4] & 0xff);
    r[PL011_RSR / 4] = 0; /* clear OE/BE/PE/FE via ECR alias */
    return c;
}

static int kbd_getc_nb(void) {
    int c = virtio_kbd_getc_nb();
    if (c >= 0) {
        return c;
    }
    return uart_getc_nb();
}

static int g_key_pending = -1;

uint64_t hc_builtin_keyhit(void) {
    if (g_key_pending >= 0) {
        return 1;
    }
    int c = kbd_getc_nb();
    if (c >= 0) {
        g_key_pending = c;
        return 1;
    }
    return 0;
}

uint64_t hc_builtin_getkey(void) {
    if (g_key_pending >= 0) {
        int c = g_key_pending;
        g_key_pending = -1;
        return (uint64_t)(int64_t)c;
    }
    int c = kbd_getc_nb();
    return (uint64_t)(int64_t)c; /* -1 if none */
}

static uint32_t g_tab_cx = ~0u, g_tab_cy = ~0u;
static int g_tab_prev_btn;
static uint32_t g_tab_clicks;

static void tablet_paint_stamp(uint32_t x, uint32_t y, uint32_t bgr) {
    for (int dy = -3; dy <= 3; dy++) {
        for (int dx = -3; dx <= 3; dx++) {
            if (dx * dx + dy * dy > 10) {
                continue;
            }
            int32_t px = (int32_t)x + dx;
            int32_t py = (int32_t)y + dy;
            if (px >= 0 && py >= 0) {
                fb_putpixel((uint32_t)px, (uint32_t)py, bgr);
            }
        }
    }
}

static void tablet_cursor_tick(void) {
    if (!virtio_tablet_ready() || !g_fb) {
        return;
    }
    uint32_t x, y;
    (void)virtio_tablet_xy((uint32_t)g_fb_w, (uint32_t)g_fb_h, &x, &y);
    int btn = virtio_tablet_buttons();
    if ((btn & 1) && !(g_tab_prev_btn & 1)) {
        tablet_paint_stamp(x, y, 0x0040C0F0u);
        g_tab_clicks++;
    } else if (btn & 1) {
        /* Drag-paint while held. */
        tablet_paint_stamp(x, y, 0x003090C0u);
    }
    if (g_tab_cx != ~0u && !(btn & 1)) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int32_t px = (int32_t)g_tab_cx + dx;
                int32_t py = (int32_t)g_tab_cy + dy;
                if (px >= 0 && py >= 0) {
                    fb_putpixel((uint32_t)px, (uint32_t)py, 0x00101010u);
                }
            }
        }
    }
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int32_t px = (int32_t)x + dx;
            int32_t py = (int32_t)y + dy;
            if (px >= 0 && py >= 0) {
                fb_putpixel((uint32_t)px, (uint32_t)py,
                            (btn & 1) ? 0x00F0F0F0u : 0x00E08020u);
            }
        }
    }
    g_tab_prev_btn = btn;
    g_tab_cx = x;
    g_tab_cy = y;
}

/* M157: after Lattice/graphics demos, match shell_run dark FB (not DCFill black). */
static void shell_fb_ready(void) {
    if (g_fb) {
        fb_clear(0x00101820u);
        g_fb_cx = 0;
        g_fb_cy = 0;
    }
}

static void shell_handle(const char *line, int *done) {
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    if (*line == '\0') {
        return;
    }
    if (streq(line, "help")) {
        con_puts("UTM freeze: vblk | rspersist | rscatalog | rsdir | runzc | runzc Notes.ZC\n");
        con_puts("Lattice: nearlatticelite | disklat | lattice | latticeplay | depthplotlite\n");
        /* M178: eyes-on controls without reading MenuPush / DiskLat source. */
        con_puts("  latticeplay: Esc Enter Space c +/- e | L-click place | R-drag aim | arrows di/speed | 0-9 layer\n");
        con_puts("cmds: help|abs|sum|bars|stars|circles|bounce|paint|netofdots|lines|minigr|memsort|globshare|life|cartlite|vec2lite|angleslite|coslite|sqrtlite|arglite|commalite|plot3lite|tospilite|colorlite|turtlelite|filllite|initlite|deflite|printlite|msglite|menulite|findlite|fslite|setuplite|ttlite|buflite|inclite|dclite|linedclite|grflite|movelite|checkedlite|cmplite|forinclite|microlite|movestacklite|endlite|drawitlite|latticelite|looplite|demolite|eventlite|playlite|inputlite|rightlite|cursorlite|uplite|ticklite|framelite|plotdclite|abortlite|aimmovelite|idlelite|layerlite|endslite|speedlite|midlite|livelite|accellite|restartlite|widthlite|bothcolorlite|menufulllite|menubiglite|trylite|stepcountlite|anglesfulllite|braceangleslite|bracepilite|setmenulite|nearlatticelite|f64iflite|wraplatticelite|menulooplite|idxalllite|disklat|lattice|depthbuflite|depthrstlite|depthplotlite|depthlinelite|peekplot|offbmp|heapstr|catfmt|heapque|jobque|jobrun|spawn|popup|doclite|ramblk|namefile|dirlook|dirdel|fopen|fwrite|multiblk|redsea|rsroot|rsfile|rsalloc|rsfree|rsmulti|rscfile|rscwrite|rscseek|rsclib|rspersist|rscatalog|rsdir|rsdel|rsrename|runzc|runzc <file.ZC>|vblk|halt|hc <src>|expr\n");
        con_puts("  hc: Print*/Str*/Mem*/Min/Max/Clamp/Sign/Sqr/Abs/Cnt/CntFrq/HashStr/Mouse*/Rand/Sleep/Gr*/Cls\n");
        con_puts("  hc: KeyHit/GetKey (Esc exits paint loops)\n");
        con_puts("  netofdots/lines/minigr/memsort/globshare/life/cartlite/vec2lite/angleslite/coslite/sqrtlite/arglite/commalite/peekplot/offbmp/heapstr/catfmt/heapque/jobque/jobrun/spawn/popup/doclite/ramblk/namefile/dirlook/dirdel/fopen/fwrite/multiblk/redsea/rsroot/rsfile/rsalloc/rsfree/rsmulti/rscfile/rscwrite/rscseek/rsclib/rspersist/runzc/vblk: upstream ZealOS demos\n");
        con_puts("  tablet: click/drag; HolyC paint via Mouse*; latticeplay place/aim\n");
        return;
    }
    if (streq(line, "halt") || streq(line, "quit")) {
        *done = 1;
        return;
    }
    if (streq(line, "netofdots")) {
        uint64_t got = 0;
        if (hc_run_src(NETOFDOTS_ZC, &got) != 0) {
            con_puts("netofdots FAIL\n");
            return;
        }
        con_puts("netofdots ok (inspect FB; sleeping)\n");
        (void)hc_builtin_sleep(3000);
        return;
    }
    if (streq(line, "lines")) {
        uint64_t got = 0;
        if (hc_run_src(LINES_ZC, &got) != 0) {
            con_puts("lines FAIL\n");
            return;
        }
        con_puts("lines ok\n");
        return;
    }
    if (streq(line, "minigr")) {
        uint64_t got = 0;
        if (hc_run_src(MINIGR_ZC, &got) != 0) {
            con_puts("minigr FAIL\n");
            return;
        }
        con_puts("minigr ok\n");
        return;
    }
    if (streq(line, "memsort")) {
        uint64_t got = 0;
        if (hc_run_src(MEMSORT_ZC, &got) != 0 || got != 16) {
            con_puts("memsort FAIL\n");
            return;
        }
        con_puts("memsort ok\n");
        return;
    }
    if (streq(line, "globshare")) {
        uint64_t got = 0;
        if (hc_run_src(GLOBSHARE_ZC, &got) != 0 || got != 8) {
            con_puts("globshare FAIL\n");
            return;
        }
        con_puts("globshare ok\n");
        return;
    }
    if (streq(line, "life")) {
        uint64_t got = 0;
        if (hc_run_src(LIFE_ZC, &got) != 0 || got != 4) {
            con_puts("life FAIL\n");
            return;
        }
        con_puts("life ok\n");
        return;
    }
    if (streq(line, "cartlite")) {
        uint64_t got = 0;
        if (hc_run_src(CARTLITE_ZC, &got) != 0 || got != 14) {
            con_puts("cartlite FAIL\n");
            return;
        }
        con_puts("cartlite ok\n");
        return;
    }
    if (streq(line, "vec2lite")) {
        uint64_t got = 0;
        if (hc_run_src(VEC2LITE_ZC, &got) != 0 || got != 25) {
            con_puts("vec2lite FAIL\n");
            return;
        }
        con_puts("vec2lite ok\n");
        return;
    }
    if (streq(line, "angleslite")) {
        uint64_t got = 0;
        if (hc_run_src(ANGLESLITE_ZC, &got) != 0 || got != 12) {
            con_puts("angleslite FAIL\n");
            return;
        }
        con_puts("angleslite ok\n");
        return;
    }
    if (streq(line, "coslite")) {
        uint64_t got = 0;
        if (hc_run_src(COSLITE_ZC, &got) != 0 || got != 63) {
            con_puts("coslite FAIL\n");
            return;
        }
        con_puts("coslite ok\n");
        return;
    }
    if (streq(line, "sqrtlite")) {
        uint64_t got = 0;
        if (hc_run_src(SQRTLITE_ZC, &got) != 0 || got != 31) {
            con_puts("sqrtlite FAIL\n");
            return;
        }
        con_puts("sqrtlite ok\n");
        return;
    }
    if (streq(line, "arglite")) {
        uint64_t got = 0;
        if (hc_run_src(ARGLITE_ZC, &got) != 0 || got != 63) {
            con_puts("arglite FAIL\n");
            return;
        }
        con_puts("arglite ok\n");
        return;
    }
    if (streq(line, "commalite")) {
        uint64_t got = 0;
        if (hc_run_src(COMMALITE_ZC, &got) != 0 || got != 63) {
            con_puts("commalite FAIL\n");
            return;
        }
        con_puts("commalite ok\n");
        return;
    }
    if (streq(line, "plot3lite")) {
        uint64_t got = 0;
        if (hc_run_src(PLOT3LITE_ZC, &got) != 0 || got != 15) {
            con_puts("plot3lite FAIL\n");
            return;
        }
        con_puts("plot3lite ok\n");
        return;
    }
    if (streq(line, "tospilite")) {
        uint64_t got = 0;
        if (hc_run_src(TOSPILITE_ZC, &got) != 0 || got != 15) {
            con_puts("tospilite FAIL\n");
            return;
        }
        con_puts("tospilite ok\n");
        return;
    }
    if (streq(line, "colorlite")) {
        uint64_t got = 0;
        if (hc_run_src(COLORLITE_ZC, &got) != 0 || got != 63) {
            con_puts("colorlite FAIL\n");
            return;
        }
        con_puts("colorlite ok\n");
        return;
    }
    if (streq(line, "turtlelite")) {
        uint64_t got = 0;
        if (hc_run_src(TURTLELITE_ZC, &got) != 0 || got != 15) {
            con_puts("turtlelite FAIL\n");
            return;
        }
        con_puts("turtlelite ok\n");
        return;
    }
    if (streq(line, "filllite")) {
        uint64_t got = 0;
        if (hc_run_src(FILLLITE_ZC, &got) != 0 || got != 15) {
            con_puts("filllite FAIL\n");
            return;
        }
        con_puts("filllite ok\n");
        return;
    }
    if (streq(line, "initlite")) {
        uint64_t got = 0;
        if (hc_run_src(INITLITE_ZC, &got) != 0 || got != 15) {
            con_puts("initlite FAIL\n");
            return;
        }
        con_puts("initlite ok\n");
        return;
    }
    if (streq(line, "deflite")) {
        uint64_t got = 0;
        if (hc_run_src(DEFLITE_ZC, &got) != 0 || got != 15) {
            con_puts("deflite FAIL\n");
            return;
        }
        con_puts("deflite ok\n");
        return;
    }
    if (streq(line, "printlite")) {
        uint64_t got = 0;
        if (hc_run_src(PRINTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("printlite FAIL\n");
            return;
        }
        con_puts("printlite ok\n");
        return;
    }
    if (streq(line, "msglite")) {
        uint64_t got = 0;
        if (hc_run_src(MSGLITE_ZC, &got) != 0 || got != 15) {
            con_puts("msglite FAIL\n");
            return;
        }
        con_puts("msglite ok\n");
        return;
    }
    if (streq(line, "menulite")) {
        uint64_t got = 0;
        if (hc_run_src(MENULITE_ZC, &got) != 0 || got != 15) {
            con_puts("menulite FAIL\n");
            return;
        }
        con_puts("menulite ok\n");
        return;
    }
    if (streq(line, "findlite")) {
        uint64_t got = 0;
        if (hc_run_src(FINDLITE_ZC, &got) != 0 || got != 15) {
            con_puts("findlite FAIL\n");
            return;
        }
        con_puts("findlite ok\n");
        return;
    }
    if (streq(line, "fslite")) {
        uint64_t got = 0;
        if (hc_run_src(FSLITE_ZC, &got) != 0 || got != 15) {
            con_puts("fslite FAIL\n");
            return;
        }
        con_puts("fslite ok\n");
        return;
    }
    if (streq(line, "setuplite")) {
        uint64_t got = 0;
        if (hc_run_src(SETUPLITE_ZC, &got) != 0 || got != 15) {
            con_puts("setuplite FAIL\n");
            return;
        }
        con_puts("setuplite ok\n");
        return;
    }
    if (streq(line, "ttlite")) {
        uint64_t got = 0;
        if (hc_run_src(TTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("ttlite FAIL\n");
            return;
        }
        con_puts("ttlite ok\n");
        return;
    }
    if (streq(line, "buflite")) {
        uint64_t got = 0;
        if (hc_run_src(BUFLITE_ZC, &got) != 0 || got != 15) {
            con_puts("buflite FAIL\n");
            return;
        }
        con_puts("buflite ok\n");
        return;
    }
    if (streq(line, "inclite")) {
        uint64_t got = 0;
        if (hc_run_src(INCLITE_ZC, &got) != 0 || got != 15) {
            con_puts("inclite FAIL\n");
            return;
        }
        con_puts("inclite ok\n");
        return;
    }
    if (streq(line, "dclite")) {
        uint64_t got = 0;
        if (hc_run_src(DCLITE_ZC, &got) != 0 || got != 15) {
            con_puts("dclite FAIL\n");
            return;
        }
        con_puts("dclite ok\n");
        return;
    }
    if (streq(line, "linedclite")) {
        uint64_t got = 0;
        if (hc_run_src(LINEDCLITE_ZC, &got) != 0 || got != 15) {
            con_puts("linedclite FAIL\n");
            return;
        }
        con_puts("linedclite ok\n");
        return;
    }
    if (streq(line, "grflite")) {
        uint64_t got = 0;
        if (hc_run_src(GRFLITE_ZC, &got) != 0 || got != 15) {
            con_puts("grflite FAIL\n");
            return;
        }
        con_puts("grflite ok\n");
        return;
    }
    if (streq(line, "movelite")) {
        uint64_t got = 0;
        if (hc_run_src(MOVELITE_ZC, &got) != 0 || got != 15) {
            con_puts("movelite FAIL\n");
            return;
        }
        con_puts("movelite ok\n");
        return;
    }
    if (streq(line, "checkedlite")) {
        uint64_t got = 0;
        if (hc_run_src(CHECKEDLITE_ZC, &got) != 0 || got != 15) {
            con_puts("checkedlite FAIL\n");
            return;
        }
        con_puts("checkedlite ok\n");
        return;
    }
    if (streq(line, "cmplite")) {
        uint64_t got = 0;
        if (hc_run_src(CMPLITE_ZC, &got) != 0 || got != 15) {
            con_puts("cmplite FAIL\n");
            return;
        }
        con_puts("cmplite ok\n");
        return;
    }
    if (streq(line, "forinclite")) {
        uint64_t got = 0;
        if (hc_run_src(FORINCLITE_ZC, &got) != 0 || got != 15) {
            con_puts("forinclite FAIL\n");
            return;
        }
        con_puts("forinclite ok\n");
        return;
    }
    if (streq(line, "microlite")) {
        uint64_t got = 0;
        if (hc_run_src(MICROLITE_ZC, &got) != 0 || got != 15) {
            con_puts("microlite FAIL\n");
            return;
        }
        con_puts("microlite ok\n");
        return;
    }
    if (streq(line, "movestacklite")) {
        uint64_t got = 0;
        if (hc_run_src(MOVESTACKLITE_ZC, &got) != 0 || got != 15) {
            con_puts("movestacklite FAIL\n");
            return;
        }
        con_puts("movestacklite ok\n");
        return;
    }
    if (streq(line, "endlite")) {
        uint64_t got = 0;
        if (hc_run_src(ENDLITE_ZC, &got) != 0 || got != 15) {
            con_puts("endlite FAIL\n");
            return;
        }
        con_puts("endlite ok\n");
        return;
    }
    if (streq(line, "drawitlite")) {
        uint64_t got = 0;
        if (hc_run_src(DRAWITLITE_ZC, &got) != 0 || got != 15) {
            con_puts("drawitlite FAIL\n");
            return;
        }
        con_puts("drawitlite ok\n");
        return;
    }
    if (streq(line, "latticelite")) {
        uint64_t got = 0;
        if (hc_run_src(LATTICELITE_ZC, &got) != 0 || got != 15) {
            con_puts("latticelite FAIL\n");
            return;
        }
        con_puts("latticelite ok\n");
        return;
    }
    if (streq(line, "looplite")) {
        uint64_t got = 0;
        if (hc_run_src(LOOPLITE_ZC, &got) != 0 || got != 15) {
            con_puts("looplite FAIL\n");
            return;
        }
        con_puts("looplite ok\n");
        return;
    }
    if (streq(line, "demolite")) {
        uint64_t got = 0;
        if (hc_run_src(DEMOLITE_ZC, &got) != 0 || got != 15) {
            con_puts("demolite FAIL\n");
            return;
        }
        con_puts("demolite ok\n");
        return;
    }
    if (streq(line, "eventlite")) {
        uint64_t got = 0;
        if (hc_run_src(EVENTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("eventlite FAIL\n");
            return;
        }
        con_puts("eventlite ok\n");
        return;
    }
    if (streq(line, "playlite")) {
        uint64_t got = 0;
        if (hc_run_src(PLAYLITE_ZC, &got) != 0 || got != 15) {
            con_puts("playlite FAIL\n");
            return;
        }
        con_puts("playlite ok\n");
        return;
    }
    if (streq(line, "inputlite")) {
        uint64_t got = 0;
        if (hc_run_src(INPUTLITE_ZC, &got) != 0 || got != 31) {
            con_puts("inputlite FAIL\n");
            return;
        }
        con_puts("inputlite ok\n");
        return;
    }
    if (streq(line, "rightlite")) {
        uint64_t got = 0;
        if (hc_run_src(RIGHTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("rightlite FAIL\n");
            return;
        }
        con_puts("rightlite ok\n");
        return;
    }
    if (streq(line, "cursorlite")) {
        uint64_t got = 0;
        if (hc_run_src(CURSORLITE_ZC, &got) != 0 || got != 15) {
            con_puts("cursorlite FAIL\n");
            return;
        }
        con_puts("cursorlite ok\n");
        return;
    }
    if (streq(line, "uplite")) {
        uint64_t got = 0;
        if (hc_run_src(UPLITE_ZC, &got) != 0 || got != 15) {
            con_puts("uplite FAIL\n");
            return;
        }
        con_puts("uplite ok\n");
        return;
    }
    if (streq(line, "ticklite")) {
        uint64_t got = 0;
        if (hc_run_src(TICKLITE_ZC, &got) != 0 || got != 15) {
            con_puts("ticklite FAIL\n");
            return;
        }
        con_puts("ticklite ok\n");
        return;
    }
    if (streq(line, "framelite")) {
        uint64_t got = 0;
        if (hc_run_src(FRAMELITE_ZC, &got) != 0 || got != 15) {
            con_puts("framelite FAIL\n");
            return;
        }
        con_puts("framelite ok\n");
        return;
    }
    if (streq(line, "plotdclite")) {
        uint64_t got = 0;
        if (hc_run_src(PLOTDCLITE_ZC, &got) != 0 || got != 15) {
            con_puts("plotdclite FAIL\n");
            return;
        }
        con_puts("plotdclite ok\n");
        return;
    }
    if (streq(line, "abortlite")) {
        uint64_t got = 0;
        if (hc_run_src(ABORTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("abortlite FAIL\n");
            return;
        }
        con_puts("abortlite ok\n");
        return;
    }
    if (streq(line, "aimmovelite")) {
        uint64_t got = 0;
        if (hc_run_src(AIMMOVELITE_ZC, &got) != 0 || got != 15) {
            con_puts("aimmovelite FAIL\n");
            return;
        }
        con_puts("aimmovelite ok\n");
        return;
    }
    if (streq(line, "idlelite")) {
        uint64_t got = 0;
        if (hc_run_src(IDLELITE_ZC, &got) != 0 || got != 15) {
            con_puts("idlelite FAIL\n");
            return;
        }
        con_puts("idlelite ok\n");
        return;
    }
    if (streq(line, "layerlite")) {
        uint64_t got = 0;
        if (hc_run_src(LAYERLITE_ZC, &got) != 0 || got != 15) {
            con_puts("layerlite FAIL\n");
            return;
        }
        con_puts("layerlite ok\n");
        return;
    }
    if (streq(line, "endslite")) {
        uint64_t got = 0;
        if (hc_run_src(ENDSLITE_ZC, &got) != 0 || got != 15) {
            con_puts("endslite FAIL\n");
            return;
        }
        con_puts("endslite ok\n");
        return;
    }
    if (streq(line, "speedlite")) {
        uint64_t got = 0;
        if (hc_run_src(SPEEDLITE_ZC, &got) != 0 || got != 15) {
            con_puts("speedlite FAIL\n");
            return;
        }
        con_puts("speedlite ok\n");
        return;
    }
    if (streq(line, "midlite")) {
        uint64_t got = 0;
        if (hc_run_src(MIDLITE_ZC, &got) != 0 || got != 15) {
            con_puts("midlite FAIL\n");
            return;
        }
        con_puts("midlite ok\n");
        return;
    }
    if (streq(line, "livelite")) {
        uint64_t got = 0;
        if (hc_run_src(LIVELITE_ZC, &got) != 0 || got != 15) {
            con_puts("livelite FAIL\n");
            return;
        }
        con_puts("livelite ok\n");
        return;
    }
    if (streq(line, "accellite")) {
        uint64_t got = 0;
        if (hc_run_src(ACCELLITE_ZC, &got) != 0 || got != 15) {
            con_puts("accellite FAIL\n");
            return;
        }
        con_puts("accellite ok\n");
        return;
    }
    if (streq(line, "restartlite")) {
        uint64_t got = 0;
        if (hc_run_src(RESTARTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("restartlite FAIL\n");
            return;
        }
        con_puts("restartlite ok\n");
        return;
    }
    if (streq(line, "widthlite")) {
        uint64_t got = 0;
        if (hc_run_src(WIDTHLITE_ZC, &got) != 0 || got != 15) {
            con_puts("widthlite FAIL\n");
            return;
        }
        con_puts("widthlite ok\n");
        return;
    }
    if (streq(line, "bothcolorlite")) {
        uint64_t got = 0;
        if (hc_run_src(BOTHCOLORLITE_ZC, &got) != 0 || got != 15) {
            con_puts("bothcolorlite FAIL\n");
            return;
        }
        con_puts("bothcolorlite ok\n");
        return;
    }
    if (streq(line, "menufulllite")) {
        uint64_t got = 0;
        if (hc_run_src(MENUFULLLITE_ZC, &got) != 0 || got != 15) {
            con_puts("menufulllite FAIL\n");
            return;
        }
        con_puts("menufulllite ok\n");
        return;
    }
    if (streq(line, "menubiglite")) {
        uint64_t got = 0;
        if (hc_run_src(MENUBIGLITE_ZC, &got) != 0 || got != 15) {
            con_puts("menubiglite FAIL\n");
            return;
        }
        con_puts("menubiglite ok\n");
        return;
    }
    if (streq(line, "trylite")) {
        uint64_t got = 0;
        if (hc_run_src(TRYLITE_ZC, &got) != 0 || got != 15) {
            con_puts("trylite FAIL\n");
            return;
        }
        con_puts("trylite ok\n");
        return;
    }
    if (streq(line, "stepcountlite")) {
        uint64_t got = 0;
        if (hc_run_src(STEPCOUNTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("stepcountlite FAIL\n");
            return;
        }
        con_puts("stepcountlite ok\n");
        return;
    }
    if (streq(line, "anglesfulllite")) {
        uint64_t got = 0;
        if (hc_run_src(ANGLESFULLLITE_ZC, &got) != 0 || got != 15) {
            con_puts("anglesfulllite FAIL\n");
            return;
        }
        con_puts("anglesfulllite ok\n");
        return;
    }
    if (streq(line, "braceangleslite")) {
        uint64_t got = 0;
        if (hc_run_src(BRACEANGLESLITE_ZC, &got) != 0 || got != 15) {
            con_puts("braceangleslite FAIL\n");
            return;
        }
        con_puts("braceangleslite ok\n");
        return;
    }
    if (streq(line, "bracepilite")) {
        uint64_t got = 0;
        if (hc_run_src(BRACEPILITE_ZC, &got) != 0 || got != 15) {
            con_puts("bracepilite FAIL\n");
            return;
        }
        con_puts("bracepilite ok\n");
        return;
    }
    if (streq(line, "setmenulite")) {
        uint64_t got = 0;
        if (hc_run_src(SETMENULITE_ZC, &got) != 0 || got != 15) {
            con_puts("setmenulite FAIL\n");
            return;
        }
        con_puts("setmenulite ok\n");
        return;
    }
    if (streq(line, "nearlatticelite")) {
        uint64_t got = 0;
        if (hc_run_src(NEARLATTICELITE_ZC, &got) != 0 || got != 15) {
            con_puts("nearlatticelite FAIL\n");
            return;
        }
        shell_fb_ready();
        con_puts("nearlatticelite ok\n");
        return;
    }
    if (streq(line, "f64iflite")) {
        uint64_t got = 0;
        if (hc_run_src(F64IFLITE_ZC, &got) != 0 || got != 15) {
            con_puts("f64iflite FAIL\n");
            return;
        }
        con_puts("f64iflite ok\n");
        return;
    }
    if (streq(line, "wraplatticelite")) {
        uint64_t got = 0;
        if (hc_run_src(WRAPLATTICELITE_ZC, &got) != 0 || got != 15) {
            con_puts("wraplatticelite FAIL\n");
            return;
        }
        con_puts("wraplatticelite ok\n");
        return;
    }
    if (streq(line, "menulooplite")) {
        uint64_t got = 0;
        if (hc_run_src(MENULOOPLITE_ZC, &got) != 0 || got != 15) {
            con_puts("menulooplite FAIL\n");
            return;
        }
        con_puts("menulooplite ok\n");
        return;
    }
    if (streq(line, "idxalllite")) {
        uint64_t got = 0;
        if (hc_run_src(IDXALLLITE_ZC, &got) != 0 || got != 15) {
            con_puts("idxalllite FAIL\n");
            return;
        }
        con_puts("idxalllite ok\n");
        return;
    }
    if (streq(line, "disklat")) {
        size_t n = 0;
        uint64_t got = 0;
        if (rs_fmt_host(128, 7) != 0) {
            con_puts("disklat fmt FAIL\n");
            return;
        }
        if (rs_put_file("DiskLat.ZC", DISKLAT_ZC, str_len(DISKLAT_ZC)) != 0) {
            con_puts("disklat put FAIL\n");
            return;
        }
        if (rs_load_file("DiskLat.ZC", g_hc_zc_src, sizeof(g_hc_zc_src), &n) != 0) {
            con_puts("disklat load FAIL\n");
            return;
        }
        if (hc_run_src(g_hc_zc_src, &got) != 0 || got != 15) {
            con_puts("disklat FAIL\n");
            return;
        }
        shell_fb_ready();
        con_puts("disklat ok\n");
        return;
    }
    if (streq(line, "latticeplay")) {
        uint64_t got = 0;
        /* Live UTM Lattice: same DiskLat body, no scripted MsgQue — ESC to exit. */
        if (hc_lattice_play_src(DISKLAT_ZC, g_hc_zc_src, sizeof(g_hc_zc_src), 0) != 0) {
            con_puts("latticeplay build FAIL\n");
            return;
        }
        /* M179: serial/FB shell cue before the live loop (UTM Terminal 1). */
        con_puts("latticeplay: Esc Enter Space c +/- e | L-click place | R-drag aim | arrows | 0-9\n");
        if (hc_run_src_ex(g_hc_zc_src, &got, 1) != 0) {
            con_puts("latticeplay FAIL\n");
            return;
        }
        shell_fb_ready();
        con_puts("latticeplay ok\n");
        return;
    }
    if (streq(line, "lattice")) {
        size_t n = 0;
        uint64_t got = 0;
        if (rs_fmt_host(128, 7) != 0) {
            con_puts("lattice fmt FAIL\n");
            return;
        }
        if (rs_put_file("Lattice.ZC", DISKLAT_ZC, str_len(DISKLAT_ZC)) != 0) {
            con_puts("lattice put FAIL\n");
            return;
        }
        if (rs_load_file("Lattice.ZC", g_hc_zc_src, sizeof(g_hc_zc_src), &n) != 0) {
            con_puts("lattice load FAIL\n");
            return;
        }
        if (hc_run_src(g_hc_zc_src, &got) != 0 || got != 15) {
            con_puts("lattice FAIL\n");
            return;
        }
        (void)rs_del_file("Lattice.ZC");
        shell_fb_ready();
        con_puts("lattice ok\n");
        return;
    }
    if (streq(line, "depthbuflite")) {
        uint64_t got = 0;
        if (hc_run_src(DEPTHBUFLITE_ZC, &got) != 0 || got != 15) {
            con_puts("depthbuflite FAIL\n");
            return;
        }
        con_puts("depthbuflite ok\n");
        return;
    }
    if (streq(line, "depthrstlite")) {
        uint64_t got = 0;
        if (hc_run_src(DEPTHRSTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("depthrstlite FAIL\n");
            return;
        }
        con_puts("depthrstlite ok\n");
        return;
    }
    if (streq(line, "depthplotlite")) {
        uint64_t got = 0;
        if (hc_run_src(DEPTHPLOTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("depthplotlite FAIL\n");
            return;
        }
        con_puts("depthplotlite ok\n");
        return;
    }
    if (streq(line, "depthlinelite")) {
        uint64_t got = 0;
        if (hc_run_src(DEPTHLINELITE_ZC, &got) != 0 || got != 15) {
            con_puts("depthlinelite FAIL\n");
            return;
        }
        con_puts("depthlinelite ok\n");
        return;
    }
    if (streq(line, "peekplot")) {
        uint64_t got = 0;
        if (hc_run_src(PEEKPLOT_ZC, &got) != 0 || got == 0) {
            con_puts("peekplot FAIL\n");
            return;
        }
        con_puts("peekplot ok n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "offbmp")) {
        uint64_t got = 0;
        if (hc_run_src(OFFBMP_ZC, &got) != 0 || got == 0) {
            con_puts("offbmp FAIL\n");
            return;
        }
        con_puts("offbmp ok lit=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "heapstr")) {
        uint64_t got = 0;
        if (hc_run_src(HEAPSTR_ZC, &got) != 0 || got != 6) {
            con_puts("heapstr FAIL\n");
            return;
        }
        con_puts("heapstr ok n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "catfmt")) {
        uint64_t got = 0;
        if (hc_run_src(CATFMT_ZC, &got) != 0 || got != 12) {
            con_puts("catfmt FAIL\n");
            return;
        }
        con_puts("catfmt ok n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "heapque")) {
        uint64_t got = 0;
        if (hc_run_src(HEAPQUE_ZC, &got) != 0 || got != 150) {
            con_puts("heapque FAIL\n");
            return;
        }
        con_puts("heapque ok sum=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "jobque")) {
        uint64_t got = 0;
        if (hc_run_src(JOBQUE_ZC, &got) != 0 || got != 120) {
            con_puts("jobque FAIL\n");
            return;
        }
        con_puts("jobque ok sum=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "jobrun")) {
        uint64_t got = 0;
        if (hc_run_src(JOBRUN_ZC, &got) != 0 || got != 15) {
            con_puts("jobrun FAIL\n");
            return;
        }
        con_puts("jobrun ok sum=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "spawn")) {
        uint64_t got = 0;
        if (hc_run_src(TASKSPAWN_ZC, &got) != 0 || got != 42) {
            con_puts("spawn FAIL\n");
            return;
        }
        con_puts("spawn ok sum=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "popup")) {
        uint64_t got = 0;
        if (hc_run_src(POPUP_ZC, &got) != 0 || got != 27) {
            con_puts("popup FAIL\n");
            return;
        }
        con_puts("popup ok sum=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "doclite")) {
        uint64_t got = 0;
        if (hc_run_src(DOCLITE_ZC, &got) != 0 || got != 7) {
            con_puts("doclite FAIL\n");
            return;
        }
        con_puts("doclite ok n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "ramblk")) {
        uint64_t got = 0;
        if (hc_run_src(RAMBLK_ZC, &got) != 0 || got != 414) {
            con_puts("ramblk FAIL\n");
            return;
        }
        con_puts("ramblk ok sum=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "namefile")) {
        uint64_t got = 0;
        if (hc_run_src(NAMEFILE_ZC, &got) != 0 || got != 5) {
            con_puts("namefile FAIL\n");
            return;
        }
        con_puts("namefile ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "dirlook")) {
        uint64_t got = 0;
        if (hc_run_src(DIRLOOK_ZC, &got) != 0 || got != 4) {
            con_puts("dirlook FAIL\n");
            return;
        }
        con_puts("dirlook ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "dirdel")) {
        uint64_t got = 0;
        if (hc_run_src(DIRDEL_ZC, &got) != 0 || got != 2) {
            con_puts("dirdel FAIL\n");
            return;
        }
        con_puts("dirdel ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "fopen")) {
        uint64_t got = 0;
        if (hc_run_src(FOPEN_ZC, &got) != 0 || got != 4) {
            con_puts("fopen FAIL\n");
            return;
        }
        con_puts("fopen ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "fwrite")) {
        uint64_t got = 0;
        if (hc_run_src(FWRITE_ZC, &got) != 0 || got != 3) {
            con_puts("fwrite FAIL\n");
            return;
        }
        con_puts("fwrite ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "multiblk")) {
        uint64_t got = 0;
        if (hc_run_src(MULTIBLK_ZC, &got) != 0 || got != 2) {
            con_puts("multiblk FAIL\n");
            return;
        }
        con_puts("multiblk ok n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "redsea")) {
        uint64_t got = 0;
        if (hc_run_src(REDSEA_ZC, &got) != 0 || got != 7) {
            con_puts("redsea FAIL\n");
            return;
        }
        con_puts("redsea ok uid=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsroot")) {
        uint64_t got = 0;
        if (hc_run_src(RSROOT_ZC, &got) != 0 || got != 2) {
            con_puts("rsroot FAIL\n");
            return;
        }
        con_puts("rsroot ok clus=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsfile")) {
        uint64_t got = 0;
        if (hc_run_src(RSFILE_ZC, &got) != 0 || got != 4) {
            con_puts("rsfile FAIL\n");
            return;
        }
        con_puts("rsfile ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsalloc")) {
        uint64_t got = 0;
        if (hc_run_src(RSALLOC_ZC, &got) != 0 || got != 3) {
            con_puts("rsalloc FAIL\n");
            return;
        }
        con_puts("rsalloc ok clus=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsfree")) {
        uint64_t got = 0;
        if (hc_run_src(RSFREE_ZC, &got) != 0 || got != 3) {
            con_puts("rsfree FAIL\n");
            return;
        }
        con_puts("rsfree ok clus=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsmulti")) {
        uint64_t got = 0;
        if (hc_run_src(RSMULTI_ZC, &got) != 0 || got != 2) {
            con_puts("rsmulti FAIL\n");
            return;
        }
        con_puts("rsmulti ok n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rscfile")) {
        uint64_t got = 0;
        if (hc_run_src(RSCFILE_ZC, &got) != 0 || got != 4) {
            con_puts("rscfile FAIL\n");
            return;
        }
        con_puts("rscfile ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rscwrite")) {
        uint64_t got = 0;
        if (hc_run_src(RSCWRITE_ZC, &got) != 0 || got != 3) {
            con_puts("rscwrite FAIL\n");
            return;
        }
        con_puts("rscwrite ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rscseek")) {
        uint64_t got = 0;
        if (hc_run_src(RSCSEEK_ZC, &got) != 0 || got != 4) {
            con_puts("rscseek FAIL\n");
            return;
        }
        con_puts("rscseek ok size=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsclib")) {
        uint64_t got = 0;
        if (hc_run_src(RSCLIB_ZC, &got) != 0 || got != 4) {
            con_puts("rsclib FAIL\n");
            return;
        }
        con_puts("rsclib ok size=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rspersist")) {
        uint64_t got = 0;
        if (hc_run_src(RSCPERSIST_ZC, &got) != 0 || got != 4) {
            con_puts("rspersist FAIL\n");
            return;
        }
        con_puts("rspersist ok size=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsdir")) {
        (void)rs_seed_catalog(); /* best-effort; always list */
        if (rs_dir_print() < 0) {
            con_puts("rsdir FAIL\n");
        }
        return;
    }
    if (streq(line, "rscatalog")) {
        if (rs_seed_catalog() != 0) {
            con_puts("rscatalog FAIL (no RedSea volume)\n");
            return;
        }
        con_puts("rscatalog ok\n");
        (void)rs_dir_print();
        return;
    }
    if (str_startswith(line, "rsdel ")) {
        const char *name = skip_ws(line + 6);
        if (!*name || rs_del_file(name) != 0) {
            con_puts("rsdel FAIL\n");
            return;
        }
        con_puts("rsdel ok\n");
        return;
    }
    if (str_startswith(line, "rsrename ")) {
        char buf[64];
        char *oldn;
        char *newn;
        unsigned i;
        for (i = 0; i < sizeof(buf) && line[i]; i++) {
            buf[i] = line[i];
        }
        buf[i < sizeof(buf) ? i : sizeof(buf) - 1] = 0;
        oldn = (char *)skip_ws(buf + 9);
        newn = oldn;
        while (*newn && *newn != ' ' && *newn != '\t') {
            newn++;
        }
        if (!*newn) {
            con_puts("rsrename usage: rsrename old new\n");
            return;
        }
        *newn++ = 0;
        newn = (char *)skip_ws(newn);
        if (!*newn || rs_rename_file(oldn, newn) != 0) {
            con_puts("rsrename FAIL\n");
            return;
        }
        con_puts("rsrename ok\n");
        return;
    }
    if (streq(line, "runzc") || str_startswith(line, "runzc ")) {
        const char *name = "Hi.ZC";
        char src[4096];
        uint64_t got = 0;
        size_t n = 0;
        const struct rs_catalog_ent *cat;

        if (str_startswith(line, "runzc ")) {
            name = skip_ws(line + 6);
            if (!*name) {
                name = "Hi.ZC";
            }
        }

        if (hc_run_src(RUNZC_ZC, &got) != 0 || got != 10) {
            con_puts("runzc seed FAIL\n");
            return;
        }
        (void)rs_seed_catalog(); /* best-effort on possibly-full PCI root */
        if (str_ieq(name, "Notes.ZC")) {
            (void)rs_ensure_catalog_file("DocLib.ZC");
        }
        if (str_ieq(name, "UseAdd.ZC")) {
            (void)rs_ensure_catalog_file("AddLib.ZC");
        }
        (void)rs_ensure_catalog_file(name);
        if (rs_load_file(name, src, sizeof(src), &n) != 0) {
            con_puts("runzc load FAIL name=");
            con_puts(name);
            con_puts(" (rsdir; free a slot if catalog full)\n");
            return;
        }
        got = 0;
        cat = rs_catalog_lookup(name);
        if (str_ieq(name, "Hi.ZC")) {
            if (hc_run_src(src, &got) != 0 || got != 42) {
                con_puts("runzc JIT FAIL got=");
                uart_put_u64_hex(g_uart, got);
                con_puts("\n");
                return;
            }
            con_puts("runzc Hi.ZC => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
            return;
        }
        if (hc_run_src(src, &got) != 0) {
            con_puts("runzc JIT FAIL name=");
            con_puts(name);
            con_puts("\n");
            return;
        }
        if (cat && !cat->graphic && str_ieq(name, "MemSort.ZC") && got != 16) {
            con_puts("runzc MemSort bad => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
            return;
        }
        if (cat && !cat->graphic && str_ieq(name, "UseAdd.ZC") && got != 42) {
            con_puts("runzc UseAdd bad => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
            return;
        }
        if (cat && str_ieq(name, "Notes.ZC") && got != 14) {
            con_puts("runzc Notes bad => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
            return;
        }
        con_puts("runzc ");
        con_puts(name);
        con_puts(" ok n=");
        uart_put_u64_hex(g_uart, (uint64_t)n);
        if (rs_is_graphic_zc(name)) {
            con_puts(" (disk+JIT; inspect FB)\n");
            (void)hc_builtin_sleep(2000);
        } else {
            con_puts(" => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
        }
        return;
    }
    if (streq(line, "vblk")) {
        uint8_t buf[512];
        unsigned i;
        if (!virtio_blk_ready()) {
            con_puts("vblk none\n");
            return;
        }
        for (i = 0; i < 512; i++) {
            buf[i] = 0;
        }
        buf[0] = 0xA5;
        buf[1] = 0x5A;
        buf[2] = 'V';
        buf[3] = 'B';
        if (!virtio_blk_write(buf, 15, 1)) {
            con_puts("vblk write FAIL\n");
            return;
        }
        for (i = 0; i < 512; i++) {
            buf[i] = 0;
        }
        if (!virtio_blk_read(buf, 15, 1) || buf[0] != 0xA5 || buf[1] != 0x5A ||
            buf[2] != 'V' || buf[3] != 'B') {
            con_puts("vblk read FAIL\n");
            return;
        }
        con_puts("vblk ok cap=");
        uart_put_u64_hex(g_uart, virtio_blk_capacity());
        con_puts("\n");
        return;
    }
    if (streq(line, "stars")) {
        uint64_t got = 0;
        const char *src =
            "Cls(0x00000810);"
            "I64 i=0;"
            "while(i<120){"
            "  PutPixel(Rand()%FbW(),Rand()%FbH(),0x00E0E0F0);"
            "  i++;"
            "}"
            "PrintAt(2,1,\"stars\");"
            "return i;";
        if (hc_run_src(src, &got) != 0 || got != 120) {
            con_puts("stars FAIL\n");
            return;
        }
        con_puts("stars n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "circles")) {
        uint64_t got = 0;
        const char *src =
            "Cls(0x00101820);"
            "GrFillCircle(200,200,50,0x004080C0);"
            "GrCircle(200,200,80,0x00E08020);"
            "GrCircle(400,220,60,0x0040C0F0);"
            "GrCircle(300,320,100,0x00C040A0);"
            "PrintAt(2,1,\"circles\");"
            "return 3;";
        if (hc_run_src(src, &got) != 0 || got != 3) {
            con_puts("circles FAIL\n");
            return;
        }
        con_puts("circles ok\n");
        return;
    }
    if (streq(line, "bars")) {
        uint64_t got = 0;
        const char *src =
            "Cls(0x00101820);"
            "FillRect(40,60,80,220,0x00C04040);"
            "FillRect(140,60,80,220,0x0040C040);"
            "FillRect(240,60,80,220,0x004040C0);"
            "GrLine(40,300,320,300,0x00E0E0E0,1);"
            "GrLine(40,60,320,300,0x00F0C040,1);"
            "PrintAt(2,1,\"bars\");"
            "return 3;";
        if (hc_run_src(src, &got) != 0 || got != 3) {
            con_puts("bars FAIL\n");
            return;
        }
        con_puts("bars ok\n");
        return;
    }
    if (streq(line, "bounce")) {
        uint64_t got = 0;
        /* Erase/redraw only the sprite — avoid full-screen Cls per frame. */
        const char *src =
            "PrintAt(2,1,\"bounce\");"
            "FillRect(20,40,380,280,0x00101820);"
            "I64 x=40;I64 y=80;I64 dx=5;I64 dy=3;I64 i=0;"
            "while(i<36){"
            "  FillRect(x,y,22,22,0x00E08020);"
            "  Sleep(8);"
            "  FillRect(x,y,22,22,0x00101820);"
            "  x=x+dx;y=y+dy;"
            "  if(x<20){x=20;dx=0-dx;}"
            "  if(x>360){x=360;dx=0-dx;}"
            "  if(y<50){y=50;dy=0-dy;}"
            "  if(y>280){y=280;dy=0-dy;}"
            "  i++;"
            "}"
            "FillRect(x,y,22,22,0x00F0C040);"
            "return i;";
        if (hc_run_src(src, &got) != 0 || got != 36) {
            con_puts("bounce FAIL\n");
            return;
        }
        con_puts("bounce frames=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "paint")) {
        uint64_t got = 0;
        /* Frame-bounded so headless script exits with dots=0 (no tablet hold). */
        const char *src =
            "Print(\"paint: drag; Esc quits\\n\");"
            "I64 i=0;I64 n=0;"
            "while(i<100){"
            "  if(KeyHit()){I64 k=GetKey();if(k==27)break;}"
            "  if(MouseBtn()){PutPixel(Clamp(MouseX(),0,799),Clamp(MouseY(),0,599),0x00E08020);n++;}"
            "  Sleep(10);"
            "  i++;"
            "}"
            "return n;";
        if (hc_run_src(src, &got) != 0) {
            con_puts("paint FAIL\n");
            return;
        }
        con_puts("paint dots=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "abs")) {
        uint8_t bc[64];
        uint64_t got;
        int n = hc_build_abs(bc, sizeof(bc), -42);
        if (n > 0 && hc_run_bc(bc, (size_t)n, &got) == 0) {
            con_puts("abs(-42) => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
        } else {
            con_puts("abs FAIL\n");
        }
        return;
    }
    if (line[0] == 'h' && line[1] == 'c' && (line[2] == ' ' || line[2] == '\t')) {
        const char *src = line + 3;
        while (*src == ' ' || *src == '\t') {
            src++;
        }
        uint64_t got = 0;
        if (hc_run_src(src, &got) != 0) {
            con_puts("hc FAIL\n");
            return;
        }
        con_puts("hc => ");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "sum")) {
        /* SumTo(10) via IR — same as demo.hcbc */
        static const uint8_t sum_bc[] = {
            HC_IMM64, 10, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 0,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 1,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 2,
            HC_LABEL, 0,
            HC_LD_LOCAL, 2, HC_LD_LOCAL, 0, HC_LT, HC_JZ, 1,
            HC_LD_LOCAL, 1, HC_LD_LOCAL, 2, HC_ADD, HC_ST_LOCAL, 1,
            HC_LD_LOCAL, 2, HC_IMM64, 1, 0, 0, 0, 0, 0, 0, 0, HC_ADD,
            HC_ST_LOCAL, 2, HC_JMP, 0,
            HC_LABEL, 1, HC_LD_LOCAL, 1, HC_RET, HC_END,
        };
        uint64_t got;
        if (hc_run_bc(sum_bc, sizeof(sum_bc), &got) == 0) {
            con_puts("sum(10) => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
        } else {
            con_puts("sum FAIL\n");
        }
        return;
    }
    uint64_t got;
    if (expr_compile_run(line, &got) == 0) {
        con_puts("=> ");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
    } else {
        con_puts("?\n");
    }
}

static void shell_run(void) {
    /* Interactive by default: no auto-script, no timeout.
     * Automated demos belong in jit_smoke / make run-serial kill, not here. */
    /* M151: wipe FB smoke residue (bars/lines) so UTM shows a clean shell. */
    if (g_fb) {
        fb_clear(0x00101820u);
        g_fb_cx = 0;
        g_fb_cy = 0;
    }
    con_puts("\nZealOS aarch64 shell (HolyC-IR exprs)\n");
    /* M152: short FB banner; full cmd list via `help` (800x600 wraps badly). */
    con_puts("type: help | vblk | rscatalog | runzc Notes.ZC | halt\n");
    con_puts("      hc <src> | bars | paint | nearlatticelite | disklat | latticeplay\n");
    con_puts("> ");
    char line[64];
    unsigned len = 0;
    int done = 0;
    while (!done) {
        int c = kbd_getc_nb();
        tablet_cursor_tick();
        if (c < 0) {
            continue;
        }
        if (c == '\r' || c == '\n') {
            con_puts("\n");
            line[len] = 0;
            if (len) {
                shell_handle(line, &done);
            }
            if (!done) {
                con_puts("> ");
            }
            len = 0;
            continue;
        }
        if (c == 0x7f || c == 0x08) {
            if (len) {
                len--;
                con_puts("\b \b");
            }
            continue;
        }
        if (c >= 32 && c < 127 && len + 1 < sizeof(line)) {
            line[len++] = (char)c;
            con_write((char)c);
        }
    }
}

static int jit_smoke(void) {
    uint8_t bc[80];
    size_t n = 0;
    uint64_t got = 0;

    /* Aiwnios-shaped BL relative call (a64_emit slice). */
    {
        uint32_t *p = g_jit_buf;
        *p++ = a64_stp_pre(30, 31, 31, -16); /* save LR */
        *p++ = a64_bl(3);                    /* call +3 words → movz */
        *p++ = a64_ldp_post(30, 31, 31, 16); /* restore LR */
        *p++ = a64_ret();
        *p++ = a64_movz_x(0, 42, 0);
        *p++ = a64_ret();
        size_t bytes = (size_t)((p - g_jit_buf) * 4);
        jit_icache_flush(g_jit_buf, bytes);
        got = ((uint64_t(*)(void))(void *)g_jit_buf)();
        if (got != 42) {
            uart_puts(g_uart, "hc: BL slice FAIL\n");
            return -36;
        }
        uart_puts(g_uart, "hc: BL slice => ");
        uart_put_u64_hex(g_uart, got);
        uart_puts(g_uart, "\n");
    }

    n += hc_pack_imm64(bc + n, 0x1000);
    n += hc_pack_imm64(bc + n, 0x20);
    bc[n++] = HC_ADD;
    n += hc_pack_imm64(bc + n, 0xC0DE);
    bc[n++] = HC_ADD;
    bc[n++] = HC_RET;
    bc[n++] = HC_END;
    if (hc_run_bc(bc, n, &got) != 0 || got != 0x1000ull + 0x20ull + 0xC0DEull) {
        uart_puts(g_uart, "hc: Foo FAIL\n");
        return -1;
    }
    uart_puts(g_uart, "hc: IR Foo => ");
    uart_put_u64_hex(g_uart, got);
    uart_puts(g_uart, "\n");

    n = 0;
    n += hc_pack_imm64(bc + n, 3);
    n += hc_pack_imm64(bc + n, 7);
    bc[n++] = HC_MUL;
    n += hc_pack_imm64(bc + n, 1);
    bc[n++] = HC_SUB;
    bc[n++] = HC_RET;
    bc[n++] = HC_END;
    if (hc_run_bc(bc, n, &got) != 0 || got != 20) {
        return -2;
    }
    uart_puts(g_uart, "hc: IR MulSub => ");
    uart_put_u64_hex(g_uart, got);
    uart_puts(g_uart, "\n");

    n = 0;
    n += hc_pack_call0(bc + n, (uint64_t)(uintptr_t)jit_host_puts);
    n += hc_pack_call0(bc + n, (uint64_t)(uintptr_t)hc_host_marker);
    bc[n++] = HC_RET;
    bc[n++] = HC_END;
    if (hc_run_bc(bc, n, &got) != 0 || got != 0x48434F4BULL) {
        return -3;
    }
    uart_puts(g_uart, "hc: IR Call => ");
    uart_put_u64_hex(g_uart, got);
    uart_puts(g_uart, "\n");

    /* Abs(-5) with locals+branches — packed offline (same layout as host test) */
    {
        static const uint8_t abs_bc[] = {
            /* IMM -5 */ HC_IMM64, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            /* ST_LOCAL 0 */ HC_ST_LOCAL, 0,
            /* LD_LOCAL 0 */ HC_LD_LOCAL, 0,
            /* IMM 0 */ HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0,
            HC_LT,
            /* JNZ 1 */ HC_JNZ, 1,
            /* LD_LOCAL 0; RET */ HC_LD_LOCAL, 0, HC_RET,
            /* LABEL 1 */ HC_LABEL, 1,
            /* LD; NEG; RET */ HC_LD_LOCAL, 0, HC_NEG, HC_RET,
            HC_END,
        };
        int rc = hc_run_bc(abs_bc, sizeof(abs_bc), &got);
        if (rc != 0 || got != 5) {
            uart_puts(g_uart, "hc: Abs(-5) FAIL rc=");
            uart_put_u32_dec(g_uart, rc < 0 ? (uint32_t)(-rc) : 0);
            uart_puts(g_uart, " got=");
            uart_put_u64_hex(g_uart, got);
            uart_puts(g_uart, " n=");
            uart_put_u32_dec(g_uart, (uint32_t)sizeof(abs_bc));
            uart_puts(g_uart, "\n");
            return -4;
        }
    }
    uart_puts(g_uart, "hc: Abs(-5) => ");
    uart_put_u64_hex(g_uart, got);
    uart_puts(g_uart, "\n");

    {
        static const uint8_t abs7_bc[] = {
            HC_IMM64, 7, 0, 0, 0, 0, 0, 0, 0,
            HC_ST_LOCAL, 0,
            HC_LD_LOCAL, 0,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0,
            HC_LT,
            HC_JNZ, 1,
            HC_LD_LOCAL, 0, HC_RET,
            HC_LABEL, 1,
            HC_LD_LOCAL, 0, HC_NEG, HC_RET,
            HC_END,
        };
        if (hc_run_bc(abs7_bc, sizeof(abs7_bc), &got) != 0 || got != 7) {
            uart_puts(g_uart, "hc: Abs(7) FAIL\n");
            return -5;
        }
    }
    uart_puts(g_uart, "hc: Abs(7) => ");
    uart_put_u64_hex(g_uart, got);
    uart_puts(g_uart, "\n");

    /* SumTo(5): s=0; i=0; while (i < 5) { s += i; i++; } → 10 */
    {
        static const uint8_t sum_bc[] = {
            HC_IMM64, 5, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 0,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 1,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 2,
            HC_LABEL, 0,
            HC_LD_LOCAL, 2, HC_LD_LOCAL, 0, HC_LT, HC_JZ, 1,
            HC_LD_LOCAL, 1, HC_LD_LOCAL, 2, HC_ADD, HC_ST_LOCAL, 1,
            HC_LD_LOCAL, 2, HC_IMM64, 1, 0, 0, 0, 0, 0, 0, 0, HC_ADD,
            HC_ST_LOCAL, 2, HC_JMP, 0,
            HC_LABEL, 1, HC_LD_LOCAL, 1, HC_RET, HC_END,
        };
        if (hc_run_bc(sum_bc, sizeof(sum_bc), &got) != 0 || got != 10) {
            uart_puts(g_uart, "hc: SumTo FAIL\n");
            return -6;
        }
        uart_puts(g_uart, "hc: SumTo(5) => ");
        uart_put_u64_hex(g_uart, got);
        uart_puts(g_uart, "\n");
    }

    /* Pointer: *(&slot0) = 0xAABB; return *(&slot0); */
    {
        static const uint8_t ptr_bc[] = {
            HC_SLOT_ADDR, 0,
            HC_IMM64, 0xBB, 0xAA, 0, 0, 0, 0, 0, 0,
            HC_STORE,
            HC_SLOT_ADDR, 0, HC_LOAD, HC_RET, HC_END,
        };
        g_hc_mem[0] = 0;
        if (hc_run_bc(ptr_bc, sizeof(ptr_bc), &got) != 0 || got != 0xAABB) {
            uart_puts(g_uart, "hc: Ptr FAIL got=");
            uart_put_u64_hex(g_uart, got);
            uart_puts(g_uart, "\n");
            return -7;
        }
        uart_puts(g_uart, "hc: Ptr => ");
        uart_put_u64_hex(g_uart, got);
        uart_puts(g_uart, "\n");
    }

    /* HolyC subset frontend */
    {
        uint64_t got2 = 0;
        if (hc_run_src("I64 s=0;I64 i=0;while(i<10){s=s+i;i=i+1;}return s;", &got2) != 0 ||
            got2 != 45) {
            uart_puts(g_uart, "hc: Front Sum FAIL\n");
            return -8;
        }
        uart_puts(g_uart, "hc: Front SumTo(10) => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=3;if(x<2)return 1;else return 9;", &got2) != 0 || got2 != 9) {
            uart_puts(g_uart, "hc: Front If FAIL\n");
            return -9;
        }
        uart_puts(g_uart, "hc: Front If => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 add(I64 a,I64 b){return a+b;}return add(20,22);", &got2) != 0 ||
            got2 != 42) {
            uart_puts(g_uart, "hc: Front Fn FAIL\n");
            return -11;
        }
        uart_puts(g_uart, "hc: Front Fn add => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=5;I64 p=&x;*p=7;return x;", &got2) != 0 || got2 != 7) {
            uart_puts(g_uart, "hc: Front Ptr FAIL\n");
            return -12;
        }
        uart_puts(g_uart, "hc: Front &/* => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 s=0;for(I64 i=0;i<10;i=i+1){s=s+i;}return s;", &got2) != 0 ||
            got2 != 45) {
            uart_puts(g_uart, "hc: Front For FAIL\n");
            return -13;
        }
        uart_puts(g_uart, "hc: Front For => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 s=0;I64 i=0;while(1){if(i>=5)break;s=s+i;i=i+1;}return s;",
                       &got2) != 0 ||
            got2 != 10) {
            uart_puts(g_uart, "hc: Front Break FAIL\n");
            return -14;
        }
        uart_puts(g_uart, "hc: Front Break => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 s=0;for(I64 i=0;i<10;i=i+1){if(i<5)continue;s=s+i;}return s;",
                       &got2) != 0 ||
            got2 != 35) {
            uart_puts(g_uart, "hc: Front Cont FAIL\n");
            return -15;
        }
        uart_puts(g_uart, "hc: Front Cont => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a[4];a[0]=3;a[1]=4;a[2]=5;return a[0]+a[1]+a[2];", &got2) != 0 ||
            got2 != 12) {
            uart_puts(g_uart, "hc: Front Arr FAIL\n");
            return -16;
        }
        uart_puts(g_uart, "hc: Front Arr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a[5];I64 s=0;for(I64 i=0;i<5;i=i+1){a[i]=i;}for(I64 j=0;j<5;j=j+1){"
                       "s=s+a[j];}return s;",
                       &got2) != 0 ||
            got2 != 10) {
            uart_puts(g_uart, "hc: Front ArrFor FAIL\n");
            return -17;
        }
        uart_puts(g_uart, "hc: Front ArrFor => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U8 b[4];b[0]=65;b[1]=2;b[2]=3;return b[0]+b[1]+b[2];", &got2) != 0 ||
            got2 != 70) {
            uart_puts(g_uart, "hc: Front U8 FAIL\n");
            return -18;
        }
        uart_puts(g_uart, "hc: Front U8 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return \"Az\"[0]+\"Az\"[1];", &got2) != 0 || got2 != (uint64_t)('A' + 'z')) {
            uart_puts(g_uart, "hc: Front Str FAIL\n");
            return -19;
        }
        uart_puts(g_uart, "hc: Front Str => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=2;switch(x){case 1:return 10;case 2:return 20;default:return 30;}",
                       &got2) != 0 ||
            got2 != 20) {
            uart_puts(g_uart, "hc: Front Switch FAIL\n");
            return -20;
        }
        uart_puts(g_uart, "hc: Front Switch => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=9;switch(x){case 1:return 1;default:return 99;}", &got2) != 0 ||
            got2 != 99) {
            uart_puts(g_uart, "hc: Front SwitchDef FAIL\n");
            return -21;
        }
        uart_puts(g_uart, "hc: Front SwitchDef => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class Point{I64 x;I64 y;};Point p;p.x=3;p.y=4;return p.x+p.y;",
                       &got2) != 0 ||
            got2 != 7) {
            uart_puts(g_uart, "hc: Front Class FAIL\n");
            return -22;
        }
        uart_puts(g_uart, "hc: Front Class => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class S{U8 a;I64 b;};S s;s.a=5;s.b=10;return s.a+s.b;", &got2) !=
                0 ||
            got2 != 15) {
            uart_puts(g_uart, "hc: Front ClassU8 FAIL\n");
            return -23;
        }
        uart_puts(g_uart, "hc: Front ClassU8 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class In{I64 v;};class Out{In i;I64 z;};Out o;o.i.v=5;o.z=2;return o.i.v+o.z;",
                       &got2) != 0 ||
            got2 != 7) {
            uart_puts(g_uart, "hc: Front Nest FAIL\n");
            return -24;
        }
        uart_puts(g_uart, "hc: Front Nest => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class P{I64 x;};P p;P *q=&p;q->x=9;return p.x;", &got2) != 0 ||
            got2 != 9) {
            uart_puts(g_uart, "hc: Front Arrow FAIL\n");
            return -25;
        }
        uart_puts(g_uart, "hc: Front Arrow => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 s=0;I64 i=0;do{s=s+i;i=i+1;}while(i<5);return s;", &got2) != 0 ||
            got2 != 10) {
            uart_puts(g_uart, "hc: Front Do FAIL\n");
            return -26;
        }
        uart_puts(g_uart, "hc: Front Do => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=3;x+=4;x*=2;x-=1;return x;", &got2) != 0 || got2 != 13) {
            uart_puts(g_uart, "hc: Front AsgOp FAIL\n");
            return -27;
        }
        uart_puts(g_uart, "hc: Front AsgOp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a=0;I64 b=5;return (a&&b)||(b==5);", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front Logic FAIL\n");
            return -28;
        }
        uart_puts(g_uart, "hc: Front Logic => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return (20/3)*3+(20%3);", &got2) != 0 || got2 != 20) {
            uart_puts(g_uart, "hc: Front DivMod FAIL\n");
            return -29;
        }
        uart_puts(g_uart, "hc: Front DivMod => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=0;return !x + !(x+1);", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front Not FAIL\n");
            return -30;
        }
        uart_puts(g_uart, "hc: Front Not => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=2;return x==2?40:7;", &got2) != 0 || got2 != 40) {
            uart_puts(g_uart, "hc: Front Tern FAIL\n");
            return -31;
        }
        uart_puts(g_uart, "hc: Front Tern => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return 0x10+0x2A;", &got2) != 0 || got2 != 0x3a) {
            uart_puts(g_uart, "hc: Front Hex FAIL\n");
            return -32;
        }
        uart_puts(g_uart, "hc: Front Hex => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return 'A'+59;", &got2) != 0 || got2 != 124) {
            uart_puts(g_uart, "hc: Front Char FAIL\n");
            return -33;
        }
        uart_puts(g_uart, "hc: Front Char => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=5;x++;x++;x--;return x;", &got2) != 0 || got2 != 6) {
            uart_puts(g_uart, "hc: Front Inc FAIL\n");
            return -34;
        }
        uart_puts(g_uart, "hc: Front Inc => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return (-8)>>2;", &got2) != 0 || got2 != (uint64_t)(int64_t)-2) {
            uart_puts(g_uart, "hc: Front Asr FAIL\n");
            return -35;
        }
        uart_puts(g_uart, "hc: Front Asr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=0;goto skip;x=99;skip:return x+3;", &got2) != 0 ||
            got2 != 3) {
            uart_puts(g_uart, "hc: Front Goto FAIL\n");
            return -37;
        }
        uart_puts(g_uart, "hc: Front Goto => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a=2,b=3,c=4;return a+b+c;", &got2) != 0 || got2 != 9) {
            uart_puts(g_uart, "hc: Front Multi FAIL\n");
            return -38;
        }
        uart_puts(g_uart, "hc: Front Multi => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return Abs(-11);", &got2) != 0 || got2 != 11) {
            uart_puts(g_uart, "hc: Front BuiltinAbs FAIL\n");
            return -39;
        }
        uart_puts(g_uart, "hc: Front BuiltinAbs => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("PutPixel(10,20,0x00FF00);return 1;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front PutPixel FAIL\n");
            return -40;
        }
        uart_puts(g_uart, "hc: Front PutPixel => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class Pt{I64 x;I64 y;};return sizeof(I64)+sizeof(U8)+sizeof(Pt);",
                       &got2) != 0 ||
            got2 != 8 + 1 + 16) {
            uart_puts(g_uart, "hc: Front Sizeof FAIL\n");
            return -41;
        }
        uart_puts(g_uart, "hc: Front Sizeof => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("Cls(0x101010);return 2;", &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Front Cls FAIL\n");
            return -42;
        }
        uart_puts(g_uart, "hc: Front Cls => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("FillRect(5,5,8,6,0x00C04020);return 3;", &got2) != 0 || got2 != 3) {
            uart_puts(g_uart, "hc: Front FillRect FAIL\n");
            return -43;
        }
        uart_puts(g_uart, "hc: Front FillRect => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return StrLen(\"Hello\");", &got2) != 0 || got2 != 5) {
            uart_puts(g_uart, "hc: Front StrLen FAIL\n");
            return -44;
        }
        uart_puts(g_uart, "hc: Front StrLen => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("PrintAt(2,2,\"Zeal\");return Print(\"ok\");", &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Front Print FAIL\n");
            return -45;
        }
        uart_puts(g_uart, "hc: Front Print => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a=Rand();I64 b=Rand();return a!=b;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front Rand FAIL\n");
            return -46;
        }
        uart_puts(g_uart, "hc: Front Rand => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Mouse* returns coords even if tablet idle (0,0). */
        if (hc_run_src("return MouseX()+MouseY()+MouseBtn()>=0;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front Mouse FAIL\n");
            return -47;
        }
        uart_puts(g_uart, "hc: Front Mouse => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return MouseDX()+MouseDY()==0;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front MouseD FAIL\n");
            return -61;
        }
        uart_puts(g_uart, "hc: Front MouseD => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("Sleep(2);return 8;", &got2) != 0 || got2 != 8) {
            uart_puts(g_uart, "hc: Front Sleep FAIL\n");
            return -48;
        }
        uart_puts(g_uart, "hc: Front Sleep => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Mini paint: a few samples; paints if button held. */
        if (hc_run_src("I64 i=0;while(i<4){if(MouseBtn())PutPixel(MouseX(),MouseY(),0x00F0C040);"
                       "i++;Sleep(1);}return i;",
                       &got2) != 0 ||
            got2 != 4) {
            uart_puts(g_uart, "hc: Front Paint FAIL\n");
            return -49;
        }
        uart_puts(g_uart, "hc: Front Paint => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return KeyHit()==0;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front KeyHit FAIL\n");
            return -50;
        }
        uart_puts(g_uart, "hc: Front KeyHit => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return GetKey()<0;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front GetKey FAIL\n");
            return -51;
        }
        uart_puts(g_uart, "hc: Front GetKey => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("GrLine(10,10,60,45,0x00FFFFFF,1);return 1;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front GrLine FAIL\n");
            return -52;
        }
        uart_puts(g_uart, "hc: Front GrLine => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("GrPlot(3,4,0x00AABBCC);return GrPeek(3,4);", &got2) != 0 ||
            got2 != 0x00AABBCC) {
            uart_puts(g_uart, "hc: Front GrPeek FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -93;
        }
        uart_puts(g_uart, "hc: Front GrPeek => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return GrPeek(-1,0);", &got2) != 0 || (int64_t)got2 != -1) {
            uart_puts(g_uart, "hc: Front GrPeekOOB FAIL\n");
            return -94;
        }
        if (hc_run_src("GrHLine(10,50,80,0x00FF8080);GrVLine(50,10,80,0x0080FF80);return 2;",
                       &got2) != 0 ||
            got2 != 2) {
            uart_puts(g_uart, "hc: Front GrHV FAIL\n");
            return -53;
        }
        uart_puts(g_uart, "hc: Front GrHV => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("Cls(0x00101820);return 1;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front ClsFast FAIL\n");
            return -54;
        }
        uart_puts(g_uart, "hc: Front ClsFast => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("enum {A,B=5,C};enum Color{RED=1,GREEN,BLUE};return A+B+C+RED+GREEN+BLUE;",
                       &got2) != 0 ||
            got2 != (0 + 5 + 6 + 1 + 2 + 3)) {
            uart_puts(g_uart, "hc: Front Enum FAIL\n");
            return -55;
        }
        uart_puts(g_uart, "hc: Front Enum => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return StrCmp(\"Zeal\",\"Zeal\");", &got2) != 0 || got2 != 0) {
            uart_puts(g_uart, "hc: Front StrCmp FAIL\n");
            return -56;
        }
        uart_puts(g_uart, "hc: Front StrCmp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return StrCmp(\"A\",\"B\")<0;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front StrCmpLt FAIL\n");
            return -57;
        }
        uart_puts(g_uart, "hc: Front StrCmpLt => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return StrCmp(\"Zeal\",\"Zeal\")==0 && StrCmp(\"A\",\"B\")<0;", &got2) != 0 ||
            got2 != 1) {
            uart_puts(g_uart, "hc: Front LandCmp FAIL\n");
            return -58;
        }
        uart_puts(g_uart, "hc: Front LandCmp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U8 d[4];MemCpy(d,\"Hi\",3);return d[0]+d[1]+d[2];", &got2) != 0 ||
            got2 != (uint64_t)('H' + 'i' + 0)) {
            uart_puts(g_uart, "hc: Front MemCpy FAIL\n");
            return -59;
        }
        uart_puts(g_uart, "hc: Front MemCpy => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U8 d[3];MemSet(d,7,3);return d[0]+d[1]+d[2];", &got2) != 0 || got2 != 21) {
            uart_puts(g_uart, "hc: Front MemSet FAIL\n");
            return -60;
        }
        uart_puts(g_uart, "hc: Front MemSet => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return Str2I64(\"-42\")+Str2I64(\"0x10\");", &got2) != 0 ||
            got2 != (uint64_t)(int64_t)(-42 + 0x10)) {
            uart_puts(g_uart, "hc: Front Str2I64 FAIL\n");
            return -62;
        }
        uart_puts(g_uart, "hc: Front Str2I64 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return PrintI64(-7);", &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Front PrintI64 FAIL\n");
            return -63;
        }
        uart_puts(g_uart, "hc: Front PrintI64 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a=Cnt();Sleep(1);return Cnt()>a;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front Cnt FAIL\n");
            return -64;
        }
        uart_puts(g_uart, "hc: Front Cnt => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return Min(3,9)+Max(-2,5);", &got2) != 0 || got2 != 8) {
            uart_puts(g_uart, "hc: Front MinMax FAIL\n");
            return -65;
        }
        uart_puts(g_uart, "hc: Front MinMax => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a(I64 x){return x+1;}I64 b(I64 x){return x+2;}I64 c(I64 x){return x+3;}"
                       "I64 d(I64 x){return x+4;}I64 e(I64 x){return x+5;}"
                       "return a(1)+b(1)+c(1)+d(1)+e(1);",
                       &got2) != 0 ||
            got2 != (2 + 3 + 4 + 5 + 6)) {
            uart_puts(g_uart, "hc: Front Fns8 FAIL\n");
            return -66;
        }
        uart_puts(g_uart, "hc: Front Fns8 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return Sign(-9)+Sign(0)+Sign(4);", &got2) != 0 ||
            got2 != (uint64_t)(int64_t)(-1 + 0 + 1)) {
            uart_puts(g_uart, "hc: Front Sign FAIL\n");
            return -67;
        }
        uart_puts(g_uart, "hc: Front Sign => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return Clamp(-5,0,10)+Clamp(3,0,10)+Clamp(99,0,10);", &got2) != 0 ||
            got2 != 13) {
            uart_puts(g_uart, "hc: Front Clamp FAIL\n");
            return -68;
        }
        uart_puts(g_uart, "hc: Front Clamp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return HashStr(\"Zeal\")!=HashStr(\"OS\") && HashStr(\"Zeal\")==HashStr(\"Zeal\");",
                       &got2) != 0 ||
            got2 != 1) {
            uart_puts(g_uart, "hc: Front HashStr FAIL\n");
            return -69;
        }
        uart_puts(g_uart, "hc: Front HashStr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return Sqr(-6);", &got2) != 0 || got2 != 36) {
            uart_puts(g_uart, "hc: Front Sqr FAIL\n");
            return -70;
        }
        uart_puts(g_uart, "hc: Front Sqr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 mix(I64 a,I64 b,I64 c,I64 d){return a+b*2+c*3+d*4;}return mix(1,2,3,4);",
                       &got2) != 0 ||
            got2 != (1 + 4 + 9 + 16)) {
            uart_puts(g_uart, "hc: Front Fn4 FAIL\n");
            return -71;
        }
        uart_puts(g_uart, "hc: Front Fn4 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 f5(I64 a,I64 b,I64 c,I64 d,I64 e){return a+b+c+d+e;}return f5(1,2,3,4,5);",
                       &got2) != 0 ||
            got2 != 15) {
            uart_puts(g_uart, "hc: Front Fn5 FAIL\n");
            return -72;
        }
        uart_puts(g_uart, "hc: Front Fn5 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U32 x=40;U32 y=2;return x+y;", &got2) != 0 || got2 != 42) {
            uart_puts(g_uart, "hc: Front U32 FAIL\n");
            return -73;
        }
        uart_puts(g_uart, "hc: Front U32 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U32 x=0xFFFFFFFF;x=x+1;return x;", &got2) != 0 || got2 != 0) {
            uart_puts(g_uart, "hc: Front U32Wrap FAIL\n");
            return -73;
        }
        uart_puts(g_uart, "hc: Front U32Wrap => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U32 a[3];a[0]=1;a[1]=2;a[2]=3;return a[0]+a[1]+a[2];", &got2) != 0 ||
            got2 != 6) {
            uart_puts(g_uart, "hc: Front U32Arr FAIL\n");
            return -73;
        }
        uart_puts(g_uart, "hc: Front U32Arr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class S{U32 a;U32 b;};S s;s.a=3;s.b=4;return s.a+s.b+sizeof(S);",
                       &got2) != 0 ||
            got2 != 15) {
            /* sizeof(S)=8 after align; 3+4+8=15 */
            uart_puts(g_uart, "hc: Front U32Struct FAIL\n");
            return -73;
        }
        uart_puts(g_uart, "hc: Front U32Struct => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("Bool t=1;Bool f=0;return t&&!f;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front Bool FAIL\n");
            return -74;
        }
        uart_puts(g_uart, "hc: Front Bool => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return sizeof(U32)+sizeof(Bool)+sizeof(U8);", &got2) != 0 || got2 != 13) {
            uart_puts(g_uart, "hc: Front SizeofU FAIL\n");
            return -75;
        }
        uart_puts(g_uart, "hc: Front SizeofU => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 nest(I64 x){I64 y=x+1;return y;}I64 a=5;return nest(a)+a;", &got2) !=
                0 ||
            got2 != 11) {
            uart_puts(g_uart, "hc: Front FrameNest FAIL\n");
            return -82;
        }
        uart_puts(g_uart, "hc: Front FrameNest => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 g;I64 inc(){g=g+1;return g;}g=0;return inc()+inc();", &got2) != 0 ||
            got2 != 3) {
            uart_puts(g_uart, "hc: Front Glob FAIL\n");
            return -149;
        }
        uart_puts(g_uart, "hc: Front Glob => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(GLOBSHARE_ZC, &got2) != 0 || got2 != 8) {
            uart_puts(g_uart, "hc: Upstream GlobShare FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -150;
        }
        uart_puts(g_uart, "hc: Upstream GlobShare => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(LIFE_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream Life FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -151;
        }
        uart_puts(g_uart, "hc: Upstream Life => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("F64 a=1.5;F64 b=2.5;return ToI64(a+b);", &got2) != 0 ||
            got2 != 4) {
            uart_puts(g_uart, "hc: Front F64 FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -152;
        }
        uart_puts(g_uart, "hc: Front F64 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return ToI64(-2.0);", &got2) != 0 ||
            got2 != (uint64_t)(int64_t)-2) {
            uart_puts(g_uart, "hc: Front F64Neg FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -154;
        }
        uart_puts(g_uart, "hc: Front F64Neg => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return ((-2.0)<=2.0)+((-2.0)<0.0);", &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Front F64Cmp FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -156;
        }
        uart_puts(g_uart, "hc: Front F64Cmp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("F64 x;x=-2.0;x=x+0.5;return ToI64(x*2.0);", &got2) != 0 ||
            got2 != (uint64_t)(int64_t)-3) {
            /* -1.5*2 = -3 */
            uart_puts(g_uart, "hc: Front F64Step FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -157;
        }
        uart_puts(g_uart, "hc: Front F64Step => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        got2 = 0;
        if (hc_run_src(
                "I64 n;F64 x;n=0;x=-2.0;while(x<=2.0){n=n+1;x=x+0.5;}return n;", &got2) != 0 ||
            got2 != 9) {
            uart_puts(g_uart, "hc: Front F64While FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -155;
        }
        uart_puts(g_uart, "hc: Front F64While => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(CARTLITE_ZC, &got2) != 0 || got2 != 14) {
            uart_puts(g_uart, "hc: Upstream CartLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -153;
        }
        uart_puts(g_uart, "hc: Upstream CartLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return (1.0!=2.0)+(3.0==3.0);", &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Front F64Ne FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -164;
        }
        uart_puts(g_uart, "hc: Front F64Ne => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(VEC2LITE_ZC, &got2) != 0 || got2 != 25) {
            uart_puts(g_uart, "hc: Upstream Vec2Lite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -165;
        }
        uart_puts(g_uart, "hc: Upstream Vec2Lite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ANGLESLITE_ZC, &got2) != 0 || got2 != 12) {
            uart_puts(g_uart, "hc: Upstream AnglesLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -166;
        }
        uart_puts(g_uart, "hc: Upstream AnglesLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(COSLITE_ZC, &got2) != 0 || got2 != 63) {
            uart_puts(g_uart, "hc: Upstream CosLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -167;
        }
        uart_puts(g_uart, "hc: Upstream CosLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(SQRTLITE_ZC, &got2) != 0 || got2 != 31) {
            uart_puts(g_uart, "hc: Upstream SqrtLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -168;
        }
        uart_puts(g_uart, "hc: Upstream SqrtLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ARGLITE_ZC, &got2) != 0 || got2 != 63) {
            uart_puts(g_uart, "hc: Upstream ArgLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -169;
        }
        uart_puts(g_uart, "hc: Upstream ArgLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(COMMALITE_ZC, &got2) != 0 || got2 != 63) {
            uart_puts(g_uart, "hc: Upstream CommaLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -170;
        }
        uart_puts(g_uart, "hc: Upstream CommaLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(PLOT3LITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream Plot3Lite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -171;
        }
        uart_puts(g_uart, "hc: Upstream Plot3Lite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TOSPILITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream TosPiLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -172;
        }
        uart_puts(g_uart, "hc: Upstream TosPiLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(COLORLITE_ZC, &got2) != 0 || got2 != 63) {
            uart_puts(g_uart, "hc: Upstream ColorLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -173;
        }
        uart_puts(g_uart, "hc: Upstream ColorLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TURTLELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream TurtleLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -174;
        }
        uart_puts(g_uart, "hc: Upstream TurtleLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FILLLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream FillLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -175;
        }
        uart_puts(g_uart, "hc: Upstream FillLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(INITLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream InitLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -176;
        }
        uart_puts(g_uart, "hc: Upstream InitLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DEFLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DefLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -177;
        }
        uart_puts(g_uart, "hc: Upstream DefLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(PRINTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream PrintLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -178;
        }
        uart_puts(g_uart, "hc: Upstream PrintLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MSGLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MsgLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -179;
        }
        uart_puts(g_uart, "hc: Upstream MsgLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MENULITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MenuLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -180;
        }
        uart_puts(g_uart, "hc: Upstream MenuLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FINDLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream FindLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -181;
        }
        uart_puts(g_uart, "hc: Upstream FindLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FSLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream FsLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -182;
        }
        uart_puts(g_uart, "hc: Upstream FsLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(SETUPLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream SetupLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -183;
        }
        uart_puts(g_uart, "hc: Upstream SetupLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream TtLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -184;
        }
        uart_puts(g_uart, "hc: Upstream TtLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(BUFLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream BufLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -185;
        }
        uart_puts(g_uart, "hc: Upstream BufLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(INCLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream IncLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -186;
        }
        uart_puts(g_uart, "hc: Upstream IncLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DCLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DcLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -187;
        }
        uart_puts(g_uart, "hc: Upstream DcLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(LINEDCLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream LineDcLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -188;
        }
        uart_puts(g_uart, "hc: Upstream LineDcLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(GRFLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream GrfLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -189;
        }
        uart_puts(g_uart, "hc: Upstream GrfLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MOVELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MoveLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -190;
        }
        uart_puts(g_uart, "hc: Upstream MoveLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(CHECKEDLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream CheckedLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -191;
        }
        uart_puts(g_uart, "hc: Upstream CheckedLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(CMPLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream CmpLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -192;
        }
        uart_puts(g_uart, "hc: Upstream CmpLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FORINCLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream ForIncLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -193;
        }
        uart_puts(g_uart, "hc: Upstream ForIncLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MICROLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MicroLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -194;
        }
        uart_puts(g_uart, "hc: Upstream MicroLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MOVESTACKLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MoveStackLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -195;
        }
        uart_puts(g_uart, "hc: Upstream MoveStackLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ENDLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream EndLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -196;
        }
        uart_puts(g_uart, "hc: Upstream EndLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DRAWITLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DrawItLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -197;
        }
        uart_puts(g_uart, "hc: Upstream DrawItLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(LATTICELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream LatticeLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -198;
        }
        uart_puts(g_uart, "hc: Upstream LatticeLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(LOOPLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream LoopLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -199;
        }
        uart_puts(g_uart, "hc: Upstream LoopLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DEMOLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DemoLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -200;
        }
        uart_puts(g_uart, "hc: Upstream DemoLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(EVENTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream EventLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -201;
        }
        uart_puts(g_uart, "hc: Upstream EventLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(PLAYLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream PlayLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -202;
        }
        uart_puts(g_uart, "hc: Upstream PlayLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(INPUTLITE_ZC, &got2) != 0 || got2 != 31) {
            uart_puts(g_uart, "hc: Upstream InputLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -203;
        }
        uart_puts(g_uart, "hc: Upstream InputLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RIGHTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream RightLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -205;
        }
        uart_puts(g_uart, "hc: Upstream RightLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(CURSORLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream CursorLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -206;
        }
        uart_puts(g_uart, "hc: Upstream CursorLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(UPLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream UpLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -207;
        }
        uart_puts(g_uart, "hc: Upstream UpLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TICKLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream TickLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -208;
        }
        uart_puts(g_uart, "hc: Upstream TickLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FRAMELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream FrameLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -209;
        }
        uart_puts(g_uart, "hc: Upstream FrameLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(PLOTDCLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream PlotDcLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -210;
        }
        uart_puts(g_uart, "hc: Upstream PlotDcLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ABORTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream AbortLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -211;
        }
        uart_puts(g_uart, "hc: Upstream AbortLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(AIMMOVELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream AimMoveLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -212;
        }
        uart_puts(g_uart, "hc: Upstream AimMoveLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(IDLELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream IdleLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -213;
        }
        uart_puts(g_uart, "hc: Upstream IdleLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(LAYERLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream LayerLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -214;
        }
        uart_puts(g_uart, "hc: Upstream LayerLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ENDSLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream EndsLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -215;
        }
        uart_puts(g_uart, "hc: Upstream EndsLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(SPEEDLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream SpeedLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -216;
        }
        uart_puts(g_uart, "hc: Upstream SpeedLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MIDLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MidLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -217;
        }
        uart_puts(g_uart, "hc: Upstream MidLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(LIVELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream LiveLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -218;
        }
        uart_puts(g_uart, "hc: Upstream LiveLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ACCELLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream AccelLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -219;
        }
        uart_puts(g_uart, "hc: Upstream AccelLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RESTARTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream RestartLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -220;
        }
        uart_puts(g_uart, "hc: Upstream RestartLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(WIDTHLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream WidthLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -221;
        }
        uart_puts(g_uart, "hc: Upstream WidthLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(BOTHCOLORLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream BothColorLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -222;
        }
        uart_puts(g_uart, "hc: Upstream BothColorLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MENUFULLLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MenuFullLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -223;
        }
        uart_puts(g_uart, "hc: Upstream MenuFullLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MENUBIGLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MenuBigLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -224;
        }
        uart_puts(g_uart, "hc: Upstream MenuBigLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TRYLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream TryLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -225;
        }
        uart_puts(g_uart, "hc: Upstream TryLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(STEPCOUNTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream StepCountLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -226;
        }
        uart_puts(g_uart, "hc: Upstream StepCountLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ANGLESFULLLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream AnglesFullLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -227;
        }
        uart_puts(g_uart, "hc: Upstream AnglesFullLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(BRACEANGLESLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream BraceAnglesLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -228;
        }
        uart_puts(g_uart, "hc: Upstream BraceAnglesLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(BRACEPILITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream BracePiLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -229;
        }
        uart_puts(g_uart, "hc: Upstream BracePiLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(SETMENULITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream SetMenuLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -230;
        }
        uart_puts(g_uart, "hc: Upstream SetMenuLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(NEARLATTICELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream NearLatticeLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -231;
        }
        uart_puts(g_uart, "hc: Upstream NearLatticeLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(F64IFLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream F64IfLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -232;
        }
        uart_puts(g_uart, "hc: Upstream F64IfLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(WRAPLATTICELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream WrapLatticeLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -233;
        }
        uart_puts(g_uart, "hc: Upstream WrapLatticeLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MENULOOPLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MenuLoopLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -234;
        }
        uart_puts(g_uart, "hc: Upstream MenuLoopLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(IDXALLLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream IdxAllLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -235;
        }
        uart_puts(g_uart, "hc: Upstream IdxAllLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Lattice MessageGet mask precedence: 1<<2|1<<5 == 36 (not 96). */
        if (hc_run_src("return 1<<MESSAGE_KEY_DOWN|1<<MESSAGE_MS_L_DOWN;", &got2) != 0 ||
            got2 != 36) {
            uart_puts(g_uart, "hc: Front MsgMask FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -204;
        }
        uart_puts(g_uart, "hc: Front MsgMask => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 f(I64 x){I64 y=x;I64 *p=&y;return *p;}return f(9);", &got2) != 0 ||
            got2 != 9) {
            uart_puts(g_uart, "hc: Front FramePtr FAIL\n");
            return -83;
        }
        uart_puts(g_uart, "hc: Front FramePtr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* ChatGPT UTM acceptance compiler checks (hex display). */
        if (hc_run_src("return sizeof(U32);", &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Accept sizeof(U32) FAIL\n");
            return -84;
        }
        uart_puts(g_uart, "hc: Accept sizeof(U32) => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U32 x=0xFFFFFFFF;x=x+1;return x;", &got2) != 0 || got2 != 0) {
            uart_puts(g_uart, "hc: Accept U32 wrap FAIL\n");
            return -85;
        }
        uart_puts(g_uart, "hc: Accept U32 wrap => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 f(I64 x){return x+1;}I64 a=5;return f(2)+a;", &got2) != 0 ||
            got2 != 8) {
            uart_puts(g_uart, "hc: Accept frame call FAIL\n");
            return -86;
        }
        uart_puts(g_uart, "hc: Accept frame call => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return true&&!false;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front TrueFalse FAIL\n");
            return -76;
        }
        uart_puts(g_uart, "hc: Front TrueFalse => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return NULL==0;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front NULL FAIL\n");
            return -77;
        }
        uart_puts(g_uart, "hc: Front NULL => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=7;I64 *p=&x;*p=9;return x+(p==NULL?0:1);", &got2) != 0 || got2 != 10) {
            uart_puts(g_uart, "hc: Front I64Ptr FAIL\n");
            return -78;
        }
        uart_puts(g_uart, "hc: Front I64Ptr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("const I64 k=6;return k*k;", &got2) != 0 || got2 != 36) {
            uart_puts(g_uart, "hc: Front Const FAIL\n");
            return -79;
        }
        uart_puts(g_uart, "hc: Front Const => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("volatile I64 x=3;return x+1;", &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Front Volatile FAIL\n");
            return -80;
        }
        uart_puts(g_uart, "hc: Front Volatile => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class A{I64 a;};class B{I64 b;};class C{I64 c;};class D{I64 d;};"
                       "class E{I64 e;};A a;B b;C c;D d;E e;a.a=1;b.b=2;c.c=3;d.d=4;e.e=5;"
                       "return a.a+b.b+c.c+d.d+e.e;",
                       &got2) != 0 ||
            got2 != 15) {
            uart_puts(g_uart, "hc: Front Classes8 FAIL\n");
            return -81;
        }
        uart_puts(g_uart, "hc: Front Classes8 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class V{I64 a;I64 b;I64 c;I64 d;I64 e;};V v;v.a=1;v.b=2;v.c=3;v.d=4;v.e=5;"
                       "return v.a+v.b+v.c+v.d+v.e;",
                       &got2) != 0 ||
            got2 != 15) {
            uart_puts(g_uart, "hc: Front Members5 FAIL\n");
            return -82;
        }
        uart_puts(g_uart, "hc: Front Members5 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U8 b=0;U8 *p=&b;*p=65;return b+*p;", &got2) != 0 || got2 != 130) {
            uart_puts(g_uart, "hc: Front U8Ptr FAIL\n");
            return -83;
        }
        uart_puts(g_uart, "hc: Front U8Ptr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("union U{I64 w;U8 b;};U u;u.w=0x41;return u.b+sizeof(U);", &got2) != 0 ||
            got2 != (0x41 + 8)) {
            uart_puts(g_uart, "hc: Front Union FAIL\n");
            return -84;
        }
        uart_puts(g_uart, "hc: Front Union => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("GrCircle(100,100,20,0x00FFFFFF);return 1;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front GrCircle FAIL\n");
            return -85;
        }
        uart_puts(g_uart, "hc: Front GrCircle => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("GrFillCircle(50,50,10,0x00FF0000);return 2;", &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Front GrFillCircle FAIL\n");
            return -86;
        }
        uart_puts(g_uart, "hc: Front GrFillCircle => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return CntFrq()>1000;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front CntFrq FAIL\n");
            return -87;
        }
        uart_puts(g_uart, "hc: Front CntFrq => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return FbW();", &got2) != 0 || got2 == 0) {
            uart_puts(g_uart, "hc: Front FbWH FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -88;
        }
        uart_puts(g_uart, "hc: Front FbWH => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/NetOfDots.ZC (embedded from that file at build). */
        if (hc_run_src(NETOFDOTS_ZC, &got2) != 0) {
            uart_puts(g_uart, "hc: Upstream NetOfDots FAIL\n");
            return -89;
        }
        uart_puts(g_uart, "hc: Upstream NetOfDots => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/Lines.ZC (Clamp/Sign/Rand/FbW/KeyHit loop). */
        if (hc_run_src(LINES_ZC, &got2) != 0) {
            uart_puts(g_uart, "hc: Upstream Lines FAIL\n");
            return -90;
        }
        uart_puts(g_uart, "hc: Upstream Lines => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/MiniGr.ZC (MiniGrLib lecture slice). */
        if (hc_run_src(MINIGR_ZC, &got2) != 0) {
            uart_puts(g_uart, "hc: Upstream MiniGr FAIL\n");
            return -91;
        }
        uart_puts(g_uart, "hc: Upstream MiniGr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/MemSort.ZC (RadixSort-inspired counting sort). */
        if (hc_run_src(MEMSORT_ZC, &got2) != 0 || got2 != 16) {
            uart_puts(g_uart, "hc: Upstream MemSort FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -92;
        }
        uart_puts(g_uart, "hc: Upstream MemSort => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/PeekPlot.ZC (Life-inspired GrPeek/GrPlot). */
        if (hc_run_src(PEEKPLOT_ZC, &got2) != 0 || got2 == 0) {
            uart_puts(g_uart, "hc: Upstream PeekPlot FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -95;
        }
        uart_puts(g_uart, "hc: Upstream PeekPlot => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Front MAlloc / Free + U8* index. */
        if (hc_run_src("U8 *p=MAlloc(8);if(!p)return 0;MemSet(p,3,8);"
                       "I64 s=p[0]+p[7];p[0]=9;s=s+p[0];Free(p);return s;",
                       &got2) != 0 ||
            got2 != 15) {
            uart_puts(g_uart, "hc: Front MAlloc FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -96;
        }
        uart_puts(g_uart, "hc: Front MAlloc => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/OffBmp.ZC (MAlloc offscreen + GrPlot blit). */
        if (hc_run_src(OFFBMP_ZC, &got2) != 0 || got2 == 0) {
            uart_puts(g_uart, "hc: Upstream OffBmp FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -97;
        }
        uart_puts(g_uart, "hc: Upstream OffBmp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Front StrNew / StrCpy / StrCat. */
        if (hc_run_src("U8 *a=StrNew(\"Zeal\");U8 *b=MAlloc(8);StrCpy(b,a);StrCat(b,\"OS\");"
                       "I64 n=StrLen(b)+(StrCmp(b,\"ZealOS\")==0);Free(0);return n;",
                       &got2) != 0 ||
            got2 != 7) {
            uart_puts(g_uart, "hc: Front StrNew FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -98;
        }
        uart_puts(g_uart, "hc: Front StrNew => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/HeapStr.ZC. */
        if (hc_run_src(HEAPSTR_ZC, &got2) != 0 || got2 != 6) {
            uart_puts(g_uart, "hc: Upstream HeapStr FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -99;
        }
        uart_puts(g_uart, "hc: Upstream HeapStr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Front CatPrint / MStrPrint. */
        if (hc_run_src("U8 *s=MStrPrint(\"%s-%d\",\"Zeal\",8);"
                       "CatPrint(s,\"/%s\",\"OS\",0);I64 n=StrLen(s);"
                       "I64 ok=StrCmp(s,\"Zeal-8/OS\")==0;Free(0);return n+ok;",
                       &got2) != 0 ||
            got2 != 10) {
            uart_puts(g_uart, "hc: Front CatPrint FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -100;
        }
        uart_puts(g_uart, "hc: Front CatPrint => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/CatFmt.ZC. */
        if (hc_run_src(CATFMT_ZC, &got2) != 0 || got2 != 12) {
            uart_puts(g_uart, "hc: Upstream CatFmt FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -101;
        }
        uart_puts(g_uart, "hc: Upstream CatFmt => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/HeapQue.ZC (QueueInit/Insert sum). */
        if (hc_run_src(HEAPQUE_ZC, &got2) != 0 || got2 != 150) {
            uart_puts(g_uart, "hc: Upstream HeapQue FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -102;
        }
        uart_puts(g_uart, "hc: Upstream HeapQue => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/JobQue.ZC (QueueRemove + job sum). */
        if (hc_run_src(JOBQUE_ZC, &got2) != 0 || got2 != 120) {
            uart_puts(g_uart, "hc: Upstream JobQue FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -103;
        }
        uart_puts(g_uart, "hc: Upstream JobQue => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(JOBRUN_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream JobRun FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -104;
        }
        uart_puts(g_uart, "hc: Upstream JobRun => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TASKSPAWN_ZC, &got2) != 0 || got2 != 42) {
            uart_puts(g_uart, "hc: Upstream TaskSpawn FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -105;
        }
        uart_puts(g_uart, "hc: Upstream TaskSpawn => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 inc(I64 x){return x+1;}return Call1(&inc,41);", &got2) != 0 ||
            got2 != 42) {
            uart_puts(g_uart, "hc: Front Call1 FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -106;
        }
        uart_puts(g_uart, "hc: Front Call1 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(POPUP_ZC, &got2) != 0 || got2 != 27) {
            uart_puts(g_uart, "hc: Upstream PopUp FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -107;
        }
        uart_puts(g_uart, "hc: Upstream PopUp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DOCLITE_ZC, &got2) != 0 || got2 != 7) {
            uart_puts(g_uart, "hc: Upstream DocLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -108;
        }
        uart_puts(g_uart, "hc: Upstream DocLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U8 b[8];MemSet(b,0,8);b[0]=1;b[1]=2;BlkWrite(b,1,1);"
                       "MemSet(b,0,8);BlkRead(b,1,1);return b[0]+b[1];",
                       &got2) != 0 ||
            got2 != 3) {
            uart_puts(g_uart, "hc: Front Blk FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -109;
        }
        uart_puts(g_uart, "hc: Front Blk => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RAMBLK_ZC, &got2) != 0 || got2 != 414) {
            uart_puts(g_uart, "hc: Upstream RamBlk FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -110;
        }
        uart_puts(g_uart, "hc: Upstream RamBlk => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(NAMEFILE_ZC, &got2) != 0 || got2 != 5) {
            uart_puts(g_uart, "hc: Upstream NameFile FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -111;
        }
        uart_puts(g_uart, "hc: Upstream NameFile => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DIRLOOK_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream DirLook FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -112;
        }
        uart_puts(g_uart, "hc: Upstream DirLook => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DIRDEL_ZC, &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Upstream DirDel FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -113;
        }
        uart_puts(g_uart, "hc: Upstream DirDel => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FOPEN_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream FOpen FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -114;
        }
        uart_puts(g_uart, "hc: Upstream FOpen => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FWRITE_ZC, &got2) != 0 || got2 != 3) {
            uart_puts(g_uart, "hc: Upstream FWrite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -115;
        }
        uart_puts(g_uart, "hc: Upstream FWrite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MULTIBLK_ZC, &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Upstream MultiBlk FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -116;
        }
        uart_puts(g_uart, "hc: Upstream MultiBlk => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(REDSEA_ZC, &got2) != 0 || got2 != 7) {
            uart_puts(g_uart, "hc: Upstream RedSea FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -117;
        }
        uart_puts(g_uart, "hc: Upstream RedSea => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSROOT_ZC, &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Upstream RSRoot FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -118;
        }
        uart_puts(g_uart, "hc: Upstream RSRoot => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSFILE_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSFile FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -119;
        }
        uart_puts(g_uart, "hc: Upstream RSFile => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSALLOC_ZC, &got2) != 0 || got2 != 3) {
            uart_puts(g_uart, "hc: Upstream RSAlloc FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -120;
        }
        uart_puts(g_uart, "hc: Upstream RSAlloc => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSFREE_ZC, &got2) != 0 || got2 != 3) {
            uart_puts(g_uart, "hc: Upstream RSFree FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -121;
        }
        uart_puts(g_uart, "hc: Upstream RSFree => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSMULTI_ZC, &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Upstream RSMulti FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -122;
        }
        uart_puts(g_uart, "hc: Upstream RSMulti => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSCFILE_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSCFile FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -123;
        }
        uart_puts(g_uart, "hc: Upstream RSCFile => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSCWRITE_ZC, &got2) != 0 || got2 != 3) {
            uart_puts(g_uart, "hc: Upstream RSCWrite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -124;
        }
        uart_puts(g_uart, "hc: Upstream RSCWrite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSCSEEK_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSCSeek FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -125;
        }
        uart_puts(g_uart, "hc: Upstream RSCSeek => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSCLIB_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSCLib FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -126;
        }
        uart_puts(g_uart, "hc: Upstream RSCLib => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSCPERSIST_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSCPersist FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -127;
        }
        uart_puts(g_uart, "hc: Upstream RSCPersist => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSCPERSIST_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSCPersist2 FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -128;
        }
        uart_puts(g_uart, "hc: Upstream RSCPersist2 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RUNZC_ZC, &got2) != 0 || got2 != 10) {
            uart_puts(g_uart, "hc: Upstream RunZC seed FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -129;
        }
        uart_puts(g_uart, "hc: Upstream RunZC seed => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        {
            char src[2048];
            size_t n = 0;
            if (rs_load_file("Hi.ZC", src, sizeof(src), &n) != 0) {
                uart_puts(g_uart, "hc: Upstream RunZC load FAIL\n");
                return -130;
            }
            if (hc_run_src(src, &got2) != 0 || got2 != 42) {
                uart_puts(g_uart, "hc: Upstream RunZC JIT FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -131;
            }
            uart_puts(g_uart, "hc: Upstream RunZC Hi.ZC => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        /* Rename/del while root still has free slots (catalog fills all 8). */
        if (rs_put_file("Tmp.ZC", "return 7;", 9) != 0) {
            uart_puts(g_uart, "hc: Upstream rs put Tmp FAIL\n");
            return -141;
        }
        if (rs_rename_file("tmp.zc", "Tmp2.ZC") != 0) {
            uart_puts(g_uart, "hc: Upstream rsrename FAIL\n");
            return -142;
        }
        {
            char src[64];
            size_t n = 0;
            if (rs_load_file("TMP2.zc", src, sizeof(src), &n) != 0 ||
                hc_run_src(src, &got2) != 0 || got2 != 7) {
                uart_puts(g_uart, "hc: Upstream rsrename load FAIL\n");
                return -143;
            }
            uart_puts(g_uart, "hc: Upstream rsrename ok => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        if (rs_del_file("tmp2.zc") != 0 || rs_file_exists("Tmp2.ZC", NULL)) {
            uart_puts(g_uart, "hc: Upstream rsdel FAIL\n");
            return -144;
        }
        uart_puts(g_uart, "hc: Upstream rsdel ok\n");
        /* M115/M145: Lattice compose from RedSea (ephemeral; not freeze catalog). */
        if (rs_fmt_host(128, 7) != 0) {
            uart_puts(g_uart, "hc: Upstream DiskLat fmt FAIL\n");
            return -245;
        }
        if (rs_put_file("DiskLat.ZC", DISKLAT_ZC, str_len(DISKLAT_ZC)) != 0) {
            uart_puts(g_uart, "hc: Upstream DiskLat put FAIL\n");
            return -236;
        }
        {
            size_t n = 0;
            if (rs_load_file("DiskLat.ZC", g_hc_zc_src, sizeof(g_hc_zc_src), &n) != 0) {
                uart_puts(g_uart, "hc: Upstream DiskLat load FAIL\n");
                return -237;
            }
            if (hc_run_src(g_hc_zc_src, &got2) != 0 || got2 != 15) {
                uart_puts(g_uart, "hc: Upstream DiskLat FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -238;
            }
            uart_puts(g_uart, "hc: Upstream DiskLat => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        (void)rs_del_file("DiskLat.ZC");
        /* M125: same compose under Lattice.ZC name (runzc-shaped; not freeze catalog). */
        if (rs_fmt_host(128, 7) != 0) {
            uart_puts(g_uart, "hc: Upstream Lattice.ZC fmt FAIL\n");
            return -246;
        }
        if (rs_put_file("Lattice.ZC", DISKLAT_ZC, str_len(DISKLAT_ZC)) != 0) {
            uart_puts(g_uart, "hc: Upstream Lattice.ZC put FAIL\n");
            return -241;
        }
        {
            size_t n = 0;
            if (rs_load_file("Lattice.ZC", g_hc_zc_src, sizeof(g_hc_zc_src), &n) != 0) {
                uart_puts(g_uart, "hc: Upstream Lattice.ZC load FAIL\n");
                return -242;
            }
            if (hc_run_src(g_hc_zc_src, &got2) != 0 || got2 != 15) {
                uart_puts(g_uart, "hc: Upstream Lattice.ZC FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -243;
            }
            uart_puts(g_uart, "hc: Upstream Lattice.ZC => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        (void)rs_del_file("Lattice.ZC");
        /* M155: LatticePlay = DiskLat without scripted MsgQue; ESC-only for smoke. */
        {
            if (hc_lattice_play_src(DISKLAT_ZC, g_hc_zc_src, sizeof(g_hc_zc_src), 1) != 0) {
                uart_puts(g_uart, "hc: Upstream LatticePlay build FAIL\n");
                return -247;
            }
            if (hc_run_src(g_hc_zc_src, &got2) != 0) {
                uart_puts(g_uart, "hc: Upstream LatticePlay FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -248;
            }
            uart_puts(g_uart, "hc: Upstream LatticePlay => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        /* M182: stock Demo/Graphics/Lattice.ZC shape (θ/dθ_idx glyphs; scripted Esc). */
        if (hc_run_src(STOCKLAT_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream StockLat FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -249;
        }
        uart_puts(g_uart, "hc: Upstream StockLat => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DEPTHBUFLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DepthBufLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -239;
        }
        uart_puts(g_uart, "hc: Upstream DepthBufLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DEPTHRSTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DepthRstLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -240;
        }
        uart_puts(g_uart, "hc: Upstream DepthRstLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DEPTHPLOTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DepthPlotLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -244;
        }
        uart_puts(g_uart, "hc: Upstream DepthPlotLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DEPTHLINELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DepthLineLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -245;
        }
        uart_puts(g_uart, "hc: Upstream DepthLineLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* M35–M39: catalog + #include + Notes (no NetOfDots/Lines — slot budget). */
        if (rs_seed_catalog() != 0) {
            uart_puts(g_uart, "hc: Upstream rscatalog FAIL\n");
            return -132;
        }
        {
            char src[4096];
            size_t n = 0;
            if (rs_load_file("memsort.zc", src, sizeof(src), &n) != 0 || n < 500) {
                uart_puts(g_uart, "hc: Upstream RunZC load MemSort FAIL\n");
                return -139;
            }
            if (hc_run_src(src, &got2) != 0 || got2 != 16) {
                uart_puts(g_uart, "hc: Upstream RunZC MemSort FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -140;
            }
            uart_puts(g_uart, "hc: Upstream RunZC MemSort.ZC => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        {
            char src[4096];
            size_t n = 0;
            if (rs_load_file("UseAdd.ZC", src, sizeof(src), &n) != 0) {
                uart_puts(g_uart, "hc: Upstream UseAdd load FAIL\n");
                return -145;
            }
            if (hc_run_src(src, &got2) != 0 || got2 != 42) {
                uart_puts(g_uart, "hc: Upstream UseAdd FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -146;
            }
            uart_puts(g_uart, "hc: Upstream UseAdd.ZC => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, " (#include)\n");
        }
        {
            char src[4096];
            size_t n = 0;
            if (rs_load_file("Notes.ZC", src, sizeof(src), &n) != 0) {
                uart_puts(g_uart, "hc: Upstream Notes load FAIL\n");
                return -147;
            }
            if (hc_run_src(src, &got2) != 0 || got2 != 14) {
                uart_puts(g_uart, "hc: Upstream Notes FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -148;
            }
            uart_puts(g_uart, "hc: Upstream Notes.ZC => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, " (DocLib#include)\n");
        }
        {
            int nd = rs_dir_print();
            if (nd < 3) {
                uart_puts(g_uart, "hc: Upstream rsdir FAIL n=");
                uart_put_u32_dec(g_uart, (uint32_t)(nd < 0 ? 0 : nd));
                uart_puts(g_uart, "\n");
                return -138;
            }
            uart_puts(g_uart, "hc: Upstream rsdir ok\n");
        }
    }
    {
        uint8_t bc[32];
        size_t n = 0;
        n += hc_pack_imm64(bc + n, 41);
        n += hc_pack_call1(bc + n, (uint64_t)(uintptr_t)hc_host_inc);
        bc[n++] = HC_RET;
        bc[n++] = HC_END;
        if (hc_run_bc(bc, n, &got) != 0 || got != 42) {
            uart_puts(g_uart, "hc: CALL1 FAIL\n");
            return -10;
        }
        uart_puts(g_uart, "hc: CALL1 => ");
        uart_put_u64_hex(g_uart, got);
        uart_puts(g_uart, "\n");
    }

    uart_puts(g_uart, "hc IR OK (front+host)\n");
    return 0;
}

#define HCBC_MAGIC 0x43424348u /* "HCBC" le */

static void bc_module_run(const struct zeal_handoff *h) {
    /* Always smoke SumTo(10) from a static blob first (isolates module map issues). */
    {
        static const uint8_t sum10[] = {
            HC_IMM64, 10, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 0,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 1,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 2,
            HC_LABEL, 0,
            HC_LD_LOCAL, 2, HC_LD_LOCAL, 0, HC_LT, HC_JZ, 1,
            HC_LD_LOCAL, 1, HC_LD_LOCAL, 2, HC_ADD, HC_ST_LOCAL, 1,
            HC_LD_LOCAL, 2, HC_IMM64, 1, 0, 0, 0, 0, 0, 0, 0, HC_ADD,
            HC_ST_LOCAL, 2, HC_JMP, 0,
            HC_LABEL, 1, HC_LD_LOCAL, 1, HC_RET, HC_END,
        };
        uint64_t got = 0;
        if (hc_run_bc(sum10, sizeof(sum10), &got) != 0 || got != 45) {
            uart_puts(g_uart, "bc: static SumTo FAIL\n");
            return;
        }
        uart_puts(g_uart, "bc: static SumTo OK\n");
    }

    if (!h->bc_virt || h->bc_size < 16) {
        uart_puts(g_uart, "bc: no module\n");
        return;
    }
    const uint8_t *raw = (const uint8_t *)(uintptr_t)h->bc_virt;
    uint32_t magic = (uint32_t)raw[0] | ((uint32_t)raw[1] << 8) |
                     ((uint32_t)raw[2] << 16) | ((uint32_t)raw[3] << 24);
    if (magic != HCBC_MAGIC) {
        uart_puts(g_uart, "bc: bad magic\n");
        return;
    }
    uint32_t off = (uint32_t)raw[4] | ((uint32_t)raw[5] << 8) |
                   ((uint32_t)raw[6] << 16) | ((uint32_t)raw[7] << 24);
    uint32_t len = (uint32_t)raw[8] | ((uint32_t)raw[9] << 8) |
                   ((uint32_t)raw[10] << 16) | ((uint32_t)raw[11] << 24);
    uint64_t expect = 0;
    for (int b = 0; b < 8; b++) {
        expect |= (uint64_t)raw[12 + b] << (8 * b);
    }
    uart_puts(g_uart, "bc: mod off=");
    uart_put_u32_dec(g_uart, off);
    uart_puts(g_uart, " len=");
    uart_put_u32_dec(g_uart, len);
    uart_puts(g_uart, "\n");
    if ((uint64_t)off + len > h->bc_size) {
        uart_puts(g_uart, "bc: truncated\n");
        return;
    }
    /* Copy out of module mapping before JIT (avoid execute/alias quirks). */
    uint8_t tmp[128];
    if (len > sizeof(tmp)) {
        uart_puts(g_uart, "bc: too big\n");
        return;
    }
    for (uint32_t i = 0; i < len; i++) {
        tmp[i] = raw[off + i];
    }
    uint64_t got = 0;
    if (hc_run_bc(tmp, len, &got) != 0) {
        uart_puts(g_uart, "bc: compile/run FAIL\n");
        return;
    }
    uart_puts(g_uart, "bc: module => ");
    uart_put_u64_hex(g_uart, got);
    if (expect && got != expect) {
        uart_puts(g_uart, " WANT ");
        uart_put_u64_hex(g_uart, expect);
        uart_puts(g_uart, " FAIL\n");
        return;
    }
    uart_puts(g_uart, " OK\n");
}

/* Paint a strip + title if Limine handed us a framebuffer. */
static void fb_smoke(const struct zeal_handoff *h) {
    (void)h;
    if (!g_fb) {
        uart_puts(g_uart, "fb: none\n");
        return;
    }
    uint32_t pxb = (g_fb_bpp + 7) / 8;
    uint64_t y0 = g_fb_h / 2;
    for (uint64_t x = 0; x < g_fb_w; x++) {
        volatile uint8_t *px = g_fb + y0 * g_fb_pitch + x * pxb;
        if (pxb >= 4) {
            px[0] = 0xC0;
            px[1] = 0x90;
            px[2] = 0x20;
            px[3] = 0x00;
        } else if (pxb == 3) {
            px[0] = 0xC0;
            px[1] = 0x90;
            px[2] = 0x20;
        }
    }
    con_puts("fb: ");
    uart_put_u32_dec(g_uart, (uint32_t)g_fb_w);
    con_puts("x");
    uart_put_u32_dec(g_uart, (uint32_t)g_fb_h);
    con_puts(" text OK\n");
}

void kernel_entry(const struct zeal_handoff *h) {
    if (!h || h->magic0 != ZEAL_HANDOFF_MAGIC_0 || h->magic1 != ZEAL_HANDOFF_MAGIC_1) {
        for (;;) {
            __asm__ volatile("wfi");
        }
    }

    int is_pi = (h->flags & ZEAL_FLAG_PLAT_PI4) != 0;
#ifdef ZEAL_FORCE_PI4
    is_pi = 1;
#endif

    g_hhdm = h->hhdm_offset;
    g_uart = NULL;

    /* Enable FP/SIMD at EL1 so JIT F64 ops (FADD/SCVTF/…) don't undef-insn. */
    {
        uint64_t cpacr;
        __asm__ volatile("mrs %0, cpacr_el1" : "=r"(cpacr));
        cpacr |= (3ull << 20); /* FPEN = 0b11 */
        __asm__ volatile("msr cpacr_el1, %0\n\tisb" ::"r"(cpacr));
    }

    /* Page-table pool in usable RAM (HHDM), not kernel PIE .bss. */
    {
        uint64_t pool = 0;
        for (uint32_t i = 0; i < h->mem_count; i++) {
            if (h->mem[i].type != ZEAL_MEM_USABLE || h->mem[i].length < 0x10000ull) {
                continue;
            }
            uint64_t end = h->mem[i].base + h->mem[i].length;
            uint64_t cand = (end - 0x8000ull) & ~0xfffull;
            if (cand >= h->mem[i].base + 0x1000ull) {
                pool = cand;
            }
        }
        if (pool) {
            mmio_pt_set_pool(pool);
        }
    }

#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
    if (is_pi) {
        /*
         * PI-DIAG-2: prove EL1 + PL011 TX. Magenta = entry; green = map.
         * Still no GIC / virtio / JIT.
         */
        fb_init_ex(h, 0);
        if (g_fb && g_fb_h > 140) {
            fb_fillrect(0, 120, (uint32_t)(g_fb_w > 600 ? 600 : g_fb_w), 20, 0x00C000C0u);
        }
        con_puts("KENTRY PI-DIAG-2\n");
        vectors_install();

        if (h->uart_phys) {
            /*
             * Prefer Limine's existing HHDM mapping first (no PT surgery).
             * Our Device punch was causing Address Size Faults on L2.
             */
            g_uart = (volatile uint8_t *)(h->hhdm_offset + h->uart_phys);
            if (g_fb && g_fb_h > 160) {
                fb_fillrect(0, 144, (uint32_t)(g_fb_w > 600 ? 600 : g_fb_w), 12, 0x0040C040u);
            }
        }

        if (g_uart) {
            uint32_t bw = (uint32_t)(g_fb_w > 600 ? 600 : g_fb_w);
            int fr_ok = 0, dr_ok = 0;
            /* Colors as 0x00BBGGRR (low byte = FB byte0 = Blue). */
            const uint32_t COL_OK = 0x0000E000u;   /* green */
            const uint32_t COL_BAD = 0x00E00000u;  /* red */
            const uint32_t COL_INFO = 0x00E0E000u; /* yellow */
            const uint32_t COL_DFSC = 0x00E000E0u; /* magenta */

            /* --- Probe A: FR read (32-bit) --- */
            g_uart_probe = 1;
            g_sync_recover = 1;
            g_uart_dead = 0;
            g_probe_esr = 0;
            g_probe_far = 0;
            {
                volatile uint32_t *r = (volatile uint32_t *)(uintptr_t)g_uart;
                (void)r[PL011_FR / 4];
                __asm__ volatile("dsb sy");
            }
            fr_ok = !g_uart_dead;
            g_sync_recover = 0;
            g_uart_probe = 0;
            if (g_fb && g_fb_h > 180) {
                fb_fillrect(0, 168, bw, 10, fr_ok ? COL_OK : COL_BAD);
            }

            /* If raw HHDM aborted, punch Device mapping and retry FR once. */
            if (!fr_ok && g_pt_pool_phys) {
                if (mmio_map_2m(h->hhdm_offset, h->uart_phys) == 0) {
                    g_uart = (volatile uint8_t *)(h->hhdm_offset + h->uart_phys);
                    g_uart_probe = 1;
                    g_sync_recover = 1;
                    g_uart_dead = 0;
                    g_probe_esr = 0;
                    {
                        volatile uint32_t *r = (volatile uint32_t *)(uintptr_t)g_uart;
                        (void)r[PL011_FR / 4];
                        __asm__ volatile("dsb sy");
                    }
                    fr_ok = !g_uart_dead;
                    g_sync_recover = 0;
                    g_uart_probe = 0;
                    if (g_fb && g_fb_h > 180) {
                        /* overwrite FR bar: green if Device retry worked */
                        fb_fillrect(0, 168, bw, 10, fr_ok ? COL_OK : COL_BAD);
                    }
                }
            }

            /* --- Probe B: DR byte write --- */
            g_uart_probe = 1;
            g_sync_recover = 1;
            g_uart_dead = 0;
            if (!fr_ok) {
                /* keep first ESR */
            } else {
                g_probe_esr = 0;
                g_probe_far = 0;
            }
            g_uart[PL011_DR] = (uint8_t)'!';
            __asm__ volatile("dsb sy");
            dr_ok = !g_uart_dead;
            g_sync_recover = 0;
            g_uart_probe = 0;
            if (g_fb && g_fb_h > 190) {
                fb_fillrect(0, 180, bw, 10, dr_ok ? COL_OK : COL_BAD);
            }

            if (fr_ok && dr_ok) {
                g_uart_dead = 0;
                pl011_enable_tx(g_uart);
                uart_puts(g_uart, "\nhello from EL");
                uart_write(g_uart, (char)('0' + (int)h->boot_el));
                uart_puts(g_uart, " (PI-DIAG-2)\n");
                uart_flush(g_uart);
                if (g_fb && g_fb_h > 300) {
                    fb_fillrect(0, 280, bw, 12, 0x00E0E0E0u); /* white = UART OK */
                }
                con_puts("UART TX OK\n");
            } else {
                uint32_t ec = (uint32_t)((g_probe_esr >> 26) & 0x3f);
                uint32_t dfsc = (uint32_t)(g_probe_esr & 0x3f);
                if (g_fb && g_fb_h > 310) {
                    uint32_t w = 40u + (ec * 8u);
                    if (w > bw) {
                        w = bw;
                    }
                    fb_fillrect(0, 280, w, 12, COL_INFO);
                    uint32_t w2 = 40u + (dfsc * 10u);
                    if (w2 > bw) {
                        w2 = bw;
                    }
                    fb_fillrect(0, 294, w2, 10, COL_DFSC);
                }
                con_puts("UART MMIO abort — continue soft timer\n");
            }
        } else {
            if (g_fb && g_fb_h > 180) {
                fb_fillrect(0, 168, (uint32_t)(g_fb_w > 600 ? 600 : g_fb_w), 14, 0x00E00000u);
            }
            con_puts("UART none — continue soft timer\n");
        }

        /*
         * PI-DIAG-3: soft CNTP (poll ISTATUS). No GICv2 yet — proves timer
         * regs before distributor/CPU interface bring-up.
         */
        {
            uint32_t bw = (uint32_t)(g_fb_w > 600 ? 600 : g_fb_w);
            uint64_t frq;
            uint32_t soft_ticks = 0;
            const uint32_t COL_TICK = 0x0000E000u; /* green */
            const uint32_t COL_DONE = 0x00E0E000u; /* yellow */

            con_puts("PI-DIAG-3 soft CNTP\n");
            uart_puts(g_uart, "PI-DIAG-3 soft CNTP (no GIC)\n");

            __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frq));
            if (frq == 0) {
                frq = 54000000; /* Pi4 typical fallback */
            }
            g_timer_ticks = frq / 4; /* ~4 Hz */
            uart_puts(g_uart, "CNTFRQ=");
            uart_put_u64_hex(g_uart, frq);
            uart_puts(g_uart, "\n");

            g_sync_recover = 1;
            g_sync_skipped = 0;
            timer_arm();
            {
                uint32_t trapped = g_sync_skipped;
                g_sync_recover = 0;
                if (trapped) {
                    uart_puts(g_uart, "CNTP trapped — skip\n");
                    con_puts("CNTP trapped\n");
                    if (g_fb && g_fb_h > 330) {
                        fb_fillrect(0, 310, bw, 12, 0x00E00000u);
                    }
                } else {
                    uint64_t t0;
                    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
                    while (soft_ticks < 3) {
                        uint64_t ctl, now;
                        __asm__ volatile("mrs %0, cntp_ctl_el0" : "=r"(ctl));
                        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
                        if ((ctl & 4u) != 0 && (now - t0) > (frq / 8)) {
                            soft_ticks++;
                            uart_puts(g_uart, "soft tick ");
                            uart_put_u32_dec(g_uart, soft_ticks);
                            uart_puts(g_uart, "\n");
                            if (g_fb && g_fb_h > 310 + soft_ticks * 14) {
                                fb_fillrect(0, 300 + soft_ticks * 14, bw, 10, COL_TICK);
                            }
                            timer_arm();
                            __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
                        }
                        /* Busy-wait — no IRQ/WFI until GICv2. */
                        for (volatile unsigned s = 0; s < 1000u; s++) {
                        }
                    }
                    uart_puts(g_uart, "soft CNTP OK\n");
                    con_puts("soft CNTP OK\n");
                    if (g_fb && g_fb_h > 360) {
                        fb_fillrect(0, 350, bw, 14, COL_DONE);
                    }
                    __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(0ull));

                    /* PI-DIAG-4: GICv2 + CNTP IRQ (raw HHDM). */
                    {
                        const uint32_t COL_IRQ = 0x0000E0E0u; /* cyan */
                        const uint32_t COL_GIC_BAD = 0x00E00000u;
                        int gic_ok = 0;
                        uint32_t spins = 0;

                        con_puts("PI-DIAG-4 GICv2\n");
                        uart_puts(g_uart, "PI-DIAG-4 GICv2+CNTP IRQ\n");
                        g_timer_irqs = 0;
                        if (gicv2_init(h->hhdm_offset) != 0) {
                            uart_puts(g_uart, "GICv2 map/probe FAIL\n");
                            con_puts("GICv2 FAIL\n");
                            if (g_fb && g_fb_h > 380) {
                                fb_fillrect(0, 370, bw, 12, COL_GIC_BAD);
                            }
                        } else {
                            gic_ok = 1;
                            uart_puts(g_uart, "GICv2 @ ");
                            uart_put_u64_hex(g_uart, g_gicd_phys);
                            uart_puts(g_uart, "\n");
                            if (g_fb && g_fb_h > 380) {
                                /* orange = GIC probe OK, waiting for IRQs */
                                fb_fillrect(0, 370, bw, 10, 0x000080E0u);
                            }
                            timer_arm();
                            enable_irq();
                            uart_puts(g_uart, "GIC+CNTP armed\n");
                            while (g_timer_irqs < 3 && spins < 50000000u) {
                                __asm__ volatile("wfi");
                                spins++;
                                /* Escape if IRQs never arrive — soft poll. */
                                if ((spins & 0xffffu) == 0) {
                                    uint64_t ctl;
                                    __asm__ volatile("mrs %0, cntp_ctl_el0" : "=r"(ctl));
                                    if ((ctl & 4u) && g_timer_irqs == 0) {
                                        /* Pending but no IRQ delivery */
                                        break;
                                    }
                                }
                            }
                            __asm__ volatile("msr daifset, #2");
                            __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(0ull));

                            uart_puts(g_uart, "irq ticks=");
                            uart_put_u32_dec(g_uart, g_timer_irqs);
                            uart_puts(g_uart, "\n");

                            if (g_fb && g_fb_h > 380) {
                                uint32_t n = g_timer_irqs > 3 ? 3 : g_timer_irqs;
                                for (uint32_t i = 1; i <= n; i++) {
                                    fb_fillrect(0, 370 + i * 14, bw, 10, COL_IRQ);
                                }
                                if (g_timer_irqs >= 3) {
                                    fb_fillrect(0, 420, bw, 14, 0x00E0E0E0u); /* white = IRQ OK */
                                    con_puts("GICv2 IRQ OK\n");
                                    uart_puts(g_uart, "GICv2 IRQ OK - entering shell\n");
                                    /* Stop timer IRQs before interactive input. */
                                    __asm__ volatile("msr daifset, #2");
                                    __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(0ull));
                                    __asm__ volatile("isb");
                                    shell_run();
                                    for (;;) {
                                        __asm__ volatile("wfi");
                                    }
                                } else if (gic_ok) {
                                    fb_fillrect(0, 420, bw, 14, COL_GIC_BAD);
                                    con_puts("GICv2 no IRQ - halt\n");
                                }
                            }
                        }
                    }
                }
            }
        }

        for (;;) {
            __asm__ volatile("wfi");
        }
    }
#endif

    if (h->uart_phys) {
        if (is_pi) {
            if (mmio_map_2m(h->hhdm_offset, h->uart_phys) == 0) {
                g_uart = (volatile uint8_t *)(h->hhdm_offset + h->uart_phys);
            }
        } else {
            g_uart = (volatile uint8_t *)(h->hhdm_offset + h->uart_phys);
        }
    }

    /* ---- QEMU / UTM virt path (full bring-up) ---- */
    fb_init(h);

    uart_puts(g_uart, "\n==============================\n");
    uart_puts(g_uart, "kernel_entry (module + vectors + CNTP)\n");
    uart_puts(g_uart, is_pi ? "platform: Pi4\n" : "platform: virt\n");
    uart_puts(g_uart, "zealos-aarch64-bringup / handoff OK\n");
    uart_puts(g_uart, "boot_el: ");
    if (g_uart) {
        uart_write(g_uart, (char)('0' + (int)h->boot_el));
    }
    uart_puts(g_uart, "\nHHDM: ");
    uart_put_u64_hex(g_uart, h->hhdm_offset);
    uart_puts(g_uart, "\n");

    vectors_install();
    uart_puts(g_uart, "VBAR_EL1 installed\n");

    __asm__ volatile("svc #0");

    uint64_t frq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frq));
    if (frq == 0) {
        frq = 62500000;
    }
    g_timer_ticks = frq / 4;
    uart_puts(g_uart, "CNTFRQ=");
    uart_put_u64_hex(g_uart, frq);
    uart_puts(g_uart, "  timer ~4Hz\n");

    if (!is_pi) {
        gic_init();
        {
            uint64_t sre, grp1;
            MRS_ICC(5, sre);
            MRS_ICC(7, grp1);
            uart_puts(g_uart, "ICC_SRE=");
            uart_put_u64_hex(g_uart, sre);
            uart_puts(g_uart, " IGRPEN1=");
            uart_put_u64_hex(g_uart, grp1);
            uart_puts(g_uart, "\n");
        }

        g_sync_recover = 1;
        g_sync_skipped = 0;
        timer_arm();
        uint32_t timer_trapped = g_sync_skipped;
        g_sync_recover = 0;

        uint32_t soft_ticks = 0;
        if (timer_trapped) {
            uart_puts(g_uart, "timer: CNTP trapped (UTM/HVF) — skip IRQ smoke\n");
        } else {
            enable_irq();
            uart_puts(g_uart, "GIC+CNTP armed, collecting ticks...\n");

            uint64_t t0;
            __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
            while (g_timer_irqs < 3 && soft_ticks < 3) {
                uint64_t ctl;
                __asm__ volatile("mrs %0, cntp_ctl_el0" : "=r"(ctl));
                if ((ctl & 4u) && g_timer_irqs == 0) {
                    uint64_t now;
                    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
                    if (now - t0 > frq) {
                        soft_ticks++;
                        uart_puts(g_uart, "tick (ISTATUS) ");
                        uart_put_u32_dec(g_uart, soft_ticks);
                        uart_puts(g_uart, "\n");
                        timer_arm();
                        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
                    }
                }
                if (g_timer_irqs + soft_ticks >= 3) {
                    break;
                }
                __asm__ volatile("wfi");
            }

            uart_puts(g_uart, "timer OK (irq=");
            uart_put_u32_dec(g_uart, g_timer_irqs);
            uart_puts(g_uart, " soft=");
            uart_put_u32_dec(g_uart, soft_ticks);
            uart_puts(g_uart, " svc=");
            uart_put_u32_dec(g_uart, g_svc_hits);
            uart_puts(g_uart, ")\n");

            g_sync_recover = 1;
            __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(0ull));
            g_sync_recover = 0;
            __asm__ volatile("msr daifset, #2");
        }
    }

    uart_puts(g_uart, "HolyC-IR → aarch64 JIT...\n");
    if (jit_smoke() != 0) {
        uart_puts(g_uart, "hc IR FAIL\n");
        uart_puts(g_uart, "hc IR FAIL — halted (fix run-serial / check-serial)\n");
        for (;;) {
            __asm__ volatile("wfi");
        }
    }

    (void)g_fb_guard;
    fb_smoke(h);

    if (!is_pi) {
        uart_puts(g_uart, "virtio-kbd: probe...\n");
        if (mmio_map_virtio_window(h->hhdm_offset) != 0) {
            uart_puts(g_uart, "virtio-kbd: mmio map FAIL\n");
        }
        if (virtio_kbd_init(h->hhdm_offset)) {
            uart_puts(g_uart, "virtio-kbd: OK @ ");
            uart_put_u64_hex(g_uart, virtio_kbd_mmio_phys());
            uart_puts(g_uart, "\n");
        } else {
            uart_puts(g_uart, "virtio-kbd: none (serial only)\n");
        }
        if (virtio_tablet_init(h->hhdm_offset)) {
            uart_puts(g_uart, "virtio-tablet: OK @ ");
            uart_put_u64_hex(g_uart, virtio_tablet_mmio_phys());
            uart_puts(g_uart, "\n");
        } else {
            uart_puts(g_uart, "virtio-tablet: none\n");
        }
        if (virtio_blk_init(h->hhdm_offset)) {
            uart_puts(g_uart, "virtio-blk: OK @ ");
            uart_put_u64_hex(g_uart, virtio_blk_mmio_phys());
            uart_puts(g_uart, " cap=");
            uart_put_u64_hex(g_uart, virtio_blk_capacity());
            uart_puts(g_uart, g_vb.xport == VB_XPORT_PCI ? " pci\n" : " mmio\n");
            uart_puts(g_uart, "virtio-blk: rw OK\n");
        } else {
            uart_puts(g_uart, "virtio-blk: none (RAM Blk* only) dbg=");
            uart_put_u64_hex(g_uart, (uint64_t)(uint32_t)g_vb_pci_dbg);
            uart_puts(g_uart, "\n");
        }
    }

    bc_module_run(h);
    shell_run();

    uart_puts(g_uart, "tablet clicks=");
    uart_put_u32_dec(g_uart, g_tab_clicks);
    uart_puts(g_uart, "\n");
    uart_puts(g_uart, "halting\n");
    uart_puts(g_uart, "==============================\n");

    for (;;) {
        __asm__ volatile("wfi");
    }
}
