/*
 * On-disk layout for the 128MiB UTM/QEMU GPT image (must match Makefile).
 *
 *   LBA 0             protective MBR
 *   LBA 1             primary GPT header
 *   LBA 2..33         primary GPT entries
 *   LBA 2048..130910  EFI System partition
 *   LBA 130911..131038  RedSea window (128 sectors) — PCI Blk* lba_base
 *   LBA 131039..196574  native source partition (32MiB)
 *   LBA 196575..262110  source shadow bank (65536 sectors)
 *   LBA 262111..262142  backup GPT entries
 *   LBA 262143        backup GPT header
 */
#pragma once

#define ZEAL_DISK_SECTS       262144ull
#define ZEAL_OLD_DISK_SECTS   131072ull
#define ZEAL_GPT_BACKUP_SECTS 34ull
#define ZEAL_RS_SECTS         128ull
#define ZEAL_RS_LBA_BASE      130911ull
#define ZEAL_SRC_LBA_BASE     131039ull
#define ZEAL_SRC_SECTS        65536ull
#define ZEAL_SRC_SHADOW_LBA   196575ull
#define ZEAL_SRC_SHADOW_SECTS 65536ull
