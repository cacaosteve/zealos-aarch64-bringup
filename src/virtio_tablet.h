/*
 * virtio-input tablet (ABS_X/ABS_Y) over virtio-mmio.
 * Polled; scales absolute axes into FB pixel coords.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#define VT_MAGIC            0x74726976u
#define VT_MMIO_BASE        0x0a000000ull
#define VT_MMIO_STRIDE      0x200ull
#define VT_MMIO_SLOTS       32
#define VT_ID_INPUT         18u

#define VT_REG_MAGIC        0x000
#define VT_REG_VERSION      0x004
#define VT_REG_DEVICE_ID    0x008
#define VT_REG_DEV_FEAT     0x010
#define VT_REG_DEV_FEAT_SEL 0x014
#define VT_REG_DRV_FEAT     0x020
#define VT_REG_DRV_FEAT_SEL 0x024
#define VT_REG_QUEUE_SEL    0x030
#define VT_REG_QUEUE_NUM_MAX 0x034
#define VT_REG_QUEUE_NUM    0x038
#define VT_REG_GUEST_PAGE_SIZE 0x028
#define VT_REG_QUEUE_ALIGN     0x03c
#define VT_REG_QUEUE_PFN       0x040
#define VT_REG_QUEUE_READY  0x044
#define VT_REG_QUEUE_NOTIFY 0x050
#define VT_REG_STATUS       0x070
#define VT_REG_DESC_LO      0x080
#define VT_REG_DESC_HI      0x084
#define VT_REG_DRIVER_LO    0x090
#define VT_REG_DRIVER_HI    0x094
#define VT_REG_DEVICE_LO    0x0a0
#define VT_REG_DEVICE_HI    0x0a4
#define VT_REG_CONFIG       0x100

#define VT_S_ACK            1u
#define VT_S_DRIVER         2u
#define VT_S_DRIVER_OK      4u
#define VT_S_FEATURES_OK    8u
#define VT_DESC_F_WRITE     2u

#define VT_CFG_EV_BITS      0x11u
#define VT_CFG_ABS_INFO     0x12u
#define VT_EV_ABS_T         0x03u
#define VT_EV_ABS           0x03u
#define VT_EV_KEY           0x01u
#define VT_BTN_LEFT         0x110u
#define VT_BTN_RIGHT        0x111u
#define VT_ABS_X            0x00u
#define VT_ABS_Y            0x01u
#define VT_QSIZE            8u

struct vt_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
};

struct vt_dev {
    volatile uint32_t *mmio;
    uint8_t *desc;
    uint8_t *avail;
    uint8_t *used;
    uint8_t *events;
    uint16_t last_used;
    uint16_t avail_idx;
    int32_t abs_x, abs_y;
    int32_t max_x, max_y;
    int ready;
    int moved;
    int buttons;
};

__attribute__((aligned(4096))) static uint8_t g_vt_dma[8192];
static struct vt_dev g_vt;
static uint64_t g_vt_hhdm;

static inline uint32_t vt_rr(volatile uint32_t *m, unsigned off) {
    return m[off / 4];
}
static inline void vt_rw(volatile uint32_t *m, unsigned off, uint32_t v) {
    m[off / 4] = v;
}
static inline void vt_dsb(void) {
    __asm__ volatile("dsb sy" ::: "memory");
}
static inline uint64_t vt_v2p(void *v) {
    return (uint64_t)(uintptr_t)v - g_vt_hhdm;
}

static uint8_t vt_cfg_size(volatile uint32_t *m, uint8_t select, uint8_t subsel) {
    volatile uint8_t *cfg = (volatile uint8_t *)((uintptr_t)m + VT_REG_CONFIG);
    cfg[0] = select;
    cfg[1] = subsel;
    return cfg[2];
}

static int32_t vt_abs_max(volatile uint32_t *m, uint8_t axis) {
    volatile uint8_t *cfg = (volatile uint8_t *)((uintptr_t)m + VT_REG_CONFIG);
    cfg[0] = VT_CFG_ABS_INFO;
    cfg[1] = axis;
    if (cfg[2] < 8) {
        return 0x7fff;
    }
    /* union at +8: le32 min, le32 max */
    uint32_t mx = (uint32_t)cfg[12] | ((uint32_t)cfg[13] << 8) | ((uint32_t)cfg[14] << 16) |
                  ((uint32_t)cfg[15] << 24);
    return mx ? (int32_t)mx : 0x7fff;
}

static void vt_offer(struct vt_dev *d, uint16_t id) {
    uint16_t *ring = (uint16_t *)(void *)(d->avail + 4);
    ring[d->avail_idx % VT_QSIZE] = id;
    d->avail_idx++;
    vt_dsb();
    *(volatile uint16_t *)(void *)(d->avail + 2) = d->avail_idx;
}

static void vt_notify(struct vt_dev *d) {
    vt_dsb();
    vt_rw(d->mmio, VT_REG_QUEUE_NOTIFY, 0);
}

static void vt_handle(struct vt_dev *d, struct vt_event ev) {
    if (ev.type == VT_EV_KEY) {
        if (ev.code == VT_BTN_LEFT) {
            if (ev.value) {
                d->buttons |= 1;
            } else {
                d->buttons &= ~1;
            }
        } else if (ev.code == VT_BTN_RIGHT) {
            if (ev.value) {
                d->buttons |= 2;
            } else {
                d->buttons &= ~2;
            }
        }
        return;
    }
    if (ev.type != VT_EV_ABS) {
        return;
    }
    if (ev.code == VT_ABS_X) {
        d->abs_x = (int32_t)ev.value;
        d->moved = 1;
    } else if (ev.code == VT_ABS_Y) {
        d->abs_y = (int32_t)ev.value;
        d->moved = 1;
    }
}

static void virtio_tablet_poll(void) {
    if (!g_vt.ready) {
        return;
    }
    struct vt_dev *d = &g_vt;
    uint16_t uidx = *(volatile uint16_t *)(void *)(d->used + 2);
    while (d->last_used != uidx) {
        uint32_t id =
            *(volatile uint32_t *)(void *)(d->used + 4 + (d->last_used % VT_QSIZE) * 8);
        __asm__ volatile("" ::: "memory");
        struct vt_event ev;
        uint8_t *ep = d->events + (id % VT_QSIZE) * 8;
        ev.type = (uint16_t)ep[0] | ((uint16_t)ep[1] << 8);
        ev.code = (uint16_t)ep[2] | ((uint16_t)ep[3] << 8);
        ev.value = (uint32_t)ep[4] | ((uint32_t)ep[5] << 8) | ((uint32_t)ep[6] << 16) |
                   ((uint32_t)ep[7] << 24);
        vt_handle(d, ev);
        d->last_used++;
        vt_offer(d, (uint16_t)id);
    }
    vt_notify(d);
}

static int virtio_tablet_xy(uint32_t fb_w, uint32_t fb_h, uint32_t *ox, uint32_t *oy) {
    if (!g_vt.ready || fb_w < 2 || fb_h < 2) {
        return 0;
    }
    virtio_tablet_poll();
    int32_t mx = g_vt.max_x > 0 ? g_vt.max_x : 0x7fff;
    int32_t my = g_vt.max_y > 0 ? g_vt.max_y : 0x7fff;
    uint32_t x = (uint32_t)(((int64_t)g_vt.abs_x * (int64_t)(fb_w - 1)) / (int64_t)mx);
    uint32_t y = (uint32_t)(((int64_t)g_vt.abs_y * (int64_t)(fb_h - 1)) / (int64_t)my);
    if (x >= fb_w) {
        x = fb_w - 1;
    }
    if (y >= fb_h) {
        y = fb_h - 1;
    }
    *ox = x;
    *oy = y;
    int moved = g_vt.moved;
    g_vt.moved = 0;
    return moved || 1;
}

static uint64_t virtio_tablet_mmio_phys(void) {
    if (!g_vt.ready || !g_vt.mmio) {
        return 0;
    }
    return (uint64_t)(uintptr_t)g_vt.mmio - g_vt_hhdm;
}

static int virtio_tablet_init(uint64_t hhdm) {
    g_vt_hhdm = hhdm;
    g_vt.ready = 0;
    g_vt.abs_x = g_vt.abs_y = 0;
    g_vt.moved = 0;

    volatile uint32_t *found = NULL;
    uint32_t found_ver = 0;
    for (unsigned slot = 0; slot < VT_MMIO_SLOTS; slot++) {
        uint64_t phys = VT_MMIO_BASE + slot * VT_MMIO_STRIDE;
        volatile uint32_t *m = (volatile uint32_t *)(uintptr_t)(hhdm + phys);
        if (vt_rr(m, VT_REG_MAGIC) != VT_MAGIC) {
            continue;
        }
        uint32_t ver = vt_rr(m, VT_REG_VERSION);
        uint32_t id = vt_rr(m, VT_REG_DEVICE_ID);
        if ((ver != 1 && ver != 2) || id != VT_ID_INPUT) {
            continue;
        }
        /* Prefer ABS tablet/pointer; skip pure keyboards. */
        if (vt_cfg_size(m, VT_CFG_EV_BITS, VT_EV_ABS_T) == 0) {
            continue;
        }
        found = m;
        found_ver = ver;
    }
    if (!found) {
        return 0;
    }

    uint8_t *mem = g_vt_dma;
    for (unsigned i = 0; i < sizeof(g_vt_dma); i++) {
        mem[i] = 0;
    }

    vt_rw(found, VT_REG_STATUS, 0);
    vt_rw(found, VT_REG_STATUS, VT_S_ACK);
    vt_rw(found, VT_REG_STATUS, VT_S_ACK | VT_S_DRIVER);

    g_vt.max_x = vt_abs_max(found, VT_ABS_X);
    g_vt.max_y = vt_abs_max(found, VT_ABS_Y);

    vt_rw(found, VT_REG_QUEUE_SEL, 0);
    if (vt_rr(found, VT_REG_QUEUE_NUM_MAX) == 0) {
        vt_rw(found, VT_REG_STATUS, 0);
        return 0;
    }
    vt_rw(found, VT_REG_QUEUE_NUM, VT_QSIZE);

    uint8_t *desc;
    uint8_t *avail;
    uint8_t *used;
    uint8_t *events;

    if (found_ver == 2) {
        desc = mem;
        avail = mem + VT_QSIZE * 16;
        uintptr_t used_u = ((uintptr_t)avail + 6 + VT_QSIZE * 2 + 15) & ~(uintptr_t)15;
        used = (uint8_t *)used_u;
        uintptr_t ev_u = ((uintptr_t)used + 6 + VT_QSIZE * 8 + 15) & ~(uintptr_t)15;
        events = (uint8_t *)ev_u;
        if (events + VT_QSIZE * 8 > mem + sizeof(g_vt_dma)) {
            return 0;
        }

        vt_rw(found, VT_REG_DEV_FEAT_SEL, 1);
        (void)vt_rr(found, VT_REG_DEV_FEAT);
        vt_rw(found, VT_REG_DRV_FEAT_SEL, 1);
        vt_rw(found, VT_REG_DRV_FEAT, 1);
        vt_rw(found, VT_REG_DRV_FEAT_SEL, 0);
        vt_rw(found, VT_REG_DRV_FEAT, 0);
        vt_rw(found, VT_REG_STATUS, VT_S_ACK | VT_S_DRIVER | VT_S_FEATURES_OK);
        if ((vt_rr(found, VT_REG_STATUS) & VT_S_FEATURES_OK) == 0) {
            vt_rw(found, VT_REG_STATUS, 0);
            return 0;
        }

        for (unsigned i = 0; i < VT_QSIZE; i++) {
            uint64_t addr = vt_v2p(events + i * 8);
            uint8_t *d = desc + i * 16;
            for (int b = 0; b < 8; b++) {
                d[b] = (uint8_t)(addr >> (8 * b));
            }
            d[8] = 8;
            d[9] = d[10] = d[11] = 0;
            d[12] = (uint8_t)VT_DESC_F_WRITE;
            d[13] = d[14] = d[15] = 0;
        }

        uint64_t dp = vt_v2p(desc), ap = vt_v2p(avail), up = vt_v2p(used);
        vt_rw(found, VT_REG_DESC_LO, (uint32_t)dp);
        vt_rw(found, VT_REG_DESC_HI, (uint32_t)(dp >> 32));
        vt_rw(found, VT_REG_DRIVER_LO, (uint32_t)ap);
        vt_rw(found, VT_REG_DRIVER_HI, (uint32_t)(ap >> 32));
        vt_rw(found, VT_REG_DEVICE_LO, (uint32_t)up);
        vt_rw(found, VT_REG_DEVICE_HI, (uint32_t)(up >> 32));
        vt_rw(found, VT_REG_QUEUE_READY, 1);
        vt_rw(found, VT_REG_STATUS,
              VT_S_ACK | VT_S_DRIVER | VT_S_FEATURES_OK | VT_S_DRIVER_OK);
    } else {
        vt_rw(found, VT_REG_DRV_FEAT_SEL, 0);
        vt_rw(found, VT_REG_DRV_FEAT, 0);
        vt_rw(found, VT_REG_GUEST_PAGE_SIZE, 4096);
        vt_rw(found, VT_REG_QUEUE_ALIGN, 4096);

        unsigned n = VT_QSIZE;
        unsigned avail_off = n * 16;
        unsigned used_off = (avail_off + (6 + n * 2) + 4095) & ~4095u;
        unsigned ev_off = used_off + 6 + n * 8;
        ev_off = (ev_off + 15) & ~15u;
        if (ev_off + n * 8 > sizeof(g_vt_dma)) {
            return 0;
        }
        desc = mem;
        avail = mem + avail_off;
        used = mem + used_off;
        events = mem + ev_off;

        for (unsigned i = 0; i < n; i++) {
            uint64_t addr = vt_v2p(events + i * 8);
            uint8_t *d = desc + i * 16;
            for (int b = 0; b < 8; b++) {
                d[b] = (uint8_t)(addr >> (8 * b));
            }
            d[8] = 8;
            d[9] = d[10] = d[11] = 0;
            d[12] = (uint8_t)VT_DESC_F_WRITE;
            d[13] = d[14] = d[15] = 0;
        }

        uint64_t region_phys = vt_v2p(mem);
        vt_rw(found, VT_REG_QUEUE_PFN, (uint32_t)(region_phys >> 12));
        vt_rw(found, VT_REG_STATUS, VT_S_ACK | VT_S_DRIVER | VT_S_DRIVER_OK);
    }

    g_vt.mmio = found;
    g_vt.desc = desc;
    g_vt.avail = avail;
    g_vt.used = used;
    g_vt.events = events;
    g_vt.last_used = 0;
    g_vt.avail_idx = 0;

    for (uint16_t i = 0; i < VT_QSIZE; i++) {
        vt_offer(&g_vt, i);
    }
    vt_notify(&g_vt);
    g_vt.ready = 1;
    return 1;
}

static int virtio_tablet_ready(void) {
    return g_vt.ready;
}

static int virtio_tablet_buttons(void) {
    if (!g_vt.ready) {
        return 0;
    }
    virtio_tablet_poll();
    return g_vt.buttons;
}
