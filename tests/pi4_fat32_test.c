#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/pi4_fat32.h"

static void put16(uint8_t *p, uint32_t v) { p[0] = v; p[1] = v >> 8; }
static void put32(uint8_t *p, uint32_t v) { put16(p, v); put16(p + 2, v >> 16); }
static int fail(const char *s) { fprintf(stderr, "FAIL Pi4 FAT32: %s\n", s); return 1; }
struct disk {
    uint8_t mbr[512], boot[512], fat[512], fat0[512], root[8][512], file[3][512];
    uint32_t fail_lba, log[32];
    unsigned reads;
};
#define FIRST 2048u
#define TOTAL 131072u
#define LAST (FIRST + TOTAL - 1u)
#define FAT_LBA (FIRST + 32u + 1024u + 1u)
#define ROOT_LBA (FIRST + 2080u + 128u)
static void fixture(struct disk *d) {
    memset(d, 0, sizeof(*d)); d->fail_lba = UINT32_MAX;
    d->mbr[510] = d->boot[510] = 0x55; d->mbr[511] = d->boot[511] = 0xaa;
    d->mbr[450] = 0x0c; put32(d->mbr + 454, FIRST); put32(d->mbr + 458, TOTAL);
    d->boot[0] = 0xeb; d->boot[2] = 0x90;
    put16(d->boot + 11, 512); d->boot[13] = 1;
    put16(d->boot + 14, 32); d->boot[16] = 2;
    put32(d->boot + 32, TOTAL); put32(d->boot + 36, 1024);
    put16(d->boot + 40, 0x81); /* Explicitly select second FAT. */
    put32(d->boot + 44, 130); memcpy(d->boot + 71, "TEST VOLUME", 11);
    put32(d->fat + 8, 0xffffffffu); /* Cluster 130 at second FAT sector offset 8. */
    put32(d->fat + 12, 0x0fffffffu);
    memcpy(d->root[0], "HELLO   TXT", 11); d->root[0][11] = 0x20;
    put16(d->root[0] + 26, 3); put32(d->root[0] + 28, 5);
}
static int read_disk(void *opaque, uint32_t lba, uint8_t out[512]) {
    struct disk *d = opaque;
    if (d->reads >= 32u || lba > LAST) return 0;
    d->log[d->reads++] = lba;
    if (lba == d->fail_lba) return 0;
    const uint8_t *source = lba == FIRST ? d->boot :
        lba == FAT_LBA ? d->fat :
        lba == FAT_LBA - 1u ? d->fat0 :
        lba == FIRST + 2081u ? d->file[0] :
        lba == FIRST + 2085u ? d->file[1] :
        lba == FIRST + 2087u ? d->file[2] :
        lba >= ROOT_LBA && lba < ROOT_LBA + 8u ? d->root[lba - ROOT_LBA] : NULL;
    if (!source) return 0;
    memcpy(out, source, 512); return 1;
}
static int read_file(void *opaque, uint32_t lba, uint8_t out[512]) {
    FILE *f = opaque;
    return fseek(f, (long)((uint64_t)lba * 512u), SEEK_SET) == 0 && fread(out, 1, 512, f) == 512;
}
static void file_fixture(struct disk *d) {
    static const char pattern[] = PI4_FAT32_TEST_PATTERN;
    fixture(d);
    memcpy(d->root[0], "ZEALTESTTXT", 11); put32(d->root[0] + 28, PI4_FAT32_TEST_BYTES);
    put32(d->fat0 + 3u * 4u, 7u); put32(d->fat0 + 7u * 4u, 9u);
    put32(d->fat0 + 9u * 4u, 0x0fffffffu);
    memset(d->file, 0xcc, sizeof(d->file));
    for (unsigned i = 0; i < PI4_FAT32_TEST_BYTES; i++)
        d->file[i / 512u][i % 512u] = (uint8_t)pattern[i % (sizeof(pattern) - 1u)];
}
int main(int argc, char **argv) {
    struct disk d;
    struct pi4_fat32_report r;
    fixture(&d);
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_ROOT_PASS || !r.complete || r.bounded ||
        r.listed != 1u || strcmp(r.entry[0].name, "HELLO.TXT") ||
        r.entry[0].bytes != 5u || r.entry[0].cluster != 3u ||
        r.partition_first != FIRST || r.volume.active_fat != 1u ||
        r.volume.root_cluster != 130u || r.volume.data_first != FIRST + 2080u ||
        d.reads != 3u || d.log[0] != FIRST || d.log[1] != FAT_LBA || d.log[2] != ROOT_LBA)
        return fail("partition, active FAT, root cluster beyond first FAT sector and root entry");
    /* Root spans two non-default clusters; deleted and LFN records are skipped. */
    fixture(&d); put32(d.fat + 8, 131u);
    for (unsigned i = 0; i < 512; i += 32) { d.root[0][i] = 0xe5; d.root[0][i + 11] = 0x20; }
    d.root[0][0] = 0x41; d.root[0][11] = 0x0f;
    memcpy(d.root[1], "SUBDIR     ", 11); d.root[1][11] = 16; put16(d.root[1] + 26, 5);
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_ROOT_PASS || r.clusters != 2u || r.directory_sectors != 2u ||
        r.listed != 1u || strcmp(r.entry[0].name, "SUBDIR") || !(r.entry[0].attributes & 16u))
        return fail("root chain traversal across clusters and skipped records");
    put32(d.fat + 8, 130u); d.reads = 0;
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_CHAIN_BAD || d.reads > 3u)
        return fail("cyclic root FAT chain terminates");
    fixture(&d); put32(d.fat + 8, 0x0ffffff7u);
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_CHAIN_BAD || r.directory_sectors)
        return fail("bad cluster marker stops before directory read");
    fixture(&d); put32(d.fat + 8, 0u);
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_CHAIN_BAD) return fail("unallocated root cluster rejects");
    fixture(&d); d.fail_lba = ROOT_LBA;
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_DIRECTORY_READ_BAD || r.last_read != ROOT_LBA)
        return fail("I/O failure is separate from malformed metadata");
    fixture(&d); put16(d.root[0] + 26, 1);
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_DIRECTORY_ENTRY_BAD) return fail("invalid live file cluster rejects");
    fixture(&d);
    for (unsigned i = 0; i < 16; i++) {
        if (i) memcpy(d.root[0] + i * 32, d.root[0], 32);
        d.root[0][i * 32] = 'A' + i;
    }
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_ROOT_PARTIAL || !r.bounded || r.complete || r.listed != 8u)
        return fail("entry cap reports partial rather than complete directory");
    fixture(&d);
    for (unsigned cluster = 0; cluster < 8u; cluster++) {
        put32(d.fat + 8u + cluster * 4u, 131u + cluster);
        for (unsigned offset = 0; offset < 512u; offset += 32u) d.root[cluster][offset] = 0xe5u;
    }
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_ROOT_PARTIAL || !r.bounded || r.complete ||
        r.directory_sectors != 8u || r.clusters != 8u || d.reads != 17u)
        return fail("long deleted-entry directory stops at sector/cluster budget");
    fixture(&d); d.mbr[450] = 0xee;
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_UNSUPPORTED || d.reads) return fail("GPT is explicitly unsupported");
    fixture(&d); put32(d.mbr + 454, UINT32_MAX - 10u);
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_PARTITION_BAD || d.reads) return fail("partition overflow/extent guard");
    fixture(&d);
    const unsigned offset[] = {11, 13, 17, 22, 42};
    const uint8_t value[] = {0, 3, 1, 1, 1};
    for (unsigned i = 0; i < sizeof(offset) / sizeof(offset[0]); i++) {
        unsigned saved = d.boot[offset[i]];
        d.boot[offset[i]] = value[i]; d.reads = 0;
        if (i == 0u) d.boot[12] = 0;
        pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
        if (r.result != PI4_FAT32_BPB_BAD) return fail("invalid BPB geometry/version/type rejects");
        d.boot[offset[i]] = saved; d.boot[12] = 2;
    }
    fixture(&d); put32(d.boot + 36, 1);
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_BPB_BAD) return fail("FAT capacity covers all data clusters");
    fixture(&d); put32(d.boot + 32, 60000u);
    pi4_fat32_probe(d.mbr, LAST, 512, read_disk, &d, &r);
    if (r.result != PI4_FAT32_BPB_BAD) return fail("cluster count classifies FAT32, not a type string");
    fixture(&d); pi4_fat32_probe(d.mbr, LAST, 1024, read_disk, &d, &r);
    if (r.result != PI4_FAT32_UNSUPPORTED || d.reads) return fail("physical block-size gate");
    file_fixture(&d);
    pi4_fat32_probe(d.mbr, LAST, 512u, read_disk, &d, &r);
    const struct pi4_fat32_entry *test = pi4_fat32_find_file(&r, "zealtest.txt");
    uint8_t contents[PI4_FAT32_FILE_MAX_BYTES];
    struct pi4_fat32_file_report fr;
    if (!test || pi4_fat32_find_file(&r, "MISSING.TXT")) return fail("case-insensitive named file lookup");
    memset(contents, 0xa5, sizeof(contents)); d.reads = 0;
    pi4_fat32_read_file(&r.volume, test, read_disk, &d, contents, sizeof(contents), &fr);
    if (fr.result != PI4_FAT32_FILE_READ_PASS || fr.bytes != 1025u || fr.clusters != 3u ||
        fr.reads != 6u || fr.last_read != FIRST + 2087u ||
        !pi4_fat32_test_match(contents, fr.bytes) || contents[1025] != 0xa5u ||
        pi4_fat32_test_match(contents, 1024u))
        return fail("fragmented three-cluster file and exact EOF padding boundary");
    contents[512] ^= 1u;
    if (pi4_fat32_test_match(contents, 1025u)) return fail("file pattern detects changed middle sector");
    d.reads = 0;
    pi4_fat32_read_file(&r.volume, test, read_disk, &d, contents, 1024u, &fr);
    if (fr.result != PI4_FAT32_FILE_TOO_LARGE || d.reads) return fail("file capacity checked before I/O");
    struct pi4_fat32_entry invalid = *test; invalid.bytes = PI4_FAT32_FILE_MAX_BYTES + 1u;
    pi4_fat32_read_file(&r.volume, &invalid, read_disk, &d, contents, sizeof(contents), &fr);
    if (fr.result != PI4_FAT32_FILE_TOO_LARGE || d.reads) return fail("file maximum checked before I/O");
    invalid = *test; invalid.attributes = 16u;
    pi4_fat32_read_file(&r.volume, &invalid, read_disk, &d, contents, sizeof(contents), &fr);
    if (fr.result != PI4_FAT32_FILE_INVALID || d.reads) return fail("directory cannot be read as test file");
    invalid = *test; invalid.bytes = 0; invalid.cluster = 0;
    pi4_fat32_read_file(&r.volume, &invalid, read_disk, &d, contents, sizeof(contents), &fr);
    if (fr.result != PI4_FAT32_FILE_READ_PASS || fr.bytes || d.reads) return fail("empty file makes no reads");
    file_fixture(&d); put32(d.fat0 + 12u, 0x0fffffffu);
    pi4_fat32_read_file(&r.volume, test, read_disk, &d, contents, sizeof(contents), &fr);
    if (fr.result != PI4_FAT32_FILE_CHAIN_BAD || fr.bytes != 512u)
        return fail("early EOC cannot silently truncate file");
    file_fixture(&d); put32(d.fat0 + 28u, 3u);
    pi4_fat32_read_file(&r.volume, test, read_disk, &d, contents, sizeof(contents), &fr);
    if (fr.result != PI4_FAT32_FILE_CHAIN_BAD || fr.bytes != 1024u || d.reads != 4u)
        return fail("cyclic file chain stops before repeated data");
    file_fixture(&d); put32(d.fat0 + 12u, 0u);
    pi4_fat32_read_file(&r.volume, test, read_disk, &d, contents, sizeof(contents), &fr);
    if (fr.result != PI4_FAT32_FILE_CHAIN_BAD || fr.bytes) return fail("free file cluster rejects");
    file_fixture(&d); d.fail_lba = FIRST + 2085u;
    pi4_fat32_read_file(&r.volume, test, read_disk, &d, contents, sizeof(contents), &fr);
    if (fr.result != PI4_FAT32_FILE_DATA_READ_BAD || fr.bytes != 512u || fr.last_read != d.fail_lba)
        return fail("file data failure preserves exact partial count and LBA");
    file_fixture(&d); d.fail_lba = FAT_LBA - 1u;
    pi4_fat32_read_file(&r.volume, test, read_disk, &d, contents, sizeof(contents), &fr);
    if (fr.result != PI4_FAT32_FILE_FAT_READ_BAD || fr.bytes) return fail("file FAT read failure is distinct");
    if (argc > 1) {
        FILE *f = fopen(argv[1], "rb"); uint8_t boot[512];
        if (!f || fseek(f, 0, SEEK_END)) return fail("open real FAT fixture");
        long size = ftell(f);
        if (size < 512 || !read_file(f, 0u, boot)) return fail("real FAT fixture length/read");
        pi4_fat32_probe(boot, (uint32_t)(size / 512 - 1), 512u, read_file, f, &r);
        if (r.result != PI4_FAT32_ROOT_PASS || !r.complete || !r.listed)
            return fail(pi4_fat32_result_name(r.result));
        int found = 0;
        for (unsigned i = 0; i < r.listed; i++) found |= !strcmp(r.entry[i].name, "HELLO.TXT");
        if (!found) return fail("real FAT32 fixture contains HELLO.TXT");
        const struct pi4_fat32_entry *real = pi4_fat32_find_file(&r, PI4_FAT32_TEST_NAME);
        if (!real) return fail("real FAT32 fixture contains ZEALTEST.TXT");
        pi4_fat32_read_file(&r.volume, real, read_file, f, contents, sizeof(contents), &fr);
        fclose(f);
        if (fr.result != PI4_FAT32_FILE_READ_PASS || !pi4_fat32_test_match(contents, fr.bytes))
            return fail(pi4_fat32_file_result_name(fr.result));
    }
    puts("PASS Pi4 read-only FAT32 directory/file reads, fragmented chains, bounds, EOF and test pattern");
    return 0;
}
