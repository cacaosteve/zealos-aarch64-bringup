/*
 * On-disk layout for the 64MiB UTM/QEMU GPT image (must match Makefile).
 *
 *   LBA 0             protective MBR
 *   LBA 1             primary GPT header
 *   LBA 2..33         primary GPT entries
 *   LBA 2048..130910  EFI System partition
 *   LBA 130911..131038  RedSea window (128 sectors) — PCI Blk* lba_base
 *   LBA 131039..131070  backup GPT entries
 *   LBA 131071        backup GPT header
 */
#pragma once

#define ZEAL_DISK_SECTS       131072ull
#define ZEAL_GPT_BACKUP_SECTS 34ull
#define ZEAL_RS_SECTS         128ull
#define ZEAL_RS_LBA_BASE \
    (ZEAL_DISK_SECTS - ZEAL_GPT_BACKUP_SECTS - ZEAL_RS_SECTS + 1ull) /* 130911 */
