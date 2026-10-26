/*
 * Aarch64 Limine → kernel handoff.
 * Intentionally not CKernel: no GDT, E820 layout quirks, or 32-bit trampoline.
 */
#pragma once

#include <stdint.h>

#define ZEAL_HANDOFF_MAGIC_0 0x5a45414c41413234ULL /* "ZEALAA64" */
#define ZEAL_HANDOFF_MAGIC_1 0xa64b00740001ULL

#define ZEAL_BL_LIMINE 1

/* handoff.flags */
#define ZEAL_FLAG_PLAT_PI4 (1u << 0) /* BCM2711 / Pi UEFI — not QEMU virt */

#define ZEAL_MEM_USABLE   1
#define ZEAL_MEM_RESERVED 2
#define ZEAL_MEM_ACPI     3
#define ZEAL_MEM_ACPI_NVS 4
#define ZEAL_MEM_BAD      5
#define ZEAL_MEM_MODULES  7
/* Bootloader reclaimable: not free until kernel finishes handoff/boot services. */
#define ZEAL_MEM_RECLAIM  8
#define ZEAL_MEM_KERNEL   9 /* loaded kernel image (reserved for runtime) */
#define ZEAL_MEM_PAGETABLE 10 /* boot-reserved 4KiB translation-table pool */
#define ZEAL_MEM_USB_DMA  11 /* boot-reserved DMA region; not a general allocator */

#define PI4_PCIE_FDT_PRESENT  (1u << 0)
#define PI4_PCIE_FDT_ENABLED  (1u << 1)
#define PI4_PCIE_FDT_REG      (1u << 2)
#define PI4_PCIE_FDT_MEM      (1u << 3)
#define PI4_PCIE_FDT_DMA      (1u << 4)

/* Stored in the old reserved word so the handoff size and offsets stay stable. */
#define PI4_FDT_STATUS_OK             0u
#define PI4_FDT_STATUS_NO_RESPONSE    1u
#define PI4_FDT_STATUS_NULL_POINTER   2u
#define PI4_FDT_STATUS_BAD_MAGIC      3u
#define PI4_FDT_STATUS_BAD_SIZE       4u
#define PI4_FDT_STATUS_NODE_NOT_FOUND 5u
#define PI4_FDT_STATUS_PARSE_BASE     0x100u

struct zeal_pi4_pcie {
    uint32_t flags; /* PI4_PCIE_FDT_*; descriptive only, no controller touched */
    uint32_t fdt_status; /* PI4_FDT_STATUS_* or PARSE_BASE + parser error code */
    uint64_t reg_base;
    uint64_t reg_size;
    uint64_t mem_bus_base;
    uint64_t mem_cpu_base;
    uint64_t mem_size;
    uint64_t dma_bus_base;
    uint64_t dma_cpu_base;
    uint64_t dma_size;
};

#define PI4_USB_DMA_RESERVED (1u << 0)
struct zeal_pi4_dma {
    uint32_t flags;
    uint32_t reserved;
    uint64_t phys_base;
    uint64_t bus_base;
    uint64_t size;
};

#define ZEAL_MEM_MAX 256

struct zeal_mem_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t pad;
};

struct zeal_handoff {
    uint64_t magic0;
    uint64_t magic1;
    uint32_t size; /* sizeof(struct zeal_handoff) */
    uint32_t revision; /* 7; extensions are appended */
    uint32_t bootloader_id; /* ZEAL_BL_* */
    uint32_t flags;
    uint64_t hhdm_offset;
    uint64_t uart_phys; /* PL011 DR base; 0 if unknown */
    uint64_t fb_addr_phys;
    uint64_t fb_width;
    uint64_t fb_height;
    uint64_t fb_pitch;
    uint32_t fb_bpp;
    uint32_t mem_count;
    uint64_t mem_physical_top;
    uint64_t boot_el; /* CurrentEL at handoff */
    /* revision >= 2 */
    uint64_t bc_virt; /* optional HolyC bytecode module (Limine ptr) */
    uint64_t bc_size;
    struct zeal_mem_entry mem[ZEAL_MEM_MAX];
    /* revision >= 3; appended to retain rev2 memory-map layout */
    uint64_t sources_virt;
    uint64_t sources_size;
    /* revision >= 4; Limine media type of the actual kernel module.
     * Generic disk = 0, optical = 1, TFTP/RAM download = 2. */
    uint32_t boot_media_type;
    uint32_t reserved;
    /* revision >= 5; read-only FDT description of the Pi 4 PCIe host. */
    struct zeal_pi4_pcie pi4_pcie;
    /* revision >= 6; reserved low-address memory, not yet used to start xHCI. */
    struct zeal_pi4_dma pi4_usb_dma;
    /* revision >= 7; optional sparse UTM/QEMU install image supplied by ISO boot. */
    uint64_t install_image_virt;
    uint64_t install_image_size;
};
