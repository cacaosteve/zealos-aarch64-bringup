/*
 * Pi 4B (BCM2711) platform constants — stub for the UEFI track (PI4.md).
 * QEMU/UTM virt still uses its own bases; select at runtime later.
 */
#pragma once

/* Common PL011 UART0 window (SoC low-peripheral). */
#define PI4_UART0_PHYS 0xfe201000ull
/* Alternate PL011s / AUX mini-UART IO (UEFI may move the console). */
#define PI4_UART2_PHYS 0xfe201400ull
#define PI4_UART3_PHYS 0xfe201600ull
#define PI4_MINIUART_IO_PHYS 0xfe215040ull

/* GIC-400 distributor / CPU interface (GICv2), typical Pi4 mapping. */
#define PI4_GICD_PHYS 0xff841000ull
#define PI4_GICC_PHYS 0xff842000ull

#define PI4_CNTP_PPI 30 /* non-secure EL1 physical timer, same PPI number */

/* BCM2711 PCIe RC status: read only after firmware FDT reports the host. */
#define PI4_PCIE_STATUS_OFFSET 0x4068u
#define PI4_PCIE_STATUS_PHY_LINKUP (1u << 4)
#define PI4_PCIE_STATUS_DL_ACTIVE  (1u << 5)

/* BCM2711 HDMI controller windows, translated from /soc in bcm2711.dtsi. */
#define PI4_HDMI0_PHYS 0xfef00700ull
#define PI4_HDMI1_PHYS 0xfef05700ull
#define PI4_HDMI_REG_SIZE 0x300u
#define PI4_HDMI_PACKET_CONFIG 0xbcu
#define PI4_HDMI_PACKET_STATUS 0xc4u
#define PI4_HDMI_AUDIO_CLOCK_CONFIG 0xc8u
#define PI4_HDMI_AUDIO_CTS 0xccu
#define PI4_HDMI_SCHEDULER_CONTROL 0xe0u
#define PI4_HDMI_PACKET_ENABLE (1u << 16)
#define PI4_HDMI_LINK_ACTIVE (1u << 1)

#define PI4_MAILBOX_PHYS 0xfe00b880ull
#define PI4_MAILBOX_READ 0x00u
#define PI4_MAILBOX_RX_STATUS 0x18u
#define PI4_MAILBOX_WRITE 0x20u
#define PI4_MAILBOX_TX_STATUS 0x38u
#define PI4_MAILBOX_FULL (1u << 31)
#define PI4_MAILBOX_EMPTY (1u << 30)
#define PI4_MAILBOX_PROPERTY_CHANNEL 8u
#define PI4_MAILBOX_GPU_ALIAS 0xc0000000u
#define PI4_PROPERTY_GET_CLOCK_RATE 0x00030002u
#define PI4_PROPERTY_PIXEL_BVB_CLOCK 14u
#define PI4_PROPERTY_GET_DMA_CHANNELS 0x00060001u
