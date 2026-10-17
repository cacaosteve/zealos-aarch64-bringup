/* Small, bounded HID decoder, adapted from YDE KUsbHid.ZC (public domain).
 * Boot keyboards and one unnumbered pointer report; reject other layouts.
 * Pure decoding stays independent of xHCI/DMA so malformed inputs are testable. */
#pragma once
#include <stdint.h>
#include <stddef.h>

struct usb_hid_axis { unsigned off, bits; int32_t min, max; };
struct usb_hid_pointer {
    struct usb_hid_axis x, y, wheel;
    unsigned buttons_off, buttons_count, report_bits;
    int absolute;
};
static uint32_t usb_hid_bits(const uint8_t *p, unsigned off, unsigned bits) {
    uint32_t v = 0;
    for (unsigned i = 0; i < bits; i++)
        v |= (uint32_t)((p[(off + i) / 8] >> ((off + i) % 8)) & 1) << i;
    return v;
}
static int32_t usb_hid_signed(uint32_t v, unsigned bits) {
    if (!bits || bits > 31) return 0;
    return (int32_t)(v ^ (1u << (bits - 1))) - (int32_t)(1u << (bits - 1));
}
static int usb_hid_pointer_parse(struct usb_hid_pointer *out,
                                  const uint8_t *desc, size_t len) {
    struct usb_hid_pointer h = {0};
    unsigned page = 0, size = 0, count = 0, bit = 0, usages[32], n = 0;
    unsigned usage_min = 0, usage_max = 0;
    int have_range = 0, relative = -1;
    int32_t min = 0, max = 0;
    for (size_t pos = 0; pos < len;) {
        unsigned b = desc[pos++], bytes = b & 3, tag = b & 0xfc;
        if (b == 0xfe) return 0; /* long items are outside this subset */
        if (bytes == 3) bytes = 4;
        if (pos + bytes > len) return 0;
        uint32_t v = 0;
        for (unsigned i = 0; i < bytes; i++) v |= (uint32_t)desc[pos++] << (i * 8);
        if (tag == 0x84 || tag == 0xa4 || tag == 0xb4) return 0; /* IDs/push/pop */
        if (tag == 0x04) page = v;
        else if (tag == 0x14) min = bytes ? (int32_t)(v << (32 - bytes * 8)) >> (32 - bytes * 8) : 0;
        else if (tag == 0x24) max = min < 0 && bytes ? (int32_t)(v << (32 - bytes * 8)) >> (32 - bytes * 8) : (int32_t)v;
        else if (tag == 0x74) size = v;
        else if (tag == 0x94) count = v;
        else if (tag == 0x08) {
            if (n == 32) return 0;
            usages[n++] = bytes == 4 ? v : (page << 16) | v;
        } else if (tag == 0x18) { usage_min = v; have_range = 1; }
        else if (tag == 0x28) usage_max = v;
        else if (tag == 0x80) {
            if (!size || size > 32 || count > 64 || size * count > 512 - bit) return 0;
            for (unsigned i = 0; i < count; i++) {
                unsigned usage = i < n ? usages[i] :
                    have_range && usage_min + i <= usage_max ? (page << 16) | (usage_min + i) : 0;
                if (v & 1) continue; /* constant padding has no usages */
                if (!(v & 2)) return 0; /* pointer fields must be variable */
                if (usage == 0x10030 || usage == 0x10031 || usage == 0x10038) {
                    if (size > 31 || max <= min) return 0;
                    struct usb_hid_axis *a = usage == 0x10030 ? &h.x : usage == 0x10031 ? &h.y : &h.wheel;
                    if (a->bits) return 0;
                    *a = (struct usb_hid_axis){bit + i * size, size, min, max};
                    if (usage != 0x10038) {
                        int rel = !!(v & 4);
                        if (relative != -1 && relative != rel) return 0;
                        relative = rel;
                    }
                } else if ((usage >> 16) == 9 && size == 1 && (usage & 65535) <= 3) {
                    if (!h.buttons_count) h.buttons_off = bit + i;
                    h.buttons_count++;
                }
            }
            bit += size * count;
        }
        /* Every main item consumes local usages, including collections/output. */
        if ((b & 0x0c) == 0) { n = 0; have_range = 0; usage_min = usage_max = 0; }
    }
    if (!h.x.bits || !h.y.bits || h.buttons_count > 3) return 0;
    h.absolute = !relative;
    h.report_bits = bit;
    *out = h;
    return 1;
}
static int usb_hid_pointer_read(const struct usb_hid_pointer *h,
                                 const uint8_t *report, size_t len,
                                 int32_t *x, int32_t *y, int32_t *wheel, int *buttons) {
    if (len * 8 < h->report_bits) return 0;
    uint32_t vx = usb_hid_bits(report, h->x.off, h->x.bits);
    uint32_t vy = usb_hid_bits(report, h->y.off, h->y.bits);
    *x = h->x.min < 0 ? usb_hid_signed(vx, h->x.bits) : (int32_t)vx;
    *y = h->y.min < 0 ? usb_hid_signed(vy, h->y.bits) : (int32_t)vy;
    *wheel = h->wheel.bits ? usb_hid_signed(usb_hid_bits(report, h->wheel.off, h->wheel.bits), h->wheel.bits) : 0;
    *buttons = (int)usb_hid_bits(report, h->buttons_off, h->buttons_count);
    return 1;
}
/* USB page-7 usage -> Linux KEY_* used by the existing shared OS decoder. */
static unsigned usb_hid_key(unsigned usage) {
    static const uint8_t letters[] = {30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44};
    if (usage >= 4 && usage <= 29) return letters[usage - 4];
    if (usage >= 30 && usage <= 38) return usage - 28;
    static const uint8_t rest[] = {11,28,1,14,15,57,12,13,26,27,43,0,39,40,41,51,52,53,58,
        59,60,61,62,63,64,65,66,67,68,87,88,99,70,119,110,102,104,111,107,109,106,105,108,103};
    if (usage >= 39 && usage < 39 + sizeof(rest)) return rest[usage - 39];
    if (usage == 224) return 29;
    if (usage == 225) return 42;
    if (usage == 226) return 56;
    if (usage == 227) return 125;
    if (usage == 228) return 97;
    if (usage == 229) return 54;
    if (usage == 230) return 100;
    if (usage == 231) return 126;
    return 0;
}
