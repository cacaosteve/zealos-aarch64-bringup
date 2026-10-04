/*
 * Map a physical MMIO window into the Limine HHDM (TTBR1) as Device memory.
 * Used for virtio-mmio @ 0x0a000000 which is not covered by RAM HHDM entries.
 *
 * This bootstrap supports a 4KiB granule, 48-bit TTBR1 regime only. Page-table
 * pages come from an explicit HHDM-backed handoff reservation.
 */
#pragma once

#include <stdint.h>

/* Physical page pool (4KiB × 8), accessed only as hhdm+phys. */
#define MMIO_PT_POOL_PAGES 8u
static uint64_t g_pt_pool_phys;
static unsigned g_pt_pool_used;
static unsigned g_pt_l1_splits;
static unsigned g_pt_l2_remaps;

/* Seed with an explicitly reserved, 4KiB-aligned physical base. */
static void mmio_pt_set_pool(uint64_t phys_base) {
    g_pt_pool_phys = (phys_base && !(phys_base & 0xfffull)) ? phys_base : 0;
    g_pt_pool_used = 0;
}

static uint64_t *pt_alloc(uint64_t hhdm) {
    uint64_t phys;
    uint64_t *p;
    unsigned i;
    if (!g_pt_pool_phys || g_pt_pool_used >= MMIO_PT_POOL_PAGES) {
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

static inline uint64_t *pt_phys_to_virt(uint64_t hhdm, uint64_t phys) {
    if (phys & 0x000f000000000000ull)
        return NULL;
    phys &= 0x0000fffffffff000ull;
    if (phys >= (1ull << 48) || hhdm > UINT64_MAX - phys)
        return NULL;
    return (uint64_t *)(uintptr_t)(hhdm + phys);
}

/* This mapper walks exactly four 4KiB-granule levels under TTBR1. */
static inline int mmio_pt_tcr_supported(uint64_t tcr) {
    unsigned t1sz = (unsigned)((tcr >> 16) & 0x3fu);
    unsigned tg1 = (unsigned)((tcr >> 30) & 3u);
    unsigned ips = (unsigned)((tcr >> 32) & 7u);
    return t1sz == 16 && tg1 == 2 && ips <= 5;
}

/* Expand one 1GiB L1 block into an ordinary 2MiB L2 block descriptor. */
static inline uint64_t mmio_split_l1_block(uint64_t l1_desc, unsigned index) {
    uint64_t phys = l1_desc & 0x0000ffffc0000000ull;
    uint64_t attrs = l1_desc & ~(0x0000ffffc0000000ull | 0x3ull |
                                  (1ull << 52));
    return (phys + ((uint64_t)index << 21)) | 0x1ull | attrs;
}

/* Return a verified Device MAIR slot, preferring nGnRE (0x04). */
static int pt_device_attr(void) {
    uint64_t mair;
    __asm__ volatile("mrs %0, mair_el1" : "=r"(mair));
    for (unsigned i = 0; i < 8; i++) {
        unsigned a = (unsigned)((mair >> (8 * i)) & 0xff);
        if (a == 0x04)
            return (int)i;
    }
    for (unsigned i = 0; i < 8; i++) {
        unsigned a = (unsigned)((mair >> (8 * i)) & 0xff);
        if (a == 0x00)
            return (int)i;
    }
    return -1;
}

/* Validate every register/layout assumption used by the TTBR1 walker. */
static int mmio_pt_validate(uint64_t hhdm, uint64_t *root_phys,
                            unsigned *device_attr) {
    uint64_t tcr, ttbr1;
    __asm__ volatile("mrs %0, tcr_el1" : "=r"(tcr));
    __asm__ volatile("mrs %0, ttbr1_el1" : "=r"(ttbr1));
    /* Four-level, 4KiB TTBR1 with 48-bit VAs and at most 48-bit PAs. */
    if (!mmio_pt_tcr_supported(tcr))
        return -10;
    if (!hhdm)
        return -11;
    if ((hhdm >> 48) != 0xffffu)
        return -11;
    *root_phys = ttbr1 & 0x0000fffffffff000ull;
    if (!*root_phys || hhdm > UINT64_MAX - *root_phys)
        return -12;
    {
        int attr = pt_device_attr();
        if (attr < 0)
            return -13;
        *device_attr = (unsigned)attr;
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
    uint64_t root_phys;
    unsigned attr;
    int valid = mmio_pt_validate(hhdm, &root_phys, &attr);
    if (valid)
        return valid;
    if (phys >= (1ull << 48) || va < hhdm)
        return -14;
    uint64_t *l0 = pt_phys_to_virt(hhdm, root_phys);
    if (!l0)
        return -15;

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
    } else if ((e0 & 3u) == 3u) {
        l1 = pt_phys_to_virt(hhdm, e0);
        if (!l1)
            return -15;
    } else {
        return -3; /* L0 blocks are not valid for this supported regime. */
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
        l2 = pt_alloc(hhdm);
        if (!l2) {
            return -2;
        }
        for (unsigned i = 0; i < 512; i++) {
            l2[i] = mmio_split_l1_block(e1, i);
        }
        g_pt_l1_splits++;
        uint64_t p = pt_v2p(hhdm, l2);
        /* Break-before-make when replacing the live 1GiB block. */
        l1[i1] = 0;
        __asm__ volatile("dsb ishst");
        __asm__ volatile("tlbi vmalle1is");
        __asm__ volatile("dsb ish\n\tisb");
        l1[i1] = p | 0x3u;
        __asm__ volatile("dsb ishst");
        __asm__ volatile("tlbi vmalle1is");
        __asm__ volatile("dsb ish\n\tisb");
    } else {
        l2 = pt_phys_to_virt(hhdm, e1);
        if (!l2)
            return -15;
    }

    /* L2 block descriptor: bits[1:0]=01, AttrIndx, AF, SH, UXN/PXN. */
    uint64_t desc = phys | 0x1ull;
    desc |= ((uint64_t)attr) << 2;
    desc |= (1ull << 10); /* AF */
    desc |= (2ull << 8);  /* OSH */
    desc |= (1ull << 54); /* UXN */
    desc |= (1ull << 53); /* PXN */
    uint64_t old = l2[i2];
    if ((old & 3u) == 3u)
        return -4; /* Do not overwrite a lower-level table descriptor. */
    if (old == desc)
        return 0;
    if (old & 1u) {
        /* Attribute changes to a valid block require break-before-make. */
        l2[i2] = 0;
        __asm__ volatile("dsb ishst");
        __asm__ volatile("tlbi vae1is, %0" ::"r"(va >> 12));
        __asm__ volatile("dsb ish\n\tisb");
        g_pt_l2_remaps++;
    }
    l2[i2] = desc;
    __asm__ volatile("dsb ishst");
    __asm__ volatile("tlbi vae1is, %0" ::"r"(va >> 12));
    __asm__ volatile("dsb ish\n\tisb");
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
