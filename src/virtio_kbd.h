/*
 * virtio-input keyboard over virtio-mmio (QEMU virt @ 0x0a000000).
 * Polled; maps Linux KEY_* → ASCII (shell GetKey) and MessageGet
 * (ASCII a1=ch / cursor a1=0,a2=SC_CURSOR_*) alongside UART.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#define VK_MAGIC            0x74726976u /* "virt" */
#define VK_MMIO_BASE        0x0a000000ull
#define VK_MMIO_STRIDE      0x200ull
#define VK_MMIO_SLOTS       32
#define VK_ID_INPUT         18u

#define VK_REG_MAGIC        0x000
#define VK_REG_VERSION      0x004
#define VK_REG_DEVICE_ID    0x008
#define VK_REG_DEV_FEAT     0x010
#define VK_REG_DEV_FEAT_SEL 0x014
#define VK_REG_DRV_FEAT     0x020
#define VK_REG_DRV_FEAT_SEL 0x024
#define VK_REG_QUEUE_SEL    0x030
#define VK_REG_QUEUE_NUM_MAX 0x034
#define VK_REG_QUEUE_NUM    0x038
#define VK_REG_GUEST_PAGE_SIZE 0x028 /* legacy */
#define VK_REG_QUEUE_ALIGN     0x03c /* legacy */
#define VK_REG_QUEUE_PFN       0x040 /* legacy */
#define VK_REG_QUEUE_READY  0x044
#define VK_REG_QUEUE_NOTIFY 0x050
#define VK_REG_STATUS       0x070
#define VK_REG_DESC_LO      0x080
#define VK_REG_DESC_HI      0x084
#define VK_REG_DRIVER_LO    0x090
#define VK_REG_DRIVER_HI    0x094
#define VK_REG_DEVICE_LO    0x0a0
#define VK_REG_DEVICE_HI    0x0a4
#define VK_REG_CONFIG       0x100

#define VK_S_ACK            1u
#define VK_S_DRIVER         2u
#define VK_S_DRIVER_OK      4u
#define VK_S_FEATURES_OK    8u
#define VK_DESC_F_WRITE     2u

#define VK_CFG_EV_BITS      0x11u
#define VK_EV_KEY_T         0x01u
#define VK_EV_REL_T         0x02u
#define VK_EV_ABS_T         0x03u

#define VK_EV_KEY           0x01u
#define VK_QSIZE            8u

#define VK_KEY_LEFTSHIFT    42u
#define VK_KEY_RIGHTSHIFT   54u
#define VK_KEY_LEFTCTRL     29u
#define VK_KEY_RIGHTCTRL    97u
/* Linux KEY_* for Lattice MessageGet scancode path (arg1==0). */
#define VK_KEY_UP           103u
#define VK_KEY_LEFT         105u
#define VK_KEY_RIGHT        106u
#define VK_KEY_DOWN         108u
/* TempleOS / KernelA.HH SC_CURSOR_* (arg2.u8[0]). */
#define VK_SC_CURSOR_UP     0x48u
#define VK_SC_CURSOR_LEFT   0x4Bu
#define VK_SC_CURSOR_RIGHT  0x4Du
#define VK_SC_CURSOR_DOWN   0x50u

struct vk_msg {
    uint8_t type; /* MESSAGE_KEY_DOWN=2 or MESSAGE_KEY_UP=3 */
    uint64_t a1;
    uint64_t a2;
};

struct vk_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
};

struct vk_dev {
    volatile uint32_t *mmio;
    uint8_t *desc;
    uint8_t *avail;
    uint8_t *used;
    uint8_t *events;
    uint16_t last_used;
    uint16_t avail_idx;
    uint8_t shift;
    uint8_t ctrl;
    int ready;
};

__attribute__((aligned(4096))) static uint8_t g_vk_dma[8192];
static struct vk_dev g_vk;
static uint8_t g_vk_ring[64];
static unsigned g_vk_rh, g_vk_rt;
static struct vk_msg g_vk_msg[32];
static unsigned g_vk_mh, g_vk_mt;
static uint64_t g_vk_hhdm;

static inline uint32_t vk_rr(volatile uint32_t *m, unsigned off) {
    return m[off / 4];
}
static inline void vk_rw(volatile uint32_t *m, unsigned off, uint32_t v) {
    m[off / 4] = v;
}
static inline void vk_dsb(void) {
    __asm__ volatile("dsb sy" ::: "memory");
}

static inline uint64_t vk_v2p(void *v) {
    return (uint64_t)(uintptr_t)v - g_vk_hhdm;
}

static uint8_t vk_cfg_size(volatile uint32_t *m, uint8_t select, uint8_t subsel) {
    volatile uint8_t *cfg = (volatile uint8_t *)((uintptr_t)m + VK_REG_CONFIG);
    cfg[0] = select;
    cfg[1] = subsel;
    return cfg[2];
}

static void vk_ring_push(uint8_t b) {
    unsigned n = (g_vk_rh + 1) % sizeof(g_vk_ring);
    if (n != g_vk_rt) {
        g_vk_ring[g_vk_rh] = b;
        g_vk_rh = n;
    }
}

static int vk_ring_pop(void) {
    if (g_vk_rh == g_vk_rt) {
        return -1;
    }
    int b = g_vk_ring[g_vk_rt];
    g_vk_rt = (g_vk_rt + 1) % sizeof(g_vk_ring);
    return b;
}

static void vk_msg_push(uint8_t type, uint64_t a1, uint64_t a2) {
    unsigned n = (g_vk_mh + 1) % (sizeof(g_vk_msg) / sizeof(g_vk_msg[0]));
    if (n != g_vk_mt) {
        g_vk_msg[g_vk_mh].type = type;
        g_vk_msg[g_vk_mh].a1 = a1;
        g_vk_msg[g_vk_mh].a2 = a2;
        g_vk_mh = n;
    }
}

static int vk_msg_pop(uint8_t *type, uint64_t *a1, uint64_t *a2) {
    if (g_vk_mh == g_vk_mt) {
        return 0;
    }
    *type = g_vk_msg[g_vk_mt].type;
    *a1 = g_vk_msg[g_vk_mt].a1;
    *a2 = g_vk_msg[g_vk_mt].a2;
    g_vk_mt = (g_vk_mt + 1) % (sizeof(g_vk_msg) / sizeof(g_vk_msg[0]));
    return 1;
}

static int vk_linux_to_sc(uint16_t code) {
    if (code == VK_KEY_UP) {
        return (int)VK_SC_CURSOR_UP;
    }
    if (code == VK_KEY_DOWN) {
        return (int)VK_SC_CURSOR_DOWN;
    }
    if (code == VK_KEY_LEFT) {
        return (int)VK_SC_CURSOR_LEFT;
    }
    if (code == VK_KEY_RIGHT) {
        return (int)VK_SC_CURSOR_RIGHT;
    }
    return -1;
}

static int vk_keycode_ascii(uint16_t code, int shift, int ctrl) {
    struct {
        uint16_t c;
        uint8_t base, sh;
    } map[] = {
        {1, 0x1b, 0x1C}, /* Esc / CH_SHIFT_ESC (Lattice Abort) */
        {2, '1', '!'},   {3, '2', '@'},   {4, '3', '#'},   {5, '4', '$'},   {6, '5', '%'},
        {7, '6', '^'},   {8, '7', '&'},   {9, '8', '*'},   {10, '9', '('},  {11, '0', ')'},
        {12, '-', '_'},  {13, '=', '+'},  {14, 0x08, 0x08}, {15, '\t', '\t'},
        {16, 'q', 'Q'},  {17, 'w', 'W'},  {18, 'e', 'E'},  {19, 'r', 'R'},  {20, 't', 'T'},
        {21, 'y', 'Y'},  {22, 'u', 'U'},  {23, 'i', 'I'},  {24, 'o', 'O'},  {25, 'p', 'P'},
        {26, '[', '{'},  {27, ']', '}'},  {28, '\n', '\n'}, /* Enter → Lattice Restart('\n') */
        {30, 'a', 'A'},  {31, 's', 'S'},  {32, 'd', 'D'},  {33, 'f', 'F'},  {34, 'g', 'G'},
        {35, 'h', 'H'},  {36, 'j', 'J'},  {37, 'k', 'K'},  {38, 'l', 'L'},  {39, ';', ':'},
        {40, '\'', '"'}, {41, '`', '~'},  {43, '\\', '|'},
        {44, 'z', 'Z'},  {45, 'x', 'X'},  {46, 'c', 'C'},  {47, 'v', 'V'},  {48, 'b', 'B'},
        {49, 'n', 'N'},  {50, 'm', 'M'},  {51, ',', '<'},  {52, '.', '>'},  {53, '/', '?'},
        {57, ' ', ' '},
    };
    for (unsigned i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
        if (map[i].c == code) {
            uint8_t ch = shift ? map[i].sh : map[i].base;
            if (ctrl && ch >= 'a' && ch <= 'z') {
                return (int)(ch - 'a' + 1);
            }
            if (ctrl && ch >= 'A' && ch <= 'Z') {
                return (int)(ch - 'A' + 1);
            }
            return (int)ch;
        }
    }
    return -1;
}

static void vk_offer(struct vk_dev *d, uint16_t id) {
    uint16_t *ring = (uint16_t *)(void *)(d->avail + 4);
    ring[d->avail_idx % VK_QSIZE] = id;
    d->avail_idx++;
    vk_dsb();
    *(volatile uint16_t *)(void *)(d->avail + 2) = d->avail_idx;
}

static void vk_notify(struct vk_dev *d) {
    vk_dsb();
    vk_rw(d->mmio, VK_REG_QUEUE_NOTIFY, 0);
}

static void vk_handle(struct vk_dev *d, struct vk_event ev) {
    if (ev.type != VK_EV_KEY) {
        return;
    }
    int pressed = ev.value != 0;
    if (ev.code == VK_KEY_LEFTSHIFT || ev.code == VK_KEY_RIGHTSHIFT) {
        d->shift = pressed ? 1 : 0;
        return;
    }
    if (ev.code == VK_KEY_LEFTCTRL || ev.code == VK_KEY_RIGHTCTRL) {
        d->ctrl = pressed ? 1 : 0;
        return;
    }
    int sc = vk_linux_to_sc(ev.code);
    if (sc >= 0) {
        /* Lattice: arg1==0, arg2.u8[0]==SC_CURSOR_* (down and up). */
        vk_msg_push(pressed ? 2 : 3, 0, (uint64_t)(unsigned)sc);
        return;
    }
    int ch = vk_keycode_ascii(ev.code, d->shift, d->ctrl);
    if (ch < 0) {
        return;
    }
    if (pressed) {
        vk_ring_push((uint8_t)ch);
        vk_msg_push(2, (uint64_t)(int64_t)ch, 0); /* MESSAGE_KEY_DOWN */
    } else {
        vk_msg_push(3, (uint64_t)(int64_t)ch, 0); /* MESSAGE_KEY_UP */
    }
}

static void vk_poll(void) {
    if (!g_vk.ready) {
        return;
    }
    struct vk_dev *d = &g_vk;
    uint16_t uidx = *(volatile uint16_t *)(void *)(d->used + 2);
    while (d->last_used != uidx) {
        uint32_t id =
            *(volatile uint32_t *)(void *)(d->used + 4 + (d->last_used % VK_QSIZE) * 8);
        __asm__ volatile("" ::: "memory");
        struct vk_event ev;
        uint8_t *ep = d->events + (id % VK_QSIZE) * 8;
        ev.type = (uint16_t)ep[0] | ((uint16_t)ep[1] << 8);
        ev.code = (uint16_t)ep[2] | ((uint16_t)ep[3] << 8);
        ev.value = (uint32_t)ep[4] | ((uint32_t)ep[5] << 8) | ((uint32_t)ep[6] << 16) |
                   ((uint32_t)ep[7] << 24);
        vk_handle(d, ev);
        d->last_used++;
        vk_offer(d, (uint16_t)id);
    }
    vk_notify(d);
}

static int virtio_kbd_init(uint64_t hhdm) {
    g_vk_hhdm = hhdm;
    g_vk.ready = 0;
    g_vk_rh = g_vk_rt = 0;

    /* Use last INPUT device (QEMU often places backends in high slots). */
    volatile uint32_t *found = NULL;
    uint32_t found_ver = 0;
    for (unsigned slot = 0; slot < VK_MMIO_SLOTS; slot++) {
        uint64_t phys = VK_MMIO_BASE + slot * VK_MMIO_STRIDE;
        volatile uint32_t *m = (volatile uint32_t *)(uintptr_t)(hhdm + phys);
        if (vk_rr(m, VK_REG_MAGIC) != VK_MAGIC) {
            continue;
        }
        uint32_t ver = vk_rr(m, VK_REG_VERSION);
        uint32_t id = vk_rr(m, VK_REG_DEVICE_ID);
        if ((ver != 1 && ver != 2) || id != VK_ID_INPUT) {
            continue;
        }
        found = m;
        found_ver = ver;
    }
    if (!found) {
        return 0;
    }

    uint8_t *mem = g_vk_dma;
    for (unsigned i = 0; i < sizeof(g_vk_dma); i++) {
        mem[i] = 0;
    }

    vk_rw(found, VK_REG_STATUS, 0);
    vk_rw(found, VK_REG_STATUS, VK_S_ACK);
    vk_rw(found, VK_REG_STATUS, VK_S_ACK | VK_S_DRIVER);

    if (vk_cfg_size(found, VK_CFG_EV_BITS, VK_EV_KEY_T) == 0) {
        vk_rw(found, VK_REG_STATUS, 0);
        return 0;
    }
    if (vk_cfg_size(found, VK_CFG_EV_BITS, VK_EV_ABS_T) ||
        vk_cfg_size(found, VK_CFG_EV_BITS, VK_EV_REL_T)) {
        vk_rw(found, VK_REG_STATUS, 0);
        return 0;
    }

    vk_rw(found, VK_REG_QUEUE_SEL, 0);
    if (vk_rr(found, VK_REG_QUEUE_NUM_MAX) == 0) {
        vk_rw(found, VK_REG_STATUS, 0);
        return 0;
    }
    vk_rw(found, VK_REG_QUEUE_NUM, VK_QSIZE);

    uint8_t *desc;
    uint8_t *avail;
    uint8_t *used;
    uint8_t *events;

    if (found_ver == 2) {
        desc = mem;
        avail = mem + VK_QSIZE * 16;
        uintptr_t used_u = ((uintptr_t)avail + 6 + VK_QSIZE * 2 + 15) & ~(uintptr_t)15;
        used = (uint8_t *)used_u;
        uintptr_t ev_u = ((uintptr_t)used + 6 + VK_QSIZE * 8 + 15) & ~(uintptr_t)15;
        events = (uint8_t *)ev_u;
        if (events + VK_QSIZE * 8 > mem + sizeof(g_vk_dma)) {
            return 0;
        }

        vk_rw(found, VK_REG_DEV_FEAT_SEL, 1);
        (void)vk_rr(found, VK_REG_DEV_FEAT);
        vk_rw(found, VK_REG_DRV_FEAT_SEL, 1);
        vk_rw(found, VK_REG_DRV_FEAT, 1);
        vk_rw(found, VK_REG_DRV_FEAT_SEL, 0);
        vk_rw(found, VK_REG_DRV_FEAT, 0);
        vk_rw(found, VK_REG_STATUS, VK_S_ACK | VK_S_DRIVER | VK_S_FEATURES_OK);
        if ((vk_rr(found, VK_REG_STATUS) & VK_S_FEATURES_OK) == 0) {
            vk_rw(found, VK_REG_STATUS, 0);
            return 0;
        }

        for (unsigned i = 0; i < VK_QSIZE; i++) {
            uint64_t addr = vk_v2p(events + i * 8);
            uint8_t *d = desc + i * 16;
            for (int b = 0; b < 8; b++) {
                d[b] = (uint8_t)(addr >> (8 * b));
            }
            d[8] = 8;
            d[9] = d[10] = d[11] = 0;
            d[12] = (uint8_t)VK_DESC_F_WRITE;
            d[13] = d[14] = d[15] = 0;
        }

        uint64_t dp = vk_v2p(desc), ap = vk_v2p(avail), up = vk_v2p(used);
        vk_rw(found, VK_REG_DESC_LO, (uint32_t)dp);
        vk_rw(found, VK_REG_DESC_HI, (uint32_t)(dp >> 32));
        vk_rw(found, VK_REG_DRIVER_LO, (uint32_t)ap);
        vk_rw(found, VK_REG_DRIVER_HI, (uint32_t)(ap >> 32));
        vk_rw(found, VK_REG_DEVICE_LO, (uint32_t)up);
        vk_rw(found, VK_REG_DEVICE_HI, (uint32_t)(up >> 32));
        vk_rw(found, VK_REG_QUEUE_READY, 1);
        vk_rw(found, VK_REG_STATUS,
              VK_S_ACK | VK_S_DRIVER | VK_S_FEATURES_OK | VK_S_DRIVER_OK);
    } else {
        /* Legacy virtio-mmio: contiguous ring addressed by QueuePFN. */
        vk_rw(found, VK_REG_DRV_FEAT_SEL, 0);
        vk_rw(found, VK_REG_DRV_FEAT, 0);
        vk_rw(found, VK_REG_GUEST_PAGE_SIZE, 4096);
        vk_rw(found, VK_REG_QUEUE_ALIGN, 4096);

        unsigned n = VK_QSIZE;
        unsigned avail_off = n * 16;
        unsigned used_off = (avail_off + (6 + n * 2) + 4095) & ~4095u;
        unsigned ev_off = used_off + 6 + n * 8;
        ev_off = (ev_off + 15) & ~15u;
        if (ev_off + n * 8 > sizeof(g_vk_dma)) {
            return 0;
        }
        desc = mem;
        avail = mem + avail_off;
        used = mem + used_off;
        events = mem + ev_off;

        for (unsigned i = 0; i < n; i++) {
            uint64_t addr = vk_v2p(events + i * 8);
            uint8_t *d = desc + i * 16;
            for (int b = 0; b < 8; b++) {
                d[b] = (uint8_t)(addr >> (8 * b));
            }
            d[8] = 8;
            d[9] = d[10] = d[11] = 0;
            d[12] = (uint8_t)VK_DESC_F_WRITE;
            d[13] = d[14] = d[15] = 0;
        }

        uint64_t region_phys = vk_v2p(mem);
        vk_rw(found, VK_REG_QUEUE_PFN, (uint32_t)(region_phys >> 12));
        vk_rw(found, VK_REG_STATUS, VK_S_ACK | VK_S_DRIVER | VK_S_DRIVER_OK);
    }

    g_vk.mmio = found;
    g_vk.desc = desc;
    g_vk.avail = avail;
    g_vk.used = used;
    g_vk.events = events;
    g_vk.last_used = 0;
    g_vk.avail_idx = 0;
    g_vk.shift = g_vk.ctrl = 0;

    for (uint16_t i = 0; i < VK_QSIZE; i++) {
        vk_offer(&g_vk, i);
    }
    vk_notify(&g_vk);
    g_vk.ready = 1;
    return 1;
}

static int virtio_kbd_getc_nb(void) {
    vk_poll();
    return vk_ring_pop();
}

/* Lattice MessageGet: typed KEY_DOWN/KEY_UP with ASCII or SC_CURSOR_* args. */
static int virtio_kbd_msg_nb(uint8_t *type, uint64_t *a1, uint64_t *a2) {
    vk_poll();
    return vk_msg_pop(type, a1, a2);
}

/* Peek: poll DMA into msg ring without consuming (Sleep wake). */
static int virtio_kbd_msg_pending(void) {
    if (!g_vk.ready) {
        return 0;
    }
    vk_poll();
    return g_vk_mh != g_vk_mt;
}

static int virtio_kbd_ready(void) {
    return g_vk.ready;
}

static uint64_t virtio_kbd_mmio_phys(void) {
    if (!g_vk.ready) {
        return 0;
    }
    return (uint64_t)(uintptr_t)g_vk.mmio - g_vk_hhdm;
}
