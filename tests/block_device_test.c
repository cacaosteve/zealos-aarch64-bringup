#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/mmio_map.h"
#include "../src/block_device.h"

#define TEST_BOOT_LBA 2048u
#define TEST_BOOT_SECTORS 2048u
#define TEST_DATA_LBA 4096u
#define TEST_DATA_EXTRA_SECTORS 32u
#define TEST_DATA_SECTORS \
    (ZEAL_RS_SECTS + ZEAL_SRC_SECTS + ZEAL_SRC_SHADOW_SECTS + \
     TEST_DATA_EXTRA_SECTORS)
#define TEST_DISK_SECTORS (TEST_DATA_LBA + TEST_DATA_SECTORS)

struct test_disk {
    uint8_t *bytes;
    uint64_t sectors;
    uint64_t data_lba;
    uint64_t data_sectors;
    unsigned writes;
    unsigned flushes;
};

/* The backend callback ABI carries the buffer first. Keep this adapter
 * separate from the context-aware fixture helpers below. */
static struct test_disk *g_disk;

static int disk_read(void *buffer, uint64_t sector, uint32_t count) {
    struct test_disk *disk = g_disk;
    if (!disk || !buffer || !count || sector >= disk->sectors ||
        count > disk->sectors - sector)
        return 0;
    memcpy(buffer, disk->bytes + (size_t)sector * 512u,
           (size_t)count * 512u);
    return 1;
}

static int disk_write(const void *buffer, uint64_t sector, uint32_t count) {
    struct test_disk *disk = g_disk;
    uint64_t data_end = disk ? disk->data_lba + disk->data_sectors : 0;
    if (!disk || !buffer || !count || sector >= disk->sectors ||
        count > disk->sectors - sector || sector < disk->data_lba ||
        sector >= data_end || count > data_end - sector)
        return 0;
    memcpy(disk->bytes + (size_t)sector * 512u, buffer,
           (size_t)count * 512u);
    disk->writes++;
    return 1;
}

static int disk_flush(void) {
    if (!g_disk) return 0;
    g_disk->flushes++;
    return 1;
}

static void put32(uint8_t *p, uint32_t value) {
    for (unsigned i = 0; i < 4; i++) p[i] = (uint8_t)(value >> (8u * i));
}

static uint8_t *sector_ptr(struct test_disk *disk, uint64_t sector) {
    return disk->bytes + (size_t)sector * 512u;
}

static void mbr_entry(uint8_t *mbr, unsigned index, uint8_t type,
                      uint32_t first, uint32_t count) {
    uint8_t *entry = mbr + 446u + index * 16u;
    entry[4] = type;
    put32(entry + 8, first);
    put32(entry + 12, count);
}

static int fail(const char *message) {
    fprintf(stderr, "FAIL block device: %s\n", message);
    return 1;
}

int main(void) {
    struct test_disk disk = {
        .bytes = calloc((size_t)TEST_DISK_SECTORS, 512u),
        .sectors = TEST_DISK_SECTORS,
        .data_lba = TEST_DATA_LBA,
        .data_sectors = TEST_DATA_SECTORS,
    };
    if (!disk.bytes) return fail("allocate disk fixture");
    g_disk = &disk;

    uint8_t *mbr = sector_ptr(&disk, 0);
    mbr[510] = 0x55;
    mbr[511] = 0xaa;
    mbr_entry(mbr, 0, 0xef, TEST_BOOT_LBA, TEST_BOOT_SECTORS);
    mbr_entry(mbr, 1, ZEAL_PARTITION_MBR_ZEAL_DATA,
              TEST_DATA_LBA, TEST_DATA_SECTORS);
    uint8_t mbr_before[512];
    memcpy(mbr_before, mbr, sizeof(mbr_before));

    memset(sector_ptr(&disk, TEST_BOOT_LBA), 0xa5,
           (size_t)TEST_BOOT_SECTORS * 512u);
    uint8_t *boot_before = malloc((size_t)TEST_BOOT_SECTORS * 512u);
    if (!boot_before) return fail("allocate boot partition snapshot");
    memcpy(boot_before, sector_ptr(&disk, TEST_BOOT_LBA),
           (size_t)TEST_BOOT_SECTORS * 512u);

    const struct zeal_block_backend device = {
        .max_sectors = 2,
        .sectors = TEST_DISK_SECTORS,
        .device_sectors = TEST_DISK_SECTORS,
        .flush_supported = 1,
        .read_abs = disk_read,
        .write_abs = disk_write,
        .flush = disk_flush,
    };
    if (!zbd_discover_and_bind_data_partition(&device))
        return fail("discover and bind the data partition");
    if (!zbd_ready() || zbd_capacity() != TEST_DATA_SECTORS ||
        g_zbd.lba_offset != TEST_DATA_LBA ||
        g_zbd.redsea_lba != 0 ||
        g_zbd.redsea_sectors != ZEAL_RS_SECTS + TEST_DATA_EXTRA_SECTORS ||
        g_zbd.source_a_lba != g_zbd.redsea_sectors ||
        g_zbd.source_a_sectors != ZEAL_SRC_SECTS ||
        g_zbd.source_b_lba != g_zbd.source_a_lba + ZEAL_SRC_SECTS ||
        g_zbd.source_b_sectors != ZEAL_SRC_SHADOW_SECTS)
        return fail("publish partition-relative RedSea and source-bank geometry");

    uint8_t write_data[3u * 512u];
    uint8_t read_data[3u * 512u];
    for (unsigned i = 0; i < sizeof(write_data); i++)
        write_data[i] = (uint8_t)(i * 17u + 3u);
    uint64_t redsea_sector = g_zbd.redsea_sectors - 3u;
    if (!zbd_write_volume(write_data, redsea_sector, 3) ||
        !zbd_read_volume(read_data, redsea_sector, 3) ||
        memcmp(write_data, read_data, sizeof(write_data)))
        return fail("translate split RedSea writes and read them back");
    if (!zbd_flush() || disk.flushes != 1)
        return fail("flush the bound physical device");

    unsigned writes_before_reject = disk.writes;
    if (zbd_write_volume(write_data, g_zbd.redsea_sectors - 1u, 2) ||
        zbd_write_abs_many(write_data, zbd_capacity(), 1) ||
        disk.writes != writes_before_reject)
        return fail("reject writes that cross RedSea or device bounds");

    if (!zbd_write_abs_many(write_data, g_zbd.source_a_lba, 2) ||
        !zbd_read_abs_many(read_data, g_zbd.source_a_lba, 2) ||
        memcmp(write_data, read_data, 2u * 512u))
        return fail("translate source-bank A I/O inside the data partition");
    if (!zbd_write_abs_many(write_data, g_zbd.source_b_lba, 2) ||
        !zbd_read_abs_many(read_data, g_zbd.source_b_lba, 2) ||
        memcmp(write_data, read_data, 2u * 512u))
        return fail("translate source-bank B I/O inside the data partition");

    if (memcmp(mbr_before, mbr, sizeof(mbr_before)))
        return fail("leave the partition table unchanged");
    if (memcmp(boot_before, sector_ptr(&disk, TEST_BOOT_LBA),
               (size_t)TEST_BOOT_SECTORS * 512u))
        return fail("leave the firmware FAT partition unchanged");

    free(boot_before);
    free(disk.bytes);
    puts("PASS partition-bounded RedSea/source-bank block I/O and FAT isolation");
    return 0;
}
