#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/pi4_usb_keyboard.h"
#include "../src/usb_hid.h"

/* Mouse interface first, boot keyboard interface 4 second. Alternate zero,
 * HID descriptor with report length 63, interrupt IN 0x82, 8 bytes/10 ms. */
static const uint8_t composite[] = {
    9,2,59,0,2,1,0,0x80,50,
    9,4,0,0,1,3,1,2,0, 9,0x21,0x11,1,0,1,0x22,50,0,
    7,5,0x81,3,4,0,10,
    9,4,4,0,1,3,1,1,0, 9,0x21,0x11,1,0,1,0x22,63,0,
    7,5,0x82,3,8,0,10
};
static unsigned emitted, usage[64], value[64];
static void emit(unsigned u, int down) {
    assert(emitted < 64u);
    usage[emitted] = u; value[emitted++] = (unsigned)down;
}

static void descriptor_tests(void) {
    struct pi4_usb_keyboard_interface k;
    assert(pi4_usb_keyboard_select(composite, sizeof(composite), &k));
    assert(k.found && k.interface == 4u && k.endpoint == 0x82u &&
           k.packet == 8u && k.interval == 10u && k.report_length == 63u);
    for (unsigned n = 0; n < sizeof(composite); n++)
        assert(!pi4_usb_keyboard_select(composite, n, &k));
    uint8_t d[sizeof(composite)];
    memcpy(d, composite, sizeof(d));
    d[40] = 0; /* Not Boot subclass: never interpret report-mode bytes as keys. */
    assert(!pi4_usb_keyboard_select(d, sizeof(d), &k));
    memcpy(d, composite, sizeof(d)); d[37] = 1; /* Nonzero alternate. */
    assert(!pi4_usb_keyboard_select(d, sizeof(d), &k));
    memcpy(d, composite, sizeof(d)); d[54] = 0x02; /* OUT instead of IN. */
    assert(!pi4_usb_keyboard_select(d, sizeof(d), &k));
    memcpy(d, composite, sizeof(d)); d[55] = 2; /* Bulk instead of interrupt. */
    assert(!pi4_usb_keyboard_select(d, sizeof(d), &k));
    memcpy(d, composite, sizeof(d)); d[56] = 7; /* Inadequate packet. */
    assert(!pi4_usb_keyboard_select(d, sizeof(d), &k));
    memcpy(d, composite, sizeof(d)); d[58] = 0; /* Invalid interval. */
    assert(!pi4_usb_keyboard_select(d, sizeof(d), &k));
    memcpy(d, composite, sizeof(d)); d[50] = d[51] = 0; /* Empty Report descriptor. */
    assert(!pi4_usb_keyboard_select(d, sizeof(d), &k));
    memcpy(d, composite, sizeof(d)); d[16] = 1; /* Two boot-keyboard owners. */
    d[31] = 8;
    assert(!pi4_usb_keyboard_select(d, sizeof(d), &k) && !k.found);
    /* Every single-byte mutation stays bounded; accepted selections must
     * remain a real interrupt-IN boot keyboard with an adequate packet. */
    for (unsigned pos = 0; pos < sizeof(d); pos++) {
        for (unsigned byte = 0; byte < 256u; byte++) {
            memcpy(d, composite, sizeof(d)); d[pos] = (uint8_t)byte;
            if (pi4_usb_keyboard_select(d, sizeof(d), &k))
                assert(k.found && (k.endpoint & 0xf0u) == 0x80u &&
                       k.packet >= 8u && k.packet <= 64u && k.interval && k.report_length);
            else assert(!k.found);
        }
    }
}

static void context_tests(void) {
    struct pi4_usb_keyboard_interface k = {1,4,0x82,8,10,63};
    assert(pi4_usb_keyboard_interval(1,1)==3 && pi4_usb_keyboard_interval(2,10)==6);
    assert(pi4_usb_keyboard_interval(1,255)==10 && pi4_usb_keyboard_interval(3,16)==15);
    assert(pi4_usb_keyboard_interval(3,17)<0 && pi4_usb_keyboard_interval(4,10)<0);
    for (unsigned size = 32; size <= 64; size *= 2) {
        for (unsigned speed = 1; speed <= 3; speed++) {
            _Alignas(64) uint8_t input[4096], output[4096];
            memset(input,0xa5,sizeof(input)); memset(output,0,sizeof(output));
            uint32_t *slot = (uint32_t *)(void *)output;
            slot[0] = 3u | speed << 20 | 1u << 27;
            slot[1] = 1u << 16; slot[2] = 1u | 3u << 8; slot[3] = 2u << 27 | 2u;
            k.interval = speed == 3u ? 7u : 10u;
            assert(!pi4_usb_keyboard_context_seed(input,sizeof(input),size,
                output,sizeof(output),speed,&k,0x404010000ull));
            uint32_t *control = (uint32_t *)(void *)input;
            uint32_t *in_slot = (uint32_t *)(void *)(input + size);
            uint32_t *ep = (uint32_t *)(void *)(input + 6u * size);
            assert(control[0]==0 && control[1]==0x21 && in_slot[0]>>27==5u);
            assert((in_slot[0]&0x07ffffffu)==(slot[0]&0x07ffffffu));
            assert(in_slot[1]==slot[1] && in_slot[2]==slot[2]); /* Root/TT preserved. */
            assert(ep[0]==6u<<16 && ep[1]==(8u<<16|7u<<3|3u<<1));
            assert(ep[2]==0x04010001u && ep[3]==4u && ep[4]==(8u<<16|8u));
            for(unsigned i=0;i<size;i++)assert(input[2u*size+i]==0); /* EP0 not added. */
            memcpy(output+5u*size,ep,size);
            ((uint32_t *)(void *)(output+5u*size))[0] |= 1u;
            assert(pi4_usb_keyboard_context_ok(output,sizeof(output),size,speed,&k,0x404010000ull));
            ((uint32_t *)(void *)(output+5u*size))[1] ^= 1u<<3;
            assert(!pi4_usb_keyboard_context_ok(output,sizeof(output),size,speed,&k,0x404010000ull));
            assert(pi4_usb_keyboard_context_seed(input,128,size,output,sizeof(output),speed,&k,0x404010000ull)<0);
            assert(pi4_usb_keyboard_context_seed(input,sizeof(input),size,output,sizeof(output),speed,&k,0x404010010ull)<0);
        }
    }
    _Alignas(16) uint32_t words[4];
    const unsigned request[3] = {0x0b21,0x03a1,0x0a21};
    for(unsigned step=0;step<3;step++) {
        assert(!pi4_usb_keyboard_setup_seed(words,step,4,1));
        assert(words[0]==request[step] && words[1]==(step==1 ? 0x10004u : 4u));
        assert(words[2]==8u && words[3]==((2u<<10)|64u|1u|(step==1 ? 3u<<16 : 0u)));
    }
    assert(pi4_usb_keyboard_setup_seed(words,3,4,1)<0);
    assert(pi4_usb_keyboard_setup_seed(words,0,256,1)<0);
}

static void report_tests(void) {
    struct pi4_usb_keyboard_state state = {{0}};
    uint8_t r[8] = {2,0,4,4,5,0,0,0}; /* Left shift + duplicate A + B. */
    assert(pi4_usb_keyboard_decode(&state,r,8,emit)==1);
    assert(emitted==3 && usage[0]==225 && value[0]==1 && usage[1]==4 && usage[2]==5);
    assert(usb_hid_key(usage[0])==42 && usb_hid_key(usage[1])==30 && usb_hid_key(usage[2])==48);
    r[1]=0xff; /* The boot report's reserved byte has no key meaning. */
    assert(pi4_usb_keyboard_decode(&state,r,8,emit)==1 && emitted==3);
    r[2]=5;r[3]=4;r[4]=0; /* Report order does not change key ownership. */
    assert(pi4_usb_keyboard_decode(&state,r,8,emit)==1 && emitted==3);
    struct pi4_usb_keyboard_state saved=state;
    r[0]=0;r[2]=1; /* Rollover preserves keys and modifiers. */
    assert(pi4_usb_keyboard_decode(&state,r,8,emit)==0 && emitted==3);
    assert(!memcmp(&state,&saved,sizeof(state)));
    assert(pi4_usb_keyboard_decode(&state,r,7,emit)<0 && emitted==3);
    memset(r,0,sizeof(r));
    assert(pi4_usb_keyboard_decode(&state,r,8,emit)==1 && emitted==6);
    assert(usage[3]==5 && usage[4]==4 && usage[5]==225 && value[5]==0);
    for(unsigned i=3;i<6;i++)assert(!value[i]); /* Key releases before modifier release. */
    for(unsigned i=0;i<1000;i++) {
        emitted=0;r[2]=11;assert(pi4_usb_keyboard_decode(&state,r,8,emit)==1);
        assert(emitted==1&&usage[0]==11&&value[0]==1); /* H -> Linux KEY_H. */
        assert(usb_hid_key(usage[0])==35);
        r[2]=0;assert(pi4_usb_keyboard_decode(&state,r,8,emit)==1);
        assert(emitted==2&&value[1]==0);
    }
}

int main(void) {
    descriptor_tests(); context_tests(); report_tests();
    puts("PASS Pi4 boot-keyboard composite selection, HID setup/context fields, modifiers, rollover and repeated key reports");
    return 0;
}
