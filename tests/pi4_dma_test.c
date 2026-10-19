#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/pi4_dma.h"

static void init(struct zeal_handoff *h) {
    memset(h, 0, sizeof(*h));
    h->pi4_pcie.flags = PI4_PCIE_FDT_PRESENT | PI4_PCIE_FDT_ENABLED |
                        PI4_PCIE_FDT_DMA;
    h->pi4_pcie.dma_cpu_base = 0x02000000ull;
    h->pi4_pcie.dma_bus_base = 0x40000000ull;
    h->pi4_pcie.dma_size = 0x10000000ull;
}

static int fail(const char *name) {
    fprintf(stderr, "FAIL Pi USB DMA: %s\n", name);
    return 1;
}

int main(void) {
    struct zeal_handoff h, before;

    /* Select an aligned subrange and preserve both sides of usable RAM. */
    init(&h);
    h.mem_count = 1;
    h.mem[0] = (struct zeal_mem_entry){
        .base = 0x04001000ull, .length = 0x00200000ull, .type = ZEAL_MEM_USABLE
    };
    if (pi4_usb_dma_reserve(&h) != 0 || h.pi4_usb_dma.flags != PI4_USB_DMA_RESERVED ||
        h.pi4_usb_dma.phys_base != 0x04010000ull ||
        h.pi4_usb_dma.bus_base != 0x42010000ull ||
        h.pi4_usb_dma.size != PI4_USB_DMA_ARENA_BYTES || h.mem_count != 3 ||
        h.mem[0].base != 0x04001000ull || h.mem[0].length != 0xf000ull ||
        h.mem[1].base != 0x04010000ull ||
        h.mem[1].length != PI4_USB_DMA_ARENA_BYTES ||
        h.mem[1].type != ZEAL_MEM_USB_DMA || h.mem[2].base != 0x04110000ull ||
        h.mem[2].length != 0x000f1000ull || h.mem[2].type != ZEAL_MEM_USABLE)
        return fail("reservation, split, or bus translation mismatch");

    /* A whole-entry reservation needs no extra memmap slot. */
    init(&h);
    h.mem_count = ZEAL_MEM_MAX;
    h.mem[0] = (struct zeal_mem_entry){
        .base = 0x04000000ull,
        .length = PI4_USB_DMA_ARENA_BYTES,
        .type = ZEAL_MEM_USABLE
    };
    if (pi4_usb_dma_reserve(&h) != 0 || h.mem_count != ZEAL_MEM_MAX ||
        h.mem[0].type != ZEAL_MEM_USB_DMA)
        return fail("in-place reservation at memmap capacity");

    /* Missing/disabled firmware windows must leave the handoff untouched. */
    init(&h);
    h.mem_count = 1;
    h.mem[0] = (struct zeal_mem_entry){
        .base = 0x04000000ull, .length = 0x00200000ull, .type = ZEAL_MEM_USABLE
    };
    h.pi4_pcie.flags &= ~PI4_PCIE_FDT_ENABLED;
    before = h;
    if (pi4_usb_dma_reserve(&h) != -1 || memcmp(&h, &before, sizeof(h)) != 0)
        return fail("disabled host was not rejected without mutation");

    /* No suitable usable memory, overflowing windows, and a full map fail closed. */
    init(&h);
    h.mem_count = 1;
    h.mem[0] = (struct zeal_mem_entry){
        .base = 0x100000000ull, .length = 0x00200000ull, .type = ZEAL_MEM_USABLE
    };
    before = h;
    if (pi4_usb_dma_reserve(&h) != -3 || memcmp(&h, &before, sizeof(h)) != 0)
        return fail("out-of-window memory was accepted");

    init(&h);
    h.pi4_pcie.dma_cpu_base = UINT64_MAX - 0x7fffull;
    h.pi4_pcie.dma_size = 0x10000ull;
    if (pi4_usb_dma_reserve(&h) != -1)
        return fail("overflowing CPU DMA aperture was accepted");

    init(&h);
    h.pi4_pcie.dma_bus_base = UINT64_MAX - 0x7fffull;
    h.pi4_pcie.dma_size = 0x10000ull;
    if (pi4_usb_dma_reserve(&h) != -1)
        return fail("overflowing bus DMA aperture was accepted");

    init(&h);
    h.mem_count = ZEAL_MEM_MAX;
    h.mem[0] = (struct zeal_mem_entry){
        .base = 0x04000000ull,
        .length = 2 * PI4_USB_DMA_ARENA_BYTES,
        .type = ZEAL_MEM_USABLE
    };
    if (pi4_usb_dma_reserve(&h) != -2)
        return fail("memmap split beyond capacity was accepted");

    init(&h);
    h.mem_count = ZEAL_MEM_MAX + 1;
    if (pi4_usb_dma_reserve(&h) != -1)
        return fail("oversized memory map count was not rejected");

    puts("PASS Pi USB DMA arena bounds, split, translation and capacity");
    return 0;
}
