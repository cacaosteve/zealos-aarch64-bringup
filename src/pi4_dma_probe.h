/* Validate the reserved Pi DMA arena before any device is allowed to use it. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "pi4_dma.h"

#define PI4_DMA_PROBE_PLAN_OK       0
#define PI4_DMA_PROBE_PLAN_HANDOFF -1
#define PI4_DMA_PROBE_PLAN_ARENA   -2
#define PI4_DMA_PROBE_PLAN_WINDOW  -3
#define PI4_DMA_PROBE_PLAN_MEMMAP  -4

struct pi4_dma_probe_plan {
    uint64_t phys_base;
    uint64_t bus_base;
    uint64_t size;
};

/* Pure validation only: no MMIO, cache operation, or bus mastering. */
static int pi4_dma_probe_plan_make(const struct zeal_handoff *h,
                                   struct pi4_dma_probe_plan *out) {
    const uint32_t required = PI4_PCIE_FDT_PRESENT | PI4_PCIE_FDT_ENABLED |
                              PI4_PCIE_FDT_DMA;
    uint64_t phys_end, bus_end, dma_cpu_end, dma_bus_end;
    unsigned matches = 0;

    if (!h || !out || h->revision < 6 ||
        h->size < offsetof(struct zeal_handoff, pi4_usb_dma) +
                  sizeof(h->pi4_usb_dma) ||
        h->mem_count > ZEAL_MEM_MAX)
        return PI4_DMA_PROBE_PLAN_HANDOFF;
    if (h->pi4_usb_dma.flags != PI4_USB_DMA_RESERVED ||
        h->pi4_usb_dma.size != PI4_USB_DMA_ARENA_BYTES ||
        (h->pi4_usb_dma.phys_base & (PI4_USB_DMA_ARENA_ALIGN - 1)) ||
        (h->pi4_usb_dma.bus_base & (PI4_USB_DMA_ARENA_ALIGN - 1)) ||
        h->pi4_usb_dma.phys_base > UINT64_MAX - h->pi4_usb_dma.size ||
        h->pi4_usb_dma.bus_base > UINT64_MAX - h->pi4_usb_dma.size)
        return PI4_DMA_PROBE_PLAN_ARENA;
    if ((h->pi4_pcie.flags & required) != required ||
        !h->pi4_pcie.dma_size ||
        h->pi4_pcie.dma_cpu_base > UINT64_MAX - h->pi4_pcie.dma_size ||
        h->pi4_pcie.dma_bus_base > UINT64_MAX - h->pi4_pcie.dma_size)
        return PI4_DMA_PROBE_PLAN_WINDOW;

    phys_end = h->pi4_usb_dma.phys_base + h->pi4_usb_dma.size;
    bus_end = h->pi4_usb_dma.bus_base + h->pi4_usb_dma.size;
    dma_cpu_end = h->pi4_pcie.dma_cpu_base + h->pi4_pcie.dma_size;
    dma_bus_end = h->pi4_pcie.dma_bus_base + h->pi4_pcie.dma_size;
    if (h->pi4_usb_dma.phys_base < h->pi4_pcie.dma_cpu_base ||
        phys_end > dma_cpu_end ||
        h->pi4_usb_dma.bus_base < h->pi4_pcie.dma_bus_base ||
        bus_end > dma_bus_end ||
        h->pi4_usb_dma.phys_base - h->pi4_pcie.dma_cpu_base !=
        h->pi4_usb_dma.bus_base - h->pi4_pcie.dma_bus_base)
        return PI4_DMA_PROBE_PLAN_WINDOW;

    for (uint32_t i = 0; i < h->mem_count; i++) {
        const struct zeal_mem_entry *entry = &h->mem[i];
        uint64_t entry_end;
        if (!entry->length || entry->base > UINT64_MAX - entry->length)
            return PI4_DMA_PROBE_PLAN_MEMMAP;
        entry_end = entry->base + entry->length;
        if (entry->base < phys_end && h->pi4_usb_dma.phys_base < entry_end) {
            if (entry->type != ZEAL_MEM_USB_DMA ||
                entry->base != h->pi4_usb_dma.phys_base ||
                entry->length != h->pi4_usb_dma.size)
                return PI4_DMA_PROBE_PLAN_MEMMAP;
            matches++;
        }
    }
    if (matches != 1) return PI4_DMA_PROBE_PLAN_MEMMAP;

    out->phys_base = h->pi4_usb_dma.phys_base;
    out->bus_base = h->pi4_usb_dma.bus_base;
    out->size = h->pi4_usb_dma.size;
    return PI4_DMA_PROBE_PLAN_OK;
}
