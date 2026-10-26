#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/pi4_usb_bus.h"

static _Alignas(64) uint8_t ring[8192];
static unsigned consumer, completed, cleans;
static void consumer_at_clean(uint8_t *p,uint64_t n) {
    assert(p>=ring && p+n<=ring+sizeof(ring)); cleans++;
    while(consumer<254u) {
        const uint32_t *setup=(const uint32_t *)(const void *)(ring+consumer*16u);
        if(!(setup[3]&1u))break;
        assert(((setup[3]>>10)&63u)==2u && setup[2]==8u);
        unsigned length=setup[1]>>16, count=length?3u:2u;
        assert(consumer+count<=254u);
        for(unsigned i=1;i<count;i++) {
            const uint32_t *t=setup+i*4u;
            /* This simulated eager consumer runs at every publication. */
            assert(t[3]&1u);
            assert(((t[3]>>10)&63u)==(i==count-1u?4u:3u));
        }
        if(length)assert(setup[6]==length && (setup[7]&(1u<<16)));
        consumer+=count; completed++;
    }
}
static void td_publication(void) {
    memset(ring,0,sizeof(ring)); consumer=completed=cleans=0;
    unsigned next=0, count=0;
    uint64_t status;
    for(unsigned i=0;i<90u;i++) {
        unsigned length=i%2?0u:18u, before=next;
        uint8_t copy[8192];memcpy(copy,ring,sizeof(copy));
        int ok=pi4_usb_bus_control_td(ring,0x404010000ull,&next,
            length?0x80u:0x21u,length?6u:0x0bu,length?0x0100u:0u,7u,
            0x404020000ull,length,consumer_at_clean,&status);
        if(before>254u-(length?3u:2u)) {
            assert(!ok && next==before && !memcmp(copy,ring,sizeof(copy)));break;
        }
        assert(ok && status==0x404010000ull+(next-1u)*16u);
        assert(consumer==next && completed==++count);
    }
    assert(completed==90u && next==225u && cleans==315u);
    unsigned before=next;
    assert(!pi4_usb_bus_control_td(ring,0x404010000ull,&next,0x21,9,0,0,
        0x404020000ull,2,consumer_at_clean,&status)); /* OUT data isn't supported. */
    assert(next==before);
    next=252u;
    assert(!pi4_usb_bus_control_td(ring,0x404010000ull,&next,0x80,6,0,0,
        0x404020000ull,8,consumer_at_clean,&status) && next==252u);
}
static void regions(void) {
    uint64_t previous=0,offset;
    for(unsigned i=0;i<4;i++) {
        assert(pi4_usb_bus_region(0x30000,0x100000,i,&offset));
        assert(!(offset&8191u) && offset>=0x30000 &&
               offset+PI4_USB_BUS_DEVICE_BYTES<=0x100000);
        if(i)assert(offset>=previous+PI4_USB_BUS_DEVICE_BYTES);
        previous=offset;
    }
    assert(!pi4_usb_bus_region(0x30000,0x100000,4,&offset));
    assert(!pi4_usb_bus_region(UINT64_MAX,0x100000,0,&offset));
    assert(!pi4_usb_bus_region(0xf0000,0x100000,0,&offset));
    assert(!pi4_usb_bus_region(0x30000,0x100000,0,NULL));
}
static _Alignas(64) uint64_t dcbaa[8];
static unsigned disable_calls, clear_calls, disable_outcome;
static int disable_slot(uint32_t words[4],unsigned slot,unsigned *returned) {
    assert(slot==4u && dcbaa[slot]==0x404080000ull);
    assert(!words[0] && !words[1] && !words[2] &&
        words[3]==(PI4_XHCI_TRB_TYPE_DISABLE_SLOT<<10|4u<<24));
    disable_calls++;
    *returned=disable_outcome==2u ? 3u : slot;
    return disable_outcome!=0u;
}
static void clear_dcbaa(uint8_t *p,uint64_t bytes) {
    assert(disable_calls==1u && disable_outcome==1u && !dcbaa[4]);
    assert(p==(uint8_t *)(void *)&dcbaa[4] && bytes==8u);
    clear_calls++;
}
static void retire_storage(void) {
    for(unsigned outcome=0;outcome<3u;outcome++) {
        for(unsigned i=0;i<8u;i++)dcbaa[i]=0x404000000ull+i*0x20000u;
        uint64_t before[8];memcpy(before,dcbaa,sizeof(dcbaa));
        disable_calls=clear_calls=0;disable_outcome=outcome;
        /* Failure/timeout and wrong-slot completion cannot detach DCBAA.
         * The successful command is observed before the clear is published. */
        assert(pi4_usb_bus_retire_idle_slot(dcbaa,4u,disable_slot,clear_dcbaa)==
            (outcome==1u));
        assert(disable_calls==1u && clear_calls==(outcome==1u));
        for(unsigned i=0;i<8u;i++)
            assert(dcbaa[i]==(i==4u && outcome==1u ? 0u : before[i]));
    }
    disable_calls=clear_calls=0;
    assert(!pi4_usb_bus_retire_idle_slot(dcbaa,0u,disable_slot,clear_dcbaa));
    assert(!pi4_usb_bus_retire_idle_slot(dcbaa,7u,disable_slot,clear_dcbaa));
    assert(!pi4_usb_bus_retire_idle_slot(NULL,4u,disable_slot,clear_dcbaa));
    assert(!disable_calls && !clear_calls);
}
/* Model independent HS/SS controller output contexts, then fault each
 * readback category. Checks stay strict; diagnostics must identify corruption
 * without accepting it. Both 32/64-byte contexts and high DMA addresses matter. */
static void bulk_readback(void) {
    for(unsigned size=32u;size<=64u;size*=2u) {
        for(unsigned speed=1u;speed<=4u;speed++) {
            if(speed==2u)continue;
            unsigned packet=speed==1u?64u:speed==3u?512u:1024u;
            unsigned burst=speed==4u?7u:0u;
            _Alignas(64) uint8_t output[4096]={0};
            uint32_t *slot=(uint32_t *)(void *)output;
            uint32_t *in=(uint32_t *)(void *)(output+3u*size);
            uint32_t *out=(uint32_t *)(void *)(output+4u*size);
            slot[0]=4u<<27|speed<<20|3u;slot[3]=3u<<27|4u;
            in[0]=out[0]=1u;
            in[1]=packet<<16|burst<<8|6u<<3|3u<<1;
            out[1]=packet<<16|burst<<8|2u<<3|3u<<1;
            in[2]=0x04074001u;out[2]=0x04076001u;in[3]=out[3]=4u;
            in[4]=out[4]=3072u;
#define ERRORS() pi4_xhci_dma_bulk_context_errors(output,sizeof(output),size, \
    0x81u,packet,burst,0x404074000ull,2u,packet,burst,0x404076000ull)
            assert(!ERRORS());
            uint32_t *field[]={slot,slot+3,in,in+1,in+1,in+1,in,in+2,in+4,
                out,out+1,out+1,out+1,out,out+3,out+4};
            unsigned flips[]={1u<<27,1u<<27,1u,1u<<3,1u<<16,1u<<8,
                1u<<10,1u<<6,1u,1u,1u<<3,1u<<16,1u<<8,1u<<10,1u,1u};
            unsigned bits[]={1,1,2,3,4,5,6,7,8,2,3,4,5,6,7,8};
            for(unsigned i=0;i<16u;i++) {
                *field[i]^=flips[i];
                assert(ERRORS()==(1u<<bits[i]));
                assert(!pi4_xhci_dma_bulk_contexts_ok(output,sizeof(output),size,
                    0x81u,packet,burst,0x404074000ull,2u,packet,burst,0x404076000ull));
                *field[i]^=flips[i];
            }
            in[0]^=1u;out[1]^=1u<<16;
            assert(ERRORS()==((1u<<2)|(1u<<4)));
#undef ERRORS
        }
    }
    assert(pi4_xhci_dma_bulk_context_errors(NULL,4096,32,0x81,512,0,
        0x404074000ull,2,512,0,0x404076000ull)==1u);
}
static int seed_bulk(uint8_t *input,const uint8_t *output,unsigned size,
        unsigned speed,const struct pi4_xhci_dma_config_summary *s,
        unsigned in_burst,unsigned out_burst) {
    return pi4_xhci_dma_bulk_contexts_seed(input,4096u,size,output,4096u,
        8u,6u,0x50u,s->bot_interface_alt,s->bot_interface_endpoint_count,
        s->bot_bulk_in_address,s->bot_bulk_in_attributes,s->bot_bulk_in_max_packet,
        in_burst,s->bot_bulk_out_address,s->bot_bulk_out_attributes,
        s->bot_bulk_out_max_packet,out_burst,speed,0x404048000ull,0x40404a000ull);
}
static void descriptors_to_bulk_contexts(void) {
    /* The physical SanDisk's USB2 BOT shape has no SS companions. Parsing
     * leaves all four companion fields at UINT32_MAX, not zero. Exercise
     * that real API boundary instead of hand-populating a valid context. */
    const uint8_t usb2[]={9,2,32,0,1,1,0,0x80,50,
        9,4,0,0,2,8,6,0x50,0,
        7,5,0x81,2,0,2,0,7,5,2,2,0,2,0};
    const uint8_t ss[]={9,2,44,0,1,1,0,0x80,50,
        9,4,0,0,2,8,6,0x50,0,
        7,5,0x81,2,0,4,0,6,48,7,0,0,0,
        7,5,2,2,0,4,0,6,48,15,0,0,0};
    for(unsigned size=32u;size<=64u;size*=2u) {
        for(unsigned speed=1u;speed<=4u;speed++) {
            if(speed==2u)continue;
            struct pi4_xhci_dma_config_summary s;
            struct pi4_usb_bus_bulk_plan plan;
            uint8_t config[sizeof(ss)];
            unsigned bytes=speed==4u?sizeof(ss):sizeof(usb2);
            memcpy(config,speed==4u?ss:usb2,bytes);
            if(speed==1u) {config[22]=config[29]=64u;config[23]=config[30]=0;}
            assert(pi4_xhci_dma_configuration_descriptor_ok(config,bytes,4096u,&s));
            _Alignas(64) uint8_t input[4096]={0},output[4096]={0};
            uint32_t *slot=(uint32_t *)(void *)output;
            slot[0]=speed<<20|1u<<27|3u;slot[1]=1u<<16;slot[3]=2u<<27|4u;
            if(speed!=4u) {
                assert(s.bot_bulk_in_max_burst==UINT32_MAX &&
                    s.bot_bulk_out_max_burst==UINT32_MAX &&
                    s.bot_bulk_in_streams==UINT32_MAX && s.bot_bulk_out_streams==UINT32_MAX);
                /* Reproduce MULTI-USB2-1/2's pre-Configure failure. */
                assert(seed_bulk(input,output,size,speed,&s,
                    s.bot_bulk_in_max_burst,s.bot_bulk_out_max_burst)<0);
            }
            assert(pi4_usb_bus_bulk_plan(&s,speed,&plan));
            assert(plan.in_burst==(speed==4u?7u:0u) && plan.out_burst==(speed==4u?15u:0u));
            assert(!seed_bulk(input,output,size,speed,&s,plan.in_burst,plan.out_burst));
            /* Model Configure Endpoint output after consuming the real seeded
             * input. The readback must use the same speed-derived plan. */
            memcpy(output,input+size,size);
            memcpy(output+3u*size,input+4u*size,size);
            memcpy(output+4u*size,input+5u*size,size);
            slot[3]=3u<<27|4u;
            ((uint32_t *)(void *)(output+3u*size))[0]=1u;
            ((uint32_t *)(void *)(output+4u*size))[0]=1u;
            assert(pi4_xhci_dma_bulk_contexts_ok(output,4096u,size,
                s.bot_bulk_in_address,s.bot_bulk_in_max_packet,plan.in_burst,0x404048000ull,
                s.bot_bulk_out_address,s.bot_bulk_out_max_packet,plan.out_burst,0x40404a000ull));
            if(speed==4u) {
                s.bot_bulk_in_streams=1u;
                assert(!pi4_usb_bus_bulk_plan(&s,speed,&plan));
                s.bot_bulk_in_streams=0u;s.bot_bulk_out_max_burst=UINT32_MAX;
                assert(!pi4_usb_bus_bulk_plan(&s,speed,&plan));
            } else {
                assert(!pi4_usb_bus_bulk_plan(&s,2u,&plan));
                s.bot_bulk_in_max_burst=0u; /* Unexpected companion at USB2. */
                assert(!pi4_usb_bus_bulk_plan(&s,speed,&plan));
            }
        }
    }
    struct pi4_xhci_dma_config_summary s;
    struct pi4_usb_bus_bulk_plan plan;
    assert(pi4_xhci_dma_configuration_descriptor_ok(usb2,sizeof(usb2),4096u,&s));
    assert(!pi4_usb_bus_bulk_plan(&s,1u,&plan)); /* FS cannot use HS 512B MPS. */
    s.bot_bulk_in_max_packet=s.bot_bulk_out_max_packet=1024u;
    assert(!pi4_usb_bus_bulk_plan(&s,4u,&plan)); /* SS companions are mandatory. */
}
struct inquiry_fixture {
    unsigned mode,calls,hid_reports;
    uint8_t cbw[31];
    struct pi4_usb_keyboard_state keyboard;
    struct pi4_usb_mouse_state mouse;
};
static unsigned inquiry_key_events;
static void inquiry_key(unsigned usage,int down) {
    assert(usage==4u && (down==0 || down==1));inquiry_key_events++;
}
static int inquiry_phase(void *opaque,unsigned phase,uint8_t *buffer,unsigned bytes,
                         unsigned *actual) {
    struct inquiry_fixture *f=opaque;
    assert(phase==f->calls++ && phase<3u && bytes==(phase==0u?31u:phase==1u?36u:13u));
    /* Inject keyboard and pointer completions during every transport phase.
     * Storage and input have different slots even when endpoint DCIs match. */
    struct pi4_usb_bus_pending pending[2]={{0x404100000ull,2,3,8,8},{0x404200000ull,3,7,64,3}};
    unsigned got;
    assert(pi4_usb_bus_completion(pending,2,pending[0].trb,1u<<24,
        2u<<24|3u<<16|32u<<10,&got)==1 && got==8u);
    uint8_t key[8]={0,0,phase==1u?4u:0u};
    assert(pi4_usb_keyboard_decode(&f->keyboard,key,8u,inquiry_key)>=0);
    assert(pi4_usb_bus_completion(pending,2,pending[1].trb,13u<<24|61u,
        3u<<24|7u<<16|32u<<10,&got)==2 && got==3u);
    uint8_t mouse[3]={1,1,0xff};
    assert(pi4_usb_mouse_decode(&f->mouse,mouse,got,1000u,1000u));
    f->hid_reports+=2u;
    if(f->mode==phase+1u)return 0; /* Transport failure, no later phase allowed. */
    *actual=bytes;
    if(phase==0u) {
        assert(!memcmp(buffer,"USBC",4u) && buffer[8]==36u && buffer[12]==0x80u &&
            buffer[14]==6u && buffer[15]==0x12u && buffer[19]==36u);
        memcpy(f->cbw,buffer,31u);
        if(f->mode==11u)*actual=30u;
    } else if(phase==1u) {
        memset(buffer,0,bytes);buffer[3]=2u;buffer[4]=31u;
        if(f->mode==7u)buffer[0]=1u;
        if(f->mode==8u)*actual=35u;
        if(f->mode==10u)*actual=37u;
    } else {
        memset(buffer,0,bytes);memcpy(buffer,"USBS",4u);memcpy(buffer+4u,f->cbw+4u,4u);
        if(f->mode==4u)buffer[4]^=1u;
        if(f->mode==5u)buffer[8]=1u;
        if(f->mode==6u)buffer[12]=1u;
        if(f->mode==8u)buffer[8]=1u;
        if(f->mode==9u)*actual=12u;
    }
    return 1;
}
static void inquiry_transaction(void) {
    const unsigned step[]={7,2,3,4,5,5,5,6,6,5,3,2};
    const unsigned calls[]={3,1,2,3,3,3,3,3,3,3,2,1};
    for(unsigned mode=0;mode<12u;mode++) {
        struct inquiry_fixture f={.mode=mode,.mouse={.x=500,.y=500}};
        struct pi4_usb_bus_inquiry r;
        _Alignas(64) uint8_t buffers[3][64];
        memset(buffers,0xa5,sizeof(buffers));
        inquiry_key_events=0;
        int ok=pi4_usb_bus_inquiry_run(&f,inquiry_phase,buffers[0],buffers[1],buffers[2],&r);
        assert(ok==(mode==0u) && r.step==step[mode] && f.calls==calls[mode]);
        assert(f.hid_reports==2u*f.calls && f.mouse.moves==f.calls);
        assert(inquiry_key_events==(f.calls>1u?f.calls-1u:0u));
        if(!mode)assert(r.csw_ok && r.identity_ok && r.actual[0]==31u &&
            r.actual[1]==36u && r.actual[2]==13u && !memcmp(r.csw,"USBS",4u));
        if(mode==4u)assert(r.csw[4]==(uint8_t)(PI4_XHCI_DMA_BOT_INQUIRY_TAG^1u));
        if(mode==7u)assert(r.csw_ok && !r.identity_ok && r.data[0]==1u);
        if(mode==8u)assert(r.csw_ok && !r.identity_ok && r.actual[1]==35u);
        for(unsigned i=0;i<3u;i++)for(unsigned j=(i==0u?31u:i==1u?36u:13u);j<64u;j++)
            assert(buffers[i][j]==0xa5u); /* No wire phase overruns its buffer. */
    }
}
struct stall_fixture {
    unsigned calls,fail,host_state,host_toggle,device_toggle,hid_reports;
    uint64_t dequeue;
    struct pi4_usb_keyboard_state keyboard;
    struct pi4_usb_mouse_state mouse;
};
static void stall_clean(uint8_t *p,uint64_t n) {(void)p;(void)n;}
static int stall_action(void *opaque,unsigned step,const struct pi4_usb_bus_stall *p) {
    struct stall_fixture *f=opaque;
    assert(step==f->calls++);
    _Alignas(16) uint32_t words[4];
    /* Real encoders, exact command target and order, without retrying CBW/data. */
    if(step==0u) {
        assert(f->host_state==2u && pi4_usb_bus_stall_command(p,step,words));
        assert(!words[0] && !words[1] && !words[2] && words[3]==(14u<<10|3u<<16|4u<<24));
    } else if(step==1u) {
        assert(f->host_state==3u && !f->host_toggle);
        _Alignas(64) uint8_t control[8192]={0};unsigned next=0;uint64_t status;
        assert(pi4_usb_bus_control_td(control,0x404080000ull,&next,2u,1u,0u,p->endpoint,
            0u,0u,stall_clean,&status) && next==2u && status==0x404080010ull);
        const uint32_t *setup=(const uint32_t *)(const void *)control;
        assert(setup[0]==0x102u && setup[1]==0x81u && setup[2]==8u &&
            setup[3]==(2u<<10|1u<<6|1u) && setup[7]==(4u<<10|1u<<16|1u<<5|1u));
    } else {
        assert(step==2u && f->host_state==3u && !f->host_toggle && !f->device_toggle);
        assert(pi4_usb_bus_stall_command(p,step,words) &&
            words[0]==(uint32_t)(p->dequeue|p->cycle) && words[1]==4u && !words[2] &&
            words[3]==(16u<<10|3u<<16|4u<<24));
    }
    struct pi4_usb_bus_pending pending[2]={{0x404100000ull,2,3,8,8},{0x404200000ull,3,7,64,3}};
    unsigned actual;
    assert(pi4_usb_bus_completion(pending,2,pending[0].trb,1u<<24,
        2u<<24|3u<<16|32u<<10,&actual)==1);
    uint8_t key[8]={0,0,step==1u?4u:0u};
    assert(pi4_usb_keyboard_decode(&f->keyboard,key,8u,inquiry_key)>=0);
    assert(pi4_usb_bus_completion(pending,2,pending[1].trb,13u<<24|61u,
        3u<<24|7u<<16|32u<<10,&actual)==2);
    uint8_t mouse[3]={1,1,0};
    assert(pi4_usb_mouse_decode(&f->mouse,mouse,actual,1000,1000));f->hid_reports+=2;
    if(f->fail==step+1u)return 0;
    if(!step) {f->host_state=3u;f->host_toggle=0;}
    if(step==1u)f->device_toggle=0;
    if(step==2u)f->dequeue=p->dequeue;
    return 1;
}
static void bulk_stall_recovery(void) {
    struct pi4_usb_bus_stall p;
    const uint64_t bus=0x40404a000ull;
    const uint32_t control=4u<<24|3u<<16|32u<<10|1u;
    assert(pi4_usb_bus_stall_plan(bus+32u,0x06000008u,control,bus,2u,4u,3u,8u,1u,NULL,&p));
    assert(!p.actual && p.endpoint==0x81u && p.next==3u && p.cycle==1u && p.dequeue==bus+48u);
    for(unsigned fail=0;fail<=3u;fail++) {
        struct stall_fixture f={.fail=fail,.host_state=2,.host_toggle=1,.device_toggle=1,
            .dequeue=bus+32u,.mouse={.x=500,.y=500}};
        unsigned stage=0;inquiry_key_events=0;
        int ok=pi4_usb_bus_stall_run(&p,&f,stall_action,&stage);
        assert(ok==!fail && stage==(fail?fail:4u) && f.calls==(fail?fail:3u));
        assert(f.hid_reports==f.calls*2u && f.mouse.moves==f.calls);
        assert(f.dequeue==(fail?bus+32u:bus+48u));
        if(ok)assert(!f.host_toggle && !f.device_toggle && f.host_state==3u);
    }
    /* Both event cycles and transfer-ring producer wrap/DCS must be distinct. */
    for(unsigned cycle=0;cycle<2u;cycle++)for(unsigned index=0;index<255u;index++) {
        unsigned next=index==254u?0u:index+1u, after=cycle^(index==254u);
        struct pi4_xhci_dma_producer producer={next,after,index==254u};
        assert(pi4_usb_bus_stall_plan(bus+index*16u,6u<<24|3u,control^1u,bus,index,
            4,3,8,cycle,&producer,&p) && p.actual==5u && p.next==next && p.cycle==after);
        producer.cycle^=1u;
        assert(!pi4_usb_bus_stall_plan(bus+index*16u,6u<<24|3u,control,bus,index,
            4,3,8,cycle,&producer,&p));
    }
    for(unsigned bad=0;bad<8u;bad++) {
        uint64_t parameter=bus+32u, base=bus;
        uint32_t status=0x06000008u,c=control;
        unsigned requested=8u;
        if(bad==0u)parameter+=16u;if(bad==1u)c^=1u<<24;
        if(bad==2u)c^=1u<<16;if(bad==3u)c|=1u<<2;
        if(bad==4u)status=0x06000009u;if(bad==5u)status=0x01000000u;
        if(bad==6u)requested=0u;if(bad==7u)base++;
        assert(!pi4_usb_bus_stall_plan(parameter,status,c,base,2u,4u,3u,
            requested,1u,NULL,&p));
    }
    assert(!pi4_usb_bus_stall_plan(bus+254u*16u,6u<<24,control,bus,254,4,3,8,1,NULL,&p));
    /* The recovered DATA stall permits CSW, not success of the SCSI command.
     * Preserve CSW failure and residue, decode Unit Attention, then build a
     * fresh read-only retry with a new tag; no failed data is parsed as capacity. */
    uint8_t csw[13]={0x55,0x53,0x42,0x53,2,0x49,0x50,0x5a,8,0,0,0,1};
    unsigned scsi=0;
    assert(pi4_xhci_dma_bot_csw_valid(csw,13,0x5a504902u,8,0,&scsi) && scsi==1u);
    uint8_t sense[18]={0x70,0,6,0,0,0,0,10,0,0,0,0,0x29,0,0,0,0,0};
    unsigned key,asc,ascq;
    assert(pi4_xhci_dma_bot_sense_parse(sense,18,&key,&asc,&ascq) && key==6u && asc==0x29u &&
        !ascq && pi4_xhci_dma_bot_sense_retryable(key,asc,ascq));
    uint8_t cbw[31];
    assert(!pi4_xhci_dma_bot_sense_cbw_seed(cbw,31,0x5a504903u) && cbw[15]==3u && cbw[8]==18u);
    assert(!pi4_xhci_dma_bot_capacity10_cbw_seed(cbw,31,0x5a504904u) && cbw[15]==0x25u && cbw[8]==8u);
    const uint8_t capacity[8]={0x07,0x4d,0xe1,0x83,0,0,2,0};
    uint32_t lba,block;uint64_t bytes;
    assert(pi4_xhci_dma_bot_capacity10_parse(capacity,8,&lba,&block,&bytes) &&
        lba==0x074de183u && block==512u && bytes==62742792192ull);
    csw[12]=2u;assert(!pi4_xhci_dma_bot_csw_valid(csw,13,0x5a504902u,8,0,&scsi));
}
static unsigned emitted;
static void emit(unsigned usage,int down) {assert(usage==4);(void)down;emitted++;}
static void interleaved(void) {
    struct pi4_usb_bus_pending pending[4]={
        {0x404100000ull,2,3,8,8}, {0x404200000ull,3,7,64,3}, {0}, {0}};
    struct pi4_usb_keyboard_state k={0};
    struct pi4_usb_mouse_state m={.x=500,.y=500};
    struct pi4_xhci_dma_event_cursor cursor={0,1};
    unsigned key_reports=0,mouse_reports=0,storage_reports=0,actual;
    emitted=0;
    for(unsigned i=0;i<2048u;i++) {
        for(unsigned kind=0;kind<4u;kind++) {
            /* Storage shares endpoint numbers with HID but owns slot 4.
             * Command completions and bulk data/CSWs must reach their waiter. */
            uint64_t parameter=kind<2u?pending[kind].trb:0x404300000ull+i*16u;
            uint32_t control=(kind==2?33u:32u)<<10|
                (kind<2u?pending[kind].slot:4u)<<24|
                (kind<2u?pending[kind].dci:3u)<<16|cursor.cycle;
            uint32_t status=kind==1u?(13u<<24|61u):1u<<24;
            assert(!pi4_xhci_dma_event_ready(control^1u,&cursor));
            assert(pi4_xhci_dma_event_ready(control,&cursor));
            int owner=pi4_usb_bus_completion(pending,4,parameter,status,control,&actual);
            if(kind==0) {
                assert(owner==1 && actual==8);
                uint8_t r[8]={0,0,(i&1)?0:4};
                assert(pi4_usb_keyboard_decode(&k,r,8,emit)==1);key_reports++;
                pending[0].trb+=16;
            } else if(kind==1) {
                assert(owner==2 && actual==3);
                uint8_t r[3]={(i&1)?0:1,(i&1)?0xff:1,0};
                assert(pi4_usb_mouse_decode(&m,r,3,1000,1000));mouse_reports++;
                pending[1].trb+=16;
            } else {assert(owner==0);storage_reports++;}
            (void)pi4_xhci_dma_event_cursor_advance(&cursor);
        }
    }
    assert(key_reports==2048 && mouse_reports==2048 && emitted==2048 &&
        storage_reports==4096 && cursor.index==0 && cursor.cycle==1 &&
        m.x==500 && m.presses==1024 && m.releases==1024);
    uint32_t control=32u<<10|2u<<24|3u<<16;
    assert(pi4_usb_bus_completion(pending,4,pending[0].trb+16,1u<<24,control,&actual)<0);
    assert(pi4_usb_bus_completion(pending,4,pending[0].trb,13u<<24|1,control,&actual)<0);
    assert(pi4_usb_bus_completion(pending,4,pending[0].trb,6u<<24,control,&actual)<0);
    control=32u<<10|3u<<24|7u<<16;
    assert(pi4_usb_bus_completion(pending,4,pending[1].trb,13u<<24|62,control,&actual)<0);
}
int main(void) {
    regions();td_publication();descriptors_to_bulk_contexts();bulk_readback();retire_storage();inquiry_transaction();bulk_stall_recovery();interleaved();
    puts("PASS Pi4 bus regions/control/contexts, retained INQUIRY, exact stall/reset/clear-halt/dequeue/DCS and bounded failure ordering with HID, 8192 mixed completions");
}
