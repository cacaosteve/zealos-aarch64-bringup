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
