/* Reversible BCM2711 PCIe BAR2 high-window DMA alias experiment. */
#pragma once

#include <stdint.h>
#include "pi4_dma_probe.h"

#define PI4_XHCI_DMA_BAR2_LO_EXPECTED 0x00000011u
#define PI4_XHCI_DMA_BAR2_HI_ORIGINAL 0x00000000u
#define PI4_XHCI_DMA_BAR2_HI_TEST     0x00000004u
#define PI4_XHCI_DMA_BUS_BIAS         (4ull << 32)

/*
 * Make a trial DMA view matching the alternate Pi bare-metal mapping:
 * BAR2 still covers 4 GiB, but starts at PCIe address 16 GiB, so xHCI DMA
 * structures must use the same high address bits. Accept only the exact
 * firmware mapping measured on the Pi and an xHC that advertises AC64.
 * This is an experiment, not a replacement for the FDT DMA contract.
 */
static int pi4_xhci_dma_alias_plan_make(
    const struct pi4_dma_probe_plan *original, const uint32_t caps[6],
    uint32_t bar2_lo, uint32_t bar2_hi,
    struct pi4_dma_probe_plan *aliased) {
    if (!original || !caps || !aliased || !(caps[3] & 1u) ||
        bar2_lo != PI4_XHCI_DMA_BAR2_LO_EXPECTED ||
        bar2_hi != PI4_XHCI_DMA_BAR2_HI_ORIGINAL ||
        !original->size ||
        original->bus_base > UINT64_MAX - PI4_XHCI_DMA_BUS_BIAS ||
        original->bus_base + PI4_XHCI_DMA_BUS_BIAS >
            UINT64_MAX - original->size)
        return -1;
    *aliased = *original;
    aliased->bus_base += PI4_XHCI_DMA_BUS_BIAS;
    return 0;
}
