/* VL805 DMA/bootstrap planning and bounded live-ring publication helpers. */
#pragma once

#include <stdint.h>
#include "pi4_dma_probe.h"
#include "pi4_xhci_probe.h"

#define PI4_XHCI_DMA_PLAN_OK       0
#define PI4_XHCI_DMA_PLAN_GATE    -1
#define PI4_XHCI_DMA_PLAN_CAPS    -2
#define PI4_XHCI_DMA_PLAN_PAGES   -3
#define PI4_XHCI_DMA_PLAN_ARENA   -4
#define PI4_XHCI_DMA_PLAN_ADDRESS -5

#define PI4_XHCI_DMA_ALIGN        64ull
#define PI4_XHCI_DMA_PAGE         4096ull
#define PI4_XHCI_DMA_EVENT_TRBS   256u
#define PI4_XHCI_DMA_EVENT_BYTES  (PI4_XHCI_DMA_EVENT_TRBS * 16u)
#define PI4_XHCI_DMA_RING_BYTES   4096u
/* The VL805 may prefetch beyond a segment; keep one extra segment of owned
 * zeroed DMA memory after each logical 4 KiB ring, as the emu68 VL805 driver
 * does for XHCI_QUIRK_TRB_OVERFETCH. */
#define PI4_XHCI_DMA_RING_ALLOC_BYTES (2u * PI4_XHCI_DMA_RING_BYTES)
#define PI4_XHCI_DMA_CONTEXT_BYTES 4096u
#define PI4_XHCI_DMA_CONTROL_BUFFER_BYTES 4096u
#define PI4_XHCI_DMA_BULK_AVG_TRB_LENGTH 3072u
#define PI4_XHCI_DMA_MAX_SLOTS 6u
#define PI4_XHCI_TRB_TYPE_NORMAL   1u
#define PI4_XHCI_TRB_TYPE_NOOP    23u
#define PI4_XHCI_TRB_TYPE_LINK     6u
#define PI4_XHCI_TRB_TYPE_SETUP_STAGE 2u
#define PI4_XHCI_TRB_TYPE_DATA_STAGE 3u
#define PI4_XHCI_TRB_TYPE_STATUS_STAGE 4u
#define PI4_XHCI_TRB_TYPE_ENABLE_SLOT 9u
#define PI4_XHCI_TRB_TYPE_DISABLE_SLOT 10u
#define PI4_XHCI_TRB_TYPE_ADDRESS_DEVICE 11u
#define PI4_XHCI_TRB_TYPE_CONFIGURE_ENDPOINT 12u
#define PI4_XHCI_TRB_TYPE_EVALUATE_CONTEXT 13u
#define PI4_XHCI_TRB_TYPE_STOP_ENDPOINT 15u
#define PI4_XHCI_TRB_TYPE_CMD_EVT 33u
#define PI4_XHCI_TRB_TYPE_TRANSFER_EVT 32u
#define PI4_XHCI_TRB_TYPE_PSC_EVT 34u
#define PI4_XHCI_TRB_TYPE_MFINDEX_EVT 39u
#define PI4_XHCI_HUB_FEATURE_PORT_POWER 8u
#define PI4_XHCI_HUB_FEATURE_PORT_RESET 4u
#define PI4_XHCI_HUB_FEATURE_C_PORT_RESET 20u
#define PI4_XHCI_HUB_PORT_STATUS_POWER  (1u << 8)
#define PI4_XHCI_HUB_PORT_STATUS_CONNECTION (1u << 0)
#define PI4_XHCI_HUB_PORT_STATUS_ENABLE (1u << 1)
#define PI4_XHCI_HUB_PORT_STATUS_RESET  (1u << 4)
#define PI4_XHCI_HUB_PORT_CHANGE_RESET  (1u << 4)
#define PI4_XHCI_DMA_TRANSFER_CHECK_TYPE      (1u << 0)
#define PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT  (1u << 1)
#define PI4_XHCI_DMA_TRANSFER_CHECK_SLOT      (1u << 2)
#define PI4_XHCI_DMA_TRANSFER_CHECK_POINTER   (1u << 3)
#define PI4_XHCI_DMA_TRANSFER_CHECK_SUCCESS   (1u << 4)
#define PI4_XHCI_DMA_TRANSFER_CHECK_ALL       0x1fu
#define PI4_XHCI_EXT_CAP_SUPPORTED_PROTOCOL 2u
#define PI4_XHCI_PROTOCOL_SLOT_TYPE_MASK 0x1fu
#define PI4_XHCI_DMA_COMMAND_WRAP_START_TRB 249u
#define PI4_XHCI_DMA_COMMAND_WRAP_COUNT     7u
#define PI4_XHCI_DMA_COMMAND_WRAP_PRELINK_COUNT \
    (PI4_XHCI_DMA_COMMAND_WRAP_COUNT - 1u)
#define PI4_XHCI_DMA_COMMAND_ENABLE_SLOT_TRB \
    (PI4_XHCI_DMA_COMMAND_WRAP_START_TRB - 1u)
#define PI4_XHCI_DMA_COMMAND_ADDRESS_DEVICE_TRB \
    PI4_XHCI_DMA_COMMAND_WRAP_START_TRB
#define PI4_XHCI_DMA_COMMAND_STOP_ENDPOINT_TRB \
    (PI4_XHCI_DMA_COMMAND_ADDRESS_DEVICE_TRB + 1u)
#define PI4_XHCI_DMA_COMMAND_HUB_GUARD_NOOP_TRB \
    PI4_XHCI_DMA_COMMAND_STOP_ENDPOINT_TRB
/* The normal path first proves the post-Address-Device command-ring position
 * with a No-op, then configures the hub at the following ring entry. */
#define PI4_XHCI_DMA_COMMAND_CONFIGURE_HUB_TRB \
    (PI4_XHCI_DMA_COMMAND_HUB_GUARD_NOOP_TRB + 1u)
#define PI4_XHCI_DMA_COMMAND_CHILD_ENABLE_SLOT_TRB \
    (PI4_XHCI_DMA_COMMAND_CONFIGURE_HUB_TRB + 1u)
#define PI4_XHCI_DMA_COMMAND_CHILD_ADDRESS_DEVICE_TRB \
    (PI4_XHCI_DMA_COMMAND_CONFIGURE_HUB_TRB + 2u)
/* Use the last command entry before the Link TRB to evaluate the child EP0
 * packet size learned from its first Device Descriptor packet. */
#define PI4_XHCI_DMA_COMMAND_CHILD_EVALUATE_CONTEXT_TRB \
    (PI4_XHCI_DMA_COMMAND_CHILD_ADDRESS_DEVICE_TRB + 1u)
/* Consecutive child EP0 control TDs. Keep a second identical 8-byte
 * descriptor read before GET_CONFIGURATION to distinguish a repeated-TD
 * failure from a standard-request-specific failure. */
#define PI4_XHCI_DMA_CHILD_DESCRIPTOR_PREFIX_SETUP_TRB 0u
#define PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_SETUP_TRB 3u
#define PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_SETUP_TRB 6u
#define PI4_XHCI_DMA_CHILD_POST_EVALUATE_GET_STATUS_SETUP_TRB 9u
#define PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_SETUP_TRB 12u
#define PI4_XHCI_DMA_CHILD_CONFIG_HEADER_SETUP_TRB 15u
#define PI4_XHCI_DMA_CHILD_CONFIG_FULL_SETUP_TRB 18u
#define PI4_XHCI_DMA_CHILD_SET_CONFIGURATION_SETUP_TRB 21u
#define PI4_XHCI_DMA_CHILD_GET_CONFIGURATION_SETUP_TRB 23u
#define PI4_XHCI_DMA_CHILD_BOT_GET_MAX_LUN_SETUP_TRB 26u
/* Direct-root EP0 sequence follows Device (0..2) and Config Header (3..5). */
#define PI4_XHCI_DMA_ROOT_CONFIG_FULL_TRB 6u
#define PI4_XHCI_DMA_ROOT_SET_CONFIG_TRB 9u
#define PI4_XHCI_DMA_ROOT_GET_CONFIG_TRB 11u
#define PI4_XHCI_DMA_ROOT_MAX_LUN_TRB 14u
#define PI4_XHCI_DMA_ROOT_CONFIG_OFFSET 128u
#define PI4_XHCI_DMA_ROOT_VALUE_OFFSET (PI4_XHCI_DMA_CONTROL_BUFFER_BYTES - 128u)
#define PI4_XHCI_DMA_ROOT_LUN_OFFSET (PI4_XHCI_DMA_CONTROL_BUFFER_BYTES - 64u)
#define PI4_XHCI_DMA_ROOT_CONFIG_CAPACITY \
    (PI4_XHCI_DMA_ROOT_VALUE_OFFSET - PI4_XHCI_DMA_ROOT_CONFIG_OFFSET)
/* Reuse the parsed configuration buffer only after EP0 has finished. Each
 * BOT buffer owns a separate cache line; the device/header at 0/64 survive. */
#define PI4_XHCI_DMA_BOT_CBW_OFFSET 512u
#define PI4_XHCI_DMA_BOT_DATA_OFFSET 576u
#define PI4_XHCI_DMA_BOT_CSW_OFFSET 640u
#define PI4_XHCI_DMA_BOT_CBW_BYTES 31u
#define PI4_XHCI_DMA_BOT_INQUIRY_BYTES 36u
#define PI4_XHCI_DMA_BOT_CSW_BYTES 13u
#define PI4_XHCI_DMA_BOT_INQUIRY_TAG 0x5a504901u
#define PI4_XHCI_DMA_BOT_SECTOR_OFFSET 1024u
#define PI4_XHCI_DMA_BOT_MAX_BLOCK_BYTES 2048u
#define PI4_XHCI_DMA_BOT_STORAGE_COMMANDS 32u
#define PI4_XHCI_DMA_COMMAND_CHILD_BULK_GUARD_NOOP_TRB 0u
#define PI4_XHCI_DMA_COMMAND_CHILD_BULK_CONFIGURE_TRB 1u
#define PI4_XHCI_MFINDEX_MASK     0x3fffu
#define PI4_XHCI_IMAN_IE           (1u << 1)

struct pi4_xhci_dma_event_cursor {
    unsigned index;
    unsigned cycle;
};

/* Single outstanding TD/command per ring. A slot is reused only after its
 * completion was consumed. Publish the Link before the final data TRB, and
 * expose payload bytes with the new cycle invalid before publishing ownership.
 * sync must clean the supplied CPU-written range and complete a DMA barrier. */
struct pi4_xhci_dma_producer {
    unsigned index, cycle, wraps;
};
typedef void (*pi4_xhci_dma_publish_fn)(uint8_t *, uint64_t);

static int pi4_xhci_dma_ring_enqueue(uint8_t *ring, uint64_t bus,
        struct pi4_xhci_dma_producer *p, const uint32_t words[4],
        pi4_xhci_dma_publish_fn sync, unsigned *queued_index) {
    const unsigned link = PI4_XHCI_DMA_RING_BYTES / 16u - 1u;
    if (!ring || !p || !words || !sync || !queued_index ||
        ((uintptr_t)ring & 63u) || (bus & 63u) ||
        p->index >= link || p->cycle > 1u ||
        ((words[3] >> 10) & 63u) == PI4_XHCI_TRB_TYPE_LINK)
        return -1;
    if (p->index == link - 1u) {
        volatile uint32_t *t = (volatile uint32_t *)(void *)(ring + link * 16u);
        t[3] = (PI4_XHCI_TRB_TYPE_LINK << 10) | 2u | (p->cycle ^ 1u);
        t[0] = (uint32_t)bus; t[1] = (uint32_t)(bus >> 32); t[2] = 0;
        sync(ring + link * 16u, 16u);
        t[3] = (PI4_XHCI_TRB_TYPE_LINK << 10) | 2u | p->cycle;
        sync(ring + link * 16u, 16u);
    }
    volatile uint32_t *t = (volatile uint32_t *)(void *)(ring + p->index * 16u);
    t[3] = (words[3] & ~1u) | (p->cycle ^ 1u);
    t[0] = words[0]; t[1] = words[1]; t[2] = words[2];
    sync(ring + p->index * 16u, 16u);
    t[3] = (words[3] & ~1u) | p->cycle;
    sync(ring + p->index * 16u, 16u);
    *queued_index = p->index;
    if (++p->index == link) {
        p->index = 0; p->cycle ^= 1u; p->wraps++;
    }
    return 0;
}

/* The bootstrap command ring starts at index 248. Its skipped prefix was
 * never visited by xHC: zero Cycle bits there would appear owned immediately
 * after the first Link toggles the consumer to zero. Invalidate that prefix
 * for the *next* lap before allowing the current lap to reach the Link. */
static int pi4_xhci_dma_ring_skipped_prefix(uint8_t *ring, unsigned end,
        unsigned cycle, pi4_xhci_dma_publish_fn sync) {
    if (!ring || ((uintptr_t)ring & 63u) || !sync || cycle > 1u ||
        end >= PI4_XHCI_DMA_RING_BYTES / 16u - 1u) return -1;
    volatile uint32_t *t = (volatile uint32_t *)(void *)ring;
    for (unsigned i = 0; i < end; i++) t[i * 4u + 3u] = cycle;
    sync(ring, (uint64_t)end * 16u);
    return 0;
}

static int pi4_xhci_dma_event_ready(uint32_t control,
        const struct pi4_xhci_dma_event_cursor *cursor) {
    return cursor && cursor->index < PI4_XHCI_DMA_EVENT_TRBS &&
           cursor->cycle <= 1u && (control & 1u) == cursor->cycle;
}

static int pi4_xhci_dma_event_cursor_advance(
        struct pi4_xhci_dma_event_cursor *cursor) {
    if (!cursor || cursor->index >= PI4_XHCI_DMA_EVENT_TRBS ||
        cursor->cycle > 1u) return -1;
    cursor->index++;
    if (cursor->index == PI4_XHCI_DMA_EVENT_TRBS) {
        cursor->index = 0;
        cursor->cycle ^= 1u;
    }
    return 0;
}

static uint64_t pi4_xhci_dma_event_dequeue(uint64_t event_bus,
        const struct pi4_xhci_dma_event_cursor *cursor) {
    if (!cursor || cursor->index >= PI4_XHCI_DMA_EVENT_TRBS)
        return UINT64_MAX;
    return (event_bus + (uint64_t)cursor->index * 16u) | 8u;
}

/* Poll the ring with local event delivery enabled but host IRQs suppressed. */
static uint32_t pi4_xhci_dma_run_command_value(uint32_t usbcmd) {
    return (usbcmd & ~PI4_XHCI_USBCMD_INTE) |
           PI4_XHCI_USBCMD_RUN | PI4_XHCI_USBCMD_EWE;
}
#define PI4_XHCI_PORTSC_CCS        (1u << 0)
#define PI4_XHCI_PORTSC_PED        (1u << 1)
#define PI4_XHCI_PORTSC_PR         (1u << 4)
#define PI4_XHCI_PORTSC_PLS_MASK   (0xfu << 5)
#define PI4_XHCI_PORTSC_SPEED_SHIFT 10u
#define PI4_XHCI_PORTSC_SPEED_MASK (0xfu << PI4_XHCI_PORTSC_SPEED_SHIFT)
#define PI4_XHCI_PORTSC_PP         (1u << 9)
#define PI4_XHCI_PORTSC_WAKE_MASK  (7u << 25)
#define PI4_XHCI_PORTSC_PRC        (1u << 21)
#define PI4_XHCI_PORTSC_WRC        (1u << 19)
#define PI4_XHCI_PORTSC_WPR        (1u << 31)
#define PI4_XHCI_PORTSC_CHANGE_MASK (0x7fu << 17)

/* Protocol ownership is valid while PORTSC's speed is still zero during
 * link training/reset. Retain a connected USB 3 candidate rather than
 * silently falling back to the always-connected USB 2 hub. */
static unsigned pi4_xhci_dma_select_root_port(const uint32_t *ports,
        const uint32_t *protocols, unsigned count) {
    unsigned fallback = 0, pending = 0;
    if (!ports || count > PI4_XHCI_DIAG_PORTS) return 0;
    for (unsigned i = 0; i < count; i++) {
        uint32_t required = PI4_XHCI_PORTSC_CCS | PI4_XHCI_PORTSC_PP;
        if ((ports[i] & required) != required) continue;
        unsigned speed = (ports[i] & PI4_XHCI_PORTSC_SPEED_MASK) >>
                          PI4_XHCI_PORTSC_SPEED_SHIFT;
        if ((protocols && protocols[i] == 3u) || speed == 4u) {
            if (!pending) pending = i + 1u;
            if (speed == 4u &&
                !(ports[i] & (PI4_XHCI_PORTSC_PR | PI4_XHCI_PORTSC_WPR)))
                return i + 1u;
        } else if (!fallback) {
            fallback = i + 1u;
        }
    }
    return pending ? pending : fallback;
}

static int pi4_xhci_dma_superspeed_port_settled(uint32_t portsc,
                                                unsigned protocol) {
    const uint32_t required = PI4_XHCI_PORTSC_CCS | PI4_XHCI_PORTSC_PP;
    unsigned speed = (portsc & PI4_XHCI_PORTSC_SPEED_MASK) >>
                      PI4_XHCI_PORTSC_SPEED_SHIFT;
    return (protocol == 3u || speed == 4u) && speed == 4u &&
           (portsc & required) == required &&
           !(portsc & (PI4_XHCI_PORTSC_PR | PI4_XHCI_PORTSC_WPR));
}

/* A VL805 SuperSpeed link needs warm-reset completion; USB 2 uses PRC. */
static uint32_t pi4_xhci_dma_port_reset_change(uint32_t portsc) {
    return ((portsc & PI4_XHCI_PORTSC_SPEED_MASK) >>
            PI4_XHCI_PORTSC_SPEED_SHIFT) == 4u
        ? PI4_XHCI_PORTSC_WRC : PI4_XHCI_PORTSC_PRC;
}

#define PI4_XHCI_DMA_SETUP_CONFIG  (1u << 0)
#define PI4_XHCI_DMA_SETUP_DCBAAP  (1u << 1)
#define PI4_XHCI_DMA_SETUP_ERSTSZ  (1u << 2)
#define PI4_XHCI_DMA_SETUP_ERSTBA  (1u << 3)
#define PI4_XHCI_DMA_SETUP_ERDP    (1u << 4)
#define PI4_XHCI_DMA_SETUP_IMAN    (1u << 5)
#define PI4_XHCI_DMA_SETUP_IMOD    (1u << 6)
#define PI4_XHCI_DMA_SETUP_ALL     0x7fu
#define PI4_XHCI_DMA_SETUP_BAD_MASK  (1u << 0)
#define PI4_XHCI_DMA_SETUP_BAD_IMAN  (1u << 1)
#define PI4_XHCI_DMA_SETUP_BAD_IMOD  (1u << 2)
#define PI4_XHCI_DMA_SETUP_BAD_CONFIG (1u << 3)
#define PI4_XHCI_DMA_SETUP_BAD_DCBAAP (1u << 4)
#define PI4_XHCI_DMA_SETUP_BAD_ERSTSZ (1u << 5)
#define PI4_XHCI_DMA_SETUP_BAD_ERSTBA (1u << 6)
#define PI4_XHCI_DMA_SETUP_BAD_ERDP   (1u << 7)
#define PI4_XHCI_IMODI_MASK        0xffffu
#define PI4_XHCI_IMODI_MIN         1u

struct pi4_xhci_dma_layout {
    uint64_t arena_phys;
    uint64_t arena_bus;
    uint64_t arena_size;
    uint64_t dcbaa_offset;
    uint64_t scratch_array_offset;
    uint64_t scratch_pages_offset;
    uint64_t command_offset;
    uint64_t event_offset;
    uint64_t erst_offset;
    uint64_t input_context_offset;
    uint64_t output_context_offset;
    uint64_t ep0_ring_offset;
    uint64_t control_buffer_offset;
    uint64_t child_output_context_offset;
    uint64_t child_ep0_ring_offset;
    uint64_t child_bulk_in_ring_offset;
    uint64_t child_bulk_out_ring_offset;
    uint64_t used_bytes;
    uint32_t max_slots;
    uint32_t scratchpads;
};

/* Decode one xHCI Supported Protocol capability for a 1-based root port.
 * Return 1 when the capability owns the port, 0 when it does not, and -1 for
 * malformed or out-of-range fields. Protocol Slot Type is five bits; zero is
 * a valid value and denotes the legacy USB Protocol Device Slot type. */
static int pi4_xhci_dma_protocol_cap_for_port(uint32_t header,
        uint32_t port_range, uint32_t slot_word, unsigned root_port,
        unsigned max_ports, unsigned *protocol_major, unsigned *slot_type) {
    unsigned id = header & 0xffu;
    unsigned major = (header >> 24) & 0xffu;
    unsigned first_port = port_range & 0xffu;
    unsigned port_count = (port_range >> 8) & 0xffu;
    unsigned last_port = first_port + port_count;
    unsigned type = slot_word & PI4_XHCI_PROTOCOL_SLOT_TYPE_MASK;

    if (id != PI4_XHCI_EXT_CAP_SUPPORTED_PROTOCOL) return 0;
    if (!root_port || !max_ports || root_port > max_ports ||
        !first_port || !port_count || last_port <= first_port ||
        last_port - 1u > max_ports || (slot_word >> 5) ||
        (major != 2u && major != 3u))
        return -1;
    if (root_port < first_port || root_port >= last_port) return 0;
    if (!protocol_major || !slot_type) return -1;
    *protocol_major = major;
    *slot_type = type;
    return 1;
}

/* Advance a relative Next Capability Pointer (in DWORDs) within the bounded
 * xHCI extended-capability aperture. Return 1 for next, 0 for end, -1 bad. */
static int pi4_xhci_dma_ext_cap_next(uint32_t offset, uint32_t header,
                                     uint32_t span, uint32_t *next_offset) {
    unsigned next = (header >> 8) & 0xffu;
    uint32_t bytes;
    if (!next) return 0;
    if (!next_offset || span < 4u || (offset & 3u) ||
        offset > span - 4u ||
        next > (span - 4u - offset) / 4u)
        return -1;
    bytes = (uint32_t)next * 4u;
    if (!bytes || offset > UINT32_MAX - bytes) return -1;
    *next_offset = offset + bytes;
    return 1;
}

static uint64_t pi4_xhci_dma_align_up(uint64_t value, uint64_t alignment) {
    if (!alignment || (alignment & (alignment - 1)) ||
        value > UINT64_MAX - (alignment - 1)) return UINT64_MAX;
    return (value + alignment - 1) & ~(alignment - 1);
}

/* Start with unused command entries cycle-invalid. The probe publishes each
 * command just before ringing the command doorbell; pre-seeding valid No-ops
 * lets the xHC consume future command positions before software replaces
 * them. Keep the Link TRB ready at the end of the segment and make entry zero
 * invalid for the post-Link cycle state. */
static int pi4_xhci_dma_command_ring_seed(void *memory, uint64_t ring_bus) {
    volatile uint32_t *ring = (volatile uint32_t *)memory;
    const unsigned link_word = (PI4_XHCI_DMA_RING_BYTES / 16u - 1u) * 4u;

    if (!memory || ((uintptr_t)memory & 15u) || (ring_bus & 63u)) return -1;
    for (unsigned i = 0; i < PI4_XHCI_DMA_COMMAND_WRAP_PRELINK_COUNT; i++) {
        unsigned word = (PI4_XHCI_DMA_COMMAND_WRAP_START_TRB + i) * 4u;
        ring[word + 0] = 0;
        ring[word + 1] = 0;
        ring[word + 2] = 0;
        ring[word + 3] = 0;
    }
    ring[0] = 0;
    ring[1] = 0;
    ring[2] = 0;
    ring[3] = (PI4_XHCI_TRB_TYPE_NOOP << 10) | 1u;
    ring[link_word + 0] = (uint32_t)ring_bus;
    ring[link_word + 1] = (uint32_t)(ring_bus >> 32);
    ring[link_word + 2] = 0;
    ring[link_word + 3] = (PI4_XHCI_TRB_TYPE_LINK << 10) | (1u << 1) | 1u;
    return 0;
}

/* Require a fresh halted/reset controller, no existing BME, and a valid 4K
 * page-capable controller before any device-visible pointer is programmed. */
static int pi4_xhci_dma_layout_make(
    const struct pi4_dma_probe_plan *arena, const uint32_t caps[6],
    uint32_t page_size, uint32_t usbcmd, uint32_t usbsts, uint32_t legacy,
    uint16_t pci_command, struct pi4_xhci_dma_layout *out) {
    uint64_t cursor, bytes, bus_end;
    uint32_t scratch;

    if (!arena || !caps || !out ||
        !pi4_xhci_reset_safe(usbcmd, usbsts, legacy, pci_command))
        return PI4_XHCI_DMA_PLAN_GATE;
    if (!pi4_xhci_caps_valid(caps) ||
        (caps[1] & 0xffu) < PI4_XHCI_DMA_MAX_SLOTS ||
        !(caps[1] >> 24))
        return PI4_XHCI_DMA_PLAN_CAPS;
    /* PAGESIZE is a 16-bit supported-page-size bitmap; upper bits are reserved. */
    if (!(page_size & 1u) || (page_size & 0xffff0000u))
        return PI4_XHCI_DMA_PLAN_PAGES;
    if (!arena->size ||
        arena->phys_base > UINT64_MAX - arena->size ||
        arena->bus_base > UINT64_MAX - arena->size)
        return PI4_XHCI_DMA_PLAN_ARENA;

    /* HCSParams2 scratchpad count is split across two five-bit fields. */
    scratch = ((caps[2] >> 27) & 31u) | (((caps[2] >> 21) & 31u) << 5);
    *out = (struct pi4_xhci_dma_layout){
        .arena_phys = arena->phys_base,
        .arena_bus = arena->bus_base,
        .arena_size = arena->size,
        .max_slots = PI4_XHCI_DMA_MAX_SLOTS,
        .scratchpads = scratch
    };
    cursor = 0;
    out->dcbaa_offset = cursor;
    cursor += PI4_XHCI_DMA_ALIGN; /* DCBAA[0] + six permitted slots fit in 64 bytes. */
    if (scratch) {
        cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_ALIGN);
        if (cursor == UINT64_MAX) return PI4_XHCI_DMA_PLAN_ARENA;
        out->scratch_array_offset = cursor;
        bytes = (uint64_t)scratch * sizeof(uint64_t);
        if (cursor > UINT64_MAX - bytes) return PI4_XHCI_DMA_PLAN_ARENA;
        cursor += bytes;
        cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_PAGE);
        if (cursor == UINT64_MAX) return PI4_XHCI_DMA_PLAN_ARENA;
        out->scratch_pages_offset = cursor;
        bytes = (uint64_t)scratch * PI4_XHCI_DMA_PAGE;
        if (cursor > UINT64_MAX - bytes) return PI4_XHCI_DMA_PLAN_ARENA;
        cursor += bytes;
    }
    cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_RING_ALLOC_BYTES);
    if (cursor == UINT64_MAX ||
        cursor > UINT64_MAX - PI4_XHCI_DMA_RING_ALLOC_BYTES)
        return PI4_XHCI_DMA_PLAN_ARENA;
    out->command_offset = cursor;
    cursor += PI4_XHCI_DMA_RING_ALLOC_BYTES;
    cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_RING_ALLOC_BYTES);
    if (cursor == UINT64_MAX ||
        cursor > UINT64_MAX - PI4_XHCI_DMA_RING_ALLOC_BYTES)
        return PI4_XHCI_DMA_PLAN_ARENA;
    out->event_offset = cursor;
    cursor += PI4_XHCI_DMA_RING_ALLOC_BYTES;
    cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_ALIGN);
    if (cursor == UINT64_MAX || cursor > UINT64_MAX - PI4_XHCI_DMA_ALIGN)
        return PI4_XHCI_DMA_PLAN_ARENA;
    out->erst_offset = cursor;
    cursor += PI4_XHCI_DMA_ALIGN;
    cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_CONTEXT_BYTES);
    if (cursor == UINT64_MAX ||
        cursor > UINT64_MAX - PI4_XHCI_DMA_CONTEXT_BYTES)
        return PI4_XHCI_DMA_PLAN_ARENA;
    out->input_context_offset = cursor;
    cursor += PI4_XHCI_DMA_CONTEXT_BYTES;
    cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_CONTEXT_BYTES);
    if (cursor == UINT64_MAX ||
        cursor > UINT64_MAX - PI4_XHCI_DMA_CONTEXT_BYTES)
        return PI4_XHCI_DMA_PLAN_ARENA;
    out->output_context_offset = cursor;
    cursor += PI4_XHCI_DMA_CONTEXT_BYTES;
    cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_RING_ALLOC_BYTES);
    if (cursor == UINT64_MAX ||
        cursor > UINT64_MAX - PI4_XHCI_DMA_RING_ALLOC_BYTES)
        return PI4_XHCI_DMA_PLAN_ARENA;
    out->ep0_ring_offset = cursor;
    cursor += PI4_XHCI_DMA_RING_ALLOC_BYTES;
    cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_PAGE);
    if (cursor == UINT64_MAX ||
        cursor > UINT64_MAX - PI4_XHCI_DMA_CONTROL_BUFFER_BYTES)
        return PI4_XHCI_DMA_PLAN_ARENA;
    out->control_buffer_offset = cursor;
    cursor += PI4_XHCI_DMA_CONTROL_BUFFER_BYTES;
    cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_CONTEXT_BYTES);
    if (cursor == UINT64_MAX ||
        cursor > UINT64_MAX - PI4_XHCI_DMA_CONTEXT_BYTES)
        return PI4_XHCI_DMA_PLAN_ARENA;
    out->child_output_context_offset = cursor;
    cursor += PI4_XHCI_DMA_CONTEXT_BYTES;
    cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_RING_ALLOC_BYTES);
    if (cursor == UINT64_MAX ||
        cursor > UINT64_MAX - PI4_XHCI_DMA_RING_ALLOC_BYTES)
        return PI4_XHCI_DMA_PLAN_ARENA;
    out->child_ep0_ring_offset = cursor;
    cursor += PI4_XHCI_DMA_RING_ALLOC_BYTES;
    cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_RING_ALLOC_BYTES);
    if (cursor == UINT64_MAX ||
        cursor > UINT64_MAX - PI4_XHCI_DMA_RING_ALLOC_BYTES)
        return PI4_XHCI_DMA_PLAN_ARENA;
    out->child_bulk_in_ring_offset = cursor;
    cursor += PI4_XHCI_DMA_RING_ALLOC_BYTES;
    cursor = pi4_xhci_dma_align_up(cursor, PI4_XHCI_DMA_RING_ALLOC_BYTES);
    if (cursor == UINT64_MAX ||
        cursor > UINT64_MAX - PI4_XHCI_DMA_RING_ALLOC_BYTES)
        return PI4_XHCI_DMA_PLAN_ARENA;
    out->child_bulk_out_ring_offset = cursor;
    cursor += PI4_XHCI_DMA_RING_ALLOC_BYTES;
    if (cursor > arena->size) return PI4_XHCI_DMA_PLAN_ARENA;
    out->used_bytes = cursor;

    bus_end = arena->bus_base + cursor;
    if (!(caps[3] & 1u) && bus_end > (1ull << 32))
        return PI4_XHCI_DMA_PLAN_ADDRESS;
    return PI4_XHCI_DMA_PLAN_OK;
}

/* Event ownership is checked against the consumer cursor before validating
 * identity. Both cycle values are valid across event-ring laps. */
static int pi4_xhci_dma_command_event_matches(uint64_t parameter,
        uint32_t control, uint64_t command_bus) {
    return ((control >> 10) & 0x3fu) == PI4_XHCI_TRB_TYPE_CMD_EVT &&
           (parameter & ~0xfull) == (command_bus & ~0xfull);
}

static int pi4_xhci_dma_command_event_ok(uint64_t parameter, uint32_t status,
                                         uint32_t control,
                                         uint64_t command_bus) {
    return pi4_xhci_dma_command_event_matches(parameter, control, command_bus) &&
           (status >> 24) == 1u;
}

static unsigned pi4_xhci_dma_context_size(uint32_t hccparams1) {
    return (hccparams1 & (1u << 2)) ? 64u : 32u;
}

static unsigned pi4_xhci_dma_ep0_default_mps(unsigned speed_id) {
    switch (speed_id) {
    case 1: return 8u;   /* Full speed, before the device descriptor read. */
    case 2: return 8u;   /* Low speed. */
    case 3: return 64u;  /* High speed. */
    case 4: return 512u; /* SuperSpeed. */
    default: return 0u;
    }
}

/* Build the Address Device input contexts for EP0 only. Buffers are zeroed by
 * the caller; Input Control, Slot, and EP0 contexts are the only fields set. */
static int pi4_xhci_dma_address_context_seed(void *input_context,
        uint64_t input_bytes, unsigned context_size, unsigned speed_id,
        unsigned root_port, uint64_t ep0_ring_bus) {
    uint8_t *bytes = (uint8_t *)input_context;
    uint32_t *control, *slot, *ep0;
    unsigned mps = pi4_xhci_dma_ep0_default_mps(speed_id);
    if (!bytes || (uintptr_t)bytes % 16u ||
        (context_size != 32u && context_size != 64u) || !mps ||
        !root_port || root_port > 255u || (ep0_ring_bus & 15u) ||
        input_bytes < (uint64_t)context_size * 3u)
        return -1;
    control = (uint32_t *)(void *)bytes;
    slot = (uint32_t *)(void *)(bytes + context_size);
    ep0 = (uint32_t *)(void *)(bytes + context_size * 2u);
    control[0] = 0; /* Drop Context flags. */
    control[1] = 3u; /* Add Slot Context and Endpoint 0 Context. */
    slot[0] = (speed_id << 20) | (1u << 27); /* EP0 is last valid context. */
    slot[1] = root_port << 16; /* Device is directly attached to this root port. */
    ep0[1] = (3u << 1) | (4u << 3) | (mps << 16); /* CErr=3, control EP. */
    ep0[2] = (uint32_t)(ep0_ring_bus | 1u); /* DCS=1, producer cycle. */
    ep0[3] = (uint32_t)(ep0_ring_bus >> 32);
    ep0[4] = 8u; /* Average TRB length for EP0. */
    return 0;
}

/* Build the Slot-only Configure Endpoint context from the current output Slot
 * Context. Preserve route/speed/root-port fields while changing only the hub
 * fields; endpoint flags remain clear. */
static int pi4_xhci_dma_hub_context_seed(void *input_context,
        uint64_t input_bytes, unsigned context_size,
        const void *output_slot_context, unsigned ports,
        unsigned tt_think_time, unsigned multi_tt) {
    uint8_t *bytes = (uint8_t *)input_context;
    const uint32_t *source_slot = (const uint32_t *)output_slot_context;
    uint32_t *control, *slot;
    if (!bytes || (uintptr_t)bytes % 16u ||
        !source_slot || (uintptr_t)source_slot % 16u ||
        (context_size != 32u && context_size != 64u) ||
        input_bytes < (uint64_t)context_size * 2u || !ports || ports > 255u ||
        tt_think_time > 3u || multi_tt > 1u ||
        ((source_slot[0] >> 20) & 0xfu) != 3u ||
        !((source_slot[0] >> 27) & 0x1fu) ||
        ((source_slot[1] >> 16) & 0xffu) == 0u)
        return -1;
    control = (uint32_t *)(void *)bytes;
    slot = (uint32_t *)(void *)(bytes + context_size);
    control[0] = 0;
    control[1] = 1u; /* Add Slot Context; leave all endpoint flags clear. */
    for (unsigned i = 0; i < context_size / sizeof(uint32_t); i++)
        slot[i] = source_slot[i];
    slot[0] = (slot[0] & ~((1u << 25) | (1u << 26))) |
              (1u << 26) | (multi_tt << 25);
    slot[1] = (slot[1] & 0x00ffffffu) | (ports << 24);
    slot[2] = (slot[2] & ~(3u << 16)) | (tt_think_time << 16);
    return 0;
}

/* USB 2.0 hub port status reports Low-Speed and High-Speed explicitly; when
 * neither bit is set on a connected port, the child is Full-Speed. */
static unsigned pi4_xhci_dma_hub_child_speed_id(uint32_t port_status) {
    unsigned low = (port_status >> 9) & 1u;
    unsigned high = (port_status >> 10) & 1u;
    if (!(port_status & PI4_XHCI_HUB_PORT_STATUS_CONNECTION) ||
        (low && high)) return 0;
    return low ? 2u : (high ? 3u : 1u);
}

static uint32_t pi4_xhci_dma_route_string_append(uint32_t parent_route,
        unsigned parent_depth, unsigned port) {
    uint32_t used_mask;
    if (parent_depth >= 5u || !port || port > 15u ||
        (parent_route & ~0xfffffu)) return UINT32_MAX;
    used_mask = parent_depth ? ((1u << (parent_depth * 4u)) - 1u) : 0u;
    if (parent_route & ~used_mask) return UINT32_MAX;
    return parent_route | ((uint32_t)port << (parent_depth * 4u));
}

/* Child Address Device input context. TT parent fields are needed only for a
 * low/full-speed child behind a high-speed hub. */
static int pi4_xhci_dma_child_address_context_seed(void *input_context,
        uint64_t input_bytes, unsigned context_size, unsigned speed_id,
        unsigned root_port, uint32_t route_string, unsigned parent_hub_slot,
        unsigned parent_port, unsigned parent_hub_speed, unsigned parent_mtt,
        uint64_t ep0_ring_bus) {
    uint8_t *bytes = (uint8_t *)input_context;
    uint32_t *control, *slot, *ep0;
    unsigned mps = pi4_xhci_dma_ep0_default_mps(speed_id);
    if (!bytes || (uintptr_t)bytes % 16u ||
        (context_size != 32u && context_size != 64u) || !mps ||
        !root_port || root_port > 255u || (route_string & ~0xfffffu) ||
        (ep0_ring_bus & 15u) || parent_hub_slot > 255u ||
        parent_port > 255u || parent_hub_speed > 4u || parent_mtt > 1u ||
        input_bytes < (uint64_t)context_size * 3u)
        return -1;
    control = (uint32_t *)(void *)bytes;
    slot = (uint32_t *)(void *)(bytes + context_size);
    ep0 = (uint32_t *)(void *)(bytes + context_size * 2u);
    control[0] = 0;
    control[1] = 3u; /* Add Slot Context and EP0 Context. */
    slot[0] = route_string | (speed_id << 20) | (1u << 27);
    slot[1] = root_port << 16;
    slot[2] = 0;
    if (speed_id <= 2u && parent_hub_speed == 3u) {
        if (!parent_hub_slot || !parent_port) return -1;
        slot[0] |= parent_mtt << 25;
        slot[2] = parent_hub_slot | (parent_port << 8);
    }
    ep0[1] = (3u << 1) | (4u << 3) | (mps << 16);
    ep0[2] = (uint32_t)(ep0_ring_bus | 1u);
    ep0[3] = (uint32_t)(ep0_ring_bus >> 32);
    ep0[4] = 8u;
    return 0;
}

/* BOT uses ordinary bulk endpoints without streams. SuperSpeed additionally
 * supplies Max Burst Size from the endpoint's companion descriptor. */
static int pi4_xhci_dma_bulk_endpoint_context_seed(void *endpoint_context,
        uint64_t context_bytes, unsigned context_size,
        unsigned endpoint_address, unsigned attributes, unsigned max_packet,
        unsigned max_burst, unsigned speed_id, uint64_t ring_bus) {
    uint32_t *ep = (uint32_t *)endpoint_context;
    unsigned endpoint_number = endpoint_address & 0x0fu;
    unsigned direction_in = (endpoint_address >> 7) & 1u;
    unsigned endpoint_type = direction_in ? 6u : 2u;
    if (!ep || ((uintptr_t)ep & 15u) ||
        (context_size != 32u && context_size != 64u) ||
        context_bytes < context_size ||
        endpoint_address > 0xffu || attributes > 0xffu ||
        (endpoint_address & 0x70u) || !endpoint_number ||
        (attributes & 3u) != 2u || (attributes & 0xfcu) ||
        (speed_id != 1u && speed_id != 3u && speed_id != 4u) ||
        max_burst > 15u || (speed_id != 4u && max_burst) || (ring_bus & 63u))
        return -1;
    if ((speed_id == 4u && max_packet != 1024u) ||
        (speed_id == 3u && max_packet != 512u) ||
        (speed_id == 1u && max_packet != 8u && max_packet != 16u &&
         max_packet != 32u && max_packet != 64u))
        return -1;
    for (unsigned i = 0; i < context_size / sizeof(uint32_t); i++)
        ep[i] = 0;
    ep[1] = (3u << 1) | (endpoint_type << 3) | (max_burst << 8) |
            (max_packet << 16);
    ep[2] = (uint32_t)(ring_bus | 1u); /* DCS=1, transfer ring is empty. */
    ep[3] = (uint32_t)(ring_bus >> 32);
    ep[4] = PI4_XHCI_DMA_BULK_AVG_TRB_LENGTH;
    return 0;
}

static int pi4_xhci_dma_bulk_max_packet_valid(unsigned speed_id,
        unsigned max_packet) {
    if (speed_id == 1u)
        return max_packet == 8u || max_packet == 16u ||
               max_packet == 32u || max_packet == 64u;
    if (speed_id == 3u) return max_packet == 512u;
    if (speed_id == 4u) return max_packet == 1024u;
    return 0;
}

/* Add the selected USB Mass Storage BOT interface's bulk IN/OUT contexts.
 * Preserve the addressed child's output Slot Context and add only the two
 * endpoint DCI flags plus Slot Context required by Configure Endpoint. */
static int pi4_xhci_dma_bulk_contexts_seed(void *input_context,
        uint64_t input_bytes, unsigned context_size,
        const void *output_context, uint64_t output_bytes,
        unsigned interface_class, unsigned interface_subclass,
        unsigned interface_protocol, unsigned interface_alt,
        unsigned interface_endpoint_count, unsigned bulk_in_address,
        unsigned bulk_in_attributes, unsigned bulk_in_max_packet,
        unsigned bulk_in_max_burst,
        unsigned bulk_out_address, unsigned bulk_out_attributes,
        unsigned bulk_out_max_packet, unsigned bulk_out_max_burst, unsigned speed_id,
        uint64_t bulk_in_ring_bus, uint64_t bulk_out_ring_bus) {
    uint8_t *bytes = (uint8_t *)input_context;
    const uint8_t *out = (const uint8_t *)output_context;
    uint32_t *control, *slot;
    uint32_t *in_ep, *out_ep;
    unsigned in_dci, out_dci, last_dci;
    uint64_t required;
    if (!bytes || ((uintptr_t)bytes & 15u) || !out ||
        ((uintptr_t)out & 15u) ||
        (context_size != 32u && context_size != 64u) ||
        interface_class != 8u || interface_subclass != 6u ||
        interface_protocol != 0x50u || interface_alt != 0u ||
        interface_endpoint_count != 2u ||
        bulk_in_address > 0xffu || bulk_out_address > 0xffu ||
        (bulk_in_address & 0x80u) == 0u || (bulk_out_address & 0x80u) != 0u)
        return -1;
    in_dci = ((bulk_in_address & 0x0fu) * 2u) + 1u;
    out_dci = ((bulk_out_address & 0x0fu) * 2u);
    if (in_dci < 2u || in_dci > 31u || out_dci < 2u || out_dci > 31u ||
        in_dci == out_dci)
        return -1;
    last_dci = in_dci > out_dci ? in_dci : out_dci;
    required = (uint64_t)(last_dci + 2u) * context_size;
    if (input_bytes < required ||
        output_bytes < (uint64_t)(last_dci + 1u) * context_size ||
        ((uint64_t)bulk_in_ring_bus & 63u) ||
        ((uint64_t)bulk_out_ring_bus & 63u))
        return -1;
    control = (uint32_t *)(void *)bytes;
    slot = (uint32_t *)(void *)(bytes + context_size);
    for (uint64_t i = 0; i < required / sizeof(uint32_t); i++)
        ((uint32_t *)(void *)bytes)[i] = 0;
    control[1] = 1u | (1u << in_dci) | (1u << out_dci);
    for (unsigned i = 0; i < context_size / sizeof(uint32_t); i++)
        slot[i] = ((const uint32_t *)(const void *)out)[i];
    slot[0] = (slot[0] & ~(0x1fu << 27)) | (last_dci << 27);
    in_ep = (uint32_t *)(void *)(bytes + (uint64_t)(in_dci + 1u) * context_size);
    out_ep = (uint32_t *)(void *)(bytes + (uint64_t)(out_dci + 1u) * context_size);
    if (pi4_xhci_dma_bulk_endpoint_context_seed(in_ep, context_size,
            context_size, bulk_in_address, bulk_in_attributes,
            bulk_in_max_packet, bulk_in_max_burst, speed_id, bulk_in_ring_bus) != 0 ||
        pi4_xhci_dma_bulk_endpoint_context_seed(out_ep, context_size,
            context_size, bulk_out_address, bulk_out_attributes,
            bulk_out_max_packet, bulk_out_max_burst, speed_id, bulk_out_ring_bus) != 0)
        return -1;
    return 0;
}

/* Preserve the strict readback gate while identifying its failing fields.
 * Bits: arguments, slot, states, types, packets, bursts, streams, rings, avg. */
static unsigned pi4_xhci_dma_bulk_context_errors(const void *output_context,
        uint64_t output_bytes, unsigned context_size,
        unsigned bulk_in_address, unsigned bulk_in_max_packet,
        unsigned bulk_in_max_burst,
        uint64_t bulk_in_ring_bus, unsigned bulk_out_address,
        unsigned bulk_out_max_packet, unsigned bulk_out_max_burst,
        uint64_t bulk_out_ring_bus) {
    const uint8_t *bytes = (const uint8_t *)output_context;
    unsigned in_dci = ((bulk_in_address & 0x0fu) * 2u) + 1u;
    unsigned out_dci = ((bulk_out_address & 0x0fu) * 2u);
    const uint32_t *slot, *in_ep, *out_ep;
    if (!bytes || ((uintptr_t)bytes & 15u) ||
        (context_size != 32u && context_size != 64u) ||
        bulk_in_address > 0xffu || bulk_out_address > 0xffu ||
        (bulk_in_address & 0x80u) == 0u ||
        (bulk_out_address & 0x80u) != 0u ||
        (bulk_in_address & 0x70u) || (bulk_out_address & 0x70u) ||
        in_dci < 3u || out_dci < 2u ||
        output_bytes < (uint64_t)(in_dci + 1u) * context_size ||
        output_bytes < (uint64_t)(out_dci + 1u) * context_size)
        return 1u;
    slot = (const uint32_t *)(const void *)bytes;
    in_ep = (const uint32_t *)(const void *)(bytes + (uint64_t)in_dci * context_size);
    out_ep = (const uint32_t *)(const void *)(bytes + (uint64_t)out_dci * context_size);
    unsigned errors = 0;
    if (((slot[0] >> 27) & 31u) != (in_dci > out_dci ? in_dci : out_dci) ||
        ((slot[3] >> 27) & 31u) != 3u) errors |= 1u << 1;
    if ((in_ep[0] & 7u) != 1u || (out_ep[0] & 7u) != 1u) errors |= 1u << 2;
    if (((in_ep[1] >> 3) & 7u) != 6u ||
        ((out_ep[1] >> 3) & 7u) != 2u) errors |= 1u << 3;
    if ((in_ep[1] >> 16) != bulk_in_max_packet ||
        (out_ep[1] >> 16) != bulk_out_max_packet) errors |= 1u << 4;
    if (((in_ep[1] >> 8) & 255u) != bulk_in_max_burst ||
        ((out_ep[1] >> 8) & 255u) != bulk_out_max_burst) errors |= 1u << 5;
    if ((in_ep[0] & (31u << 10)) ||
        (out_ep[0] & (31u << 10))) errors |= 1u << 6;
    if ((((uint64_t)in_ep[2] | ((uint64_t)in_ep[3] << 32)) & ~1ull) !=
            bulk_in_ring_bus || !(in_ep[2] & 1u) ||
        (((uint64_t)out_ep[2] | ((uint64_t)out_ep[3] << 32)) & ~1ull) !=
            bulk_out_ring_bus || !(out_ep[2] & 1u)) errors |= 1u << 7;
    if (in_ep[4] != PI4_XHCI_DMA_BULK_AVG_TRB_LENGTH ||
        out_ep[4] != PI4_XHCI_DMA_BULK_AVG_TRB_LENGTH) errors |= 1u << 8;
    return errors;
}

static int pi4_xhci_dma_bulk_contexts_ok(const void *output_context,
        uint64_t output_bytes, unsigned context_size,
        unsigned bulk_in_address, unsigned bulk_in_max_packet,
        unsigned bulk_in_max_burst, uint64_t bulk_in_ring_bus,
        unsigned bulk_out_address, unsigned bulk_out_max_packet,
        unsigned bulk_out_max_burst, uint64_t bulk_out_ring_bus) {
    return !pi4_xhci_dma_bulk_context_errors(output_context, output_bytes,
        context_size, bulk_in_address, bulk_in_max_packet, bulk_in_max_burst,
        bulk_in_ring_bus, bulk_out_address, bulk_out_max_packet,
        bulk_out_max_burst, bulk_out_ring_bus);
}

/* Output Context index 0 is the Slot Context and DCI 1 (EP0) is index 1.
 * Input Context has a leading Input Control Context, so its EP0 is index 2;
 * keep the two layouts explicit to avoid accidentally reading output DCI 2. */
static const uint32_t *pi4_xhci_dma_output_ep0_context(
        const void *output_context, uint64_t output_bytes,
        unsigned context_size) {
    const uint8_t *bytes = (const uint8_t *)output_context;
    if (!bytes || ((uintptr_t)bytes & 15u) ||
        (context_size != 32u && context_size != 64u) ||
        output_bytes < (uint64_t)context_size * 2u)
        return 0;
    return (const uint32_t *)(const void *)(bytes + context_size);
}

/* Evaluate Context updates the addressed Full-Speed child's EP0 MPS from the
 * first descriptor packet. Preserve the accepted output EP0 context, then
 * change only Max Packet Size and set Add Context flag DCI 1. */
static int pi4_xhci_dma_child_ep0_evaluate_context_seed(void *input_context,
        uint64_t input_bytes, unsigned context_size,
        const void *output_context, uint64_t output_bytes, unsigned speed_id,
        unsigned max_packet_size) {
    uint8_t *bytes = (uint8_t *)input_context;
    const uint32_t *source = (const uint32_t *)
        pi4_xhci_dma_output_ep0_context(output_context, output_bytes,
                                        context_size);
    uint32_t *control, *ep0;
    if (!bytes || ((uintptr_t)bytes & 15u) || !source ||
        (context_size != 32u && context_size != 64u) ||
        input_bytes < (uint64_t)context_size * 3u ||
        speed_id != 1u ||
        (max_packet_size != 8u && max_packet_size != 16u &&
         max_packet_size != 32u && max_packet_size != 64u))
        return -1;
    control = (uint32_t *)(void *)bytes;
    ep0 = (uint32_t *)(void *)(bytes + context_size * 2u);
    for (unsigned i = 0; i < context_size * 3u / sizeof(uint32_t); i++)
        ((uint32_t *)(void *)bytes)[i] = 0;
    control[1] = 1u << 1; /* Add EP0 only; do not update Slot Context. */
    for (unsigned i = 0; i < 5u; i++) ep0[i] = source[i];
    ep0[1] = (ep0[1] & ~(0xffffu << 16)) |
             ((uint32_t)max_packet_size << 16);
    return 0;
}

static int pi4_xhci_dma_evaluate_context_seed(uint32_t trb[4],
        uint64_t input_context_bus, unsigned slot_id, unsigned cycle) {
    if (!trb || (uintptr_t)trb & 15u ||
        (input_context_bus & 15u) || !slot_id || slot_id > 255u ||
        cycle > 1u)
        return -1;
    trb[0] = (uint32_t)input_context_bus;
    trb[1] = (uint32_t)(input_context_bus >> 32);
    trb[2] = 0;
    trb[3] = (PI4_XHCI_TRB_TYPE_EVALUATE_CONTEXT << 10) |
             (slot_id << 24) | cycle;
    return 0;
}

static unsigned pi4_xhci_dma_output_ep0_max_packet_size(
        const void *output_ep0_context) {
    const uint32_t *ep0 = (const uint32_t *)output_ep0_context;
    return ep0 ? ((ep0[1] >> 16) & 0xffffu) : 0u;
}

static int pi4_xhci_dma_enable_slot_seed(uint32_t trb[4], unsigned slot_type,
                                         unsigned cycle) {
    if (!trb || slot_type > PI4_XHCI_PROTOCOL_SLOT_TYPE_MASK || cycle > 1u ||
        ((uintptr_t)trb & 15u)) return -1;
    trb[0] = trb[1] = trb[2] = 0;
    trb[3] = (PI4_XHCI_TRB_TYPE_ENABLE_SLOT << 10) |
             (slot_type << 16) | cycle;
    return 0;
}

static int pi4_xhci_dma_noop_seed(uint32_t trb[4], unsigned cycle) {
    if (!trb || cycle > 1u || ((uintptr_t)trb & 15u)) return -1;
    trb[0] = trb[1] = trb[2] = 0;
    trb[3] = (PI4_XHCI_TRB_TYPE_NOOP << 10) | cycle;
    return 0;
}

static int pi4_xhci_dma_address_device_seed(uint32_t trb[4],
        uint64_t input_context_bus, unsigned slot_id, unsigned cycle,
        unsigned block_set_address) {
    if (!trb || (input_context_bus & 15u) || !slot_id || slot_id > 255u ||
        cycle > 1u || block_set_address > 1u ||
        ((uintptr_t)trb & 15u)) return -1;
    trb[0] = (uint32_t)input_context_bus;
    trb[1] = (uint32_t)(input_context_bus >> 32);
    trb[2] = 0; /* Reserved. */
    trb[3] = (PI4_XHCI_TRB_TYPE_ADDRESS_DEVICE << 10) |
             (block_set_address << 9) | (slot_id << 24) | cycle;
    return 0;
}

static int pi4_xhci_dma_configure_endpoint_seed(uint32_t trb[4],
        uint64_t input_context_bus, unsigned slot_id, unsigned cycle) {
    if (!trb || (input_context_bus & 15u) || !slot_id || slot_id > 255u ||
        cycle > 1u || ((uintptr_t)trb & 15u)) return -1;
    trb[0] = (uint32_t)input_context_bus;
    trb[1] = (uint32_t)(input_context_bus >> 32);
    trb[2] = 0;
    trb[3] = (PI4_XHCI_TRB_TYPE_CONFIGURE_ENDPOINT << 10) |
             (slot_id << 24) | cycle;
    return 0;
}

static int pi4_xhci_dma_output_hub_context_ok(const void *output_context,
        unsigned context_size, unsigned ports, unsigned tt_think_time,
        unsigned multi_tt) {
    const uint32_t *slot = (const uint32_t *)output_context;
    if (!slot || (context_size != 32u && context_size != 64u) ||
        !ports || ports > 255u || tt_think_time > 3u || multi_tt > 1u)
        return 0;
    return (slot[0] & (1u << 26)) &&
           (((slot[0] >> 25) & 1u) == multi_tt) &&
           ((slot[1] >> 24) == ports) &&
           (((slot[2] >> 16) & 3u) == tt_think_time);
}

static int pi4_xhci_dma_stop_endpoint_seed(uint32_t trb[4],
        unsigned slot_id, unsigned endpoint_id, unsigned cycle) {
    if (!trb || !slot_id || slot_id > 255u || !endpoint_id ||
        endpoint_id > 31u || cycle > 1u || ((uintptr_t)trb & 15u))
        return -1;
    trb[0] = trb[1] = trb[2] = 0;
    trb[3] = (PI4_XHCI_TRB_TYPE_STOP_ENDPOINT << 10) |
             (endpoint_id << 16) | (slot_id << 24) | cycle;
    return 0;
}

static int pi4_xhci_dma_transfer_ring_seed(void *memory, uint64_t ring_bus) {
    volatile uint32_t *ring = (volatile uint32_t *)memory;
    const unsigned link_word = (PI4_XHCI_DMA_RING_BYTES / 16u - 1u) * 4u;
    if (!memory || ((uintptr_t)memory & 15u) || (ring_bus & 63u)) return -1;
    ring[link_word + 0] = (uint32_t)ring_bus;
    ring[link_word + 1] = (uint32_t)(ring_bus >> 32);
    ring[link_word + 2] = 0;
    ring[link_word + 3] = (PI4_XHCI_TRB_TYPE_LINK << 10) | (1u << 1) | 1u;
    return 0;
}

/* Setup data is immediate, so the TRB carries the eight-byte USB setup
 * packet. TRT selects no data, OUT data, or IN data. */
static int pi4_xhci_dma_setup_stage_seed(uint32_t trb[4],
        unsigned request_type, unsigned request, unsigned value,
        unsigned index, unsigned length, unsigned transfer_type,
        unsigned cycle) {
    if (!trb || request_type > 0xffu || request > 0xffu ||
        value > 0xffffu || index > 0xffffu || length > 0xffffu ||
        (transfer_type != 0u && transfer_type != 2u && transfer_type != 3u) ||
        (transfer_type == 0u ? length != 0u : length == 0u) ||
        cycle > 1u || ((uintptr_t)trb & 15u)) return -1;
    trb[0] = (uint32_t)request_type | ((uint32_t)request << 8) |
             ((uint32_t)value << 16);
    trb[1] = (uint32_t)index | ((uint32_t)length << 16);
    trb[2] = 8u; /* Setup packet length. */
    /* Setup Stage bits 4:1 are reserved and must stay zero; IDT is bit 6. */
    trb[3] = (PI4_XHCI_TRB_TYPE_SETUP_STAGE << 10) |
             (transfer_type << 16) | (1u << 6) | cycle;
    return 0;
}

/* Seed a standard IN GET_DESCRIPTOR Setup Stage TRB. */
static int pi4_xhci_dma_get_descriptor_setup_seed(uint32_t trb[4],
        unsigned descriptor_type, unsigned descriptor_index,
        unsigned length, unsigned cycle) {
    if (!descriptor_type || descriptor_type > 0xffu ||
        descriptor_index > 0xffu) return -1;
    return pi4_xhci_dma_setup_stage_seed(trb, 0x80u, 0x06u,
        (descriptor_type << 8) | descriptor_index, 0u, length, 3u, cycle);
}

/* Standard device GET_STATUS, used to check that EP0 still accepts a
 * control-IN request after Evaluate Context changes its maximum packet size. */
static int pi4_xhci_dma_get_status_setup_seed(uint32_t trb[4],
        unsigned cycle) {
    return pi4_xhci_dma_setup_stage_seed(trb, 0x80u, 0x00u,
        0u, 0u, 2u, 3u, cycle);
}

/* Standard device GET_CONFIGURATION, valid before Set Configuration. */
static int pi4_xhci_dma_get_configuration_setup_seed(uint32_t trb[4],
        unsigned cycle) {
    return pi4_xhci_dma_setup_stage_seed(trb, 0x80u, 0x08u,
        0u, 0u, 1u, 3u, cycle);
}

/* USB Mass Storage Bulk-Only Transport GET_MAX_LUN is a read-only class
 * request on EP0. Its interface number is carried in wIndex. */
static int pi4_xhci_dma_bot_get_max_lun_setup_seed(uint32_t trb[4],
        unsigned interface_number, unsigned cycle) {
    return pi4_xhci_dma_setup_stage_seed(trb, 0xa1u, 0xfeu,
        0u, interface_number, 1u, 3u, cycle);
}

/* Standard device SET_CONFIGURATION with the no-data control-transfer form. */
static int pi4_xhci_dma_set_configuration_setup_seed(uint32_t trb[4],
        unsigned configuration_value, unsigned cycle) {
    return pi4_xhci_dma_setup_stage_seed(trb, 0x00u, 0x09u,
        configuration_value, 0u, 0u, 0u, cycle);
}

static int pi4_xhci_dma_data_stage_seed(uint32_t trb[4], uint64_t buffer_bus,
                                        uint32_t length, unsigned cycle) {
    if (!trb || (buffer_bus & 15u) || !length || length > 0x1ffffu ||
        cycle > 1u || ((uintptr_t)trb & 15u)) return -1;
    trb[0] = (uint32_t)buffer_bus;
    trb[1] = (uint32_t)(buffer_bus >> 32);
    trb[2] = length;
    /* One TRB holds the whole Data Stage TD, so its terminal CH bit is zero. */
    trb[3] = (PI4_XHCI_TRB_TYPE_DATA_STAGE << 10) |
             (1u << 16) | cycle; /* Single IN Data Stage TD; CH=0. */
    return 0;
}

static int pi4_xhci_dma_status_stage_seed_direction(uint32_t trb[4],
        unsigned direction_in, unsigned cycle) {
    if (!trb || direction_in > 1u || cycle > 1u ||
        ((uintptr_t)trb & 15u)) return -1;
    trb[0] = trb[1] = trb[2] = 0;
    trb[3] = (PI4_XHCI_TRB_TYPE_STATUS_STAGE << 10) |
             (direction_in << 16) | (1u << 5) | cycle;
    return 0;
}

static int pi4_xhci_dma_status_stage_seed(uint32_t trb[4], unsigned cycle) {
    /* Control-IN data has an OUT status stage. */
    return pi4_xhci_dma_status_stage_seed_direction(trb, 0u, cycle);
}

static unsigned pi4_xhci_dma_transfer_event_check_mask(uint64_t parameter,
        uint32_t status, uint32_t control, uint64_t status_trb_bus,
        unsigned slot_id, unsigned endpoint_id) {
    unsigned checks = 0;
    /* Cycle state is checked against the event-ring consumer cursor before
     * this validator runs. A valid Transfer Event can therefore have Cycle
     * 0 after the event ring wraps. */
    if (((control >> 10) & 0x3fu) == PI4_XHCI_TRB_TYPE_TRANSFER_EVT)
        checks |= PI4_XHCI_DMA_TRANSFER_CHECK_TYPE;
    if (((control >> 16) & 0x1fu) == endpoint_id)
        checks |= PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT;
    if (((control >> 24) & 0xffu) == slot_id)
        checks |= PI4_XHCI_DMA_TRANSFER_CHECK_SLOT;
    if ((parameter & ~0xfull) == (status_trb_bus & ~0xfull))
        checks |= PI4_XHCI_DMA_TRANSFER_CHECK_POINTER;
    if ((status >> 24) == 1u && (status & 0x00ffffffu) == 0)
        checks |= PI4_XHCI_DMA_TRANSFER_CHECK_SUCCESS;
    return checks;
}

static int pi4_xhci_dma_transfer_event_ok(uint64_t parameter,
        uint32_t status, uint32_t control, uint64_t status_trb_bus,
        unsigned slot_id, unsigned endpoint_id) {
    return pi4_xhci_dma_transfer_event_check_mask(parameter, status, control,
        status_trb_bus, slot_id, endpoint_id) ==
        PI4_XHCI_DMA_TRANSFER_CHECK_ALL;
}

/* One Normal TRB per BOT phase, no streams or chaining. IOC reports completion;
 * ISP additionally reports a short IN packet. No TRB crosses a 64 KiB boundary. */
static int pi4_xhci_dma_bulk_normal_seed(uint32_t trb[4], uint64_t buffer_bus,
        unsigned length, unsigned direction_in, unsigned cycle) {
    if (!trb || ((uintptr_t)trb & 15u) || !length || length > 65536u ||
        (buffer_bus & 0xffffu) + length > 65536u ||
        buffer_bus > UINT64_MAX - length || direction_in > 1u || cycle > 1u)
        return -1;
    trb[0] = (uint32_t)buffer_bus;
    trb[1] = (uint32_t)(buffer_bus >> 32);
    trb[2] = length; /* TD Size=0: this is the last (and only) TRB. */
    trb[3] = (PI4_XHCI_TRB_TYPE_NORMAL << 10) | (1u << 5) |
             (direction_in << 2) | cycle;
    return 0;
}

static int pi4_xhci_dma_bulk_event_ok(uint64_t parameter, uint32_t status,
        uint32_t control, uint64_t trb_bus, unsigned slot, unsigned dci,
        unsigned requested, unsigned allow_short, unsigned *actual) {
    unsigned checks = pi4_xhci_dma_transfer_event_check_mask(parameter,
        status, control, trb_bus, slot, dci);
    unsigned cc = status >> 24, residual = status & 0x00ffffffu;
    if (!actual || !requested || allow_short > 1u || !slot || slot > 255u ||
        dci < 2u || dci > 31u || (control & (1u << 2)) || /* no Event Data */
        (checks & 0x0fu) != 0x0fu || residual > requested ||
        !((cc == 1u && !residual) || (allow_short && cc == 13u)))
        return 0;
    *actual = requested - residual;
    return 1;
}

/* USB MSC BOT 1.0 wrapper fields are LE; SCSI CDB fields are independently BE.
 * This helper accepts only the read-only commands used by the bounded probe. */
static int pi4_xhci_dma_bot_read_cbw_seed(uint8_t *cbw, unsigned length,
        uint32_t tag, const uint8_t *cdb, unsigned cdb_length, unsigned bytes) {
    if (!cbw || length < PI4_XHCI_DMA_BOT_CBW_BYTES || !cdb ||
        (cdb_length != 6u && cdb_length != 10u) || !bytes ||
        bytes > PI4_XHCI_DMA_BOT_MAX_BLOCK_BYTES ||
        (cdb[0] != 0x12u && cdb[0] != 0x25u &&
         cdb[0] != 0x28u && cdb[0] != 0x03u))
        return -1;
    for (unsigned i = 0; i < PI4_XHCI_DMA_BOT_CBW_BYTES; i++) cbw[i] = 0;
    cbw[0] = 0x55u; cbw[1] = 0x53u; cbw[2] = 0x42u; cbw[3] = 0x43u;
    for (unsigned i = 0; i < 4u; i++) cbw[4u + i] = (uint8_t)(tag >> (i * 8u));
    for (unsigned i = 0; i < 4u; i++) cbw[8u + i] = (uint8_t)(bytes >> (i * 8u));
    cbw[12] = 0x80u;
    cbw[14] = cdb_length;
    for (unsigned i = 0; i < cdb_length; i++) cbw[15u + i] = cdb[i];
    return 0;
}

static int pi4_xhci_dma_bot_inquiry_cbw_seed(uint8_t *cbw, unsigned length,
        uint32_t tag) {
    const uint8_t cdb[6] = {0x12u, 0u, 0u, 0u, PI4_XHCI_DMA_BOT_INQUIRY_BYTES, 0u};
    return pi4_xhci_dma_bot_read_cbw_seed(cbw, length, tag, cdb, 6u,
                                          PI4_XHCI_DMA_BOT_INQUIRY_BYTES);
}

/* A command failure is valid BOT status and may require REQUEST SENSE.
 * Phase Error/reserved status, corrupt framing, and mismatched residue stop. */
static int pi4_xhci_dma_bot_csw_valid(const uint8_t *csw, unsigned length,
        uint32_t tag, unsigned requested, unsigned actual, unsigned *scsi_status) {
    uint32_t returned_tag = 0, residue = 0;
    if (!csw || !scsi_status || length != PI4_XHCI_DMA_BOT_CSW_BYTES || actual > requested ||
        csw[0] != 0x55u || csw[1] != 0x53u || csw[2] != 0x42u ||
        csw[3] != 0x53u || csw[12] > 1u)
        return 0;
    for (unsigned i = 0; i < 4u; i++) {
        returned_tag |= (uint32_t)csw[4u + i] << (i * 8u);
        residue |= (uint32_t)csw[8u + i] << (i * 8u);
    }
    if (returned_tag != tag || residue != requested - actual) return 0;
    *scsi_status = csw[12];
    return 1;
}

static int pi4_xhci_dma_bot_csw_ok(const uint8_t *csw, unsigned length,
        uint32_t tag, unsigned requested, unsigned actual) {
    unsigned status = UINT32_MAX;
    return pi4_xhci_dma_bot_csw_valid(csw, length, tag, requested, actual,
                                      &status) && status == 0u;
}

static uint32_t pi4_xhci_dma_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static int pi4_xhci_dma_bot_capacity10_cbw_seed(uint8_t *cbw, unsigned length,
        uint32_t tag) {
    const uint8_t cdb[10] = {0x25u}; /* PMI=0, LBA=0: full medium capacity. */
    return pi4_xhci_dma_bot_read_cbw_seed(cbw, length, tag, cdb, 10u, 8u);
}

static int pi4_xhci_dma_bot_block_size_ok(unsigned bytes) {
    return bytes == 512u || bytes == 1024u || bytes == 2048u;
}

static int pi4_xhci_dma_bot_capacity10_parse(const uint8_t *data, unsigned length,
        uint32_t *last_lba, uint32_t *block_bytes, uint64_t *capacity_bytes) {
    if (!data || length != 8u || !last_lba || !block_bytes || !capacity_bytes)
        return 0;
    *last_lba = pi4_xhci_dma_be32(data);
    *block_bytes = pi4_xhci_dma_be32(data + 4u);
    *capacity_bytes = 0;
    /* 0xffffffff signals that READ CAPACITY(16) is needed; do not guess.
     * This probe bounds one logical block to the reserved DMA buffer. */
    if (*last_lba == UINT32_MAX || !pi4_xhci_dma_bot_block_size_ok(*block_bytes))
        return 0;
    *capacity_bytes = ((uint64_t)*last_lba + 1u) * *block_bytes;
    return 1;
}

static int pi4_xhci_dma_bot_read10_cbw_seed(uint8_t *cbw, unsigned length,
        uint32_t tag, uint32_t lba, uint32_t last_lba, unsigned block_bytes) {
    uint8_t cdb[10] = {0x28u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u, 0u};
    if (last_lba == UINT32_MAX || lba > last_lba ||
        !pi4_xhci_dma_bot_block_size_ok(block_bytes)) return -1;
    for (unsigned i = 0; i < 4u; i++) cdb[2u + i] = (uint8_t)(lba >> (24u - 8u * i));
    return pi4_xhci_dma_bot_read_cbw_seed(cbw, length, tag, cdb, 10u, block_bytes);
}

static int pi4_xhci_dma_bot_sense_cbw_seed(uint8_t *cbw, unsigned length,
        uint32_t tag) {
    const uint8_t cdb[6] = {0x03u, 0u, 0u, 0u, 18u, 0u};
    return pi4_xhci_dma_bot_read_cbw_seed(cbw, length, tag, cdb, 6u, 18u);
}

static int pi4_xhci_dma_bot_sense_parse(const uint8_t *data, unsigned length,
        unsigned *key, unsigned *asc, unsigned *ascq) {
    unsigned response;
    if (!data || !key || !asc || !ascq || length < 8u) return 0;
    response = data[0] & 0x7fu;
    if (response == 0x70u || response == 0x71u) {
        if (length < 14u || data[7] < 6u) return 0;
        *key = data[2] & 15u; *asc = data[12]; *ascq = data[13];
    } else if (response == 0x72u || response == 0x73u) {
        *key = data[1] & 15u; *asc = data[2]; *ascq = data[3];
    } else return 0;
    return 1;
}

static int pi4_xhci_dma_bot_sense_retryable(unsigned key, unsigned asc, unsigned ascq) {
    return key == 6u || (key == 2u && asc == 4u && ascq == 1u);
}

static uint32_t pi4_xhci_dma_crc32(const uint8_t *data, unsigned length) {
    uint32_t crc = UINT32_MAX;
    for (unsigned i = 0; i < length; i++) {
        crc ^= data[i];
        for (unsigned j = 0; j < 8u; j++)
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
    }
    return ~crc;
}

static int pi4_xhci_dma_bot_inquiry_ok(const uint8_t *data, unsigned length) {
    /* This milestone requires the standard direct-access device identity. */
    return data && length >= PI4_XHCI_DMA_BOT_INQUIRY_BYTES &&
           data[0] == 0u && (data[3] & 0x0fu) >= 2u && data[4] >= 31u;
}

static void pi4_xhci_dma_bot_identity_text(char *out, const uint8_t *data,
        unsigned length) {
    for (unsigned i = 0; i < length; i++)
        out[i] = data[i] >= 0x20u && data[i] <= 0x7eu ? (char)data[i] : '.';
    out[length] = 0;
}

static int pi4_xhci_dma_device_descriptor_prefix_ok(const uint8_t *descriptor,
                                                    uint32_t length) {
    return descriptor && length >= 8u && descriptor[0] == 18u &&
           descriptor[1] == 1u && descriptor[7] == 64u;
}

/* The first child Device Descriptor request runs at the speed-specific EP0
 * default packet size. Accept only USB-defined bMaxPacketSize0 values for the
 * reported xHCI speed; the later full descriptor may refine Full-Speed EP0. */
static int pi4_xhci_dma_child_device_descriptor_prefix_ok(
        const uint8_t *descriptor, uint32_t length, unsigned speed_id) {
    unsigned mps;
    if (!descriptor || length < 8u || descriptor[0] != 18u ||
        descriptor[1] != 1u)
        return 0;
    mps = descriptor[7];
    switch (speed_id) {
    case 1u: return mps == 8u || mps == 16u || mps == 32u || mps == 64u;
    case 2u: return mps == 8u;
    case 3u: return mps == 64u;
    case 4u: return mps == 9u;
    default: return 0;
    }
}

static int pi4_xhci_dma_child_device_descriptor_ok(const uint8_t *descriptor,
        uint32_t length, unsigned speed_id) {
    return length >= 18u &&
           pi4_xhci_dma_child_device_descriptor_prefix_ok(descriptor,
               length, speed_id) && descriptor[17] != 0u;
}

/* Full-Speed EP0 may need its default 8-byte MPS refined after the first
 * descriptor packet. Other speeds have fixed EP0 sizes. Return 1 to evaluate
 * a changed Full-Speed context, 0 to preserve it, and -1 for a mismatch. */
static int pi4_xhci_dma_ep0_mps_update_plan(unsigned speed_id,
        unsigned descriptor_mps, unsigned current_mps, unsigned *requested) {
    unsigned desired = descriptor_mps;
    if (!requested) return -1;
    if (speed_id == 1u) {
        if ((desired != 8u && desired != 16u && desired != 32u && desired != 64u) ||
            (current_mps != 8u && current_mps != 16u &&
             current_mps != 32u && current_mps != 64u)) return -1;
    } else {
        if (speed_id == 4u && descriptor_mps == 9u) desired = 512u;
        if ((speed_id != 2u && speed_id != 3u && speed_id != 4u) ||
            desired != pi4_xhci_dma_ep0_default_mps(speed_id) ||
            current_mps != desired) return -1;
    }
    *requested = desired;
    return desired != current_mps;
}

static int __attribute__((unused)) pi4_xhci_dma_device_descriptor_ok(const uint8_t *descriptor,
                                             uint32_t length) {
    return descriptor && length >= 18u &&
           pi4_xhci_dma_device_descriptor_prefix_ok(descriptor, length) &&
           descriptor[17] != 0u;
}

static int pi4_xhci_dma_configuration_header_ok(const uint8_t *descriptor,
                                                uint32_t length) {
    unsigned total;
    if (!descriptor || length < 9u || descriptor[0] != 9u ||
        descriptor[1] != 2u || descriptor[4] == 0u || descriptor[5] == 0u ||
        !(descriptor[7] & 0x80u))
        return 0;
    total = (unsigned)descriptor[2] | ((unsigned)descriptor[3] << 8);
    return total >= 9u && total >= length;
}

struct pi4_xhci_dma_config_summary {
    uint32_t total_length;
    uint32_t configuration_value;
    uint32_t interface_descriptors;
    uint32_t unique_interfaces;
    uint32_t endpoint_descriptors;
    uint32_t first_interface_number;
    uint32_t first_interface_class;
    uint32_t first_interface_subclass;
    uint32_t first_interface_protocol;
    uint32_t first_interface_endpoint_count;
    uint32_t first_endpoint_address;
    uint32_t first_endpoint_attributes;
    uint32_t first_endpoint_max_packet;
    uint32_t first_endpoint_interval;
    uint32_t bot_interface_found;
    uint32_t bot_interface_number;
    uint32_t bot_interface_alt;
    uint32_t bot_interface_endpoint_count;
    uint32_t bot_bulk_in_address;
    uint32_t bot_bulk_in_attributes;
    uint32_t bot_bulk_in_max_packet;
    uint32_t bot_bulk_in_interval;
    uint32_t bot_bulk_in_max_burst;
    uint32_t bot_bulk_in_streams;
    uint32_t bot_bulk_out_address;
    uint32_t bot_bulk_out_attributes;
    uint32_t bot_bulk_out_max_packet;
    uint32_t bot_bulk_out_interval;
    uint32_t bot_bulk_out_max_burst;
    uint32_t bot_bulk_out_streams;
};

/* Validate the bounded descriptor chain returned for one configuration.
 * Walk every descriptor without assuming class-specific descriptor formats;
 * verify interface and endpoint descriptor sizes and each interface
 * alternate-setting's declared endpoint count. */
static int pi4_xhci_dma_configuration_descriptor_ok(const uint8_t *descriptor,
        uint32_t length, uint32_t max_length,
        struct pi4_xhci_dma_config_summary *summary) {
    uint8_t seen_interfaces[32] = {0};
    uint32_t total, interface_descriptors = 0, unique_interfaces = 0;
    uint32_t endpoint_descriptors = 0, current_endpoints = 0;
    uint32_t expected_endpoints = 0, have_interface = 0;
    uint32_t first_interface_number = UINT32_MAX;
    uint32_t first_interface_class = UINT32_MAX;
    uint32_t first_interface_subclass = UINT32_MAX;
    uint32_t first_interface_protocol = UINT32_MAX;
    uint32_t first_interface_endpoint_count = UINT32_MAX;
    uint32_t first_endpoint_address = UINT32_MAX;
    uint32_t first_endpoint_attributes = UINT32_MAX;
    uint32_t first_endpoint_max_packet = UINT32_MAX;
    uint32_t first_endpoint_interval = UINT32_MAX;
    uint32_t current_bot_interface = 0, bot_interface_found = 0;
    uint32_t bot_interface_number = UINT32_MAX, bot_interface_alt = UINT32_MAX;
    uint32_t bot_interface_endpoint_count = UINT32_MAX;
    uint32_t bot_bulk_in_address = UINT32_MAX;
    uint32_t bot_bulk_in_attributes = UINT32_MAX;
    uint32_t bot_bulk_in_max_packet = UINT32_MAX;
    uint32_t bot_bulk_in_interval = UINT32_MAX;
    uint32_t bot_bulk_out_address = UINT32_MAX;
    uint32_t bot_bulk_out_attributes = UINT32_MAX;
    uint32_t bot_bulk_out_max_packet = UINT32_MAX;
    uint32_t bot_bulk_out_interval = UINT32_MAX;
    uint32_t bot_bulk_in_max_burst = UINT32_MAX, bot_bulk_in_streams = UINT32_MAX;
    uint32_t bot_bulk_out_max_burst = UINT32_MAX, bot_bulk_out_streams = UINT32_MAX;
    unsigned companion_endpoint = 0;

    if (!descriptor || !summary || length < 9u || length > max_length ||
        !pi4_xhci_dma_configuration_header_ok(descriptor, 9u))
        return 0;
    total = (uint32_t)descriptor[2] | ((uint32_t)descriptor[3] << 8);
    if (total != length) return 0;

    for (uint32_t offset = 9u; offset < total;) {
        uint32_t descriptor_length;
        uint8_t type;
        if (total - offset < 2u) return 0;
        descriptor_length = descriptor[offset];
        type = descriptor[offset + 1u];
        if (descriptor_length < 2u || descriptor_length > total - offset)
            return 0;
        if (type != 48u) companion_endpoint = 0;

        if (type == 4u) { /* Interface Descriptor */
            uint32_t number;
            if (descriptor_length < 9u ||
                (have_interface && current_endpoints != expected_endpoints))
                return 0;
            number = descriptor[offset + 2u];
            if (!(seen_interfaces[number >> 3] & (1u << (number & 7u)))) {
                seen_interfaces[number >> 3] |=
                    (uint8_t)(1u << (number & 7u));
                unique_interfaces++;
            }
            if (!interface_descriptors) {
                first_interface_number = number;
                first_interface_endpoint_count = descriptor[offset + 4u];
                first_interface_class = descriptor[offset + 5u];
                first_interface_subclass = descriptor[offset + 6u];
                first_interface_protocol = descriptor[offset + 7u];
            }
            interface_descriptors++;
            expected_endpoints = descriptor[offset + 4u];
            current_endpoints = 0;
            have_interface = 1u;
            current_bot_interface =
                descriptor[offset + 3u] == 0u &&
                descriptor[offset + 5u] == 8u &&
                descriptor[offset + 6u] == 6u &&
                descriptor[offset + 7u] == 0x50u;
            if (current_bot_interface) {
                if (bot_interface_found) return 0;
                bot_interface_found = 1u;
                bot_interface_number = number;
                bot_interface_alt = descriptor[offset + 3u];
                bot_interface_endpoint_count = descriptor[offset + 4u];
            }
        } else if (type == 5u) { /* Endpoint Descriptor */
            uint32_t max_packet;
            uint32_t address, attributes;
            if (descriptor_length < 7u || !have_interface ||
                current_endpoints >= expected_endpoints ||
                (descriptor[offset + 2u] & 0x0fu) == 0u ||
                (descriptor[offset + 2u] & 0x70u) != 0u)
                return 0;
            address = descriptor[offset + 2u];
            attributes = descriptor[offset + 3u];
            max_packet = (uint32_t)descriptor[offset + 4u] |
                         ((uint32_t)descriptor[offset + 5u] << 8);
            if (current_bot_interface && (attributes & 3u) == 2u &&
                (max_packet & 0xf800u) != 0u)
                return 0;
            if (!endpoint_descriptors) {
                first_endpoint_address = descriptor[offset + 2u];
                first_endpoint_attributes = descriptor[offset + 3u];
                first_endpoint_max_packet = max_packet;
                first_endpoint_interval = descriptor[offset + 6u];
            }
            if (current_bot_interface && (attributes & 3u) == 2u) {
                if (address & 0x80u) {
                    if (bot_bulk_in_address != UINT32_MAX) return 0;
                    bot_bulk_in_address = address;
                    bot_bulk_in_attributes = attributes;
                    bot_bulk_in_max_packet = max_packet & 0x07ffu;
                    bot_bulk_in_interval = descriptor[offset + 6u];
                    companion_endpoint = 1u;
                } else {
                    if (bot_bulk_out_address != UINT32_MAX) return 0;
                    bot_bulk_out_address = address;
                    bot_bulk_out_attributes = attributes;
                    bot_bulk_out_max_packet = max_packet & 0x07ffu;
                    bot_bulk_out_interval = descriptor[offset + 6u];
                    companion_endpoint = 2u;
                }
            }
            endpoint_descriptors++;
            current_endpoints++;
        } else if (type == 48u && current_bot_interface) {
            /* USB 3 bulk companion must immediately follow its endpoint.
             * MaxStreams is a capability; this probe requires ordinary BOT. */
            if (descriptor_length != 6u || !companion_endpoint ||
                descriptor[offset + 2u] > 15u ||
                descriptor[offset + 3u] > 16u ||
                descriptor[offset + 4u] || descriptor[offset + 5u])
                return 0;
            if (companion_endpoint == 1u) {
                bot_bulk_in_max_burst = descriptor[offset + 2u];
                bot_bulk_in_streams = descriptor[offset + 3u];
            } else {
                bot_bulk_out_max_burst = descriptor[offset + 2u];
                bot_bulk_out_streams = descriptor[offset + 3u];
            }
            companion_endpoint = 0;
        }
        offset += descriptor_length;
    }
    if ((have_interface && current_endpoints != expected_endpoints) ||
        unique_interfaces != descriptor[4] || !interface_descriptors)
        return 0;

    summary->total_length = total;
    summary->configuration_value = descriptor[5];
    summary->interface_descriptors = interface_descriptors;
    summary->unique_interfaces = unique_interfaces;
    summary->endpoint_descriptors = endpoint_descriptors;
    summary->first_interface_number = first_interface_number;
    summary->first_interface_class = first_interface_class;
    summary->first_interface_subclass = first_interface_subclass;
    summary->first_interface_protocol = first_interface_protocol;
    summary->first_interface_endpoint_count = first_interface_endpoint_count;
    summary->first_endpoint_address = first_endpoint_address;
    summary->first_endpoint_attributes = first_endpoint_attributes;
    summary->first_endpoint_max_packet = first_endpoint_max_packet;
    summary->first_endpoint_interval = first_endpoint_interval;
    summary->bot_interface_found = bot_interface_found;
    summary->bot_interface_number = bot_interface_number;
    summary->bot_interface_alt = bot_interface_alt;
    summary->bot_interface_endpoint_count = bot_interface_endpoint_count;
    summary->bot_bulk_in_address = bot_bulk_in_address;
    summary->bot_bulk_in_attributes = bot_bulk_in_attributes;
    summary->bot_bulk_in_max_packet = bot_bulk_in_max_packet;
    summary->bot_bulk_in_interval = bot_bulk_in_interval;
    summary->bot_bulk_in_max_burst = bot_bulk_in_max_burst;
    summary->bot_bulk_in_streams = bot_bulk_in_streams;
    summary->bot_bulk_out_address = bot_bulk_out_address;
    summary->bot_bulk_out_attributes = bot_bulk_out_attributes;
    summary->bot_bulk_out_max_packet = bot_bulk_out_max_packet;
    summary->bot_bulk_out_interval = bot_bulk_out_interval;
    summary->bot_bulk_out_max_burst = bot_bulk_out_max_burst;
    summary->bot_bulk_out_streams = bot_bulk_out_streams;
    return 1;
}

static int pi4_xhci_dma_hub_descriptor_ok(const uint8_t *descriptor,
                                          uint32_t length) {
    unsigned ports, bitmap_bytes, required;
    if (!descriptor || length < 7u || descriptor[1] != 0x29u)
        return 0;
    ports = descriptor[2];
    if (!ports || ports > 31u) return 0;
    bitmap_bytes = (ports + 8u) / 8u;
    required = 7u + 2u * bitmap_bytes;
    return required <= length && descriptor[0] >= required &&
           descriptor[0] <= length;
}

static unsigned pi4_xhci_dma_hub_power_request_count(
    uint32_t hub_characteristics, unsigned ports) {
    if (!ports || ports > 31u) return 0;
    switch (hub_characteristics & 3u) {
    case 0u: return 1u;       /* Ganged: one request powers the gang. */
    case 1u: return ports;    /* Individual: one request per port. */
    default: return 0u;       /* No switching or reserved mode. */
    }
}

/* For the bring-up diagnostic, issue PORT_POWER on every switched port even
 * when the descriptor claims ganged switching; this compares reported mode
 * with observed per-port status behavior. */
static unsigned pi4_xhci_dma_hub_power_probe_request_count(
        uint32_t hub_characteristics, unsigned ports) {
    if (!ports || ports > 31u) return 0;
    switch (hub_characteristics & 3u) {
    case 0u: /* Ganged per descriptor; repeated requests are diagnostic. */
    case 1u: return ports;
    default: return 0u;
    }
}

/* Keep each EP0 control TD directly after the previous TD. The endpoint
 * dequeue pointer advances past the Status TRB; leaving zero-cycle gaps makes
 * the next request unreachable to the controller. */
static unsigned pi4_xhci_dma_hub_port_status_trb_index(
        unsigned power_request_count, unsigned port_index) {
    if (!power_request_count || power_request_count > 4u || port_index >= 4u)
        return UINT32_MAX;
    return 11u + 2u * power_request_count + 3u * port_index;
}

static int __attribute__((unused)) pi4_xhci_dma_hub_port_status_powered(
        const uint8_t *status, uint32_t length) {
    uint16_t port_status;
    if (!status || length < 4u) return 0;
    port_status = status[0] | ((uint16_t)status[1] << 8);
    return (port_status & PI4_XHCI_HUB_PORT_STATUS_POWER) != 0;
}

static uint32_t pi4_xhci_dma_hub_port_unpowered_mask(
        const uint32_t port_status[4], uint32_t valid_mask) {
    uint32_t unpowered = 0;
    if (!port_status) return UINT32_MAX;
    for (unsigned i = 0; i < 4u; i++) {
        if ((valid_mask & (1u << i)) &&
            !(port_status[i] & PI4_XHCI_HUB_PORT_STATUS_POWER))
            unpowered |= 1u << i;
    }
    return unpowered;
}

static uint32_t pi4_xhci_dma_hub_port_powered_mask(
        const uint32_t port_status[4], uint32_t valid_mask) {
    uint32_t powered = 0;
    if (!port_status) return UINT32_MAX;
    for (unsigned i = 0; i < 4u; i++) {
        if ((valid_mask & (1u << i)) &&
            (port_status[i] & PI4_XHCI_HUB_PORT_STATUS_POWER))
            powered |= 1u << i;
    }
    return powered;
}

static uint32_t pi4_xhci_dma_hub_port_connected_mask(
        const uint32_t port_status[4], uint32_t valid_mask) {
    uint32_t connected = 0;
    if (!port_status) return UINT32_MAX;
    for (unsigned i = 0; i < 4u; i++) {
        if ((valid_mask & (1u << i)) &&
            (port_status[i] & PI4_XHCI_HUB_PORT_STATUS_CONNECTION))
            connected |= 1u << i;
    }
    return connected;
}

/* Return the lowest connected 1-based port number, or zero when none is
 * connected. The caller supplies the descriptor-bounded port count. */
static unsigned pi4_xhci_dma_hub_first_connected_port(uint32_t connected_mask,
                                                       unsigned ports) {
    if (!ports || ports > 31u) return 0;
    for (unsigned i = 0; i < ports; i++)
        if (connected_mask & (1u << i)) return i + 1u;
    return 0;
}

/* Only reset a child after its port status was actually received and reports
 * both connection and power. This keeps incomplete scans and unpowered ports
 * out of the downstream reset path. */
static int pi4_xhci_dma_hub_child_port_reset_allowed(
        const uint32_t port_status[4], uint32_t valid_mask,
        unsigned port_number) {
    unsigned i;
    uint32_t required = PI4_XHCI_HUB_PORT_STATUS_CONNECTION |
                       PI4_XHCI_HUB_PORT_STATUS_POWER;
    if (!port_status || !port_number || port_number > 4u) return 0;
    i = port_number - 1u;
    return (valid_mask & (1u << i)) != 0 &&
           (port_status[i] & required) == required;
}

/* A hub reports reset completion by clearing PORT_RESET and setting
 * C_PORT_RESET in the corresponding status/change words. */
static int pi4_xhci_dma_hub_port_reset_complete(uint32_t status,
                                                uint32_t change) {
    return !(status & PI4_XHCI_HUB_PORT_STATUS_RESET) &&
           (change & PI4_XHCI_HUB_PORT_CHANGE_RESET) != 0;
}

static int pi4_xhci_dma_hub_port_reset_change_cleared(uint32_t status,
                                                      uint32_t change) {
    return (status & (PI4_XHCI_HUB_PORT_STATUS_CONNECTION |
                      PI4_XHCI_HUB_PORT_STATUS_ENABLE |
                      PI4_XHCI_HUB_PORT_STATUS_POWER)) ==
                     (PI4_XHCI_HUB_PORT_STATUS_CONNECTION |
                      PI4_XHCI_HUB_PORT_STATUS_ENABLE |
                      PI4_XHCI_HUB_PORT_STATUS_POWER) &&
           !(status & PI4_XHCI_HUB_PORT_STATUS_RESET) &&
           !(change & PI4_XHCI_HUB_PORT_CHANGE_RESET);
}

static unsigned pi4_xhci_dma_output_usb_address(const void *output_context) {
    const uint32_t *slot = (const uint32_t *)output_context;
    return slot ? slot[3] & 0xffu : 0u;
}

static unsigned pi4_xhci_dma_output_slot_state(const void *output_context) {
    const uint32_t *slot = (const uint32_t *)output_context;
    return slot ? (slot[3] >> 27) & 0x1fu : 0u;
}

/* No-op completions must have Slot ID zero. */
static int __attribute__((unused)) pi4_xhci_dma_noop_event_ok(
        uint64_t parameter, uint32_t status, uint32_t control,
        uint64_t command_bus) {
    return pi4_xhci_dma_command_event_ok(parameter, status, control,
                                         command_bus) &&
           ((control >> 24) & 0xffu) == 0;
}

/* Validate the independent primary-interrupter MFINDEX wrap DMA event. */
static int pi4_xhci_dma_mfindex_event_ok(uint32_t status, uint32_t control) {
    return ((control >> 10) & 0x3fu) == PI4_XHCI_TRB_TYPE_MFINDEX_EVT &&
           (status >> 24) == 1u;
}

/* A sampled 14-bit index wrapped when the later value is numerically smaller. */
static int pi4_xhci_dma_mfindex_wrapped(uint32_t start, uint32_t end) {
    return (end & PI4_XHCI_MFINDEX_MASK) <
           (start & PI4_XHCI_MFINDEX_MASK);
}

/* PORTSC change bits are RW1C; preserve power/wake enables and clear only
 * asserted flags so this diagnostic does not alter unrelated port settings. */
static uint32_t pi4_xhci_dma_portsc_clear_changes(uint32_t portsc) {
    return (portsc & (PI4_XHCI_PORTSC_PP | PI4_XHCI_PORTSC_WAKE_MASK)) |
           (portsc & PI4_XHCI_PORTSC_CHANGE_MASK);
}

/* Request a reset only for a powered, connected port that is not resetting. */
static int pi4_xhci_dma_port_reset_value(uint32_t portsc, uint32_t *value) {
    if (!value || !(portsc & PI4_XHCI_PORTSC_CCS) ||
        !(portsc & PI4_XHCI_PORTSC_PP) ||
        (portsc & (PI4_XHCI_PORTSC_PR | PI4_XHCI_PORTSC_WPR))) return -1;
    *value = portsc & (PI4_XHCI_PORTSC_PP | PI4_XHCI_PORTSC_WAKE_MASK);
    *value |= pi4_xhci_dma_port_reset_change(portsc) == PI4_XHCI_PORTSC_WRC
        ? PI4_XHCI_PORTSC_WPR : PI4_XHCI_PORTSC_PR;
    return 0;
}

/* Validate a Port Status Change event generated by a chosen root port reset. */
static int pi4_xhci_dma_port_event_ok(uint64_t parameter, uint32_t status,
                                     uint32_t control, uint32_t port_id) {
    return port_id && ((uint32_t)(parameter >> 24) & 0xffu) == port_id &&
           ((control >> 10) & 0x3fu) == PI4_XHCI_TRB_TYPE_PSC_EVT &&
           (status >> 24) == 1u;
}

static unsigned pi4_xhci_dma_port_speed_id(uint32_t portsc) {
    if (portsc == UINT32_MAX) return UINT32_MAX;
    return (portsc & PI4_XHCI_PORTSC_SPEED_MASK) >>
           PI4_XHCI_PORTSC_SPEED_SHIFT;
}

/* After reset, require a connected, powered, enabled port in U0 with a
 * defined speed ID before any device context can be built. */
static int pi4_xhci_dma_port_ready(uint32_t portsc) {
    unsigned speed = pi4_xhci_dma_port_speed_id(portsc);
    return (portsc & (PI4_XHCI_PORTSC_CCS | PI4_XHCI_PORTSC_PED |
                      PI4_XHCI_PORTSC_PP)) ==
                      (PI4_XHCI_PORTSC_CCS | PI4_XHCI_PORTSC_PED |
                       PI4_XHCI_PORTSC_PP) &&
           !(portsc & PI4_XHCI_PORTSC_PR) &&
           (portsc & PI4_XHCI_PORTSC_PLS_MASK) == 0 &&
           speed >= 1u && speed <= 4u;
}

/* Compare architecturally implemented setup fields, ignoring reserved bits.
 * IMOD's low IMODI field may read back as either 0 (no throttling) or 1
 * (the VL805's observed minimum interval); ignore the upper IMODC counter. */
static uint32_t pi4_xhci_dma_setup_mismatch_mask(
    uint32_t mask, uint32_t iman, uint32_t imod,
    uint32_t config, uint64_t dcbaa,
    uint32_t erstsz, uint64_t erstba, uint64_t erdp,
    uint32_t want_config, uint64_t want_dcbaa,
    uint32_t want_erstsz, uint64_t want_erstba, uint64_t want_erdp) {
    uint32_t mismatch = 0;
    if (mask != PI4_XHCI_DMA_SETUP_ALL)
        mismatch |= PI4_XHCI_DMA_SETUP_BAD_MASK;
    if ((iman & 3u) != PI4_XHCI_IMAN_IE)
        mismatch |= PI4_XHCI_DMA_SETUP_BAD_IMAN;
    if ((imod & PI4_XHCI_IMODI_MASK) != 0 &&
        (imod & PI4_XHCI_IMODI_MASK) != PI4_XHCI_IMODI_MIN)
        mismatch |= PI4_XHCI_DMA_SETUP_BAD_IMOD;
    if ((config & 0xffu) != (want_config & 0xffu))
        mismatch |= PI4_XHCI_DMA_SETUP_BAD_CONFIG;
    if ((dcbaa & ~0x3full) != (want_dcbaa & ~0x3full))
        mismatch |= PI4_XHCI_DMA_SETUP_BAD_DCBAAP;
    if ((erstsz & 0xffffu) != (want_erstsz & 0xffffu))
        mismatch |= PI4_XHCI_DMA_SETUP_BAD_ERSTSZ;
    if ((erstba & ~0x3full) != (want_erstba & ~0x3full))
        mismatch |= PI4_XHCI_DMA_SETUP_BAD_ERSTBA;
    if ((erdp & ~0xfull) != (want_erdp & ~0xfull))
        mismatch |= PI4_XHCI_DMA_SETUP_BAD_ERDP;
    return mismatch;
}

static int pi4_xhci_dma_setup_readback_ok(
    uint32_t mask, uint32_t iman, uint32_t imod,
    uint32_t config, uint64_t dcbaa,
    uint32_t erstsz, uint64_t erstba, uint64_t erdp,
    uint32_t want_config, uint64_t want_dcbaa,
    uint32_t want_erstsz, uint64_t want_erstba, uint64_t want_erdp) {
    return pi4_xhci_dma_setup_mismatch_mask(mask, iman, imod,
            config, dcbaa, erstsz, erstba, erdp,
            want_config, want_dcbaa, want_erstsz, want_erstba,
            want_erdp) == 0;
}

/* Preserve PCI command bits and enable only memory decoding plus BME. */
static int pi4_xhci_dma_enable_command(uint16_t original, uint16_t *enabled) {
    if (!enabled || (original & PI4_PCI_COMMAND_MASTER)) return -1;
    *enabled = original | PI4_PCI_COMMAND_MEMORY | PI4_PCI_COMMAND_MASTER;
    return 0;
}
