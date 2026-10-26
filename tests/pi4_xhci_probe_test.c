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
    struct pi4_xhci_outbound_window window;
    struct pi4_xhci_bar_plan plan;
    struct pi4_xhci_state_plan state_plan;
    struct pi4_xhci_port_plan port_plan;
    const uint32_t valid_caps[6] = {
        0x01000020u, 0x08004020u, 0, 0x02840000u, 0x1000u, 0x800u
    };
    uint32_t invalid_caps[6];
    uint16_t command;
    uint32_t legacy_request;
    uint64_t cpu_address = 0;

    if (PI4_XHCI_USBSTS_HALTED != (1u << 0) ||
        PI4_XHCI_USBSTS_HSE != (1u << 2) ||
        PI4_XHCI_USBSTS_PCD != (1u << 4) ||
        PI4_XHCI_USBSTS_CNR != (1u << 11))
        return fail("USBSTS standard bit assignments");

    init_handoff(&h);
    /* Physical Pi: broad FDT range, but live window 0 is f8000000 -> 600000000. */
    if (pi4_xhci_outbound_window_decode(0xf8000000u, 0, 0x03f00000u,
                                       6, 6, &window) != 0 ||
        window.bus_base != 0xf8000000ull || window.cpu_base != 0x600000000ull ||
        window.size != 0x04000000ull)
        return fail("physical BCM2711 outbound window decode");
    if (pi4_xhci_bar_plan(&h, 0xf8000004u, 0, &window, &plan) != 0 ||
        plan.bus_base != 0xf8000000ull || plan.cpu_base != 0x600000000ull)
        return fail("physical VL805 BAR translation through live window");
    if (pi4_xhci_state_offsets(valid_caps, &state_plan) != 0 ||
        state_plan.usbcmd_offset != 0x20u ||
        state_plan.usbsts_offset != 0x24u ||
        state_plan.first_ext_cap_offset != 0xa10u)
        return fail("read-only xHCI operational and extended-capability offsets");
    if (pi4_xhci_portsc_plan(valid_caps, &port_plan) != 0 ||
        port_plan.first_portsc_offset != 0x420u || port_plan.count != 8u ||
        port_plan.total_ports != 8u ||
        port_plan.first_portsc_offset +
            (port_plan.count - 1u) * PI4_XHCI_PORTSC_STRIDE != 0x490u)
        return fail("bounded read-only xHCI PORTSC offsets");
    if (!pi4_xhci_handoff_safe(0, PI4_XHCI_USBSTS_HALTED) ||
        pi4_xhci_handoff_safe(PI4_XHCI_USBCMD_RUN,
                              PI4_XHCI_USBSTS_HALTED) ||
        pi4_xhci_handoff_safe(0, 0))
        return fail("firmware ownership handoff halt gate");
    if (!pi4_xhci_reset_safe(0, PI4_XHCI_USBSTS_HALTED,
                             PI4_XHCI_LEGACY_OS_OWNED,
                             PI4_PCI_COMMAND_MEMORY) ||
        pi4_xhci_reset_safe(PI4_XHCI_USBCMD_RUN,
                            PI4_XHCI_USBSTS_HALTED,
                            PI4_XHCI_LEGACY_OS_OWNED,
                            PI4_PCI_COMMAND_MEMORY) ||
        pi4_xhci_reset_safe(PI4_XHCI_USBCMD_HCRST,
                            PI4_XHCI_USBSTS_HALTED,
                            PI4_XHCI_LEGACY_OS_OWNED,
                            PI4_PCI_COMMAND_MEMORY) ||
        pi4_xhci_reset_safe(0, PI4_XHCI_USBSTS_HALTED | PI4_XHCI_USBSTS_CNR,
                            PI4_XHCI_LEGACY_OS_OWNED,
                            PI4_PCI_COMMAND_MEMORY) ||
        pi4_xhci_reset_safe(0, 0, PI4_XHCI_LEGACY_OS_OWNED,
                            PI4_PCI_COMMAND_MEMORY) ||
        pi4_xhci_reset_safe(0, PI4_XHCI_USBSTS_HALTED,
                            PI4_XHCI_LEGACY_BIOS_OWNED |
                                PI4_XHCI_LEGACY_OS_OWNED,
                            PI4_PCI_COMMAND_MEMORY) ||
        pi4_xhci_reset_safe(0, PI4_XHCI_USBSTS_HALTED, 0,
                            PI4_PCI_COMMAND_MEMORY) ||
        pi4_xhci_reset_safe(0, PI4_XHCI_USBSTS_HALTED,
                            PI4_XHCI_LEGACY_OS_OWNED,
                            PI4_PCI_COMMAND_MEMORY | PI4_PCI_COMMAND_MASTER))
        return fail("HCRST handoff, readiness, halt, and bus-master-off gate");
    if (pi4_xhci_legacy_claim_value(0x00010001u, &legacy_request) != 0 ||
        legacy_request != 0x01010001u ||
        pi4_xhci_legacy_claim_value(0x01010001u, &legacy_request) != 0 ||
        legacy_request != 0x01010001u)
        return fail("xHCI OS ownership request preserves existing capability bits");
    if (pi4_xhci_legacy_claim_value(0x00020002u, &legacy_request) == 0 ||
        pi4_xhci_legacy_claim_value(0x00010001u, NULL) == 0)
        return fail("xHCI ownership request accepted invalid capability");
    memcpy(invalid_caps, valid_caps, sizeof(invalid_caps));
    invalid_caps[1] = (invalid_caps[1] & 0x00ffffffu) | (16u << 24);
    if (pi4_xhci_portsc_plan(invalid_caps, &port_plan) != 0 ||
        port_plan.count != PI4_XHCI_DIAG_PORTS || port_plan.total_ports != 16u)
        return fail("xHCI PORTSC diagnostic port bound");
    memcpy(invalid_caps, valid_caps, sizeof(invalid_caps));
    invalid_caps[1] &= 0x00ffffffu;
    if (pi4_xhci_portsc_plan(invalid_caps, &port_plan) == 0)
        return fail("xHCI PORTSC plan accepted zero root ports");
    if (pi4_xhci_bar_offset_cpu(&h, 0xf8400000ull, &window,
                                state_plan.first_ext_cap_offset, 4,
                                &cpu_address) != 0 ||
        cpu_address != 0x600400a10ull)
        return fail("extended capability address through live window");
    if (pi4_xhci_bar_offset_cpu(&h, 0xf8400000ull, &window,
                                port_plan.first_portsc_offset, 4,
                                &cpu_address) != 0 ||
        cpu_address != 0x600400420ull)
        return fail("root-port status address through live window");
    if (pi4_xhci_bar_offset_cpu(&h, 0xf8400000ull, &window,
                                0x04000000u, 4, &cpu_address) == 0)
        return fail("register read beyond live outbound window accepted");
    if (pi4_xhci_bar_offset_cpu(&h, 0xbffff000ull, &window,
                                0x1000u, 4, &cpu_address) == 0 ||
        pi4_xhci_bar_offset_cpu(&h, 0xf7fff000ull, &window,
                                0x1000u, 4, &cpu_address) == 0)
        return fail("offset read accepted when BAR base is outside its windows");

    if (pi4_xhci_bar_plan(&h, 0xf8000000u, 0, &window, &plan) == 0)
        return fail("32-bit BAR accepted");
    if (pi4_xhci_bar_plan(&h, 0xf8000005u, 0, &window, &plan) == 0)
        return fail("I/O BAR accepted");
    if (pi4_xhci_bar_plan(&h, 0xb8000004u, 0, &window, &plan) == 0)
        return fail("BAR below FDT window accepted");
    if (pi4_xhci_bar_plan(&h, 0xfbfffff4u, 0, &window, &plan) == 0)
        return fail("BAR with truncated live-window capability header accepted");
    if (pi4_xhci_bar_plan(&h, 0xfe000004u, 0, &window, &plan) == 0)
        return fail("BAR outside live window accepted");
    if (pi4_xhci_bar_plan(&h, 0xf8000004u, 1, &window, &plan) == 0)
        return fail("BAR outside 32-bit PCI window accepted");

    if (pi4_xhci_outbound_window_decode(0xf8000000u, 0, 0x03f00000u,
                                        6, 5, &window) == 0)
        return fail("descending outbound CPU range accepted");
    if (pi4_xhci_outbound_window_decode(0xfffffff0u, 0xffffffffu, 0x03f00000u,
                                        6, 6, &window) == 0)
        return fail("overflowing outbound bus range accepted");

    h.pi4_pcie.flags &= ~PI4_PCIE_FDT_ENABLED;
    if (pi4_xhci_bar_plan(&h, 0xf8000004u, 0, &window, &plan) == 0)
        return fail("disabled PCIe host accepted");
    init_handoff(&h);
    h.pi4_pcie.mem_size = UINT64_MAX;
    if (pi4_xhci_bar_plan(&h, 0xf8000004u, 0, &window, &plan) == 0)
        return fail("overflowing PCI window accepted");

    if (pi4_xhci_memory_command(0, &command) != 0 || command != 2u)
        return fail("memory decode enable plan");
    if (pi4_xhci_memory_command(0x40u, &command) != 0 || command != 0x42u)
        return fail("other command bits not preserved");
    if (pi4_xhci_memory_command(PI4_PCI_COMMAND_MASTER, &command) == 0)
        return fail("bus-mastering state accepted");

    if (!pi4_xhci_caps_valid(valid_caps))
        return fail("valid xHCI capability header rejected");
    memcpy(invalid_caps, valid_caps, sizeof(invalid_caps));
    invalid_caps[0] = 0xdeadbeadu;
    invalid_caps[1] = 0xdeadbeadu;
    invalid_caps[4] = 0xdeadbeadu;
    invalid_caps[5] = 0xdeadbeadu;
    if (pi4_xhci_caps_valid(invalid_caps))
        return fail("poisoned BAR values accepted as xHCI");
    memcpy(invalid_caps, valid_caps, sizeof(invalid_caps));
    invalid_caps[0] = (valid_caps[0] & ~0xffu) | 0x21u;
    if (pi4_xhci_caps_valid(invalid_caps))
        return fail("unaligned capability length accepted");
    memcpy(invalid_caps, valid_caps, sizeof(invalid_caps));
    invalid_caps[1] &= ~0xffu;
    if (pi4_xhci_caps_valid(invalid_caps))
        return fail("controller reporting no slots accepted");
    memcpy(invalid_caps, valid_caps, sizeof(invalid_caps));
    invalid_caps[3] = 0x00010000u;
    if (pi4_xhci_state_offsets(invalid_caps, &state_plan) == 0)
        return fail("extended-capability pointer inside the header accepted");

    puts("PASS Pi4 xHCI BAR translation, header validation, ownership handoff, halted reset gate, and bus-master-off MMIO plan");
    return 0;
}
