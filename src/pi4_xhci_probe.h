/* Safe planning helpers for a read-only BCM2711/VL805 xHCI capability probe. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "handoff.h"

#define PI4_XHCI_CAP_BYTES       0x20u
#define PI4_PCI_COMMAND_MEMORY   (1u << 1)
#define PI4_PCI_COMMAND_MASTER   (1u << 2)

struct pi4_xhci_bar_plan {
    uint64_t bus_base;
    uint64_t cpu_base;
};

/*
 * Translate the firmware-assigned 64-bit PCI memory BAR through the FDT
 * aperture. The capability header (0x00..0x1f) must fit entirely in the
 * aperture. This never sizes, relocates, or writes the BAR.
 */
static int pi4_xhci_bar_plan(const struct zeal_handoff *h,
                             uint32_t bar_low, uint32_t bar_high,
                             struct pi4_xhci_bar_plan *out) {
    const uint32_t required = PI4_PCIE_FDT_PRESENT | PI4_PCIE_FDT_ENABLED |
                              PI4_PCIE_FDT_MEM;
    uint64_t bus_base, delta, cpu_base;

    if (!h || !out || h->revision < 5 ||
        h->size < offsetof(struct zeal_handoff, pi4_pcie) + sizeof(h->pi4_pcie) ||
        (h->pi4_pcie.flags & required) != required ||
        !h->pi4_pcie.mem_size ||
        h->pi4_pcie.mem_bus_base > UINT64_MAX - h->pi4_pcie.mem_size ||
        h->pi4_pcie.mem_cpu_base > UINT64_MAX - h->pi4_pcie.mem_size)
        return -1;

    /* I/O BARs and 32-bit BARs are not accepted for this endpoint probe. */
    if ((bar_low & 1u) || (bar_low & 6u) != 4u) return -2;
    bus_base = ((uint64_t)bar_high << 32) | (bar_low & ~0x0full);
    if (!bus_base || (bus_base & 3u) || bus_base < h->pi4_pcie.mem_bus_base)
        return -3;

    delta = bus_base - h->pi4_pcie.mem_bus_base;
    if (delta > h->pi4_pcie.mem_size ||
        PI4_XHCI_CAP_BYTES > h->pi4_pcie.mem_size - delta ||
        h->pi4_pcie.mem_cpu_base > UINT64_MAX - delta)
        return -4;
    cpu_base = h->pi4_pcie.mem_cpu_base + delta;
    if (cpu_base >= (1ull << 48) ||
        PI4_XHCI_CAP_BYTES > (1ull << 48) - cpu_base)
        return -5;

    out->bus_base = bus_base;
    out->cpu_base = cpu_base;
    return 0;
}

/*
 * Permit CPU MMIO capability reads only when firmware has not enabled PCI
 * bus mastering. Preserve other command bits and add Memory Space Enable;
 * the caller must restore the original low 16-bit command word afterward.
 */
static int pi4_xhci_memory_command(uint16_t original, uint16_t *enabled) {
    if (!enabled || (original & PI4_PCI_COMMAND_MASTER)) return -1;
    *enabled = original | PI4_PCI_COMMAND_MEMORY;
    return 0;
}

/* Reject reads that look like an unmapped or poisoned BAR rather than xHCI. */
static int pi4_xhci_caps_valid(const uint32_t caps[6]) {
    uint32_t cap_length, version, max_slots, max_ports;

    if (!caps) return 0;
    cap_length = caps[0] & 0xffu;
    version = caps[0] >> 16;
    max_slots = caps[1] & 0xffu;
    max_ports = (caps[1] >> 24) & 0xffu;
    if (cap_length < 0x20u || cap_length > 0xfcu || (cap_length & 3u) ||
        version < 0x0100u || version > 0x01ffu || !max_slots || !max_ports ||
        (caps[4] & 3u) || (caps[5] & 0xfu))
        return 0;
    return 1;
}
