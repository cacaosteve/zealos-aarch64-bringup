#include <stdint.h>
#include <stdio.h>
#include "../src/pi4_dma_probe.h"
#include "../src/pi4_xhci_dma_probe.h"
#include "../src/pi4_xhci_dma_alias.h"

static int fail(const char *what) {
    fprintf(stderr, "FAIL Pi4 xHCI DMA: %s\n", what);
    return 1;
}

/* Adversarial xHC model: consume at every cache publication, including the
 * interval between payload and ownership publication. It must never run ahead
 * into old slots, including the bootstrap command ring's skipped prefix. */
static struct {
    uint32_t *ring;
    unsigned index, cycle, consumed, issued, fault;
    struct pi4_xhci_dma_event_cursor events;
} live_model;

static void live_model_sync(uint8_t *bytes, uint64_t length) {
    (void)bytes; (void)length;
    for (unsigned steps = 0; steps < 257u; steps++) {
        uint32_t *t = live_model.ring + live_model.index * 4u;
        if ((t[3] & 1u) != live_model.cycle || live_model.fault) return;
        unsigned type = (t[3] >> 10) & 63u;
        if (type == PI4_XHCI_TRB_TYPE_LINK) {
            if (live_model.index != 255u || t[0] != 0x04010000u ||
                t[1] != 4u || !(t[3] & 2u)) { live_model.fault = 1u; return; }
            live_model.index = 0;
            live_model.cycle ^= 1u;
            continue;
        }
        if (type != PI4_XHCI_TRB_TYPE_NOOP ||
            live_model.consumed >= live_model.issued ||
            t[0] != live_model.consumed + 1u || t[1] != ~t[0] || t[2] != 0x4321u) {
            live_model.fault = 1u; return;
        }
        live_model.consumed++;
        live_model.index++;
        if (pi4_xhci_dma_event_cursor_advance(&live_model.events) != 0)
            live_model.fault = 1u;
    }
    live_model.fault = 1u;
}

static int live_ring_test(unsigned prepare_prefix) {
    _Alignas(64) uint32_t ring[1024] = {0};
    const uint64_t bus = 0x404010000ull;
    struct pi4_xhci_dma_producer p = {252u, 1u, 0u};
    live_model = (__typeof__(live_model)){
        .ring = ring, .index = 252u, .cycle = 1u, .events = {0u, 1u}};
    if (pi4_xhci_dma_transfer_ring_seed(ring, bus) != 0) return 0;
    for (unsigned i = 248u; i < 252u; i++) ring[i * 4u + 3u] = 1u;
    if (prepare_prefix && pi4_xhci_dma_ring_skipped_prefix((uint8_t *)ring,
            248u, 1u, live_model_sync) != 0) return 0;
    for (unsigned i = 0; i < 1000u; i++) {
        uint32_t words[4] = {i + 1u, ~(i + 1u), 0x4321u,
                            PI4_XHCI_TRB_TYPE_NOOP << 10};
        unsigned queued = UINT32_MAX, expected_index = p.index;
        live_model.issued++;
        if (pi4_xhci_dma_ring_enqueue((uint8_t *)ring, bus, &p, words,
                live_model_sync, &queued) != 0 || queued != expected_index)
            return 0;
        if (live_model.fault) return !prepare_prefix && i == 2u;
        if (live_model.consumed != i + 1u) return 0;
    }
    if (!prepare_prefix || p.wraps != 4u || p.index != 232u || p.cycle != 1u ||
        live_model.events.index != 232u || live_model.events.cycle != 0u)
        return 0;
    uint32_t words[4] = {0, 0, 0, PI4_XHCI_TRB_TYPE_NOOP << 10};
    unsigned queued;
    p.index = 255u;
    if (pi4_xhci_dma_ring_enqueue((uint8_t *)ring, bus, &p, words,
            live_model_sync, &queued) == 0) return 0;
    p.index = 0; p.cycle = 2;
    if (pi4_xhci_dma_ring_enqueue((uint8_t *)ring, bus, &p, words,
            live_model_sync, &queued) == 0) return 0;
    p.cycle = 1;
    if (pi4_xhci_dma_ring_enqueue((uint8_t *)ring + 16, bus, &p, words,
            live_model_sync, &queued) == 0 ||
        pi4_xhci_dma_ring_enqueue((uint8_t *)ring, bus + 16, &p, words,
            live_model_sync, &queued) == 0 ||
        pi4_xhci_dma_ring_skipped_prefix((uint8_t *)ring, 255u, 1u,
            live_model_sync) == 0) return 0;
    return 1;
}

/* Mirror the live session's interleaved command/OUT/IN/IN traffic, with a
 * separate device-written event ring. Each publication runs the consumer;
 * each completion must pass the production pointer/slot/DCI validator before
 * software advances ERDP. This tests the different producer wrap rates. */
struct live_mixed_ring {
    _Alignas(64) uint32_t memory[1024];
    struct pi4_xhci_dma_producer producer;
    uint64_t bus;
    unsigned index, cycle, dci, pending, fault;
    uint32_t expected[4];
};
static struct {
    struct live_mixed_ring ring[3]; /* command, bulk OUT, bulk IN */
    uint32_t events[256][4];
    struct pi4_xhci_dma_event_cursor consumer;
    unsigned event_index, event_cycle, event_count, fault;
} live_mixed;

static void live_mixed_sync(uint8_t *bytes, uint64_t length) {
    (void)length;
    struct live_mixed_ring *r = NULL;
    for (unsigned i = 0; i < 3u; i++) {
        uintptr_t start = (uintptr_t)live_mixed.ring[i].memory;
        if ((uintptr_t)bytes >= start && (uintptr_t)bytes < start + 4096u)
            r = &live_mixed.ring[i];
    }
    if (!r) { live_mixed.fault = 1u; return; }
    for (unsigned guard = 0; guard < 257u; guard++) {
        uint32_t *t = r->memory + r->index * 4u;
        if ((t[3] & 1u) != r->cycle || r->fault) return;
        if (((t[3] >> 10) & 63u) == PI4_XHCI_TRB_TYPE_LINK) {
            if (r->index != 255u || t[0] != (uint32_t)r->bus ||
                t[1] != (uint32_t)(r->bus >> 32) || !(t[3] & 2u)) {
                r->fault = 1u; return;
            }
            r->index = 0; r->cycle ^= 1u;
            continue;
        }
        if (!r->pending || t[0] != r->expected[0] || t[1] != r->expected[1] ||
            t[2] != r->expected[2] || (t[3] & ~1u) != (r->expected[3] & ~1u)) {
            r->fault = 1u; return;
        }
        uint64_t ptr = r->bus + r->index * 16u;
        uint32_t *e = live_mixed.events[live_mixed.event_index];
        e[0] = (uint32_t)ptr; e[1] = (uint32_t)(ptr >> 32); e[2] = 1u << 24;
        e[3] = (r->dci ? (32u << 10) | (1u << 24) | (r->dci << 16) :
                         33u << 10) | live_mixed.event_cycle;
        if (++live_mixed.event_index == 256u) {
            live_mixed.event_index = 0; live_mixed.event_cycle ^= 1u;
        }
        live_mixed.event_count++;
        r->pending = 0; r->index++;
    }
    r->fault = 1u;
}

static int live_mixed_phase(unsigned ring, unsigned bytes) {
    struct live_mixed_ring *r = &live_mixed.ring[ring];
    _Alignas(16) uint32_t words[4];
    unsigned queued, actual, status = 1u << 24;
    unsigned index = live_mixed.consumer.index;
    uint32_t *e = live_mixed.events[index];
    if (pi4_xhci_dma_event_ready(e[3], &live_mixed.consumer)) return 0;
    if (ring ? pi4_xhci_dma_bulk_normal_seed(words, 0x404002400ull,
                    bytes, ring == 2u, r->producer.cycle) != 0 :
               pi4_xhci_dma_noop_seed(words, r->producer.cycle) != 0)
        return 0;
    for (unsigned i = 0; i < 4u; i++) r->expected[i] = words[i];
    uint64_t expected = r->bus + r->producer.index * 16u;
    r->pending = 1u;
    if (pi4_xhci_dma_ring_enqueue((uint8_t *)r->memory, r->bus, &r->producer,
            words, live_mixed_sync, &queued) != 0 || r->fault || r->pending ||
        live_mixed.fault || r->bus + queued * 16u != expected ||
        !pi4_xhci_dma_event_ready(e[3], &live_mixed.consumer) || e[2] != status)
        return 0;
    uint64_t ptr = (uint64_t)e[0] | (uint64_t)e[1] << 32;
    if (ring) {
        if (!pi4_xhci_dma_bulk_event_ok(ptr, e[2], e[3], expected,
                1u, r->dci, bytes, 0u, &actual) || actual != bytes ||
            pi4_xhci_dma_bulk_event_ok(ptr, e[2], e[3], expected + 16u,
                1u, r->dci, bytes, 0u, &actual) ||
            pi4_xhci_dma_bulk_event_ok(ptr, e[2], e[3], expected,
                2u, r->dci, bytes, 0u, &actual)) return 0;
    } else if (!pi4_xhci_dma_command_event_ok(ptr, e[2], e[3], expected) ||
               pi4_xhci_dma_command_event_ok(ptr, e[2], e[3], expected + 16u))
        return 0;
    if (pi4_xhci_dma_event_cursor_advance(&live_mixed.consumer) != 0 ||
        pi4_xhci_dma_event_dequeue(0x404004000ull, &live_mixed.consumer) !=
            ((0x404004000ull + live_mixed.event_index * 16u) | 8u)) return 0;
    return live_mixed.consumer.index == live_mixed.event_index &&
           live_mixed.consumer.cycle == live_mixed.event_cycle;
}

static int live_mixed_test(void) {
    live_mixed = (__typeof__(live_mixed)){0};
    /* The accepted file test used 14 commands after INQUIRY and 59 events. */
    const unsigned next[3] = {252u, 15u, 30u};
    live_mixed.event_index = live_mixed.consumer.index = 59u;
    live_mixed.event_cycle = live_mixed.consumer.cycle = 1u;
    /* Previously consumed events retain the first lap's Cycle bit in RAM. */
    for (unsigned i = 0; i < 59u; i++) live_mixed.events[i][3] = 1u;
    for (unsigned i = 0; i < 3u; i++) {
        struct live_mixed_ring *r = &live_mixed.ring[i];
        r->bus = 0x404010000ull + i * 0x2000u;
        r->producer.index = r->index = next[i];
        r->producer.cycle = r->cycle = 1u;
        r->dci = i == 1u ? 2u : (i == 2u ? 3u : 0u);
        if (pi4_xhci_dma_transfer_ring_seed(r->memory, r->bus) != 0) return 0;
        for (unsigned j = 0; j < next[i]; j++) r->memory[j * 4u + 3u] = 1u;
    }
    for (unsigned n = 0; n < 1000u; n++) {
        const unsigned rings[4] = {0u, 1u, 2u, 2u};
        const unsigned bytes[4] = {0u, 31u, 512u, 13u};
        for (unsigned phase = 0; phase < 4u; phase++) {
            if (live_mixed_phase(rings[phase], bytes[phase])) continue;
            struct live_mixed_ring *r = &live_mixed.ring[rings[phase]];
            fprintf(stderr, "mixed command=%u phase=%u producer=%u/%u consumer=%u/%u fault=%u pending=%u event=%u/%u\n",
                n, phase, r->producer.index, r->producer.cycle, r->index, r->cycle,
                r->fault, r->pending, live_mixed.consumer.index, live_mixed.consumer.cycle);
            return 0;
        }
        if (n == 271u && (live_mixed.ring[0].producer.wraps != 2u ||
            live_mixed.ring[1].producer.wraps != 1u ||
            live_mixed.ring[2].producer.wraps != 2u ||
            (59u + live_mixed.event_count) / 256u != 4u)) return 0;
    }
    return live_mixed.event_count == 4000u &&
        live_mixed.consumer.index == 219u && live_mixed.consumer.cycle == 0u &&
        live_mixed.ring[0].producer.wraps == 4u &&
        live_mixed.ring[1].producer.wraps == 3u &&
        live_mixed.ring[2].producer.wraps == 7u;
}

int main(void) {
    if (!live_ring_test(0u) || !live_ring_test(1u))
        return fail("live ring publication, skipped prefix, four producer laps and three event laps");
    if (!live_mixed_test())
        return fail("mixed No-op/CBW/data/CSW and event-ring wraps after bootstrap adoption");
    {
        uint8_t cbw[31] = {0};
        const uint8_t read_expected[31] = {
            0x55, 0x53, 0x42, 0x43, 4, 3, 2, 1,
            0, 2, 0, 0, 0x80, 0, 10,
            0x28, 0, 0x12, 0x34, 0x56, 0x78, 0, 0, 1, 0,
            0, 0, 0, 0, 0, 0
        };
        uint8_t capacity[8] = {0, 0, 7, 0xff, 0, 0, 2, 0};
        uint32_t last = 0, block = 0;
        uint64_t size = 0;
        if (pi4_xhci_dma_bot_read10_cbw_seed(cbw, 31u, 0x01020304u,
                0x12345678u, 0x12345679u, 512u) != 0)
            return fail("READ(10) one-block wrapper");
        for (unsigned i = 0; i < 31u; i++)
            if (cbw[i] != read_expected[i])
                return fail("READ(10) CDB is BE while CBW tag/byte count are LE");
        if (pi4_xhci_dma_bot_capacity10_cbw_seed(cbw, 31u, 0x01020304u) != 0 ||
            cbw[8] != 8u || cbw[14] != 10u || cbw[15] != 0x25u)
            return fail("READ CAPACITY(10) CDB and eight-byte data length");
        for (unsigned i = 16u; i < 31u; i++)
            if (cbw[i]) return fail("READ CAPACITY PMI, LBA and reserved bytes zero");
        if (!pi4_xhci_dma_bot_capacity10_parse(capacity, 8u, &last, &block, &size) ||
            last != 2047u || block != 512u || size != 1048576ull ||
            pi4_xhci_dma_bot_capacity10_parse(capacity, 7u, &last, &block, &size))
            return fail("capacity is (last LBA + 1) times BE block length");
        for (unsigned i = 0; i < 4u; i++) capacity[i] = 0xffu;
        if (pi4_xhci_dma_bot_capacity10_parse(capacity, 8u, &last, &block, &size) || size)
            return fail("READ CAPACITY(16) sentinel cannot be truncated or guessed");
        capacity[3] = 0xfeu;
        if (!pi4_xhci_dma_bot_capacity10_parse(capacity, 8u, &last, &block, &size) ||
            size != 0x1fffffffe00ull)
            return fail("large capacity arithmetic remains 64-bit");
        capacity[0] = capacity[1] = capacity[2] = capacity[3] = 0u;
        for (unsigned bytes = 512u; bytes <= 2048u; bytes *= 2u) {
            capacity[6] = (uint8_t)(bytes >> 8);
            if (!pi4_xhci_dma_bot_capacity10_parse(capacity, 8u, &last, &block, &size) ||
                block != bytes || size != bytes ||
                pi4_xhci_dma_bot_read10_cbw_seed(cbw, 31u, 42u, 0u, last, block) != 0 ||
                pi4_xhci_dma_bot_read10_cbw_seed(cbw, 31u, 42u, 1u, last, block) == 0)
                return fail("bounded logical block sizes and last-LBA boundary");
        }
        capacity[6] = 0u;
        if (pi4_xhci_dma_bot_capacity10_parse(capacity, 8u, &last, &block, &size) ||
            pi4_xhci_dma_bot_read10_cbw_seed(cbw, 31u, 42u, 0u, 10u, 0u) == 0)
            return fail("zero block length rejects");
        capacity[6] = 16u;
        if (pi4_xhci_dma_bot_capacity10_parse(capacity, 8u, &last, &block, &size) ||
            pi4_xhci_dma_bot_read10_cbw_seed(cbw, 31u, 42u, 0u, 10u, 4096u) == 0 ||
            pi4_xhci_dma_bot_read10_cbw_seed(cbw, 30u, 42u, 0u, 10u, 512u) == 0)
            return fail("oversized block and undersized CBW buffer reject");
        const uint8_t write_cdb[10] = {0x2au};
        if (pi4_xhci_dma_bot_read_cbw_seed(cbw, 31u, 42u, write_cdb, 10u, 512u) == 0)
            return fail("read-only BOT wrapper rejects WRITE(10)");
        if (pi4_xhci_dma_bot_sense_cbw_seed(cbw, 31u, 42u) != 0 ||
            cbw[8] != 18u || cbw[14] != 6u || cbw[15] != 3u || cbw[19] != 18u)
            return fail("REQUEST SENSE standard fixed-format allocation");
        uint8_t sense[18] = {0x70u, 0, 6u, 0, 0, 0, 0, 10u, 0, 0, 0, 0, 0x29u};
        unsigned key = 0, asc = 0, ascq = 0, status = UINT32_MAX;
        if (!pi4_xhci_dma_bot_sense_parse(sense, 18u, &key, &asc, &ascq) ||
            key != 6u || asc != 0x29u || ascq ||
            !pi4_xhci_dma_bot_sense_retryable(key, asc, ascq) ||
            pi4_xhci_dma_bot_sense_parse(sense, 13u, &key, &asc, &ascq))
            return fail("power-on Unit Attention and truncated fixed sense");
        sense[7] = 5u;
        if (pi4_xhci_dma_bot_sense_parse(sense, 18u, &key, &asc, &ascq))
            return fail("fixed sense must declare ASC/ASCQ bytes");
        sense[0] = 0x72u; sense[1] = 2u; sense[2] = 4u; sense[3] = 1u;
        if (!pi4_xhci_dma_bot_sense_parse(sense, 8u, &key, &asc, &ascq) ||
            key != 2u || asc != 4u || ascq != 1u ||
            !pi4_xhci_dma_bot_sense_retryable(key, asc, ascq) ||
            pi4_xhci_dma_bot_sense_retryable(2u, 0x3au, 0u) ||
            pi4_xhci_dma_bot_sense_retryable(5u, 0x20u, 0u))
            return fail("retry becoming-ready only; no-medium/illegal-request stop");
        uint8_t csw[13] = {0x55, 0x53, 0x42, 0x53, 4, 3, 2, 1, 8, 0, 0, 0, 1};
        if (!pi4_xhci_dma_bot_csw_valid(csw, 13u, 0x01020304u, 8u, 0u, &status) ||
            status != 1u || pi4_xhci_dma_bot_csw_ok(csw, 13u, 0x01020304u, 8u, 0u))
            return fail("command failure is valid BOT status but never command success");
        csw[12] = 2u;
        if (pi4_xhci_dma_bot_csw_valid(csw, 13u, 0x01020304u, 8u, 0u, &status))
            return fail("phase error is not eligible for sense/retry");
        const uint8_t crc_vector[] = "123456789";
        if (pi4_xhci_dma_crc32(crc_vector, 9u) != 0xcbf43926u ||
            pi4_xhci_dma_crc32(crc_vector, 0u) != 0u)
            return fail("sector fingerprint uses standard CRC32");
        if (PI4_XHCI_DMA_BOT_SECTOR_OFFSET % 64u ||
            PI4_XHCI_DMA_BOT_CSW_OFFSET + 64u > PI4_XHCI_DMA_BOT_SECTOR_OFFSET ||
            PI4_XHCI_DMA_BOT_SECTOR_OFFSET + PI4_XHCI_DMA_BOT_MAX_BLOCK_BYTES >
                PI4_XHCI_DMA_ROOT_VALUE_OFFSET ||
            2u + PI4_XHCI_DMA_BOT_STORAGE_COMMANDS * 2u >= 255u)
            return fail("bounded storage buffers and command sequence fit before ring links");
    }
    {
        /* Wire-format oracle: USB MSC BOT CBW, standard SCSI INQUIRY CDB. */
        const uint8_t expected[31] = {
            0x55, 0x53, 0x42, 0x43, 1, 0x49, 0x50, 0x5a,
            36, 0, 0, 0, 0x80, 0, 6, 0x12, 0, 0, 0, 36, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };
        uint8_t cbw[32] = {0}, inquiry[36] = {0};
        uint8_t csw[13] = {0x55, 0x53, 0x42, 0x53, 1, 0x49, 0x50, 0x5a};
        _Alignas(16) uint32_t trb[4] = {0};
        const uint64_t bus = 0x400040240ull, trb_bus = 0x400010000ull;
        const uint32_t control = (32u << 10) | (3u << 16) | (1u << 24) | 1u;
        unsigned actual = 0;
        if (pi4_xhci_dma_bot_inquiry_cbw_seed(cbw, 30u,
                PI4_XHCI_DMA_BOT_INQUIRY_TAG) == 0 ||
            pi4_xhci_dma_bot_inquiry_cbw_seed(cbw, sizeof(cbw),
                PI4_XHCI_DMA_BOT_INQUIRY_TAG) != 0)
            return fail("BOT CBW capacity bounds");
        for (unsigned i = 0; i < 31u; i++)
            if (cbw[i] != expected[i]) return fail("INQUIRY CBW wire format");
        if (pi4_xhci_dma_bulk_normal_seed(trb, bus, 36u, 1u, 1u) != 0 ||
            trb[0] != 0x40240u || trb[1] != 4u || trb[2] != 36u ||
            trb[3] != ((1u << 10) | (1u << 5) | (1u << 2) | 1u) ||
            pi4_xhci_dma_bulk_normal_seed(trb, 0x40000fff0ull, 36u, 1u, 1u) == 0 ||
            pi4_xhci_dma_bulk_normal_seed(trb, bus, 0u, 0u, 1u) == 0 ||
            pi4_xhci_dma_bulk_normal_seed(trb, bus, 31u, 0u, 0u) != 0 ||
            trb[3] != ((1u << 10) | (1u << 5)))
            return fail("bulk Normal TRB address, IOC/ISP, cycle, boundary and length");
        if (!pi4_xhci_dma_bulk_event_ok(trb_bus, 1u << 24, control,
                trb_bus, 1u, 3u, 36u, 0u, &actual) || actual != 36u ||
            !pi4_xhci_dma_bulk_event_ok(trb_bus, (13u << 24) | 4u, control,
                trb_bus, 1u, 3u, 36u, 1u, &actual) || actual != 32u ||
            pi4_xhci_dma_bulk_event_ok(trb_bus, (13u << 24) | 4u, control,
                trb_bus, 1u, 3u, 36u, 0u, &actual) ||
            pi4_xhci_dma_bulk_event_ok(trb_bus, (13u << 24) | 37u, control,
                trb_bus, 1u, 3u, 36u, 1u, &actual) ||
            pi4_xhci_dma_bulk_event_ok(trb_bus, 6u << 24, control,
                trb_bus, 1u, 3u, 36u, 1u, &actual) ||
            pi4_xhci_dma_bulk_event_ok(trb_bus + 16u, 1u << 24, control,
                trb_bus, 1u, 3u, 36u, 1u, &actual) ||
            pi4_xhci_dma_bulk_event_ok(trb_bus, 1u << 24, control,
                trb_bus, 2u, 3u, 36u, 1u, &actual) ||
            pi4_xhci_dma_bulk_event_ok(trb_bus, 1u << 24, control,
                trb_bus, 1u, 4u, 36u, 1u, &actual) ||
            pi4_xhci_dma_bulk_event_ok(trb_bus, 1u << 24, control | (1u << 2),
                trb_bus, 1u, 3u, 36u, 1u, &actual))
            return fail("bulk events reject stalls, wrong identity, invalid residuals and Event Data");
        if (!pi4_xhci_dma_bot_csw_ok(csw, 13u, PI4_XHCI_DMA_BOT_INQUIRY_TAG, 36u, 36u) ||
            pi4_xhci_dma_bot_csw_ok(csw, 12u, PI4_XHCI_DMA_BOT_INQUIRY_TAG, 36u, 36u) ||
            pi4_xhci_dma_bot_csw_ok(csw, 13u, 42u, 36u, 36u))
            return fail("BOT CSW signature/length/tag");
        csw[8] = 4u;
        if (!pi4_xhci_dma_bot_csw_ok(csw, 13u, PI4_XHCI_DMA_BOT_INQUIRY_TAG, 36u, 32u) ||
            pi4_xhci_dma_bot_csw_ok(csw, 13u, PI4_XHCI_DMA_BOT_INQUIRY_TAG, 36u, 36u))
            return fail("CSW residue reconciles with the IN transfer count");
        csw[8] = 0u;
        for (unsigned status = 1u; status <= 3u; status++) {
            csw[12] = status;
            if (pi4_xhci_dma_bot_csw_ok(csw, 13u, PI4_XHCI_DMA_BOT_INQUIRY_TAG, 36u, 36u))
                return fail("CSW command-failed, phase-error and reserved status reject");
        }
        csw[12] = 0u; csw[0] = 0;
        if (pi4_xhci_dma_bot_csw_ok(csw, 13u, PI4_XHCI_DMA_BOT_INQUIRY_TAG, 36u, 36u))
            return fail("CSW corrupt signature rejects");
        inquiry[3] = 2u; inquiry[4] = 31u;
        if (!pi4_xhci_dma_bot_inquiry_ok(inquiry, 36u) ||
            pi4_xhci_dma_bot_inquiry_ok(inquiry, 35u))
            return fail("INQUIRY must contain the full standard identity");
        inquiry[0] = 0x7fu;
        if (pi4_xhci_dma_bot_inquiry_ok(inquiry, 36u))
            return fail("INQUIRY must describe a present direct-access device");
        char text[5];
        const uint8_t unsafe[4] = {'A', 0, '\n', 0xffu};
        pi4_xhci_dma_bot_identity_text(text, unsafe, 4u);
        if (text[0] != 'A' || text[1] != '.' || text[2] != '.' ||
            text[3] != '.' || text[4]) return fail("identity text sanitizes control bytes");
        if (PI4_XHCI_DMA_BOT_CBW_OFFSET % 64u ||
            PI4_XHCI_DMA_BOT_DATA_OFFSET < PI4_XHCI_DMA_BOT_CBW_OFFSET + 64u ||
            PI4_XHCI_DMA_BOT_CSW_OFFSET < PI4_XHCI_DMA_BOT_DATA_OFFSET + 64u ||
            PI4_XHCI_DMA_BOT_CSW_OFFSET + 64u > PI4_XHCI_DMA_ROOT_VALUE_OFFSET)
            return fail("BOT DMA buffers own distinct cache lines");
    }
    {
        /* IMG_9124: the USB 2 hub is on root 1, but the stick negotiated
         * SuperSpeed on root 3. Its absence from the hub is expected. */
        uint32_t ports[5] = {0x00400e03u, 0x000002a0u, 0x00081203u,
                             0x000002a0u, 0x000002a0u};
        const uint32_t protocols[5] = {2u, 3u, 3u, 3u, 3u};
        uint32_t reset;
        if (pi4_xhci_dma_select_root_port(ports, protocols, 5u) != 3u ||
            pi4_xhci_dma_port_reset_value(ports[2], &reset) != 0 ||
            reset != (PI4_XHCI_PORTSC_PP | PI4_XHCI_PORTSC_WPR) ||
            pi4_xhci_dma_port_reset_change(ports[2]) != PI4_XHCI_PORTSC_WRC ||
            pi4_xhci_dma_port_reset_change(ports[0]) != PI4_XHCI_PORTSC_PRC)
            return fail("connected SuperSpeed root beats the always-connected USB2 hub");
        ports[2] &= ~PI4_XHCI_PORTSC_PP;
        if (pi4_xhci_dma_select_root_port(ports, protocols, 5u) != 1u)
            return fail("an unpowered SuperSpeed root cannot displace USB2 fallback");
        ports[2] |= PI4_XHCI_PORTSC_PP;
        ports[2] &= ~PI4_XHCI_PORTSC_CCS;
        if (pi4_xhci_dma_select_root_port(ports, protocols, 5u) != 1u)
            return fail("disconnected SuperSpeed port retains USB2 fallback");
        ports[0] &= ~PI4_XHCI_PORTSC_CCS;
        if (pi4_xhci_dma_select_root_port(ports, protocols, 5u) != 0u ||
            pi4_xhci_dma_select_root_port(0, protocols, 5u) != 0u ||
            pi4_xhci_dma_select_root_port(ports, protocols, PI4_XHCI_DIAG_PORTS + 1u) != 0u ||
            pi4_xhci_dma_port_reset_value(PI4_XHCI_PORTSC_CCS |
                PI4_XHCI_PORTSC_PP | PI4_XHCI_PORTSC_WPR, &reset) == 0)
            return fail("root selection/reset validates bounds, connection and busy state");
    }
    {
        /* IMG_9127: HCRST/link training still has PR=1 and speed=0 on root 3
         * in the early snapshot. The protocol capability already owns it. */
        uint32_t ports[5] = {0x400202e1u, 0x000002a0u, 0x000002b1u,
                             0x000002a0u, 0x000002a0u};
        const uint32_t protocols[5] = {2u, 3u, 3u, 3u, 3u};
        uint32_t reset;
        if (pi4_xhci_dma_select_root_port(ports, protocols, 5u) != 3u ||
            pi4_xhci_dma_superspeed_port_settled(ports[2], protocols[2]) ||
            pi4_xhci_dma_port_reset_value(ports[2], &reset) == 0)
            return fail("training USB3 connection stays selected while its reset is busy");
        ports[2] = 0x00081203u;
        if (pi4_xhci_dma_select_root_port(ports, protocols, 5u) != 3u ||
            !pi4_xhci_dma_superspeed_port_settled(ports[2], protocols[2]) ||
            pi4_xhci_dma_select_root_port(ports, 0, 5u) != 3u)
            return fail("trained SuperSpeed root can proceed to warm reset");
        ports[2] |= PI4_XHCI_PORTSC_PR;
        if (pi4_xhci_dma_superspeed_port_settled(ports[2], protocols[2]))
            return fail("known speed alone does not make a resetting root settled");
        ports[2] = 0x2a0u;
        if (pi4_xhci_dma_select_root_port(ports, protocols, 5u) != 1u)
            return fail("USB2 fallback remains available when no USB3 device is connected");
    }
    {
        unsigned requested = 0;
        if (pi4_xhci_dma_ep0_mps_update_plan(1u, 64u, 8u, &requested) != 1 ||
            requested != 64u ||
            pi4_xhci_dma_ep0_mps_update_plan(1u, 8u, 8u, &requested) != 0 ||
            pi4_xhci_dma_ep0_mps_update_plan(3u, 64u, 64u, &requested) != 0 ||
            requested != 64u ||
            pi4_xhci_dma_ep0_mps_update_plan(2u, 8u, 8u, &requested) != 0 ||
            pi4_xhci_dma_ep0_mps_update_plan(4u, 9u, 512u, &requested) != 0 ||
            requested != 512u ||
            pi4_xhci_dma_ep0_mps_update_plan(3u, 64u, 8u, &requested) != -1 ||
            pi4_xhci_dma_ep0_mps_update_plan(4u, 64u, 512u, &requested) != -1 ||
            pi4_xhci_dma_ep0_mps_update_plan(1u, 9u, 8u, &requested) != -1)
            return fail("only a changed Full-Speed EP0 needs Evaluate Context");
    }
    {
        const uint8_t device[18] = {
            18, 1, 0, 3, 0, 0, 0, 9, 0x34, 0x12, 0x78, 0x56, 0, 1, 1, 2, 3, 1
        };
        uint8_t config[] = {
            9, 2, 44, 0, 1, 1, 0, 0x80, 50,
            9, 4, 0, 0, 2, 8, 6, 0x50, 0,
            7, 5, 0x81, 2, 0, 4, 0,
            6, 48, 15, 0, 0, 0,
            7, 5, 0x02, 2, 0, 4, 0,
            6, 48, 15, 0, 0, 0
        };
        struct pi4_xhci_dma_config_summary summary;
        _Alignas(64) uint8_t contexts[192] = {0};
        const uint32_t *slot = (const uint32_t *)(const void *)(contexts + 64u);
        const uint32_t *ep0 = (const uint32_t *)(const void *)(contexts + 128u);
        if (pi4_xhci_dma_address_context_seed(contexts, sizeof(contexts),
                64u, 4u, 3u, 0x04042000u) != 0 ||
            ((slot[0] >> 20) & 15u) != 4u || slot[1] != (3u << 16) ||
            (ep0[1] >> 16) != 512u ||
            !pi4_xhci_dma_child_device_descriptor_ok(device, sizeof(device), 4u) ||
            pi4_xhci_dma_child_device_descriptor_ok(device, sizeof(device), 3u) ||
            !pi4_xhci_dma_configuration_descriptor_ok(config, sizeof(config),
                PI4_XHCI_DMA_ROOT_CONFIG_CAPACITY, &summary) ||
            !summary.bot_interface_found || summary.bot_bulk_in_address != 0x81u ||
            summary.bot_bulk_out_address != 2u ||
            summary.bot_bulk_in_max_packet != 1024u ||
            summary.bot_bulk_out_max_packet != 1024u ||
            summary.bot_bulk_in_max_burst != 15u || summary.bot_bulk_in_streams ||
            summary.bot_bulk_out_max_burst != 15u || summary.bot_bulk_out_streams)
            return fail("direct SuperSpeed EP0 and BOT descriptors with SS companions");
        for (unsigned context_size = 32u; context_size <= 64u; context_size += 32u) {
            _Alignas(64) uint8_t in[4096] = {0}, out[4096] = {0};
            uint32_t *slot = (uint32_t *)(void *)out;
            slot[0] = (4u << 20) | (1u << 27);
            slot[1] = 3u << 16;
            slot[3] = 2u << 27;
            if (pi4_xhci_dma_bulk_contexts_seed(in, sizeof(in), context_size,
                    out, sizeof(out), 8u, 6u, 0x50u, 0u, 2u,
                    0x81u, 2u, 1024u, 7u, 2u, 2u, 1024u, 15u, 4u,
                    0x400010000ull, 0x400012000ull) != 0 ||
                ((uint32_t *)(void *)in)[1] != (1u | (1u << 3) | (1u << 4)))
                return fail("SS bulk context pair supports both context sizes");
            /* Model the controller's configured Output Context layout. */
            for (unsigned i = 0; i < context_size; i++) {
                out[i] = in[context_size + i];
                out[3u * context_size + i] = in[4u * context_size + i];
                out[4u * context_size + i] = in[5u * context_size + i];
            }
            slot[3] = 3u << 27;
            ((uint32_t *)(void *)(out + 3u * context_size))[0] = 1u;
            ((uint32_t *)(void *)(out + 4u * context_size))[0] = 1u;
            if (!pi4_xhci_dma_bulk_contexts_ok(out, sizeof(out), context_size,
                    0x81u, 1024u, 7u, 0x400010000ull, 2u, 1024u, 15u, 0x400012000ull) ||
                pi4_xhci_dma_bulk_contexts_ok(out, sizeof(out), context_size,
                    0x81u, 1024u, 0u, 0x400010000ull, 2u, 1024u, 15u, 0x400012000ull))
                return fail("SS Output Context verifies MaxBurst, types, MPS and rings");
            if (pi4_xhci_dma_bulk_contexts_seed(in, sizeof(in), context_size,
                    out, sizeof(out), 8u, 6u, 0x50u, 0u, 2u,
                    0x81u, 2u, 512u, 7u, 2u, 2u, 1024u, 15u, 4u,
                    0x400010000ull, 0x400012000ull) == 0 ||
                pi4_xhci_dma_bulk_contexts_seed(in, sizeof(in), context_size,
                    out, sizeof(out), 8u, 6u, 0x50u, 0u, 2u,
                    0x81u, 2u, 1024u, 16u, 2u, 2u, 1024u, 15u, 4u,
                    0x400010000ull, 0x400012000ull) == 0)
                return fail("SS rejects wrong MPS and oversized burst");
        }
        const unsigned bad_offset[] = {25u, 27u, 28u, 29u, 30u};
        const uint8_t bad_value[] = {5u, 16u, 17u, 1u, 1u};
        for (unsigned i = 0; i < 5u; i++) {
            uint8_t saved = config[bad_offset[i]];
            config[bad_offset[i]] = bad_value[i];
            if (pi4_xhci_dma_configuration_descriptor_ok(config, sizeof(config),
                    sizeof(config), &summary))
                return fail("SS companion validates size, burst, streams and interval bytes");
            config[bad_offset[i]] = saved;
        }
        config[16] = 0x62u;
        if (!pi4_xhci_dma_configuration_descriptor_ok(config, sizeof(config),
                sizeof(config), &summary) || summary.bot_interface_found)
            return fail("a UAS interface must not trigger BOT class requests");
        if (PI4_XHCI_DMA_ROOT_CONFIG_OFFSET + PI4_XHCI_DMA_ROOT_CONFIG_CAPACITY >
                PI4_XHCI_DMA_ROOT_VALUE_OFFSET ||
            PI4_XHCI_DMA_ROOT_VALUE_OFFSET + 64u > PI4_XHCI_DMA_ROOT_LUN_OFFSET ||
            PI4_XHCI_DMA_ROOT_LUN_OFFSET + 64u > PI4_XHCI_DMA_CONTROL_BUFFER_BYTES)
            return fail("direct-root configuration and values use separate cache lines");
    }
    const struct pi4_dma_probe_plan arena = {
        .phys_base = 0x04010000ull,
        .bus_base = 0x42010000ull,
        .size = PI4_USB_DMA_ARENA_BYTES
    };
    const uint32_t caps[6] = {
        0x01000020u, 0x05000420u, 0, 0x02841e01u, 0x1000u, 0x800u
    };
    struct pi4_xhci_dma_layout layout;
    _Alignas(64) uint8_t command_ring[PI4_XHCI_DMA_RING_BYTES] = {0};
    _Alignas(64) uint8_t transfer_ring[PI4_XHCI_DMA_RING_BYTES] = {0};
    _Alignas(64) uint8_t input_context[PI4_XHCI_DMA_CONTEXT_BYTES] = {0};
    _Alignas(64) uint8_t output_context[PI4_XHCI_DMA_CONTEXT_BYTES] = {0};
    _Alignas(64) uint8_t child_context[PI4_XHCI_DMA_CONTEXT_BYTES] = {0};
    uint16_t enabled;
    uint32_t evt_control = (PI4_XHCI_TRB_TYPE_CMD_EVT << 10) | 1u;
    uint64_t dcbaa_bus = arena.bus_base;
    uint64_t erst_bus = arena.bus_base + 0x6000;
    uint64_t event_bus = arena.bus_base + 0x4000;

    {
        struct pi4_dma_probe_plan aliased, invalid = arena;
        uint32_t narrow_caps[6];
        for (unsigned i = 0; i < 6; i++) narrow_caps[i] = caps[i];
        if (pi4_xhci_dma_alias_plan_make(&arena, caps,
                PI4_XHCI_DMA_BAR2_LO_EXPECTED,
                PI4_XHCI_DMA_BAR2_HI_ORIGINAL, &aliased) != 0 ||
            aliased.phys_base != arena.phys_base ||
            aliased.bus_base != arena.bus_base + PI4_XHCI_DMA_BUS_BIAS ||
            aliased.size != arena.size ||
            PI4_XHCI_DMA_BAR2_HI_TEST !=
                (PI4_XHCI_DMA_BUS_BIAS >> 32))
            return fail("BAR2 16-GiB DMA alias keeps CPU arena and shifts bus view");
        if (pi4_xhci_dma_alias_plan_make(&arena, caps, 0x10u, 0, &aliased) == 0 ||
            pi4_xhci_dma_alias_plan_make(&arena, caps, 0x11u, 1, &aliased) == 0)
            return fail("BAR2 alias rejects an unexpected inbound baseline");
        narrow_caps[3] &= ~1u;
        if (pi4_xhci_dma_alias_plan_make(&arena, narrow_caps, 0x11u, 0,
                                         &aliased) == 0)
            return fail("BAR2 alias requires xHCI 64-bit DMA capability");
        invalid.bus_base = UINT64_MAX - PI4_XHCI_DMA_BUS_BIAS + 1u;
        if (pi4_xhci_dma_alias_plan_make(&invalid, caps, 0x11u, 0,
                                         &aliased) == 0)
            return fail("BAR2 alias rejects bus address overflow");
    }

    {
        uint32_t run_cmd = pi4_xhci_dma_run_command_value(
            PI4_XHCI_USBCMD_INTE | (1u << 13));
        if ((run_cmd & PI4_XHCI_USBCMD_INTE) ||
            !(run_cmd & PI4_XHCI_USBCMD_RUN) ||
            !(run_cmd & PI4_XHCI_USBCMD_EWE) || !(run_cmd & (1u << 13)))
            return fail("polled run keeps host IRQs off and requests MFINDEX events");
    }

    if (pi4_xhci_dma_layout_make(&arena, caps, 1, 0,
            PI4_XHCI_USBSTS_HALTED, PI4_XHCI_LEGACY_OS_OWNED,
            PI4_PCI_COMMAND_MEMORY, &layout) != PI4_XHCI_DMA_PLAN_OK ||
        layout.max_slots != PI4_XHCI_DMA_MAX_SLOTS || layout.scratchpads != 0 ||
        layout.dcbaa_offset != 0 ||
        layout.command_offset % PI4_XHCI_DMA_RING_ALLOC_BYTES ||
        layout.event_offset % PI4_XHCI_DMA_RING_ALLOC_BYTES ||
        layout.event_offset < layout.command_offset +
                             PI4_XHCI_DMA_RING_ALLOC_BYTES ||
        layout.erst_offset < layout.event_offset +
                             PI4_XHCI_DMA_RING_ALLOC_BYTES ||
        layout.erst_offset % 64 ||
        layout.command_offset != 0x2000 || layout.event_offset != 0x4000 ||
        layout.erst_offset != 0x6000 ||
        layout.input_context_offset != 0x7000 ||
        layout.output_context_offset != 0x8000 ||
        layout.ep0_ring_offset != 0xa000 ||
        layout.control_buffer_offset != 0xc000 ||
        layout.child_output_context_offset != 0xd000 ||
        layout.child_ep0_ring_offset != 0xe000 ||
        layout.child_bulk_in_ring_offset != 0x10000 ||
        layout.child_bulk_out_ring_offset != 0x12000 ||
        layout.used_bytes != 0x14000 ||
        layout.used_bytes > arena.size)
        return fail("safe two-slot DMA layout with independent child context and ring");
    {
        uint32_t one_slot_caps[6];
        for (unsigned i = 0; i < 6; i++) one_slot_caps[i] = caps[i];
        one_slot_caps[1] = (one_slot_caps[1] & ~0xffu) | 1u;
        if (pi4_xhci_dma_layout_make(&arena, one_slot_caps, 1, 0,
                PI4_XHCI_USBSTS_HALTED, PI4_XHCI_LEGACY_OS_OWNED,
                PI4_PCI_COMMAND_MEMORY, &layout) != PI4_XHCI_DMA_PLAN_CAPS)
            return fail("two-slot layout requires controller capacity for two slots");
        if (pi4_xhci_dma_layout_make(&arena, caps, 1, 0,
                PI4_XHCI_USBSTS_HALTED, PI4_XHCI_LEGACY_OS_OWNED,
                PI4_PCI_COMMAND_MEMORY, &layout) != PI4_XHCI_DMA_PLAN_OK)
            return fail("restore valid two-slot DMA layout after capacity test");
    }
    {
        const unsigned link = PI4_XHCI_DMA_RING_BYTES / 16u - 1u;
        const uint32_t *words = (const uint32_t *)command_ring;
        const unsigned start = PI4_XHCI_DMA_COMMAND_WRAP_START_TRB;
        const unsigned prelink = PI4_XHCI_DMA_COMMAND_WRAP_PRELINK_COUNT;
        if (PI4_XHCI_DMA_COMMAND_ENABLE_SLOT_TRB + 1u != start ||
            ((PI4_XHCI_DMA_COMMAND_ENABLE_SLOT_TRB * 16u) & 63u) != 0 ||
            pi4_xhci_dma_command_ring_seed(command_ring, arena.bus_base) != 0 ||
            words[3] != ((PI4_XHCI_TRB_TYPE_NOOP << 10) | 1u) ||
            ((uint64_t)words[link * 4u] |
                ((uint64_t)words[link * 4u + 1u] << 32)) != arena.bus_base ||
            words[link * 4u + 2u] != 0 ||
            words[link * 4u + 3u] !=
                ((PI4_XHCI_TRB_TYPE_LINK << 10) | (1u << 1) | 1u))
            return fail("aligned Enable Slot start and Link TRB ring seed");
        for (unsigned i = 0; i < prelink; i++) {
            unsigned word = (start + i) * 4u;
            if (words[word] || words[word + 1u] || words[word + 2u] ||
                words[word + 3u] != 0u)
                return fail("unused pre-Link command entries start invalid");
        }
        if (pi4_xhci_dma_command_ring_seed(command_ring, arena.bus_base + 1u) == 0)
            return fail("misaligned command ring DMA base accepted");
    }
    {
        uint32_t *in = (uint32_t *)input_context;
        uint32_t *slot = (uint32_t *)(input_context + 32);
        uint32_t *ep0 = (uint32_t *)(input_context + 64);
        uint32_t enable[4], address[4], stop[4], configure[4];
        uint32_t evaluate[4];
        _Alignas(16) uint32_t guard[4];
        uint32_t setup[4], data[4];
        uint32_t status_stage[4];
        uint32_t no_data_status_stage[4];
        uint32_t get_status_setup[4];
        uint32_t get_status_data[4];
        uint32_t get_status_stage[4];
        uint32_t get_configuration_setup[4];
        uint32_t get_configuration_data[4];
        uint32_t get_configuration_stage[4];
        uint32_t get_max_lun_setup[4];
        uint32_t config_full_setup[4];
        uint32_t config_full_data[4];
        uint32_t config_full_stage[4];
        uint32_t set_configuration_setup[4];
        const uint8_t descriptor_prefix[8] = {18u, 1u, 0u, 0u,
                                               0u, 0u, 0u, 64u};
        const uint8_t child_descriptor_prefix[8] = {18u, 1u, 0u, 2u,
                                                      0u, 0u, 0u, 8u};
        const uint8_t device_descriptor[18] = {
            18u, 1u, 0x10u, 0x02u, 9u, 0u, 1u, 64u,
            0x34u, 0x12u, 0x78u, 0x56u, 0u, 1u, 0u, 0u, 0u, 1u
        };
        const uint8_t config_header[9] = {
            9u, 2u, 25u, 0u, 1u, 1u, 0u, 0x80u, 50u
        };
        const uint8_t config_full[25] = {
            9u, 2u, 25u, 0u, 1u, 1u, 0u, 0x80u, 50u,
            9u, 4u, 0u, 0u, 1u, 8u, 6u, 0x50u, 0u,
            7u, 5u, 0x81u, 2u, 0x00u, 0x02u, 0u
        };
        const uint8_t config_bulk_pair[32] = {
            9u, 2u, 32u, 0u, 1u, 1u, 0u, 0x80u, 50u,
            9u, 4u, 0u, 0u, 2u, 8u, 6u, 0x50u, 0u,
            7u, 5u, 0x81u, 2u, 0x00u, 0x02u, 0u,
            7u, 5u, 0x02u, 2u, 0x00u, 0x02u, 0u
        };
        const uint8_t config_alt_settings[27] = {
            9u, 2u, 27u, 0u, 1u, 1u, 0u, 0x80u, 50u,
            9u, 4u, 0u, 0u, 0u, 8u, 6u, 0x50u, 0u,
            9u, 4u, 0u, 1u, 0u, 8u, 6u, 0x50u, 0u
        };
        const uint8_t hub_descriptor[9] = {
            9u, 0x29u, 4u, 0xe0u, 0u, 10u, 0u, 0x1eu, 0xffu
        };
        const uint64_t control_buffer_bus =
            arena.bus_base + layout.control_buffer_offset;
        const uint64_t status_trb_bus = arena.bus_base + 0xa000u + 32u;
        const uint32_t transfer_status = 1u << 24;
        const uint32_t transfer_control =
            (PI4_XHCI_TRB_TYPE_TRANSFER_EVT << 10) |
            (1u << 16) | (1u << 24) | 1u;
        unsigned proto_major = 0, slot_type = 0;
        uint32_t next_offset = 0;
        unsigned ctx_size = pi4_xhci_dma_context_size(caps[3]);
        if (ctx_size != 32u ||
            pi4_xhci_dma_address_context_seed(input_context,
                sizeof(input_context), ctx_size, 3, 1,
                arena.bus_base + 0xa000) != 0 ||
            in[0] != 0 || in[1] != 3u ||
            slot[0] != ((3u << 20) | (1u << 27)) ||
            slot[1] != (1u << 16) ||
            ep0[1] != ((3u << 1) | (4u << 3) | (64u << 16)) ||
            ((uint64_t)ep0[2] | ((uint64_t)ep0[3] << 32)) !=
                ((arena.bus_base + 0xa000) | 1u) ||
            ep0[4] != 8u)
            return fail("high-speed Address Device input context describes EP0 and root port");
        if (pi4_xhci_dma_ep0_default_mps(1) != 8u ||
            pi4_xhci_dma_ep0_default_mps(2) != 8u ||
            pi4_xhci_dma_ep0_default_mps(3) != 64u ||
            pi4_xhci_dma_ep0_default_mps(4) != 512u ||
            pi4_xhci_dma_ep0_default_mps(0) != 0u)
            return fail("EP0 default max packet size follows xHCI speed ID");
        if (pi4_xhci_dma_enable_slot_seed(enable, 0, 0) != 0 ||
            enable[0] || enable[1] || enable[2] ||
            enable[3] != (PI4_XHCI_TRB_TYPE_ENABLE_SLOT << 10) ||
            PI4_XHCI_DMA_COMMAND_ADDRESS_DEVICE_TRB !=
                PI4_XHCI_DMA_COMMAND_ENABLE_SLOT_TRB + 1u ||
            PI4_XHCI_DMA_COMMAND_ADDRESS_DEVICE_TRB >=
                PI4_XHCI_DMA_RING_BYTES / 16u - 1u ||
            pi4_xhci_dma_address_device_seed(address, arena.bus_base + 0x7000,
                1, 0, 0) != 0 ||
            ((uint64_t)address[0] | ((uint64_t)address[1] << 32)) !=
                arena.bus_base + 0x7000 || address[2] != 0 ||
            address[3] != ((PI4_XHCI_TRB_TYPE_ADDRESS_DEVICE << 10) |
                           (1u << 24)) ||
            pi4_xhci_dma_address_device_seed(address, arena.bus_base + 0x7000,
                1, 0, 1) != 0 ||
            address[3] != ((PI4_XHCI_TRB_TYPE_ADDRESS_DEVICE << 10) |
                           (1u << 24) | (1u << 9)) ||
            pi4_xhci_dma_address_device_seed(address, arena.bus_base + 0x7000,
                1, 1, 0) != 0 ||
            address[3] != ((PI4_XHCI_TRB_TYPE_ADDRESS_DEVICE << 10) |
                           (1u << 24) | 1u) ||
            pi4_xhci_dma_address_device_seed(address, arena.bus_base + 0x7000,
                1, 1, 1) != 0 ||
            address[3] != ((PI4_XHCI_TRB_TYPE_ADDRESS_DEVICE << 10) |
                           (1u << 24) | (1u << 9) | 1u) ||
            pi4_xhci_dma_address_device_seed(address, arena.bus_base + 0x7000,
                1, 0, 2) == 0)
            return fail("pre-Link Address Device placement and TRB fields");
        if (pi4_xhci_dma_protocol_cap_for_port(
                (2u << 24) | (0x10u << 8) |
                    PI4_XHCI_EXT_CAP_SUPPORTED_PROTOCOL,
                1u | (4u << 8), 0x15u, 3u, 8u,
                &proto_major, &slot_type) != 1 ||
            proto_major != 2u || slot_type != 0x15u ||
            pi4_xhci_dma_enable_slot_seed(enable, slot_type, 0) != 0 ||
            enable[3] != ((PI4_XHCI_TRB_TYPE_ENABLE_SLOT << 10) |
                          (0x15u << 16)) ||
            pi4_xhci_dma_enable_slot_seed(enable, slot_type, 1) != 0 ||
            enable[3] != ((PI4_XHCI_TRB_TYPE_ENABLE_SLOT << 10) |
                          (0x15u << 16) | 1u) ||
            PI4_XHCI_DMA_COMMAND_CHILD_ENABLE_SLOT_TRB !=
                PI4_XHCI_DMA_COMMAND_CONFIGURE_HUB_TRB + 1u ||
            PI4_XHCI_DMA_COMMAND_CHILD_ADDRESS_DEVICE_TRB !=
                PI4_XHCI_DMA_COMMAND_CHILD_ENABLE_SLOT_TRB + 1u ||
            PI4_XHCI_DMA_COMMAND_HUB_GUARD_NOOP_TRB !=
                PI4_XHCI_DMA_COMMAND_STOP_ENDPOINT_TRB ||
            PI4_XHCI_DMA_COMMAND_CONFIGURE_HUB_TRB !=
                PI4_XHCI_DMA_COMMAND_HUB_GUARD_NOOP_TRB + 1u ||
            PI4_XHCI_DMA_COMMAND_CHILD_BULK_GUARD_NOOP_TRB != 0u ||
            PI4_XHCI_DMA_COMMAND_CHILD_BULK_CONFIGURE_TRB != 1u ||
            PI4_XHCI_DMA_COMMAND_CHILD_ADDRESS_DEVICE_TRB >=
                PI4_XHCI_DMA_COMMAND_WRAP_START_TRB +
                    PI4_XHCI_DMA_COMMAND_WRAP_COUNT ||
            pi4_xhci_dma_ext_cap_next(0xa0u,
                (0x10u << 8) | PI4_XHCI_EXT_CAP_SUPPORTED_PROTOCOL,
                0x40000u, &next_offset) != 1 || next_offset != 0xe0u)
            return fail("port protocol capability selects its advertised slot type");
        if (pi4_xhci_dma_noop_seed(guard, 0u) != 0 ||
            guard[3] != (PI4_XHCI_TRB_TYPE_NOOP << 10) ||
            pi4_xhci_dma_configure_endpoint_seed(configure,
                arena.bus_base + layout.input_context_offset, 2u, 0u) != 0 ||
            configure[3] !=
                ((PI4_XHCI_TRB_TYPE_CONFIGURE_ENDPOINT << 10) | (2u << 24)))
            return fail("post-Link bulk endpoint commands use the wrapped producer cycle");
        if (pi4_xhci_dma_noop_seed(guard, 1u) != 0 ||
            guard[0] || guard[1] || guard[2] ||
            guard[3] != ((PI4_XHCI_TRB_TYPE_NOOP << 10) | 1u) ||
            pi4_xhci_dma_noop_seed(guard, 0u) != 0 ||
            guard[3] != (PI4_XHCI_TRB_TYPE_NOOP << 10) ||
            pi4_xhci_dma_noop_seed(guard, 2u) == 0)
            return fail("post-address command-ring guard No-op TRB fields");
        if (PI4_XHCI_DMA_COMMAND_STOP_ENDPOINT_TRB !=
                PI4_XHCI_DMA_COMMAND_ADDRESS_DEVICE_TRB + 1u ||
            PI4_XHCI_DMA_COMMAND_STOP_ENDPOINT_TRB >=
                PI4_XHCI_DMA_RING_BYTES / 16u - 1u ||
            pi4_xhci_dma_stop_endpoint_seed(stop, 1u, 1u, 1u) != 0 ||
            stop[0] || stop[1] || stop[2] ||
            stop[3] != ((PI4_XHCI_TRB_TYPE_STOP_ENDPOINT << 10) |
                        (1u << 16) | (1u << 24) | 1u) ||
            pi4_xhci_dma_stop_endpoint_seed(stop, 0u, 1u, 1u) == 0 ||
            pi4_xhci_dma_stop_endpoint_seed(stop, 256u, 1u, 1u) == 0 ||
            pi4_xhci_dma_stop_endpoint_seed(stop, 1u, 0u, 1u) == 0 ||
            pi4_xhci_dma_stop_endpoint_seed(stop, 1u, 32u, 1u) == 0 ||
            pi4_xhci_dma_stop_endpoint_seed(stop, 1u, 1u, 2u) == 0 ||
            pi4_xhci_dma_stop_endpoint_seed(stop + 1, 1u, 1u, 1u) == 0)
            return fail("Stop Endpoint placement, encoding, and field validation");
        if (pi4_xhci_dma_protocol_cap_for_port(
                (2u << 24) | PI4_XHCI_EXT_CAP_SUPPORTED_PROTOCOL,
                1u | (4u << 8), 0x15u, 6u, 8u,
                &proto_major, &slot_type) != 0 ||
            pi4_xhci_dma_protocol_cap_for_port(
                (2u << 24) | PI4_XHCI_EXT_CAP_SUPPORTED_PROTOCOL,
                1u | (4u << 8), 0x20u, 3u, 8u,
                &proto_major, &slot_type) != -1 ||
            pi4_xhci_dma_ext_cap_next(0x3fffcu,
                (1u << 8), 0x40000u, &next_offset) != -1 ||
            pi4_xhci_dma_enable_slot_seed(enable, 32u, 0) == 0)
            return fail("protocol lookup rejects unrelated and malformed capabilities");
        if (pi4_xhci_dma_address_context_seed(input_context,
                sizeof(input_context), 48, 3, 1, arena.bus_base) == 0 ||
            pi4_xhci_dma_address_context_seed(input_context,
                sizeof(input_context), ctx_size, 3, 0,
                arena.bus_base) == 0 ||
            pi4_xhci_dma_address_context_seed(input_context,
                sizeof(input_context), ctx_size, 3, 1,
                arena.bus_base + 1u) == 0)
            return fail("Address Device context rejects invalid size, port, and EP0 alignment");
        ((uint32_t *)(void *)output_context)[0] = (3u << 20) | (1u << 27);
        ((uint32_t *)(void *)output_context)[1] = 1u << 16;
        ((uint32_t *)(void *)output_context)[2] = 0u;
        if (pi4_xhci_dma_hub_context_seed(input_context,
                sizeof(input_context), ctx_size, output_context, 4u,
                0u, 0u) != 0 ||
            in[1] != 1u || slot[0] != ((3u << 20) | (1u << 27) | (1u << 26)) ||
            slot[1] != ((4u << 24) | (1u << 16)) || slot[2] != 0u ||
            pi4_xhci_dma_configure_endpoint_seed(configure,
                arena.bus_base + layout.input_context_offset, 1u, 1u) != 0 ||
            ((uint64_t)configure[0] | ((uint64_t)configure[1] << 32)) !=
                arena.bus_base + layout.input_context_offset ||
            configure[2] != 0u ||
            configure[3] != ((PI4_XHCI_TRB_TYPE_CONFIGURE_ENDPOINT << 10) |
                             (1u << 24) | 1u))
            return fail("Configure Endpoint seeds the addressed hub Slot Context");
        ((uint32_t *)(void *)output_context)[0] = slot[0];
        ((uint32_t *)(void *)output_context)[1] = slot[1];
        ((uint32_t *)(void *)output_context)[2] = 0u;
        if (!pi4_xhci_dma_output_hub_context_ok(output_context, ctx_size,
                4u, 0u, 0u) ||
            pi4_xhci_dma_hub_context_seed(input_context,
                sizeof(input_context), ctx_size, output_context, 4u,
                4u, 0u) == 0 ||
            pi4_xhci_dma_hub_context_seed(input_context,
                sizeof(input_context), ctx_size, input_context, 4u,
                1u, 0u) == 0)
            return fail("hub context validation checks speed and output fields");
        if (pi4_xhci_dma_hub_child_speed_id(0x103u) != 1u ||
            pi4_xhci_dma_hub_child_speed_id(0x303u) != 2u ||
            pi4_xhci_dma_hub_child_speed_id(0x503u) != 3u ||
            pi4_xhci_dma_hub_child_speed_id(0x102u) != 0u ||
            pi4_xhci_dma_hub_child_speed_id(0x703u) != 0u ||
            pi4_xhci_dma_route_string_append(0u, 0u, 3u) != 3u ||
            pi4_xhci_dma_route_string_append(3u, 1u, 2u) != 0x23u ||
            pi4_xhci_dma_route_string_append(0u, 0u, 0u) != UINT32_MAX ||
            pi4_xhci_dma_route_string_append(0x30u, 1u, 2u) != UINT32_MAX)
            return fail("hub child speed and route string decode");
        if (pi4_xhci_dma_child_address_context_seed(child_context,
                sizeof(child_context), ctx_size, 1u, 1u, 3u, 1u, 3u,
                3u, 0u, arena.bus_base + layout.child_ep0_ring_offset) != 0 ||
            ((uint32_t *)(void *)(child_context + ctx_size))[0] !=
                ((1u << 27) | (1u << 20) | 3u) ||
            ((uint32_t *)(void *)(child_context + ctx_size))[1] != (1u << 16) ||
            ((uint32_t *)(void *)(child_context + ctx_size))[2] != 0x301u ||
            ((uint32_t *)(void *)(child_context + ctx_size * 2u))[1] !=
                ((3u << 1) | (4u << 3) | (8u << 16)) ||
            pi4_xhci_dma_child_address_context_seed(child_context,
                sizeof(child_context), ctx_size, 3u, 1u, 3u, 1u, 3u,
                3u, 0u, arena.bus_base + layout.child_ep0_ring_offset) != 0 ||
            ((uint32_t *)(void *)(child_context + ctx_size))[2] != 0u ||
            pi4_xhci_dma_child_address_context_seed(child_context,
                sizeof(child_context), ctx_size, 1u, 1u, 3u, 0u, 0u,
                3u, 0u, arena.bus_base + layout.child_ep0_ring_offset) == 0)
            return fail("child Address Device context routes FS child through TT");
        {
            uint32_t *out_ep0 = (uint32_t *)(void *)(output_context +
                                      ctx_size);
            uint32_t *decoy_ep1 = (uint32_t *)(void *)(output_context +
                                      ctx_size * 2u);
            out_ep0[0] = 1u;
            out_ep0[1] = (3u << 1) | (4u << 3) | (8u << 16);
            out_ep0[2] = (uint32_t)((arena.bus_base + 0xe000u) | 1u);
            out_ep0[3] = (uint32_t)((arena.bus_base + 0xe000u) >> 32);
            out_ep0[4] = 8u;
            decoy_ep1[1] = 512u << 16; /* DCI 2 must not be mistaken for EP0. */
            if (pi4_xhci_dma_child_ep0_evaluate_context_seed(input_context,
                    sizeof(input_context), ctx_size, output_context,
                    sizeof(output_context), 1u, 64u) != 0 ||
                pi4_xhci_dma_output_ep0_context(output_context,
                    sizeof(output_context), ctx_size) != out_ep0 ||
                in[0] != 0u || in[1] != 2u || slot[0] || slot[1] ||
                ep0[0] != out_ep0[0] ||
                ep0[1] != ((3u << 1) | (4u << 3) | (64u << 16)) ||
                ep0[2] != out_ep0[2] || ep0[3] != out_ep0[3] ||
                ep0[4] != out_ep0[4] ||
                pi4_xhci_dma_child_ep0_evaluate_context_seed(input_context,
                    sizeof(input_context), ctx_size, output_context,
                    sizeof(output_context), 1u, 9u) == 0 ||
                pi4_xhci_dma_child_ep0_evaluate_context_seed(input_context,
                    sizeof(input_context), ctx_size, output_context,
                    sizeof(output_context), 3u, 64u) == 0 ||
                pi4_xhci_dma_child_ep0_evaluate_context_seed(input_context,
                    sizeof(input_context), ctx_size, output_context,
                    (uint64_t)ctx_size * 2u - 1u, 1u, 64u) == 0 ||
                pi4_xhci_dma_evaluate_context_seed(evaluate,
                    arena.bus_base + layout.input_context_offset, 2u, 1u) != 0 ||
                ((uint64_t)evaluate[0] | ((uint64_t)evaluate[1] << 32)) !=
                    arena.bus_base + layout.input_context_offset ||
                evaluate[2] != 0u || evaluate[3] !=
                    ((PI4_XHCI_TRB_TYPE_EVALUATE_CONTEXT << 10) |
                     (2u << 24) | 1u) ||
                PI4_XHCI_DMA_COMMAND_CHILD_EVALUATE_CONTEXT_TRB !=
                    PI4_XHCI_DMA_COMMAND_CHILD_ADDRESS_DEVICE_TRB + 1u ||
                PI4_XHCI_DMA_COMMAND_CHILD_ADDRESS_DEVICE_TRB !=
                    PI4_XHCI_DMA_RING_BYTES / 16u - 3u ||
                PI4_XHCI_DMA_COMMAND_CHILD_EVALUATE_CONTEXT_TRB !=
                    PI4_XHCI_DMA_RING_BYTES / 16u - 2u ||
                pi4_xhci_dma_evaluate_context_seed(evaluate,
                    arena.bus_base + 1u, 2u, 0u) == 0 ||
                pi4_xhci_dma_evaluate_context_seed(evaluate,
                    arena.bus_base, 0u, 0u) == 0 ||
                pi4_xhci_dma_evaluate_context_seed(evaluate,
                    arena.bus_base, 2u, 2u) == 0 ||
                pi4_xhci_dma_output_ep0_max_packet_size(out_ep0) != 8u ||
                pi4_xhci_dma_output_ep0_max_packet_size(decoy_ep1) != 512u ||
                pi4_xhci_dma_output_ep0_max_packet_size(ep0) != 64u)
                return fail("Evaluate Context updates only child EP0 MPS across command wrap");
        }
        if (PI4_XHCI_DMA_CHILD_DESCRIPTOR_PREFIX_SETUP_TRB != 0u ||
            PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_SETUP_TRB !=
                PI4_XHCI_DMA_CHILD_DESCRIPTOR_PREFIX_SETUP_TRB + 3u ||
            PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_SETUP_TRB !=
                PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_SETUP_TRB + 3u ||
            PI4_XHCI_DMA_CHILD_POST_EVALUATE_GET_STATUS_SETUP_TRB !=
                PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_SETUP_TRB + 3u ||
            PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_SETUP_TRB !=
                PI4_XHCI_DMA_CHILD_POST_EVALUATE_GET_STATUS_SETUP_TRB + 3u ||
            PI4_XHCI_DMA_CHILD_CONFIG_HEADER_SETUP_TRB !=
                PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_SETUP_TRB + 3u ||
            PI4_XHCI_DMA_CHILD_CONFIG_FULL_SETUP_TRB !=
                PI4_XHCI_DMA_CHILD_CONFIG_HEADER_SETUP_TRB + 3u ||
            PI4_XHCI_DMA_CHILD_CONFIG_FULL_SETUP_TRB + 2u >=
                PI4_XHCI_DMA_RING_BYTES / 16u ||
            PI4_XHCI_DMA_CHILD_SET_CONFIGURATION_SETUP_TRB !=
                PI4_XHCI_DMA_CHILD_CONFIG_FULL_SETUP_TRB + 3u ||
            PI4_XHCI_DMA_CHILD_GET_CONFIGURATION_SETUP_TRB !=
                PI4_XHCI_DMA_CHILD_SET_CONFIGURATION_SETUP_TRB + 2u ||
            PI4_XHCI_DMA_CHILD_BOT_GET_MAX_LUN_SETUP_TRB !=
                PI4_XHCI_DMA_CHILD_GET_CONFIGURATION_SETUP_TRB + 3u ||
            PI4_XHCI_DMA_CHILD_BOT_GET_MAX_LUN_SETUP_TRB + 2u >=
                PI4_XHCI_DMA_RING_BYTES / 16u)
            return fail("child EP0 descriptor, configuration, and BOT TDs use consecutive ring slots");
        if (pi4_xhci_dma_transfer_ring_seed(transfer_ring,
                arena.bus_base + 0xa000) != 0 ||
            ((uint64_t)((uint32_t *)transfer_ring)[(255u * 4u)] |
             ((uint64_t)((uint32_t *)transfer_ring)[(255u * 4u) + 1u] << 32)) !=
                arena.bus_base + 0xa000 ||
            ((uint32_t *)transfer_ring)[(255u * 4u) + 3u] !=
                ((PI4_XHCI_TRB_TYPE_LINK << 10) | (1u << 1) | 1u))
            return fail("EP0 transfer ring has a self-link with cycle toggle");
        if (pi4_xhci_dma_get_descriptor_setup_seed(setup, 1u, 0u, 18u, 1u) != 0 ||
            setup[0] != 0x01000680u || setup[1] != 0x00120000u ||
            setup[2] != 8u || setup[3] !=
                ((PI4_XHCI_TRB_TYPE_SETUP_STAGE << 10) | (3u << 16) |
                 (1u << 6) | 1u) ||
            pi4_xhci_dma_get_descriptor_setup_seed(setup, 2u, 0u, 9u, 1u) != 0 ||
            setup[0] != 0x02000680u || setup[1] != 0x00090000u ||
            setup[2] != 8u ||
            pi4_xhci_dma_get_descriptor_setup_seed(config_full_setup,
                2u, 0u, sizeof(config_full), 1u) != 0 ||
            config_full_setup[0] != 0x02000680u ||
            config_full_setup[1] != ((uint32_t)sizeof(config_full) << 16) ||
            pi4_xhci_dma_data_stage_seed(config_full_data,
                control_buffer_bus + 1280u, sizeof(config_full), 1u) != 0 ||
            config_full_data[2] != sizeof(config_full) ||
            pi4_xhci_dma_status_stage_seed(config_full_stage, 1u) != 0 ||
            pi4_xhci_dma_get_descriptor_setup_seed(setup, 1u, 0u, 8u, 1u) != 0 ||
            setup[0] != 0x01000680u || setup[1] != 0x00080000u ||
            setup[2] != 8u || setup[3] !=
                ((PI4_XHCI_TRB_TYPE_SETUP_STAGE << 10) | (3u << 16) |
                 (1u << 6) | 1u) ||
            pi4_xhci_dma_setup_stage_seed(setup, 0x00u, 0x09u, 1u,
                0u, 0u, 0u, 1u) != 0 ||
            setup[0] != 0x00010900u || setup[1] != 0u || setup[2] != 8u ||
            setup[3] != ((PI4_XHCI_TRB_TYPE_SETUP_STAGE << 10) |
                         (1u << 6) | 1u) ||
            pi4_xhci_dma_set_configuration_setup_seed(
                set_configuration_setup, 1u, 1u) != 0 ||
            set_configuration_setup[0] != 0x00010900u ||
            set_configuration_setup[1] != 0u ||
            set_configuration_setup[2] != 8u ||
            set_configuration_setup[3] !=
                ((PI4_XHCI_TRB_TYPE_SETUP_STAGE << 10) | (1u << 6) | 1u) ||
            pi4_xhci_dma_set_configuration_setup_seed(
                set_configuration_setup, 0x10000u, 1u) == 0 ||
            pi4_xhci_dma_setup_stage_seed(setup, 0xa0u, 0x06u, 0x2900u,
                0u, 9u, 3u, 1u) != 0 ||
            setup[0] != 0x290006a0u || setup[1] != 0x00090000u ||
            setup[2] != 8u || setup[3] !=
                ((PI4_XHCI_TRB_TYPE_SETUP_STAGE << 10) | (3u << 16) |
                 (1u << 6) | 1u) ||
            pi4_xhci_dma_setup_stage_seed(setup, 0x23u, 0x03u,
                PI4_XHCI_HUB_FEATURE_PORT_POWER, 4u, 0u, 0u, 1u) != 0 ||
            setup[0] != 0x00080323u || setup[1] != 0x00000004u ||
            setup[2] != 8u || setup[3] !=
                ((PI4_XHCI_TRB_TYPE_SETUP_STAGE << 10) |
                 (1u << 6) | 1u) ||
            pi4_xhci_dma_setup_stage_seed(setup, 0xa3u, 0x00u,
                0u, 4u, 4u, 3u, 1u) != 0 ||
            setup[0] != 0x000000a3u || setup[1] != 0x00040004u ||
            setup[2] != 8u || setup[3] !=
                ((PI4_XHCI_TRB_TYPE_SETUP_STAGE << 10) | (3u << 16) |
                 (1u << 6) | 1u) ||
            pi4_xhci_dma_setup_stage_seed(setup, 0x00u, 0x09u, 1u,
                0u, 0u, 3u, 1u) == 0 ||
            pi4_xhci_dma_get_status_setup_seed(get_status_setup, 1u) != 0 ||
            get_status_setup[0] != 0x00000080u ||
            get_status_setup[1] != 0x00020000u ||
            get_status_setup[2] != 8u || get_status_setup[3] !=
                ((PI4_XHCI_TRB_TYPE_SETUP_STAGE << 10) | (3u << 16) |
                 (1u << 6) | 1u) ||
            pi4_xhci_dma_data_stage_seed(get_status_data,
                control_buffer_bus + 1152u, 2u, 1u) != 0 ||
            ((uint64_t)get_status_data[0] |
             ((uint64_t)get_status_data[1] << 32)) !=
                control_buffer_bus + 1152u ||
            get_status_data[2] != 2u || get_status_data[3] !=
                ((PI4_XHCI_TRB_TYPE_DATA_STAGE << 10) | (1u << 16) |
                 1u) ||
            pi4_xhci_dma_status_stage_seed_direction(
                get_status_stage, 0u, 1u) != 0 ||
            get_status_stage[0] || get_status_stage[1] ||
            get_status_stage[2] || get_status_stage[3] !=
                ((PI4_XHCI_TRB_TYPE_STATUS_STAGE << 10) | (1u << 5) | 1u) ||
            pi4_xhci_dma_get_configuration_setup_seed(
                get_configuration_setup, 1u) != 0 ||
            get_configuration_setup[0] != 0x00000880u ||
            get_configuration_setup[1] != 0x00010000u ||
            get_configuration_setup[2] != 8u ||
            get_configuration_setup[3] !=
                ((PI4_XHCI_TRB_TYPE_SETUP_STAGE << 10) | (3u << 16) |
                 (1u << 6) | 1u) ||
            pi4_xhci_dma_data_stage_seed(get_configuration_data,
                control_buffer_bus + 1152u, 1u, 1u) != 0 ||
            ((uint64_t)get_configuration_data[0] |
             ((uint64_t)get_configuration_data[1] << 32)) !=
                control_buffer_bus + 1152u ||
            get_configuration_data[2] != 1u ||
            get_configuration_data[3] !=
                ((PI4_XHCI_TRB_TYPE_DATA_STAGE << 10) | (1u << 16) |
                 1u) ||
            pi4_xhci_dma_status_stage_seed_direction(
                get_configuration_stage, 0u, 1u) != 0 ||
            get_configuration_stage[0] || get_configuration_stage[1] ||
            get_configuration_stage[2] || get_configuration_stage[3] !=
                ((PI4_XHCI_TRB_TYPE_STATUS_STAGE << 10) | (1u << 5) | 1u) ||
            pi4_xhci_dma_bot_get_max_lun_setup_seed(
                get_max_lun_setup, 3u, 1u) != 0 ||
            get_max_lun_setup[0] != 0x0000fea1u ||
            get_max_lun_setup[1] != 0x00010003u ||
            get_max_lun_setup[2] != 8u || get_max_lun_setup[3] !=
                ((PI4_XHCI_TRB_TYPE_SETUP_STAGE << 10) | (3u << 16) |
                 (1u << 6) | 1u) ||
            pi4_xhci_dma_bot_get_max_lun_setup_seed(
                get_max_lun_setup, 0x10000u, 1u) == 0 ||
            pi4_xhci_dma_data_stage_seed(data, control_buffer_bus, 18u, 1) != 0 ||
            ((uint64_t)data[0] | ((uint64_t)data[1] << 32)) !=
                control_buffer_bus || data[2] != 18u || data[3] !=
                ((PI4_XHCI_TRB_TYPE_DATA_STAGE << 10) | (1u << 16) | 1u) ||
            pi4_xhci_dma_data_stage_seed(data, control_buffer_bus + 64u,
                9u, 1u) != 0 || data[2] != 9u ||
            pi4_xhci_dma_data_stage_seed(data,
                control_buffer_bus + 1024u, 8u, 1u) != 0 || data[2] != 8u ||
            pi4_xhci_dma_status_stage_seed(status_stage, 1) != 0 ||
            status_stage[0] || status_stage[1] || status_stage[2] ||
            status_stage[3] !=
                ((PI4_XHCI_TRB_TYPE_STATUS_STAGE << 10) | (1u << 5) | 1u) ||
            pi4_xhci_dma_status_stage_seed_direction(
                no_data_status_stage, 1u, 1u) != 0 ||
            no_data_status_stage[0] || no_data_status_stage[1] ||
            no_data_status_stage[2] || no_data_status_stage[3] !=
                ((PI4_XHCI_TRB_TYPE_STATUS_STAGE << 10) |
                 (1u << 16) | (1u << 5) | 1u))
            return fail("EP0 standard control setup/data/status TRBs");
        if (!pi4_xhci_dma_bulk_max_packet_valid(1u, 64u) ||
            !pi4_xhci_dma_bulk_max_packet_valid(1u, 8u) ||
            pi4_xhci_dma_bulk_max_packet_valid(1u, 512u) ||
            pi4_xhci_dma_bulk_max_packet_valid(1u, 65u) ||
            !pi4_xhci_dma_bulk_max_packet_valid(3u, 512u) ||
            pi4_xhci_dma_bulk_max_packet_valid(3u, 64u) ||
            pi4_xhci_dma_bulk_max_packet_valid(2u, 64u))
            return fail("BOT bulk maximum packet size is compatible with child link speed");
        if (!pi4_xhci_dma_transfer_event_ok(status_trb_bus, transfer_status,
                transfer_control, status_trb_bus, 1u, 1u) ||
            !pi4_xhci_dma_transfer_event_ok(status_trb_bus, transfer_status,
                transfer_control & ~1u, status_trb_bus, 1u, 1u) ||
            pi4_xhci_dma_transfer_event_ok(status_trb_bus + 16u,
                transfer_status, transfer_control, status_trb_bus, 1u, 1u) ||
            pi4_xhci_dma_transfer_event_ok(status_trb_bus, transfer_status,
                transfer_control & ~(1u << 24), status_trb_bus, 1u, 1u) ||
            pi4_xhci_dma_transfer_event_check_mask(status_trb_bus,
                transfer_status, transfer_control, status_trb_bus, 1u, 1u) !=
                PI4_XHCI_DMA_TRANSFER_CHECK_ALL ||
            (pi4_xhci_dma_transfer_event_check_mask(status_trb_bus + 16u,
                transfer_status, transfer_control, status_trb_bus, 1u, 1u) &
                PI4_XHCI_DMA_TRANSFER_CHECK_POINTER) ||
            (pi4_xhci_dma_transfer_event_check_mask(status_trb_bus,
                transfer_status, transfer_control & ~(1u << 16),
                status_trb_bus, 1u, 1u) & PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT) ||
            (pi4_xhci_dma_transfer_event_check_mask(status_trb_bus,
                transfer_status & ~(1u << 24), transfer_control,
                status_trb_bus, 1u, 1u) & PI4_XHCI_DMA_TRANSFER_CHECK_SUCCESS) ||
            !pi4_xhci_dma_device_descriptor_prefix_ok(
                descriptor_prefix, sizeof(descriptor_prefix)) ||
            pi4_xhci_dma_device_descriptor_prefix_ok(
                descriptor_prefix, 7u) ||
            !pi4_xhci_dma_child_device_descriptor_prefix_ok(
                child_descriptor_prefix, sizeof(child_descriptor_prefix), 1u) ||
            pi4_xhci_dma_child_device_descriptor_prefix_ok(
                child_descriptor_prefix, 7u, 1u) ||
            pi4_xhci_dma_child_device_descriptor_prefix_ok(
                child_descriptor_prefix, sizeof(child_descriptor_prefix), 0u) ||
            pi4_xhci_dma_child_device_descriptor_prefix_ok(
                child_descriptor_prefix, sizeof(child_descriptor_prefix), 3u) ||
            pi4_xhci_dma_child_device_descriptor_prefix_ok(
                child_descriptor_prefix, sizeof(child_descriptor_prefix), 4u) ||
            !pi4_xhci_dma_device_descriptor_ok(device_descriptor,
                sizeof(device_descriptor)) ||
            pi4_xhci_dma_device_descriptor_ok(device_descriptor, 17u) ||
            !pi4_xhci_dma_child_device_descriptor_ok(device_descriptor,
                sizeof(device_descriptor), 1u) ||
            pi4_xhci_dma_child_device_descriptor_ok(device_descriptor,
                17u, 1u) ||
            !pi4_xhci_dma_configuration_header_ok(config_header,
                sizeof(config_header)) ||
            pi4_xhci_dma_configuration_header_ok(config_header, 8u))
            return fail("EP0 Transfer Events and root/child descriptor validation");
        {
            uint8_t child_prefix[8];
            for (unsigned i = 0; i < sizeof(child_prefix); i++)
                child_prefix[i] = child_descriptor_prefix[i];
            child_prefix[7] = 16u;
            if (!pi4_xhci_dma_child_device_descriptor_prefix_ok(
                    child_prefix, sizeof(child_prefix), 1u))
                return fail("Full-Speed child descriptor accepts 16-byte EP0");
            child_prefix[7] = 32u;
            if (!pi4_xhci_dma_child_device_descriptor_prefix_ok(
                    child_prefix, sizeof(child_prefix), 1u))
                return fail("Full-Speed child descriptor accepts 32-byte EP0");
            child_prefix[7] = 64u;
            if (!pi4_xhci_dma_child_device_descriptor_prefix_ok(
                    child_prefix, sizeof(child_prefix), 1u))
                return fail("Full-Speed child descriptor accepts 64-byte EP0");
            child_prefix[7] = 9u;
            if (pi4_xhci_dma_child_device_descriptor_prefix_ok(
                    child_prefix, sizeof(child_prefix), 1u))
                return fail("Full-Speed child descriptor rejects nonstandard EP0");
            child_prefix[7] = 8u;
            if (!pi4_xhci_dma_child_device_descriptor_prefix_ok(
                    child_prefix, sizeof(child_prefix), 2u))
                return fail("Low-Speed child descriptor accepts 8-byte EP0");
            child_prefix[7] = 64u;
            if (!pi4_xhci_dma_child_device_descriptor_prefix_ok(
                    child_prefix, sizeof(child_prefix), 3u))
                return fail("High-Speed child descriptor accepts 64-byte EP0");
            child_prefix[7] = 9u;
            if (!pi4_xhci_dma_child_device_descriptor_prefix_ok(
                    child_prefix, sizeof(child_prefix), 4u))
                return fail("SuperSpeed child descriptor accepts exponent MPS");
        }
        if (!pi4_xhci_dma_hub_descriptor_ok(hub_descriptor,
                sizeof(hub_descriptor)))
            return fail("USB hub descriptor validates port-derived bitmap length");
        {
            struct pi4_xhci_dma_config_summary summary;
            uint8_t invalid_config[sizeof(config_full)];
            if (!pi4_xhci_dma_configuration_descriptor_ok(config_full,
                    sizeof(config_full), sizeof(config_full), &summary) ||
                summary.total_length != sizeof(config_full) ||
                summary.configuration_value != 1u ||
                summary.interface_descriptors != 1u ||
                summary.unique_interfaces != 1u ||
                summary.endpoint_descriptors != 1u ||
                summary.first_interface_number != 0u ||
                summary.first_interface_class != 8u ||
                summary.first_interface_subclass != 6u ||
                summary.first_interface_protocol != 0x50u ||
                summary.first_interface_endpoint_count != 1u ||
                summary.first_endpoint_address != 0x81u ||
                summary.first_endpoint_attributes != 2u ||
                summary.first_endpoint_max_packet != 512u ||
                summary.first_endpoint_interval != 0u)
                return fail("full configuration descriptor parses interface and endpoint summary");
            for (unsigned i = 0; i < sizeof(config_full); i++)
                invalid_config[i] = config_full[i];
            if (pi4_xhci_dma_configuration_descriptor_ok(invalid_config,
                    sizeof(config_full) - 1u, sizeof(config_full), &summary))
                return fail("full configuration length must match wTotalLength");
            invalid_config[9] = 8u;
            if (pi4_xhci_dma_configuration_descriptor_ok(invalid_config,
                    sizeof(config_full), sizeof(config_full), &summary))
                return fail("interface descriptor length is validated");
            for (unsigned i = 0; i < sizeof(config_full); i++)
                invalid_config[i] = config_full[i];
            invalid_config[13] = 0u;
            if (pi4_xhci_dma_configuration_descriptor_ok(invalid_config,
                    sizeof(config_full), sizeof(config_full), &summary))
                return fail("interface endpoint count must match its descriptors");
            if (pi4_xhci_dma_configuration_descriptor_ok(config_full,
                    sizeof(config_full), 24u, &summary))
                return fail("configuration descriptor respects DMA buffer capacity");
            if (!pi4_xhci_dma_configuration_descriptor_ok(config_alt_settings,
                    sizeof(config_alt_settings), sizeof(config_alt_settings),
                    &summary) || summary.interface_descriptors != 2u ||
                summary.unique_interfaces != 1u ||
                summary.endpoint_descriptors != 0u)
                return fail("configuration descriptor accepts alternate settings for one interface");
            if (!pi4_xhci_dma_configuration_descriptor_ok(config_bulk_pair,
                    sizeof(config_bulk_pair), sizeof(config_bulk_pair),
                    &summary) || !summary.bot_interface_found ||
                summary.bot_interface_number != 0u ||
                summary.bot_interface_alt != 0u ||
                summary.bot_interface_endpoint_count != 2u ||
                summary.bot_bulk_in_address != 0x81u ||
                summary.bot_bulk_in_attributes != 2u ||
                summary.bot_bulk_in_max_packet != 512u ||
                summary.bot_bulk_out_address != 0x02u ||
                summary.bot_bulk_out_attributes != 2u ||
                summary.bot_bulk_out_max_packet != 512u)
                return fail("BOT interface captures one bulk IN and OUT endpoint pair");
            {
                uint8_t malformed_bulk[sizeof(config_bulk_pair)];
                for (unsigned i = 0; i < sizeof(config_bulk_pair); i++)
                    malformed_bulk[i] = config_bulk_pair[i];
                malformed_bulk[23] |= 0x08u;
                if (pi4_xhci_dma_configuration_descriptor_ok(malformed_bulk,
                        sizeof(malformed_bulk), sizeof(malformed_bulk),
                        &summary))
                    return fail("BOT bulk endpoint rejects nonzero high-bandwidth bits");
            }
        }
        {
            const unsigned context_size = 32u;
            const uint64_t bulk_in_bus = arena.bus_base + 0x10000u;
            const uint64_t bulk_out_bus = arena.bus_base + 0x12000u;
            uint32_t *out_slot = (uint32_t *)(void *)output_context;
            uint8_t *in_ep_context = input_context + 4u * context_size;
            uint8_t *out_ep_context = input_context + 5u * context_size;
            uint8_t *out_in_ep = output_context + 3u * context_size;
            uint8_t *out_out_ep = output_context + 4u * context_size;
            uint32_t *in_words = (uint32_t *)(void *)in_ep_context;
            uint32_t *out_words = (uint32_t *)(void *)out_ep_context;
            uint32_t *output_in_words = (uint32_t *)(void *)out_in_ep;
            uint32_t *output_out_words = (uint32_t *)(void *)out_out_ep;
            out_slot[0] = 1u << 27;
            out_slot[1] = 1u << 16;
            out_slot[3] = 2u << 27; /* Addressed slot. */
            if (pi4_xhci_dma_bulk_contexts_seed(input_context,
                    sizeof(input_context), context_size, output_context,
                    sizeof(output_context), 8u, 6u, 0x50u, 0u, 2u,
                    0x81u, 2u, 512u, 0u, 0x02u, 2u, 512u, 0u, 3u,
                    bulk_in_bus, bulk_out_bus) != 0 ||
                ((uint32_t *)(void *)input_context)[1] !=
                    (1u | (1u << 3) | (1u << 4)) ||
                (((uint32_t *)(void *)(input_context + context_size))[0] >> 27) != 4u ||
                ((in_words[1] >> 3) & 7u) != 6u ||
                ((out_words[1] >> 3) & 7u) != 2u ||
                ((in_words[1] >> 16) & 0xffffu) != 512u ||
                ((out_words[1] >> 16) & 0xffffu) != 512u ||
                (((uint64_t)in_words[2] | ((uint64_t)in_words[3] << 32)) & ~1ull) !=
                    bulk_in_bus || (in_words[2] & 1u) != 1u ||
                (((uint64_t)out_words[2] | ((uint64_t)out_words[3] << 32)) & ~1ull) !=
                    bulk_out_bus || (out_words[2] & 1u) != 1u ||
                in_words[4] != PI4_XHCI_DMA_BULK_AVG_TRB_LENGTH ||
                out_words[4] != PI4_XHCI_DMA_BULK_AVG_TRB_LENGTH)
                return fail("bulk contexts seed DCI flags, packet sizes, rings, and average length");
            for (unsigned i = 0; i < context_size / sizeof(uint32_t); i++) {
                output_in_words[i] = in_words[i];
                output_out_words[i] = out_words[i];
            }
            output_in_words[0] = (output_in_words[0] & ~7u) | 1u;
            output_out_words[0] = (output_out_words[0] & ~7u) | 1u;
            out_slot[0] = (out_slot[0] & ~(0x1fu << 27)) | (4u << 27);
            out_slot[3] = (out_slot[3] & ~(3u << 27)) | (3u << 27);
            if (!pi4_xhci_dma_bulk_contexts_ok(output_context,
                    sizeof(output_context), context_size, 0x81u, 512u, 0u,
                    bulk_in_bus, 0x02u, 512u, 0u, bulk_out_bus) ||
                pi4_xhci_dma_bulk_contexts_ok(output_context,
                    sizeof(output_context), context_size, 0x81u, 64u, 0u,
                    bulk_in_bus, 0x02u, 512u, 0u, bulk_out_bus))
                return fail("configured bulk Output Contexts verify and reject wrong packet size");
            if (pi4_xhci_dma_bulk_contexts_seed(input_context,
                    sizeof(input_context), context_size, output_context,
                    sizeof(output_context), 8u, 6u, 0x50u, 0u, 2u,
                    0x81u, 2u, 64u, 0u, 0x02u, 2u, 512u, 0u, 3u,
                    bulk_in_bus, bulk_out_bus) == 0 ||
                pi4_xhci_dma_bulk_contexts_seed(input_context,
                    sizeof(input_context), context_size, output_context,
                    sizeof(output_context), 8u, 6u, 0x50u, 0u, 2u,
                    0x81u, 2u, 512u, 0u, 0x02u, 2u, 512u, 0u, 2u,
                    bulk_in_bus, bulk_out_bus) == 0)
                return fail("bulk context seeding rejects speed-incompatible MPS and unsupported speed");
        }
        {
            uint8_t invalid_config[9];
            for (unsigned i = 0; i < sizeof(config_header); i++)
                invalid_config[i] = config_header[i];
            invalid_config[1] = 1u;
            if (pi4_xhci_dma_configuration_header_ok(invalid_config,
                    sizeof(invalid_config)))
                return fail("configuration header descriptor type is validated");
            invalid_config[1] = 2u;
            invalid_config[2] = 8u;
            if (pi4_xhci_dma_configuration_header_ok(invalid_config,
                    sizeof(invalid_config)))
                return fail("configuration total length cannot be shorter than header");
            invalid_config[2] = 25u;
            invalid_config[4] = 0u;
            if (pi4_xhci_dma_configuration_header_ok(invalid_config,
                    sizeof(invalid_config)))
                return fail("configuration header requires an interface");
            invalid_config[4] = 1u;
            invalid_config[7] = 0u;
            if (pi4_xhci_dma_configuration_header_ok(invalid_config,
                    sizeof(invalid_config)))
                return fail("configuration attributes reserved bit is required");
        }
        if (pi4_xhci_dma_hub_power_request_count(0x0000u, 4u) != 1u ||
            pi4_xhci_dma_hub_power_request_count(0x0009u, 4u) != 4u ||
            pi4_xhci_dma_hub_power_request_count(0x0002u, 4u) != 0u ||
            pi4_xhci_dma_hub_power_request_count(0x0003u, 4u) != 0u ||
            pi4_xhci_dma_hub_power_probe_request_count(0x0000u, 4u) != 4u ||
            pi4_xhci_dma_hub_power_probe_request_count(0x0001u, 4u) != 4u ||
            pi4_xhci_dma_hub_power_probe_request_count(0x0002u, 4u) != 0u ||
            pi4_xhci_dma_hub_power_request_count(0x0000u, 0u) != 0u ||
            pi4_xhci_dma_hub_power_request_count(0x0001u, 32u) != 0u)
            return fail("USB hub declared mode and diagnostic per-port power sweep are safe");
        if (pi4_xhci_dma_hub_port_status_trb_index(1u, 0u) != 13u ||
            pi4_xhci_dma_hub_port_status_trb_index(1u, 3u) != 22u ||
            pi4_xhci_dma_hub_port_status_trb_index(2u, 0u) != 15u ||
            pi4_xhci_dma_hub_port_status_trb_index(2u, 3u) != 24u ||
            pi4_xhci_dma_hub_port_status_trb_index(4u, 0u) != 19u ||
            pi4_xhci_dma_hub_port_status_trb_index(4u, 3u) != 28u ||
            pi4_xhci_dma_hub_port_status_trb_index(0u, 0u) != UINT32_MAX ||
            pi4_xhci_dma_hub_port_status_trb_index(1u, 4u) != UINT32_MAX)
            return fail("USB hub GET_STATUS TDs continue at EP0 dequeue without cycle gaps");
        {
            struct pi4_xhci_dma_event_cursor cursor = {
                PI4_XHCI_DMA_EVENT_TRBS - 1u, 1u
            };
            if (!pi4_xhci_dma_event_ready(1u, &cursor) ||
                pi4_xhci_dma_event_ready(0u, &cursor) ||
                pi4_xhci_dma_event_dequeue(0x1000u, &cursor) != 0x1ff8u ||
                pi4_xhci_dma_event_cursor_advance(&cursor) != 0 ||
                cursor.index != 0u || cursor.cycle != 0u ||
                pi4_xhci_dma_event_ready(1u, &cursor) ||
                !pi4_xhci_dma_event_ready(0u, &cursor) ||
                pi4_xhci_dma_event_dequeue(0x1000u, &cursor) != 0x1008u)
                return fail("event-ring consumer wraps and toggles cycle state");
        }
        {
            const uint8_t powered_port_status[4] = {0x01u, 0x01u, 0u, 0u};
            const uint8_t unpowered_port_status[4] = {0x01u, 0u, 0u, 0u};
            if (!pi4_xhci_dma_hub_port_status_powered(powered_port_status,
                    sizeof(powered_port_status)) ||
                pi4_xhci_dma_hub_port_status_powered(unpowered_port_status,
                    sizeof(unpowered_port_status)) ||
                pi4_xhci_dma_hub_port_status_powered(powered_port_status, 3u))
                return fail("USB hub port status requires the power bit and full status payload");
        }

        {
            const uint32_t status[4] = { 0x0100u, 0x0000u,
                                         0x0100u, 0x0000u };
            if (pi4_xhci_dma_hub_port_unpowered_mask(status, 0x0fu) !=
                    0x0au ||
                pi4_xhci_dma_hub_port_unpowered_mask(status, 0x05u) != 0u ||
                pi4_xhci_dma_hub_port_unpowered_mask(NULL, 0x0fu) !=
                    UINT32_MAX ||
                pi4_xhci_dma_hub_port_powered_mask(status, 0x0fu) !=
                    0x05u ||
                pi4_xhci_dma_hub_port_powered_mask(status, 0x01u) !=
                    0x01u ||
                pi4_xhci_dma_hub_port_powered_mask(NULL, 0x0fu) !=
                    UINT32_MAX)
                return fail("USB hub powered/unpowered masks honor valid status events");
        }
        {
            const uint32_t status[4] = { 0x0101u, 0x0100u,
                                         0x0001u, 0x0000u };
            if (pi4_xhci_dma_hub_port_connected_mask(status, 0x0fu) !=
                    0x05u ||
                pi4_xhci_dma_hub_port_connected_mask(status, 0x02u) !=
                    0u ||
                pi4_xhci_dma_hub_port_connected_mask(NULL, 0x0fu) !=
                    UINT32_MAX)
                return fail("USB hub connected-port mask honors status event validity");
        }
        if (pi4_xhci_dma_hub_first_connected_port(0x04u, 4u) != 3u ||
            pi4_xhci_dma_hub_first_connected_port(0x0fu, 4u) != 1u ||
            pi4_xhci_dma_hub_first_connected_port(0u, 4u) != 0u ||
            pi4_xhci_dma_hub_first_connected_port(0x01u, 0u) != 0u ||
            pi4_xhci_dma_hub_first_connected_port(0x01u, 32u) != 0u)
            return fail("USB hub child reset selects the first bounded connected port");
        {
            const uint32_t status[4] = { 0x0101u, 0x0100u,
                                         0x0101u, 0x0001u };
            if (!pi4_xhci_dma_hub_child_port_reset_allowed(status, 0x05u, 3u) ||
                pi4_xhci_dma_hub_child_port_reset_allowed(status, 0x01u, 3u) ||
                pi4_xhci_dma_hub_child_port_reset_allowed(status, 0x0fu, 2u) ||
                pi4_xhci_dma_hub_child_port_reset_allowed(status, 0x0fu, 4u) ||
                pi4_xhci_dma_hub_child_port_reset_allowed(status, 0x0fu, 0u) ||
                pi4_xhci_dma_hub_child_port_reset_allowed(NULL, 0x0fu, 3u))
                return fail("USB hub child reset requires valid connected powered port status");
        }
        if (!pi4_xhci_dma_hub_port_reset_complete(0x0103u, 0x0010u) ||
            pi4_xhci_dma_hub_port_reset_complete(0x0113u, 0x0010u) ||
            pi4_xhci_dma_hub_port_reset_complete(0x0103u, 0u) ||
            !pi4_xhci_dma_hub_port_reset_change_cleared(0x0103u, 0u) ||
            pi4_xhci_dma_hub_port_reset_change_cleared(0x0113u, 0u) ||
            pi4_xhci_dma_hub_port_reset_change_cleared(0x0101u, 0u) ||
            pi4_xhci_dma_hub_port_reset_change_cleared(0x0103u, 0x0010u))
            return fail("USB hub child reset and C_PORT_RESET completion checks");
        {
            uint8_t invalid_hub[9];
            for (unsigned i = 0; i < sizeof(hub_descriptor); i++)
                invalid_hub[i] = hub_descriptor[i];
            invalid_hub[1] = 2u;
            if (pi4_xhci_dma_hub_descriptor_ok(invalid_hub,
                    sizeof(invalid_hub)))
                return fail("hub descriptor type is validated");
            invalid_hub[1] = 0x29u;
            invalid_hub[2] = 0u;
            if (pi4_xhci_dma_hub_descriptor_ok(invalid_hub,
                    sizeof(invalid_hub)))
                return fail("hub descriptor rejects zero ports");
            invalid_hub[2] = 4u;
            invalid_hub[0] = 8u;
            if (pi4_xhci_dma_hub_descriptor_ok(invalid_hub,
                    sizeof(invalid_hub)))
                return fail("hub descriptor rejects short bitmap payload");
        }
        ((uint32_t *)output_context)[3] = (2u << 27) | 5u;
        if (pi4_xhci_dma_output_usb_address(output_context) != 5u ||
            pi4_xhci_dma_output_slot_state(output_context) != 2u)
            return fail("Address Device output slot state and USB address decode");
    }
    if (pi4_xhci_dma_layout_make(&arena, caps, 0, 0,
            PI4_XHCI_USBSTS_HALTED, PI4_XHCI_LEGACY_OS_OWNED,
            PI4_PCI_COMMAND_MEMORY, &layout) != PI4_XHCI_DMA_PLAN_PAGES)
        return fail("4K page support required");
    if (pi4_xhci_dma_layout_make(&arena, caps, 0xdeaddeadu, 0,
            PI4_XHCI_USBSTS_HALTED, PI4_XHCI_LEGACY_OS_OWNED,
            PI4_PCI_COMMAND_MEMORY, &layout) != PI4_XHCI_DMA_PLAN_PAGES)
        return fail("reserved xHCI page-size bits rejected");

    if (pi4_xhci_dma_layout_make(&arena, caps, 1, PI4_XHCI_USBCMD_RUN,
            PI4_XHCI_USBSTS_HALTED, PI4_XHCI_LEGACY_OS_OWNED,
            PI4_PCI_COMMAND_MEMORY, &layout) != PI4_XHCI_DMA_PLAN_GATE ||
        pi4_xhci_dma_layout_make(&arena, caps, 1, 0,
            PI4_XHCI_USBSTS_HALTED, PI4_XHCI_LEGACY_OS_OWNED,
            PI4_PCI_COMMAND_MEMORY | PI4_PCI_COMMAND_MASTER,
            &layout) != PI4_XHCI_DMA_PLAN_GATE)
        return fail("running controller or active BME passed gate");

    uint32_t scratch_caps[6];
    for (unsigned i = 0; i < 6; i++) scratch_caps[i] = caps[i];
    scratch_caps[2] = 2u << 27;
    if (pi4_xhci_dma_layout_make(&arena, scratch_caps, 1, 0,
            PI4_XHCI_USBSTS_HALTED, PI4_XHCI_LEGACY_OS_OWNED,
            PI4_PCI_COMMAND_MEMORY, &layout) != PI4_XHCI_DMA_PLAN_OK ||
        layout.scratchpads != 2 || layout.scratch_array_offset % 64 ||
        layout.scratch_pages_offset % 4096 ||
        layout.command_offset < layout.scratch_pages_offset + 8192 ||
        layout.command_offset % PI4_XHCI_DMA_RING_ALLOC_BYTES)
        return fail("scratchpad array and pages layout");
    scratch_caps[2] = (8u << 27) | (1u << 21); /* 40, split fields */
    if (pi4_xhci_dma_layout_make(&arena, scratch_caps, 1, 0,
            PI4_XHCI_USBSTS_HALTED, PI4_XHCI_LEGACY_OS_OWNED,
            PI4_PCI_COMMAND_MEMORY, &layout) != PI4_XHCI_DMA_PLAN_OK ||
        layout.scratchpads != 40)
        return fail("split scratchpad count decoded");
    scratch_caps[2] = (4u << 27) | (8u << 21); /* 260 exceeds 1 MiB */
    if (pi4_xhci_dma_layout_make(&arena, scratch_caps, 1, 0,
            PI4_XHCI_USBSTS_HALTED, PI4_XHCI_LEGACY_OS_OWNED,
            PI4_PCI_COMMAND_MEMORY, &layout) != PI4_XHCI_DMA_PLAN_ARENA)
        return fail("scratchpad layout beyond arena accepted");

    uint32_t narrow_caps[6];
    for (unsigned i = 0; i < 6; i++) narrow_caps[i] = caps[i];
    narrow_caps[3] &= ~1u; /* no 64-bit DMA */
    struct pi4_dma_probe_plan high_arena = arena;
    high_arena.bus_base = 0x142010000ull;
    if (pi4_xhci_dma_layout_make(&high_arena, narrow_caps, 1, 0,
            PI4_XHCI_USBSTS_HALTED, PI4_XHCI_LEGACY_OS_OWNED,
            PI4_PCI_COMMAND_MEMORY, &layout) != PI4_XHCI_DMA_PLAN_ADDRESS)
        return fail("32-bit DMA address limit ignored");

    if (pi4_xhci_dma_noop_event_ok(arena.bus_base + 0x2000, 1u << 24,
            evt_control, arena.bus_base + 0x2010) ||
        pi4_xhci_dma_noop_event_ok(arena.bus_base + 0x2000, 0,
            evt_control, arena.bus_base + 0x2000) ||
        !pi4_xhci_dma_noop_event_ok(arena.bus_base + 0x2000, 1u << 24,
            evt_control & ~1u, arena.bus_base + 0x2000) ||
        pi4_xhci_dma_noop_event_ok(arena.bus_base + 0x2000, 1u << 24,
            (PI4_XHCI_TRB_TYPE_CMD_EVT << 10) | 1u, arena.bus_base + 0x3000))
        return fail("No-op completion accepts both owned cycles; rejects wrong pointer, status, and event type");
    for (unsigned i = 0; i < PI4_XHCI_DMA_COMMAND_WRAP_PRELINK_COUNT; i++) {
        uint64_t pointer = arena.bus_base + 0x2000 +
            (uint64_t)(PI4_XHCI_DMA_COMMAND_WRAP_START_TRB + i) * 16u;
        if (!pi4_xhci_dma_noop_event_ok(pointer, 1u << 24,
                evt_control, pointer))
            return fail("pre-Link No-op completion points to exact TRB");
    }
    if (!pi4_xhci_dma_noop_event_ok(arena.bus_base + 0x2000, 1u << 24,
            evt_control, arena.bus_base + 0x2000))
        return fail("post-Link No-op completion points to ring base");

    if (!pi4_xhci_dma_mfindex_event_ok(1u << 24,
            (PI4_XHCI_TRB_TYPE_MFINDEX_EVT << 10) | 1u) ||
        pi4_xhci_dma_mfindex_event_ok(0, (PI4_XHCI_TRB_TYPE_MFINDEX_EVT << 10) | 1u) ||
        !pi4_xhci_dma_mfindex_event_ok(1u << 24,
            (PI4_XHCI_TRB_TYPE_MFINDEX_EVT << 10)) ||
        pi4_xhci_dma_mfindex_event_ok(1u << 24,
            (PI4_XHCI_TRB_TYPE_CMD_EVT << 10) | 1u))
        return fail("MFINDEX-wrap event validation");
    if (!pi4_xhci_dma_mfindex_wrapped(0x3ffe, 0x0002) ||
        pi4_xhci_dma_mfindex_wrapped(0x0010, 0x0011) ||
        pi4_xhci_dma_mfindex_wrapped(0x3fff, 0x3fff))
        return fail("14-bit MFINDEX wrap detection");

    {
        uint32_t reset_value;
        uint32_t portsc = PI4_XHCI_PORTSC_CCS | PI4_XHCI_PORTSC_PP |
                          PI4_XHCI_PORTSC_PRC | (1u << 18) |
                          PI4_XHCI_PORTSC_WAKE_MASK;
        if (pi4_xhci_dma_portsc_clear_changes(portsc) !=
                (PI4_XHCI_PORTSC_PP | PI4_XHCI_PORTSC_WAKE_MASK |
                 PI4_XHCI_PORTSC_PRC | (1u << 18)) ||
            pi4_xhci_dma_port_reset_value(portsc, &reset_value) != 0 ||
            reset_value != (PI4_XHCI_PORTSC_PP | PI4_XHCI_PORTSC_WAKE_MASK |
                            PI4_XHCI_PORTSC_PR) ||
            pi4_xhci_dma_port_reset_value(portsc & ~PI4_XHCI_PORTSC_CCS,
                                           &reset_value) == 0 ||
            pi4_xhci_dma_port_reset_value(portsc & ~PI4_XHCI_PORTSC_PP,
                                           &reset_value) == 0 ||
            pi4_xhci_dma_port_reset_value(portsc | PI4_XHCI_PORTSC_PR,
                                           &reset_value) == 0)
            return fail("safe powered connected-port reset planning");
        if (!pi4_xhci_dma_port_event_ok((uint64_t)1u << 24, 1u << 24,
                (PI4_XHCI_TRB_TYPE_PSC_EVT << 10) | 1u, 1) ||
            !pi4_xhci_dma_port_event_ok((uint64_t)1u << 24, 1u << 24,
                PI4_XHCI_TRB_TYPE_PSC_EVT << 10, 1) ||
            pi4_xhci_dma_port_event_ok((uint64_t)2u << 24, 1u << 24,
                (PI4_XHCI_TRB_TYPE_PSC_EVT << 10) | 1u, 1) ||
            pi4_xhci_dma_port_event_ok((uint64_t)1u << 24, 0,
                (PI4_XHCI_TRB_TYPE_PSC_EVT << 10) | 1u, 1) ||
            pi4_xhci_dma_port_event_ok((uint64_t)1u << 24, 1u << 24,
                (PI4_XHCI_TRB_TYPE_MFINDEX_EVT << 10) | 1u, 1))
            return fail("root-port reset event validation");
    }
    {
        const uint32_t ready = PI4_XHCI_PORTSC_CCS | PI4_XHCI_PORTSC_PED |
                               PI4_XHCI_PORTSC_PP | (3u << 10);
        if (pi4_xhci_dma_port_speed_id(ready) != 3u ||
            !pi4_xhci_dma_port_ready(ready) ||
            pi4_xhci_dma_port_ready(ready & ~PI4_XHCI_PORTSC_SPEED_MASK) ||
            pi4_xhci_dma_port_ready(ready | PI4_XHCI_PORTSC_PR) ||
            pi4_xhci_dma_port_ready(ready & ~PI4_XHCI_PORTSC_PP) ||
            pi4_xhci_dma_port_ready(ready & ~PI4_XHCI_PORTSC_PED) ||
            pi4_xhci_dma_port_ready(ready | (5u << 10)) ||
            pi4_xhci_dma_port_ready(ready | (1u << 5)))
            return fail("post-reset port readiness requires valid speed, power, and U0");
    }

    if (!pi4_xhci_dma_setup_readback_ok(PI4_XHCI_DMA_SETUP_ALL,
            PI4_XHCI_IMAN_IE, 0, 1, dcbaa_bus, 1, erst_bus, event_bus,
            1, dcbaa_bus, 1, erst_bus, event_bus) ||
        !pi4_xhci_dma_setup_readback_ok(PI4_XHCI_DMA_SETUP_ALL,
            PI4_XHCI_IMAN_IE, (0x1234u << 16) | PI4_XHCI_IMODI_MIN,
            1, dcbaa_bus, 1, erst_bus, event_bus,
            1, dcbaa_bus, 1, erst_bus, event_bus) ||
        !pi4_xhci_dma_setup_readback_ok(PI4_XHCI_DMA_SETUP_ALL,
            PI4_XHCI_IMAN_IE, 0, 0x101, dcbaa_bus | 0x3f, 0x10001, erst_bus | 0x3f,
            event_bus | 0xf, 1, dcbaa_bus, 1, erst_bus, event_bus) ||
        pi4_xhci_dma_setup_readback_ok(PI4_XHCI_DMA_SETUP_ALL & ~
            PI4_XHCI_DMA_SETUP_ERDP, 0, 0, 1, dcbaa_bus, 1, erst_bus, event_bus,
            1, dcbaa_bus, 1, erst_bus, event_bus) ||
        pi4_xhci_dma_setup_readback_ok(PI4_XHCI_DMA_SETUP_ALL,
            0, 0, 2, dcbaa_bus, 1, erst_bus, event_bus,
            1, dcbaa_bus, 1, erst_bus, event_bus) ||
        pi4_xhci_dma_setup_readback_ok(PI4_XHCI_DMA_SETUP_ALL,
            0, 0, 1, dcbaa_bus, 1, erst_bus + 0x40, event_bus,
            1, dcbaa_bus, 1, erst_bus, event_bus))
        return fail("pre-BME setup register readback validation");
    if (pi4_xhci_dma_setup_mismatch_mask(PI4_XHCI_DMA_SETUP_ALL,
            PI4_XHCI_IMAN_IE, 0, 1, dcbaa_bus + 0x40, 1,
            erst_bus, event_bus, 1, dcbaa_bus, 1, erst_bus, event_bus) !=
            PI4_XHCI_DMA_SETUP_BAD_DCBAAP ||
        pi4_xhci_dma_setup_mismatch_mask(PI4_XHCI_DMA_SETUP_ALL,
            PI4_XHCI_IMAN_IE, 0, 1, dcbaa_bus, 1,
            erst_bus + 0x40, event_bus, 1, dcbaa_bus, 1, erst_bus,
            event_bus) != PI4_XHCI_DMA_SETUP_BAD_ERSTBA ||
        pi4_xhci_dma_setup_mismatch_mask(PI4_XHCI_DMA_SETUP_ALL,
            PI4_XHCI_IMAN_IE, 0, 1, dcbaa_bus, 1,
            erst_bus, event_bus + 0x10, 1, dcbaa_bus, 1, erst_bus,
            event_bus) != PI4_XHCI_DMA_SETUP_BAD_ERDP)
        return fail("pre-BME setup mismatch identifies the specific address register");
    if (pi4_xhci_dma_setup_readback_ok(PI4_XHCI_DMA_SETUP_ALL,
            PI4_XHCI_IMAN_IE, 2, 1, dcbaa_bus, 1, erst_bus, event_bus,
            1, dcbaa_bus, 1, erst_bus, event_bus) ||
        pi4_xhci_dma_setup_readback_ok(PI4_XHCI_DMA_SETUP_ALL,
            PI4_XHCI_IMAN_IE | 1, 0, 1, dcbaa_bus, 1, erst_bus, event_bus,
            1, dcbaa_bus, 1, erst_bus, event_bus) ||
        pi4_xhci_dma_setup_readback_ok(PI4_XHCI_DMA_SETUP_ALL,
            PI4_XHCI_IMAN_IE, 2, 1, dcbaa_bus, 1, erst_bus, event_bus,
            1, dcbaa_bus, 1, erst_bus, event_bus))
        return fail("primary interrupter IP/IE or interval not initialized");

    if (pi4_xhci_dma_enable_command(PI4_PCI_COMMAND_MEMORY, &enabled) != 0 ||
        enabled != (PI4_PCI_COMMAND_MEMORY | PI4_PCI_COMMAND_MASTER) ||
        pi4_xhci_dma_enable_command(0, NULL) == 0 ||
        pi4_xhci_dma_enable_command(PI4_PCI_COMMAND_MASTER, &enabled) == 0)
        return fail("PCI command enables BME only from a clear state");

    puts("PASS Pi4 xHCI DMA, live ring publication/wrap/prefix, mixed command/BOT/event laps and both event cycles, SuperSpeed root selection/warm reset/EP0, fixed-speed MPS, BOT/SS companions, bulk contexts/Normal TRBs, read-only INQUIRY/capacity/READ10/sense/CSW, CRC32, ring and hub controls");
    return 0;
}
