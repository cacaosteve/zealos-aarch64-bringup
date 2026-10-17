/* ARM64 USB platform hooks. QEMU virt only; Pi PCIe/VL805 is a separate backend.
 * DMA uses reserved kernel RAM, never a pointer value as a bus address. */
#pragma once
#include <stdint.h>
#include <stddef.h>

#define USB_DMA_BYTES (1024u * 1024u)
static uint8_t usb_dma_memory[USB_DMA_BYTES] __attribute__((aligned(4096)));
static size_t usb_dma_used;
static uint64_t usb_hhdm, usb_counter_hz;

static void usb_zero(void *p, size_t n) {
    uint8_t *b = p;
    while (n--) *b++ = 0;
}
static void usb_copy(void *to, const void *from, size_t n) {
    uint8_t *d = to;
    const uint8_t *s = from;
    while (n--) *d++ = *s++;
}
static uint32_t usb_r32(volatile uint8_t *p, unsigned off) {
    return *(volatile uint32_t *)(p + off);
}
static void usb_w32(volatile uint8_t *p, unsigned off, uint32_t v) {
    *(volatile uint32_t *)(p + off) = v;
}
/* xHCI permits low/high 32-bit accesses; do not assume atomic U64 MMIO. */
static void usb_w64(volatile uint8_t *p, unsigned off, uint64_t v) {
    usb_w32(p, off, (uint32_t)v);
    usb_w32(p, off + 4, (uint32_t)(v >> 32));
}
static uint64_t usb_dma_addr(const void *p) {
    return (uint64_t)(uintptr_t)p - usb_hhdm;
}
static void usb_dma_sync(const void *p, size_t n, int from_device) {
    uint64_t ctr;
    __asm__ volatile("mrs %0, ctr_el0" : "=r"(ctr));
    size_t line = (size_t)4 << ((ctr >> 16) & 15);
    uintptr_t end = (uintptr_t)p + n;
    for (uintptr_t a = (uintptr_t)p & ~(line - 1); a < end; a += line) {
        if (from_device) __asm__ volatile("dc ivac, %0" :: "r"(a) : "memory");
        else __asm__ volatile("dc cvac, %0" :: "r"(a) : "memory");
    }
    __asm__ volatile("dsb sy" ::: "memory");
}
static void usb_publish(void) {
    __asm__ volatile("dmb oshst" ::: "memory");
}
static void *usb_dma_alloc(size_t n, size_t align) {
    /* Isolate allocations on cache lines, including short HID reports. */
    if (align < 256) align = 256;
    size_t start = (usb_dma_used + align - 1) & ~(align - 1);
    n = (n + 255) & ~(size_t)255;
    if (start > USB_DMA_BYTES || n > USB_DMA_BYTES - start) return NULL;
    void *p = usb_dma_memory + start;
    usb_dma_used = start + n;
    usb_zero(p, n);
    usb_dma_sync(p, n, 0);
    return p;
}
static uint64_t usb_now(void) {
    uint64_t t;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t));
    return t;
}
static uint64_t usb_deadline(unsigned ms) {
    return usb_now() + usb_counter_hz * ms / 1000;
}
static void usb_delay(unsigned ms) {
    uint64_t end = usb_deadline(ms);
    while (usb_now() < end) __asm__ volatile("yield");
}
static int usb_wait(volatile uint8_t *base, unsigned off,
                    uint32_t mask, uint32_t want, unsigned ms) {
    uint64_t end = usb_deadline(ms);
    do {
        if ((usb_r32(base, off) & mask) == want) return 1;
        __asm__ volatile("yield");
    } while (usb_now() < end);
    return 0;
}
static volatile uint8_t *usb_pci_function(unsigned dev, unsigned fn) {
    return (volatile uint8_t *)(uintptr_t)(usb_hhdm + PCI_ECAM_PHYS +
                                          ((uint64_t)dev << 15) + (fn << 12));
}
static int usb_map_bar(volatile uint8_t *cfg, volatile uint8_t **mapped,
                       uint64_t *physical) {
    uint32_t lo = usb_r32(cfg, 0x10);
    if (lo & 1) return 0;
    uint64_t pa = lo & ~15u;
    if ((lo & 6) == 4) pa |= (uint64_t)usb_r32(cfg, 0x14) << 32;
    /* Firmware assigns PCI resources. Do not guess an address or move BARs. */
    if (!pa || pa >= (1ull << 48) - 65536) return 0;
    for (uint64_t p = pa & ~0x1fffffull; p <= ((pa + 65535) & ~0x1fffffull);
         p += 0x200000)
        if (mmio_map_2m(usb_hhdm, p)) return 0;
    uint16_t command = *(volatile uint16_t *)(cfg + 4);
    *(volatile uint16_t *)(cfg + 4) = command | 6; /* memory + bus mastering */
    usb_publish();
    *mapped = (volatile uint8_t *)(uintptr_t)(usb_hhdm + pa);
    *physical = pa;
    return 1;
}
static int usb_platform_init(uint64_t hhdm) {
    usb_hhdm = hhdm;
    usb_dma_used = 0;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(usb_counter_hz));
    if (!usb_counter_hz || !hhdm) return 0;
    return mmio_map_pci_ecam(hhdm) == 0;
}
