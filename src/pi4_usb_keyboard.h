/* Bounded USB2 boot-keyboard selection, xHCI context and report decoding.
 * USB HID 1.11 boot protocol, section 7.2.6 and appendix B.1/F.5. */
#pragma once
#include <stdint.h>
#include "pi4_xhci_dma_probe.h"

struct pi4_usb_keyboard_interface {
    unsigned found, interface, endpoint, packet, interval, report_length;
};

static int pi4_usb_keyboard_setup_seed(uint32_t trb[4], unsigned step,
        unsigned interface, unsigned cycle) {
    const unsigned requests[3] = {0x0bu, 0x03u, 0x0au};
    if (step >= 3u || interface > 255u) return -1;
    unsigned length = step == 1u;
    return pi4_xhci_dma_setup_stage_seed(trb, length ? 0xa1u : 0x21u,
        requests[step], 0u, interface, length, length ? 3u : 0u, cycle);
}

/* The complete chain must pass the shared structural validator first. Select
 * an alternate-zero boot keyboard, including one in a composite device. */
static int pi4_usb_boot_hid_select(const uint8_t *d, unsigned length,
        unsigned protocol, unsigned min_packet,
        struct pi4_usb_keyboard_interface *out) {
    struct pi4_xhci_dma_config_summary checked;
    if (!out) return 0;
    *out = (struct pi4_usb_keyboard_interface){0};
    if ((protocol != 1u && protocol != 2u) || min_packet < 3u ||
        min_packet > 64u ||
        !pi4_xhci_dma_configuration_descriptor_ok(d, length, 4096u, &checked))
        return 0;
    struct pi4_usb_keyboard_interface chosen = {0};
    unsigned candidate = 0, number = 0, report_length = 0;
    unsigned endpoint = 0, packet = 0, interval = 0, seen = 0, count = 0;
    for (unsigned pos = 9u; pos <= length;) {
        unsigned type = pos < length ? d[pos + 1u] : 4u;
        if (type == 4u) {
            if (candidate && endpoint && report_length) {
                if (chosen.found) return 0; /* Ambiguous owners. */
                chosen = (struct pi4_usb_keyboard_interface){
                    1u, number, endpoint, packet, interval, report_length};
            }
            if (pos == length) break;
            candidate = d[pos + 3u] == 0u && d[pos + 5u] == 3u &&
                        d[pos + 6u] == 1u && d[pos + 7u] == protocol;
            number = d[pos + 2u]; report_length = 0;
            endpoint = packet = interval = seen = count = 0;
        } else if (candidate && type == 0x21u) {
            unsigned n = d[pos], descriptors = n >= 6u ? d[pos + 5u] : 0;
            if (!descriptors || n != 6u + 3u * descriptors || report_length)
                return 0;
            for (unsigned i = 0; i < descriptors; i++) {
                if (d[pos + 6u + 3u * i] != 0x22u) continue;
                if (report_length) return 0;
                report_length = d[pos + 7u + 3u * i] |
                    (unsigned)d[pos + 8u + 3u * i] << 8;
                if (!report_length || report_length > 4096u) return 0;
            }
        } else if (candidate && type == 5u) {
            unsigned addr = d[pos + 2u], attributes = d[pos + 3u];
            unsigned bit = (addr & 15u) * 2u + !!(addr & 0x80u);
            if (seen & (1u << bit)) return 0;
            seen |= 1u << bit;
            if ((addr & 0x80u) && attributes == 3u) {
                if (++count != 1u) return 0;
                endpoint = addr;
                packet = d[pos + 4u] | (unsigned)d[pos + 5u] << 8;
                interval = d[pos + 6u];
                if (packet < min_packet || packet > 64u || !interval) return 0;
            }
        }
        pos += d[pos];
    }
    *out = chosen;
    return chosen.found != 0u;
}

static int pi4_usb_keyboard_select(const uint8_t *d, unsigned length,
        struct pi4_usb_keyboard_interface *out) {
    return pi4_usb_boot_hid_select(d, length, 1u, 8u, out);
}

static int pi4_usb_keyboard_interval(unsigned speed, unsigned interval) {
    if (!interval || interval > 255u) return -1;
    if (speed == 3u) return interval <= 16u ? (int)interval - 1 : -1;
    if (speed != 1u && speed != 2u) return -1;
    unsigned n = 3u;
    while (n < 10u && (1u << (n + 1u - 3u)) <= interval) n++;
    return (int)n;
}

static int pi4_usb_boot_hid_context_seed(void *input, unsigned bytes,
        unsigned context_size, const void *output, unsigned output_bytes,
        unsigned speed, const struct pi4_usb_keyboard_interface *k,
        uint64_t ring_bus, unsigned min_packet, unsigned report_bytes) {
    if (!input || !output || !k || !k->found ||
        (context_size != 32u && context_size != 64u) ||
        ((uintptr_t)input & 63u) || ((uintptr_t)output & 63u) ||
        (ring_bus & 63u) || (k->endpoint & 0xf0u) != 0x80u ||
        !(k->endpoint & 15u) || min_packet < 3u || min_packet > 64u ||
        k->packet < min_packet || k->packet > 64u ||
        report_bytes < min_packet || report_bytes > k->packet ||
        (speed == 2u && k->packet > 8u)) return -1;
    unsigned dci = (k->endpoint & 15u) * 2u + 1u;
    int interval = pi4_usb_keyboard_interval(speed, k->interval);
    const uint32_t *source = (const uint32_t *)output;
    if (interval < 0 || bytes < (dci + 2u) * context_size ||
        output_bytes < (dci + 1u) * context_size ||
        ((source[0] >> 20) & 15u) != speed || (source[3] >> 27) < 2u)
        return -1;
    uint8_t *b = (uint8_t *)input;
    for (unsigned i = 0; i < bytes; i++) b[i] = 0;
    uint32_t *control = (uint32_t *)input;
    uint32_t *slot = (uint32_t *)(void *)(b + context_size);
    control[1] = 1u | (1u << dci);
    for (unsigned i = 0; i < context_size / 4u; i++) slot[i] = source[i];
    unsigned entries = slot[0] >> 27;
    if (entries < dci) slot[0] = (slot[0] & 0x07ffffffu) | (dci << 27);
    uint32_t *ep = (uint32_t *)(void *)(b + (dci + 1u) * context_size);
    ep[0] = (unsigned)interval << 16;
    ep[1] = (3u << 1) | (7u << 3) | (k->packet << 16);
    ep[2] = (uint32_t)ring_bus | 1u; ep[3] = (uint32_t)(ring_bus >> 32);
    ep[4] = report_bytes | (k->packet << 16); /* Avg TRB length / Max ESIT payload. */
    return 0;
}

static int pi4_usb_keyboard_context_seed(void *input, unsigned bytes,
        unsigned context_size, const void *output, unsigned output_bytes,
        unsigned speed, const struct pi4_usb_keyboard_interface *k,
        uint64_t ring_bus) {
    return pi4_usb_boot_hid_context_seed(input, bytes, context_size, output,
        output_bytes, speed, k, ring_bus, 8u, 8u);
}

struct pi4_usb_keyboard_state { uint8_t previous[8]; };
static int pi4_usb_keyboard_context_ok(const void *output, unsigned bytes,
        unsigned size, unsigned speed, const struct pi4_usb_keyboard_interface *k,
        uint64_t ring_bus) {
    if (!output || !k || !k->found || (size != 32u && size != 64u) ||
        ((uintptr_t)output & 63u)) return 0;
    unsigned dci = (k->endpoint & 15u) * 2u + 1u;
    if (bytes < (dci + 1u) * size) return 0;
    const uint32_t *ep = (const uint32_t *)(const void *)(
        (const uint8_t *)output + dci * size);
    return (ep[0] & 7u) == 1u && ((ep[0] >> 16) & 255u) ==
            (unsigned)pi4_usb_keyboard_interval(speed, k->interval) &&
        ((ep[1] >> 3) & 7u) == 7u && (ep[1] >> 16) == k->packet &&
        (((uint64_t)ep[3] << 32) | (ep[2] & ~15u)) == ring_bus && (ep[2] & 1u);
}
typedef void (*pi4_usb_keyboard_emit)(unsigned usage, int down);
static int pi4_usb_keyboard_has(const uint8_t *r, unsigned usage) {
    for (unsigned i = 2u; i < 8u; i++) if (r[i] == usage) return 1;
    return 0;
}
static int pi4_usb_keyboard_decode(struct pi4_usb_keyboard_state *s,
        const uint8_t *report, unsigned bytes, pi4_usb_keyboard_emit emit) {
    if (!s || !report || bytes != 8u || !emit) return -1;
    for (unsigned i = 2u; i < 8u; i++)
        if (report[i] && report[i] < 4u) return 0; /* Rollover: preserve state. */
    /* Release old keys while their original modifiers are still active. */
    for (unsigned i = 2u; i < 8u; i++) {
        unsigned u = s->previous[i];
        int earlier = 0;
        for (unsigned j = 2u; j < i; j++) earlier |= s->previous[j] == u;
        if (u && !earlier && !pi4_usb_keyboard_has(report, u)) emit(u, 0);
    }
    for (unsigned i = 0; i < 8u; i++)
        if ((report[0] ^ s->previous[0]) & (1u << i))
            emit(224u + i, !!(report[0] & (1u << i)));
    for (unsigned i = 2u; i < 8u; i++) {
        unsigned u = report[i];
        int earlier = 0;
        for (unsigned j = 2u; j < i; j++) earlier |= report[j] == u;
        if (u && !earlier && !pi4_usb_keyboard_has(s->previous, u)) emit(u, 1);
    }
    for (unsigned i = 0; i < 8u; i++) s->previous[i] = report[i];
    return 1;
}
