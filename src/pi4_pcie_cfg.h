/* Bounded BCM2711 downstream PCI config-read planning. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "handoff.h"
#include "plat_pi4.h"

#define PI4_PCIE_CFG_INDEX_OFFSET 0x9000u
#define PI4_PCIE_CFG_DATA_OFFSET  0x8000u
#define PI4_PCIE_CFG_MAX_OFFSET   0x0ffcu

struct pi4_pcie_cfg_read {
    uint32_t index;
    uint32_t data_offset;
};

/*
 * Build one aligned 32-bit read through BCM2711's indirect config window.
 * Bus zero is excluded because Linux accesses the root complex directly;
 * this helper is only for a downstream function after link-up.
 */
static int pi4_pcie_cfg_read_plan(const struct zeal_handoff *h,
                                  uint32_t link_status,
                                  unsigned bus, unsigned device,
                                  unsigned function, unsigned offset,
                                  struct pi4_pcie_cfg_read *out) {
    const uint32_t required_flags = PI4_PCIE_FDT_PRESENT |
                                    PI4_PCIE_FDT_ENABLED |
                                    PI4_PCIE_FDT_REG;
    const uint32_t link_mask = PI4_PCIE_STATUS_PHY_LINKUP |
                               PI4_PCIE_STATUS_DL_ACTIVE;
    uint32_t reg_end;
    unsigned devfn;

    if (!h || !out || h->revision < 5 ||
        h->size < offsetof(struct zeal_handoff, pi4_pcie) + sizeof(h->pi4_pcie) ||
        (h->pi4_pcie.flags & required_flags) != required_flags ||
        !h->pi4_pcie.reg_size || (h->pi4_pcie.reg_base & 3u) ||
        (link_status & link_mask) != link_mask ||
        bus == 0 || bus > 255 || device > 31 || function > 7 ||
        offset > PI4_PCIE_CFG_MAX_OFFSET || (offset & 3u))
        return -1;

    /* Both indirect registers must be fully contained in the FDT range. */
    reg_end = PI4_PCIE_CFG_INDEX_OFFSET + 4u;
    if (reg_end > h->pi4_pcie.reg_size ||
        PI4_PCIE_CFG_DATA_OFFSET > h->pi4_pcie.reg_size ||
        offset > h->pi4_pcie.reg_size - PI4_PCIE_CFG_DATA_OFFSET ||
        4u > h->pi4_pcie.reg_size - PI4_PCIE_CFG_DATA_OFFSET - offset)
        return -2;
    if (h->pi4_pcie.reg_base > UINT64_MAX - reg_end)
        return -3;

    devfn = (device << 3) | function;
    out->index = (bus << 20) | (devfn << 12);
    out->data_offset = PI4_PCIE_CFG_DATA_OFFSET + offset;
    return 0;
}
