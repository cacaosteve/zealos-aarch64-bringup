#include <stdio.h>
#include <string.h>
#include "../src/pi4_xhci_probe.h"

static int fail(const char *name) {
    fprintf(stderr, "FAIL Pi4 xHCI probe plan: %s\n", name);
    return 1;
}

static void init_handoff(struct zeal_handoff *h) {
    memset(h, 0, sizeof(*h));
    h->revision = 6;
    h->size = sizeof(*h);
    h->pi4_pcie.flags = PI4_PCIE_FDT_PRESENT | PI4_PCIE_FDT_ENABLED |
                        PI4_PCIE_FDT_MEM;
    h->pi4_pcie.mem_bus_base = 0xc0000000ull;
    h->pi4_pcie.mem_cpu_base = 0x600000000ull;
    h->pi4_pcie.mem_size = 0x40000000ull;
}

int main(void) {
    struct zeal_handoff h;
    struct pi4_xhci_bar_plan plan;
    uint16_t command;

    init_handoff(&h);
    /* Physical Pi report: BAR0=f8000004, BAR1=0, window C0000000..FFFFFFFF. */
    if (pi4_xhci_bar_plan(&h, 0xf8000004u, 0, &plan) != 0 ||
        plan.bus_base != 0xf8000000ull || plan.cpu_base != 0x638000000ull)
        return fail("physical VL805 BAR translation");

    if (pi4_xhci_bar_plan(&h, 0xf8000000u, 0, &plan) == 0)
        return fail("32-bit BAR accepted");
    if (pi4_xhci_bar_plan(&h, 0xf8000005u, 0, &plan) == 0)
        return fail("I/O BAR accepted");
    if (pi4_xhci_bar_plan(&h, 0xb8000004u, 0, &plan) == 0)
        return fail("BAR below FDT window accepted");
    if (pi4_xhci_bar_plan(&h, 0xfffffff4u, 0, &plan) == 0)
        return fail("BAR with truncated capability header accepted");
    if (pi4_xhci_bar_plan(&h, 0xf8000004u, 1, &plan) == 0)
        return fail("BAR outside 32-bit PCI window accepted");

    h.pi4_pcie.flags &= ~PI4_PCIE_FDT_ENABLED;
    if (pi4_xhci_bar_plan(&h, 0xf8000004u, 0, &plan) == 0)
        return fail("disabled PCIe host accepted");
    init_handoff(&h);
    h.pi4_pcie.mem_size = UINT64_MAX;
    if (pi4_xhci_bar_plan(&h, 0xf8000004u, 0, &plan) == 0)
        return fail("overflowing PCI window accepted");

    if (pi4_xhci_memory_command(0, &command) != 0 || command != 2u)
        return fail("memory decode enable plan");
    if (pi4_xhci_memory_command(0x40u, &command) != 0 || command != 0x42u)
        return fail("other command bits not preserved");
    if (pi4_xhci_memory_command(PI4_PCI_COMMAND_MASTER, &command) == 0)
        return fail("bus-mastering state accepted");

    puts("PASS Pi4 xHCI BAR translation and bus-master-off MMIO command plan");
    return 0;
}
