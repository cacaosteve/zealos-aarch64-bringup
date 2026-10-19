/* Constrained Pi 4 USB DMA arena reservation. This does not initialize PCIe. */
#pragma once

#include <stdint.h>
#include "handoff.h"

#define PI4_USB_DMA_ARENA_BYTES (1024ull * 1024ull)
#define PI4_USB_DMA_ARENA_ALIGN (64ull * 1024ull)
#define PI4_USB_DMA_MIN_PHYS    (4ull * 1024ull * 1024ull)

static int pi4_usb_dma_reserve(struct zeal_handoff *h) {
    const uint32_t required = PI4_PCIE_FDT_PRESENT | PI4_PCIE_FDT_ENABLED |
                              PI4_PCIE_FDT_DMA;
    const uint64_t bytes = PI4_USB_DMA_ARENA_BYTES;
    const uint64_t align = PI4_USB_DMA_ARENA_ALIGN;
    uint64_t dma_cpu_end, dma_bus_end;

    if (!h || h->mem_count > ZEAL_MEM_MAX ||
        (h->pi4_pcie.flags & required) != required ||
        !h->pi4_pcie.dma_size || h->pi4_usb_dma.flags ||
        h->pi4_pcie.dma_cpu_base > UINT64_MAX - h->pi4_pcie.dma_size ||
        h->pi4_pcie.dma_bus_base > UINT64_MAX - h->pi4_pcie.dma_size)
        return -1;
    dma_cpu_end = h->pi4_pcie.dma_cpu_base + h->pi4_pcie.dma_size;
    dma_bus_end = h->pi4_pcie.dma_bus_base + h->pi4_pcie.dma_size;

    for (uint32_t i = 0; i < h->mem_count; i++) {
        struct zeal_mem_entry *entry = &h->mem[i];
        uint64_t entry_end, low, high, base, end, bus;
        int before, after;
        uint32_t extra;

        if (entry->type != ZEAL_MEM_USABLE || !entry->length ||
            entry->base > UINT64_MAX - entry->length)
            continue;
        entry_end = entry->base + entry->length;
        low = entry->base;
        if (low < h->pi4_pcie.dma_cpu_base) low = h->pi4_pcie.dma_cpu_base;
        if (low < PI4_USB_DMA_MIN_PHYS) low = PI4_USB_DMA_MIN_PHYS;
        high = entry_end < dma_cpu_end ? entry_end : dma_cpu_end;
        if (low > UINT64_MAX - (align - 1)) continue;
        base = (low + align - 1) & ~(align - 1);
        if (base > UINT64_MAX - bytes) continue;
        end = base + bytes;
        if (end > high || base < entry->base) continue;
        if (h->pi4_pcie.dma_bus_base > UINT64_MAX -
                                               (base - h->pi4_pcie.dma_cpu_base))
            continue;
        bus = h->pi4_pcie.dma_bus_base +
              (base - h->pi4_pcie.dma_cpu_base);
        if (bus > UINT64_MAX - bytes || bus + bytes > dma_bus_end) continue;

        before = base > entry->base;
        after = end < entry_end;
        extra = before ? (after ? 2u : 1u) : (after ? 1u : 0u);
        if (extra > ZEAL_MEM_MAX - h->mem_count)
            return -2;

        if (before) {
            entry->length = base - entry->base;
            h->mem[h->mem_count++] = (struct zeal_mem_entry){
                .base = base, .length = bytes, .type = ZEAL_MEM_USB_DMA
            };
            if (after) {
                h->mem[h->mem_count++] = (struct zeal_mem_entry){
                    .base = end, .length = entry_end - end, .type = ZEAL_MEM_USABLE
                };
            }
        } else {
            entry->base = base;
            entry->length = bytes;
            entry->type = ZEAL_MEM_USB_DMA;
            if (after) {
                h->mem[h->mem_count++] = (struct zeal_mem_entry){
                    .base = end, .length = entry_end - end, .type = ZEAL_MEM_USABLE
                };
            }
        }
        h->pi4_usb_dma.flags = PI4_USB_DMA_RESERVED;
        h->pi4_usb_dma.phys_base = base;
        h->pi4_usb_dma.bus_base = bus;
        h->pi4_usb_dma.size = bytes;
        return 0;
    }
    return -3;
}
