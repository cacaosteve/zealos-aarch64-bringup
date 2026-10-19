#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/pi4_pcie_fdt.h"

#define TEST_BLOB_BYTES 4096
#define TEST_STRUCT_OFF 56

static uint8_t blob[TEST_BLOB_BYTES];
static uint8_t names[] = "#address-cells\0#size-cells\0ranges\0compatible\0reg\0dma-ranges\0status\0";
static uint8_t *structure;
static uint32_t structure_size;

static void put_be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

static uint32_t string_offset(const char *s) {
    uint32_t at = 0;
    while (at < sizeof(names)) {
        if (!strcmp((const char *)names + at, s)) return at;
        at += (uint32_t)strlen((const char *)names + at) + 1;
    }
    return UINT32_MAX;
}

static void token(uint32_t value) {
    put_be32(structure + structure_size, value);
    structure_size += 4;
}

static void begin_node(const char *name) {
    size_t n = strlen(name) + 1;
    token(1);
    memcpy(structure + structure_size, name, n);
    structure_size += (uint32_t)n;
    while (structure_size & 3u) structure[structure_size++] = 0;
}

static void end_node(void) { token(2); }

static void property(const char *name, const uint32_t *cells, unsigned count) {
    uint32_t len = count * 4;
    token(3);
    put_be32(structure + structure_size, len);
    put_be32(structure + structure_size + 4, string_offset(name));
    structure_size += 8;
    for (unsigned i = 0; i < count; i++) {
        put_be32(structure + structure_size, cells[i]);
        structure_size += 4;
    }
}

static void property_string(const char *name, const char *value) {
    uint32_t len = (uint32_t)strlen(value) + 1;
    token(3);
    put_be32(structure + structure_size, len);
    put_be32(structure + structure_size + 4, string_offset(name));
    structure_size += 8;
    memcpy(structure + structure_size, value, len);
    structure_size += len;
    while (structure_size & 3u) structure[structure_size++] = 0;
}

static size_t make_tree(const char *status) {
    static const uint32_t root_ac[] = {2}, root_sc[] = {1};
    static const uint32_t scb_ac[] = {2}, scb_sc[] = {1};
    static const uint32_t scb_ranges[] = {
        0x00000000, 0x7c000000, 0x00000000, 0xfc000000, 0x03800000
    };
    static const uint32_t reg[] = {0x00000000, 0x7d500000, 0x00009310};
    static const uint32_t pcie_ac[] = {3}, pcie_sc[] = {2};
    static const uint32_t ranges[] = {
        0x02000000, 0x00000000, 0xf8000000,
        0x00000006, 0x00000000, 0x00000000, 0x04000000
    };
    static const uint32_t dma_ranges[] = {
        0x02000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0xc0000000
    };
    uint32_t off_strings, total;

    memset(blob, 0, sizeof(blob));
    structure = blob + TEST_STRUCT_OFF;
    structure_size = 0;
    begin_node("");
    property("#address-cells", root_ac, 1);
    property("#size-cells", root_sc, 1);
    begin_node("scb");
    property("#address-cells", scb_ac, 1);
    property("#size-cells", scb_sc, 1);
    property("ranges", scb_ranges, 5);
    begin_node("pcie@7d500000");
    property_string("compatible", "brcm,bcm2711-pcie");
    property("reg", reg, 3);
    property("#address-cells", pcie_ac, 1);
    property("#size-cells", pcie_sc, 1);
    property("ranges", ranges, 7);
    property("dma-ranges", dma_ranges, 7);
    if (status) property_string("status", status);
    end_node(); end_node(); end_node(); token(9);

    off_strings = TEST_STRUCT_OFF + structure_size;
    memcpy(blob + off_strings, names, sizeof(names));
    total = off_strings + (uint32_t)sizeof(names);
    put_be32(blob, 0xd00dfeedu);
    put_be32(blob + 4, total);
    put_be32(blob + 8, TEST_STRUCT_OFF);
    put_be32(blob + 12, off_strings);
    put_be32(blob + 16, 40); /* reserve map follows the v17 header */
    put_be32(blob + 20, 17);
    put_be32(blob + 24, 16);
    put_be32(blob + 28, 0);
    put_be32(blob + 32, sizeof(names));
    put_be32(blob + 36, structure_size);
    return total;
}

int main(void) {
    struct zeal_pi4_pcie pcie;
    size_t size = make_tree("okay");
    if (pi4_fdt_find_pcie(blob, size, &pcie) != 0 ||
        pcie.flags != (PI4_PCIE_FDT_PRESENT | PI4_PCIE_FDT_ENABLED |
                       PI4_PCIE_FDT_REG | PI4_PCIE_FDT_MEM | PI4_PCIE_FDT_DMA) ||
        pcie.reg_base != 0xfd500000ull || pcie.reg_size != 0x9310ull ||
        pcie.mem_bus_base != 0xf8000000ull || pcie.mem_cpu_base != 0x600000000ull ||
        pcie.mem_size != 0x04000000ull || pcie.dma_bus_base != 0 ||
        pcie.dma_cpu_base != 0 || pcie.dma_size != 0xc0000000ull) {
        fprintf(stderr, "BCM2711 DTB decode mismatch: flags=%x reg=%llx/%llx mem=%llx->%llx/%llx dma=%llx->%llx/%llx\n",
                pcie.flags, (unsigned long long)pcie.reg_base,
                (unsigned long long)pcie.reg_size,
                (unsigned long long)pcie.mem_bus_base,
                (unsigned long long)pcie.mem_cpu_base,
                (unsigned long long)pcie.mem_size,
                (unsigned long long)pcie.dma_bus_base,
                (unsigned long long)pcie.dma_cpu_base,
                (unsigned long long)pcie.dma_size);
        return 1;
    }
    size = make_tree("disabled");
    if (pi4_fdt_find_pcie(blob, size, &pcie) != 0 ||
        !(pcie.flags & PI4_PCIE_FDT_PRESENT) || (pcie.flags & PI4_PCIE_FDT_ENABLED)) {
        fprintf(stderr, "disabled PCIe node was not rejected\n");
        return 2;
    }
    size = make_tree("okay");
    if (pi4_fdt_find_pcie(blob, size - 1, &pcie) == 0) {
        fprintf(stderr, "truncated FDT was accepted\n");
        return 3;
    }
    puts("PASS BCM2711 FDT host registers, PCI memory window and 3 GiB DMA aperture");
    return 0;
}
