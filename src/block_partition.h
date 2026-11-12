/* Bounds-checked primary MBR/GPT partition discovery for block devices. */
#pragma once

#include <stddef.h>
#include <stdint.h>

#define ZEAL_PARTITION_SECTOR_SIZE 512u
#define ZEAL_PARTITION_MAX         16u
#define ZEAL_GPT_MAX_ENTRIES       128u
#define ZEAL_PARTITION_MBR_ZEAL_DATA 0xdau

enum zeal_partition_scheme {
    ZEAL_PARTITION_MBR = 1,
    ZEAL_PARTITION_GPT = 2,
};

enum zeal_partition_result {
    ZEAL_PARTITION_NOT_FOUND = 0,
    ZEAL_PARTITION_INVALID = -1,
    ZEAL_PARTITION_UNSUPPORTED = -2,
    ZEAL_PARTITION_OK = 1,
};

/* GPT type UUID 7a65616c-6f73-4152-6d36-340000000001, in on-disk byte order.
 * The MBR type is reserved by this image format for the same data partition. */
static const uint8_t ZEAL_PARTITION_GPT_ZEAL_DATA_GUID[16] = {
    0x6c, 0x61, 0x65, 0x7a, 0x73, 0x6f, 0x52, 0x41,
    0x6d, 0x36, 0x34, 0x00, 0x00, 0x00, 0x00, 0x01,
};

struct zeal_partition {
    uint64_t first_lba;
    uint64_t sector_count;
    uint8_t scheme;
    uint8_t mbr_type;
    uint8_t type_guid[16];
};

typedef int (*zeal_partition_read_fn)(void *context, uint64_t lba,
                                      uint8_t sector[ZEAL_PARTITION_SECTOR_SIZE]);

static uint32_t zeal_partition_crc_step(uint32_t crc, uint8_t value) {
    crc ^= value;
    for (unsigned bit = 0; bit < 8; bit++)
        crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
    return crc;
}

static uint32_t zeal_partition_crc(const uint8_t *data, size_t size) {
    uint32_t crc = 0xffffffffu;
    for (size_t i = 0; i < size; i++)
        crc = zeal_partition_crc_step(crc, data[i]);
    return crc ^ 0xffffffffu;
}

static uint32_t zeal_partition_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t zeal_partition_u64(const uint8_t *p) {
    return (uint64_t)zeal_partition_u32(p) |
           ((uint64_t)zeal_partition_u32(p + 4) << 32);
}

static int zeal_partition_is_zero_guid(const uint8_t *guid) {
    uint8_t value = 0;
    for (unsigned i = 0; i < 16; i++) value |= guid[i];
    return value == 0;
}

static int zeal_partition_range_valid(uint64_t first, uint64_t count,
                                      uint64_t capacity) {
    return first && count && first < capacity && count <= capacity - first;
}

static int zeal_partition_overlaps(const struct zeal_partition *parts,
                                   size_t count, uint64_t first,
                                   uint64_t sectors) {
    uint64_t end = first + sectors;
    for (size_t i = 0; i < count; i++) {
        uint64_t other_end = parts[i].first_lba + parts[i].sector_count;
        if (first < other_end && parts[i].first_lba < end) return 1;
    }
    return 0;
}

static int zeal_partition_add(struct zeal_partition *parts, size_t capacity,
                              size_t *count, uint64_t first, uint64_t sectors,
                              uint64_t disk_sectors, uint8_t scheme,
                              uint8_t mbr_type, const uint8_t *type_guid) {
    if (!zeal_partition_range_valid(first, sectors, disk_sectors) ||
        zeal_partition_overlaps(parts, *count, first, sectors))
        return ZEAL_PARTITION_INVALID;
    if (*count >= capacity) return ZEAL_PARTITION_UNSUPPORTED;
    struct zeal_partition *part = &parts[(*count)++];
    part->first_lba = first;
    part->sector_count = sectors;
    part->scheme = scheme;
    part->mbr_type = mbr_type;
    for (unsigned i = 0; i < 16; i++)
        part->type_guid[i] = type_guid ? type_guid[i] : 0;
    return ZEAL_PARTITION_OK;
}

static int zeal_partition_scan_gpt(zeal_partition_read_fn read_sector,
                                   void *context, uint64_t disk_sectors,
                                   struct zeal_partition *parts,
                                   size_t part_capacity, size_t *part_count) {
    uint8_t header[ZEAL_PARTITION_SECTOR_SIZE];
    if (!read_sector(context, 1, header) ||
        header[0] != 'E' || header[1] != 'F' || header[2] != 'I' ||
        header[3] != ' ' || header[4] != 'P' || header[5] != 'A' ||
        header[6] != 'R' || header[7] != 'T')
        return ZEAL_PARTITION_INVALID;

    uint32_t revision = zeal_partition_u32(header + 8);
    uint32_t header_size = zeal_partition_u32(header + 12);
    uint32_t expected_header_crc = zeal_partition_u32(header + 16);
    uint64_t current_lba = zeal_partition_u64(header + 24);
    uint64_t backup_lba = zeal_partition_u64(header + 32);
    uint64_t first_usable = zeal_partition_u64(header + 40);
    uint64_t last_usable = zeal_partition_u64(header + 48);
    uint64_t entries_lba = zeal_partition_u64(header + 72);
    uint32_t entry_count = zeal_partition_u32(header + 80);
    uint32_t entry_size = zeal_partition_u32(header + 84);
    uint32_t expected_entries_crc = zeal_partition_u32(header + 88);

    if (revision != 0x00010000u)
        return ZEAL_PARTITION_UNSUPPORTED;
    if (header_size < 92u || header_size > ZEAL_PARTITION_SECTOR_SIZE ||
        current_lba != 1 || backup_lba <= current_lba ||
        backup_lba >= disk_sectors || first_usable < 2 ||
        first_usable > last_usable || last_usable >= disk_sectors)
        return ZEAL_PARTITION_INVALID;
    if (!entry_count || entry_count > ZEAL_GPT_MAX_ENTRIES ||
        entry_size < 128u || entry_size > ZEAL_PARTITION_SECTOR_SIZE ||
        (entry_size & 7u) || ZEAL_PARTITION_SECTOR_SIZE % entry_size)
        return ZEAL_PARTITION_UNSUPPORTED;

    uint64_t table_bytes = (uint64_t)entry_count * entry_size;
    uint64_t table_sectors = (table_bytes + ZEAL_PARTITION_SECTOR_SIZE - 1u) /
                             ZEAL_PARTITION_SECTOR_SIZE;
    if (!zeal_partition_range_valid(entries_lba, table_sectors, disk_sectors) ||
        entries_lba < 2 || entries_lba + table_sectors > first_usable)
        return ZEAL_PARTITION_INVALID;

    uint8_t header_copy[ZEAL_PARTITION_SECTOR_SIZE];
    for (unsigned i = 0; i < ZEAL_PARTITION_SECTOR_SIZE; i++)
        header_copy[i] = header[i];
    for (unsigned i = 16; i < 20; i++) header_copy[i] = 0;
    if (zeal_partition_crc(header_copy, header_size) != expected_header_crc)
        return ZEAL_PARTITION_INVALID;

    uint32_t entries_crc = 0xffffffffu;
    uint64_t bytes_left = table_bytes;
    uint32_t entries_index = 0;
    uint8_t sector[ZEAL_PARTITION_SECTOR_SIZE];
    *part_count = 0;
    for (uint64_t sector_index = 0; sector_index < table_sectors;
         sector_index++) {
        if (!read_sector(context, entries_lba + sector_index, sector))
            return ZEAL_PARTITION_INVALID;
        size_t valid_bytes = bytes_left > ZEAL_PARTITION_SECTOR_SIZE ?
                             ZEAL_PARTITION_SECTOR_SIZE : (size_t)bytes_left;
        for (size_t i = 0; i < valid_bytes; i++)
            entries_crc = zeal_partition_crc_step(entries_crc, sector[i]);
        bytes_left -= valid_bytes;

        size_t entries_here = valid_bytes / entry_size;
        for (size_t i = 0; i < entries_here; i++, entries_index++) {
            const uint8_t *entry = sector + i * entry_size;
            if (zeal_partition_is_zero_guid(entry)) continue;
            uint64_t first = zeal_partition_u64(entry + 32);
            uint64_t last = zeal_partition_u64(entry + 40);
            if (first < first_usable || last > last_usable || first > last)
                return ZEAL_PARTITION_INVALID;
            int result = zeal_partition_add(parts, part_capacity, part_count,
                                            first, last - first + 1,
                                            disk_sectors, ZEAL_PARTITION_GPT,
                                            0, entry);
            if (result != ZEAL_PARTITION_OK) return result;
        }
    }
    if (bytes_left || entries_index != entry_count ||
        (entries_crc ^ 0xffffffffu) != expected_entries_crc)
        return ZEAL_PARTITION_INVALID;
    return ZEAL_PARTITION_OK;
}

static int zeal_partition_scan(zeal_partition_read_fn read_sector,
                               void *context, uint64_t disk_sectors,
                               struct zeal_partition *parts,
                               size_t part_capacity, size_t *part_count) {
    if (!read_sector || !disk_sectors || !parts || !part_capacity ||
        !part_count || part_capacity > ZEAL_PARTITION_MAX)
        return ZEAL_PARTITION_INVALID;
    *part_count = 0;
    uint8_t mbr[ZEAL_PARTITION_SECTOR_SIZE];
    if (!read_sector(context, 0, mbr) || mbr[510] != 0x55 || mbr[511] != 0xaa)
        return ZEAL_PARTITION_INVALID;

    int protective = 0;
    for (unsigned i = 0; i < 4; i++) {
        const uint8_t *entry = mbr + 446u + i * 16u;
        uint8_t status = entry[0], type = entry[4];
        uint32_t first = zeal_partition_u32(entry + 8);
        uint32_t sectors = zeal_partition_u32(entry + 12);
        if (!type && !first && !sectors) continue;
        if (status != 0 && status != 0x80)
            return ZEAL_PARTITION_INVALID;
        if (type == 0xee) {
            protective = 1;
            continue;
        }
        if (type == 0x05 || type == 0x0f || type == 0x85)
            return ZEAL_PARTITION_UNSUPPORTED;
        if (!type || !first || !sectors)
            return ZEAL_PARTITION_INVALID;
        int result = zeal_partition_add(parts, part_capacity, part_count,
                                        first, sectors, disk_sectors,
                                        ZEAL_PARTITION_MBR, type, 0);
        if (result != ZEAL_PARTITION_OK) return result;
    }
    if (protective) {
        *part_count = 0;
        return zeal_partition_scan_gpt(read_sector, context, disk_sectors,
                                       parts, part_capacity, part_count);
    }
    return ZEAL_PARTITION_OK;
}

/* Return NOT_FOUND, OK with the single designated partition, or INVALID if
 * the table marks more than one partition as writable ZealOS data. */
static int zeal_partition_find_data(const struct zeal_partition *parts,
                                    size_t part_count,
                                    struct zeal_partition *out) {
    size_t found = 0;
    if ((!parts && part_count) || !out || part_count > ZEAL_PARTITION_MAX)
        return ZEAL_PARTITION_INVALID;
    for (size_t i = 0; i < part_count; i++) {
        int guid_matches = parts[i].scheme == ZEAL_PARTITION_GPT;
        for (unsigned byte = 0; guid_matches && byte < 16; byte++)
            if (parts[i].type_guid[byte] !=
                ZEAL_PARTITION_GPT_ZEAL_DATA_GUID[byte])
                guid_matches = 0;
        int matches = (parts[i].scheme == ZEAL_PARTITION_MBR &&
                       parts[i].mbr_type == ZEAL_PARTITION_MBR_ZEAL_DATA) ||
                      guid_matches;
        if (!matches) continue;
        if (++found != 1) return ZEAL_PARTITION_INVALID;
        *out = parts[i];
    }
    return found ? ZEAL_PARTITION_OK : ZEAL_PARTITION_NOT_FOUND;
}
