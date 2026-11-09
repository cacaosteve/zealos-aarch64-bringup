/*
 * Minimal modern VirtIO PCI network transport for AArch64.
 *
 * This is deliberately a transport layer only: it exposes Ethernet frames,
 * leaving IP/DHCP/TCP and the stock ZealOS network stack in source space.
 * Legacy PCI I/O BARs are not used; the target has no x86 port-I/O address
 * space. The driver negotiates only MAC and VERSION_1 and polls split rings.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "mmio_map.h"

#define VN_VENDOR                 0x1af4u
#define VN_DEVICE_MODERN          0x1041u
#define VN_QUEUE_COUNT            2u
#define VN_QUEUE_SIZE             8u
#define VN_FRAME_MAX              1518u
/* QEMU's VERSION_1 path uses virtio_net_hdr_mrg_rxbuf (12 bytes), even when
 * mergeable RX buffers are not negotiated. The final two bytes are
 * num_buffers and are ignored for TX and stripped from RX with the header. */
#define VN_HDR_SIZE               12u
#define VN_STATUS_ACK             1u
#define VN_STATUS_DRIVER          2u
#define VN_STATUS_DRIVER_OK       4u
#define VN_STATUS_FEATURES_OK     8u
#define VN_F_MAC                  (1u << 5)
#define VN_F_VERSION_1            1u
#define VN_PCI_CAP_COMMON         1u
#define VN_PCI_CAP_NOTIFY         2u
#define VN_PCI_CAP_DEVICE         4u

struct vn_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};

struct vn_used_elem {
    uint32_t id;
    uint32_t len;
};

struct vn_avail {
    volatile uint16_t flags;
    volatile uint16_t idx;
    volatile uint16_t ring[VN_QUEUE_SIZE];
    volatile uint16_t used_event;
};

struct vn_used {
    volatile uint16_t flags;
    volatile uint16_t idx;
    volatile struct vn_used_elem ring[VN_QUEUE_SIZE];
    volatile uint16_t avail_event;
};

struct vn_queue {
    struct vn_desc *desc;
    struct vn_avail *avail;
    struct vn_used *used;
    volatile uint8_t *notify;
    uint16_t notify_off;
    uint16_t size;
    uint16_t last_used;
};

struct vn_pci_caps {
    volatile uint8_t *common;
    volatile uint8_t *notify;
    volatile uint8_t *device;
    uint32_t notify_mult;
};

/* Each ring object is naturally page-aligned for simple DMA/cache handling. */
__attribute__((aligned(4096))) static struct vn_desc g_vn_desc[VN_QUEUE_COUNT][VN_QUEUE_SIZE];
__attribute__((aligned(4096))) static struct vn_avail g_vn_avail[VN_QUEUE_COUNT];
__attribute__((aligned(4096))) static struct vn_used g_vn_used[VN_QUEUE_COUNT];
__attribute__((aligned(4096))) static uint8_t g_vn_rx[VN_QUEUE_SIZE][VN_HDR_SIZE + VN_FRAME_MAX];
__attribute__((aligned(4096))) static uint8_t g_vn_tx[VN_QUEUE_SIZE][VN_HDR_SIZE + VN_FRAME_MAX];
static struct vn_queue g_vn_q[VN_QUEUE_COUNT];
static uint8_t g_vn_mac[6];
static uint64_t g_vn_hhdm;
static int g_vn_ready;
static int g_vn_debug;

static inline void vn_dsb(void) {
    __asm__ volatile("dsb sy" ::: "memory");
}

static void vn_cache_range(void *ptr, size_t size, int invalidate) {
    uint64_t ctr, line;
    uintptr_t start, end;
    if (!ptr || !size) return;
    __asm__ volatile("mrs %0, ctr_el0" : "=r"(ctr));
    line = 4ull << ((ctr >> 16) & 15u);
    if (line < 16 || line > 2048 || (line & (line - 1))) line = 64;
    start = (uintptr_t)ptr & ~(uintptr_t)(line - 1);
    end = (uintptr_t)ptr + size;
    if (end < (uintptr_t)ptr) return;
    for (uintptr_t p = start; p < end; p += (uintptr_t)line) {
        if (invalidate)
            __asm__ volatile("dc civac, %0" :: "r"(p) : "memory");
        else
            __asm__ volatile("dc cvac, %0" :: "r"(p) : "memory");
    }
    vn_dsb();
}

static inline uint64_t vn_phys(const void *p) {
    return (uint64_t)(uintptr_t)p - g_vn_hhdm;
}

static volatile uint8_t *vn_ecam_fn(unsigned bus, unsigned dev, unsigned fn) {
    uint64_t phys = PCI_ECAM_PHYS + ((uint64_t)bus << 20) +
                    ((uint64_t)dev << 15) + ((uint64_t)fn << 12);
    return (volatile uint8_t *)(uintptr_t)(g_vn_hhdm + phys);
}

static inline uint8_t vn_r8(volatile uint8_t *p) { return *p; }
static inline uint16_t vn_r16(volatile uint8_t *p) {
    return *(volatile uint16_t *)(uintptr_t)p;
}
static inline uint32_t vn_r32(volatile uint8_t *p) {
    return *(volatile uint32_t *)(uintptr_t)p;
}
static inline void vn_w8(volatile uint8_t *p, uint8_t v) { *p = v; }
static inline void vn_w16(volatile uint8_t *p, uint16_t v) {
    *(volatile uint16_t *)(uintptr_t)p = v;
}
static inline void vn_w32(volatile uint8_t *p, uint32_t v) {
    *(volatile uint32_t *)(uintptr_t)p = v;
}
static inline void vn_w64(volatile uint8_t *p, uint64_t v) {
    vn_w32(p, (uint32_t)v);
    vn_w32(p + 4, (uint32_t)(v >> 32));
}

static uint64_t vn_bar(volatile uint8_t *cfg, unsigned off) {
    uint32_t lo = vn_r32(cfg + off);
    uint64_t addr;
    if (lo & 1u) return 0; /* I/O BARs are intentionally unsupported. */
    addr = lo & ~0xfull;
    if (((lo >> 1) & 3u) == 2u)
        addr |= (uint64_t)vn_r32(cfg + off + 4) << 32;
    return addr;
}

static int vn_caps(volatile uint8_t *cfg, struct vn_pci_caps *caps) {
    uint16_t status = (uint16_t)(vn_r32(cfg + 4) >> 16);
    uint8_t pos;
    unsigned steps = 0;
    if (!(status & (1u << 4))) return 0;
    caps->common = caps->notify = caps->device = NULL;
    caps->notify_mult = 0;
    pos = cfg[0x34] & 0xfcu;
    while (pos >= 0x40 && steps++ < 48) {
        uint8_t next = cfg[pos + 1] & 0xfcu;
        if (cfg[pos] == 0x09) {
            uint8_t len = cfg[pos + 2];
            uint8_t type = cfg[pos + 3];
            uint8_t bar = cfg[pos + 4];
            uint32_t off = vn_r32(cfg + pos + 8);
            uint64_t bar_phys = bar < 6 ? vn_bar(cfg, 0x10u + bar * 4u) : 0;
            volatile uint8_t *base;
            if (len < 16 || !bar_phys) { pos = next; continue; }
            if (mmio_map_2m(g_vn_hhdm, bar_phys & ~((1ull << 21) - 1)) != 0) {
                g_vn_debug = 0x20 | (bar & 0xf);
                return 0;
            }
            base = (volatile uint8_t *)(uintptr_t)(g_vn_hhdm + bar_phys);
            if (type == VN_PCI_CAP_COMMON) caps->common = base + off;
            else if (type == VN_PCI_CAP_NOTIFY) {
                caps->notify = base + off;
                if (len >= 20) caps->notify_mult = vn_r32(cfg + pos + 16);
            } else if (type == VN_PCI_CAP_DEVICE) caps->device = base + off;
        }
        pos = next;
    }
    return caps->common && caps->notify && caps->device;
}

static int vn_queue_setup(struct vn_pci_caps *caps, unsigned qid) {
    volatile uint8_t *c = caps->common;
    uint16_t n, notify_off;
    struct vn_queue *q = &g_vn_q[qid];
    vn_w16(c + 22, (uint16_t)qid);
    n = vn_r16(c + 24);
    if (n < 2) return 0;
    if (n > VN_QUEUE_SIZE) n = VN_QUEUE_SIZE;
    q->size = n;
    vn_w16(c + 24, n);
    notify_off = vn_r16(c + 30);
    q->notify_off = notify_off;
    q->notify = caps->notify + (uint64_t)notify_off * caps->notify_mult;
    q->desc = g_vn_desc[qid];
    q->avail = &g_vn_avail[qid];
    q->used = &g_vn_used[qid];
    q->last_used = 0;
    vn_w16(c + 16, 0xffffu); /* polling, no MSI-X */
    vn_w16(c + 26, 0xffffu);
    vn_w64(c + 32, vn_phys(q->desc));
    vn_w64(c + 40, vn_phys(q->avail));
    vn_w64(c + 48, vn_phys(q->used));
    vn_dsb();
    vn_w16(c + 28, 1);
    return 1;
}

static int virtio_net_init(uint64_t hhdm) {
    volatile uint8_t *cfg = NULL;
    struct vn_pci_caps caps;
    unsigned dev;
    uint32_t feature0, feature1;
    uint8_t pos;

    g_vn_ready = 0;
    g_vn_debug = 0;
    g_vn_hhdm = hhdm;
    if (mmio_map_pci_ecam(hhdm) != 0) {
        g_vn_debug = 1;
        return 0;
    }
    for (dev = 0; dev < 32; dev++) {
        volatile uint8_t *candidate = vn_ecam_fn(0, dev, 0);
        uint32_t id = vn_r32(candidate);
        if ((uint16_t)id == VN_VENDOR && (uint16_t)(id >> 16) == VN_DEVICE_MODERN) {
            cfg = candidate;
            break;
        }
    }
    if (!cfg) {
        g_vn_debug = 2; /* no modern VirtIO-net function on bus zero */
        return 0;
    }
    vn_w16(cfg + 4, 0x0006u); /* memory decode and bus mastering */
    if (!vn_caps(cfg, &caps)) {
        if (!g_vn_debug) g_vn_debug = 3;
        return 0;
    }

    /* Reset and negotiate exactly MAC + VERSION_1. */
    vn_w8(caps.common + 20, 0);
    vn_dsb();
    for (unsigned i = 0; i < 100000u && vn_r8(caps.common + 20); i++) { }
    if (vn_r8(caps.common + 20)) { g_vn_debug = 4; return 0; }
    vn_w8(caps.common + 20, VN_STATUS_ACK);
    vn_w8(caps.common + 20, VN_STATUS_ACK | VN_STATUS_DRIVER);
    vn_w32(caps.common + 0, 0);
    feature0 = vn_r32(caps.common + 4);
    vn_w32(caps.common + 0, 1);
    feature1 = vn_r32(caps.common + 4);
    if (!(feature0 & VN_F_MAC) || !(feature1 & VN_F_VERSION_1)) {
        vn_w8(caps.common + 20, VN_STATUS_ACK | VN_STATUS_DRIVER | 0x80);
        g_vn_debug = 5;
        return 0;
    }
    vn_w32(caps.common + 8, 0);
    vn_w32(caps.common + 12, VN_F_MAC);
    vn_w32(caps.common + 8, 1);
    vn_w32(caps.common + 12, VN_F_VERSION_1);
    vn_w8(caps.common + 20, VN_STATUS_ACK | VN_STATUS_DRIVER | VN_STATUS_FEATURES_OK);
    if (!(vn_r8(caps.common + 20) & VN_STATUS_FEATURES_OK)) {
        g_vn_debug = 6;
        return 0;
    }

    for (unsigned q = 0; q < VN_QUEUE_COUNT; q++) {
        for (unsigned i = 0; i < VN_QUEUE_SIZE; i++) {
            g_vn_desc[q][i].addr = 0;
            g_vn_desc[q][i].len = 0;
            g_vn_desc[q][i].flags = 0;
            g_vn_desc[q][i].next = 0;
        }
        g_vn_avail[q].flags = 0;
        g_vn_avail[q].idx = 0;
        g_vn_used[q].flags = 0;
        g_vn_used[q].idx = 0;
        g_vn_q[q].last_used = 0;
        vn_w16(caps.common + 22, (uint16_t)q);
        vn_w16(caps.common + 26, 0xffffu);
    }
    if (!vn_queue_setup(&caps, 0) || !vn_queue_setup(&caps, 1)) {
        g_vn_debug = 7;
        return 0;
    }

    /* Post receive buffers; device writes virtio_net_hdr then Ethernet data. */
    for (unsigned i = 0; i < VN_QUEUE_SIZE; i++) {
        struct vn_desc *d = &g_vn_desc[0][i];
        d->addr = vn_phys(g_vn_rx[i]);
        d->len = VN_HDR_SIZE + VN_FRAME_MAX;
        d->flags = 2; /* VRING_DESC_F_WRITE */
        d->next = 0;
        g_vn_avail[0].ring[i] = (uint16_t)i;
    }
    g_vn_avail[0].idx = VN_QUEUE_SIZE;
    for (unsigned i = 0; i < VN_QUEUE_COUNT; i++) {
        vn_cache_range(g_vn_desc[i], sizeof(g_vn_desc[i]), 0);
        vn_cache_range(&g_vn_avail[i], sizeof(g_vn_avail[i]), 0);
        vn_cache_range(&g_vn_used[i], sizeof(g_vn_used[i]), 1);
    }
    vn_dsb();
    vn_w8(caps.common + 20,
          VN_STATUS_ACK | VN_STATUS_DRIVER | VN_STATUS_FEATURES_OK | VN_STATUS_DRIVER_OK);
    if ((vn_r8(caps.common + 20) & VN_STATUS_DRIVER_OK) == 0) {
        g_vn_debug = 8;
        return 0;
    }
    vn_w16(g_vn_q[0].notify, 0);

    /* Device config capability carries the negotiated MAC at offset zero. */
    pos = 0;
    while (pos < sizeof(g_vn_mac)) {
        g_vn_mac[pos] = vn_r8(caps.device + pos);
        pos++;
    }
    g_vn_ready = 1;
    return 1;
}

static int virtio_net_ready(void) { return g_vn_ready; }
static int virtio_net_debug(void) { return g_vn_debug; }
static int virtio_net_mac(uint8_t out[6]) {
    if (!g_vn_ready || !out) return 0;
    for (unsigned i = 0; i < 6; i++) out[i] = g_vn_mac[i];
    return 1;
}

/* Copy one Ethernet frame to a free TX descriptor; returns bytes or 0 if full. */
static int virtio_net_send(const void *frame, unsigned len) {
    struct vn_queue *q = &g_vn_q[1];
    uint16_t used, idx, slot;
    if (!g_vn_ready || !frame || len < 14 || len > VN_FRAME_MAX) return -1;
    vn_cache_range(q->used, sizeof(*q->used), 1);
    used = q->used->idx;
    idx = q->avail->idx;
    if ((uint16_t)(idx - used) >= q->size) return 0;
    slot = idx % q->size;
    for (unsigned i = 0; i < VN_HDR_SIZE; i++) g_vn_tx[slot][i] = 0;
    for (unsigned i = 0; i < len; i++) g_vn_tx[slot][VN_HDR_SIZE + i] = ((const uint8_t *)frame)[i];
    q->desc[slot].addr = vn_phys(g_vn_tx[slot]);
    q->desc[slot].len = VN_HDR_SIZE + len;
    q->desc[slot].flags = 0;
    q->desc[slot].next = 0;
    q->avail->ring[slot] = slot;
    vn_cache_range(g_vn_tx[slot], VN_HDR_SIZE + len, 0);
    vn_cache_range(q->desc, sizeof(*q->desc) * q->size, 0);
    vn_cache_range(q->avail, sizeof(*q->avail), 0);
    vn_dsb();
    q->avail->idx = (uint16_t)(idx + 1);
    vn_cache_range(q->avail, sizeof(*q->avail), 0);
    vn_dsb();
    vn_w16(q->notify, 1);
    return (int)len;
}

/* Poll and copy one received Ethernet frame; 0 means no frame is queued. */
static int virtio_net_recv(void *out, unsigned cap) {
    struct vn_queue *q = &g_vn_q[0];
    uint16_t used_idx;
    struct vn_used_elem elem;
    unsigned slot, n;
    if (!g_vn_ready || !out || cap == 0) return -1;
    vn_cache_range(q->used, sizeof(*q->used), 1);
    used_idx = q->used->idx;
    if (q->last_used == used_idx) return 0;
    elem.id = q->used->ring[q->last_used % q->size].id;
    elem.len = q->used->ring[q->last_used % q->size].len;
    q->last_used++;
    if (elem.id >= q->size || elem.len < VN_HDR_SIZE || elem.len > VN_HDR_SIZE + VN_FRAME_MAX) {
        g_vn_debug = 9;
        return -1;
    }
    slot = elem.id;
    vn_cache_range(g_vn_rx[slot], elem.len, 1);
    n = elem.len - VN_HDR_SIZE;
    if (n > cap) n = cap;
    for (unsigned i = 0; i < n; i++) ((uint8_t *)out)[i] = g_vn_rx[slot][VN_HDR_SIZE + i];
    q->desc[slot].addr = vn_phys(g_vn_rx[slot]);
    q->desc[slot].len = VN_HDR_SIZE + VN_FRAME_MAX;
    q->desc[slot].flags = 2;
    q->desc[slot].next = 0;
    q->avail->ring[q->avail->idx % q->size] = (uint16_t)slot;
    q->avail->idx++;
    vn_cache_range(q->desc, sizeof(*q->desc) * q->size, 0);
    vn_cache_range(q->avail, sizeof(*q->avail), 0);
    vn_dsb();
    vn_w16(q->notify, 0);
    return (int)n;
}
