#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/pi4_usb_mouse.h"

static const uint8_t composite[] = {
    9,2,59,0,2,1,0,0x80,50,
    9,4,4,0,1,3,1,1,0, 9,0x21,0x11,1,0,1,0x22,63,0,
    7,5,0x82,3,8,0,10,
    9,4,7,0,1,3,1,2,0, 9,0x21,0x11,1,0,1,0x22,50,0,
    7,5,0x81,3,4,0,10
};
static void descriptors(void) {
    struct pi4_usb_keyboard_interface m;
    assert(pi4_usb_mouse_select(composite,sizeof(composite),&m));
    assert(m.found && m.interface==7 && m.endpoint==0x81 && m.packet==4 &&
           m.interval==10 && m.report_length==50);
    assert(pi4_usb_keyboard_select(composite,sizeof(composite),&m));
    assert(m.interface==4 && m.endpoint==0x82 && m.packet==8);
    for(unsigned n=0;n<sizeof(composite);n++)
        assert(!pi4_usb_mouse_select(composite,n,&m) && !m.found);
    uint8_t d[sizeof(composite)];
    const unsigned positions[]={37,39,40,41,48,50,54,55,56,58};
    const unsigned values[]={1,0,0,0,0,0,1,2,2,0};
    for(unsigned i=0;i<sizeof(positions)/sizeof(positions[0]);i++) {
        memcpy(d,composite,sizeof(d)); d[positions[i]]=values[i];
        assert(!pi4_usb_mouse_select(d,sizeof(d),&m) && !m.found);
    }
    memcpy(d,composite,sizeof(d)); d[16]=2; /* Ambiguous boot mice. */
    assert(!pi4_usb_mouse_select(d,sizeof(d),&m) && !m.found);
    for(unsigned pos=0;pos<sizeof(d);pos++) for(unsigned byte=0;byte<256;byte++) {
        memcpy(d,composite,sizeof(d)); d[pos]=(uint8_t)byte;
        if(pi4_usb_mouse_select(d,sizeof(d),&m))
            assert(m.found && (m.endpoint&0xf0)==0x80 && m.packet>=3 &&
                   m.packet<=64 && m.interval && m.report_length);
        else assert(!m.found);
    }
}
static void contexts(void) {
    _Alignas(64) uint8_t input[4096],output[4096];
    struct pi4_usb_keyboard_interface m={1,7,0x81,4,10,50};
    for(unsigned size=32;size<=64;size*=2) for(unsigned speed=1;speed<=3;speed++) {
        memset(output,0,sizeof(output));
        uint32_t *slot=(uint32_t *)(void *)output;
        slot[0]=3u|speed<<20|1u<<27; slot[1]=1u<<16;
        slot[2]=1u|3u<<8; slot[3]=2u<<27|2u;
        assert(!pi4_usb_mouse_context_seed(input,sizeof(input),size,output,
            sizeof(output),speed,&m,0x404010000ull));
        uint32_t *control=(uint32_t *)(void *)input;
        uint32_t *in_slot=(uint32_t *)(void *)(input+size);
        uint32_t *ep=(uint32_t *)(void *)(input+4*size);
        assert(control[0]==0 && control[1]==9 && in_slot[0]>>27==3);
        assert((in_slot[0]&0x07ffffff)==(slot[0]&0x07ffffff));
        assert(in_slot[1]==slot[1] && in_slot[2]==slot[2]);
        assert(ep[0]==(speed==3?9u:6u)<<16);
        assert(ep[1]==(4u<<16|7u<<3|3u<<1));
        assert(ep[2]==0x04010001 && ep[3]==4 && ep[4]==(4u<<16|4u));
        for(unsigned i=0;i<size;i++) assert(input[2*size+i]==0);
        memcpy(output+3*size,ep,size);
        ((uint32_t *)(void *)(output+3*size))[0]|=1;
        assert(pi4_usb_keyboard_context_ok(output,sizeof(output),size,speed,&m,
                                          0x404010000ull));
        assert(pi4_usb_keyboard_context_seed(input,sizeof(input),size,output,
            sizeof(output),speed,&m,0x404010000ull)<0); /* Keyboard still needs 8. */
        assert(pi4_usb_mouse_context_seed(input,4*size-1,size,output,
            sizeof(output),speed,&m,0x404010000ull)<0);
        m.packet=9;
        if(speed==2) assert(pi4_usb_mouse_context_seed(input,sizeof(input),size,
            output,sizeof(output),speed,&m,0x404010000ull)<0);
        m.packet=4;
    }
    m.packet=3;
    assert(!pi4_usb_mouse_context_seed(input,sizeof(input),64,output,
        sizeof(output),3,&m,0x404010000ull));
    assert(pi4_usb_mouse_context_seed(input+1,sizeof(input)-1,64,output,
        sizeof(output),3,&m,0x404010000ull)<0);
}
static void reports(void) {
    struct pi4_usb_mouse_state m={.x=50,.y=50};
    uint8_t r[64]={0xff,127,0x81,0x7f}; /* Ignore high buttons / wheel extension. */
    assert(pi4_usb_mouse_decode(&m,r,4,100,100));
    assert(m.x==99 && m.y==0 && m.buttons==7 && m.seen_buttons==7 && m.presses==1);
    r[0]=0; r[1]=0x80; r[2]=127;
    assert(pi4_usb_mouse_decode(&m,r,3,100,100));
    assert(m.x==0 && m.y==99 && !m.buttons && m.releases==1 && m.moves==2);
    struct pi4_usb_mouse_state saved=m;
    for(unsigned n=0;n<3;n++) assert(!pi4_usb_mouse_decode(&m,r,n,100,100));
    assert(!pi4_usb_mouse_decode(&m,r,65,100,100));
    assert(!pi4_usb_mouse_decode(&m,r,3,0,100));
    assert(!pi4_usb_mouse_decode(&m,NULL,3,100,100));
    assert(!memcmp(&m,&saved,sizeof(m)));
    memset(r,0,sizeof(r));
    assert(pi4_usb_mouse_decode(&m,r,64,1,1) && !m.x && !m.y);
    for(unsigned byte=0;byte<256;byte++) {
        m.x=m.y=1000; r[1]=r[2]=(uint8_t)byte;
        assert(pi4_usb_mouse_decode(&m,r,3,2000,2000));
        unsigned expected=1000+(byte<128?(int)byte:(int)byte-256);
        assert(m.x==expected && m.y==expected);
    }
}
static void completion(void) {
    unsigned actual;
    uint64_t trb=0x404010000ull;
    uint32_t control=32u<<10|3u<<16|2u<<24;
    for(unsigned cycle=0;cycle<2;cycle++) {
        assert(pi4_usb_mouse_event_ok(trb,13u<<24|1u,control|cycle,trb,2,3,4,&actual));
        assert(actual==3);
        assert(pi4_usb_mouse_event_ok(trb,1u<<24,control|cycle,trb,2,3,4,&actual));
        assert(actual==4);
        assert(!pi4_usb_mouse_event_ok(trb,13u<<24|2u,control|cycle,trb,2,3,4,&actual));
        assert(!pi4_usb_mouse_event_ok(trb+16,1u<<24,control|cycle,trb,2,3,4,&actual));
        assert(!pi4_usb_mouse_event_ok(trb,1u<<24,control|cycle,trb,3,3,4,&actual));
        assert(!pi4_usb_mouse_event_ok(trb,1u<<24,control|cycle,trb,2,5,4,&actual));
        assert(!pi4_usb_mouse_event_ok(trb,6u<<24,control|cycle,trb,2,3,4,&actual));
    }
    /* Run the actual decoder and completion validator across 20 event laps.
     * A quiet interval does not produce an owned event or release a button. */
    struct pi4_xhci_dma_event_cursor cursor={0,1};
    struct pi4_usb_mouse_state m={.x=100,.y=100};
    for(unsigned i=0;i<5120;i++) {
        uint8_t r[4]={(i&1)?0:1, (i&1)?0xff:1, 0, 0xa5};
        assert(!pi4_xhci_dma_event_ready(control|(!cursor.cycle),&cursor));
        assert(pi4_xhci_dma_event_ready(control|cursor.cycle,&cursor));
        assert(pi4_usb_mouse_event_ok(trb,13u<<24|1,control|cursor.cycle,
                                     trb,2,3,4,&actual));
        assert(pi4_usb_mouse_decode(&m,r,actual,1000,1000));
        (void)pi4_xhci_dma_event_cursor_advance(&cursor);
    }
    assert(cursor.index==0 && cursor.cycle==1 && m.reports==5120 &&
           m.presses==2560 && m.releases==2560 && m.x==100 && m.y==100);
}
int main(void) {
    descriptors(); contexts(); reports(); completion();
    puts("pi4 boot mouse selection/context/report/completion tests passed");
}
