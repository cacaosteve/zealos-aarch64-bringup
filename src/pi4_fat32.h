/* Bounded read-only FAT32 root discovery. No allocation and no write callback.
 * Disk layout: Microsoft FAT specification v1.03, BPB/FAT/directory sections. */
#pragma once
#include <stdint.h>

#define PI4_FAT32_ENTRIES 8u
#define PI4_FAT32_SECTOR_LIMIT 8u
#define PI4_FAT32_CLUSTER_LIMIT 8u
#define PI4_FAT32_FILE_MAX_BYTES 2048u
#define PI4_FAT32_FILE_CLUSTER_LIMIT 4u
#define PI4_FAT32_TEST_BYTES 1025u
#define PI4_FAT32_TEST_NAME "ZEALTEST.TXT"
#define PI4_FAT32_TEST_PATTERN "ZealOS Pi4B FAT32 read test.\n"
#define PI4_FAT32_TEST_CRC32 0x873b79acu

enum pi4_fat32_result {
    PI4_FAT32_NOT_RUN, PI4_FAT32_UNSUPPORTED, PI4_FAT32_PARTITION_BAD,
    PI4_FAT32_BOOT_READ_BAD, PI4_FAT32_BPB_BAD, PI4_FAT32_FAT_READ_BAD,
    PI4_FAT32_CHAIN_BAD, PI4_FAT32_DIRECTORY_READ_BAD,
    PI4_FAT32_DIRECTORY_ENTRY_BAD, PI4_FAT32_ROOT_PASS, PI4_FAT32_ROOT_PARTIAL
};
struct pi4_fat32_volume {
    uint32_t first, sectors, fat_first, fat_sectors, data_first;
    uint32_t sectors_per_cluster, clusters, root_cluster, active_fat;
};
struct pi4_fat32_entry {
    char name[13];
    uint32_t attributes, cluster, bytes;
};
struct pi4_fat32_report {
    unsigned result, partition_index, partition_type;
    unsigned partition_types[4];
    uint32_t partition_first, partition_sectors;
    unsigned listed, directory_sectors, clusters, complete, bounded, reads;
    uint32_t last_read;
    char label[12];
    struct pi4_fat32_volume volume;
    struct pi4_fat32_entry entry[PI4_FAT32_ENTRIES];
};
typedef int (*pi4_fat32_read_fn)(void *, uint32_t, uint8_t[512]);

static const char *pi4_fat32_result_name(unsigned result) {
    switch (result) {
    case PI4_FAT32_UNSUPPORTED: return "unsupported-layout";
    case PI4_FAT32_PARTITION_BAD: return "invalid-partition";
    case PI4_FAT32_BOOT_READ_BAD: return "boot-read-failed";
    case PI4_FAT32_BPB_BAD: return "invalid-BPB";
    case PI4_FAT32_FAT_READ_BAD: return "FAT-read-failed";
    case PI4_FAT32_CHAIN_BAD: return "invalid-cluster-chain";
    case PI4_FAT32_DIRECTORY_READ_BAD: return "directory-read-failed";
    case PI4_FAT32_DIRECTORY_ENTRY_BAD: return "invalid-directory-entry";
    case PI4_FAT32_ROOT_PASS: return "root-directory-pass";
    case PI4_FAT32_ROOT_PARTIAL: return "root-directory-bounded";
    default: return "not-run";
    }
}

static uint32_t pi4_fat32_le16(const uint8_t *p) {
    return p[0] | ((uint32_t)p[1] << 8);
}
static uint32_t pi4_fat32_le32(const uint8_t *p) {
    return pi4_fat32_le16(p) | (pi4_fat32_le16(p + 2u) << 16);
}
static int pi4_fat32_signature(const uint8_t *p) {
    return p[510] == 0x55u && p[511] == 0xaau;
}
static int pi4_fat32_bpb(const uint8_t b[512], uint32_t first,
        uint32_t partition_sectors, uint32_t last_lba, struct pi4_fat32_volume *v) {
    uint32_t spc = b[13], reserved = pi4_fat32_le16(b + 14u), fats = b[16];
    uint32_t total = pi4_fat32_le32(b + 32u), fat = pi4_fat32_le32(b + 36u);
    uint32_t flags = pi4_fat32_le16(b + 40u), active = flags & 0x80u ? flags & 15u : 0u;
    uint32_t root = pi4_fat32_le32(b + 44u);
    uint64_t overhead = (uint64_t)reserved + (uint64_t)fats * fat;
    uint64_t end = (uint64_t)first + total;
    uint64_t clusters;
    if (!v || !pi4_fat32_signature(b) ||
        !((b[0] == 0xebu && b[2] == 0x90u) || b[0] == 0xe9u) ||
        pi4_fat32_le16(b + 11u) != 512u || !spc || spc > 128u || (spc & (spc - 1u)) ||
        !reserved || (fats != 1u && fats != 2u) || !fat || !total ||
        pi4_fat32_le16(b + 17u) || pi4_fat32_le16(b + 19u) ||
        pi4_fat32_le16(b + 22u) || pi4_fat32_le16(b + 42u) ||
        (flags & 0xff70u) || active >= fats || overhead >= total ||
        total > partition_sectors || end > (uint64_t)last_lba + 1u)
        return 0;
    clusters = (total - overhead) / spc;
    /* Classify by cluster count, never by the cosmetic "FAT32" label. */
    if (clusters < 65525u || clusters + 1u >= 0x0ffffff0u ||
        clusters + 2u > (uint64_t)fat * 128u || root < 2u || root > clusters + 1u)
        return 0;
    v->first = first; v->sectors = total;
    v->fat_first = first + reserved + active * fat;
    v->fat_sectors = fat; v->data_first = first + (uint32_t)overhead;
    v->sectors_per_cluster = spc; v->clusters = (uint32_t)clusters;
    v->root_cluster = root; v->active_fat = active;
    return 1;
}

static int pi4_fat32_cluster_lba(const struct pi4_fat32_volume *v,
        uint32_t cluster, uint32_t *lba) {
    uint64_t start;
    if (!v || !lba || cluster < 2u || cluster > v->clusters + 1u) return 0;
    start = (uint64_t)v->data_first + (uint64_t)(cluster - 2u) * v->sectors_per_cluster;
    if (start + v->sectors_per_cluster > (uint64_t)v->first + v->sectors ||
        start > UINT32_MAX) return 0;
    *lba = (uint32_t)start;
    return 1;
}

/* 0=end, 1=live entry, 2=skip deleted/LFN/label/dot entries, -1=malformed. */
static int pi4_fat32_directory_entry(const uint8_t *d,
        const struct pi4_fat32_volume *v, struct pi4_fat32_entry *entry) {
    unsigned n = 0, attr = d[11];
    uint32_t cluster, bytes;
    if (!d[0]) return 0;
    if (d[0] == 0xe5u || attr == 0x0fu || (attr & 8u) ||
        (d[0] == '.' && (d[1] == ' ' || (d[1] == '.' && d[2] == ' '))))
        return 2;
    cluster = ((pi4_fat32_le16(d + 20u) << 16) | pi4_fat32_le16(d + 26u)) & 0x0fffffffu;
    bytes = pi4_fat32_le32(d + 28u);
    if ((attr & 0xc0u) || d[0] == ' ' ||
        ((bytes || (attr & 16u) || cluster) &&
         (cluster < 2u || cluster > v->clusters + 1u)))
        return -1;
    for (unsigned part = 0; part < 2u; part++) {
        unsigned start = part ? 8u : 0u, end = part ? 11u : 8u;
        while (end > start && d[end - 1u] == ' ') end--;
        if (part && end > start) entry->name[n++] = '.';
        for (unsigned i = start; i < end; i++) {
            unsigned c = d[i];
            if (c < 0x20u || c > 0x7eu) c = '?';
            if ((d[12] & (part ? 16u : 8u)) && c >= 'A' && c <= 'Z') c += 'a' - 'A';
            entry->name[n++] = (char)c;
        }
    }
    entry->name[n] = 0;
    entry->attributes = attr; entry->cluster = cluster; entry->bytes = bytes;
    return 1;
}

static int pi4_fat32_read(pi4_fat32_read_fn read, void *opaque,
        struct pi4_fat32_report *r, uint32_t lba, uint8_t *buffer) {
    r->reads++; r->last_read = lba;
    return read(opaque, lba, buffer);
}

static void pi4_fat32_probe(const uint8_t sector0[512], uint32_t last_lba,
        unsigned block_bytes, pi4_fat32_read_fn read, void *opaque,
        struct pi4_fat32_report *r) {
    uint8_t buffer[512];
    uint32_t seen[PI4_FAT32_CLUSTER_LIMIT], cluster;
    if (!r) return;
    for (unsigned i = 0; i < sizeof(*r); i++) ((uint8_t *)(void *)r)[i] = 0;
    r->result = PI4_FAT32_UNSUPPORTED;
    if (!sector0 || !read || block_bytes != 512u || !pi4_fat32_signature(sector0))
        return;
    if (last_lba != UINT32_MAX &&
        pi4_fat32_bpb(sector0, 0u, last_lba + 1u, last_lba, &r->volume)) {
        r->partition_sectors = last_lba + 1u;
        for (unsigned i = 0; i < 512u; i++) buffer[i] = sector0[i];
    } else {
        int found = 0;
        for (unsigned i = 0; i < 4u; i++) r->partition_types[i] = sector0[450u + i * 16u];
        for (unsigned i = 0; i < 4u; i++) {
            const uint8_t *p = sector0 + 446u + i * 16u;
            unsigned type = p[4];
            uint32_t first = pi4_fat32_le32(p + 8u), sectors = pi4_fat32_le32(p + 12u);
            if (type == 0xeeu) return; /* GPT needs a separate bounded parser. */
            if (type != 0x0bu && type != 0x0cu && type != 0x1bu && type != 0x1cu) continue;
            if ((p[0] != 0u && p[0] != 0x80u) || !first || !sectors ||
                (uint64_t)first + sectors > (uint64_t)last_lba + 1u) {
                r->result = PI4_FAT32_PARTITION_BAD; return;
            }
            if (!found) {
                found = 1; r->partition_index = i + 1u; r->partition_type = type;
                r->partition_first = first; r->partition_sectors = sectors;
            }
        }
        if (!found) return;
        r->result = PI4_FAT32_BOOT_READ_BAD;
        if (!pi4_fat32_read(read, opaque, r, r->partition_first, buffer)) return;
        r->result = PI4_FAT32_BPB_BAD;
        if (!pi4_fat32_bpb(buffer, r->partition_first, r->partition_sectors,
                            last_lba, &r->volume)) return;
    }
    for (unsigned i = 0; i < 11u; i++)
        r->label[i] = buffer[71u + i] >= 0x20u && buffer[71u + i] <= 0x7eu ?
            (char)buffer[71u + i] : '?';
    r->label[11] = 0;
    cluster = r->volume.root_cluster;
    for (unsigned hop = 0; hop < PI4_FAT32_CLUSTER_LIMIT; hop++) {
        uint32_t first, fat_sector = cluster / 128u, next;
        r->result = PI4_FAT32_CHAIN_BAD;
        for (unsigned i = 0; i < hop; i++) if (seen[i] == cluster) return;
        seen[hop] = cluster;
        if (!pi4_fat32_cluster_lba(&r->volume, cluster, &first) ||
            fat_sector >= r->volume.fat_sectors) return;
        r->result = PI4_FAT32_FAT_READ_BAD;
        if (!pi4_fat32_read(read, opaque, r, r->volume.fat_first + fat_sector, buffer)) return;
        next = pi4_fat32_le32(buffer + (cluster % 128u) * 4u) & 0x0fffffffu;
        r->result = PI4_FAT32_CHAIN_BAD;
        if (next < 0x0ffffff8u &&
            (next < 2u || next >= 0x0ffffff0u || next > r->volume.clusters + 1u)) return;
        r->clusters++;
        for (unsigned sector = 0; sector < r->volume.sectors_per_cluster; sector++) {
            if (r->directory_sectors >= PI4_FAT32_SECTOR_LIMIT) goto bounded;
            r->result = PI4_FAT32_DIRECTORY_READ_BAD;
            if (!pi4_fat32_read(read, opaque, r, first + sector, buffer)) return;
            r->directory_sectors++;
            for (unsigned offset = 0; offset < 512u; offset += 32u) {
                struct pi4_fat32_entry entry;
                int rc = pi4_fat32_directory_entry(buffer + offset, &r->volume, &entry);
                if (rc < 0) { r->result = PI4_FAT32_DIRECTORY_ENTRY_BAD; return; }
                if (rc == 0) goto complete;
                if (rc == 2) continue;
                r->entry[r->listed++] = entry;
                if (r->listed == PI4_FAT32_ENTRIES) goto bounded;
            }
        }
        if (next >= 0x0ffffff8u) goto complete;
        cluster = next;
    }
bounded:
    r->bounded = 1u; r->result = PI4_FAT32_ROOT_PARTIAL; return;
complete:
    r->complete = 1u; r->result = PI4_FAT32_ROOT_PASS;
}

enum pi4_fat32_file_result {
    PI4_FAT32_FILE_NOT_RUN, PI4_FAT32_FILE_NOT_FOUND, PI4_FAT32_FILE_ROOT_BOUNDED,
    PI4_FAT32_FILE_INVALID, PI4_FAT32_FILE_TOO_LARGE, PI4_FAT32_FILE_FAT_READ_BAD,
    PI4_FAT32_FILE_CHAIN_BAD, PI4_FAT32_FILE_DATA_READ_BAD, PI4_FAT32_FILE_READ_PASS
};
struct pi4_fat32_file_report {
    unsigned result, bytes, reads, clusters;
    uint32_t last_read;
};
static const char *pi4_fat32_file_result_name(unsigned result) {
    switch (result) {
    case PI4_FAT32_FILE_NOT_FOUND: return "test-file-not-found";
    case PI4_FAT32_FILE_ROOT_BOUNDED: return "test-file-not-seen-in-bounded-root";
    case PI4_FAT32_FILE_INVALID: return "invalid-file-entry";
    case PI4_FAT32_FILE_TOO_LARGE: return "file-exceeds-buffer";
    case PI4_FAT32_FILE_FAT_READ_BAD: return "file-FAT-read-failed";
    case PI4_FAT32_FILE_CHAIN_BAD: return "invalid-file-chain";
    case PI4_FAT32_FILE_DATA_READ_BAD: return "file-data-read-failed";
    case PI4_FAT32_FILE_READ_PASS: return "file-read-pass";
    default: return "not-run";
    }
}

static int pi4_fat32_name_equal(const char *a, const char *b) {
    for (unsigned i = 0; i < 13u; i++) {
        unsigned ac = (uint8_t)a[i], bc = (uint8_t)b[i];
        if (ac >= 'a' && ac <= 'z') ac -= 'a' - 'A';
        if (bc >= 'a' && bc <= 'z') bc -= 'a' - 'A';
        if (ac != bc) return 0;
        if (!ac) return 1;
    }
    return 0;
}

static const struct pi4_fat32_entry *pi4_fat32_find_file(
        const struct pi4_fat32_report *root, const char *name) {
    if (!root || !name || root->listed > PI4_FAT32_ENTRIES) return 0;
    for (unsigned i = 0; i < root->listed; i++)
        if (!(root->entry[i].attributes & 0xd8u) &&
            pi4_fat32_name_equal(root->entry[i].name, name)) return &root->entry[i];
    return 0;
}

/* Read exactly the declared length of a small file through the same bounded
 * FAT chain rules as the root scan. Last-sector padding never enters output.
 * A short/cyclic/free/bad chain or any I/O fault is a failure, not a short read. */
static void pi4_fat32_read_file(const struct pi4_fat32_volume *v,
        const struct pi4_fat32_entry *file, pi4_fat32_read_fn read, void *opaque,
        uint8_t *out, unsigned capacity, struct pi4_fat32_file_report *r) {
    uint8_t buffer[512];
    uint32_t seen[PI4_FAT32_FILE_CLUSTER_LIMIT], cluster;
    if (!r) return;
    for (unsigned i = 0; i < sizeof(*r); i++) ((uint8_t *)(void *)r)[i] = 0;
    r->result = PI4_FAT32_FILE_INVALID;
    if (!v || !file || !read || !out || (file->attributes & 0xd8u) ||
        !v->sectors_per_cluster || v->sectors_per_cluster > 128u)
        return;
    if (file->bytes > capacity || file->bytes > PI4_FAT32_FILE_MAX_BYTES) {
        r->result = PI4_FAT32_FILE_TOO_LARGE; return;
    }
    if (!file->bytes) { r->result = PI4_FAT32_FILE_READ_PASS; return; }
    cluster = file->cluster;
    for (unsigned hop = 0; hop < PI4_FAT32_FILE_CLUSTER_LIMIT; hop++) {
        uint32_t first, fat_sector = cluster / 128u, next;
        r->result = PI4_FAT32_FILE_CHAIN_BAD;
        for (unsigned i = 0; i < hop; i++) if (seen[i] == cluster) return;
        seen[hop] = cluster;
        if (!pi4_fat32_cluster_lba(v, cluster, &first) || fat_sector >= v->fat_sectors) return;
        r->result = PI4_FAT32_FILE_FAT_READ_BAD;
        r->reads++; r->last_read = v->fat_first + fat_sector;
        if (!read(opaque, r->last_read, buffer)) return;
        next = pi4_fat32_le32(buffer + (cluster % 128u) * 4u) & 0x0fffffffu;
        r->result = PI4_FAT32_FILE_CHAIN_BAD;
        if (next < 0x0ffffff8u &&
            (next < 2u || next >= 0x0ffffff0u || next > v->clusters + 1u)) return;
        r->clusters++;
        for (unsigned sector = 0; sector < v->sectors_per_cluster; sector++) {
            unsigned remaining = file->bytes - r->bytes;
            unsigned count = remaining < 512u ? remaining : 512u;
            r->result = PI4_FAT32_FILE_DATA_READ_BAD;
            r->reads++; r->last_read = first + sector;
            if (!read(opaque, r->last_read, buffer)) return;
            for (unsigned i = 0; i < count; i++) out[r->bytes + i] = buffer[i];
            r->bytes += count;
            if (r->bytes == file->bytes) { r->result = PI4_FAT32_FILE_READ_PASS; return; }
        }
        r->result = PI4_FAT32_FILE_CHAIN_BAD;
        if (next >= 0x0ffffff8u) return; /* EOF before the directory's length. */
        cluster = next;
    }
    r->result = PI4_FAT32_FILE_CHAIN_BAD;
}

static int pi4_fat32_test_match(const uint8_t *data, unsigned length) {
    static const char pattern[] = PI4_FAT32_TEST_PATTERN;
    if (!data || length != PI4_FAT32_TEST_BYTES) return 0;
    for (unsigned i = 0; i < length; i++)
        if (data[i] != (uint8_t)pattern[i % (sizeof(pattern) - 1u)]) return 0;
    return 1;
}
