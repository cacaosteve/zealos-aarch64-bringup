/* Polled native USB input for QEMU virt / UTM NEC and QEMU xHCI.
 * Ring/enumeration/HID behavior adapted from the public-domain YDE drivers:
 * Kernel/Usb/{KXhci,KUsbCore,KUsbHid}.ZC. ARM64 platform hooks are local.
 * Bounded bootstrap: two root-bus controllers, eight devices, five hub tiers.
 * Unsupported devices never claim input. Timeouts preserve serial recovery.
 */
#pragma once
#include "usb_platform.h"
#include "usb_hid.h"

#define USB_HCS 2
#define USB_DEVS 8
#define USB_EPS 4
#define USB_TRBS 64
#define USB_EVENTS 256
#define USB_HUB_PORTS 15
#define USB_DEVICE_DMA (48u * 1024u)
#define USB_TRB(t) ((uint32_t)(t) << 10)
#define USB_CC_OK 1
#define USB_CC_SHORT 13

struct usb_trb { uint64_t parameter; uint32_t status, control; };
struct usb_ring { struct usb_trb *trbs; unsigned enqueue, cycle; };
struct usb_hc;
struct usb_dev;
struct usb_port_state { uint64_t ready_at; unsigned connected, attempted; };
struct usb_ep {
    struct usb_ring ring;
    uint8_t *report;
    uint64_t pending;
    unsigned dci, packet, interval, interface, protocol, subclass;
    unsigned kind; /* 1=boot keyboard, 2=relative pointer, 3=tablet, 4=hub */
    unsigned report_descriptor_length;
    unsigned active, failed;
    uint8_t previous[8];
    unsigned repeat_usage;
    uint64_t repeat_at, reports;
    struct usb_hid_pointer pointer;
};
struct usb_dev {
    struct usb_hc *hc;
    struct usb_dev *parent;
    uint8_t *dma;
    size_t dma_used;
    uint8_t *input_ctx, *device_ctx, *buffer;
    struct usb_ring control_ring;
    struct usb_ep ep[USB_EPS];
    unsigned slot, port, speed, packet0, ep_count, vendor, product;
    unsigned present, configured, failed;
    unsigned quarantined, route, depth, parent_port, tt_slot, tt_port;
    unsigned hub_ports, hub_protocol, hub_think, hub_pending;
    uint64_t hub_rescan_at;
    struct usb_port_state children[USB_HUB_PORTS];
    uint64_t control_pending, control_data;
    unsigned control_done, control_code, control_actual;
};
struct usb_hc {
    volatile uint8_t *cfg, *mmio, *op, *rt, *db;
    uint64_t bar;
    uint64_t *dcbaa;
    struct usb_ring command;
    struct usb_trb *events;
    unsigned event_dequeue, event_cycle;
    unsigned slots, ports, context_size, running, error, dev, fn;
    uint64_t command_pending;
    unsigned command_done, command_code, command_slot;
    uint64_t event_count, command_count, port_changes;
    struct usb_port_state root[32];
};
static struct {
    struct usb_hc hc[USB_HCS];
    struct usb_dev devices[USB_DEVS];
    struct usb_ep *keyboard, *pointer;
    unsigned controllers, device_count, errors, busy;
    uint64_t topology_at, attaches, detaches, rejected;
    unsigned width, height;
    int32_t x, y, wheel;
    int buttons;
    uint64_t key_events, pointer_reports, wheel_events;
    void (*emit_key)(unsigned, int);
} g_usb;

static void usb_poll(void);
static void usb_log(const char *s) { uart_puts(g_uart, s); }
static void usb_number(unsigned n) { uart_put_u32_dec(g_uart, n); }
static unsigned usb_le16(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static int usb_success(unsigned code) { return code == USB_CC_OK || code == USB_CC_SHORT; }

/* Each record owns a reserved region for its entire lifetime. Reuse is allowed
 * only after Disable Slot completed and old events were drained. A timeout
 * quarantines the record; no memory possibly owned by DMA is recycled. */
static void *usb_dev_alloc(struct usb_dev *d, size_t n, size_t align) {
    if (align < 256) align = 256;
    size_t start = (d->dma_used + align - 1) & ~(align - 1);
    n = (n + 255) & ~(size_t)255;
    if (!d->dma || start > USB_DEVICE_DMA || n > USB_DEVICE_DMA - start) return NULL;
    void *p = d->dma + start;
    d->dma_used = start + n;
    usb_zero(p, n);
    usb_dma_sync(p, n, 0);
    return p;
}
static int usb_dev_ring_init(struct usb_dev *d, struct usb_ring *r) {
    r->trbs = usb_dev_alloc(d, 4096, 4096);
    r->enqueue = 0; r->cycle = 1;
    return r->trbs != NULL;
}

static int usb_ring_init(struct usb_ring *r) {
    r->trbs = usb_dma_alloc(4096, 4096);
    r->cycle = 1;
    r->enqueue = 0;
    return r->trbs != NULL;
}
static uint64_t usb_ring_put(struct usb_ring *r, uint64_t parameter,
                              uint32_t status, uint32_t control) {
    struct usb_trb *t = &r->trbs[r->enqueue];
    t->parameter = parameter;
    t->status = status;
    usb_publish();
    t->control = (control & ~1u) | r->cycle;
    usb_dma_sync(t, sizeof(*t), 0);
    uint64_t address = usb_dma_addr(t);
    if (++r->enqueue == USB_TRBS - 1) {
        struct usb_trb *link = &r->trbs[USB_TRBS - 1];
        link->parameter = usb_dma_addr(r->trbs);
        link->status = 0;
        usb_publish();
        link->control = USB_TRB(6) | 2 | r->cycle; /* link, toggle cycle */
        usb_dma_sync(link, sizeof(*link), 0);
        r->enqueue = 0;
        r->cycle ^= 1;
    }
    return address;
}
static void usb_doorbell(struct usb_hc *hc, unsigned slot, unsigned dci) {
    usb_publish();
    usb_w32(hc->db, slot * 4, dci);
}
static void usb_interrupt_queue(struct usb_dev *d, struct usb_ep *ep) {
    /* The previous IN transfer is complete before this buffer is reused. */
    usb_zero(ep->report, 64);
    usb_dma_sync(ep->report, 64, 0);
    ep->pending = usb_ring_put(&ep->ring, usb_dma_addr(ep->report),
                               ep->packet, USB_TRB(1) | (1u << 5) | (1u << 2));
    usb_doorbell(d->hc, d->slot, ep->dci);
}
static int usb_report_has(const uint8_t *report, unsigned usage) {
    for (unsigned i = 2; i < 8; i++) if (report[i] == usage) return 1;
    return 0;
}
static void usb_key(unsigned usage, int down) {
    unsigned code = usb_hid_key(usage);
    if (code && g_usb.emit_key) {
        g_usb.key_events++;
        g_usb.emit_key(code, down);
    }
}
static void usb_keyboard_report(struct usb_ep *ep, unsigned length) {
    if (length < 8 || ep != g_usb.keyboard) return;
    const uint8_t *r = ep->report;
    /* Boot rollover reports must not release all previously-held keys. */
    for (unsigned i = 2; i < 8; i++) if (r[i] && r[i] < 4) return;
    for (unsigned i = 0; i < 8; i++)
        if ((r[0] ^ ep->previous[0]) & (1u << i)) usb_key(224 + i, !!(r[0] & (1u << i)));
    for (unsigned i = 2; i < 8; i++)
        if (ep->previous[i] && !usb_report_has(r, ep->previous[i])) {
            usb_key(ep->previous[i], 0);
            if (ep->repeat_usage == ep->previous[i]) ep->repeat_usage = 0;
        }
    for (unsigned i = 2; i < 8; i++)
        if (r[i] && !usb_report_has(ep->previous, r[i])) {
            int duplicate = 0;
            for (unsigned j = 2; j < i; j++) duplicate |= r[j] == r[i];
            if (duplicate) continue;
            usb_key(r[i], 1);
            ep->repeat_usage = r[i];
            ep->repeat_at = usb_deadline(400);
        }
    usb_copy(ep->previous, r, 8);
}
static int32_t usb_clamp(int64_t value, int32_t max) {
    return value < 0 ? 0 : value > max ? max : (int32_t)value;
}
static void usb_pointer_report(struct usb_ep *ep, unsigned length) {
    if (ep != g_usb.pointer) return;
    int32_t x, y, wheel;
    int buttons;
    if (!usb_hid_pointer_read(&ep->pointer, ep->report, length, &x, &y, &wheel, &buttons)) return;
    if (ep->pointer.absolute) {
        const struct usb_hid_axis *ax = &ep->pointer.x, *ay = &ep->pointer.y;
        g_usb.x = usb_clamp(((int64_t)x - ax->min) * (g_usb.width - 1) / ((int64_t)ax->max - ax->min), g_usb.width - 1);
        g_usb.y = usb_clamp(((int64_t)y - ay->min) * (g_usb.height - 1) / ((int64_t)ay->max - ay->min), g_usb.height - 1);
    } else {
        g_usb.x = usb_clamp((int64_t)g_usb.x + x, g_usb.width - 1);
        g_usb.y = usb_clamp((int64_t)g_usb.y + y, g_usb.height - 1);
    }
    g_usb.buttons = buttons;
    int64_t next = (int64_t)g_usb.wheel + wheel;
    g_usb.wheel = next > 127 ? 127 : next < -128 ? -128 : (int32_t)next;
    g_usb.pointer_reports++;
    if (wheel) g_usb.wheel_events++;
}
static void usb_events(struct usb_hc *hc) {
    if (!hc->running) return;
    for (unsigned work = 0; work < USB_EVENTS; work++) {
        struct usb_trb *p = &hc->events[hc->event_dequeue];
        usb_dma_sync(p, sizeof(*p), 1);
        if ((p->control & 1) != hc->event_cycle) break;
        struct usb_trb e = *p;
        unsigned type = (e.control >> 10) & 63, code = e.status >> 24;
        unsigned slot = e.control >> 24, dci = (e.control >> 16) & 31;
        hc->event_count++;
        if (type == 33 && e.parameter == hc->command_pending) {
            hc->command_done = 1;
            hc->command_code = code;
            hc->command_slot = slot;
        } else if (type == 32) {
            for (unsigned i = 0; i < g_usb.device_count; i++) {
                struct usb_dev *d = &g_usb.devices[i];
                if (!d->present || d->hc != hc || d->slot != slot) continue;
                if (dci == 1) {
                    if (e.parameter == d->control_data && code == USB_CC_SHORT) {
                        unsigned residue = e.status & 0xffffff;
                        if (residue > d->control_actual) { d->control_code = 31; d->control_done = 1; }
                        else d->control_actual -= residue;
                    }
                    if (e.parameter == d->control_pending || !usb_success(code)) {
                        d->control_code = code;
                        d->control_done = 1;
                    }
                } else for (unsigned j = 0; j < d->ep_count; j++) {
                    struct usb_ep *ep = &d->ep[j];
                    if (!ep->active || ep->dci != dci || e.parameter != ep->pending) continue;
                    unsigned residue = e.status & 0xffffff;
                    if (!usb_success(code) || residue > ep->packet) {
                        ep->active = 0;
                        ep->failed = code;
                        g_usb.errors++;
                        continue; /* halt/stall never spins or requeues forever */
                    }
                    usb_dma_sync(ep->report, 64, 1);
                    ep->reports++;
                    if (ep->kind == 1) usb_keyboard_report(ep, ep->packet - residue);
                    else if (ep->kind == 4) {
                        unsigned actual = ep->packet - residue;
                        for (unsigned k = 0; k < actual && k < 2; k++)
                            d->hub_pending |= (unsigned)ep->report[k] << (8 * k);
                    } else usb_pointer_report(ep, ep->packet - residue);
                    usb_interrupt_queue(d, ep);
                }
            }
        } else if (type == 34) hc->port_changes++;
        else if (type == 37) {
            hc->error = code; g_usb.errors++;
            hc->running = 0;
            usb_w32(hc->op, 0, 0);
        }
        if (++hc->event_dequeue == USB_EVENTS) {
            hc->event_dequeue = 0;
            hc->event_cycle ^= 1;
        }
        usb_w64(hc->rt, 0x38, usb_dma_addr(&hc->events[hc->event_dequeue]) | 8);
        if (!hc->running) break;
    }
}
static unsigned usb_command(struct usb_hc *hc, uint64_t parameter, uint32_t control) {
    hc->command_done = 0;
    hc->command_code = 0;
    hc->command_pending = usb_ring_put(&hc->command, parameter, 0, control);
    hc->command_count++;
    usb_doorbell(hc, 0, 0);
    uint64_t end = usb_deadline(1000);
    do {
        usb_events(hc);
        if (hc->command_done) return hc->command_code;
    } while (usb_now() < end);
    /* A timed-out command ring cannot accept another command safely. */
    hc->error = 0x100;
    hc->running = 0;
    usb_w32(hc->op, 0, 0);
    g_usb.errors++;
    return 0;
}
static uint32_t *usb_input_ctx(struct usb_dev *d, unsigned index) {
    return (uint32_t *)(d->input_ctx + index * d->hc->context_size);
}
static void usb_input_clear(struct usb_dev *d) {
    usb_zero(d->input_ctx, 33 * d->hc->context_size);
}
static unsigned usb_context_command(struct usb_dev *d, unsigned type) {
    usb_dma_sync(d->input_ctx, 33 * d->hc->context_size, 0);
    return usb_command(d->hc, usb_dma_addr(d->input_ctx), USB_TRB(type) | (d->slot << 24));
}
static int usb_control(struct usb_dev *d, unsigned rt, unsigned request,
                        unsigned value, unsigned index, unsigned length) {
    if (length > 4096 || !d->hc->running) return -1;
    d->control_done = 0;
    d->control_actual = length;
    d->control_data = 0;
    if (length) {
        usb_zero(d->buffer, length);
        usb_dma_sync(d->buffer, length, 0);
    }
    uint64_t setup = rt | ((uint64_t)request << 8) | ((uint64_t)value << 16) |
                     ((uint64_t)index << 32) | ((uint64_t)length << 48);
    unsigned trt = length ? ((rt & 128) ? 3 : 2) : 0;
    usb_ring_put(&d->control_ring, setup, 8, USB_TRB(2) | (1u << 6) | (trt << 16));
    if (length)
        d->control_data = usb_ring_put(&d->control_ring, usb_dma_addr(d->buffer), length,
            USB_TRB(3) | (1u << 2) | ((rt & 128) ? 1u << 16 : 0));
    d->control_pending = usb_ring_put(&d->control_ring, 0, 0,
        USB_TRB(4) | (1u << 5) | ((!length || !(rt & 128)) ? 1u << 16 : 0));
    usb_doorbell(d->hc, d->slot, 1);
    uint64_t end = usb_deadline(1000);
    do {
        usb_events(d->hc);
        if (d->control_done) {
            usb_dma_sync(d->buffer, length, 1);
            return usb_success(d->control_code) ? (int)d->control_actual : -1;
        }
    } while (usb_now() < end);
    d->failed = 0x101;
    g_usb.errors++;
    return -1;
}
static int usb_port_reset(struct usb_hc *hc, unsigned port) {
    unsigned off = 0x400 + port * 16;
    for (unsigned retry = 0; retry < 3; retry++) {
        uint32_t sc = usb_r32(hc->op, off);
        if (!(sc & 1)) return 0;
        if (sc & 2) return 1;
        /* Retain PORTSC power/link/wake state (YDE's NEC fix), but mask
         * PED (write-1-disable) and all write-1-to-clear change bits. */
        usb_w32(hc->op, off, (sc & ~((0x7fu << 17) | 2u)) | (1u << 9) | (1u << 4));
        if (!usb_wait(hc->op, off, 1u << 4, 0, 1000)) continue;
        usb_delay(20);
    }
    return (usb_r32(hc->op, off) & 3) == 3;
}
static int usb_address(struct usb_dev *d) {
    struct usb_hc *hc = d->hc;
    if (usb_command(hc, 0, USB_TRB(9)) != USB_CC_OK) return 0;
    d->slot = hc->command_slot;
    if (!d->slot || d->slot > hc->slots) return 0;
    d->device_ctx = usb_dev_alloc(d, 32 * hc->context_size, 4096);
    d->input_ctx = usb_dev_alloc(d, 33 * hc->context_size, 4096);
    d->buffer = usb_dev_alloc(d, 4096, 4096);
    if (!d->device_ctx || !d->input_ctx || !d->buffer || !usb_dev_ring_init(d, &d->control_ring)) return 0;
    d->packet0 = d->speed >= 4 ? 512 : d->speed == 3 ? 64 : 8;
    uint32_t *control = usb_input_ctx(d, 0), *slot = usb_input_ctx(d, 1), *ep0 = usb_input_ctx(d, 2);
    control[1] = 3;
    slot[0] = (1u << 27) | (d->speed << 20) | d->route;
    slot[1] = (d->port + 1) << 16;
    slot[2] = d->tt_slot | (d->tt_port << 8);
    ep0[1] = (3u << 1) | (4u << 3) | (d->packet0 << 16);
    uint64_t ring = usb_dma_addr(d->control_ring.trbs);
    ep0[2] = (uint32_t)ring | 1;
    ep0[3] = ring >> 32;
    ep0[4] = 8;
    hc->dcbaa[d->slot] = usb_dma_addr(d->device_ctx);
    usb_dma_sync(hc->dcbaa + d->slot, 8, 0);
    return usb_context_command(d, 11) == USB_CC_OK;
}
static int usb_config_parse(struct usb_dev *d, unsigned length) {
    unsigned interface = 0, protocol = 0, subclass = 0, hid = 0, hub = 0, report_len = 0;
    for (unsigned pos = 0; pos + 2 <= length;) {
        uint8_t *p = d->buffer + pos;
        unsigned n = p[0], type = p[1];
        if (n < 2 || pos + n > length) return 0;
        if (type == 4) {
            if (n < 9) return 0;
            interface = p[2];
            hid = p[5] == 3 && p[3] == 0;
            hub = p[5] == 9 && p[3] == 0;
            subclass = p[6]; protocol = p[7]; report_len = 0;
        } else if (type == 0x21 && hid) {
            if (n < 6 || 6u + 3u * p[5] > n) return 0;
            for (unsigned j = 0; j < p[5]; j++)
                if (p[6 + 3*j] == 0x22) report_len = usb_le16(p + 7 + 3*j);
        } else if (type == 5 && (hid || hub)) {
            if (n < 7) return 0;
            if ((p[2] & 0x80) && (p[3] & 3) == 3) {
                unsigned packet = usb_le16(p + 4) & 0x7ff;
                if (!packet || packet > 64 || !(p[2] & 15) || d->ep_count == USB_EPS) return 0;
                struct usb_ep *ep = &d->ep[d->ep_count++];
                ep->dci = (p[2] & 15) * 2 + 1;
                ep->packet = packet; ep->interval = p[6];
                ep->interface = interface; ep->protocol = protocol;
                ep->subclass = subclass; ep->report_descriptor_length = report_len;
                if (hub) ep->kind = 4;
            }
        }
        pos += n;
    }
    return d->ep_count != 0;
}
static unsigned usb_interval(struct usb_dev *d, unsigned value) {
    if (d->speed >= 3) return value < 1 ? 0 : value > 16 ? 15 : value - 1;
    unsigned n = 3;
    /* FS/LS bInterval is the maximum period in milliseconds. Round down
     * to the supported power of two (10 ms -> 8 ms, not 16 ms). */
    while (n < 10 && (1u << (n + 1 - 3)) <= value) n++;
    return n;
}
static int usb_configure(struct usb_dev *d) {
    usb_input_clear(d);
    uint32_t *control = usb_input_ctx(d, 0), *slot = usb_input_ctx(d, 1);
    unsigned max_dci = 1;
    control[1] = 1;
    usb_dma_sync(d->device_ctx, d->hc->context_size, 1);
    usb_copy(slot, d->device_ctx, d->hc->context_size);
    for (unsigned j = 0; j < d->ep_count; j++) {
        struct usb_ep *ep = &d->ep[j];
        if (!ep->kind) continue;
        if (!usb_dev_ring_init(d, &ep->ring) || !(ep->report = usb_dev_alloc(d, 64, 256))) return 0;
        control[1] |= 1u << ep->dci;
        if (ep->dci > max_dci) max_dci = ep->dci;
        uint32_t *ctx = usb_input_ctx(d, ep->dci + 1);
        uint64_t ring = usb_dma_addr(ep->ring.trbs);
        ctx[0] = usb_interval(d, ep->interval) << 16;
        ctx[1] = (3u << 1) | (7u << 3) | (ep->packet << 16);
        ctx[2] = (uint32_t)ring | 1; ctx[3] = ring >> 32;
        ctx[4] = ep->packet | (ep->packet << 16);
    }
    slot[0] = (slot[0] & 0x07ffffff) | (max_dci << 27);
    if (d->hub_ports) {
        slot[0] |= 1u << 26; /* hub, single TT (alternate setting zero) */
        slot[1] = (slot[1] & 0x00ffffff) | (d->hub_ports << 24);
        slot[2] = (slot[2] & ~(3u << 16)) | (d->hub_think << 16);
    }
    return usb_context_command(d, 12) == USB_CC_OK;
}
static int usb_enumerate(struct usb_dev *d) {
    if (!usb_address(d)) return 0;
    if (usb_control(d, 0x80, 6, 0x100, 0, 8) < 8) return 0;
    unsigned packet0 = d->buffer[7];
    if (d->speed >= 4) {
        if (packet0 != 9) return 0;
        packet0 = 512;
    }
    if (packet0 != 8 && packet0 != 16 && packet0 != 32 && packet0 != 64 && packet0 != 512) return 0;
    if (packet0 != d->packet0) {
        usb_input_clear(d);
        usb_input_ctx(d, 0)[1] = 2;
        usb_dma_sync(d->device_ctx, 32 * d->hc->context_size, 1);
        uint32_t *ep0 = usb_input_ctx(d, 2);
        usb_copy(ep0, d->device_ctx + d->hc->context_size, d->hc->context_size);
        ep0[1] = (ep0[1] & 0xffff) | packet0 << 16;
        if (usb_context_command(d, 13) != USB_CC_OK) return 0;
        d->packet0 = packet0;
    }
    if (usb_control(d, 0x80, 6, 0x100, 0, 18) < 18 || d->buffer[1] != 1) return 0;
    d->vendor = usb_le16(d->buffer + 8); d->product = usb_le16(d->buffer + 10);
    unsigned device_class = d->buffer[4];
    d->hub_protocol = d->buffer[6];
    if (usb_control(d, 0x80, 6, 0x200, 0, 9) < 9 || d->buffer[1] != 2) return 0;
    unsigned total = usb_le16(d->buffer + 2), config = d->buffer[5];
    if (total < 9 || total > 4096 || !config || usb_control(d, 0x80, 6, 0x200, 0, total) != (int)total) return 0;
    if (!usb_config_parse(d, total)) return 0;
    unsigned hub = device_class == 9;
    for (unsigned j = 0; j < d->ep_count; j++) hub |= d->ep[j].kind == 4;
    if (hub) {
        /* USB 1.x/2.0 hub class. SuperSpeed hubs need a separate descriptor,
         * depth and link-state path; reject before claiming any endpoints. */
        if (d->depth >= 5 || d->speed >= 4 ||
            usb_control(d, 0xa0, 6, 0x2900, 0, 9) < 7 || d->buffer[1] != 0x29)
            return 0;
        d->hub_ports = d->buffer[2];
        d->hub_think = (usb_le16(d->buffer + 3) >> 5) & 3;
        if (!d->hub_ports || d->hub_ports > USB_HUB_PORTS) return 0;
    }
    /* Parse before configuring endpoints; unsupported HID layouts stay unowned. */
    unsigned accepted = 0;
    for (unsigned j = 0; j < d->ep_count; j++) {
        struct usb_ep *ep = &d->ep[j];
        if (ep->kind == 4) {
            if (ep->packet * 8 < d->hub_ports + 1) return 0;
        } else if (ep->subclass == 1 && ep->protocol == 1 && ep->packet >= 8) {
            ep->kind = 1;
        } else if (ep->report_descriptor_length && ep->report_descriptor_length <= 1024) {
            int n = usb_control(d, 0x81, 6, 0x2200, ep->interface, ep->report_descriptor_length);
            if (n == (int)ep->report_descriptor_length &&
                usb_hid_pointer_parse(&ep->pointer, d->buffer, n) &&
                ep->pointer.report_bits <= ep->packet * 8)
                ep->kind = ep->pointer.absolute ? 3 : 2;
        }
        accepted += ep->kind != 0;
    }
    if (!accepted || !usb_configure(d) || usb_control(d, 0, 9, config, 0, 0) != 0) return 0;
    d->configured = 1;
    for (unsigned j = 0; j < d->ep_count; j++) {
        struct usb_ep *ep = &d->ep[j];
        if (!ep->kind) continue;
        if (ep->kind != 4 && ep->subclass == 1 && usb_control(d, 0x21, 11, ep->kind == 1 ? 0 : 1, ep->interface, 0) != 0) return 0;
        /* SET_IDLE is advisory for pointers; a STALL cannot be followed by
         * more EP0 requests without endpoint recovery, so omit it here. */
        ep->active = 1;
        if (ep->kind == 1 && !g_usb.keyboard) g_usb.keyboard = ep;
        if ((ep->kind == 2 || ep->kind == 3) && (!g_usb.pointer || (g_usb.pointer->kind == 2 && ep->kind == 3))) g_usb.pointer = ep;
        usb_interrupt_queue(d, ep);
        usb_log(ep->kind == 4 ? "usb: hub" : ep->kind == 1 ? "usb: HID keyboard" : ep->kind == 3 ? "usb: HID tablet" : "usb: HID mouse");
        usb_log(" port="); usb_number(d->port + 1);
        usb_log(" slot="); usb_number(d->slot);
        usb_log("\n");
    }
    if (d->hub_ports) {
        for (unsigned p = 1; p <= d->hub_ports; p++)
            if (usb_control(d, 0x23, 3, 8, p, 0) != 0) return 0; /* PORT_POWER */
        usb_delay(100);
        d->hub_pending = (1u << (d->hub_ports + 1)) - 1;
    }
    return 1;
}
static int usb_handoff(struct usb_hc *hc, unsigned hcc) {
    unsigned off = (hcc >> 16) * 4;
    for (unsigned n = 0; off && n < 64; n++) {
        if (off > 65528) return 0;
        uint32_t cap = usb_r32(hc->mmio, off);
        if ((cap & 255) == 1) {
            usb_w32(hc->mmio, off, cap | (1u << 24));
            if (!usb_wait(hc->mmio, off, 1u << 16, 0, 1000)) return 0;
            usb_w32(hc->mmio, off + 4, 0); /* legacy SMI enables */
            return 1;
        }
        unsigned next = ((cap >> 8) & 255) * 4;
        if (!next) return 1;
        off += next;
    }
    return off == 0;
}
static int usb_hc_start(struct usb_hc *hc) {
    if (!usb_map_bar(hc->cfg, &hc->mmio, &hc->bar)) { hc->error = 1; return 0; }
    unsigned cap = usb_r32(hc->mmio, 0) & 255;
    unsigned rt = usb_r32(hc->mmio, 0x18) & ~31u, db = usb_r32(hc->mmio, 0x14) & ~3u;
    uint32_t hcs = usb_r32(hc->mmio, 4), hcs2 = usb_r32(hc->mmio, 8), hcc = usb_r32(hc->mmio, 16);
    hc->slots = hcs & 255; hc->ports = hcs >> 24;
    hc->context_size = hcc & 4 ? 64 : 32;
    if (cap < 32 || cap + 0x400 + hc->ports * 16 > 65536 ||
        rt < 32 || rt > 65536 - 64 || db < 32 || db > 65536 - (hc->slots + 1) * 4 ||
        !hc->slots || !hc->ports || hc->ports > 32) { hc->error = 2; return 0; }
    hc->op = hc->mmio + cap; hc->rt = hc->mmio + rt; hc->db = hc->mmio + db;
    if (!usb_handoff(hc, hcc)) { hc->error = 3; return 0; }
    usb_w32(hc->op, 0, usb_r32(hc->op, 0) & ~1u);
    if (!usb_wait(hc->op, 4, 1, 1, 1000)) { hc->error = 4; return 0; }
    usb_w32(hc->op, 0, 2);
    if (!usb_wait(hc->op, 0, 2, 0, 1000) || !usb_wait(hc->op, 4, 1u << 11, 0, 1000)) { hc->error = 5; return 0; }
    if (!(usb_r32(hc->op, 8) & 1)) { hc->error = 6; return 0; } /* 4K pages */
    if (!(hcc & 1) && usb_dma_addr(usb_dma_memory) + USB_DMA_BYTES > (1ull << 32)) { hc->error = 7; return 0; }
    hc->dcbaa = usb_dma_alloc((hc->slots + 1) * 8, 4096);
    unsigned scratch = ((hcs2 >> 27) & 31) | (((hcs2 >> 21) & 31) << 5);
    if (!hc->dcbaa || scratch > 32) { hc->error = 8; return 0; }
    if (scratch) {
        uint64_t *array = usb_dma_alloc(scratch * 8, 4096);
        if (!array) { hc->error = 8; return 0; }
        for (unsigned i = 0; i < scratch; i++) {
            void *page = usb_dma_alloc(4096, 4096);
            if (!page) { hc->error = 8; return 0; }
            array[i] = usb_dma_addr(page);
        }
        usb_dma_sync(array, scratch * 8, 0);
        hc->dcbaa[0] = usb_dma_addr(array);
        usb_dma_sync(hc->dcbaa, 8, 0);
    }
    hc->events = usb_dma_alloc(4096, 4096);
    uint64_t *erst = usb_dma_alloc(64, 256);
    if (!hc->events || !erst || !usb_ring_init(&hc->command)) { hc->error = 8; return 0; }
    erst[0] = usb_dma_addr(hc->events); erst[1] = USB_EVENTS;
    usb_dma_sync(erst, 64, 0);
    hc->event_cycle = 1;
    usb_w32(hc->op, 0x38, hc->slots);
    usb_w64(hc->op, 0x30, usb_dma_addr(hc->dcbaa));
    usb_w64(hc->op, 0x18, usb_dma_addr(hc->command.trbs) | 1);
    usb_w32(hc->rt, 0x20, 0); /* interrupter 0 disabled: polling only */
    usb_w32(hc->rt, 0x28, 1);
    usb_w64(hc->rt, 0x38, usb_dma_addr(hc->events));
    usb_w64(hc->rt, 0x30, usb_dma_addr(erst));
    usb_publish();
    usb_w32(hc->op, 0, 1);
    if (!usb_wait(hc->op, 4, 1, 0, 1000)) { hc->error = 9; return 0; }
    hc->running = 1;
    if (usb_command(hc, 0, USB_TRB(23)) != USB_CC_OK) { hc->error = 10; return 0; }
    for (unsigned i = 0; i < hc->ports; i++) {
        unsigned off = 0x400 + i * 16;
        uint32_t sc = usb_r32(hc->op, off);
        usb_w32(hc->op, off, (sc & ~((0x7fu << 17) | 2u)) | (1u << 9));
    }
    return 1;
}
static int usb_keyboard_ready(void) { return g_usb.keyboard && g_usb.keyboard->active; }
static int usb_pointer_ready(void) { return g_usb.pointer && g_usb.pointer->active; }
static void usb_release_device(struct usb_dev *d) {
    for (unsigned j = 0; j < d->ep_count; j++) {
        struct usb_ep *ep = &d->ep[j];
        if (g_usb.keyboard == ep) {
            for (unsigned bit = 0; bit < 8; bit++)
                if (ep->previous[0] & (1u << bit)) usb_key(224 + bit, 0);
            for (unsigned k = 2; k < 8; k++) if (ep->previous[k]) usb_key(ep->previous[k], 0);
            g_usb.keyboard = NULL;
        }
        if (g_usb.pointer == ep) { g_usb.pointer = NULL; g_usb.buttons = g_usb.wheel = 0; }
        ep->active = 0;
        ep->repeat_usage = 0;
    }
}
static void usb_detach(struct usb_dev *d) {
    if (!d->present) return;
    /* Children must be disabled before their hub's slot can be reused. */
    for (unsigned i = 0; i < g_usb.device_count; i++)
        if (g_usb.devices[i].present && g_usb.devices[i].parent == d)
            usb_detach(&g_usb.devices[i]);
    usb_release_device(d);
    d->present = d->configured = 0;
    if (d->slot) {
        if (d->hc->running &&
            usb_command(d->hc, 0, USB_TRB(10) | d->slot << 24) == USB_CC_OK) {
            usb_events(d->hc);
            d->hc->dcbaa[d->slot] = 0;
            usb_dma_sync(d->hc->dcbaa + d->slot, 8, 0);
            d->slot = 0;
        } else d->quarantined = 1;
    }
    d->parent = NULL;
    g_usb.detaches++;
}
static void usb_select_owners(void) {
    if (!usb_keyboard_ready()) g_usb.keyboard = NULL;
    if (!usb_pointer_ready()) g_usb.pointer = NULL;
    for (unsigned i = 0; i < g_usb.device_count; i++) {
        struct usb_dev *d = &g_usb.devices[i];
        if (!d->present || !d->configured || !d->hc->running) continue;
        for (unsigned j = 0; j < d->ep_count; j++) {
            struct usb_ep *ep = &d->ep[j];
            if (!ep->active) continue;
            if (ep->kind == 1 && !g_usb.keyboard) g_usb.keyboard = ep;
            if ((ep->kind == 2 || ep->kind == 3) &&
                (!g_usb.pointer || (g_usb.pointer->kind == 2 && ep->kind == 3))) {
                g_usb.pointer = ep;
                g_usb.buttons = g_usb.wheel = 0;
            }
        }
    }
}
static struct usb_dev *usb_child(struct usb_hc *hc, struct usb_dev *parent, unsigned port) {
    for (unsigned i = 0; i < g_usb.device_count; i++) {
        struct usb_dev *d = &g_usb.devices[i];
        if (d->present && d->hc == hc && d->parent == parent &&
            (parent ? d->parent_port == port : d->port == port)) return d;
    }
    return NULL;
}
static void usb_attach(struct usb_hc *hc, struct usb_dev *parent,
                       unsigned port, unsigned speed) {
    struct usb_dev *d = NULL;
    for (unsigned i = 0; i < USB_DEVS; i++)
        if (!g_usb.devices[i].present && !g_usb.devices[i].quarantined) {
            d = &g_usb.devices[i];
            if (i >= g_usb.device_count) g_usb.device_count = i + 1;
            break;
        }
    if (!d) { g_usb.rejected++; return; }
    uint8_t *dma = d->dma;
    if (!dma) dma = usb_dma_alloc(USB_DEVICE_DMA, 4096);
    if (!dma) { g_usb.errors++; return; }
    usb_zero(d, sizeof(*d));
    d->dma = dma; d->hc = hc; d->parent = parent;
    d->port = parent ? parent->port : port;
    d->parent_port = parent ? port : 0;
    d->speed = speed; d->present = 1;
    if (parent) {
        d->route = parent->route | port << (parent->depth * 4);
        d->depth = parent->depth + 1;
        /* A LS/FS descendant uses the first upstream high-speed hub's TT,
         * including when intervening full-speed hubs are present. */
        if (speed <= 2) {
            if (parent->speed == 3) { d->tt_slot = parent->slot; d->tt_port = port; }
            else { d->tt_slot = parent->tt_slot; d->tt_port = parent->tt_port; }
        }
    }
    if (!speed || !usb_enumerate(d)) {
        usb_log("usb: rejected device root="); usb_number(d->port + 1);
        usb_log(" route="); usb_number(d->route);
        usb_log(" control="); usb_number(d->control_code); usb_log("\n");
        g_usb.rejected++;
        usb_detach(d);
    } else g_usb.attaches++;
}
static int usb_hub_status(struct usb_dev *hub, unsigned port, uint32_t *status) {
    if (usb_control(hub, 0xa3, 0, 0, port, 4) != 4) return 0;
    *status = usb_le16(hub->buffer) | (uint32_t)usb_le16(hub->buffer + 2) << 16;
    return 1;
}
static void usb_port_ack(struct usb_hc *hc, struct usb_dev *hub,
                          unsigned port, uint32_t status) {
    if (hub) {
        for (unsigned bit = 0; bit < 5; bit++)
            if (status & (1u << (16 + bit)))
                (void)usb_control(hub, 0x23, 1, 16 + bit, port, 0);
    } else if (status & (0x7fu << 17)) {
        usb_w32(hc->op, 0x400 + port * 16, status & ~(2u | (1u << 4) | (1u << 31)));
    }
}
static unsigned usb_hub_reset(struct usb_dev *hub, unsigned port) {
    if (usb_control(hub, 0x23, 3, 4, port, 0) != 0) return 0;
    uint64_t end = usb_deadline(1000);
    uint32_t status = 0;
    do {
        usb_delay(10);
        if (!usb_hub_status(hub, port, &status) || !(status & 1)) return 0;
        if (!(status & 16) && (status & 2)) {
            usb_port_ack(hub->hc, hub, port, status);
            usb_delay(10); /* USB2 reset recovery */
            return status & (1u << 9) ? 2 : status & (1u << 10) ? 3 : 1;
        }
    } while (usb_now() < end);
    return 0;
}
static void usb_scan_port(struct usb_hc *hc, struct usb_dev *hub,
                          unsigned port, uint32_t status, int boot) {
    struct usb_port_state *s = hub ? &hub->children[port - 1] : &hc->root[port];
    struct usb_dev *child = usb_child(hc, hub, port);
    int connected = !!(status & 1);
    int changed = !!(status & (1u << (hub ? 16 : 17)));
    if (child && (!connected || changed)) usb_detach(child);
    usb_port_ack(hc, hub, port, status);
    if (!connected) { usb_zero(s, sizeof(*s)); return; }
    if (!s->connected || changed) {
        s->connected = 1; s->attempted = 0;
        s->ready_at = usb_deadline(100); /* debounce without blocking frames */
    }
    if (usb_child(hc, hub, port) || s->attempted || (!boot && usb_now() < s->ready_at)) return;
    s->attempted = 1; /* one attempt per connection, including unsupported devices */
    unsigned speed = 0;
    if (hub) speed = usb_hub_reset(hub, port);
    else if (usb_port_reset(hc, port)) {
        uint32_t sc = usb_r32(hc->op, 0x400 + port * 16);
        speed = (sc >> 10) & 15;
        usb_port_ack(hc, NULL, port, sc);
    }
    if (speed) usb_attach(hc, hub, port, speed);
    else g_usb.rejected++;
}
static void usb_scan_topology(int boot) {
    for (unsigned i = 0; i < g_usb.controllers; i++) {
        struct usb_hc *hc = &g_usb.hc[i];
        if (!hc->running) continue;
        for (unsigned port = 0; port < hc->ports && hc->running; port++)
            usb_scan_port(hc, NULL, port, usb_r32(hc->op, 0x400 + port * 16), boot);
    }
    for (unsigned i = 0; i < g_usb.device_count; i++) {
        struct usb_dev *hub = &g_usb.devices[i];
        if (!hub->present || !hub->configured || !hub->hub_ports || !hub->hc->running) continue;
        unsigned pending = hub->hub_pending;
        hub->hub_pending = 0;
        int rescan = boot || usb_now() >= hub->hub_rescan_at;
        if (rescan) hub->hub_rescan_at = usb_deadline(1000);
        for (unsigned port = 1; port <= hub->hub_ports; port++) {
            struct usb_port_state *s = &hub->children[port - 1];
            if (!rescan && !(pending & (1u << port)) && !(s->connected && !s->attempted)) continue;
            uint32_t status;
            if (!usb_hub_status(hub, port, &status)) { hub->failed = 0x103; break; }
            usb_scan_port(hub->hc, hub, port, status, boot);
        }
    }
    usb_select_owners();
}
static void usb_poll(void) {
    if (g_usb.busy) return;
    g_usb.busy = 1;
    for (unsigned i = 0; i < g_usb.controllers; i++) {
        struct usb_hc *hc = &g_usb.hc[i];
        if (hc->running && (usb_r32(hc->op, 4) & ((1u << 2) | (1u << 12)))) {
            hc->error = 0x102; hc->running = 0; g_usb.errors++;
            usb_w32(hc->op, 0, 0);
        }
        usb_events(hc);
    }
    /* Failures release held input and quarantine memory if Disable Slot cannot
     * finish. Root/hub disconnects are handled by the topology scan. */
    for (unsigned i = 0; i < g_usb.device_count; i++) {
        struct usb_dev *d = &g_usb.devices[i];
        if (!d->present || !d->configured) continue;
        int failed = !d->hc->running || d->failed;
        for (unsigned j = 0; j < d->ep_count; j++) failed |= d->ep[j].failed != 0;
        if (failed) {
            usb_detach(d);
        }
    }
    if (usb_now() >= g_usb.topology_at) {
        g_usb.topology_at = usb_deadline(100);
        usb_scan_topology(0);
    }
    usb_select_owners();
    if (usb_keyboard_ready() && g_usb.keyboard->repeat_usage && usb_now() >= g_usb.keyboard->repeat_at) {
        usb_key(g_usb.keyboard->repeat_usage, 1);
        g_usb.keyboard->repeat_at = usb_deadline(35);
    }
    g_usb.busy = 0;
}
static void usb_init(uint64_t hhdm, unsigned width, unsigned height,
                       void (*emit_key)(unsigned, int)) {
    usb_zero(&g_usb, sizeof(g_usb));
    g_usb.width = width ? width : 800; g_usb.height = height ? height : 600;
    g_usb.x = g_usb.width / 2; g_usb.y = g_usb.height / 2;
    g_usb.emit_key = emit_key;
    if (!usb_platform_init(hhdm)) { g_usb.errors++; return; }
    for (unsigned dev = 0; dev < 32 && g_usb.controllers < USB_HCS; dev++) {
        for (unsigned fn = 0; fn < 8 && g_usb.controllers < USB_HCS; fn++) {
            volatile uint8_t *cfg = usb_pci_function(dev, fn);
            uint32_t id = usb_r32(cfg, 0);
            if ((id & 65535) == 65535) continue;
            if ((usb_r32(cfg, 8) >> 8) != 0x0c0330) continue;
            /* First backend is deliberately limited to the emulated models.
             * Real PCIe (including Pi VL805) needs platform DMA/firmware hooks. */
            if (id != 0x01941033 && id != 0x000d1b36) {
                usb_log("usb: unsupported xHCI PCI model\n"); continue;
            }
            struct usb_hc *hc = &g_usb.hc[g_usb.controllers++];
            hc->cfg = cfg; hc->dev = dev; hc->fn = fn;
            if (!usb_hc_start(hc)) {
                hc->running = 0; g_usb.errors++;
                if (hc->op) usb_w32(hc->op, 0, 0);
                usb_log("usb: xHCI failed stage="); usb_number(hc->error); usb_log("\n");
                continue;
            }
            usb_log("usb: xHCI running ports="); usb_number(hc->ports);
            usb_log(" slots="); usb_number(hc->slots); usb_log("\n");
            usb_delay(20);
        }
    }
    g_usb.busy = 1;
    usb_scan_topology(1);
    g_usb.topology_at = usb_deadline(100);
    g_usb.busy = 0;
    usb_log("usb: owners keyboard="); usb_log(usb_keyboard_ready() ? "usb" : "virtio/serial");
    usb_log(" pointer="); usb_log(usb_pointer_ready() ? "usb" : "virtio"); usb_log("\n");
}
