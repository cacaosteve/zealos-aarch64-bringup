/* BCM2711 HDMI0 stereo tone output over the firmware's existing video mode.
 * Register fields follow the VC4 HDMI map and the BCM2711 DMA specification.
 * The DMA loop emits an integral second of 48 kHz IEC 60958 subframes.
 */
#define PI4_HDMI0_HD_PHYS 0xfef20000ull
#define PI4_HDMI0_PHY_PHYS 0xfef00f00ull
#define PI4_HDMI0_PACKET_PHYS 0xfef01b00ull
#define PI4_DMA_PHYS 0xfe007000ull
#define PI4_HDMI_AUDIO_INFOFRAME (1u << 4)
#define PI4_DMA_ACTIVE (1u << 0)
#define PI4_DMA_PENDING_WRITES (1u << 6)
#define PI4_DMA_ERROR (1u << 8)
#define PI4_DMA_RESET (1u << 31)
#define PI4_HDMI_RATE 48000u
#define PI4_HDMI_DMA_SEGMENTS 6u
#define PI4_HDMI_SEGMENT_BYTES (PI4_HDMI_RATE * 8u / PI4_HDMI_DMA_SEGMENTS)
#include "pi4_audio_policy.h"

struct pi4_hdmi_dma_cb {
    uint32_t info, source, destination, length, stride, next, reserved[2];
};

_Static_assert(sizeof(struct pi4_hdmi_dma_cb) == 32, "DMA control block size");
_Static_assert(PI4_HDMI_SEGMENT_BYTES <= 65535, "DMA Lite transfer size");
_Static_assert(4096 + PI4_HDMI_RATE * 8 <= ZEAL_PI4_AUDIO_BYTES, "audio DMA reservation");

static volatile uint32_t *g_pi4_hdmi_dma;
static unsigned g_pi4_hdmi_stage;
static uint32_t g_pi4_hdmi_last_dma_status, g_pi4_hdmi_last_dma_debug;
static uint32_t g_pi4_hdmi_last_dma_source;
static uint32_t g_pi4_hdmi_last_hz;
static unsigned g_pi4_hdmi_starts;
static int g_pi4_hdmi_dma_stuck;
static int8_t g_pi4_hdmi_active_note;

static int pi4_hdmi_reuses_tone(int8_t ona) {
    if (!g_pi4_hdmi_running || !g_pi4_hdmi_dma) return 0;
    return pi4_hdmi_can_reuse_tone(
        g_pi4_hdmi_running, g_pi4_hdmi_selected, g_pi4_hdmi_active_note,
        ona, g_pi4_hdmi_dma[0]);
}

static volatile uint32_t *pi4_hdmi_regs(uint64_t phys) {
    return (volatile uint32_t *)(uintptr_t)(g_hhdm + phys);
}

static uint32_t pi4_hdmi_subframe(uint32_t sample, unsigned frame) {
    /* Consumer PCM, copy permitted, 48 kHz, 24 significant bits. */
    const uint64_t channel_status = 0xdb02000004ull;
    uint32_t word = (sample & 0xffffffu) << 4;
    if (frame < 40 && ((channel_status >> frame) & 1))
        word |= 1u << 30;
    uint32_t parity = word ^ (word >> 16);
    parity ^= parity >> 8;
    parity ^= parity >> 4;
    return word | (((0x6996u >> (parity & 15)) & 1) << 31) |
           (frame == 0 ? 0xfu : 0);
}

static void pi4_hdmi_prepare_dma(uint32_t hz) {
    const uint32_t bus = (uint32_t)g_pi4_mailbox_phys | PI4_MAILBOX_GPU_ALIAS;
    struct pi4_hdmi_dma_cb *blocks =
        (void *)(uintptr_t)(g_hhdm + g_pi4_mailbox_phys + 256);
    uint32_t *samples = (void *)(uintptr_t)(g_hhdm + g_pi4_mailbox_phys + 4096);
    uint32_t phase = 0;
    unsigned frame = 0;
    for (unsigned i = 0; i < PI4_HDMI_RATE; i++) {
        const uint32_t amplitude = 0x080000;
        uint32_t sample = phase < PI4_HDMI_RATE / 2 ? amplitude : 0u - amplitude;
        samples[2 * i] = samples[2 * i + 1] = pi4_hdmi_subframe(sample, frame);
        phase += hz;
        if (phase >= PI4_HDMI_RATE)
            phase -= PI4_HDMI_RATE;
        if (++frame == 192)
            frame = 0;
    }
    for (unsigned i = 0; i < PI4_HDMI_DMA_SEGMENTS; i++) {
        blocks[i] = (struct pi4_hdmi_dma_cb){
            /* DREQ 10, source increment, destination DREQ, wait for response. */
            .info = (10u << 16) | (1u << 8) | (1u << 6) | (1u << 3),
            .source = bus + 4096 + i * PI4_HDMI_SEGMENT_BYTES,
            .destination = 0x7ef2001cu, /* MAI sample FIFO on the peripheral bus */
            .length = PI4_HDMI_SEGMENT_BYTES,
            .next = bus + 256 + ((i + 1) % PI4_HDMI_DMA_SEGMENTS) * sizeof(*blocks)
        };
    }
    uint64_t ctr;
    __asm__ volatile("mrs %0, ctr_el0" : "=r"(ctr));
    uintptr_t line = 4u << ((ctr >> 16) & 15u);
    uintptr_t end = (uintptr_t)samples + PI4_HDMI_RATE * 8u;
    for (uintptr_t p = (uintptr_t)blocks & ~(line - 1); p < end; p += line)
        __asm__ volatile("dc cvac, %0" :: "r"(p) : "memory");
    pi4_audio_dsb();
}

static void pi4_hdmi_stop(void) {
    volatile uint32_t *hd = pi4_hdmi_regs(PI4_HDMI0_HD_PHYS);
    volatile uint32_t *hdmi = pi4_hdmi_regs(PI4_HDMI0_PHYS);
    g_pi4_hdmi_last_dma_status = g_pi4_hdmi_dma[0];
    g_pi4_hdmi_last_dma_debug = g_pi4_hdmi_dma[0x20 / 4];
    g_pi4_hdmi_last_dma_source = g_pi4_hdmi_dma[0x0c / 4];
    g_pi4_hdmi_dma[0] = 0;
    g_pi4_hdmi_dma[0] = PI4_DMA_RESET;
    pi4_audio_dsb();
    uint64_t deadline = pi4_audio_counter() + g_pi4_audio_cntfrq / 50;
    if (!pi4_mailbox_wait(g_pi4_hdmi_dma, PI4_DMA_ACTIVE | PI4_DMA_PENDING_WRITES,
                          deadline))
        g_pi4_hdmi_dma_stuck = 1;
    hd[0x10 / 4] = (1u << 9) | (1u << 2) | (1u << 1); /* flush and clear errors */
    hdmi[PI4_HDMI_PACKET_CONFIG / 4] &= ~PI4_HDMI_AUDIO_INFOFRAME;
    pi4_audio_dsb();
    g_pi4_hdmi_running = 0;
}

static int pi4_hdmi_tone(int8_t ona) {
    g_pi4_hdmi_stage = 1;
    if (!g_pi4_mailbox_phys || !g_pi4_audio_cntfrq || g_pi4_hdmi_dma_stuck ||
        mmio_map_2m(g_hhdm, PI4_HDMI0_PHYS) ||
        mmio_map_2m(g_hhdm, PI4_DMA_PHYS))
        return 0;

    g_pi4_hdmi_stage = 2;
    volatile uint32_t *hdmi = pi4_hdmi_regs(PI4_HDMI0_PHYS);
    uint32_t scheduler, packet;
    if (pi4_xhci_read_mmio32((volatile uint8_t *)hdmi, PI4_HDMI_SCHEDULER_CONTROL,
                            PI4_HDMI_REG_SIZE, &scheduler) ||
        pi4_xhci_read_mmio32((volatile uint8_t *)hdmi, PI4_HDMI_PACKET_CONFIG,
                            PI4_HDMI_REG_SIZE, &packet) ||
        !(scheduler & PI4_HDMI_LINK_ACTIVE) || !(packet & PI4_HDMI_PACKET_ENABLE))
        return 0;

    g_pi4_hdmi_stage = 3;
    uint32_t pixel_hz = 0, dma_mask = 0;
    if (!pi4_hdmi_firmware_resources(&pixel_hz, &dma_mask) ||
        pixel_hz < 25000000 || pixel_hz > 340000000)
        return 0;

    g_pi4_hdmi_stage = 4;
    volatile uint32_t *dma = pi4_hdmi_regs(PI4_DMA_PHYS);
    unsigned channel;
    for (channel = 7; channel <= 10; channel++) {
        if (!(dma_mask & (1u << channel)))
            continue;
        volatile uint32_t *candidate = dma + (0x100 / 4) * channel;
        if (!(candidate[0] & (PI4_DMA_ACTIVE | PI4_DMA_PENDING_WRITES))) {
            g_pi4_hdmi_dma = candidate;
            break;
        }
    }
    if (channel > 10)
        return 0;
    dma[0xff0 / 4] |= 1u << channel;
    g_pi4_hdmi_dma[0] = PI4_DMA_RESET;
    pi4_audio_dsb();
    uint32_t hz = pi4_audio_note_hz(ona);
    if (!hz || hz >= PI4_HDMI_RATE / 2)
        return 0;
    pi4_hdmi_prepare_dma(hz);

    g_pi4_hdmi_stage = 5;
    volatile uint32_t *status = &hdmi[PI4_HDMI_PACKET_STATUS / 4];
    uint64_t deadline = pi4_audio_counter() + g_pi4_audio_cntfrq / 10;
    hdmi[PI4_HDMI_PACKET_CONFIG / 4] = packet & ~PI4_HDMI_AUDIO_INFOFRAME;
    pi4_audio_dsb();
    if (!pi4_mailbox_wait(status, PI4_HDMI_AUDIO_INFOFRAME, deadline)) {
        hdmi[PI4_HDMI_PACKET_CONFIG / 4] = packet;
        return 0;
    }
    /* Audio InfoFrame slot 4, with two channels and stream-defined format. */
    volatile uint32_t *infoframe = pi4_hdmi_regs(PI4_HDMI0_PACKET_PHYS) + 0x90 / 4;
    infoframe[0] = 0x0a0184u;
    infoframe[1] = 0x0170u;
    for (unsigned i = 2; i < 9; i++)
        infoframe[i] = 0;
    hdmi[PI4_HDMI_PACKET_CONFIG / 4] = packet | PI4_HDMI_AUDIO_INFOFRAME;
    pi4_audio_dsb();

    if (g_pi4_audio_last_hz)
        pi4_audio_set_tone(0);
    volatile uint32_t *hd = pi4_hdmi_regs(PI4_HDMI0_HD_PHYS);
    volatile uint32_t *phy = pi4_hdmi_regs(PI4_HDMI0_PHY_PHYS);
    hd[0x10 / 4] = (1u << 15) | (1u << 9) | (1u << 2) | (1u << 1) | 1u;
    hd[0x20 / 4] = (2250u << 8); /* 108 MHz / 48 kHz, divider denominator 1 */
    hd[0x18 / 4] = (2u << 16) | (9u << 8); /* 48 kHz PCM */
    hd[0x14 / 4] = 0x10101010u; /* DREQ and panic FIFO watermarks */
    hdmi[0x9c / 4] = 1u << 4; /* route channel 1 to slot 1 */
    hdmi[0xa0 / 4] = (1u << 27) | (1u << 26) | 3u;
    hdmi[0xb8 / 4] = (1u << 29) | (1u << 24) | (0xfu << 10) | 3u;
    hdmi[PI4_HDMI_AUDIO_CLOCK_CONFIG / 4] = (1u << 24) | 6144u;
    hdmi[PI4_HDMI_AUDIO_CTS / 4] = pixel_hz / 1000;
    hdmi[(PI4_HDMI_AUDIO_CTS + 4) / 4] = pixel_hz / 1000;
    phy[1] &= ~(1u << 4); /* power the HDMI random generator */
    g_pi4_hdmi_dma[1] = ((uint32_t)g_pi4_mailbox_phys | PI4_MAILBOX_GPU_ALIAS) + 256;
    pi4_audio_dsb();
    g_pi4_hdmi_dma[0] = PI4_DMA_ACTIVE | (8u << 16) | (8u << 20);
    hd[0x10 / 4] = (1u << 13) | (1u << 12) | (2u << 4) | (1u << 3);
    pi4_audio_dsb();
    g_pi4_hdmi_running = 1;
    g_pi4_hdmi_stage = 6;
    uint32_t first_source = ((uint32_t)g_pi4_mailbox_phys | PI4_MAILBOX_GPU_ALIAS) + 4096;
    deadline = pi4_audio_counter() + g_pi4_audio_cntfrq / 50;
    do {
        uint32_t cs = g_pi4_hdmi_dma[0];
        uint32_t source = g_pi4_hdmi_dma[0x0c / 4];
        if (cs & PI4_DMA_ERROR)
            break;
        if ((cs & PI4_DMA_ACTIVE) && source > first_source &&
            source < first_source + PI4_HDMI_RATE * 8u) {
            g_pi4_hdmi_last_hz = hz;
            g_pi4_hdmi_active_note = ona;
            g_pi4_hdmi_starts++;
            g_pi4_hdmi_stage = 7;
            return 1;
        }
        __asm__ volatile("yield");
    } while ((int64_t)(pi4_audio_counter() - deadline) < 0);
    pi4_hdmi_stop();
    return 0;
}
