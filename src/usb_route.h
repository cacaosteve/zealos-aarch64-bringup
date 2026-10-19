/* xHCI USB2 transaction-translator routing helpers. */
#pragma once

struct usb_tt_route {
    unsigned slot;
    unsigned port;
};

/* USB xHCI speed IDs: 1 full, 2 low, 3 high, 4 SuperSpeed. A full/low-speed
 * child of a high-speed hub uses that hub's TT and the immediate downstream
 * port. Through a full/low-speed hub, preserve the upstream TT route. */
static inline struct usb_tt_route usb_tt_child_route(
        unsigned parent_speed, unsigned parent_slot,
        unsigned parent_tt_slot, unsigned parent_tt_port,
        unsigned child_speed, unsigned parent_port) {
    struct usb_tt_route route = {0, 0};
    if (child_speed > 2) return route;
    if (parent_speed == 3) {
        route.slot = parent_slot;
        route.port = parent_port;
    } else {
        route.slot = parent_tt_slot;
        route.port = parent_tt_port;
    }
    return route;
}

static inline unsigned usb_tt_context_word(unsigned tt_slot, unsigned tt_port) {
    return tt_slot | (tt_port << 8);
}
