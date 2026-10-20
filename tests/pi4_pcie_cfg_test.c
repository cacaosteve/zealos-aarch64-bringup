#include <stdio.h>
#include <string.h>
#include "../src/pi4_pcie_cfg.h"

static int fail(const char *name) {
    fprintf(stderr, "FAIL Pi4 PCI config read plan: %s\n", name);
    return 1;
}

static void init_handoff(struct zeal_handoff *h) {
    memset(h, 0, sizeof(*h));
    h->revision = 6;
    h->size = sizeof(*h);
    h->pi4_pcie.flags = PI4_PCIE_FDT_PRESENT | PI4_PCIE_FDT_ENABLED |
                        PI4_PCIE_FDT_REG;
    h->pi4_pcie.reg_base = 0xfd500000ull;
    h->pi4_pcie.reg_size = 0x9310ull;
}

int main(void) {
    struct zeal_handoff h;
    struct pi4_pcie_cfg_read plan;
    const uint32_t up = PI4_PCIE_STATUS_PHY_LINKUP | PI4_PCIE_STATUS_DL_ACTIVE;

    init_handoff(&h);
    if (pi4_pcie_cfg_read_plan(&h, up, 1, 0, 0, 0, &plan) != 0 ||
        plan.index != 0x00100000u || plan.data_offset != 0x8000u)
        return fail("bus 1 device 0 vendor/device plan");
    if (pi4_pcie_cfg_read_plan(&h, up, 1, 0, 0, 8, &plan) != 0 ||
        plan.index != 0x00100000u || plan.data_offset != 0x8008u)
        return fail("class/revision plan");
    if (pi4_pcie_cfg_read_plan(&h, up, 2, 3, 1, 0x0ffc, &plan) != 0 ||
        plan.index != 0x00219000u || plan.data_offset != 0x8ffcu)
        return fail("last aligned config dword plan");

    if (pi4_pcie_cfg_read_plan(&h, 0, 1, 0, 0, 0, &plan) == 0)
        return fail("link-down access accepted");
    if (pi4_pcie_cfg_read_plan(&h, PI4_PCIE_STATUS_PHY_LINKUP,
                               1, 0, 0, 0, &plan) == 0)
        return fail("inactive data link accepted");
    if (pi4_pcie_cfg_read_plan(&h, up, 0, 0, 0, 0, &plan) == 0)
        return fail("root bus config access accepted");
    if (pi4_pcie_cfg_read_plan(&h, up, 1, 32, 0, 0, &plan) == 0 ||
        pi4_pcie_cfg_read_plan(&h, up, 1, 0, 8, 0, &plan) == 0 ||
        pi4_pcie_cfg_read_plan(&h, up, 1, 0, 0, 2, &plan) == 0 ||
        pi4_pcie_cfg_read_plan(&h, up, 1, 0, 0, 0x1000, &plan) == 0)
        return fail("invalid device/function/offset accepted");

    h.pi4_pcie.reg_size = PI4_PCIE_CFG_INDEX_OFFSET;
    if (pi4_pcie_cfg_read_plan(&h, up, 1, 0, 0, 0, &plan) == 0)
        return fail("truncated index register range accepted");
    h.pi4_pcie.reg_size = PI4_PCIE_CFG_DATA_OFFSET;
    if (pi4_pcie_cfg_read_plan(&h, up, 1, 0, 0, 0, &plan) == 0)
        return fail("truncated data register range accepted");
    init_handoff(&h);
    h.pi4_pcie.flags &= ~PI4_PCIE_FDT_ENABLED;
    if (pi4_pcie_cfg_read_plan(&h, up, 1, 0, 0, 0, &plan) == 0)
        return fail("disabled bridge accepted");
    init_handoff(&h);
    h.revision = 4;
    if (pi4_pcie_cfg_read_plan(&h, up, 1, 0, 0, 0, &plan) == 0)
        return fail("old handoff revision accepted");
    init_handoff(&h);
    h.pi4_pcie.reg_base = UINT64_MAX - 0x1000u;
    if (pi4_pcie_cfg_read_plan(&h, up, 1, 0, 0, 0, &plan) == 0)
        return fail("overflowing register base accepted");

    puts("PASS BCM2711 config-read bounds, link gate and indirect address plan");
    return 0;
}
