/* Bounded, read-only probe plan for the BCM2711 eMMC2 SDHCI register window. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "handoff.h"

#define PI4_SDHCI_REG_SIZE_MIN 0x100u
#define PI4_SDHCI_PRESENT_STATE 0x24u
#define PI4_SDHCI_CLOCK_CONTROL 0x2cu
#define PI4_SDHCI_INTERRUPT_STATUS 0x30u
#define PI4_SDHCI_CAPABILITIES 0x40u
#define PI4_SDHCI_CAPABILITIES_1 0x44u
#define PI4_SDHCI_HOST_VERSION_WORD 0xfcu

struct pi4_sdhci_probe_plan {
    uint64_t phys_base;
    uint64_t reg_size;
};

static int pi4_sdhci_probe_plan_make(const struct zeal_handoff *h,
                                     struct pi4_sdhci_probe_plan *out) {
    const uint32_t required = PI4_SD_FDT_PRESENT | PI4_SD_FDT_ENABLED | PI4_SD_FDT_REG;
    if (!h || !out || h->revision < 9 ||
        h->size < offsetof(struct zeal_handoff, pi4_sd) + sizeof(h->pi4_sd) ||
        (h->pi4_sd.flags & required) != required ||
        h->pi4_sd.reg_size < PI4_SDHCI_REG_SIZE_MIN ||
        (h->pi4_sd.reg_base & 3u) || h->pi4_sd.reg_base >= (1ull << 48) ||
        h->pi4_sd.reg_base > UINT64_MAX - h->pi4_sd.reg_size)
        return -1;
    out->phys_base = h->pi4_sd.reg_base;
    out->reg_size = h->pi4_sd.reg_size;
    return 0;
}

static int pi4_sdhci_probe_offset_valid(const struct pi4_sdhci_probe_plan *plan,
                                        unsigned offset) {
    return plan && !(offset & 3u) && offset <= plan->reg_size - sizeof(uint32_t);
}
