/* Read-only BCM2711 PCIe host description from a firmware FDT. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "handoff.h"

#define PI4_FDT_MAX_DEPTH 16u

struct pi4_fdt_node {
    unsigned address_cells;
    unsigned size_cells;
    const uint8_t *reg;
    uint32_t reg_size;
    const uint8_t *ranges;
    uint32_t ranges_size;
    const uint8_t *dma_ranges;
    uint32_t dma_ranges_size;
    const uint8_t *compatible;
    uint32_t compatible_size;
    const uint8_t *status;
    uint32_t status_size;
    unsigned have_ranges;
    unsigned have_dma_ranges;
    unsigned have_status;
};

static uint32_t pi4_fdt_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static int pi4_fdt_span(uint32_t off, uint32_t len, uint32_t total) {
    return off <= total && len <= total - off;
}

static int pi4_fdt_cells(const uint8_t *p, unsigned n, uint64_t *out) {
    uint64_t value = 0;
    if (!p || !out || n == 0 || n > 2) return 0;
    for (unsigned i = 0; i < n; i++) value = (value << 32) | pi4_fdt_be32(p + i * 4);
    *out = value;
    return 1;
}

static int pi4_fdt_string_eq(const uint8_t *p, uint32_t n, const char *want) {
    uint32_t i = 0;
    if (!p || !want) return 0;
    while (i < n && p[i] && want[i] && p[i] == (uint8_t)want[i]) i++;
    return i < n && p[i] == 0 && want[i] == 0;
}

static int pi4_fdt_string_list_has(const uint8_t *p, uint32_t n, const char *want) {
    uint32_t at = 0;
    while (p && at < n) {
        uint32_t end = at;
        while (end < n && p[end]) end++;
        if (end == n) return 0;
        if (pi4_fdt_string_eq(p + at, end - at + 1, want)) return 1;
        at = end + 1;
    }
    return 0;
}

static int pi4_fdt_property_name(const uint8_t *strings, uint32_t strings_size,
                                 uint32_t nameoff, const char *want) {
    uint32_t i = 0;
    if (nameoff >= strings_size) return 0;
    while (nameoff + i < strings_size && strings[nameoff + i] && want[i] &&
           strings[nameoff + i] == (uint8_t)want[i]) i++;
    return nameoff + i < strings_size && strings[nameoff + i] == 0 && want[i] == 0;
}

static int pi4_fdt_translate_reg(const struct pi4_fdt_node *stack, unsigned depth,
                                 uint64_t *address) {
    /* Translate child reg addresses through each simple-bus ranges window. */
    for (unsigned i = depth - 2; i > 0; i--) {
        const struct pi4_fdt_node *bus = &stack[i];
        const struct pi4_fdt_node *parent = &stack[i - 1];
        unsigned child_cells = bus->address_cells;
        unsigned parent_cells = parent->address_cells;
        unsigned size_cells = bus->size_cells;
        uint64_t child_base, parent_base, size;
        uint32_t stride;
        int matched = 0;

        if (!bus->have_ranges) return 0;
        if (bus->ranges_size == 0) continue; /* An explicit empty ranges is identity. */
        if (child_cells == 0 || child_cells > 2 || parent_cells == 0 || parent_cells > 2 ||
            size_cells == 0 || size_cells > 2) return 0;
        stride = (child_cells + parent_cells + size_cells) * 4;
        if (!stride || bus->ranges_size % stride) return 0;
        for (uint32_t off = 0; off < bus->ranges_size; off += stride) {
            const uint8_t *tuple = bus->ranges + off;
            if (!pi4_fdt_cells(tuple, child_cells, &child_base) ||
                !pi4_fdt_cells(tuple + child_cells * 4, parent_cells, &parent_base) ||
                !pi4_fdt_cells(tuple + (child_cells + parent_cells) * 4,
                               size_cells, &size) || size == 0 || child_base > UINT64_MAX - size)
                return 0;
            if (*address >= child_base && *address - child_base < size) {
                uint64_t delta = *address - child_base;
                if (parent_base > UINT64_MAX - delta) return 0;
                *address = parent_base + delta;
                matched = 1;
                break;
            }
        }
        if (!matched) return 0;
    }
    return 1;
}

static int pi4_fdt_pci_window(const uint8_t *ranges, uint32_t ranges_size,
                              unsigned parent_address_cells, uint64_t *bus_base,
                              uint64_t *cpu_base, uint64_t *window_size) {
    const unsigned child_cells = 3; /* PCI address-space, high and low address. */
    const unsigned size_cells = 2;
    uint32_t stride;
    if (!ranges || parent_address_cells == 0 || parent_address_cells > 2) return 0;
    stride = (child_cells + parent_address_cells + size_cells) * 4;
    if (!stride || ranges_size % stride) return 0;
    for (uint32_t off = 0; off < ranges_size; off += stride) {
        const uint8_t *tuple = ranges + off;
        uint32_t hi = pi4_fdt_be32(tuple);
        uint64_t child, parent, size;
        /* PCI space code 2 is non-prefetchable or prefetchable memory. */
        if (((hi >> 24) & 3u) != 2u) continue;
        if (!pi4_fdt_cells(tuple + 4, 2, &child) ||
            !pi4_fdt_cells(tuple + child_cells * 4, parent_address_cells, &parent) ||
            !pi4_fdt_cells(tuple + (child_cells + parent_address_cells) * 4,
                           size_cells, &size) || size == 0 ||
            child > UINT64_MAX - size || parent > UINT64_MAX - size) return 0;
        *bus_base = child;
        *cpu_base = parent;
        *window_size = size;
        return 1;
    }
    return 0;
}

static int pi4_fdt_decode_pcie_node(const struct pi4_fdt_node *stack, unsigned depth,
                                    struct zeal_pi4_pcie *out) {
    const struct pi4_fdt_node *node = &stack[depth - 1];
    const struct pi4_fdt_node *parent = depth > 1 ? &stack[depth - 2] : NULL;
    uint64_t address, size;
    unsigned address_cells;

    if (!pi4_fdt_string_list_has(node->compatible, node->compatible_size,
                                 "brcm,bcm2711-pcie")) return 0;
    out->flags = PI4_PCIE_FDT_PRESENT;
    if (!node->have_status || pi4_fdt_string_eq(node->status, node->status_size, "okay") ||
        pi4_fdt_string_eq(node->status, node->status_size, "ok"))
        out->flags |= PI4_PCIE_FDT_ENABLED;

    if (parent && node->reg && parent->address_cells > 0 && parent->address_cells <= 2 &&
        parent->size_cells > 0 && parent->size_cells <= 2) {
        uint32_t reg_stride = (parent->address_cells + parent->size_cells) * 4;
        if (reg_stride && node->reg_size >= reg_stride &&
            pi4_fdt_cells(node->reg, parent->address_cells, &address) &&
            pi4_fdt_cells(node->reg + parent->address_cells * 4, parent->size_cells, &size) &&
            size && pi4_fdt_translate_reg(stack, depth, &address) &&
            address <= UINT64_MAX - size) {
            out->reg_base = address;
            out->reg_size = size;
            out->flags |= PI4_PCIE_FDT_REG;
        }
    }
    address_cells = parent ? parent->address_cells : 0;
    if (node->ranges && pi4_fdt_pci_window(node->ranges, node->ranges_size, address_cells,
                                          &out->mem_bus_base, &out->mem_cpu_base,
                                          &out->mem_size))
        out->flags |= PI4_PCIE_FDT_MEM;
    if (node->dma_ranges &&
        pi4_fdt_pci_window(node->dma_ranges, node->dma_ranges_size, address_cells,
                           &out->dma_bus_base, &out->dma_cpu_base, &out->dma_size))
        out->flags |= PI4_PCIE_FDT_DMA;
    return 1;
}

/*
 * Parse the first BCM2711 PCIe host bridge from a Limine FDT. This does not
 * touch MMIO or enable PCIe: the returned addresses/windows are descriptive
 * inputs for a later, separately validated host-controller backend.
 */
static int pi4_fdt_find_pcie(const void *blob, size_t available,
                             struct zeal_pi4_pcie *out) {
    const uint8_t *fdt = (const uint8_t *)blob;
    const uint8_t *structure, *strings;
    uint32_t total, off_struct, size_struct, off_strings, size_strings;
    uint32_t cursor = 0;
    struct pi4_fdt_node stack[PI4_FDT_MAX_DEPTH];
    unsigned depth = 0;
    int found = 0;

    if (!fdt || !out || available < 40 || pi4_fdt_be32(fdt) != 0xd00dfeedu) return -1;
    total = pi4_fdt_be32(fdt + 4);
    off_struct = pi4_fdt_be32(fdt + 8);
    off_strings = pi4_fdt_be32(fdt + 12);
    size_strings = pi4_fdt_be32(fdt + 32);
    size_struct = pi4_fdt_be32(fdt + 36);
    if (total < 40 || total > available ||
        !pi4_fdt_span(off_struct, size_struct, total) ||
        !pi4_fdt_span(off_strings, size_strings, total)) return -2;
    structure = fdt + off_struct;
    strings = fdt + off_strings;
    out->flags = 0;
    out->reg_base = out->reg_size = 0;
    out->mem_bus_base = out->mem_cpu_base = out->mem_size = 0;
    out->dma_bus_base = out->dma_cpu_base = out->dma_size = 0;

    while (cursor + 4 <= size_struct) {
        uint32_t token = pi4_fdt_be32(structure + cursor);
        cursor += 4;
        if (token == 1) { /* FDT_BEGIN_NODE */
            uint32_t name_end = cursor;
            if (depth >= PI4_FDT_MAX_DEPTH) return -3;
            while (name_end < size_struct && structure[name_end]) name_end++;
            if (name_end >= size_struct) return -4;
            cursor = (name_end + 4) & ~3u;
            stack[depth] = (struct pi4_fdt_node){.address_cells = 2, .size_cells = 1};
            depth++;
        } else if (token == 2) { /* FDT_END_NODE */
            if (!depth) return -5;
            if (!found && pi4_fdt_decode_pcie_node(stack, depth, out)) found = 1;
            depth--;
        } else if (token == 3) { /* FDT_PROP */
            uint32_t len, nameoff;
            const uint8_t *data;
            struct pi4_fdt_node *node;
            if (!depth || cursor + 8 > size_struct) return -6;
            len = pi4_fdt_be32(structure + cursor);
            nameoff = pi4_fdt_be32(structure + cursor + 4);
            cursor += 8;
            if (len > size_struct - cursor || nameoff >= size_strings) return -7;
            data = structure + cursor;
            node = &stack[depth - 1];
            if (pi4_fdt_property_name(strings, size_strings, nameoff, "#address-cells")) {
                if (len != 4) return -8;
                node->address_cells = pi4_fdt_be32(data);
            } else if (pi4_fdt_property_name(strings, size_strings, nameoff, "#size-cells")) {
                if (len != 4) return -9;
                node->size_cells = pi4_fdt_be32(data);
            } else if (pi4_fdt_property_name(strings, size_strings, nameoff, "reg")) {
                node->reg = data; node->reg_size = len;
            } else if (pi4_fdt_property_name(strings, size_strings, nameoff, "ranges")) {
                node->ranges = data; node->ranges_size = len; node->have_ranges = 1;
            } else if (pi4_fdt_property_name(strings, size_strings, nameoff, "dma-ranges")) {
                node->dma_ranges = data; node->dma_ranges_size = len;
                node->have_dma_ranges = 1;
            } else if (pi4_fdt_property_name(strings, size_strings, nameoff, "compatible")) {
                node->compatible = data; node->compatible_size = len;
            } else if (pi4_fdt_property_name(strings, size_strings, nameoff, "status")) {
                node->status = data; node->status_size = len; node->have_status = 1;
            }
            cursor = (cursor + len + 3u) & ~3u;
        } else if (token == 4) { /* FDT_NOP */
            continue;
        } else if (token == 9) { /* FDT_END */
            if (depth != 0) return -10;
            return found ? 0 : -13;
        } else {
            return -11;
        }
    }
    return -12;
}
