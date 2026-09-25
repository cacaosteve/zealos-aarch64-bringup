/*
 * ZealBooter aarch64: Limine protocol entry → fill handoff → load kernel module → jump.
 * No GDT / identity-unmap / 32-bit drop (those are x86 ZealBooter only).
 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "handoff.h"
#include "elf64.h"
#include "plat_pi4.h"
#include "mmio_map.h"

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_bootloader_info_request bootloader_info_request = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_dtb_request dtb_request = {
    .id = LIMINE_DTB_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_cmdline_request cmdline_request = {
    .id = LIMINE_EXECUTABLE_CMDLINE_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_stack_size_request stack_size_request = {
    .id = LIMINE_STACK_SIZE_REQUEST_ID,
    .revision = 0,
    .stack_size = 65536,
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

#define PL011_UART0_PHYS 0x09000000ULL /* QEMU virt */
#define PL011_DR         0x00
#define PL011_FR         0x18
#define PL011_FR_TXFF    (1u << 5)

static struct zeal_handoff g_handoff;

typedef void (*kernel_entry_fn)(const struct zeal_handoff *h);

static void halt(void) {
    for (;;) {
        __asm__ volatile("wfi");
    }
}

/* Bounded TX wait — unmapped/wrong UART must not hang forever. */
static void uart_write(volatile uint8_t *uart, char c) {
    unsigned spin = 0;
    if (!uart) {
        return;
    }
    while ((uart[PL011_FR] & PL011_FR_TXFF) && spin++ < 100000u) {
    }
    uart[PL011_DR] = (uint8_t)c;
}

static void uart_puts(volatile uint8_t *uart, const char *s) {
    if (!uart) {
        return;
    }
    while (*s) {
        if (*s == '\n') {
            uart_write(uart, '\r');
        }
        uart_write(uart, *s++);
    }
}

/* Visible on HDMI — call BEFORE any UART (Pi may fault on QEMU UART phys). */
static void fb_bar(uint32_t y0, uint32_t y1, uint8_t b, uint8_t g, uint8_t r) {
    if (!framebuffer_request.response ||
        framebuffer_request.response->framebuffer_count == 0) {
        return;
    }
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    volatile uint8_t *base = (volatile uint8_t *)fb->address;
    uint64_t pitch = fb->pitch;
    uint32_t bpp = (fb->bpp + 7) / 8;
    uint64_t w = fb->width;
    uint64_t y, x;
    if (!base || w < 8 || fb->height < y1 || bpp < 3 || y1 <= y0) {
        return;
    }
    for (y = y0; y < y1 && y < fb->height; y++) {
        for (x = 0; x < w && x < 600; x++) {
            volatile uint8_t *px = base + y * pitch + x * bpp;
            px[0] = b;
            px[1] = g;
            px[2] = r;
        }
    }
}

static int dtb_has_substr(const void *dtb, size_t max, const char *needle) {
    const uint8_t *p = (const uint8_t *)dtb;
    size_t nlen = 0;
    size_t i, j;
    while (needle[nlen]) {
        nlen++;
    }
    if (!p || nlen == 0 || max < nlen) {
        return 0;
    }
    if (max > 512u * 1024u) {
        max = 512u * 1024u;
    }
    for (i = 0; i + nlen <= max; i++) {
        for (j = 0; j < nlen; j++) {
            if (p[i + j] != (uint8_t)needle[j]) {
                break;
            }
        }
        if (j == nlen) {
            return 1;
        }
    }
    return 0;
}

/* Returns FDT totalsize, or 0 if no valid DTB. */
static uint32_t dtb_size(const uint8_t **out_ptr) {
    const uint8_t *p;
    uint32_t total;
    if (!dtb_request.response || !dtb_request.response->dtb_ptr) {
        return 0;
    }
    p = (const uint8_t *)dtb_request.response->dtb_ptr;
    if (p[0] != 0xd0 || p[1] != 0x0d || p[2] != 0xfe || p[3] != 0xed) {
        return 0;
    }
    total = ((uint32_t)p[4] << 24) | ((uint32_t)p[5] << 16) | ((uint32_t)p[6] << 8) |
            (uint32_t)p[7];
    if (total < 32 || total > 512u * 1024u) {
        return 0;
    }
    if (out_ptr) {
        *out_ptr = p;
    }
    return total;
}

static int cmdline_has(const char *needle) {
    const char *c;
    size_t nlen = 0;
    size_t i;
    if (!cmdline_request.response || !cmdline_request.response->cmdline || !needle) {
        return 0;
    }
    c = cmdline_request.response->cmdline;
    while (needle[nlen]) {
        nlen++;
    }
    for (i = 0; c[i]; i++) {
        size_t j = 0;
        while (j < nlen && c[i + j] && c[i + j] == needle[j]) {
            j++;
        }
        if (j == nlen) {
            return 1;
        }
    }
    return 0;
}

static int detect_pi4(void) {
#ifdef ZEAL_FORCE_PI4
    return 1;
#else
    const uint8_t *p = NULL;
    uint32_t total;
    if (cmdline_has("zeal.plat=pi4")) {
        return 1;
    }
    total = dtb_size(&p);
    if (!total) {
        return 0;
    }
    return dtb_has_substr(p, total, "raspberrypi") ||
           dtb_has_substr(p, total, "brcm,bcm2711");
#endif
}

static void uart_put_u64_hex(volatile uint8_t *uart, uint64_t v) {
    static const char hex[] = "0123456789abcdef";
    uart_puts(uart, "0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_write(uart, hex[(v >> i) & 0xf]);
    }
}

static void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

static void *memset(void *dst, int c, size_t n) {
    uint8_t *d = dst;
    while (n--) {
        *d++ = (uint8_t)c;
    }
    return dst;
}

static uint64_t current_el(void) {
    uint64_t el;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(el));
    return (el >> 2) & 3;
}

static uint32_t limine_type_to_zeal(uint64_t t) {
    switch (t) {
        case LIMINE_MEMMAP_USABLE:
            return ZEAL_MEM_USABLE;
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            /* Still live (page tables / stack / responses) during handoff. */
            return ZEAL_MEM_RECLAIM;
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            return ZEAL_MEM_ACPI;
        case LIMINE_MEMMAP_ACPI_NVS:
            return ZEAL_MEM_ACPI_NVS;
        case LIMINE_MEMMAP_BAD_MEMORY:
            return ZEAL_MEM_BAD;
        case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
            return ZEAL_MEM_MODULES;
        default:
            return ZEAL_MEM_RESERVED;
    }
}

static uint64_t align_up(uint64_t v, uint64_t a) {
    return (v + a - 1) & ~(a - 1);
}

/* Kernel image placement — only true USABLE RAM (never reclaimable). */
static uint64_t g_kernel_phys;
static uint64_t g_kernel_size;

static void *alloc_phys_hhdm(uint64_t hhdm, uint64_t size, uint64_t align) {
    size = align_up(size, align);
    if (!memmap_request.response) {
        return NULL;
    }
    uint64_t best = 0;
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
        struct limine_memmap_entry *e = memmap_request.response->entries[i];
        /* Do not place the kernel in bootloader-reclaimable memory. */
        if (e->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }
        uint64_t base = align_up(e->base, align);
        if (base < 0x400000) {
            base = align_up(0x400000, align);
        }
        if (base >= e->base + e->length) {
            continue;
        }
        if (e->base + e->length - base < size) {
            continue;
        }
        /* Prefer higher addresses so we stay clear of firmware / booter. */
        if (base + size > best + size || best == 0) {
            best = base;
        }
    }
    if (!best) {
        return NULL;
    }
    g_kernel_phys = best;
    g_kernel_size = size;
    return (void *)(uintptr_t)(hhdm + best);
}

/* Insert/reserve the loaded kernel range in the handoff memmap. */
static void handoff_reserve_kernel(struct zeal_handoff *h) {
    if (!g_kernel_phys || !g_kernel_size || h->mem_count >= ZEAL_MEM_MAX) {
        return;
    }
    uint64_t kb = g_kernel_phys;
    uint64_t ke = g_kernel_phys + g_kernel_size;
    uint32_t n = h->mem_count;
    /* Split any overlapping USABLE entry; append KERNEL for the image. */
    for (uint32_t i = 0; i < n && h->mem_count < ZEAL_MEM_MAX; i++) {
        if (h->mem[i].type != ZEAL_MEM_USABLE) {
            continue;
        }
        uint64_t b = h->mem[i].base;
        uint64_t e = b + h->mem[i].length;
        if (ke <= b || kb >= e) {
            continue;
        }
        /* Overlap: shrink/split usable around [kb, ke). */
        if (b < kb && e > ke && h->mem_count + 1 < ZEAL_MEM_MAX) {
            /* usable | kernel | usable */
            h->mem[i].length = kb - b;
            h->mem[h->mem_count].base = ke;
            h->mem[h->mem_count].length = e - ke;
            h->mem[h->mem_count].type = ZEAL_MEM_USABLE;
            h->mem_count++;
        } else if (b < kb && e > kb && e <= ke) {
            h->mem[i].length = kb - b;
        } else if (b >= kb && b < ke && e > ke) {
            h->mem[i].base = ke;
            h->mem[i].length = e - ke;
        } else if (b >= kb && e <= ke) {
            h->mem[i].type = ZEAL_MEM_KERNEL;
            h->mem[i].base = kb;
            h->mem[i].length = g_kernel_size;
            return;
        }
    }
    if (h->mem_count < ZEAL_MEM_MAX) {
        h->mem[h->mem_count].base = kb;
        h->mem[h->mem_count].length = g_kernel_size;
        h->mem[h->mem_count].type = ZEAL_MEM_KERNEL;
        h->mem_count++;
    }
}

static void icache_flush(void *addr, uint64_t size) {
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

static kernel_entry_fn load_kernel_elf(volatile uint8_t *uart, uint64_t hhdm,
                                       const void *file, uint64_t file_size) {
    if (file_size < sizeof(Elf64_Ehdr)) {
        uart_puts(uart, "kernel ELF too small\n");
        return NULL;
    }

    const Elf64_Ehdr *eh = file;
    if (eh->e_ident[0] != ELFMAG0 || eh->e_ident[1] != ELFMAG1 ||
        eh->e_ident[2] != ELFMAG2 || eh->e_ident[3] != ELFMAG3) {
        uart_puts(uart, "kernel: bad ELF magic\n");
        return NULL;
    }
    if (eh->e_machine != EM_AARCH64 ||
        (eh->e_type != ET_DYN && eh->e_type != ET_EXEC)) {
        uart_puts(uart, "kernel: need aarch64 ET_DYN/EXEC\n");
        return NULL;
    }

    const uint8_t *bytes = file;
    uint64_t image_end = 0;
    const Elf64_Phdr *ph = (const Elf64_Phdr *)(bytes + eh->e_phoff);
    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) {
            continue;
        }
        uint64_t end = ph[i].p_vaddr + ph[i].p_memsz;
        if (end > image_end) {
            image_end = end;
        }
    }
    if (image_end == 0) {
        uart_puts(uart, "kernel: no PT_LOAD\n");
        return NULL;
    }

    uint8_t *dest = alloc_phys_hhdm(hhdm, image_end, 0x10000);
    if (!dest) {
        uart_puts(uart, "kernel: no usable memory for image\n");
        return NULL;
    }
    memset(dest, 0, image_end);

    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) {
            continue;
        }
        if (ph[i].p_offset + ph[i].p_filesz > file_size) {
            uart_puts(uart, "kernel: PT_LOAD past EOF\n");
            return NULL;
        }
        memcpy(dest + ph[i].p_vaddr, bytes + ph[i].p_offset, ph[i].p_filesz);
    }

    /* Apply R_AARCH64_RELATIVE (PT_DYNAMIC or SHT_DYNAMIC — lld PIE often omits PT_DYNAMIC). */
    const Elf64_Dyn *dyn = NULL;
    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type == PT_DYNAMIC) {
            dyn = (const Elf64_Dyn *)(bytes + ph[i].p_offset);
            break;
        }
    }
    if (!dyn && eh->e_shoff && eh->e_shnum) {
        const Elf64_Shdr *sh = (const Elf64_Shdr *)(bytes + eh->e_shoff);
        for (uint16_t i = 0; i < eh->e_shnum; i++) {
            if (sh[i].sh_type == SHT_DYNAMIC) {
                dyn = (const Elf64_Dyn *)(bytes + sh[i].sh_offset);
                break;
            }
        }
    }

    uint64_t rela = 0, relasz = 0, relaent = sizeof(Elf64_Rela);
    if (dyn) {
        for (const Elf64_Dyn *d = dyn; d->d_tag != DT_NULL; d++) {
            if (d->d_tag == DT_RELA) {
                rela = d->d_un.d_ptr;
            } else if (d->d_tag == DT_RELASZ) {
                relasz = d->d_un.d_val;
            } else if (d->d_tag == DT_RELAENT) {
                relaent = d->d_un.d_val;
            }
        }
    }
    /* Fallback: SHT_RELA */
    if ((!rela || !relasz) && eh->e_shoff && eh->e_shnum) {
        const Elf64_Shdr *sh = (const Elf64_Shdr *)(bytes + eh->e_shoff);
        for (uint16_t i = 0; i < eh->e_shnum; i++) {
            if (sh[i].sh_type == SHT_RELA && sh[i].sh_size) {
                rela = sh[i].sh_addr ? sh[i].sh_addr : sh[i].sh_offset;
                /* Prefer sh_addr (vaddr in image); if zero, file offset == vaddr for our PIE layout is wrong —
                   use sh_addr from loaded image: section addr. */
                rela = sh[i].sh_addr;
                relasz = sh[i].sh_size;
                if (sh[i].sh_entsize) {
                    relaent = sh[i].sh_entsize;
                }
                break;
            }
        }
    }

    if (rela && relasz && relaent) {
        uint64_t n = relasz / relaent;
        const Elf64_Rela *r = (const Elf64_Rela *)(dest + rela);
        uart_puts(uart, "relocs=");
        uart_put_u64_hex(uart, n);
        uart_puts(uart, "\n");
        for (uint64_t i = 0; i < n; i++) {
            uint32_t type = (uint32_t)ELF64_R_TYPE(r[i].r_info);
            if (type == R_AARCH64_NONE) {
                continue;
            }
            if (type != R_AARCH64_RELATIVE) {
                uart_puts(uart, "kernel: unsupported reloc type\n");
                return NULL;
            }
            uint64_t *slot = (uint64_t *)(dest + r[i].r_offset);
            *slot = (uint64_t)(uintptr_t)dest + (uint64_t)r[i].r_addend;
        }
    }

    icache_flush(dest, image_end);

    uart_puts(uart, "kernel image @ ");
    uart_put_u64_hex(uart, (uint64_t)(uintptr_t)dest);
    uart_puts(uart, " size=");
    uart_put_u64_hex(uart, image_end);
    uart_puts(uart, " entry=");
    uart_put_u64_hex(uart, (uint64_t)(uintptr_t)(dest + eh->e_entry));
    uart_puts(uart, "\n");

    return (kernel_entry_fn)(void *)(dest + eh->e_entry);
}

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        halt();
    }

    uint64_t hhdm = hhdm_request.response ? hhdm_request.response->offset : 0;
    int is_pi = detect_pi4();
    uint64_t uart_phys = 0;
    volatile uint8_t *uart = NULL;
    int uart_mapped = 0;

#ifdef ZEAL_FORCE_PI4
    /* Pi SD diagnostic build: never fall through to QEMU UART. */
    is_pi = 1;
#endif

    if (is_pi) {
        /* FB-only until cyan: leave uart NULL so TX cannot fault-stop boot. */
        fb_bar(40, 56, 0x00, 0xE0, 0xE0); /* yellow = booter entry */
        uart_phys = PI4_UART0_PHYS;
        /* Do not punch Device PTEs in the booter — kernel probes raw HHDM first. */
        uart_mapped = 1;
        fb_bar(60, 76, 0x40, 0xC0, 0x40); /* green = Pi path (UART map deferred) */
        uart = NULL; /* keep NULL through jump; handoff still has uart_phys */
        fb_bar(200, 216, 0x00, 0x80, 0xE0); /* orange = past map, continuing */
        fb_bar(220, 236, 0xC0, 0x40, 0xC0); /* purple = enter handoff/module path */
        (void)uart_mapped;
    } else {
        uart_phys = PL011_UART0_PHYS;
        uart = (volatile uint8_t *)(uintptr_t)(hhdm + uart_phys);
        uart_puts(uart, "\nZealBooter build=PI-DIAG-1\n");
        uart_puts(uart, "platform: QEMU virt\n");
        uart_puts(uart, "------------------------------------\n");
    }

    if (bootloader_info_request.response) {
        uart_puts(uart, "bootloader: ");
        uart_puts(uart, bootloader_info_request.response->name);
        uart_puts(uart, " ");
        uart_puts(uart, bootloader_info_request.response->version);
        uart_puts(uart, "\n");
    }

    for (size_t i = 0; i < sizeof(g_handoff); i++) {
        ((volatile uint8_t *)&g_handoff)[i] = 0;
    }

    g_handoff.magic0 = ZEAL_HANDOFF_MAGIC_0;
    g_handoff.magic1 = ZEAL_HANDOFF_MAGIC_1;
    g_handoff.size = (uint32_t)sizeof(g_handoff);
    g_handoff.revision = 2;
    g_handoff.bootloader_id = ZEAL_BL_LIMINE;
    g_handoff.flags = is_pi ? ZEAL_FLAG_PLAT_PI4 : 0;
    g_handoff.hhdm_offset = hhdm;
    g_handoff.uart_phys = uart_phys;
    g_handoff.boot_el = current_el();

    if (framebuffer_request.response &&
        framebuffer_request.response->framebuffer_count > 0) {
        struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
        g_handoff.fb_addr_phys = (uint64_t)(uintptr_t)fb->address - hhdm;
        g_handoff.fb_width = fb->width;
        g_handoff.fb_height = fb->height;
        g_handoff.fb_pitch = fb->pitch;
        g_handoff.fb_bpp = fb->bpp;
    }

    uint64_t top = 0;
    uint32_t n = 0;
    if (memmap_request.response) {
        uint64_t count = memmap_request.response->entry_count;
        if (count > ZEAL_MEM_MAX) {
            count = ZEAL_MEM_MAX;
        }
        for (uint64_t i = 0; i < count; i++) {
            struct limine_memmap_entry *e = memmap_request.response->entries[i];
            g_handoff.mem[n].base = e->base;
            g_handoff.mem[n].length = e->length;
            g_handoff.mem[n].type = limine_type_to_zeal(e->type);
            if (e->base + e->length > top) {
                top = e->base + e->length;
            }
            n++;
        }
    }
    g_handoff.mem_count = n;
    g_handoff.mem_physical_top = top;

    uart_puts(uart, "handoff @ ");
    uart_put_u64_hex(uart, (uint64_t)(uintptr_t)&g_handoff);
    uart_puts(uart, "  mem_count=");
    uart_put_u64_hex(uart, n);
    uart_puts(uart, "\n");

    if (!module_request.response || module_request.response->module_count < 1) {
        if (is_pi) {
            fb_bar(240, 256, 0x00, 0x00, 0xE0); /* red low = no module */
        }
        uart_puts(uart, "ERROR: no kernel module (module_path)\n");
        halt();
    }

    struct limine_file *mod = module_request.response->modules[0];
    if (is_pi) {
        fb_bar(240, 256, 0x40, 0xC0, 0xC0); /* teal = module present */
    }
    uart_puts(uart, "module: ");
    if (mod->path) {
        uart_puts(uart, mod->path);
    } else {
        uart_puts(uart, "(no path)");
    }
    uart_puts(uart, " size=");
    uart_put_u64_hex(uart, mod->size);
    uart_puts(uart, "\n");

    if (module_request.response->module_count >= 2) {
        struct limine_file *bc = module_request.response->modules[1];
        g_handoff.bc_virt = (uint64_t)(uintptr_t)bc->address;
        g_handoff.bc_size = bc->size;
        uart_puts(uart, "bytecode module size=");
        uart_put_u64_hex(uart, bc->size);
        uart_puts(uart, "\n");
    }

    if (is_pi) {
        fb_bar(260, 276, 0x80, 0x80, 0x20); /* olive = loading ELF */
    }
    kernel_entry_fn entry = load_kernel_elf(uart, hhdm, mod->address, mod->size);
    if (!entry) {
        if (is_pi) {
            fb_bar(260, 276, 0x00, 0x00, 0xE0); /* red = ELF load fail */
        }
        uart_puts(uart, "ERROR: kernel load failed\n");
        halt();
    }

    handoff_reserve_kernel(&g_handoff);
    uart_puts(uart, "kernel phys=");
    uart_put_u64_hex(uart, g_kernel_phys);
    uart_puts(uart, " size=");
    uart_put_u64_hex(uart, g_kernel_size);
    uart_puts(uart, " mem_count=");
    uart_put_u64_hex(uart, g_handoff.mem_count);
    uart_puts(uart, "\n");

    uart_puts(uart, "jumping to kernel_entry...\n");
    if (is_pi) {
        fb_bar(80, 96, 0xE0, 0xE0, 0x20); /* cyan = jump */
    }
    entry(&g_handoff);
    if (is_pi) {
        fb_bar(100, 116, 0x00, 0x00, 0xE0); /* red = returned */
    }
    uart_puts(uart, "ERROR: kernel_entry returned\n");
    halt();
}
