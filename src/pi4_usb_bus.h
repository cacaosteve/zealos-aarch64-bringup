/* Bounded USB2 bus resources and completion routing, independent of MMIO. */
#pragma once
#include "pi4_usb_mouse.h"
#define PI4_USB_BUS_CHILDREN 4u
#define PI4_USB_BUS_DEVICE_BYTES 40960u

struct pi4_usb_bus_bulk_plan { unsigned in_burst, out_burst; };

struct pi4_usb_bus_stall {
    uint64_t ring_bus, failed, dequeue;
    unsigned slot, dci, endpoint, requested, actual, next, cycle;
    uint32_t status, control;
};
/* The caller consumed an owned event for one outstanding Normal data-IN TD.
 * Recover only an exact Stall Error; timeout, identity or residual corruption
 * must never be converted to a short success. The producer already advanced
 * when the failed TD was published. Bootstrap rings haven't wrapped yet. */
static int pi4_usb_bus_stall_plan(uint64_t parameter,uint32_t status,uint32_t control,
        uint64_t ring_bus,unsigned index,unsigned slot,unsigned dci,
        unsigned requested,unsigned trb_cycle,const struct pi4_xhci_dma_producer *producer,
        struct pi4_usb_bus_stall *out) {
    unsigned next=index+1u,cycle=trb_cycle;
    if(!out || (ring_bus&63u) || ring_bus>UINT64_MAX-4096u || index>=255u ||
        !slot || slot>PI4_XHCI_DMA_MAX_SLOTS || dci<3u || dci>31u || !(dci&1u) ||
        !requested || requested>PI4_XHCI_DMA_BOT_MAX_BLOCK_BYTES || trb_cycle>1u ||
        parameter!=ring_bus+index*16u || (status>>24)!=6u ||
        (status&0xffffffu)>requested || (control&(1u<<2)) ||
        ((control>>10)&63u)!=PI4_XHCI_TRB_TYPE_TRANSFER_EVT ||
        (control>>24)!=slot || ((control>>16)&31u)!=dci)return 0;
    if(next==255u) {next=0u;cycle^=1u;}
    if(producer) {
        if(producer->index!=next || producer->cycle!=cycle)return 0;
    } else if(trb_cycle!=1u || index>=254u)return 0;
    *out=(struct pi4_usb_bus_stall){.ring_bus=ring_bus,.failed=parameter,
        .dequeue=ring_bus+next*16u,.slot=slot,.dci=dci,.endpoint=0x80u|(dci>>1),
        .requested=requested,.actual=requested-(status&0xffffffu),.next=next,.cycle=cycle,
        .status=status,.control=control};
    return 1;
}

static int pi4_usb_bus_stall_command(const struct pi4_usb_bus_stall *p,
        unsigned step,uint32_t words[4]) {
    if(!p || !words || ((uintptr_t)words&15u) || !p->slot ||
        p->slot>PI4_XHCI_DMA_MAX_SLOTS || p->dci<3u || p->dci>31u ||
        !(p->dci&1u) || p->cycle>1u || (p->dequeue&15u) || (step!=0u && step!=2u))return 0;
    words[0]=step ? (uint32_t)(p->dequeue|p->cycle) : 0u;
    words[1]=step ? (uint32_t)(p->dequeue>>32) : 0u;
    words[2]=0; /* No streams. Reset Endpoint TSP=0 resets the USB2 toggle. */
    words[3]=(step?16u:14u)<<10|p->dci<<16|p->slot<<24;
    return 1; /* The command producer supplies its current cycle bit. */
}
typedef int (*pi4_usb_bus_stall_step_fn)(void *,unsigned,const struct pi4_usb_bus_stall *);
/* xHCI 4.6.8: Reset Endpoint(TSP=0), device ClearFeature(ENDPOINT_HALT),
 * Set TR Dequeue Pointer past the failed TD. The normal CSW submission then
 * rings its doorbell. No retry of the failed data request or CBW is issued. */
static int pi4_usb_bus_stall_run(const struct pi4_usb_bus_stall *p,void *opaque,
        pi4_usb_bus_stall_step_fn action,unsigned *stage) {
    if(!p || !action || !stage)return 0;
    for(unsigned step=0;step<3u;step++) {
        *stage=step+1u;
        if(!action(opaque,step,p))return 0;
    }
    *stage=4u;
    return 1;
}

struct pi4_usb_bus_inquiry {
    unsigned step, actual[3], csw_ok, identity_ok;
    uint8_t data[PI4_XHCI_DMA_BOT_INQUIRY_BYTES], csw[PI4_XHCI_DMA_BOT_CSW_BYTES];
};
typedef int (*pi4_usb_bus_bulk_phase_fn)(void *, unsigned, uint8_t *, unsigned,
                                        unsigned *);
/* Preserve the received bytes and the exact phase even when validation fails.
 * The callback owns DMA publication/waiting and exact event identity checks;
 * this transaction owns the read-only wire request and CSW/identity validation. */
static int pi4_usb_bus_inquiry_run(void *opaque, pi4_usb_bus_bulk_phase_fn phase,
        uint8_t *cbw,uint8_t *data,uint8_t *csw,struct pi4_usb_bus_inquiry *r) {
    if (!phase || !cbw || !data || !csw || !r) return 0;
    r->step=1u; r->csw_ok=r->identity_ok=0u;
    for(unsigned i=0;i<3u;i++)r->actual[i]=0;
    for(unsigned i=0;i<sizeof(r->data);i++)r->data[i]=0;
    for(unsigned i=0;i<sizeof(r->csw);i++)r->csw[i]=0;
    if(pi4_xhci_dma_bot_inquiry_cbw_seed(cbw,31u,PI4_XHCI_DMA_BOT_INQUIRY_TAG)) return 0;
    r->step=2u;
    if(!phase(opaque,0u,cbw,31u,&r->actual[0]) || r->actual[0]!=31u) return 0;
    r->step=3u;
    if(!phase(opaque,1u,data,36u,&r->actual[1]) || r->actual[1]>36u) return 0;
    for(unsigned i=0;i<r->actual[1];i++)r->data[i]=data[i];
    r->step=4u;
    if(!phase(opaque,2u,csw,13u,&r->actual[2]) || r->actual[2]>13u) return 0;
    for(unsigned i=0;i<r->actual[2];i++)r->csw[i]=csw[i];
    r->step=5u;
    r->csw_ok=pi4_xhci_dma_bot_csw_ok(r->csw,r->actual[2],
        PI4_XHCI_DMA_BOT_INQUIRY_TAG,36u,r->actual[1]);
    if(!r->csw_ok)return 0;
    r->step=6u;
    r->identity_ok=pi4_xhci_dma_bot_inquiry_ok(r->data,r->actual[1]);
    if(!r->identity_ok)return 0;
    r->step=7u;
    return 1;
}
/* The speed-independent descriptor parser uses UINT32_MAX for absent SS
 * companions. USB2 bulk contexts require Max Burst Size=0; the sentinel must
 * never be passed to their context constructor. SS must retain its stricter
 * companion/no-stream requirements rather than treating absence as zero. */
static int pi4_usb_bus_bulk_plan(const struct pi4_xhci_dma_config_summary *s,
        unsigned speed, struct pi4_usb_bus_bulk_plan *plan) {
    if (!plan) return 0;
    *plan=(struct pi4_usb_bus_bulk_plan){0};
    if (!s || !s->bot_interface_found || s->bot_interface_alt ||
        s->bot_interface_endpoint_count!=2u ||
        !pi4_xhci_dma_bulk_max_packet_valid(speed,s->bot_bulk_in_max_packet) ||
        !pi4_xhci_dma_bulk_max_packet_valid(speed,s->bot_bulk_out_max_packet)) return 0;
    if (speed==1u || speed==3u) {
        return s->bot_bulk_in_max_burst==UINT32_MAX &&
            s->bot_bulk_out_max_burst==UINT32_MAX &&
            s->bot_bulk_in_streams==UINT32_MAX && s->bot_bulk_out_streams==UINT32_MAX;
    }
    if (speed!=4u || s->bot_bulk_in_max_burst>15u || s->bot_bulk_out_max_burst>15u ||
        s->bot_bulk_in_streams || s->bot_bulk_out_streams) return 0;
    plan->in_burst=s->bot_bulk_in_max_burst;
    plan->out_burst=s->bot_bulk_out_max_burst;
    return 1;
}

typedef int (*pi4_usb_bus_command_fn)(uint32_t[4], unsigned, unsigned *);
/* Only for a bootstrap slot whose EP0 and Configure Endpoint completions were
 * received and whose new data endpoints have never been queued/doorbelled.
 * xHCI 4.6.4: wait for Disable Slot success before clearing DCBAA. Retain all
 * arena allocations, including the rejected slot's diagnostic snapshots. */
static int pi4_usb_bus_retire_idle_slot(uint64_t *dcbaa, unsigned slot,
        pi4_usb_bus_command_fn command, pi4_xhci_dma_publish_fn clean) {
    _Alignas(16) uint32_t words[4] = {0};
    unsigned returned = 0;
    if (!dcbaa || ((uintptr_t)dcbaa & 63u) || !command || !clean || !slot ||
        slot > PI4_XHCI_DMA_MAX_SLOTS || !dcbaa[slot]) return 0;
    words[3] = PI4_XHCI_TRB_TYPE_DISABLE_SLOT << 10 | slot << 24;
    if (!command(words, slot, &returned) || returned != slot) return 0;
    dcbaa[slot] = 0;
    clean((uint8_t *)(void *)&dcbaa[slot], sizeof(dcbaa[slot]));
    return 1;
}

static int pi4_usb_bus_region(uint64_t used, uint64_t arena_bytes,
        unsigned child, uint64_t *offset) {
    uint64_t base = pi4_xhci_dma_align_up(used, 8192u);
    if (!offset || child >= PI4_USB_BUS_CHILDREN || base == UINT64_MAX ||
        base > arena_bytes || PI4_USB_BUS_DEVICE_BYTES >
            (arena_bytes - base) / PI4_USB_BUS_CHILDREN) return 0;
    *offset = base + child * PI4_USB_BUS_DEVICE_BYTES;
    return 1;
}

struct pi4_usb_bus_pending {
    uint64_t trb;
    unsigned slot, dci, requested, minimum;
};
/* Zero: another endpoint's event. Positive: owner+1. Negative: bad transfer
 * for an owned endpoint. Caller must first validate event cycle ownership. */
static int pi4_usb_bus_completion(const struct pi4_usb_bus_pending *pending,
        unsigned count, uint64_t parameter, uint32_t status, uint32_t control,
        unsigned *actual) {
    if (!pending || !actual || count > PI4_USB_BUS_CHILDREN) return -1;
    if (((control >> 10) & 63u) != PI4_XHCI_TRB_TYPE_TRANSFER_EVT) return 0;
    unsigned slot = control >> 24, dci = (control >> 16) & 31u;
    for (unsigned i = 0; i < count; i++) {
        const struct pi4_usb_bus_pending *p = &pending[i];
        if (!p->trb || slot != p->slot || dci != p->dci) continue;
        if (!p->minimum || p->minimum > p->requested || p->requested > 64u ||
            !pi4_xhci_dma_bulk_event_ok(parameter, status, control, p->trb,
                p->slot, p->dci, p->requested, 1u, actual) || *actual < p->minimum)
            return -1;
        return (int)i + 1;
    }
    return 0;
}

/* Append a complete, bounded EP0 TD. This boot-time control ring never wraps.
 * Publish its first TRB last, so an eager xHC cannot see an incomplete TD.
 * Each input buffer is separate from the ring and cache-line isolated. */
static int pi4_usb_bus_control_td(uint8_t *ring, uint64_t ring_bus,
        unsigned *next, unsigned rt, unsigned request, unsigned value,
        unsigned interface, uint64_t buffer_bus, unsigned length,
        void (*clean)(uint8_t *, uint64_t), uint64_t *status_bus) {
    _Alignas(16) uint32_t words[3][4];
    unsigned count = length ? 3u : 2u;
    if (!ring || !next || !clean || !status_bus || ((uintptr_t)ring & 63u) ||
        (ring_bus & 63u) || *next > 254u - count || length > 4096u ||
        (length && (!(rt & 0x80u) || !buffer_bus || (buffer_bus & 63u))) ||
        pi4_xhci_dma_setup_stage_seed(words[0], rt, request, value, interface,
            length, length ? 3u : 0u, 1u) != 0 ||
        (length && pi4_xhci_dma_data_stage_seed(words[1], buffer_bus, length, 1u)) ||
        pi4_xhci_dma_status_stage_seed_direction(words[count-1], !length, 1u))
        return 0;
    unsigned first = *next;
    for (unsigned i = 0; i < count; i++) {
        volatile uint32_t *dst = (volatile uint32_t *)(void *)(ring + (first+i)*16u);
        for (unsigned j = 0; j < 3u; j++) dst[j] = words[i][j];
        dst[3] = words[i][3] & ~1u;
    }
    clean(ring + first*16u, count*16u);
    for (unsigned i = count; i-- > 0;) {
        volatile uint32_t *dst = (volatile uint32_t *)(void *)(ring + (first+i)*16u);
        dst[3] = words[i][3];
        clean(ring + (first+i)*16u, 16u);
    }
    *status_bus = ring_bus + (first+count-1u)*16u;
    *next += count;
    return 1;
}
