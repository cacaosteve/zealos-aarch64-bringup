#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/block_partition.h"

#define TEST_SECTORS 16384u

struct test_disk {
    uint8_t *bytes;
    uint64_t sectors;
};

static int read_sector(void *context, uint64_t lba, uint8_t out[512]) {
    struct test_disk *disk = context;
    if (lba >= disk->sectors) return 0;
    memcpy(out, disk->bytes + (size_t)lba * 512u, 512u);
    return 1;
}

static void put32(uint8_t *p, uint32_t value) {
    for (unsigned i = 0; i < 4; i++) p[i] = (uint8_t)(value >> (8u * i));
}

static void put64(uint8_t *p, uint64_t value) {
    put32(p, (uint32_t)value);
    put32(p + 4, (uint32_t)(value >> 32));
}

static uint8_t *sector(struct test_disk *disk, uint64_t lba) {
    return disk->bytes + (size_t)lba * 512u;
}

static void mbr_entry(uint8_t *mbr, unsigned index, uint8_t type,
                      uint32_t first, uint32_t count) {
    uint8_t *entry = mbr + 446u + index * 16u;
    entry[4] = type;
    put32(entry + 8, first);
    put32(entry + 12, count);
}

static void make_mbr(struct test_disk *disk) {
    uint8_t *mbr = sector(disk, 0);
    memset(mbr, 0, 512);
    mbr[510] = 0x55;
    mbr[511] = 0xaa;
    mbr_entry(mbr, 0, 0x0c, 2048, 2048);
    mbr_entry(mbr, 1, 0xda, 4096, 8192);
}

static void make_gpt(struct test_disk *disk) {
    uint8_t *mbr = sector(disk, 0);
    memset(mbr, 0, 512);
    mbr[510] = 0x55;
    mbr[511] = 0xaa;
    mbr_entry(mbr, 0, 0xee, 1, (uint32_t)(disk->sectors - 1));

    uint8_t *entries = sector(disk, 2);
    memset(entries, 0, 512);
    for (unsigned i = 0; i < 16; i++) entries[i] = (uint8_t)(i + 1);
    put64(entries + 32, 2048);
    put64(entries + 40, 4095);
    uint8_t *second = entries + 128;
    memcpy(second, ZEAL_PARTITION_GPT_ZEAL_DATA_GUID,
           sizeof(ZEAL_PARTITION_GPT_ZEAL_DATA_GUID));
    put64(second + 32, 4096);
    put64(second + 40, 12287);

    uint8_t *header = sector(disk, 1);
    memset(header, 0, 512);
    memcpy(header, "EFI PART", 8);
    put32(header + 8, 0x00010000u);
    put32(header + 12, 92);
    put64(header + 24, 1);
    put64(header + 32, disk->sectors - 1);
    put64(header + 40, 34);
    put64(header + 48, disk->sectors - 34);
    put64(header + 72, 2);
    put32(header + 80, 4);
    put32(header + 84, 128);
    put32(header + 88, zeal_partition_crc(entries, 4u * 128u));
    put32(header + 16, 0);
    put32(header + 16, zeal_partition_crc(header, 92));
}

static int fail(const char *message) {
    fprintf(stderr, "FAIL block partition: %s\n", message);
    return 1;
}

int main(void) {
    struct test_disk disk = { .bytes = calloc(TEST_SECTORS, 512u),
                              .sectors = TEST_SECTORS };
    if (!disk.bytes) return fail("allocate fixture");
    struct zeal_partition parts[ZEAL_PARTITION_MAX];
    struct zeal_partition data;
    size_t count = 0;

    make_mbr(&disk);
    if (zeal_partition_scan(read_sector, &disk, disk.sectors, parts,
                            ZEAL_PARTITION_MAX, &count) != ZEAL_PARTITION_OK ||
        count != 2 || parts[0].scheme != ZEAL_PARTITION_MBR ||
        parts[0].mbr_type != 0x0c || parts[0].first_lba != 2048 ||
        parts[0].sector_count != 2048 || parts[1].mbr_type != 0xda ||
        parts[1].first_lba != 4096 || parts[1].sector_count != 8192)
        return fail("discover bounded MBR boot and data partitions");
    if (zeal_partition_find_data(parts, count, &data) != ZEAL_PARTITION_OK ||
        data.scheme != ZEAL_PARTITION_MBR || data.mbr_type != 0xda ||
        data.first_lba != 4096 || data.sector_count != 8192)
        return fail("select the unique ZealOS MBR data partition");

    mbr_entry(sector(&disk, 0), 2, 0xda, 12288, 2048);
    if (zeal_partition_scan(read_sector, &disk, disk.sectors, parts,
                            ZEAL_PARTITION_MAX, &count) != ZEAL_PARTITION_OK ||
        zeal_partition_find_data(parts, count, &data) != ZEAL_PARTITION_INVALID)
        return fail("reject multiple ZealOS MBR data partitions");

    make_mbr(&disk);
    mbr_entry(sector(&disk, 0), 1, 0xda, 3000, 8192);
    if (zeal_partition_scan(read_sector, &disk, disk.sectors, parts,
                            ZEAL_PARTITION_MAX, &count) != ZEAL_PARTITION_INVALID)
        return fail("reject overlapping MBR partitions");
    make_mbr(&disk);
    mbr_entry(sector(&disk, 0), 1, 0xda, TEST_SECTORS - 4, 8);
    if (zeal_partition_scan(read_sector, &disk, disk.sectors, parts,
                            ZEAL_PARTITION_MAX, &count) != ZEAL_PARTITION_INVALID)
        return fail("reject MBR partition beyond device capacity");
    make_mbr(&disk);
    mbr_entry(sector(&disk, 0), 1, 0x0f, 4096, 8192);
    if (zeal_partition_scan(read_sector, &disk, disk.sectors, parts,
                            ZEAL_PARTITION_MAX, &count) != ZEAL_PARTITION_UNSUPPORTED)
        return fail("reject unsupported extended MBR partitions");

    make_gpt(&disk);
    if (zeal_partition_scan(read_sector, &disk, disk.sectors, parts,
                            ZEAL_PARTITION_MAX, &count) != ZEAL_PARTITION_OK ||
        count != 2 || parts[0].scheme != ZEAL_PARTITION_GPT ||
        parts[0].first_lba != 2048 || parts[0].sector_count != 2048 ||
        parts[1].first_lba != 4096 || parts[1].sector_count != 8192 ||
        memcmp(parts[1].type_guid, ZEAL_PARTITION_GPT_ZEAL_DATA_GUID,
               sizeof(data.type_guid)))
        return fail("discover CRC-verified GPT partitions");
    if (zeal_partition_find_data(parts, count, &data) != ZEAL_PARTITION_OK ||
        data.scheme != ZEAL_PARTITION_GPT || data.first_lba != 4096 ||
        data.sector_count != 8192)
        return fail("select the unique ZealOS GPT data partition");

    sector(&disk, 2)[128 + 32] ^= 1;
    if (zeal_partition_scan(read_sector, &disk, disk.sectors, parts,
                            ZEAL_PARTITION_MAX, &count) != ZEAL_PARTITION_INVALID)
        return fail("reject GPT entries CRC mismatch");
    make_gpt(&disk);
    sector(&disk, 1)[40] ^= 1;
    if (zeal_partition_scan(read_sector, &disk, disk.sectors, parts,
                            ZEAL_PARTITION_MAX, &count) != ZEAL_PARTITION_INVALID)
        return fail("reject GPT header CRC mismatch");
    make_gpt(&disk);
    put64(sector(&disk, 2) + 32, disk.sectors - 1);
    put64(sector(&disk, 2) + 40, disk.sectors + 5);
    put32(sector(&disk, 1) + 88,
          zeal_partition_crc(sector(&disk, 2), 4u * 128u));
    put32(sector(&disk, 1) + 16, 0);
    put32(sector(&disk, 1) + 16, zeal_partition_crc(sector(&disk, 1), 92));
    if (zeal_partition_scan(read_sector, &disk, disk.sectors, parts,
                            ZEAL_PARTITION_MAX, &count) != ZEAL_PARTITION_INVALID)
        return fail("reject GPT extent outside usable/device bounds");

    make_gpt(&disk);
    if (zeal_partition_scan(read_sector, &disk, 1, parts, ZEAL_PARTITION_MAX,
                            &count) != ZEAL_PARTITION_INVALID)
        return fail("reject reported device smaller than GPT metadata");
    if (zeal_partition_scan(read_sector, &disk, disk.sectors, parts, 0,
                            &count) != ZEAL_PARTITION_INVALID)
        return fail("reject missing output capacity");

    free(disk.bytes);
    puts("PASS MBR/GPT partition discovery, CRCs, overlap and device bounds");
    return 0;
}
