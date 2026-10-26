/* USB HID 1.11 appendix B.2: three buttons and signed relative X/Y.
 * Boot protocol deliberately ignores device-specific bytes after byte 2. */
#pragma once
#include "pi4_usb_keyboard.h"

static int pi4_usb_mouse_select(const uint8_t *d, unsigned length,
        struct pi4_usb_keyboard_interface *out) {
    return pi4_usb_boot_hid_select(d, length, 2u, 3u, out);
}
static int pi4_usb_mouse_context_seed(void *input, unsigned bytes,
        unsigned size, const void *output, unsigned output_bytes, unsigned speed,
        const struct pi4_usb_keyboard_interface *m, uint64_t ring_bus) {
    return pi4_usb_boot_hid_context_seed(input, bytes, size, output, output_bytes,
        speed, m, ring_bus, 3u, m ? m->packet : 0u);
}
struct pi4_usb_mouse_state {
    uint32_t x, y;
    unsigned buttons, reports, moves, presses, releases, seen_buttons;
};
static uint32_t pi4_usb_mouse_clamp(int64_t value, uint32_t dimension) {
    return value < 0 ? 0u : (uint64_t)value >= dimension ? dimension - 1u :
        (uint32_t)value;
}
static int pi4_usb_mouse_decode(struct pi4_usb_mouse_state *s,
        const uint8_t *report, unsigned bytes, uint32_t width, uint32_t height) {
    if (!s || !report || bytes < 3u || bytes > 64u || !width || !height) return 0;
    int dx = report[1] < 128u ? report[1] : (int)report[1] - 256;
    int dy = report[2] < 128u ? report[2] : (int)report[2] - 256;
    unsigned buttons = report[0] & 7u;
    s->x = pi4_usb_mouse_clamp((int64_t)s->x + dx, width);
    s->y = pi4_usb_mouse_clamp((int64_t)s->y + dy, height);
    s->moves += dx != 0 || dy != 0;
    s->presses += (buttons & ~s->buttons) != 0;
    s->releases += (s->buttons & ~buttons) != 0;
    s->seen_buttons |= buttons;
    s->buttons = buttons;
    s->reports++;
    return 1;
}
/* Caller must check event-ring cycle ownership before this identity check. */
static int pi4_usb_mouse_event_ok(uint64_t parameter, uint32_t status,
        uint32_t control, uint64_t trb, unsigned slot, unsigned dci,
        unsigned packet, unsigned *actual) {
    return packet >= 3u && packet <= 64u &&
        pi4_xhci_dma_bulk_event_ok(parameter, status, control, trb, slot, dci,
            packet, 1u, actual) && *actual >= 3u;
}
