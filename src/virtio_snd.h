/*
 * Minimal modern VirtIO sound playback for the AArch64 QEMU virt machine.
 * Uses one stereo S16/48 kHz playback stream and polling split virtqueues.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "mmio_map.h"

#define VS_VENDOR       0x1af4u
#define VS_DEVICE       0x1059u /* modern VirtIO sound, device id 25 */
#define VS_VERSION_1    1u
#define VS_STATUS_ACK   1u
#define VS_STATUS_DRIVER 2u
#define VS_STATUS_FEATURES_OK 8u
#define VS_STATUS_DRIVER_OK 4u
#define VS_STATUS_FAILED 128u
#define VS_CAP_COMMON   1u
#define VS_CAP_NOTIFY   2u
#define VS_CAP_DEVICE   4u
#define VS_QSIZE        16u
#define VS_PCM_SLOTS    4u
#define VS_RATE         48000u
#define VS_FRAMES       960u /* 20 ms at 48 kHz */
#define VS_PCM_BYTES    (VS_FRAMES * 2u * sizeof(int16_t))
#define VS_PCM_STATUS_BYTES 8u
#define VS_DESC_F_NEXT  1u
#define VS_DESC_F_WRITE 2u

enum { VS_Q_CONTROL = 0, VS_Q_EVENT = 1, VS_Q_TX = 2, VS_Q_RX = 3 };
enum {
    VS_R_PCM_INFO = 0x0100, VS_R_PCM_SET_PARAMS, VS_R_PCM_PREPARE,
    VS_R_PCM_RELEASE, VS_R_PCM_START, VS_R_PCM_STOP,
    VS_S_OK = 0x8000,
    VS_FMT_S16 = 5,
    VS_RATE_48000 = 7
};

struct vs_desc { uint64_t addr; uint32_t len; uint16_t flags, next; };
struct vs_used_elem { uint32_t id, len; };
struct vs_avail {
    volatile uint16_t flags, idx;
    volatile uint16_t ring[VS_QSIZE];
    volatile uint16_t used_event;
};
struct vs_used {
    volatile uint16_t flags, idx;
    volatile struct vs_used_elem ring[VS_QSIZE];
    volatile uint16_t avail_event;
};
struct vs_queue {
    struct vs_desc *desc;
    struct vs_avail *avail;
    struct vs_used *used;
    volatile uint8_t *notify;
    uint16_t size, last_used;
};
struct vs_caps { volatile uint8_t *common, *notify, *device; uint32_t notify_mult; };
struct vs_hdr { uint32_t code; };
struct vs_pcm_hdr { uint32_t code, stream_id; };
struct vs_query { struct vs_hdr hdr; uint32_t start_id, count, size; };
struct vs_pcm_info {
    uint32_t hda_fn_nid, features;
    uint64_t formats, rates;
    uint8_t direction, channels_min, channels_max, padding[5];
};
struct vs_set_params {
    struct vs_pcm_hdr hdr;
    uint32_t buffer_bytes, period_bytes, features;
    uint8_t channels, format, rate, padding;
};
struct vs_xfer { uint32_t stream_id; };
struct vs_pcm_status { uint32_t status, latency_bytes; };

__attribute__((aligned(4096))) static struct vs_desc g_vs_desc[4][VS_QSIZE];
__attribute__((aligned(4096))) static struct vs_avail g_vs_avail[4];
__attribute__((aligned(4096))) static struct vs_used g_vs_used[4];
__attribute__((aligned(4096))) static int16_t g_vs_pcm[VS_PCM_SLOTS][VS_FRAMES * 2];
__attribute__((aligned(64))) static struct vs_xfer g_vs_xfer[VS_PCM_SLOTS];
/* The device writes each 8-byte status while the CPU owns adjacent playback
 * state. Give every status its own cache line so dc civac cannot write back a
 * stale device status from a line shared with CPU-owned globals. */
struct vs_pcm_status_cacheline {
    struct vs_pcm_status value;
    uint8_t padding[64 - sizeof(struct vs_pcm_status)];
};
__attribute__((aligned(64))) static struct vs_pcm_status_cacheline g_vs_status[VS_PCM_SLOTS];
static struct vs_queue g_vs_q[4];
static uint64_t g_vs_hhdm;
static uint32_t g_vs_hz;
static uint32_t g_vs_phase;
static uint32_t g_vs_tone_frames;
static int g_vs_tone_bounded;
static uint8_t g_vs_busy[VS_PCM_SLOTS];
static uint8_t g_vs_beep_slot[VS_PCM_SLOTS];
static uint8_t g_vs_beep_inflight;
static uint8_t g_vs_slots;
static int g_vs_ready, g_vs_stream_started, g_vs_debug;
static uint64_t g_vs_pump_previous, g_vs_pump_max_gap;
static uint64_t g_vs_submitted_frames;

static uint64_t vs_counter(void) {
    uint64_t counter;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(counter));
    return counter;
}
static void vs_record_pump_gap(void) {
    uint64_t now = vs_counter();
    if (g_vs_pump_previous && now - g_vs_pump_previous > g_vs_pump_max_gap)
        g_vs_pump_max_gap = now - g_vs_pump_previous;
    g_vs_pump_previous = now;
}
static uint64_t virtio_snd_pump_max_gap_us(void) {
    uint64_t frequency;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    if (!frequency) return 0;
    return g_vs_pump_max_gap / frequency * 1000000u +
           (g_vs_pump_max_gap % frequency) * 1000000u / frequency;
}
static uint64_t virtio_snd_submitted_frames(void) { return g_vs_submitted_frames; }

static inline void vs_dsb(void) { __asm__ volatile("dsb sy" ::: "memory"); }
static inline uint8_t vs_r8(volatile uint8_t *p) { return *p; }
static inline uint16_t vs_r16(volatile uint8_t *p) { return *(volatile uint16_t *)(uintptr_t)p; }
static inline uint32_t vs_r32(volatile uint8_t *p) { return *(volatile uint32_t *)(uintptr_t)p; }
static inline void vs_w8(volatile uint8_t *p, uint8_t v) { *p = v; }
static inline void vs_w16(volatile uint8_t *p, uint16_t v) { *(volatile uint16_t *)(uintptr_t)p = v; }
static inline void vs_w32(volatile uint8_t *p, uint32_t v) { *(volatile uint32_t *)(uintptr_t)p = v; }
static inline void vs_w64(volatile uint8_t *p, uint64_t v) {
    vs_w32(p, (uint32_t)v); vs_w32(p + 4, (uint32_t)(v >> 32));
}
static inline uint64_t vs_phys(const void *p) { return (uint64_t)(uintptr_t)p - g_vs_hhdm; }

static void vs_cache(void *ptr, size_t size, int invalidate) {
    uint64_t ctr, line;
    uintptr_t start, end;
    if (!ptr || !size) return;
    __asm__ volatile("mrs %0, ctr_el0" : "=r"(ctr));
    line = 4ull << ((ctr >> 16) & 15u);
    if (line < 16 || line > 2048 || (line & (line - 1))) line = 64;
    start = (uintptr_t)ptr & ~(uintptr_t)(line - 1);
    end = (uintptr_t)ptr + size;
    if (end < (uintptr_t)ptr) return;
    for (uintptr_t p = start; p < end; p += (uintptr_t)line) {
        if (invalidate) __asm__ volatile("dc civac, %0" :: "r"(p) : "memory");
        else __asm__ volatile("dc cvac, %0" :: "r"(p) : "memory");
    }
    vs_dsb();
}

static volatile uint8_t *vs_ecam(unsigned dev) {
    uint64_t phys = PCI_ECAM_PHYS + ((uint64_t)dev << 15);
    return (volatile uint8_t *)(uintptr_t)(g_vs_hhdm + phys);
}
static uint64_t vs_bar(volatile uint8_t *cfg, unsigned off) {
    uint32_t lo = vs_r32(cfg + off);
    uint64_t addr;
    if (lo & 1u) return 0;
    addr = lo & ~0xfull;
    if (((lo >> 1) & 3u) == 2u) addr |= (uint64_t)vs_r32(cfg + off + 4) << 32;
    return addr;
}
static int vs_find_caps(volatile uint8_t *cfg, struct vs_caps *out) {
    uint16_t status = (uint16_t)(vs_r32(cfg + 4) >> 16);
    uint8_t pos;
    unsigned steps = 0;
    if (!(status & (1u << 4))) return 0;
    out->common = out->notify = out->device = NULL; out->notify_mult = 0;
    pos = cfg[0x34] & 0xfcu;
    while (pos >= 0x40 && steps++ < 48) {
        uint8_t next = cfg[pos + 1] & 0xfcu;
        if (cfg[pos] == 0x09) {
            uint8_t len = cfg[pos + 2], type = cfg[pos + 3], bar = cfg[pos + 4];
            uint32_t off = vs_r32(cfg + pos + 8);
            uint64_t base_phys = bar < 6 ? vs_bar(cfg, 0x10u + bar * 4u) : 0;
            if (len >= 16 && base_phys) {
                if (mmio_map_2m(g_vs_hhdm, base_phys & ~((1ull << 21) - 1)) != 0) return 0;
                volatile uint8_t *base = (volatile uint8_t *)(uintptr_t)(g_vs_hhdm + base_phys);
                if (type == VS_CAP_COMMON) out->common = base + off;
                else if (type == VS_CAP_NOTIFY) {
                    out->notify = base + off;
                    if (len >= 20) out->notify_mult = vs_r32(cfg + pos + 16);
                } else if (type == VS_CAP_DEVICE) out->device = base + off;
            }
        }
        pos = next;
    }
    return out->common && out->notify && out->device;
}
static int vs_queue_setup(struct vs_caps *caps, unsigned qid) {
    volatile uint8_t *c = caps->common;
    struct vs_queue *q = &g_vs_q[qid];
    uint16_t n, notify_off;
    vs_w16(c + 22, (uint16_t)qid);
    n = vs_r16(c + 24);
    if (n < 4) return 0;
    if (n > VS_QSIZE) n = VS_QSIZE;
    /* QueueNumMax is a device limit, not the active queue size. Program the
     * selected size before enabling the queue so device and ring storage use
     * the same modulus after the available/used indices wrap. */
    vs_w16(c + 24, n);
    if (vs_r16(c + 24) != n) return 0;
    q->size = n; q->last_used = 0;
    q->desc = g_vs_desc[qid]; q->avail = &g_vs_avail[qid]; q->used = &g_vs_used[qid];
    notify_off = vs_r16(c + 30);
    q->notify = caps->notify + (uint64_t)notify_off * caps->notify_mult;
    vs_w16(c + 16, 0xffffu); vs_w16(c + 26, 0xffffu);
    vs_w64(c + 32, vs_phys(q->desc)); vs_w64(c + 40, vs_phys(q->avail));
    vs_w64(c + 48, vs_phys(q->used)); vs_dsb();
    vs_w16(c + 28, 1);
    return 1;
}

/* One synchronous control request: request descriptor followed by response. */
static int vs_control(const void *request, uint32_t req_len, void *response,
                      uint32_t resp_cap, uint32_t *resp_len) {
    struct vs_queue *q = &g_vs_q[VS_Q_CONTROL];
    uint16_t idx, slot;
    struct vs_desc *d = q->desc;
    if (!g_vs_ready || req_len > 128 || resp_cap > 128) return 0;
    idx = q->avail->idx; slot = idx % q->size;
    d[0].addr = vs_phys(request); d[0].len = req_len; d[0].flags = VS_DESC_F_NEXT; d[0].next = 1;
    d[1].addr = vs_phys(response); d[1].len = resp_cap; d[1].flags = VS_DESC_F_WRITE; d[1].next = 0;
    q->avail->ring[slot] = 0;
    vs_cache((void *)request, req_len, 0); vs_cache(response, resp_cap, 0);
    vs_cache(d, sizeof(*d) * 2, 0); vs_cache(q->avail, sizeof(*q->avail), 0);
    vs_dsb(); q->avail->idx = (uint16_t)(idx + 1); vs_cache(q->avail, sizeof(*q->avail), 0); vs_dsb();
    vs_w16(q->notify, 0);
    for (unsigned spin = 0; spin < 5000000u; spin++) {
        vs_cache(q->used, sizeof(*q->used), 1);
        if (q->last_used != q->used->idx) {
            struct vs_used_elem e = q->used->ring[q->last_used % q->size];
            q->last_used++;
            if (e.id != 0 || e.len < 4 || e.len > resp_cap) { g_vs_debug = 20; return 0; }
            vs_cache(response, e.len, 1);
            if (resp_len) *resp_len = e.len;
            return 1;
        }
        __asm__ volatile("yield" ::: "memory");
    }
    g_vs_debug = 21;
    return 0;
}
static int vs_pcm_command(uint32_t code) {
    struct vs_pcm_hdr req = { code, 0 };
    uint32_t resp = 0, len = 0;
    if (!vs_control(&req, sizeof(req), &resp, sizeof(resp), &len) || len < 4 || resp != VS_S_OK)
        return 0;
    return 1;
}

static int virtio_snd_init(uint64_t hhdm) {
    volatile uint8_t *cfg = NULL;
    struct vs_caps caps;
    uint32_t f0, f1, streams;
    g_vs_ready = g_vs_stream_started = 0; g_vs_debug = 0; g_vs_hhdm = hhdm;
    if (mmio_map_pci_ecam(hhdm) != 0) { g_vs_debug = 1; return 0; }
    for (unsigned dev = 0; dev < 32; dev++) {
        volatile uint8_t *candidate = vs_ecam(dev);
        uint32_t id = vs_r32(candidate);
        if ((uint16_t)id == VS_VENDOR && (uint16_t)(id >> 16) == VS_DEVICE) { cfg = candidate; break; }
    }
    if (!cfg) { g_vs_debug = 2; return 0; }
    vs_w16(cfg + 4, 0x0006u);
    if (!vs_find_caps(cfg, &caps)) { g_vs_debug = 3; return 0; }
    vs_w8(caps.common + 20, 0); vs_dsb();
    for (unsigned i = 0; i < 100000u && vs_r8(caps.common + 20); i++) { }
    if (vs_r8(caps.common + 20)) { g_vs_debug = 4; return 0; }
    vs_w8(caps.common + 20, VS_STATUS_ACK); vs_w8(caps.common + 20, VS_STATUS_ACK | VS_STATUS_DRIVER);
    vs_w32(caps.common + 0, 0); f0 = vs_r32(caps.common + 4);
    vs_w32(caps.common + 0, 1); f1 = vs_r32(caps.common + 4);
    if (!(f1 & VS_VERSION_1)) { g_vs_debug = 5; return 0; }
    vs_w32(caps.common + 8, 0); vs_w32(caps.common + 12, 0);
    vs_w32(caps.common + 8, 1); vs_w32(caps.common + 12, VS_VERSION_1);
    (void)f0;
    vs_w8(caps.common + 20, VS_STATUS_ACK | VS_STATUS_DRIVER | VS_STATUS_FEATURES_OK);
    if (!(vs_r8(caps.common + 20) & VS_STATUS_FEATURES_OK)) { g_vs_debug = 6; return 0; }
    for (unsigned q = 0; q < 4; q++) {
        for (unsigned i = 0; i < VS_QSIZE; i++) g_vs_desc[q][i] = (struct vs_desc){0};
        g_vs_avail[q].flags = g_vs_avail[q].idx = 0;
        g_vs_used[q].flags = g_vs_used[q].idx = 0;
        g_vs_q[q].last_used = 0;
        vs_w16(caps.common + 22, (uint16_t)q); vs_w16(caps.common + 26, 0xffffu);
    }
    if (!vs_queue_setup(&caps, VS_Q_CONTROL) || !vs_queue_setup(&caps, VS_Q_TX)) { g_vs_debug = 7; return 0; }
    streams = vs_r32(caps.device + 4);
    if (!streams) { g_vs_debug = 8; return 0; }
    vs_w8(caps.common + 20, VS_STATUS_ACK | VS_STATUS_DRIVER | VS_STATUS_FEATURES_OK | VS_STATUS_DRIVER_OK);
    if (!(vs_r8(caps.common + 20) & VS_STATUS_DRIVER_OK)) { g_vs_debug = 9; return 0; }
    g_vs_ready = 1;
    {
        struct vs_query req = {{VS_R_PCM_INFO}, 0, 1, sizeof(struct vs_pcm_info)};
        uint8_t resp[4 + sizeof(struct vs_pcm_info)] __attribute__((aligned(8))) = {0};
        uint32_t len = 0;
        if (!vs_control(&req, sizeof(req), resp, sizeof(resp), &len) || len < sizeof(resp) ||
            *(uint32_t *)resp != VS_S_OK) { g_vs_debug = 10; g_vs_ready = 0; return 0; }
        struct vs_pcm_info *info = (struct vs_pcm_info *)(void *)(resp + 4);
        if (info->direction != 0 || info->channels_min > 2 || info->channels_max < 2 ||
            !(info->formats & (1ull << VS_FMT_S16)) || !(info->rates & (1ull << VS_RATE_48000))) {
            g_vs_debug = 11; g_vs_ready = 0; return 0;
        }
    }
    {
        struct vs_set_params req = {{VS_R_PCM_SET_PARAMS, 0}, VS_PCM_BYTES * VS_PCM_SLOTS,
                                    VS_PCM_BYTES, 0, 2, VS_FMT_S16, VS_RATE_48000, 0};
        uint32_t resp = 0, len = 0;
        if (!vs_control(&req, sizeof(req), &resp, sizeof(resp), &len) || len < 4 || resp != VS_S_OK ||
            !vs_pcm_command(VS_R_PCM_PREPARE) || !vs_pcm_command(VS_R_PCM_START)) {
            g_vs_debug = 12; g_vs_ready = 0; return 0;
        }
    }
    g_vs_stream_started = 1;
    g_vs_slots = (uint8_t)(g_vs_q[VS_Q_TX].size / 3u);
    if (g_vs_slots > VS_PCM_SLOTS) g_vs_slots = VS_PCM_SLOTS;
    if (!g_vs_slots) { g_vs_debug = 13; g_vs_ready = 0; return 0; }
    return 1;
}

static uint32_t vs_note_hz(int8_t ona) {
    int delta = (int)ona - 60;
    uint32_t hz = 440;
    if (delta > 0) while (delta--) hz = (uint32_t)(((uint64_t)hz * 1059463u + 500000u) / 1000000u);
    else while (delta++) hz = (uint32_t)(((uint64_t)hz * 943874u + 500000u) / 1000000u);
    return hz;
}
static void vs_submit_tone(unsigned slot) {
    struct vs_queue *q = &g_vs_q[VS_Q_TX];
    unsigned base = slot * 3u;
    uint32_t phase = g_vs_phase;
    uint32_t hz = g_vs_hz;
    g_vs_submitted_frames += VS_FRAMES;
    int beep_slot = 0;
    for (unsigned i = 0; i < VS_FRAMES; i++) {
        int16_t sample;
        if (hz && (!g_vs_tone_bounded || g_vs_tone_frames)) {
            phase += hz;
            if (phase >= VS_RATE) phase -= VS_RATE;
            sample = phase < (VS_RATE / 2) ? 7000 : -7000;
            if (g_vs_tone_bounded) {
                g_vs_tone_frames--;
                beep_slot = 1;
            }
        } else sample = 0;
        g_vs_pcm[slot][i * 2] = sample; g_vs_pcm[slot][i * 2 + 1] = sample;
    }
    if (g_vs_tone_bounded && !g_vs_tone_frames) {
        g_vs_tone_bounded = 0;
        g_vs_hz = 0;
    }
    g_vs_phase = phase;
    g_vs_xfer[slot].stream_id = 0;
    q->desc[base + 0] = (struct vs_desc){vs_phys(&g_vs_xfer[slot]), sizeof(g_vs_xfer[slot]),
                                         VS_DESC_F_NEXT, (uint16_t)(base + 1)};
    q->desc[base + 1] = (struct vs_desc){vs_phys(g_vs_pcm[slot]), VS_PCM_BYTES, VS_DESC_F_NEXT, (uint16_t)(base + 2)};
    q->desc[base + 2] = (struct vs_desc){vs_phys(&g_vs_status[slot].value), VS_PCM_STATUS_BYTES, VS_DESC_F_WRITE, 0};
    g_vs_status[slot].value = (struct vs_pcm_status){0, 0};
    uint16_t idx = q->avail->idx;
    q->avail->ring[idx % q->size] = (uint16_t)base;
    g_vs_beep_slot[slot] = (uint8_t)beep_slot;
    if (beep_slot) g_vs_beep_inflight++;
    vs_cache(&g_vs_xfer[slot], sizeof(g_vs_xfer[slot]), 0); vs_cache(g_vs_pcm[slot], VS_PCM_BYTES, 0);
    vs_cache(&g_vs_status[slot], sizeof(g_vs_status[slot]), 0);
    vs_cache(q->desc, sizeof(*q->desc) * q->size, 0); vs_cache(q->avail, sizeof(*q->avail), 0);
    vs_dsb(); q->avail->idx = (uint16_t)(idx + 1); vs_cache(q->avail, sizeof(*q->avail), 0); vs_dsb();
    vs_w16(q->notify, 0);
    g_vs_busy[slot] = 1;
}
static void virtio_snd_set_tone(int8_t ona) {
    if (ona && !g_vs_hz) {
        g_vs_pump_previous = vs_counter();
        g_vs_pump_max_gap = 0;
        g_vs_submitted_frames = 0;
    } else if (!ona && g_vs_hz) {
        vs_record_pump_gap();
    }
    g_vs_tone_bounded = 0;
    g_vs_tone_frames = 0;
    g_vs_hz = ona ? vs_note_hz(ona) : 0;
    if (g_vs_ready && g_vs_stream_started) {
        for (unsigned i = 0; i < g_vs_slots; i++) if (!g_vs_busy[i]) { vs_submit_tone(i); break; }
    }
}
static void virtio_snd_pump(void);
static void virtio_snd_beep(int8_t ona) {
    g_vs_hz = ona ? vs_note_hz(ona) : 0;
    g_vs_tone_frames = VS_RATE / 2u;
    g_vs_tone_bounded = g_vs_hz != 0;
    if (g_vs_ready && g_vs_stream_started) virtio_snd_pump();
}
static int virtio_snd_beep_done(void) {
    return !g_vs_ready || !g_vs_stream_started ||
        (!g_vs_tone_bounded && !g_vs_beep_inflight);
}
static void virtio_snd_pump(void) {
    struct vs_queue *q = &g_vs_q[VS_Q_TX];
    if (!g_vs_ready || !g_vs_stream_started) return;
    if (g_vs_hz) {
        vs_record_pump_gap();
    }
    vs_cache(q->used, sizeof(*q->used), 1);
    while (q->last_used != q->used->idx) {
        struct vs_used_elem e = q->used->ring[q->last_used % q->size];
        q->last_used++;
        unsigned slot = e.id / 3u;
        if (slot < g_vs_slots) {
            vs_cache(&g_vs_status[slot], sizeof(g_vs_status[slot]), 1);
            if (g_vs_status[slot].value.status != VS_S_OK) g_vs_debug = 15;
            if (g_vs_beep_slot[slot]) {
                g_vs_beep_slot[slot] = 0;
                if (g_vs_beep_inflight) g_vs_beep_inflight--;
            }
            g_vs_busy[slot] = 0;
        }
        else g_vs_debug = 14;
    }
    if (g_vs_hz) for (unsigned i = 0; i < g_vs_slots; i++) if (!g_vs_busy[i]) vs_submit_tone(i);
}

static int virtio_snd_ready(void) { return g_vs_ready; }
static int virtio_snd_debug(void) { return g_vs_debug; }
