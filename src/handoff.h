/*
 * Aarch64 Limine → kernel handoff.
 * Intentionally not CKernel: no GDT, E820 layout quirks, or 32-bit trampoline.
 */
#pragma once

#include <stdint.h>

#define ZEAL_HANDOFF_MAGIC_0 0x5a45414c41413234ULL /* "ZEALAA64" */
#define ZEAL_HANDOFF_MAGIC_1 0xa64b00740001ULL

#define ZEAL_BL_LIMINE 1

/* handoff.flags */
#define ZEAL_FLAG_PLAT_PI4 (1u << 0) /* BCM2711 / Pi UEFI — not QEMU virt */

#define ZEAL_MEM_USABLE   1
#define ZEAL_MEM_RESERVED 2
#define ZEAL_MEM_ACPI     3
#define ZEAL_MEM_ACPI_NVS 4
#define ZEAL_MEM_BAD      5
#define ZEAL_MEM_MODULES  7
/* Bootloader reclaimable: not free until kernel finishes handoff/boot services. */
#define ZEAL_MEM_RECLAIM  8
#define ZEAL_MEM_KERNEL   9 /* loaded kernel image (reserved for runtime) */

#define ZEAL_MEM_MAX 256

struct zeal_mem_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t pad;
};

struct zeal_handoff {
    uint64_t magic0;
    uint64_t magic1;
    uint32_t size; /* sizeof(struct zeal_handoff) */
    uint32_t revision; /* 1 */
    uint32_t bootloader_id; /* ZEAL_BL_* */
    uint32_t flags;
    uint64_t hhdm_offset;
    uint64_t uart_phys; /* PL011 DR base; 0 if unknown */
    uint64_t fb_addr_phys;
    uint64_t fb_width;
    uint64_t fb_height;
    uint64_t fb_pitch;
    uint32_t fb_bpp;
    uint32_t mem_count;
    uint64_t mem_physical_top;
    uint64_t boot_el; /* CurrentEL at handoff */
    /* revision >= 2 */
    uint64_t bc_virt; /* optional HolyC bytecode module (Limine ptr) */
    uint64_t bc_size;
    struct zeal_mem_entry mem[ZEAL_MEM_MAX];
};
