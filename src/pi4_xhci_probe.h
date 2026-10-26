/* Safe planning helpers for a read-only BCM2711/VL805 xHCI capability probe. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "handoff.h"

#define PI4_XHCI_CAP_BYTES       0x20u
#define PI4_XHCI_MMIO_BYTES      0x200000u
#define PI4_XHCI_EXT_CAP_BYTES   0x40000u
#define PI4_XHCI_DIAG_PORTS      8u
#define PI4_XHCI_PORTSC_BASE     0x400u
#define PI4_XHCI_PORTSC_STRIDE   0x10u
#define PI4_PCI_COMMAND_MEMORY   (1u << 1)
#define PI4_PCI_COMMAND_MASTER   (1u << 2)

#define PI4_XHCI_STATE_OP_REGS   (1u << 0)
#define PI4_XHCI_STATE_LEGACY    (1u << 1)
#define PI4_XHCI_STATE_NO_LEGACY (1u << 2)
#define PI4_XHCI_STATE_READ_FAIL (1u << 3)
#define PI4_XHCI_STATE_PORTS     (1u << 4)
#define PI4_XHCI_STATE_HANDOFF_OK (1u << 5)
#define PI4_XHCI_STATE_HANDOFF_TIMEOUT (1u << 6)
#define PI4_XHCI_STATE_HANDOFF_UNSAFE (1u << 7)
#define PI4_XHCI_STATE_HANDOFF_FAIL (1u << 8)
#define PI4_XHCI_STATE_RESET_OK (1u << 9)
#define PI4_XHCI_STATE_RESET_UNSAFE (1u << 10)
#define PI4_XHCI_STATE_RESET_TIMEOUT (1u << 11)
#define PI4_XHCI_STATE_RESET_FAIL (1u << 12)

#define PI4_XHCI_LEGACY_BIOS_OWNED (1u << 16)
#define PI4_XHCI_LEGACY_OS_OWNED   (1u << 24)
#define PI4_XHCI_USBCMD_RUN        (1u << 0)
#define PI4_XHCI_USBCMD_HCRST      (1u << 1)
#define PI4_XHCI_USBCMD_INTE       (1u << 2)
#define PI4_XHCI_USBCMD_EWE        (1u << 10)
#define PI4_XHCI_USBSTS_HALTED     (1u << 0)
#define PI4_XHCI_USBSTS_HSE        (1u << 2)
#define PI4_XHCI_USBSTS_PCD        (1u << 4)
#define PI4_XHCI_USBSTS_CNR        (1u << 11)

struct pi4_xhci_bar_plan {
    uint64_t bus_base;
    uint64_t cpu_base;
};

struct pi4_xhci_outbound_window {
    uint64_t bus_base;
    uint64_t cpu_base;
    uint64_t size;
};

struct pi4_xhci_state_plan {
    uint32_t usbcmd_offset;
    uint32_t usbsts_offset;
    uint32_t first_ext_cap_offset;
};

struct pi4_xhci_port_plan {
    uint32_t first_portsc_offset;
    uint32_t count;
    uint32_t total_ports;
};

/* Preserve the read-only capability fields while requesting OS ownership. */
static int pi4_xhci_legacy_claim_value(uint32_t capability,
                                       uint32_t *request) {
    if (!request || (capability & 0xffu) != 1u) return -1;
    *request = capability | PI4_XHCI_LEGACY_OS_OWNED;
    return 0;
}

/* Do not touch ownership while the controller is running or not halted. */
static int pi4_xhci_handoff_safe(uint32_t usbcmd, uint32_t usbsts) {
    return !(usbcmd & PI4_XHCI_USBCMD_RUN) &&
           (usbsts & PI4_XHCI_USBSTS_HALTED);
}

/* Require a completed legacy handoff and a ready, halted controller before HCRST. */
static int pi4_xhci_reset_safe(uint32_t usbcmd, uint32_t usbsts,
                               uint32_t legacy, uint16_t pci_command) {
    return !(pci_command & PI4_PCI_COMMAND_MASTER) &&
           !(usbcmd & (PI4_XHCI_USBCMD_RUN | PI4_XHCI_USBCMD_HCRST)) &&
           (usbsts & PI4_XHCI_USBSTS_HALTED) &&
           !(usbsts & PI4_XHCI_USBSTS_CNR) &&
           !(legacy & PI4_XHCI_LEGACY_BIOS_OWNED) &&
           (legacy & PI4_XHCI_LEGACY_OS_OWNED);
}

/* Decode the BCM2711 CPU-to-PCIe memory window 0 registers. */
static int pi4_xhci_outbound_window_decode(uint32_t bus_lo, uint32_t bus_hi,
                                           uint32_t base_limit,
                                           uint32_t base_hi,
                                           uint32_t limit_hi,
                                           struct pi4_xhci_outbound_window *out) {
    const uint64_t mb = 1ull << 20;
    uint64_t base_mb, limit_mb, size_mb;

    if (!out || (base_hi & ~0xffu) || (limit_hi & ~0xffu)) return -1;
    base_mb = ((uint64_t)base_hi << 12) | ((base_limit & 0x0000fff0u) >> 4);
    limit_mb = ((uint64_t)limit_hi << 12) | ((base_limit & 0xfff00000u) >> 20);
    if (limit_mb < base_mb) return -2;
    size_mb = limit_mb - base_mb + 1;
    if (size_mb > UINT64_MAX / mb || base_mb > UINT64_MAX / mb) return -3;

    out->bus_base = ((uint64_t)bus_hi << 32) | bus_lo;
    out->cpu_base = base_mb * mb;
    out->size = size_mb * mb;
    if (!out->size || out->bus_base > UINT64_MAX - (out->size - 1) ||
        out->cpu_base > UINT64_MAX - (out->size - 1))
        return -4;
    return 0;
}

/*
 * Translate the firmware-assigned PCI memory BAR through the live BCM2711
 * outbound window. The FDT aperture bounds the PCI bus addresses, but firmware
 * may have programmed a narrower window with a different CPU target. The
 * capability header (0x00..0x1f) must fit both ranges. This never sizes,
 * relocates, or writes the BAR.
 */
static int pi4_xhci_bar_plan(const struct zeal_handoff *h,
                             uint32_t bar_low, uint32_t bar_high,
                             const struct pi4_xhci_outbound_window *window,
                             struct pi4_xhci_bar_plan *out) {
    const uint32_t required = PI4_PCIE_FDT_PRESENT | PI4_PCIE_FDT_ENABLED |
                              PI4_PCIE_FDT_MEM;
    uint64_t bus_base, fdt_delta, window_delta, cpu_base;

    if (!h || !window || !out || h->revision < 5 ||
        h->size < offsetof(struct zeal_handoff, pi4_pcie) + sizeof(h->pi4_pcie) ||
        (h->pi4_pcie.flags & required) != required ||
        !h->pi4_pcie.mem_size ||
        h->pi4_pcie.mem_bus_base > UINT64_MAX - h->pi4_pcie.mem_size ||
        h->pi4_pcie.mem_cpu_base > UINT64_MAX - h->pi4_pcie.mem_size ||
        !window->size || window->bus_base > UINT64_MAX - (window->size - 1) ||
        window->cpu_base > UINT64_MAX - (window->size - 1))
        return -1;

    /* I/O BARs and 32-bit BARs are not accepted for this endpoint probe. */
    if ((bar_low & 1u) || (bar_low & 6u) != 4u) return -2;
    bus_base = ((uint64_t)bar_high << 32) | (bar_low & ~0x0full);
    if (!bus_base || (bus_base & 3u) || bus_base < h->pi4_pcie.mem_bus_base)
        return -3;

    fdt_delta = bus_base - h->pi4_pcie.mem_bus_base;
    if (fdt_delta > h->pi4_pcie.mem_size ||
        PI4_XHCI_CAP_BYTES > h->pi4_pcie.mem_size - fdt_delta ||
        window->bus_base < h->pi4_pcie.mem_bus_base ||
        window->bus_base - h->pi4_pcie.mem_bus_base > h->pi4_pcie.mem_size ||
        window->size > h->pi4_pcie.mem_size -
                       (window->bus_base - h->pi4_pcie.mem_bus_base))
        return -4;
    if (bus_base < window->bus_base) return -4;
    window_delta = bus_base - window->bus_base;
    if (window_delta > window->size ||
        PI4_XHCI_CAP_BYTES > window->size - window_delta ||
        window->cpu_base > UINT64_MAX - window_delta)
        return -4;
    cpu_base = window->cpu_base + window_delta;
    if (cpu_base >= (1ull << 48) ||
        PI4_XHCI_CAP_BYTES > (1ull << 48) - cpu_base)
        return -5;

    out->bus_base = bus_base;
    out->cpu_base = cpu_base;
    return 0;
}

/*
 * Permit CPU BAR reads only while PCI bus mastering is off. Preserve other
 * command bits and add Memory Space Enable; the caller restores the original
 * low 16-bit command word afterward.
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

/* Plan read-only USBCMD/USBSTS and first extended-capability reads. */
static int pi4_xhci_state_offsets(const uint32_t caps[6],
                                  struct pi4_xhci_state_plan *out) {
    uint32_t cap_length, ecp;

    if (!caps || !out || !pi4_xhci_caps_valid(caps)) return -1;
    cap_length = caps[0] & 0xffu;
    ecp = ((caps[3] >> 16) & 0xffffu) * 4u;
    if (cap_length > PI4_XHCI_MMIO_BYTES - 8u ||
        (ecp && (ecp < cap_length || ecp > PI4_XHCI_EXT_CAP_BYTES - 4u ||
                 (ecp & 3u))))
        return -2;
    out->usbcmd_offset = cap_length;
    out->usbsts_offset = cap_length + 4u;
    out->first_ext_cap_offset = ecp;
    return 0;
}

/* Plan bounded, read-only PORTSC snapshots from the xHCI operational base. */
static int pi4_xhci_portsc_plan(const uint32_t caps[6],
                                struct pi4_xhci_port_plan *out) {
    uint32_t cap_length, total, count, first, last;

    if (!caps || !out || !pi4_xhci_caps_valid(caps)) return -1;
    cap_length = caps[0] & 0xffu;
    total = (caps[1] >> 24) & 0xffu;
    if (!total) return -2;
    count = total < PI4_XHCI_DIAG_PORTS ? total : PI4_XHCI_DIAG_PORTS;
    first = cap_length + PI4_XHCI_PORTSC_BASE;
    last = first + (count - 1u) * PI4_XHCI_PORTSC_STRIDE;
    if (first > PI4_XHCI_MMIO_BYTES - sizeof(uint32_t) ||
        last > PI4_XHCI_MMIO_BYTES - sizeof(uint32_t))
        return -3;
    out->first_portsc_offset = first;
    out->count = count;
    out->total_ports = total;
    return 0;
}

/* Translate a register offset through both firmware's FDT range and the
 * currently programmed outbound window before any guarded MMIO load. */
static int pi4_xhci_bar_offset_cpu(const struct zeal_handoff *h,
                                   uint64_t bar_bus,
                                   const struct pi4_xhci_outbound_window *window,
                                   uint32_t offset, uint32_t bytes,
                                   uint64_t *cpu_address) {
    const uint32_t required = PI4_PCIE_FDT_PRESENT | PI4_PCIE_FDT_ENABLED |
                              PI4_PCIE_FDT_MEM;
    uint64_t fdt_delta, window_delta, access_delta;

    if (!h || !window || !cpu_address || !bytes ||
        h->revision < 5 ||
        h->size < offsetof(struct zeal_handoff, pi4_pcie) + sizeof(h->pi4_pcie) ||
        (h->pi4_pcie.flags & required) != required ||
        !h->pi4_pcie.mem_size || !window->size ||
        h->pi4_pcie.mem_bus_base > UINT64_MAX - h->pi4_pcie.mem_size ||
        window->bus_base > UINT64_MAX - window->size ||
        window->cpu_base > UINT64_MAX - window->size ||
        bar_bus > UINT64_MAX - offset ||
        bar_bus < h->pi4_pcie.mem_bus_base || bar_bus < window->bus_base)
        return -1;
    fdt_delta = bar_bus - h->pi4_pcie.mem_bus_base;
    window_delta = bar_bus - window->bus_base;
    if (fdt_delta > h->pi4_pcie.mem_size ||
        offset > h->pi4_pcie.mem_size - fdt_delta ||
        bytes > h->pi4_pcie.mem_size - fdt_delta - offset ||
        window_delta > window->size ||
        offset > window->size - window_delta ||
        bytes > window->size - window_delta - offset)
        return -3;
    access_delta = window_delta + offset;
    if (window->cpu_base > UINT64_MAX - access_delta) return -4;
    *cpu_address = window->cpu_base + access_delta;
    if (*cpu_address >= (1ull << 48) ||
        bytes > (1ull << 48) - *cpu_address)
        return -5;
    return 0;
}
