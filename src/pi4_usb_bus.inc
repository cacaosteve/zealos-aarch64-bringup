/* Physical USB2 multi-child session. Included after the accepted single-device
 * helpers; all handles and event cursors outlive bootstrap. Media stays read-only. */
struct pi4_usb_bus_device {
    uint8_t *output, *ep0, *in, *out, *buffer;
    uint64_t output_bus, ep0_bus, in_bus, out_bus, buffer_bus;
    unsigned port, slot, speed, packet0, vendor, product, role, stage, next;
    unsigned protocol, active, reports, keys, repeat_usage;
    unsigned context_errors, disabled, configure_status, configure_control, configure_step;
    uint32_t configured_slot[4], configured_in[5], configured_out[5];
    uint64_t repeat_at;
    struct pi4_usb_keyboard_interface hid;
    struct pi4_usb_keyboard_state keyboard;
    struct pi4_usb_mouse_state mouse;
    struct pi4_xhci_dma_config_summary config;
    struct pi4_usb_bus_bulk_plan bulk;
    struct pi4_xhci_dma_producer producer;
};
static struct {
    struct pi4_usb_bus_device hub, device[PI4_USB_BUS_CHILDREN];
    struct pi4_usb_bus_pending pending[PI4_USB_BUS_CHILDREN];
    uint8_t *input;
    uint64_t *dcbaa;
    uint64_t input_bus;
    unsigned attempted, active, busy, reading, stage, fault, count, connected;
    unsigned keyboard, mouse, storage, checks, interleaved, announced;
    unsigned storage_fault;
    unsigned storage_pending, storage_ready, probe_state;
    unsigned storage_automatic;
    uint64_t ready_at;
    unsigned context_size;
    struct {
        struct pi4_usb_bus_stall plan;
        unsigned attempts, completed, stage, endpoint_state;
        unsigned rc[3], status[3], control[3];
    } recovery;
    struct pi4_usb_bus_inquiry inquiry;
    struct pi4_xhci_bot_io_report inquiry_io;
    unsigned last_rc, last_status, last_control, last_port;
} g_pi4_bus;
static struct pi4_usb_bus_device *g_pi4_bus_decoding;

static void pi4_usb_bus_key(unsigned usage, int down) {
    struct pi4_usb_bus_device *d = g_pi4_bus_decoding;
    if (!d) return;
    unsigned code = usb_hid_key(usage);
    if (code) { input_usb_key(code, down); d->keys++; }
    if (usage < 224u) {
        if (down) {
            uint64_t now;
            __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
            d->repeat_usage = usage;
            d->repeat_at = now + g_pi4_usb_live.frequency * 400u / 1000u;
        } else if (usage == d->repeat_usage) d->repeat_usage = 0;
    }
}
static int pi4_usb_bus_keyboard_ready(void) {
    return g_pi4_bus.active && g_pi4_bus.keyboard < g_pi4_bus.count;
}
static int pi4_usb_bus_pointer_ready(void) {
    return g_pi4_bus.active && g_pi4_bus.mouse < g_pi4_bus.count;
}
static void pi4_usb_bus_release(void) {
    if (!g_pi4_bus.active) return;
    g_pi4_bus.active = 0;
    g_pi4_bus.storage_pending=g_pi4_bus.storage_ready=0u;
    for (unsigned i=0; i<g_pi4_bus.count; i++) {
        struct pi4_usb_bus_device *d=&g_pi4_bus.device[i];
        const uint8_t released[8]={0};
        g_pi4_bus_decoding=d;
        if (i==g_pi4_bus.keyboard)
            (void)pi4_usb_keyboard_decode(&d->keyboard,released,8u,pi4_usb_bus_key);
        d->mouse.buttons=0; d->repeat_usage=0; d->active=0;
    }
    g_pi4_bus_decoding=0;
}
static int pi4_usb_bus_queue(unsigned i) {
    struct pi4_usb_bus_device *d=&g_pi4_bus.device[i];
    _Alignas(16) uint32_t words[4];
    unsigned index, bytes=d->role==1u ? 8u : d->hid.packet;
    uint8_t *report=d->buffer+3072u;
    for (unsigned n=0;n<64u;n++) report[n]=0xa5u;
    pi4_dma_clean_invalidate_bytes(report,64u);
    if (pi4_xhci_dma_bulk_normal_seed(words,d->buffer_bus+3072u,bytes,1u,
            d->producer.cycle) ||
        pi4_xhci_dma_ring_enqueue(d->in,d->in_bus,&d->producer,words,
            pi4_dma_clean_bytes,&index)) return 0;
    g_pi4_bus.pending[i]=(struct pi4_usb_bus_pending){d->in_bus+index*16u,
        d->slot,(d->hid.endpoint&15u)*2u+1u,bytes,d->role==1u ? 8u : 3u};
    uint32_t flush;
    return !pi4_xhci_write_mmio32(g_pi4_usb_live.transport.db,d->slot*4u,
               PI4_XHCI_MMIO_BYTES,g_pi4_bus.pending[i].dci) &&
        !pi4_xhci_read_mmio32(g_pi4_usb_live.transport.db,d->slot*4u,
               PI4_XHCI_MMIO_BYTES,&flush);
}
/* Called only for an owned event. A non-HID event is left to its synchronous
 * command/control/BOT waiter. The caller advances ERDP exactly once. */
static int pi4_usb_bus_dispatch(uint64_t parameter,uint32_t status,uint32_t control) {
    unsigned actual=0;
    int owner=pi4_usb_bus_completion(g_pi4_bus.pending,g_pi4_bus.count,
                                    parameter,status,control,&actual);
    if (owner<=0) return owner;
    unsigned i=(unsigned)owner-1u;
    struct pi4_usb_bus_device *d=&g_pi4_bus.device[i];
    g_pi4_bus.pending[i].trb=0;
    pi4_dma_invalidate_bytes(d->buffer+3072u,64u);
    g_pi4_bus.last_status=status; g_pi4_bus.last_control=control;
    d->reports++;
    int ok;
    if (d->role==1u) {
        g_pi4_bus_decoding=d;
        ok=pi4_usb_keyboard_decode(&d->keyboard,d->buffer+3072u,actual,
                                   pi4_usb_bus_key)>=0;
        g_pi4_bus_decoding=0;
    } else ok=pi4_usb_mouse_decode(&d->mouse,d->buffer+3072u,actual,
                                  (uint32_t)g_fb_w,(uint32_t)g_fb_h);
    if (g_pi4_bus.reading) g_pi4_bus.interleaved++;
    return ok && pi4_usb_bus_queue(i) ? 1 : -1;
}
static int pi4_usb_bus_control(struct pi4_usb_bus_device *d,unsigned rt,
        unsigned request,unsigned value,unsigned index,unsigned length) {
    uint64_t status_bus;
    if (length) {
        for (unsigned i=0;i<length;i++) d->buffer[i]=0xa5u;
        pi4_dma_clean_invalidate_bytes(d->buffer,length);
    }
    if (!pi4_usb_bus_control_td(d->ep0,d->ep0_bus,&d->next,rt,request,value,
            index,d->buffer_bus,length,pi4_dma_clean_bytes,&status_bus)) return 0;
    struct pi4_xhci_bot_transport *t=&g_pi4_usb_live.transport;
    int rc=pi4_xhci_dma_control_transfer_submit_wait(t->op,t->rt,t->db,t->events,
        t->event_bus,status_bus,d->slot,1u,t->cursor);
    g_pi4_bus.last_rc=(unsigned)rc;
    g_pi4_bus.last_status=g_pi4_xhci_dma_transfer_status;
    g_pi4_bus.last_control=g_pi4_xhci_dma_transfer_control;
    if (rc!=1) return 0;
    if (length) pi4_dma_invalidate_bytes(d->buffer,length);
    return 1;
}
static int pi4_usb_bus_command(uint32_t words[4],unsigned expected_slot,
                               unsigned *returned_slot) {
    unsigned index;
    uint32_t status=UINT32_MAX,control=UINT32_MAX;
    words[3]=(words[3]&~1u)|g_pi4_usb_live.command.cycle;
    if (pi4_xhci_dma_ring_enqueue(g_pi4_usb_live.commands,g_pi4_usb_live.command_bus,
            &g_pi4_usb_live.command,words,pi4_dma_clean_bytes,&index)) return 0;
    struct pi4_xhci_bot_transport *t=&g_pi4_usb_live.transport;
    int rc=pi4_xhci_dma_command_submit_wait(t->op,t->rt,t->db,t->events,
        t->event_bus,g_pi4_usb_live.command_bus,index,expected_slot,t->cursor,
        returned_slot,&status,&control);
    g_pi4_bus.last_rc=(unsigned)rc; g_pi4_bus.last_status=status;
    g_pi4_bus.last_control=control;
    if (rc==1) g_pi4_xhci_dma_command_completions++;
    return rc==1;
}
static int pi4_usb_bus_stall_step(void *opaque,unsigned step,
                                  const struct pi4_usb_bus_stall *p) {
    struct pi4_usb_bus_device *d=opaque;
    unsigned returned=0;
    _Alignas(16) uint32_t words[4];
    int ok;
    if(step==1u)ok=pi4_usb_bus_control(d,0x02u,1u,0u,p->endpoint,0u);
    else ok=pi4_usb_bus_stall_command(p,step,words) &&
             pi4_usb_bus_command(words,p->slot,&returned);
    g_pi4_bus.recovery.rc[step]=g_pi4_bus.last_rc;
    g_pi4_bus.recovery.status[step]=g_pi4_bus.last_status;
    g_pi4_bus.recovery.control[step]=g_pi4_bus.last_control;
    if(!ok)return 0;
    pi4_dma_invalidate_bytes(d->output,4096u);
    const uint32_t *ep=(const uint32_t *)(const void *)(
        d->output+p->dci*g_pi4_bus.context_size);
    g_pi4_bus.recovery.endpoint_state=ep[0]&7u;
    if((ep[0]&7u)!=3u)return 0; /* Still stopped; only the CSW may restart it. */
    return step!=2u || ((uint64_t)ep[2]|(uint64_t)ep[3]<<32)==(p->dequeue|p->cycle);
}
static int pi4_usb_bus_recover_stall(const struct pi4_usb_bus_stall *p) {
    if(!g_pi4_bus.active || !g_pi4_bus.busy || !g_pi4_bus.reading ||
        g_pi4_bus.storage>=g_pi4_bus.count || g_pi4_bus.recovery.attempts>=4u)return 0;
    struct pi4_usb_bus_device *d=&g_pi4_bus.device[g_pi4_bus.storage];
    /* This gate covers the physical High-Speed BOT target, without a TT.
     * FS hub children additionally need CLEAR_TT_BUFFER and are not retried. */
    if(d->role!=3u || d->speed!=3u || p->slot!=d->slot || p->ring_bus!=d->in_bus ||
        p->endpoint!=d->config.bot_bulk_in_address ||
        (g_pi4_bus.context_size!=32u && g_pi4_bus.context_size!=64u))return 0;
    g_pi4_bus.recovery.attempts++; g_pi4_bus.recovery.stage=0;
    for(unsigned i=0;i<sizeof(*p);i++)
        ((uint8_t *)(void *)&g_pi4_bus.recovery.plan)[i]=((const uint8_t *)(const void *)p)[i];
    for(unsigned i=0;i<3u;i++)g_pi4_bus.recovery.rc[i]=g_pi4_bus.recovery.status[i]=
        g_pi4_bus.recovery.control[i]=UINT32_MAX;
    pi4_dma_invalidate_bytes(d->output,4096u);
    const uint32_t *ep=(const uint32_t *)(const void *)(
        d->output+p->dci*g_pi4_bus.context_size);
    g_pi4_bus.recovery.endpoint_state=ep[0]&7u;
    if((ep[0]&7u)!=2u)return 0; /* Reset Endpoint is valid only when Halted. */
    if(!pi4_usb_bus_stall_run(p,d,pi4_usb_bus_stall_step,&g_pi4_bus.recovery.stage))return 0;
    g_pi4_bus.recovery.completed++;
    return 1;
}
static int pi4_usb_bus_reset_child(unsigned port,unsigned *speed) {
    struct pi4_usb_bus_device *hub=&g_pi4_bus.hub;
    if (!pi4_usb_bus_control(hub,0x23u,3u,PI4_XHCI_HUB_FEATURE_PORT_RESET,port,0u) ||
        !pi4_xhci_dma_wait_ms(100u)) return 0;
    for (unsigned n=0;n<5u;n++) {
        if (!pi4_usb_bus_control(hub,0xa3u,0u,0u,port,4u)) return 0;
        uint32_t status=hub->buffer[0]|(uint32_t)hub->buffer[1]<<8;
        uint32_t change=hub->buffer[2]|(uint32_t)hub->buffer[3]<<8;
        if (pi4_xhci_dma_hub_port_reset_complete(status,change) &&
            (status&0x103u)==0x103u) {
            *speed=pi4_xhci_dma_hub_child_speed_id(status);
            return *speed && pi4_usb_bus_control(hub,0x23u,1u,
                PI4_XHCI_HUB_FEATURE_C_PORT_RESET,port,0u);
        }
        if (!pi4_xhci_dma_wait_ms(50u)) return 0;
    }
    return 0;
}
static int pi4_usb_bus_configure_child(struct pi4_usb_bus_device *d,unsigned size,
                                      unsigned hub_slot,unsigned root_port,
                                      unsigned hub_speed) {
    _Alignas(16) uint32_t words[4];
    unsigned returned;
    uint8_t *input=g_pi4_bus.input;
    d->stage=g_pi4_bus.stage=2u;
    if (!pi4_usb_bus_reset_child(d->port,&d->speed)) return 0;
    d->packet0=pi4_xhci_dma_ep0_default_mps(d->speed);
    if (pi4_xhci_dma_transfer_ring_seed(d->ep0,d->ep0_bus) ||
        pi4_xhci_dma_transfer_ring_seed(d->in,d->in_bus) ||
        pi4_xhci_dma_transfer_ring_seed(d->out,d->out_bus)) return 0;
    pi4_dma_clean_bytes(d->ep0,8192u); pi4_dma_clean_bytes(d->in,8192u);
    pi4_dma_clean_bytes(d->out,8192u);
    d->stage=g_pi4_bus.stage=3u;
    if (pi4_xhci_dma_enable_slot_seed(words,g_pi4_xhci_dma_protocol_slot_type,1u) ||
        !pi4_usb_bus_command(words,UINT32_MAX,&d->slot) || !d->slot ||
        d->slot>PI4_XHCI_DMA_MAX_SLOTS || d->slot==hub_slot) return 0;
    /* Every slot has an independent, retained output context. */
    uint64_t *dcbaa=g_pi4_bus.dcbaa;
    dcbaa[d->slot]=d->output_bus;
    pi4_dma_clean_bytes((uint8_t *)(void *)&dcbaa[d->slot],8u);
    d->stage=g_pi4_bus.stage=4u;
    pi4_dma_zero_bytes(input,4096u);
    if (pi4_xhci_dma_child_address_context_seed(input,4096u,size,d->speed,
            root_port,d->port,hub_slot,d->port,hub_speed,0u,d->ep0_bus) ||
        pi4_xhci_dma_address_device_seed(words,g_pi4_bus.input_bus,d->slot,1u,0u)) return 0;
    pi4_dma_clean_bytes(input,4096u); pi4_dma_clean_invalidate_bytes(d->output,4096u);
    if (!pi4_usb_bus_command(words,d->slot,&returned)) return 0;
    pi4_dma_invalidate_bytes(d->output,4096u);
    const uint32_t *slot=(const uint32_t *)(const void *)d->output;
    if ((slot[3]>>27)!=2u || !pi4_xhci_dma_output_usb_address(d->output))
        return 0;
    d->stage=g_pi4_bus.stage=5u;
    if (!pi4_usb_bus_control(d,0x80u,6u,0x0100u,0u,8u) ||
        !pi4_xhci_dma_child_device_descriptor_prefix_ok(d->buffer,8u,d->speed)) return 0;
    unsigned requested;
    int evaluate=pi4_xhci_dma_ep0_mps_update_plan(d->speed,d->buffer[7],d->packet0,&requested);
    if (evaluate<0) return 0;
    if (evaluate) {
        d->stage=g_pi4_bus.stage=6u;
        if (pi4_xhci_dma_child_ep0_evaluate_context_seed(input,4096u,size,
                d->output,4096u,d->speed,requested) ||
            pi4_xhci_dma_evaluate_context_seed(words,g_pi4_bus.input_bus,d->slot,1u)) return 0;
        pi4_dma_clean_bytes(input,4096u); pi4_dma_clean_invalidate_bytes(d->output,4096u);
        if (!pi4_usb_bus_command(words,d->slot,&returned)) return 0;
        pi4_dma_invalidate_bytes(d->output,4096u);
        const void *ep0=pi4_xhci_dma_output_ep0_context(d->output,4096u,size);
        if (pi4_xhci_dma_output_ep0_max_packet_size(ep0)!=requested) return 0;
        d->packet0=requested;
    }
    d->stage=g_pi4_bus.stage=7u;
    if (!pi4_usb_bus_control(d,0x80u,6u,0x0100u,0u,18u) ||
        !pi4_xhci_dma_child_device_descriptor_ok(d->buffer,18u,d->speed)) return 0;
    d->vendor=d->buffer[8]|(unsigned)d->buffer[9]<<8;
    d->product=d->buffer[10]|(unsigned)d->buffer[11]<<8;
    d->stage=g_pi4_bus.stage=8u;
    if (!pi4_usb_bus_control(d,0x80u,6u,0x0200u,0u,9u) ||
        !pi4_xhci_dma_configuration_header_ok(d->buffer,9u)) return 0;
    unsigned length=d->buffer[2]|(unsigned)d->buffer[3]<<8;
    d->stage=g_pi4_bus.stage=9u;
    if (length>2048u || !pi4_usb_bus_control(d,0x80u,6u,0x0200u,0u,length) ||
        !pi4_xhci_dma_configuration_descriptor_ok(d->buffer,length,2048u,&d->config)) return 0;
    if (pi4_usb_keyboard_select(d->buffer,length,&d->hid)) d->role=1u;
    else if (pi4_usb_mouse_select(d->buffer,length,&d->hid)) d->role=2u;
    else if (d->config.bot_interface_found) d->role=3u;
    else return 1; /* Reserved slot, no unsupported endpoint traffic. */
    d->stage=g_pi4_bus.stage=10u;
    if (!pi4_usb_bus_control(d,0u,9u,d->config.configuration_value,0u,0u) ||
        !pi4_usb_bus_control(d,0x80u,8u,0u,0u,1u) ||
        d->buffer[0]!=d->config.configuration_value) return 0;
    d->stage=g_pi4_bus.stage=11u;
    if (d->role<3u) {
        if (!pi4_usb_bus_control(d,0x21u,0x0bu,0u,d->hid.interface,0u) ||
            !pi4_usb_bus_control(d,0xa1u,3u,0u,d->hid.interface,1u)) return 0;
        d->protocol=d->buffer[0];
        if (d->protocol || (d->role==1u &&
            !pi4_usb_bus_control(d,0x21u,0x0au,0u,d->hid.interface,0u))) return 0;
    } else if (!pi4_usb_bus_control(d,0xa1u,0xfeu,0u,d->config.bot_interface_number,1u) ||
               d->buffer[0]>15u) return 0;
    d->stage=g_pi4_bus.stage=12u;
    d->configure_step=1u;
    pi4_dma_invalidate_bytes(d->output,4096u);
    int rc;
    if (d->role==1u) rc=pi4_usb_keyboard_context_seed(input,4096u,size,d->output,
        4096u,d->speed,&d->hid,d->in_bus);
    else if (d->role==2u) rc=pi4_usb_mouse_context_seed(input,4096u,size,d->output,
        4096u,d->speed,&d->hid,d->in_bus);
    else {
        const struct pi4_xhci_dma_config_summary *s=&d->config;
        if (!pi4_usb_bus_bulk_plan(s,d->speed,&d->bulk)) return 0;
        rc=pi4_xhci_dma_bulk_contexts_seed(input,4096u,size,d->output,4096u,8u,6u,
            0x50u,s->bot_interface_alt,s->bot_interface_endpoint_count,
            s->bot_bulk_in_address,s->bot_bulk_in_attributes,s->bot_bulk_in_max_packet,
            d->bulk.in_burst,s->bot_bulk_out_address,s->bot_bulk_out_attributes,
            s->bot_bulk_out_max_packet,d->bulk.out_burst,d->speed,d->in_bus,d->out_bus);
    }
    if (rc || pi4_xhci_dma_configure_endpoint_seed(words,g_pi4_bus.input_bus,d->slot,1u))
        return 0;
    d->configure_step=2u;
    pi4_dma_clean_bytes(input,4096u); pi4_dma_clean_invalidate_bytes(d->output,4096u);
    if (!pi4_usb_bus_command(words,d->slot,&returned)) return 0;
    d->configure_status=g_pi4_bus.last_status;
    d->configure_control=g_pi4_bus.last_control;
    d->configure_step=3u;
    pi4_dma_invalidate_bytes(d->output,4096u);
    if (d->role<3u) {
        if (!pi4_usb_keyboard_context_ok(d->output,4096u,size,d->speed,&d->hid,d->in_bus))
            return 0;
        d->producer=(struct pi4_xhci_dma_producer){0u,1u,0u};
        d->mouse.x=(uint32_t)g_fb_w/2u; d->mouse.y=(uint32_t)g_fb_h/2u;
    } else {
        const struct pi4_xhci_dma_config_summary *s=&d->config;
        const uint32_t *in=(const uint32_t *)(const void *)(d->output+
            ((s->bot_bulk_in_address&15u)*2u+1u)*size);
        const uint32_t *out=(const uint32_t *)(const void *)(d->output+
            (s->bot_bulk_out_address&15u)*2u*size);
        for (unsigned i=0;i<4u;i++)
            d->configured_slot[i]=((const uint32_t *)(const void *)d->output)[i];
        for (unsigned i=0;i<5u;i++) {
            d->configured_in[i]=in[i]; d->configured_out[i]=out[i];
        }
        d->context_errors=pi4_xhci_dma_bulk_context_errors(d->output,4096u,size,
            s->bot_bulk_in_address,s->bot_bulk_in_max_packet,d->bulk.in_burst,d->in_bus,
            s->bot_bulk_out_address,s->bot_bulk_out_max_packet,d->bulk.out_burst,d->out_bus);
        if (d->context_errors) return 0;
    }
    d->configure_step=4u;
    d->stage=13u;
    return 1;
}
static int pi4_usb_bus_inquiry_phase(void *opaque,unsigned phase,uint8_t *buffer,
                                    unsigned bytes,unsigned *actual) {
    struct pi4_usb_bus_device *d=opaque;
    struct pi4_xhci_bot_transport *t=&g_pi4_usb_live.transport;
    unsigned dci=phase ? (d->config.bot_bulk_in_address&15u)*2u+1u :
                         (d->config.bot_bulk_out_address&15u)*2u;
    uint64_t bus=phase ? d->in_bus : d->out_bus;
    uint8_t *ring=phase ? d->in : d->out;
    unsigned offset=phase==0u ? PI4_XHCI_DMA_BOT_CBW_OFFSET :
                    phase==1u ? PI4_XHCI_DMA_BOT_DATA_OFFSET : PI4_XHCI_DMA_BOT_CSW_OFFSET;
    if(phase>2u || !actual || buffer!=d->buffer+offset)return 0;
    int ok=pi4_xhci_dma_bot_transfer(t->op,t->rt,t->db,t->events,t->event_bus,t->cursor,
        d->slot,dci,ring,bus,phase==2u?1u:0u,buffer,d->buffer_bus+offset,
        bytes,phase,phase!=0u,phase==1u,0,&g_pi4_bus.inquiry_io);
    *actual=g_pi4_bus.inquiry_io.actual[phase];
    return ok;
}
/* One probe after HID startup, shared by automatic startup and the shell.
 * Keep all phase evidence; never retry a failed probe on stale rings. */
static int pi4_usb_bus_storage(struct pi4_usb_bus_device *d) {
    struct pi4_xhci_bot_transport *t=&g_pi4_usb_live.transport;
    g_pi4_xhci_dma_state=PI4_XHCI_DMA_ROOT_INQUIRY_BAD;
    if(!pi4_usb_bus_inquiry_run(d,pi4_usb_bus_inquiry_phase,
        d->buffer+PI4_XHCI_DMA_BOT_CBW_OFFSET,d->buffer+PI4_XHCI_DMA_BOT_DATA_OFFSET,
        d->buffer+PI4_XHCI_DMA_BOT_CSW_OFFSET,&g_pi4_bus.inquiry))return 0;
    g_pi4_xhci_dma_state=PI4_XHCI_DMA_ROOT_INQUIRY_PASS;
    pi4_xhci_dma_storage_probe(t->op,t->rt,t->db,t->events,t->event_bus,t->cursor,
        d->slot,(d->config.bot_bulk_in_address&15u)*2u+1u,
        (d->config.bot_bulk_out_address&15u)*2u,
        d->in,d->in_bus,d->out,d->out_bus,d->buffer,d->buffer_bus);
    return g_pi4_xhci_dma_state==PI4_XHCI_DMA_ROOT_FILE_PASS;
}
static int pi4_usb_bus_start(const struct zeal_handoff *h, unsigned hub_slot,
        unsigned root_port,unsigned hub_speed,unsigned size,unsigned connected,
        uint8_t *memory,const struct pi4_xhci_dma_layout *layout,
        volatile uint8_t *op,volatile uint8_t *rt,volatile uint8_t *db,
        uint8_t *events,uint64_t event_bus,struct pi4_xhci_dma_event_cursor *cursor,
        uint8_t *commands,uint64_t command_bus,uint8_t *input,uint64_t input_bus,
        uint8_t *hub_ep0,uint64_t hub_ep0_bus,uint8_t *hub_buffer,uint64_t hub_buffer_bus,
        uint16_t original_pci) {
    pi4_dma_zero_bytes((uint8_t *)(void *)&g_pi4_bus,sizeof(g_pi4_bus));
    g_pi4_bus.attempted=1u; g_pi4_bus.busy=1u; g_pi4_bus.stage=1u;
    g_pi4_bus.keyboard=g_pi4_bus.mouse=g_pi4_bus.storage=UINT32_MAX;
    g_pi4_bus.connected=connected;
    g_pi4_usb_live.handoff=h; g_pi4_usb_live.original_pci=original_pci;
    g_pi4_usb_live.events=*cursor;
    g_pi4_usb_live.commands=commands; g_pi4_usb_live.command_bus=command_bus;
    g_pi4_usb_live.command=(struct pi4_xhci_dma_producer){252u,1u,0u};
    struct pi4_xhci_bot_transport *t=&g_pi4_usb_live.transport;
    t->op=op; t->rt=rt; t->db=db; t->events=events; t->event_bus=event_bus;
    t->cursor=&g_pi4_usb_live.events;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(g_pi4_usb_live.frequency));
    g_pi4_bus.input=input; g_pi4_bus.input_bus=input_bus;
    g_pi4_bus.context_size=size;
    g_pi4_bus.dcbaa=(uint64_t *)(void *)(memory+layout->dcbaa_offset);
    struct pi4_usb_bus_device *hub=&g_pi4_bus.hub;
    hub->slot=hub_slot; hub->ep0=hub_ep0; hub->ep0_bus=hub_ep0_bus;
    hub->buffer=hub_buffer; hub->buffer_bus=hub_buffer_bus;
    uint64_t last=g_pi4_xhci_dma_hub_expected_status_bus;
    if (!g_pi4_usb_live.frequency || last<hub_ep0_bus || last>=hub_ep0_bus+254u*16u ||
        (last&15u) || !g_fb_w || !g_fb_h ||
        pi4_xhci_dma_ring_skipped_prefix(commands,248u,1u,pi4_dma_clean_bytes)) goto bad;
    hub->next=(unsigned)((last-hub_ep0_bus)/16u)+1u;
    for (unsigned port=1u;port<=4u;port++) {
        if (!(connected&(1u<<(port-1u)))) continue;
        unsigned i=g_pi4_bus.count++;
        struct pi4_usb_bus_device *d=&g_pi4_bus.device[i];
        uint64_t offset;
        d->port=port; g_pi4_bus.last_port=port;
        if (!pi4_usb_bus_region(layout->used_bytes,layout->arena_size,i,&offset)) goto bad;
        uint8_t *base=memory+offset;
        pi4_dma_zero_bytes(base,PI4_USB_BUS_DEVICE_BYTES);
        pi4_dma_clean_bytes(base,PI4_USB_BUS_DEVICE_BYTES);
        uint64_t bus=layout->arena_bus+offset;
        d->output=base; d->output_bus=bus;
        d->ep0=base+8192u; d->ep0_bus=bus+8192u;
        d->in=base+16384u; d->in_bus=bus+16384u;
        d->out=base+24576u; d->out_bus=bus+24576u;
        d->buffer=base+32768u; d->buffer_bus=bus+32768u;
        if (!pi4_usb_bus_configure_child(d,size,hub_slot,root_port,hub_speed)) {
            /* The only recoverable bootstrap failure here is strict bulk
             * readback after a successful Configure command. All EP0 events
             * have been consumed, no bulk TD was published, and HID queues
             * have not started. Unknown/timeout/transport failures still halt. */
            if (d->role!=3u || d->stage!=12u || d->configure_step!=3u || !d->context_errors ||
                (d->context_errors&1u) || g_pi4_bus.storage_fault ||
                g_pi4_bus.storage!=UINT32_MAX ||
                !pi4_usb_bus_retire_idle_slot(g_pi4_bus.dcbaa,d->slot,
                    pi4_usb_bus_command,pi4_dma_clean_bytes)) goto bad;
            d->disabled=1u; d->stage=16u; g_pi4_bus.storage_fault=12u;
            continue;
        }
        if (d->role==1u && g_pi4_bus.keyboard==UINT32_MAX) g_pi4_bus.keyboard=i;
        if (d->role==2u && g_pi4_bus.mouse==UINT32_MAX) g_pi4_bus.mouse=i;
        if (d->role==3u) {
            if (g_pi4_bus.storage!=UINT32_MAX || g_pi4_bus.storage_fault)
                goto bad; /* Exactly one read-only target. */
            g_pi4_bus.storage=i;
        }
    }
    if (g_pi4_bus.keyboard==UINT32_MAX || g_pi4_bus.mouse==UINT32_MAX) goto bad;
    g_pi4_bus.storage_pending=g_pi4_bus.storage<g_pi4_bus.count;
    g_pi4_bus.stage=14u;
    for (unsigned i=0;i<g_pi4_bus.count;i++) {
        if (i!=g_pi4_bus.keyboard && i!=g_pi4_bus.mouse) continue;
        if (!pi4_usb_bus_queue(i)) goto bad;
        g_pi4_bus.device[i].active=1u;
    }
    g_pi4_bus.active=1u; g_pi4_bus.busy=0u; g_pi4_bus.stage=15u;
    g_pi4_usb_live.kind=3u; g_pi4_usb_live.active=1u;
    uint64_t now;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
    g_pi4_usb_live.next_poll=now+5u*g_pi4_usb_live.frequency;
    pi4_usb_event_sink=pi4_usb_bus_dispatch;
    pi4_usb_bulk_stall_sink=pi4_usb_bus_recover_stall;
    g_pi4_xhci_dma_state=g_pi4_bus.storage_fault ?
        PI4_XHCI_DMA_BUS_INPUT_ONLY : g_pi4_bus.storage_pending ?
        PI4_XHCI_DMA_BUS_STORAGE_PENDING : PI4_XHCI_DMA_BUS_READY;
    return 1;
bad:
    g_pi4_bus.fault=g_pi4_bus.stage;
    g_pi4_bus.busy=0u;
    *cursor=g_pi4_usb_live.events;
    g_pi4_xhci_dma_state=PI4_XHCI_DMA_BUS_BAD;
    return 0;
}
static void pi4_usb_bus_read(unsigned automatic);
static void pi4_usb_bus_poll(void) {
    if (!g_pi4_bus.active || g_pi4_bus.busy) return;
    g_pi4_bus.busy=1u;
    struct pi4_xhci_bot_transport *t=&g_pi4_usb_live.transport;
    unsigned fault=0;
    uint32_t controller;
    if (pi4_xhci_read_mmio32(t->op,4u,PI4_XHCI_MMIO_BYTES,&controller) ||
        (controller&(PI4_XHCI_USBSTS_HALTED|PI4_XHCI_USBSTS_HSE))) fault=20u;
    for (unsigned work=0;!fault && work<256u;work++) {
        uint8_t *entry=t->events+g_pi4_usb_live.events.index*16u;
        volatile uint32_t *event=(volatile uint32_t *)(void *)entry;
        pi4_dma_invalidate_bytes(entry,16u);
        uint32_t control=event[3];
        if (!pi4_xhci_dma_event_ready(control,&g_pi4_usb_live.events)) break;
        __asm__ volatile("dmb oshld" ::: "memory");
        uint64_t parameter=(uint64_t)event[0]|(uint64_t)event[1]<<32;
        uint32_t status=event[2];
        g_pi4_xhci_dma_events++;
        (void)pi4_xhci_dma_event_cursor_advance(&g_pi4_usb_live.events);
        if (!pi4_xhci_dma_write64(t->rt,0x38,PI4_XHCI_MMIO_BYTES,
            pi4_xhci_dma_event_dequeue(t->event_bus,&g_pi4_usb_live.events))) fault=21u;
        else if (((control>>10)&63u)!=PI4_XHCI_TRB_TYPE_PSC_EVT &&
                 pi4_usb_bus_dispatch(parameter,status,control)!=1) fault=22u;
    }
    uint64_t now;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
    if (!fault && g_pi4_bus.storage_pending) {
        /* The controller is retained and both HID queues/dispatcher are live.
         * Consume pending before probing; recursive polling sees busy and the
         * same INQUIRY can never be submitted twice on the bootstrap rings. */
        g_pi4_bus.busy=0u;
        pi4_usb_bus_read(1u);
        return;
    }
    if (!fault && g_pi4_bus.storage_ready && now>=g_pi4_usb_live.next_poll) {
        g_pi4_bus.reading=1u;
        pi4_xhci_dma_fat32_file_probe(t);
        g_pi4_bus.probe_state=g_pi4_xhci_dma_state;
        g_pi4_bus.reading=0u;
        if (g_pi4_xhci_dma_state!=PI4_XHCI_DMA_ROOT_FILE_PASS) fault=23u;
        else {
            g_pi4_bus.checks++;
            g_pi4_xhci_dma_state=PI4_XHCI_DMA_BUS_READY;
            g_pi4_usb_live.next_poll=now+5u*g_pi4_usb_live.frequency;
        }
    }
    if (fault) {
        g_pi4_bus.fault=fault;
        g_pi4_xhci_dma_state=PI4_XHCI_DMA_BUS_BAD;
        pi4_usb_live_stop();
        pi4_usb_bus_report(0); pi4_usb_bus_report(1);
        pi_diag_fb_puts("\nUSB multi-device session stopped after a fault.\n");
    } else {
        struct pi4_usb_bus_device *d=&g_pi4_bus.device[g_pi4_bus.keyboard];
        if (d->repeat_usage && now>=d->repeat_at) {
            unsigned code=usb_hid_key(d->repeat_usage);
            if (code) { input_usb_key(code,1); d->keys++; }
            d->repeat_at=now+g_pi4_usb_live.frequency*35u/1000u;
        }
        if (g_pi4_bus.checks>=11u && !g_pi4_bus.announced) {
            g_pi4_bus.announced=1u;
            pi4_usb_bus_report(0); pi4_usb_bus_report(1);
            pi_diag_fb_puts("\nUSB bus: ten live file checks passed with input online.\n");
        }
    }
    g_pi4_bus.busy=0u;
}
static int pi4_usb_bus_sample(uint32_t w,uint32_t h,uint32_t *x,uint32_t *y,
                               int *buttons,int *wheel) {
    pi4_usb_bus_poll();
    if (!pi4_usb_bus_pointer_ready() || !w || !h || !x || !y) return 0;
    const struct pi4_usb_mouse_state *m=&g_pi4_bus.device[g_pi4_bus.mouse].mouse;
    *x=pi4_usb_mouse_clamp(m->x,w); *y=pi4_usb_mouse_clamp(m->y,h);
    if (buttons) *buttons=(int)m->buttons;
    if (wheel) *wheel=0;
    return 1;
}
static void pi4_usb_bus_report(int framebuffer) {
    if (!g_pi4_bus.attempted) return;
    unsigned values[]={g_pi4_bus.active,g_pi4_bus.stage,g_pi4_bus.fault,
        g_pi4_bus.connected,g_pi4_bus.count,g_pi4_bus.keyboard<g_pi4_bus.count,
        g_pi4_bus.mouse<g_pi4_bus.count,g_pi4_bus.storage<g_pi4_bus.count,
        g_pi4_bus.checks,g_pi4_bus.interleaved,g_pi4_fat32_file.crc[0],
        g_pi4_fat32_file.crc[1]};
    pi4_xhci_route_text(framebuffer,
        "\nUSB bus MULTI-USB2-6 active/stage/fault/mask/devices/kbd/mouse/disk/checks/interleaved/CRC1/CRC2=");
    for(unsigned i=0;i<12u;i++) {
        if(i)pi4_xhci_route_text(framebuffer,"/");
        pi4_xhci_route_hex(framebuffer,values[i]);
    }
    pi4_xhci_route_text(framebuffer,"\nUSB storage pending/ready/probe-state/INQUIRY-step=");
    unsigned gate[]={g_pi4_bus.storage_pending,g_pi4_bus.storage_ready,
        g_pi4_bus.probe_state,g_pi4_bus.inquiry.step};
    for(unsigned i=0;i<4u;i++) {
        if(i)pi4_xhci_route_text(framebuffer,"/");
        pi4_xhci_route_hex(framebuffer,gate[i]);
    }
    if(g_pi4_bus.probe_state) {
        pi4_xhci_route_text(framebuffer," ");
        pi4_xhci_route_text(framebuffer,pi4_xhci_dma_state_name(g_pi4_bus.probe_state));
    }
    if(g_pi4_bus.storage_ready) {
        uint64_t now;
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        uint64_t seconds=g_pi4_usb_live.frequency ?
            (now-g_pi4_bus.ready_at)/g_pi4_usb_live.frequency : 0u;
        pi4_xhci_route_text(framebuffer,"\nUSB storage automatic/ready-seconds/checks/target=");
        pi4_xhci_route_hex(framebuffer,g_pi4_bus.storage_automatic);
        pi4_xhci_route_text(framebuffer,"/");
        pi4_xhci_route_hex(framebuffer,seconds>UINT32_MAX ? UINT32_MAX : (uint32_t)seconds);
        pi4_xhci_route_text(framebuffer,"/");
        pi4_xhci_route_hex(framebuffer,g_pi4_bus.checks);
        pi4_xhci_route_text(framebuffer,"/");
        pi4_xhci_route_hex(framebuffer,11u);
    }
    if(g_pi4_bus.recovery.attempts) {
        const struct pi4_usb_bus_stall *p=&g_pi4_bus.recovery.plan;
        pi4_xhci_route_text(framebuffer,"\nUSB BOT stall attempts/completed/stage/EP-state/actual/requested=");
        unsigned fields[]={g_pi4_bus.recovery.attempts,g_pi4_bus.recovery.completed,
            g_pi4_bus.recovery.stage,g_pi4_bus.recovery.endpoint_state,p->actual,p->requested};
        for(unsigned i=0;i<6u;i++) {
            if(i)pi4_xhci_route_text(framebuffer,"/");
            pi4_xhci_route_hex(framebuffer,fields[i]);
        }
        pi4_xhci_route_text(framebuffer,"\nUSB BOT stall status/control/ptr-hi/lo/next-hi/lo/DCS=");
        unsigned events[]={p->status,p->control,(uint32_t)(p->failed>>32),(uint32_t)p->failed,
            (uint32_t)(p->dequeue>>32),(uint32_t)p->dequeue,p->cycle};
        for(unsigned i=0;i<7u;i++) {
            if(i)pi4_xhci_route_text(framebuffer,"/");
            pi4_xhci_route_hex(framebuffer,events[i]);
        }
        const char *labels[]={"\nUSB BOT reset rc/status/control=",
            "\nUSB BOT clear-halt rc/status/control=","\nUSB BOT set-dequeue rc/status/control="};
        for(unsigned step=0;step<3u && step<g_pi4_bus.recovery.stage;step++) {
            pi4_xhci_route_text(framebuffer,labels[step]);
            pi4_xhci_route_hex(framebuffer,g_pi4_bus.recovery.rc[step]);
            pi4_xhci_route_text(framebuffer,"/");
            pi4_xhci_route_hex(framebuffer,g_pi4_bus.recovery.status[step]);
            pi4_xhci_route_text(framebuffer,"/");
            pi4_xhci_route_hex(framebuffer,g_pi4_bus.recovery.control[step]);
        }
    }
    if(g_pi4_bus.inquiry.step) {
        const struct pi4_usb_bus_inquiry *r=&g_pi4_bus.inquiry;
        pi4_xhci_route_text(framebuffer,"\nUSB INQUIRY CSW-ok/identity-ok/type/format/add-length=");
        unsigned fields[]={r->csw_ok,r->identity_ok,r->data[0],r->data[3],r->data[4]};
        for(unsigned i=0;i<5u;i++) {
            if(i)pi4_xhci_route_text(framebuffer,"/");
            pi4_xhci_route_hex(framebuffer,fields[i]);
        }
        for(unsigned phase=0;phase<3u && r->step>=phase+2u;phase++) {
            const struct pi4_xhci_bot_io_report *io=&g_pi4_bus.inquiry_io;
            pi4_xhci_route_text(framebuffer,"\nUSB INQUIRY phase/rc/status/checks/actual/control/ptr-hi/lo=");
            unsigned values[]={phase+1u,io->rc[phase],io->status[phase],io->checks[phase],
                io->actual[phase],io->control[phase],(uint32_t)(io->parameter[phase]>>32),
                (uint32_t)io->parameter[phase]};
            for(unsigned i=0;i<8u;i++) {
                if(i)pi4_xhci_route_text(framebuffer,"/");
                pi4_xhci_route_hex(framebuffer,values[i]);
            }
        }
        pi4_xhci_route_text(framebuffer,"\nUSB INQUIRY CSW sig/tag/residue/status=");
        for(unsigned i=0;i<4u;i++) {
            uint32_t word=0;
            for(unsigned j=0;j<(i==3u?1u:4u);j++)word|=(uint32_t)r->csw[i*4u+j]<<(j*8u);
            if(i)pi4_xhci_route_text(framebuffer,"/");
            pi4_xhci_route_hex(framebuffer,word);
        }
    }
    if(g_pi4_xhci_storage.step) {
        for(unsigned i=0;i<g_pi4_xhci_storage.count;i++) {
            const struct pi4_xhci_storage_command_report *r=&g_pi4_xhci_storage.command[i];
            if(r->opcode!=3u || r->sense_key==UINT32_MAX)continue;
            pi4_xhci_route_text(framebuffer,"\nUSB recovered sense key/ASC/ASCQ=");
            pi4_xhci_route_hex(framebuffer,r->sense_key);pi4_xhci_route_text(framebuffer,"/");
            pi4_xhci_route_hex(framebuffer,r->sense_asc);pi4_xhci_route_text(framebuffer,"/");
            pi4_xhci_route_hex(framebuffer,r->sense_ascq);
        }
        pi4_xhci_route_text(framebuffer,"\nUSB read step/commands/reads/block/last-LBA=");
        unsigned fields[]={g_pi4_xhci_storage.step,g_pi4_xhci_storage.count,
            g_pi4_xhci_storage.reads,g_pi4_xhci_storage.block_bytes,g_pi4_xhci_storage.last_lba};
        for(unsigned i=0;i<5u;i++) {
            if(i)pi4_xhci_route_text(framebuffer,"/");
            pi4_xhci_route_hex(framebuffer,fields[i]);
        }
        const struct pi4_xhci_storage_command_report *r=g_pi4_usb_live.transport.in_producer ?
            &g_pi4_usb_live.transport.last : g_pi4_xhci_storage.count ?
            &g_pi4_xhci_storage.command[g_pi4_xhci_storage.count-1u] : 0;
        if(r) {
            pi4_xhci_route_text(framebuffer,"\nUSB read last op/tag/phase/valid/SCSI/bytes/actual=");
            unsigned fields[]={r->opcode,r->tag,r->io.phase,r->transport_ok,r->scsi_status,
                r->bytes,r->io.actual[1]};
            for(unsigned i=0;i<7u;i++) {
                if(i)pi4_xhci_route_text(framebuffer,"/");
                pi4_xhci_route_hex(framebuffer,fields[i]);
            }
            if(r->io.phase && r->io.phase<=3u) {
                unsigned phase=r->io.phase-1u;
                pi4_xhci_route_text(framebuffer,"\nUSB read last rc/status/checks/control/ptr-hi/lo=");
                unsigned fields[]={r->io.rc[phase],r->io.status[phase],r->io.checks[phase],
                    r->io.control[phase],(uint32_t)(r->io.parameter[phase]>>32),
                    (uint32_t)r->io.parameter[phase]};
                for(unsigned i=0;i<6u;i++) {
                    if(i)pi4_xhci_route_text(framebuffer,"/");
                    pi4_xhci_route_hex(framebuffer,fields[i]);
                }
            }
            pi4_xhci_route_text(framebuffer,"\nUSB read CSW sig/tag/residue/status=");
            for(unsigned i=0;i<4u;i++) {
                uint32_t word=0;
                for(unsigned j=0;j<(i==3u?1u:4u);j++)word|=(uint32_t)r->csw[i*4u+j]<<(j*8u);
                if(i)pi4_xhci_route_text(framebuffer,"/");
                pi4_xhci_route_hex(framebuffer,word);
            }
        }
    }
    for(unsigned n=0;n<g_pi4_bus.count;n++) {
        const struct pi4_usb_bus_device *d=&g_pi4_bus.device[n];
        unsigned fields[]={d->port,d->slot,d->role,d->speed,d->stage,d->vendor,d->product,
            d->reports,d->keys,d->producer.wraps,d->mouse.moves,d->mouse.seen_buttons};
        pi4_xhci_route_text(framebuffer,"\nUSB child port/slot/role/speed/stage/VID/PID/reports/keys/wraps/moves/buttons=");
        for(unsigned i=0;i<12u;i++) {
            if(i)pi4_xhci_route_text(framebuffer,"/");
            pi4_xhci_route_hex(framebuffer,fields[i]);
        }
        if (d->role==3u) {
            pi4_xhci_route_text(framebuffer,"\nUSB bulk setup-step/errors/disabled/config-status/control=");
            unsigned summary[]={d->configure_step,d->context_errors,d->disabled,
                d->configure_status,d->configure_control};
            for(unsigned i=0;i<5u;i++) {
                if(i)pi4_xhci_route_text(framebuffer,"/");
                pi4_xhci_route_hex(framebuffer,summary[i]);
            }
            const struct pi4_xhci_dma_config_summary *s=&d->config;
            unsigned expected[]={s->bot_bulk_in_address,s->bot_bulk_in_max_packet,
                d->bulk.in_burst,(uint32_t)d->in_bus,(uint32_t)(d->in_bus>>32),
                s->bot_bulk_out_address,s->bot_bulk_out_max_packet,d->bulk.out_burst,
                (uint32_t)d->out_bus,(uint32_t)(d->out_bus>>32)};
            pi4_xhci_route_text(framebuffer,"\nUSB bulk expected IN ep/MPS/burst/ringlo/hi=");
            for(unsigned i=0;i<10u;i++) {
                if(i==5u)pi4_xhci_route_text(framebuffer,"\nUSB bulk expected OUT ep/MPS/burst/ringlo/hi=");
                else if(i)pi4_xhci_route_text(framebuffer,"/");
                pi4_xhci_route_hex(framebuffer,expected[i]);
            }
            if(d->configure_step<3u) {
                pi4_xhci_route_text(framebuffer,
                    "\nUSB bulk output not captured: endpoint setup did not complete.");
                continue;
            }
            const uint32_t *groups[]={d->configured_slot,d->configured_in,d->configured_out};
            const char *labels[]={"\nUSB bulk output Slot[0..3]=",
                "\nUSB bulk output IN[0..4]=","\nUSB bulk output OUT[0..4]="};
            for(unsigned group=0;group<3u;group++) {
                pi4_xhci_route_text(framebuffer,labels[group]);
                for(unsigned i=0;i<(group?5u:4u);i++) {
                    if(i)pi4_xhci_route_text(framebuffer,"/");
                    pi4_xhci_route_hex(framebuffer,groups[group][i]);
                }
            }
        }
    }
    if(g_pi4_bus.storage_fault)
        pi4_xhci_route_text(framebuffer,
            "\nUSB storage disabled after bulk readback failure; keyboard/pointer only. No storage reads.");
    pi4_xhci_route_text(framebuffer,"\nUSB bus last-port/rc/status/control=");
    unsigned last[]={g_pi4_bus.last_port,g_pi4_bus.last_rc,g_pi4_bus.last_status,g_pi4_bus.last_control};
    for(unsigned i=0;i<4u;i++) {
        if(i)pi4_xhci_route_text(framebuffer,"/");
        pi4_xhci_route_hex(framebuffer,last[i]);
    }
}

static void pi4_usb_bus_read(unsigned automatic) {
    if(g_pi4_bus.active && !g_pi4_bus.busy && g_pi4_bus.storage_ready) {
        pi4_usb_bus_report(0); pi4_usb_bus_report(1);
        const char *message="\nusbread: storage already verified; periodic file checks are active.\n";
        pi4_xhci_route_text(0,message); pi4_xhci_route_text(1,message);
        return; /* No second bootstrap INQUIRY on the adopted live rings. */
    }
    if(!g_pi4_bus.active || g_pi4_bus.busy || !g_pi4_bus.storage_pending ||
        g_pi4_bus.storage>=g_pi4_bus.count) {
        pi4_xhci_route_text(0,"\nusbread: no untested storage target on an active USB2 bus.\n");
        pi4_xhci_route_text(1,"\nusbread: no untested storage target on an active USB2 bus.\n");
        return;
    }
    g_pi4_bus.storage_pending=0u; g_pi4_bus.busy=1u; g_pi4_bus.reading=1u;
    g_pi4_bus.storage_automatic=automatic!=0u;
    g_pi4_bus.stage=13u;
    for(unsigned i=0;i<3u;i++) {
        g_pi4_bus.inquiry_io.rc[i]=g_pi4_bus.inquiry_io.status[i]=
            g_pi4_bus.inquiry_io.control[i]=UINT32_MAX;
        g_pi4_bus.inquiry_io.parameter[i]=UINT64_MAX;
    }
    const char *intro=automatic ?
        "\nUSB startup: automatic read-only INQUIRY/capacity/FAT32 test; HID dispatch remains enabled.\n" :
        "\nusbread: read-only INQUIRY/capacity/FAT32 test; HID dispatch remains enabled.\n";
    pi4_xhci_route_text(0,intro); pi4_xhci_route_text(1,intro);
    int ok=pi4_usb_bus_storage(&g_pi4_bus.device[g_pi4_bus.storage]);
    g_pi4_bus.probe_state=g_pi4_xhci_dma_state;
    g_pi4_bus.reading=0u;
    if(ok) {
        g_pi4_bus.storage_ready=1u; g_pi4_bus.checks=1u; g_pi4_bus.stage=15u;
        uint64_t now;
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        g_pi4_bus.ready_at=now;
        g_pi4_usb_live.next_poll=now+5u*g_pi4_usb_live.frequency;
        g_pi4_xhci_dma_state=PI4_XHCI_DMA_BUS_READY;
    } else {
        g_pi4_bus.fault=13u; g_pi4_xhci_dma_state=PI4_XHCI_DMA_BUS_BAD;
        pi4_usb_live_stop();
    }
    g_pi4_bus.busy=0u;
    pi4_usb_bus_report(0); pi4_usb_bus_report(1);
    const char *message=ok ? "\nusbread: passed; periodic file checks enabled.\n" :
        "\nusbread: failed; controller stopped safely. Photograph USB INQUIRY/read lines; cold boot to recover input.\n";
    pi4_xhci_route_text(0,message); pi4_xhci_route_text(1,message);
}
