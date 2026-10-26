/* One OS input path with per-capability ownership. Park initialized VirtIO
 * transports while USB owns input; reinitialize them for recovery. */
#pragma once
#include "usb_xhci.h"

static int input_virtio_keyboard_parked, input_virtio_pointer_parked;

static void input_usb_key(unsigned code, int down) {
    struct vk_event e = {VK_EV_KEY, (uint16_t)code, (uint32_t)down};
    vk_decode_key(&g_vk, e);
}
static void input_poll(void) {
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
    pi4_usb_keyboard_poll();
    pi4_usb_bus_poll();
#endif
    usb_poll();
    g_vk_input_suppressed = usb_keyboard_ready();
    /* QEMU activates its VirtIO host input handler at DRIVER_OK. Merely
     * discarding events in the guest can starve USB of host keys entirely.
     * Reset that transport while USB owns it, and rebuild its DMA queues on
     * fallback. The configured recovery devices remain in the VM. */
    if (usb_keyboard_ready() && g_vk.ready) {
        vk_poll();
        vk_rw(g_vk.mmio, VK_REG_STATUS, 0);
        vk_dsb();
        g_vk.ready = 0;
        input_virtio_keyboard_parked = 1;
    } else if (!usb_keyboard_ready() && input_virtio_keyboard_parked) {
        input_virtio_keyboard_parked = 0;
        unsigned rh = g_vk_rh, rt = g_vk_rt, mh = g_vk_mh, mt = g_vk_mt;
        (void)virtio_kbd_init(g_vk_hhdm);
        /* Keep the synthetic USB key-up events produced during disconnect. */
        g_vk_rh = rh; g_vk_rt = rt; g_vk_mh = mh; g_vk_mt = mt;
    }
    if (usb_pointer_ready() && g_vt.ready) {
        vt_rw(g_vt.mmio, VT_REG_STATUS, 0);
        vt_dsb();
        g_vt.ready = 0;
        input_virtio_pointer_parked = 1;
    } else if (!usb_pointer_ready() && input_virtio_pointer_parked) {
        input_virtio_pointer_parked = 0;
        (void)virtio_tablet_init(g_vt_hhdm);
    }
    vk_poll();
}
static int input_keyboard_ready(void) {
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
    if (pi4_usb_keyboard_ready() || pi4_usb_bus_keyboard_ready()) return 1;
#endif
    return usb_keyboard_ready() || virtio_kbd_ready();
}
static int input_keyboard_getc_nb(void) { input_poll(); return virtio_kbd_getc_nb(); }
static int input_keyboard_msg_nb(uint8_t *type, uint64_t *a1, uint64_t *a2) {
    input_poll(); return virtio_kbd_msg_nb(type, a1, a2);
}
static int input_keyboard_msg_pending(void) { input_poll(); return virtio_kbd_msg_pending(); }
static int input_keyboard_ctrl_down(void) { input_poll(); return virtio_kbd_ctrl_down(); }
static int input_keyboard_take_ctrl_c(void) {
    input_poll(); return virtio_kbd_take_ctrl_c();
}
static int input_pointer_ready(void) {
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
    if (pi4_usb_mouse_ready() || pi4_usb_bus_pointer_ready()) return 1;
#endif
    return usb_pointer_ready() || virtio_tablet_ready();
}
static void input_pointer_poll(void) {
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
    pi4_usb_keyboard_poll();
    pi4_usb_bus_poll();
    if (pi4_usb_mouse_ready() || pi4_usb_bus_pointer_ready()) return;
#endif
    usb_poll(); if (!usb_pointer_ready()) virtio_tablet_poll();
}
static int input_pointer_sample(uint32_t w, uint32_t h, uint32_t *x, uint32_t *y,
                                  int *buttons, int *wheel) {
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
    if (pi4_usb_bus_sample(w, h, x, y, buttons, wheel)) return 1;
    if (pi4_usb_mouse_sample(w, h, x, y, buttons, wheel)) return 1;
#endif
    usb_poll();
    if (!usb_pointer_ready()) return virtio_tablet_sample(w, h, x, y, buttons, wheel);
    *x = g_usb.x; *y = g_usb.y;
    if (buttons) *buttons = g_usb.buttons;
    if (wheel) { *wheel = g_usb.wheel; g_usb.wheel = 0; }
    return 1;
}
static int input_pointer_xy(uint32_t w, uint32_t h, uint32_t *x, uint32_t *y) {
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
    if (pi4_usb_bus_sample(w, h, x, y, NULL, NULL)) return 1;
    if (pi4_usb_mouse_sample(w, h, x, y, NULL, NULL)) return 1;
#endif
    usb_poll();
    if (!usb_pointer_ready()) return virtio_tablet_xy(w, h, x, y);
    return input_pointer_sample(w, h, x, y, NULL, NULL);
}
static int input_pointer_buttons(void) {
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
    uint32_t x, y;
    int buttons;
    if (pi4_usb_bus_sample(1u, 1u, &x, &y, &buttons, NULL)) return buttons;
    if (pi4_usb_mouse_sample(1u, 1u, &x, &y, &buttons, NULL)) return buttons;
#endif
    usb_poll();
    return usb_pointer_ready() ? g_usb.buttons : virtio_tablet_buttons();
}
