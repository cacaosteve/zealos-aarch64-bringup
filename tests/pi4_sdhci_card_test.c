#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/pi4_sdhci_card.h"

#define TEST_SECTORS 8u

enum fake_data_mode {
    FAKE_DATA_NONE,
    FAKE_DATA_READ,
    FAKE_DATA_WRITE,
};

struct fake_sdhci {
    uint32_t regs[64];
    uint8_t media[TEST_SECTORS * 512u];
    uint64_t counter;
    uint32_t int_status;
    uint32_t argument;
    uint64_t data_lba;
    unsigned data_words;
    enum fake_data_mode data_mode;
    unsigned read_commands;
    unsigned write_commands;
    unsigned status_commands;
    int write_protected;
};

static int fail(const char *message) {
    fprintf(stderr, "FAIL Pi SDHCI card: %s\n", message);
    return 1;
}

static uint64_t fake_counter(void *context) {
    struct fake_sdhci *fake = context;
    return ++fake->counter;
}

static int fake_read32(void *context, unsigned offset, uint32_t *value) {
    struct fake_sdhci *fake = context;
    if (!fake || !value || (offset & 3u) || offset >= sizeof(fake->regs))
        return 0;
    if (offset == PI4_SDHCI_REG_INT_STATUS) {
        *value = fake->int_status;
        return 1;
    }
    if (offset == PI4_SDHCI_CARD_REG_PRESENT_STATE) {
        *value = PI4_SDHCI_CARD_PRESENT_INSERTED |
                 (fake->write_protected ?
                  PI4_SDHCI_CARD_PRESENT_WRITE_PROTECT : 0u);
        return 1;
    }
    if (offset == PI4_SDHCI_REG_DATA &&
        fake->data_mode == FAKE_DATA_READ && fake->data_words < 128u &&
        fake->data_lba < TEST_SECTORS) {
        size_t base = (size_t)fake->data_lba * 512u +
                      (size_t)fake->data_words * 4u;
        *value = (uint32_t)fake->media[base] |
                 ((uint32_t)fake->media[base + 1] << 8) |
                 ((uint32_t)fake->media[base + 2] << 16) |
                 ((uint32_t)fake->media[base + 3] << 24);
        fake->data_words++;
        return 1;
    }
    *value = fake->regs[offset / 4u];
    return 1;
}

static int fake_write32(void *context, unsigned offset, uint32_t value) {
    struct fake_sdhci *fake = context;
    if (!fake || (offset & 3u) || offset >= sizeof(fake->regs)) return 0;
    if (offset == PI4_SDHCI_REG_INT_STATUS) {
        fake->int_status &= ~value; /* SDHCI interrupt status is W1C. */
        if ((value & PI4_SDHCI_CARD_INT_READ_READY) &&
            fake->data_mode == FAKE_DATA_READ && fake->data_words == 128u)
            fake->int_status |= PI4_SDHCI_CARD_INT_XFER_COMPLETE;
        if ((value & PI4_SDHCI_CARD_INT_WRITE_READY) &&
            fake->data_mode == FAKE_DATA_WRITE && fake->data_words == 128u)
            fake->int_status |= PI4_SDHCI_CARD_INT_XFER_COMPLETE;
        return 1;
    }
    if (offset == PI4_SDHCI_REG_DATA &&
        fake->data_mode == FAKE_DATA_WRITE && fake->data_words < 128u &&
        fake->data_lba < TEST_SECTORS) {
        size_t base = (size_t)fake->data_lba * 512u +
                      (size_t)fake->data_words * 4u;
        fake->media[base] = (uint8_t)value;
        fake->media[base + 1] = (uint8_t)(value >> 8);
        fake->media[base + 2] = (uint8_t)(value >> 16);
        fake->media[base + 3] = (uint8_t)(value >> 24);
        fake->data_words++;
        return 1;
    }
    fake->regs[offset / 4u] = value;
    if (offset == PI4_SDHCI_CARD_REG_ARGUMENT) {
        fake->argument = value;
    } else if (offset == PI4_SDHCI_REG_TRANSFER_COMMAND) {
        uint16_t command = (uint16_t)(value >> 16);
        unsigned index = (command >> 8) & 0x3fu;
        fake->int_status |= PI4_SDHCI_CARD_INT_CMD_COMPLETE;
        if (index == 17u || index == 24u) {
            fake->data_lba = fake->argument;
            fake->data_words = 0;
            if (index == 17u) {
                fake->data_mode = FAKE_DATA_READ;
                fake->int_status |= PI4_SDHCI_CARD_INT_READ_READY;
                fake->read_commands++;
            } else {
                fake->data_mode = FAKE_DATA_WRITE;
                fake->int_status |= PI4_SDHCI_CARD_INT_WRITE_READY;
                fake->write_commands++;
            }
        } else if (index == 13u) {
            fake->regs[PI4_SDHCI_REG_RESPONSE0 / 4u] =
                PI4_SDHCI_CARD_R1_READY_FOR_DATA |
                PI4_SDHCI_CARD_R1_STATE_TRAN;
            fake->status_commands++;
        } else {
            return 0;
        }
    }
    return 1;
}

int main(void) {
    struct fake_sdhci fake = {0};
    struct pi4_sdhci_card card = {0};
    struct pi4_sdhci_card_io io = {
        .context = &fake,
        .counter_hz = 1000,
        .counter = fake_counter,
        .read32 = fake_read32,
        .write32 = fake_write32,
    };
    uint8_t input[2 * 512u], output[2 * 512u];

    /* Keep initialization helpers linked in this host harness as well; this
     * invalid call must fail before touching the fake controller. */
    if (pi4_sdhci_card_init(NULL, NULL) != PI4_SDHCI_CARD_ERR_IO)
        return fail("reject invalid card initialization arguments");

    card.io = io;
    card.rca = 1;
    card.sectors = TEST_SECTORS;
    card.high_capacity = 1;
    card.ready = 1;
    for (size_t i = 0; i < sizeof(fake.media); i++)
        fake.media[i] = (uint8_t)(i * 17u + 3u);

    if (pi4_sdhci_card_read_many(&card, 2, 2, output) != 0 ||
        memcmp(output, fake.media + 2u * 512u, sizeof(output)) != 0 ||
        fake.read_commands != 2u)
        return fail("bounded two-sector PIO read");
    if (pi4_sdhci_card_read_many(&card, TEST_SECTORS - 1u, 2, output) !=
            PI4_SDHCI_CARD_ERR_RANGE ||
        pi4_sdhci_card_read_many(&card, 0, 0, output) !=
            PI4_SDHCI_CARD_ERR_RANGE)
        return fail("reject reads outside card capacity");

    for (size_t i = 0; i < sizeof(input); i++)
        input[i] = (uint8_t)(255u - i * 11u);
    fake.write_protected = 1;
    if (pi4_sdhci_card_write_many(&card, 3, 2, input) !=
            PI4_SDHCI_CARD_ERR_UNSUPPORTED || fake.write_commands != 0u)
        return fail("respect the SDHCI write-protect signal");
    fake.write_protected = 0;
    if (pi4_sdhci_card_write_many(&card, 3, 2, input) != 0 ||
        fake.write_commands != 2u ||
        memcmp(fake.media + 3u * 512u, input, sizeof(input)) != 0)
        return fail("bounded two-sector PIO write");
    if (pi4_sdhci_card_write_many(&card, TEST_SECTORS - 1u, 2, input) !=
            PI4_SDHCI_CARD_ERR_RANGE ||
        pi4_sdhci_card_write_many(&card, 0, 0, input) !=
            PI4_SDHCI_CARD_ERR_RANGE)
        return fail("reject writes outside card capacity");
    if (pi4_sdhci_card_flush(&card) != 0 || fake.status_commands != 1u)
        return fail("wait for SD transfer state and ready status");

    if (pi4_sdhci_card_read_many(&card, 3, 2, output) != 0 ||
        memcmp(output, input, sizeof(output)) != 0)
        return fail("read back written sectors");

    puts("PASS Pi SDHCI bounded single-block reads/writes, flush and read-back");
    return 0;
}
