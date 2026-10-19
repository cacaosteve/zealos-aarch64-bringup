#include <assert.h>
#include <stdio.h>
#include "../src/usb_route.h"

int main(void) {
    /* A full-speed hub directly below HS slot 5 routes through port 1. */
    struct usb_tt_route fs_hub = usb_tt_child_route(3, 5, 0, 0, 1, 1);
    assert(fs_hub.slot == 5 && fs_hub.port == 1);

    /* LS/FS descendants keep the upstream HS TT and its port across FS hubs. */
    struct usb_tt_route fs_device = usb_tt_child_route(
        1, 6, fs_hub.slot, fs_hub.port, 1, 3);
    struct usb_tt_route ls_device = usb_tt_child_route(
        1, 6, fs_hub.slot, fs_hub.port, 2, 4);
    assert(fs_device.slot == 5 && fs_device.port == 1);
    assert(ls_device.slot == 5 && ls_device.port == 1);

    /* A direct child of the HS hub uses its own downstream port. */
    struct usb_tt_route direct = usb_tt_child_route(3, 5, 0, 0, 2, 4);
    assert(direct.slot == 5 && direct.port == 4);

    /* A nearer HS hub supplies the TT for its own LS/FS descendants. */
    struct usb_tt_route high_speed_hub = usb_tt_child_route(3, 7, 5, 1, 3, 2);
    struct usb_tt_route nested_fs_device = usb_tt_child_route(
        3, 7, high_speed_hub.slot, high_speed_hub.port, 1, 3);
    assert(high_speed_hub.slot == 0 && high_speed_hub.port == 0);
    assert(nested_fs_device.slot == 7 && nested_fs_device.port == 3);

    /* HS and SuperSpeed children do not carry TT fields. */
    struct usb_tt_route hs_device = usb_tt_child_route(3, 5, 0, 0, 3, 1);
    struct usb_tt_route ss_device = usb_tt_child_route(4, 9, 0, 0, 1, 2);
    assert(hs_device.slot == 0 && hs_device.port == 0);
    assert(ss_device.slot == 0 && ss_device.port == 0);

    assert(usb_tt_context_word(5, 4) == 0x405);
    puts("PASS USB2 TT selection, inherited routes, speed filtering and xHCI context packing");
}
