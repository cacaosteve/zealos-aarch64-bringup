/*
 * virtio-blk: MMIO (QEMU virtio-blk-device) or PCI (UTM virtio-blk-pci).
 * UTM strips -drive from AdditionalArguments, so the boot disk's PCI blk
 * is the only persist path there — RedSea lives in the last 128 sectors.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "disk_layout.h"

#define VB_MAGIC            0x74726976u
#define VB_MMIO_BASE        0x0a000000ull
#define VB_MMIO_STRIDE      0x200ull
#define VB_MMIO_SLOTS       32
#define VB_ID_BLOCK         2u

#define VB_REG_MAGIC        0x000
#define VB_REG_VERSION      0x004
#define VB_REG_DEVICE_ID    0x008
#define VB_REG_DEV_FEAT     0x010
#define VB_REG_DEV_FEAT_SEL 0x014
#define VB_REG_DRV_FEAT     0x020
#define VB_REG_DRV_FEAT_SEL 0x024
#define VB_REG_QUEUE_SEL    0x030
#define VB_REG_QUEUE_NUM_MAX 0x034
#define VB_REG_QUEUE_NUM    0x038
#define VB_REG_GUEST_PAGE_SIZE 0x028
#define VB_REG_QUEUE_ALIGN  0x03c
#define VB_REG_QUEUE_PFN    0x040
#define VB_REG_QUEUE_READY  0x044
#define VB_REG_QUEUE_NOTIFY 0x050
#define VB_REG_STATUS       0x070
#define VB_REG_DESC_LO      0x080
#define VB_REG_DESC_HI      0x084
#define VB_REG_DRIVER_LO    0x090
#define VB_REG_DRIVER_HI    0x094
#define VB_REG_DEVICE_LO    0x0a0
#define VB_REG_DEVICE_HI    0x0a4
#define VB_REG_CONFIG       0x100

#define VB_S_ACK            1u
#define VB_S_DRIVER         2u
#define VB_S_DRIVER_OK      4u
#define VB_S_FEATURES_OK    8u

#define VB_DESC_F_NEXT      1u
#define VB_DESC_F_WRITE     2u
#define VB_T_IN             0u
#define VB_T_OUT            1u
#define VB_QSIZE            8u
#define VB_SECT             512u
#define VB_RS_SECTS         ZEAL_RS_SECTS
#define VB_RS_LBA_BASE      ZEAL_RS_LBA_BASE

#define VB_XPORT_MMIO       0
#define VB_XPORT_PCI        1

#define VIRTIO_PCI_CAP_COMMON 1
#define VIRTIO_PCI_CAP_NOTIFY 2
#define VIRTIO_PCI_CAP_DEVICE 4
#define PCI_VENDOR_VIRTIO   0x1af4u
#define PCI_DEVICE_BLK_MODERN 0x1042u
#define PCI_DEVICE_BLK_TRANS  0x1001u

struct vb_dev {
    int xport;
    volatile uint32_t *mmio;           /* MMIO transport */
    volatile uint8_t *pci_common;      /* modern common cfg */
    volatile uint8_t *pci_notify;      /* notify base */
    volatile uint8_t *pci_devcfg;      /* device cfg (capacity) */
    uint32_t notify_mult;
    uint32_t ver;
    uint8_t *desc;
    uint8_t *avail;
    uint8_t *used;
    uint8_t *hdr;
    uint8_t *data;
    uint16_t avail_idx;
    uint64_t capacity; /* sectors visible to Blk* (≤ VB_RS_SECTS) */
    uint64_t lba_base; /* added to sector for PCI boot-disk tail */
    uint64_t phys;
    int ready;
};

__attribute__((aligned(4096))) static uint8_t g_vb_dma[16384];
static struct vb_dev g_vb;
static uint64_t g_vb_hhdm;
static int g_vb_last_rw;
static int g_vb_pci_dbg;

static inline uint32_t vb_rr(volatile uint32_t *m, unsigned off) {
    return m[off / 4];
}
static inline void vb_rw(volatile uint32_t *m, unsigned off, uint32_t v) {
    m[off / 4] = v;
}
static inline void vb_dsb(void) {
    __asm__ volatile("dsb sy" ::: "memory");
}
static inline uint64_t vb_v2p(void *v) {
    return (uint64_t)(uintptr_t)v - g_vb_hhdm;
}
static inline void vb_w16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static inline void vb_w32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}
static inline void vb_w64(uint8_t *p, uint64_t v) {
    unsigned i;
    for (i = 0; i < 8; i++) {
        p[i] = (uint8_t)(v >> (8 * i));
    }
}
static inline uint16_t vb_r16(volatile uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline uint32_t vb_r32(volatile uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}
static inline uint64_t vb_r64(volatile uint8_t *p) {
    return (uint64_t)vb_r32(p) | ((uint64_t)vb_r32(p + 4) << 32);
}
static inline uint8_t vb_r8(volatile uint8_t *p) {
    return p[0];
}
/* Width-correct PCI MMIO — byte stores to Device memory break 16/32-bit regs. */
static inline void vb_pci_w8(volatile uint8_t *p, uint8_t v) {
    *p = v;
}
static inline void vb_pci_w16(volatile uint8_t *p, uint16_t v) {
    *(volatile uint16_t *)(uintptr_t)p = v;
}
static inline void vb_pci_w32(volatile uint8_t *p, uint32_t v) {
    *(volatile uint32_t *)(uintptr_t)p = v;
}
static inline void vb_pci_w64(volatile uint8_t *p, uint64_t v) {
    vb_pci_w32(p, (uint32_t)v);
    vb_pci_w32(p + 4, (uint32_t)(v >> 32));
}
static inline uint16_t vb_pci_r16(volatile uint8_t *p) {
    return *(volatile uint16_t *)(uintptr_t)p;
}
static inline uint32_t vb_pci_r32(volatile uint8_t *p) {
    return *(volatile uint32_t *)(uintptr_t)p;
}
static inline uint64_t vb_pci_r64(volatile uint8_t *p) {
    return (uint64_t)vb_pci_r32(p) | ((uint64_t)vb_pci_r32(p + 4) << 32);
}

static uint64_t vb_cfg_capacity(volatile uint32_t *m) {
    uint32_t lo = vb_rr(m, VB_REG_CONFIG);
    uint32_t hi = vb_rr(m, VB_REG_CONFIG + 4);
    return (uint64_t)lo | ((uint64_t)hi << 32);
}

static void vb_desc(uint8_t *d, uint64_t addr, uint32_t len, uint16_t flags,
                    uint16_t next) {
    vb_w64(d, addr);
    vb_w32(d + 8, len);
    vb_w16(d + 12, flags);
    vb_w16(d + 14, next);
}

static void vb_dma_layout_legacy(void) {
    uint8_t *mem = g_vb_dma;
    g_vb.desc = mem;
    g_vb.avail = mem + VB_QSIZE * 16;
    g_vb.used = mem + 4096;
    g_vb.hdr = mem + 8192;
    g_vb.data = g_vb.hdr + 32;
}

static void vb_dma_layout_modern(void) {
    uint8_t *mem = g_vb_dma;
    g_vb.desc = mem;
    g_vb.avail = mem + 4096;
    g_vb.used = mem + 8192;
    g_vb.hdr = mem + 12288;
    g_vb.data = g_vb.hdr + 32;
}

static int vb_init_mmio(volatile uint32_t *best, uint32_t best_ver, uint64_t best_phys,
                        uint64_t best_cap) {
    unsigned i;
    uint32_t qmax;
    for (i = 0; i < sizeof(g_vb_dma); i++) {
        g_vb_dma[i] = 0;
    }
    g_vb.capacity = best_cap;
    g_vb.lba_base = 0;
    g_vb.xport = VB_XPORT_MMIO;

    vb_rw(best, VB_REG_STATUS, 0);
    vb_rw(best, VB_REG_STATUS, VB_S_ACK);
    vb_rw(best, VB_REG_STATUS, VB_S_ACK | VB_S_DRIVER);
    vb_rw(best, VB_REG_QUEUE_SEL, 0);
    qmax = vb_rr(best, VB_REG_QUEUE_NUM_MAX);
    if (qmax < VB_QSIZE) {
        vb_rw(best, VB_REG_STATUS, 0);
        return 0;
    }
    vb_rw(best, VB_REG_QUEUE_NUM, VB_QSIZE);

    if (best_ver == 2) {
        uint64_t dp, ap, up;
        vb_dma_layout_modern();
        vb_rw(best, VB_REG_DEV_FEAT_SEL, 1);
        (void)vb_rr(best, VB_REG_DEV_FEAT);
        vb_rw(best, VB_REG_DRV_FEAT_SEL, 1);
        vb_rw(best, VB_REG_DRV_FEAT, 1);
        vb_rw(best, VB_REG_DRV_FEAT_SEL, 0);
        vb_rw(best, VB_REG_DRV_FEAT, 0);
        vb_rw(best, VB_REG_STATUS, VB_S_ACK | VB_S_DRIVER | VB_S_FEATURES_OK);
        if ((vb_rr(best, VB_REG_STATUS) & VB_S_FEATURES_OK) == 0) {
            vb_rw(best, VB_REG_STATUS, 0);
            return 0;
        }
        dp = vb_v2p(g_vb.desc);
        ap = vb_v2p(g_vb.avail);
        up = vb_v2p(g_vb.used);
        vb_rw(best, VB_REG_DESC_LO, (uint32_t)dp);
        vb_rw(best, VB_REG_DESC_HI, (uint32_t)(dp >> 32));
        vb_rw(best, VB_REG_DRIVER_LO, (uint32_t)ap);
        vb_rw(best, VB_REG_DRIVER_HI, (uint32_t)(ap >> 32));
        vb_rw(best, VB_REG_DEVICE_LO, (uint32_t)up);
        vb_rw(best, VB_REG_DEVICE_HI, (uint32_t)(up >> 32));
        vb_rw(best, VB_REG_QUEUE_READY, 1);
        vb_rw(best, VB_REG_STATUS,
              VB_S_ACK | VB_S_DRIVER | VB_S_FEATURES_OK | VB_S_DRIVER_OK);
    } else {
        uint64_t pfn;
        vb_dma_layout_legacy();
        vb_rw(best, VB_REG_GUEST_PAGE_SIZE, 4096);
        vb_rw(best, VB_REG_DRV_FEAT_SEL, 0);
        vb_rw(best, VB_REG_DRV_FEAT, 0);
        vb_rw(best, VB_REG_QUEUE_ALIGN, 4096);
        pfn = vb_v2p(g_vb.desc) >> 12;
        if (pfn >> 32) {
            vb_rw(best, VB_REG_STATUS, 0);
            return 0;
        }
        vb_rw(best, VB_REG_QUEUE_PFN, (uint32_t)pfn);
        vb_rw(best, VB_REG_STATUS, VB_S_ACK | VB_S_DRIVER | VB_S_DRIVER_OK);
    }

    g_vb.capacity = vb_cfg_capacity(best);
    if (g_vb.capacity < 16) {
        vb_rw(best, VB_REG_STATUS, 0);
        return 0;
    }
    if (g_vb.capacity > VB_RS_SECTS) {
        g_vb.capacity = VB_RS_SECTS;
    }
    g_vb.mmio = best;
    g_vb.ver = best_ver;
    g_vb.phys = best_phys;
    g_vb.avail_idx = 0;
    g_vb.ready = 0; /* set after rw probe in virtio_blk_init */
    return 1;
}

/* --- PCI modern virtio-blk (UTM boot disk) --- */

static volatile uint8_t *vb_ecam_fn(uint64_t hhdm, unsigned bus, unsigned dev,
                                    unsigned fn) {
    uint64_t phys =
        PCI_ECAM_PHYS + ((uint64_t)bus << 20) + ((uint64_t)dev << 15) + ((uint64_t)fn << 12);
    return (volatile uint8_t *)(uintptr_t)(hhdm + phys);
}

static uint32_t vb_pci_cfg32(volatile uint8_t *cfg, unsigned off) {
    return vb_pci_r32(cfg + off);
}
static void vb_pci_cfg_w16(volatile uint8_t *cfg, unsigned off, uint16_t v) {
    vb_pci_w16(cfg + off, v);
}
static void vb_pci_cfg_w32(volatile uint8_t *cfg, unsigned off, uint32_t v) {
    vb_pci_w32(cfg + off, v);
}

static uint64_t vb_pci_bar_addr(volatile uint8_t *cfg, unsigned bar_off) {
    uint32_t lo = vb_pci_cfg32(cfg, bar_off);
    uint64_t addr;
    if (lo & 1u) {
        return 0; /* I/O BAR */
    }
    addr = (uint64_t)(lo & ~0xfu);
    if (((lo >> 1) & 3u) == 2u) {
        uint32_t hi = vb_pci_cfg32(cfg, bar_off + 4);
        addr |= (uint64_t)hi << 32;
    }
    return addr;
}

static int vb_pci_place_bar(volatile uint8_t *cfg, unsigned bar_idx, uint32_t addr,
                            uint64_t hhdm) {
    unsigned off = 0x10u + bar_idx * 4u;
    uint32_t lo = vb_pci_cfg32(cfg, off);
    int is64;
    if (lo & 1u) {
        return -1;
    }
    is64 = (((lo >> 1) & 3u) == 2u);
    vb_pci_cfg_w32(cfg, off, 0xffffffffu);
    if (is64) {
        vb_pci_cfg_w32(cfg, off + 4, 0xffffffffu);
    }
    (void)vb_pci_cfg32(cfg, off); /* size */
    if (is64) {
        vb_pci_cfg_w32(cfg, off + 4, 0);
    }
    vb_pci_cfg_w32(cfg, off, (addr & ~0xfu) | (lo & 0xfu));
    return mmio_map_2m(hhdm, (uint64_t)addr);
}

static int vb_pci_find_caps(volatile uint8_t *cfg, uint64_t hhdm,
                            volatile uint8_t **common, volatile uint8_t **notify,
                            volatile uint8_t **devcfg, uint32_t *notify_mult) {
    uint8_t pos;
    uint16_t status = (uint16_t)(vb_pci_cfg32(cfg, 0x04) >> 16);
    uint8_t bars_seen[6];
    unsigned n_bars = 0;
    unsigned bi;

    if ((status & (1u << 4)) == 0) {
        return 0;
    }

    /* Collect BAR indices referenced by virtio caps, then place them low. */
    pos = cfg[0x34];
    while (pos && pos != 0xff) {
        if (cfg[pos] == 0x09) {
            uint8_t bar = cfg[pos + 4];
            unsigned k, found = 0;
            if (bar < 6) {
                for (k = 0; k < n_bars; k++) {
                    if (bars_seen[k] == bar) {
                        found = 1;
                        break;
                    }
                }
                if (!found && n_bars < 6) {
                    bars_seen[n_bars++] = bar;
                }
            }
        }
        pos = cfg[pos + 1];
    }
    for (bi = 0; bi < n_bars; bi++) {
        unsigned off = 0x10u + bars_seen[bi] * 4u;
        uint32_t lo = vb_pci_cfg32(cfg, off);
        uint64_t cur;
        if (lo & 1u) {
            continue; /* I/O BAR — skip */
        }
        cur = vb_pci_bar_addr(cfg, off);
        if (!cur) {
            continue;
        }
        /* Map wherever firmware/QEMU placed it (incl. highmem ECAM BARs). */
        if (mmio_map_2m(hhdm, cur & ~((1ull << 21) - 1)) != 0) {
            g_vb_pci_dbg = 0x30 + bi;
            return 0;
        }
    }

    *common = *notify = *devcfg = NULL;
    *notify_mult = 0;
    pos = cfg[0x34];
    while (pos && pos != 0xff) {
        uint8_t id = cfg[pos];
        uint8_t next = cfg[pos + 1];
        if (id == 0x09) {
            uint8_t typ = cfg[pos + 3];
            uint8_t bar = cfg[pos + 4];
            uint32_t off = vb_pci_r32(cfg + pos + 8);
            uint64_t bar_phys = vb_pci_bar_addr(cfg, 0x10 + bar * 4);
            volatile uint8_t *base;
            if (!bar_phys) {
                pos = next;
                continue;
            }
            base = (volatile uint8_t *)(uintptr_t)(hhdm + bar_phys);
            if (typ == VIRTIO_PCI_CAP_COMMON) {
                *common = base + off;
            } else if (typ == VIRTIO_PCI_CAP_NOTIFY) {
                *notify = base + off;
                if (cfg[pos + 2] >= 20) {
                    *notify_mult = vb_pci_r32(cfg + pos + 16);
                }
            } else if (typ == VIRTIO_PCI_CAP_DEVICE) {
                *devcfg = base + off;
            }
        }
        pos = next;
    }
    if (!*common) {
        g_vb_pci_dbg = 0x21;
    } else if (!*notify) {
        g_vb_pci_dbg = 0x22;
    } else if (!*devcfg) {
        g_vb_pci_dbg = 0x23;
    } else if (n_bars == 0) {
        g_vb_pci_dbg = 0x24;
    }
    return *common && *notify && *devcfg;
}

static int vb_init_pci(uint64_t hhdm) {
    unsigned dev;
    volatile uint8_t *best_cfg = NULL;
    uint16_t best_did = 0;
    unsigned seen = 0;

    g_vb_pci_dbg = 0;
    if (mmio_map_pci_ecam(hhdm) != 0) {
        g_vb_pci_dbg = 1;
        return 0;
    }
    g_vb_pci_dbg = 2;

    for (dev = 0; dev < 32; dev++) {
        volatile uint8_t *cfg = vb_ecam_fn(hhdm, 0, dev, 0);
        uint32_t id = vb_pci_cfg32(cfg, 0);
        uint16_t vend = (uint16_t)id;
        uint16_t did = (uint16_t)(id >> 16);
        if (vend == 0xffffu || vend == 0) {
            continue;
        }
        seen++;
        if (vend != PCI_VENDOR_VIRTIO) {
            continue;
        }
        if (did != PCI_DEVICE_BLK_MODERN && did != PCI_DEVICE_BLK_TRANS &&
            did != 0x1041u) {
            g_vb_pci_dbg = 0x10 | (did & 0xff);
            continue;
        }
        /* First matching virtio-blk wins (UTM has one boot disk). */
        best_cfg = cfg;
        best_did = did;
        break;
    }
    if (!best_cfg) {
        g_vb_pci_dbg = seen ? 7 : 8;
        return 0;
    }
    (void)best_did;

    {
        volatile uint8_t *common, *notify, *devcfg;
        uint32_t nmult;
        uint64_t full_cap, dp, ap, up;
        uint16_t qmax;
        unsigned i;

        vb_pci_cfg_w16(best_cfg, 0x04, 0x0006); /* mem + bus master */

        if (!vb_pci_find_caps(best_cfg, hhdm, &common, &notify, &devcfg, &nmult)) {
            return 0;
        }
        full_cap = vb_pci_r64(devcfg);
        if (full_cap < 16) {
            g_vb_pci_dbg = 5;
            return 0;
        }

        for (i = 0; i < sizeof(g_vb_dma); i++) {
            g_vb_dma[i] = 0;
        }
        vb_dma_layout_modern();

        /* Spec: after status=0, wait until readback is 0 (UEFI left device live). */
        vb_pci_w8(common + 20, 0);
        vb_dsb();
        for (i = 0; i < 100000u; i++) {
            if (vb_r8(common + 20) == 0) {
                break;
            }
        }
        if (vb_r8(common + 20) != 0) {
            g_vb_pci_dbg = 0x0b;
            return 0;
        }

        vb_pci_w8(common + 20, VB_S_ACK);
        vb_pci_w8(common + 20, VB_S_ACK | VB_S_DRIVER);
        /* Negotiate VIRTIO_F_VERSION_1 only (bit 0 of feature page 1). */
        vb_pci_w32(common + 0, 1);
        (void)vb_pci_r32(common + 4);
        vb_pci_w32(common + 8, 1);
        vb_pci_w32(common + 12, 1);
        vb_pci_w32(common + 8, 0);
        vb_pci_w32(common + 12, 0);
        vb_pci_w8(common + 20, VB_S_ACK | VB_S_DRIVER | VB_S_FEATURES_OK);
        if ((vb_r8(common + 20) & VB_S_FEATURES_OK) == 0) {
            g_vb_pci_dbg = 9;
            return 0;
        }

        /* Disable MSI-X vectors — we poll used.idx. */
        vb_pci_w16(common + 16, 0xffffu); /* msix_config = NO_VECTOR */
        vb_pci_w16(common + 22, 0);       /* queue_select = 0 */
        vb_pci_w16(common + 26, 0xffffu); /* queue_msix_vector */
        {
            uint16_t nq = vb_pci_r16(common + 18);
            qmax = vb_pci_r16(common + 24); /* max size after reset */
            if (qmax == 0) {
                /* Some QEMU builds report 0 until written; try VB_QSIZE. */
                vb_pci_w16(common + 24, VB_QSIZE);
                qmax = vb_pci_r16(common + 24);
            }
            if (qmax < 4) {
                g_vb_pci_dbg = 0x0a00 | (nq & 0xff) | ((qmax & 0xff) << 8);
                return 0;
            }
            if (qmax > VB_QSIZE) {
                qmax = VB_QSIZE;
            }
            vb_pci_w16(common + 24, qmax);
        }

        dp = vb_v2p(g_vb.desc);
        ap = vb_v2p(g_vb.avail);
        up = vb_v2p(g_vb.used);
        vb_pci_w64(common + 32, dp);
        vb_pci_w64(common + 40, ap);
        vb_pci_w64(common + 48, up);
        vb_pci_w16(common + 28, 1); /* queue_enable */
        vb_pci_w8(common + 20,
                  VB_S_ACK | VB_S_DRIVER | VB_S_FEATURES_OK | VB_S_DRIVER_OK);

        g_vb.xport = VB_XPORT_PCI;
        g_vb.pci_common = common;
        g_vb.pci_notify = notify;
        g_vb.pci_devcfg = devcfg;
        g_vb.notify_mult = nmult;
        g_vb.mmio = NULL;
        g_vb.ver = 2;
        g_vb.phys = PCI_ECAM_PHYS;
        g_vb.avail_idx = 0;
        if (full_cap > VB_RS_SECTS) {
            /* Prefer explicit layout on our 64MiB GPT image; else end-relative. */
            if (full_cap == ZEAL_DISK_SECTS) {
                g_vb.lba_base = VB_RS_LBA_BASE;
            } else if (full_cap > ZEAL_GPT_BACKUP_SECTS + VB_RS_SECTS) {
                g_vb.lba_base = full_cap - ZEAL_GPT_BACKUP_SECTS - VB_RS_SECTS + 1;
            } else {
                g_vb.lba_base = full_cap - VB_RS_SECTS;
            }
            g_vb.capacity = VB_RS_SECTS;
        } else {
            g_vb.lba_base = 0;
            g_vb.capacity = full_cap;
        }
        g_vb.ready = 0;
        g_vb_pci_dbg = 0;
    }
    return 1;
}

static int virtio_blk_read(void *buf, uint64_t sector, uint64_t count);
static int virtio_blk_write(const void *buf, uint64_t sector, uint64_t count);

static int virtio_blk_probe_rw(void) {
    uint8_t vb[512];
    unsigned i;
    for (i = 0; i < 512; i++) {
        vb[i] = 0;
    }
    vb[0] = 0xC3;
    vb[1] = 0x3C;
    if (virtio_blk_write(vb, 15, 1) != 1) {
        return 0;
    }
    for (i = 0; i < 512; i++) {
        vb[i] = 0;
    }
    if (virtio_blk_read(vb, 15, 1) != 1 || vb[0] != 0xC3 || vb[1] != 0x3C) {
        return 0;
    }
    return 1;
}

static int virtio_blk_init(uint64_t hhdm) {
    volatile uint32_t *best = NULL;
    uint32_t best_ver = 0;
    uint64_t best_phys = 0;
    uint64_t best_cap = ~0ull;
    unsigned slot;
    int ok;

    g_vb_hhdm = hhdm;
    g_vb.ready = 0;
    g_vb.capacity = 0;
    g_vb.lba_base = 0;
    g_vb.avail_idx = 0;
    g_vb.xport = VB_XPORT_MMIO;
    g_vb.mmio = NULL;
    g_vb.pci_common = NULL;

    for (slot = 0; slot < VB_MMIO_SLOTS; slot++) {
        uint64_t phys = VB_MMIO_BASE + slot * VB_MMIO_STRIDE;
        volatile uint32_t *m = (volatile uint32_t *)(uintptr_t)(hhdm + phys);
        uint32_t ver, id;
        uint64_t cap;
        if (vb_rr(m, VB_REG_MAGIC) != VB_MAGIC) {
            continue;
        }
        ver = vb_rr(m, VB_REG_VERSION);
        id = vb_rr(m, VB_REG_DEVICE_ID);
        if ((ver != 1 && ver != 2) || id != VB_ID_BLOCK) {
            continue;
        }
        cap = vb_cfg_capacity(m);
        if (cap < 16 || cap >= best_cap) {
            continue;
        }
        best = m;
        best_ver = ver;
        best_cap = cap;
        best_phys = phys;
    }
    if (best) {
        ok = vb_init_mmio(best, best_ver, best_phys, best_cap);
    } else {
        ok = vb_init_pci(hhdm);
    }
    if (!ok) {
        return 0;
    }
    /* Must prove R/W before advertising the device (Blk* would otherwise
     * steal traffic from the RAM disk and break shell demos). */
    if (!virtio_blk_probe_rw()) {
        /* Encode last R/W status: 0x40xx = timeout/status after PCI bring-up. */
        if (g_vb_pci_dbg == 0) {
            int st = g_vb_last_rw;
            if (st < 0) {
                g_vb_pci_dbg = 0x4000 | ((-st) & 0xfff);
            } else {
                g_vb_pci_dbg = 0x4f00;
            }
        }
        g_vb.ready = 0;
        g_vb.mmio = NULL;
        g_vb.pci_common = NULL;
        return 0;
    }
    g_vb.ready = 1;
    return 1;
}

static int virtio_blk_ready(void) {
    return g_vb.ready;
}

static uint64_t virtio_blk_capacity(void) {
    return g_vb.capacity;
}

static uint64_t virtio_blk_mmio_phys(void) {
    return g_vb.phys;
}

static void vb_notify(void) {
    if (g_vb.xport == VB_XPORT_PCI) {
        uint16_t noff = vb_pci_r16(g_vb.pci_common + 30); /* queue_notify_off */
        uint32_t delta = g_vb.notify_mult ? (uint32_t)noff * g_vb.notify_mult : 0;
        volatile uint16_t *n = (volatile uint16_t *)(uintptr_t)(g_vb.pci_notify + delta);
        vb_dsb();
        *n = 0; /* queue index */
        vb_dsb();
    } else {
        vb_rw(g_vb.mmio, VB_REG_QUEUE_NOTIFY, 0);
    }
}

static int virtio_blk_rw_sector(uint64_t sector, void *buf, int write) {
    struct vb_dev *d = &g_vb;
    uint8_t *hdr;
    uint16_t cur, next, ring;
    uint64_t spin;
    unsigned i;
    uint64_t host_lba;

    /* Allow probe while bringing the device up (ready not set yet). */
    if ((!d->ready && !d->mmio && !d->pci_common) || !buf || sector >= d->capacity) {
        return 0;
    }
    if (d->capacity == 0) {
        return 0;
    }
    host_lba = d->lba_base + sector;

    hdr = d->hdr;
    vb_w32(hdr, write ? VB_T_OUT : VB_T_IN);
    vb_w32(hdr + 4, 0);
    vb_w64(hdr + 8, host_lba);
    hdr[16] = 0xff;

    if (write) {
        uint8_t *src = (uint8_t *)buf;
        for (i = 0; i < VB_SECT; i++) {
            d->data[i] = src[i];
        }
    }

    vb_desc(d->desc + 0 * 16, vb_v2p(hdr), 16, VB_DESC_F_NEXT, 1);
    vb_desc(d->desc + 1 * 16, vb_v2p(d->data), VB_SECT,
            VB_DESC_F_NEXT | (write ? 0u : VB_DESC_F_WRITE), 2);
    vb_desc(d->desc + 2 * 16, vb_v2p(hdr + 16), 1, VB_DESC_F_WRITE, 0);

    cur = d->avail_idx;
    ring = (uint16_t)(cur % VB_QSIZE);
    vb_w16(d->avail + 4 + ring * 2, 0);
    vb_dsb();
    next = (uint16_t)(cur + 1);
    d->avail_idx = next;
    vb_w16(d->avail + 2, next);
    if (d->xport == VB_XPORT_PCI) {
        __asm__ volatile("dc cvac, %0" ::"r"(d->desc) : "memory");
        __asm__ volatile("dc cvac, %0" ::"r"(d->avail) : "memory");
        __asm__ volatile("dc cvac, %0" ::"r"(hdr) : "memory");
        __asm__ volatile("dc cvac, %0" ::"r"(d->data) : "memory");
    }
    vb_dsb();
    vb_notify();

    spin = 0;
    while (1) {
        if (d->xport == VB_XPORT_PCI) {
            __asm__ volatile("dc civac, %0" ::"r"(d->used) : "memory");
        }
        vb_dsb();
        if (vb_r16((volatile uint8_t *)d->used + 2) == next) {
            break;
        }
        if (++spin > 20000000ull) {
            return -1;
        }
    }
    vb_dsb();
    if (d->xport == VB_XPORT_PCI) {
        __asm__ volatile("dc civac, %0" ::"r"(hdr) : "memory");
        __asm__ volatile("dc civac, %0" ::"r"(d->data) : "memory");
        vb_dsb();
    }

    if (vb_r8((volatile uint8_t *)hdr + 16) != 0) {
        return -2 - (int)hdr[16];
    }
    if (!write) {
        uint8_t *dst = (uint8_t *)buf;
        for (i = 0; i < VB_SECT; i++) {
            dst[i] = d->data[i];
        }
    }
    return 1;
}

static int virtio_blk_read(void *buf, uint64_t sector, uint64_t count) {
    uint64_t i;
    uint8_t *p = (uint8_t *)buf;
    for (i = 0; i < count; i++) {
        g_vb_last_rw = virtio_blk_rw_sector(sector + i, p + i * VB_SECT, 0);
        if (g_vb_last_rw != 1) {
            return 0;
        }
    }
    return 1;
}

static int virtio_blk_write(const void *buf, uint64_t sector, uint64_t count) {
    uint64_t i;
    const uint8_t *p = (const uint8_t *)buf;
    for (i = 0; i < count; i++) {
        g_vb_last_rw =
            virtio_blk_rw_sector(sector + i, (void *)(uintptr_t)(p + i * VB_SECT), 1);
        if (g_vb_last_rw != 1) {
            return 0;
        }
    }
    return 1;
}

static int virtio_blk_write_dbg(const void *buf, uint64_t sector, uint64_t count) {
    return virtio_blk_write(buf, sector, count);
}
