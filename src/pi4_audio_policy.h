#ifndef ZEAL_PI4_AUDIO_POLICY_H
#define ZEAL_PI4_AUDIO_POLICY_H

#include <stdint.h>

/* Reuse an HDMI tone only while the matching DMA stream is healthy. */
static inline int pi4_hdmi_can_reuse_tone(int running, int selected,
                                           int8_t current, int8_t requested,
                                           uint32_t dma_status) {
    const uint32_t dma_active = 1u << 0;
    const uint32_t dma_error = 1u << 8;
    return running && selected && requested && requested == current &&
           (dma_status & dma_active) && !(dma_status & dma_error);
}

#endif
