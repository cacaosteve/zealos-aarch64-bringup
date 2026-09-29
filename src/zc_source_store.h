/* Append-only native ZealC source partition. See scripts/source-volume.py.
 * RedSea remains a separate 128-sector legacy volume at its original LBA. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "disk_layout.h"

#define ZSS_MAX_FILE (4u * 1024u * 1024u)
#define ZSS_MAX_NAME 95u

/* Bank A is the original 32 MiB partition. Bank B occupies the previously
 * reserved tail of the GPT disk. A v2 superblock is committed last when a
 * compacted bank is ready; old v1 images remain readable as generation 0. */
static uint64_t g_zss_base = ZEAL_SRC_LBA_BASE;
static uint32_t g_zss_sectors = ZEAL_SRC_SECTS;
static uint32_t g_zss_generation;

struct zss_entry {
    uint32_t header_sector, length, checksum, blocks;
};

static uint32_t zss_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void zss_w32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static int zss_eq(const uint8_t *p, const char *s, unsigned n) {
    unsigned i;
    for (i = 0; i < n; i++) if (p[i] != (uint8_t)s[i]) return 0;
    return 1;
}
static int zss_str_eq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}
static int zss_name_ok(const char *s, unsigned *out_len) {
    unsigned i = 0, segment = 0;
    if (!s || !*s) return 0;
    while (s[i]) {
        unsigned char c = (unsigned char)s[i];
        if (++i > ZSS_MAX_NAME || c < 33 || c > 126 || c == ':' || c == '\\')
            return 0;
        if (c == '/') {
            if (!segment) return 0;
            segment = 0;
        } else {
            segment++;
        }
    }
    if (!segment) return 0;
    /* Reject traversal components without banning dots in normal file names. */
    for (unsigned start = 0, end; start < i; start = end + 1) {
        end = start;
        while (end < i && s[end] != '/') end++;
        if ((end - start == 1 && s[start] == '.') ||
            (end - start == 2 && s[start] == '.' && s[start + 1] == '.'))
            return 0;
    }
    if (out_len) *out_len = i;
    return 1;
}
static uint32_t zss_crc_step(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (unsigned i = 0; i < 8; i++)
        crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    return crc;
}
static uint32_t zss_crc(const uint8_t *data, unsigned length) {
    uint32_t crc = 0xffffffffu;
    for (unsigned i = 0; i < length; i++) crc = zss_crc_step(crc, data[i]);
    return crc ^ 0xffffffffu;
}
static int zss_bank_read(uint64_t base, uint32_t sector, uint8_t buf[512]) {
    return virtio_blk_read_abs(buf, base + sector);
}
static int zss_bank_write(uint64_t base, uint32_t sector, const uint8_t buf[512]) {
    return virtio_blk_write_abs(buf, base + sector);
}
static int zss_sector_read(uint32_t sector, uint8_t buf[512]) {
    return sector < g_zss_sectors && zss_bank_read(g_zss_base, sector, buf);
}
static int zss_sector_write(uint32_t sector, const uint8_t buf[512]) {
    return sector < g_zss_sectors && zss_bank_write(g_zss_base, sector, buf);
}
/* A rejected upload may leave data beyond the first empty header. Before a
 * shorter append becomes visible, place a fresh empty terminator after it. */
static int zss_terminate_log(uint32_t next) {
    uint8_t zero[512];
    if (next >= g_zss_sectors) return 1;
    for (unsigned i = 0; i < 512; i++) zero[i] = 0;
    return zss_sector_write(next, zero);
}
static int zss_probe_bank(uint64_t base, uint32_t sectors, uint32_t *generation) {
    uint8_t sector[512];
    if (g_vb.full_capacity < base + sectors ||
        !zss_bank_read(base, 0, sector) || zss_u32(sector + 12) != sectors)
        return 0;
    if (zss_eq(sector, "ZCSRC001", 8) && zss_u32(sector + 8) == 1) {
        *generation = 0;
        return 1;
    }
    if (zss_eq(sector, "ZCSRC002", 8) && zss_u32(sector + 8) == 2 &&
        zss_u32(sector + 20) == zss_crc(sector, 20)) {
        *generation = zss_u32(sector + 16);
        return *generation != 0;
    }
    return 0;
}
static int zss_available(void) {
    uint32_t a_gen = 0, b_gen = 0;
    if (!virtio_blk_ready() || g_vb.xport != VB_XPORT_PCI) return 0;
    int a = zss_probe_bank(ZEAL_SRC_LBA_BASE, ZEAL_SRC_SECTS, &a_gen);
    int b = zss_probe_bank(ZEAL_SRC_SHADOW_LBA, ZEAL_SRC_SHADOW_SECTS, &b_gen);
    if (!a && !b) return 0;
    if (b && (!a || b_gen > a_gen)) {
        g_zss_base = ZEAL_SRC_SHADOW_LBA;
        g_zss_sectors = ZEAL_SRC_SHADOW_SECTS;
        g_zss_generation = b_gen;
    } else {
        g_zss_base = ZEAL_SRC_LBA_BASE;
        g_zss_sectors = ZEAL_SRC_SECTS;
        g_zss_generation = a_gen;
    }
    return 1;
}
/* ZCDEL1 is a header-only tombstone. It hides the same native path, including
 * any older version, without rewriting data that may still be in use. */
static int zss_decode_record(const uint8_t sector[512], uint32_t pos,
                             struct zss_entry *out, char name[ZSS_MAX_NAME + 1],
                             int *deleted) {
    uint32_t length = zss_u32(sector + 8);
    uint32_t checksum = zss_u32(sector + 12);
    uint32_t blocks = zss_u32(sector + 16);
    uint32_t name_len = zss_u32(sector + 20);
    int is_delete = zss_eq(sector, "ZCDEL1", 6) && !sector[6] && !sector[7];
    int is_file = zss_eq(sector, "ZCFILE1", 7) && !sector[7];
    if ((!is_file && !is_delete) || !name_len || name_len > ZSS_MAX_NAME ||
        pos + 1u + blocks > g_zss_sectors ||
        (is_delete ? (length || checksum || blocks) :
                     (!length || length > ZSS_MAX_FILE ||
                      blocks != (length + 511u) / 512u))) return -1;
    for (uint32_t i = 0; i < name_len; i++) name[i] = (char)sector[32 + i];
    name[name_len] = 0;
    if (!zss_name_ok(name, NULL)) return -1;
    if (out) *out = (struct zss_entry){pos, length, checksum, blocks};
    if (deleted) *deleted = is_delete;
    return 0;
}
/* 1 found, 0 absent, -1 damaged/I/O, -2 unavailable, -3 tombstoned. */
static int zss_find(const char *name, struct zss_entry *out,
                    uint32_t *out_tail, uint32_t *out_records) {
    uint8_t sector[512];
    char record_name[ZSS_MAX_NAME + 1];
    uint32_t pos = 1, records = 0;
    int found = 0;
    if (name && !zss_name_ok(name, NULL)) return -1;
    if (!zss_available()) return -2;
    while (pos < g_zss_sectors) {
        struct zss_entry entry;
        int deleted;
        int empty = 1;
        if (!zss_sector_read(pos, sector)) return -1;
        for (unsigned i = 0; i < 512; i++) if (sector[i]) { empty = 0; break; }
        if (empty) break;
        if (zss_decode_record(sector, pos, &entry, record_name, &deleted)) return -1;
        if (name && zss_str_eq(record_name, name)) {
            if (out) *out = entry;
            found = deleted ? -3 : 1;
        }
        pos += 1u + entry.blocks;
        records++;
    }
    if (out_tail) *out_tail = pos;
    if (out_records) *out_records = records;
    return found;
}
/* 0 deleted, -2 unavailable, -3 full, -4 missing, -1 invalid/I/O. */
static int zss_delete(const char *name) {
    uint8_t header[512];
    uint32_t tail = 0;
    unsigned name_len = 0;
    if (!zss_name_ok(name, &name_len)) return -1;
    int found = zss_find(name, NULL, &tail, NULL);
    if (found == 0 || found == -3) return -4;
    if (found < 0) return found;
    if (tail >= g_zss_sectors) return -3;
    for (unsigned i = 0; i < 512; i++) header[i] = 0;
    if (!zss_terminate_log(tail + 1u)) return -1;
    if (g_vb.flush_supported && !virtio_blk_flush_abs()) return -1;
    for (unsigned i = 0; i < 7; i++) header[i] = (uint8_t)"ZCDEL1"[i];
    zss_w32(header + 20, name_len);
    for (unsigned i = 0; i < name_len; i++) header[32 + i] = (uint8_t)name[i];
    if (!zss_sector_write(tail, header)) return -1;
    return g_vb.flush_supported && !virtio_blk_flush_abs() ? -5 : 0;
}
static int zss_read(const struct zss_entry *entry, char *dst, size_t cap, size_t *out_len) {
    uint8_t sector[512];
    uint32_t crc = 0xffffffffu, off = 0;
    if (out_len) *out_len = entry->length;
    if (!dst) return 0;
    if (cap <= entry->length) return -1;
    for (uint32_t b = 0; b < entry->blocks; b++) {
        uint32_t chunk = entry->length - off;
        if (chunk > 512) chunk = 512;
        if (!zss_sector_read(entry->header_sector + 1u + b, sector)) return -1;
        for (uint32_t i = 0; i < chunk; i++) {
            dst[off + i] = (char)sector[i];
            crc = zss_crc_step(crc, sector[i]);
        }
        off += chunk;
    }
    dst[entry->length] = 0;
    return (crc ^ 0xffffffffu) == entry->checksum ? 0 : -1;
}
static int zss_verify(const struct zss_entry *entry) {
    uint8_t sector[512];
    uint32_t crc = 0xffffffffu, off = 0;
    for (uint32_t b = 0; b < entry->blocks; b++) {
        uint32_t chunk = entry->length - off;
        if (chunk > 512) chunk = 512;
        if (!zss_sector_read(entry->header_sector + 1u + b, sector)) return -1;
        for (uint32_t i = 0; i < chunk; i++) crc = zss_crc_step(crc, sector[i]);
        off += chunk;
    }
    return (crc ^ 0xffffffffu) == entry->checksum ? 0 : -1;
}
/* 0 saved, -2 unavailable, -3 full, -1 invalid or I/O. */
static int zss_put(const char *name, const char *src, size_t len) {
    uint8_t sector[512];
    uint32_t tail = 0, blocks, crc = 0xffffffffu;
    unsigned name_len = 0;
    if (!src || !len || len > ZSS_MAX_FILE || !zss_name_ok(name, &name_len)) return -1;
    int scan = zss_find(NULL, NULL, &tail, NULL);
    if (scan < 0) return scan;
    blocks = (uint32_t)((len + 511u) / 512u);
    if (tail + 1u + blocks > g_zss_sectors) return -3;
    for (uint32_t b = 0; b < blocks; b++) {
        size_t off = (size_t)b * 512u, chunk = len - off;
        if (chunk > 512) chunk = 512;
        for (unsigned i = 0; i < 512; i++) sector[i] = 0;
        for (size_t i = 0; i < chunk; i++) {
            uint8_t byte = (uint8_t)src[off + i];
            sector[i] = byte;
            crc = zss_crc_step(crc, byte);
        }
        if (!zss_sector_write(tail + 1u + b, sector)) return -1;
    }
    for (unsigned i = 0; i < 512; i++) sector[i] = 0;
    if (!zss_terminate_log(tail + 1u + blocks)) return -1;
    if (g_vb.flush_supported && !virtio_blk_flush_abs()) return -1;
    for (unsigned i = 0; i < 7; i++) sector[i] = (uint8_t)"ZCFILE1"[i];
    zss_w32(sector + 8, (uint32_t)len);
    zss_w32(sector + 12, crc ^ 0xffffffffu);
    zss_w32(sector + 16, blocks);
    zss_w32(sector + 20, name_len);
    for (unsigned i = 0; i < name_len; i++) sector[32 + i] = (uint8_t)name[i];
    if (!zss_sector_write(tail, sector)) return -1;
    return g_vb.flush_supported && !virtio_blk_flush_abs() ? -5 : 0;
}

/* Copy only the latest record for each path to the inactive bank. Both data
 * and catalog are verified before its v2 superblock is committed. Until that
 * final sector write, the old bank stays selected across a guest restart. */
static int zss_compact(uint32_t *old_sectors, uint32_t *new_sectors,
                       uint32_t *kept_records) {
    uint8_t header[512], copied[512], check[512];
    char name[ZSS_MAX_NAME + 1];
    uint32_t tail, need = 1, kept = 0, target_sectors, next = 1;
    uint64_t source, target;
    if (zss_find(NULL, NULL, &tail, NULL) < 0) return -2;
    source = g_zss_base;
    target = source == ZEAL_SRC_LBA_BASE ? ZEAL_SRC_SHADOW_LBA : ZEAL_SRC_LBA_BASE;
    target_sectors = source == ZEAL_SRC_LBA_BASE ?
                     ZEAL_SRC_SHADOW_SECTS : ZEAL_SRC_SECTS;
    if (g_vb.full_capacity < target + target_sectors ||
        g_zss_generation == UINT32_MAX || !g_vb.flush_supported) return -4;
    if (!virtio_blk_flush_abs()) return -1;
    /* Do all capacity and source-CRC checks before modifying the other bank. */
    for (uint32_t pos = 1; pos < tail;) {
        struct zss_entry entry, latest;
        int deleted, status;
        if (!zss_sector_read(pos, header) ||
            zss_decode_record(header, pos, &entry, name, &deleted)) return -1;
        status = zss_find(name, &latest, NULL, NULL);
        if (status != 1 && status != -3) return -1;
        if (latest.header_sector == pos &&
            status == (deleted ? -3 : 1)) {
            if (!deleted && zss_verify(&entry)) return -1;
            if (need + 1u + entry.blocks > target_sectors) return -3;
            need += 1u + entry.blocks;
            kept++;
        }
        pos += 1u + entry.blocks;
    }
    /* An old target superblock has a lower generation. Partial writes to that
     * bank cannot become active if copying is interrupted. */
    for (uint32_t pos = 1; pos < tail;) {
        struct zss_entry entry, latest;
        int deleted, status;
        if (!zss_sector_read(pos, header) ||
            zss_decode_record(header, pos, &entry, name, &deleted)) return -1;
        status = zss_find(name, &latest, NULL, NULL);
        if (status != 1 && status != -3) return -1;
        if (latest.header_sector == pos &&
            status == (deleted ? -3 : 1)) {
            for (uint32_t b = 0; b <= entry.blocks; b++) {
                if (!zss_sector_read(pos + b, copied) ||
                    !zss_bank_write(target, next + b, copied) ||
                    !zss_bank_read(target, next + b, check)) return -1;
                for (unsigned i = 0; i < 512; i++)
                    if (copied[i] != check[i]) return -1;
            }
            next += 1u + entry.blocks;
        }
        pos += 1u + entry.blocks;
    }
    if (next != need) return -1;
    if (next < target_sectors) {
        for (unsigned i = 0; i < 512; i++) header[i] = 0;
        if (!zss_bank_write(target, next, header)) return -1;
    }
    if (!virtio_blk_flush_abs()) return -1;
    for (unsigned i = 0; i < 512; i++) header[i] = 0;
    for (unsigned i = 0; i < 8; i++) header[i] = (uint8_t)"ZCSRC002"[i];
    zss_w32(header + 8, 2);
    zss_w32(header + 12, target_sectors);
    zss_w32(header + 16, g_zss_generation + 1u);
    zss_w32(header + 20, zss_crc(header, 20));
    if (!zss_bank_write(target, 0, header)) return -1;
    if (!virtio_blk_flush_abs()) return -5;
    if (!zss_available() || g_zss_base != target) return -1;
    if (old_sectors) *old_sectors = tail;
    if (new_sectors) *new_sectors = next;
    if (kept_records) *kept_records = kept;
    return 0;
}

/* Streaming append for a serial upload. Data sectors are written first; the
 * record becomes visible only after length and CRC agree and the header is
 * written. A failed upload leaves the old version of the name intact. */
struct zss_stream {
    uint32_t tail, length, written, expected_crc, crc;
    unsigned name_len;
    char name[ZSS_MAX_NAME + 1];
    uint8_t sector[512];
};
static int zss_stream_begin(struct zss_stream *s, const char *name,
                            uint32_t length, uint32_t expected_crc) {
    uint32_t tail = 0;
    unsigned name_len = 0;
    if (!s || !length || length > ZSS_MAX_FILE ||
        !zss_name_ok(name, &name_len)) return -1;
    int scan = zss_find(NULL, NULL, &tail, NULL);
    if (scan < 0) return scan;
    if (tail + 1u + (length + 511u) / 512u > g_zss_sectors) return -3;
    for (size_t i = 0; i < sizeof(*s); i++) ((uint8_t *)s)[i] = 0;
    for (unsigned i = 0; i < name_len; i++) s->name[i] = name[i];
    s->name_len = name_len;
    s->tail = tail;
    s->length = length;
    s->expected_crc = expected_crc;
    s->crc = 0xffffffffu;
    return 0;
}
static int zss_stream_write(struct zss_stream *s, const uint8_t *src, size_t length) {
    if (!s || !src || length > s->length - s->written) return -1;
    for (size_t i = 0; i < length; i++) {
        unsigned pos = s->written & 511u;
        s->sector[pos] = src[i];
        s->crc = zss_crc_step(s->crc, src[i]);
        s->written++;
        if ((s->written & 511u) == 0) {
            if (!zss_sector_write(s->tail + s->written / 512u, s->sector)) return -1;
            for (unsigned j = 0; j < 512; j++) s->sector[j] = 0;
        }
    }
    return 0;
}
/* 0 saved, -4 checksum/length mismatch, -1 I/O. */
static int zss_stream_finish(struct zss_stream *s) {
    uint8_t header[512];
    if (!s || s->written != s->length ||
        (s->crc ^ 0xffffffffu) != s->expected_crc) return -4;
    if ((s->written & 511u) &&
        !zss_sector_write(s->tail + 1u + s->written / 512u, s->sector)) return -1;
    for (unsigned i = 0; i < 512; i++) header[i] = 0;
    if (!zss_terminate_log(s->tail + 1u + (s->length + 511u) / 512u)) return -1;
    if (g_vb.flush_supported && !virtio_blk_flush_abs()) return -1;
    for (unsigned i = 0; i < 7; i++) header[i] = (uint8_t)"ZCFILE1"[i];
    zss_w32(header + 8, s->length);
    zss_w32(header + 12, s->expected_crc);
    zss_w32(header + 16, (s->length + 511u) / 512u);
    zss_w32(header + 20, s->name_len);
    for (unsigned i = 0; i < s->name_len; i++) header[32 + i] = (uint8_t)s->name[i];
    if (!zss_sector_write(s->tail, header)) return -1;
    return g_vb.flush_supported && !virtio_blk_flush_abs() ? -5 : 0;
}
