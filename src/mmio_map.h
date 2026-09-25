/*
 * Map a physical MMIO window into the Limine HHDM (TTBR1) as Device-nGnRE.
 * Used for virtio-mmio @ 0x0a000000 which is not covered by RAM HHDM entries.
 *
 * Page-table pages MUST be HHDM-backed physical RAM. Kernel PIE pointers are
 * not HHDM — never use (va - hhdm) on .bss addresses (causes Address Size
 * Faults when the garbage PA is installed as an L2 table).
 */
#pragma once

#include <stdint.h>

/* Physical page pool (4KiB × 8), accessed only as hhdm+phys. */
static uint64_t g_pt_pool_phys;
static unsigned g_pt_pool_used;

/* Seed with a 32KiB-aligned physical base (caller finds usable RAM). */
static void mmio_pt_set_pool(uint64_t phys_base) {
    g_pt_pool_phys = phys_base & ~0xfffull;
    g_pt_pool_used = 0;
}

static uint64_t *pt_alloc(uint64_t hhdm) {
    uint64_t phys;
    uint64_t *p;
    unsigned i;
    if (!g_pt_pool_phys || g_pt_pool_used >= 8) {
        return NULL;
    }
    phys = g_pt_pool_phys + (uint64_t)g_pt_pool_used * 4096ull;
    g_pt_pool_used++;
    p = (uint64_t *)(uintptr_t)(hhdm + phys);
    for (i = 0; i < 512; i++) {
        p[i] = 0;
    }
    return p;
}

static inline uint64_t pt_v2p(uint64_t hhdm, void *v) {
    return (uint64_t)(uintptr_t)v - hhdm;
}

/* Read current AttrIndx for Device: prefer MAIR attr with 0x00 / 0x04. */
static unsigned pt_device_attr(void) {
    uint64_t mair;
    __asm__ volatile("mrs %0, mair_el1" : "=r"(mair));
    for (unsigned i = 0; i < 8; i++) {
        unsigned a = (unsigned)((mair >> (8 * i)) & 0xff);
        if (a == 0x00 || a == 0x04) { /* nGnRnE or nGnRE */
            return i;
        }
    }
    return 0;
}

/*
 * Install a 2MiB block mapping for [phys, phys+2MiB) at VA hhdm+phys.
 * Returns 0 on success. Requires mmio_pt_set_pool() first if a split/alloc
 * is needed; if L2 already exists, pool may be unused.
 */
static int mmio_map_2m(uint64_t hhdm, uint64_t phys) {
    phys &= ~((1ull << 21) - 1);
    uint64_t va = hhdm + phys;

    uint64_t ttbr1, tcr;
    __asm__ volatile("mrs %0, ttbr1_el1" : "=r"(ttbr1));
    __asm__ volatile("mrs %0, tcr_el1" : "=r"(tcr));
    (void)tcr;

    uint64_t root_phys = ttbr1 & 0x0000fffffffff000ull;
    uint64_t *l0 = (uint64_t *)(uintptr_t)(hhdm + root_phys);

    unsigned i0 = (unsigned)((va >> 39) & 0x1ff);
    unsigned i1 = (unsigned)((va >> 30) & 0x1ff);
    unsigned i2 = (unsigned)((va >> 21) & 0x1ff);

    uint64_t e0 = l0[i0];
    uint64_t *l1;
    if ((e0 & 1u) == 0) {
        l1 = pt_alloc(hhdm);
        if (!l1) {
            return -1;
        }
        {
            uint64_t p = pt_v2p(hhdm, l1);
            l0[i0] = p | 0x3u;
            __asm__ volatile("dsb sy");
        }
    } else {
        l1 = (uint64_t *)(uintptr_t)(hhdm + (e0 & 0x0000fffffffff000ull));
    }
    uint64_t e1 = l1[i1];
    uint64_t *l2;
    if ((e1 & 1u) == 0) {
        l2 = pt_alloc(hhdm);
        if (!l2) {
            return -2;
        }
        uint64_t p = pt_v2p(hhdm, l2);
        l1[i1] = p | 0x3u; /* table, valid */
        __asm__ volatile("dsb sy");
    } else if ((e1 & 0x3u) == 0x1u) {
        /*
         * L1 is a 1GiB block (common HHDM). Split into an L2 table so we can
         * punch a Device 2MiB hole — MMIO through Normal memory hangs/faults TX.
         */
        uint64_t block_phys = e1 & 0x0000ffffc0000000ull;
        l2 = pt_alloc(hhdm);
        if (!l2) {
            return -2;
        }
        for (unsigned i = 0; i < 512; i++) {
            uint64_t d = (block_phys + ((uint64_t)i << 21));
            d |= 0x1ull; /* L2 block */
            d |= e1 & 0x0000000000000ffcull; /* AttrIndx, NS, AP, SH, AF, nG */
            d |= e1 & ((1ull << 53) | (1ull << 54)); /* PXN UXN */
            l2[i] = d;
        }
        {
            uint64_t p = pt_v2p(hhdm, l2);
            l1[i1] = p | 0x3u;
            __asm__ volatile("dsb sy");
            __asm__ volatile("tlbi vmalle1is");
            __asm__ volatile("dsb sy");
            __asm__ volatile("isb");
        }
    } else {
        l2 = (uint64_t *)(uintptr_t)(hhdm + (e1 & 0x0000fffffffff000ull));
    }

    unsigned attr = pt_device_attr();
    /* L2 block descriptor: bits[1:0]=01, AttrIndx, AF, SH, UXN/PXN. */
    uint64_t desc = phys | 0x1ull;
    desc |= ((uint64_t)attr) << 2;
    desc |= (1ull << 10); /* AF */
    desc |= (2ull << 8);  /* OSH */
    desc |= (1ull << 54); /* UXN */
    desc |= (1ull << 53); /* PXN */
    l2[i2] = desc;
    __asm__ volatile("dsb sy");
    __asm__ volatile("tlbi vae1is, %0" ::"r"(va >> 12));
    __asm__ volatile("dsb sy");
    __asm__ volatile("isb");
    return 0;
}

/* Map virtio-mmio window (32 × 0x200). */
static int mmio_map_virtio_window(uint64_t hhdm) {
    return mmio_map_2m(hhdm, 0x0a000000ull);
}

/* QEMU virt high ECAM (bus0); enough to scan devices. */
#define PCI_ECAM_PHYS 0x4010000000ull
static int mmio_map_pci_ecam(uint64_t hhdm) {
    return mmio_map_2m(hhdm, PCI_ECAM_PHYS);
}
