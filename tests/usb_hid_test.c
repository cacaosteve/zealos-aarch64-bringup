#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/usb_hid.h"

/* Standard, unnumbered button/X/Y/wheel report layouts, assembled as HID
 * descriptor items. Real QEMU descriptors are exercised by check-usb-input. */
static const uint8_t tablet[] = {
    0x05,1, 0x09,2, 0xa1,1,
    0x05,9, 0x19,1, 0x29,3, 0x15,0, 0x25,1, 0x75,1, 0x95,3, 0x81,2,
    0x75,5, 0x95,1, 0x81,1,
    0x05,1, 0x09,0x30, 0x09,0x31, 0x15,0, 0x26,0xff,0x7f,
    0x75,16, 0x95,2, 0x81,2,
    0x09,0x38, 0x15,0x81, 0x25,0x7f, 0x75,8, 0x95,1, 0x81,6, 0xc0
};
static const uint8_t mouse[] = {
    0x05,1, 0x09,2, 0xa1,1,
    0x05,9, 0x19,1, 0x29,3, 0x15,0, 0x25,1, 0x75,1, 0x95,3, 0x81,2,
    0x75,5, 0x95,1, 0x81,1,
    0x05,1, 0x09,0x30, 0x09,0x31, 0x09,0x38,
    0x15,0x81, 0x25,0x7f, 0x75,8, 0x95,3, 0x81,6, 0xc0
};
int main(void) {
    struct usb_hid_pointer h;
    int32_t x,y,z; int buttons;
    uint8_t r[] = {5,0xff,0x7f,0x00,0x40,0xff};
    assert(usb_hid_pointer_parse(&h,tablet,sizeof(tablet)));
    assert(h.absolute && h.report_bits == 48 && h.buttons_count == 3);
    assert(usb_hid_pointer_read(&h,r,sizeof(r),&x,&y,&z,&buttons));
    assert(x == 32767 && y == 16384 && z == -1 && buttons == 5);
    assert(!usb_hid_pointer_read(&h,r,sizeof(r)-1,&x,&y,&z,&buttons));
    assert(usb_hid_pointer_parse(&h,mouse,sizeof(mouse)));
    uint8_t m[] = {2,0x81,0x7f,1};
    assert(!h.absolute && usb_hid_pointer_read(&h,m,sizeof(m),&x,&y,&z,&buttons));
    assert(x == -127 && y == 127 && z == 1 && buttons == 2);
    uint8_t invalid[sizeof(tablet)+2];
    memcpy(invalid,tablet,sizeof(tablet)); invalid[sizeof(tablet)] = 0x85; invalid[sizeof(tablet)+1] = 1;
    assert(!usb_hid_pointer_parse(&h,invalid,sizeof(invalid))); /* report IDs */
    assert(!usb_hid_pointer_parse(&h,tablet,sizeof(tablet)-2)); /* incomplete item */
    uint8_t oversized[] = {0x75,32,0x96,0xff,0xff,0x81,2};
    assert(!usb_hid_pointer_parse(&h,oversized,sizeof(oversized)));
    uint8_t long_item[] = {0xfe,0xff,0xff};
    assert(!usb_hid_pointer_parse(&h,long_item,sizeof(long_item)));
    for (unsigned n = 0; n < sizeof(tablet); n++) {
        /* Every truncated prefix is safe, even when not a complete report. */
        if (usb_hid_pointer_parse(&h,tablet,n))
            assert(h.report_bits <= 512 && h.x.bits <= 31 && h.y.bits <= 31);
    }
    uint8_t mutated[sizeof(tablet)], report[64] = {0};
    for (unsigned pos = 0; pos < sizeof(tablet); pos++) {
        for (unsigned value = 0; value < 256; value++) {
            memcpy(mutated,tablet,sizeof(tablet)); mutated[pos] = value;
            if (usb_hid_pointer_parse(&h,mutated,sizeof(mutated))) {
                assert(h.report_bits <= 512 && h.x.bits <= 31 && h.y.bits <= 31);
                assert(usb_hid_pointer_read(&h,report,sizeof(report),&x,&y,&z,&buttons));
            }
        }
    }
    assert(usb_hid_key(4)==30 && usb_hid_key(29)==44 && usb_hid_key(40)==28);
    assert(usb_hid_key(79)==106 && usb_hid_key(80)==105 && usb_hid_key(81)==108 && usb_hid_key(82)==103);
    assert(usb_hid_key(224)==29 && usb_hid_key(229)==54 && !usb_hid_key(255));
    puts("PASS USB HID absolute/relative reports, signed wheel, bounds, rejected layouts and key usages");
}
