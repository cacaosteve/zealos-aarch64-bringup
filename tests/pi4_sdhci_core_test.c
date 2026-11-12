#include <stdio.h>
#include <stdint.h>

#include "../src/pi4_sdhci_core.h"

static int fail(const char *message) {
    fprintf(stderr, "FAIL Pi SDHCI core: %s\n", message);
    return 1;
}

static void set_response_bits(uint32_t words[4], unsigned most,
                              unsigned least, uint32_t value) {
    for (unsigned bit = least; bit <= most; bit++) {
        unsigned from_msb = 127u - bit;
        unsigned word = from_msb / 32u;
        unsigned position = 31u - (from_msb & 31u);
        uint32_t mask = 1u << position;
        if ((value >> (bit - least)) & 1u)
            words[word] |= mask;
        else
            words[word] &= ~mask;
    }
}

int main(void) {
    uint16_t divisor_bits = 0;
    uint32_t actual_hz = 0;
    uint16_t command = 0;
    uint32_t paired = 0;
    uint32_t response[4] = {0};
    uint64_t sectors = 0;

    if (!pi4_sdhci_clock_divisor(54000000u, 400000u, 2,
                                  &divisor_bits, &actual_hz) ||
        divisor_bits != 0x4400u || actual_hz != 397058u)
        return fail("SDHCI 3.0 54 MHz identification clock divisor");
    if (!pi4_sdhci_clock_divisor(54000000u, 400000u, 1,
                                  &divisor_bits, &actual_hz) ||
        divisor_bits != 0x8000u || actual_hz != 210937u)
        return fail("SDHCI 2.0 power-of-two identification clock divisor");
    if (!pi4_sdhci_clock_divisor(400000u, 400000u, 2,
                                  &divisor_bits, &actual_hz) ||
        divisor_bits != 0 || actual_hz != 400000u)
        return fail("divisor-one zero encoding");
    if (pi4_sdhci_clock_divisor(54000000u, 1000u, 2,
                                 &divisor_bits, &actual_hz))
        return fail("reject unrepresentable requested clock");
    if (pi4_sdhci_clock_divisor(0, 400000u, 2,
                                 &divisor_bits, &actual_hz) ||
        pi4_sdhci_clock_divisor(54000000u, 400000u, 2,
                                 NULL, &actual_hz))
        return fail("reject invalid clock parameters");

    if (!pi4_sdhci_command_word(0, PI4_SDHCI_RESPONSE_NONE, 0, 0, 0,
                                 &command) || command != 0)
        return fail("CMD0 no-response command word");
    if (!pi4_sdhci_command_word(17, PI4_SDHCI_RESPONSE_48, 1, 1, 1,
                                 &command) || command != 0x113au)
        return fail("CMD17 read command flags");
    paired = pi4_sdhci_transfer_command(
        PI4_SDHCI_TRANSFER_BLOCK_COUNT_ENABLE | PI4_SDHCI_TRANSFER_READ,
        command);
    if (paired != 0x113a0012u)
        return fail("single-block read uses paired transfer/command write");
    if (!pi4_sdhci_command_word(7, PI4_SDHCI_RESPONSE_48_BUSY, 1, 1, 0,
                                 &command) || command != 0x071bu)
        return fail("CMD7 busy response flags");
    if (pi4_sdhci_command_word(64, PI4_SDHCI_RESPONSE_48, 1, 1, 0,
                                &command) ||
        pi4_sdhci_command_word(8, (enum pi4_sdhci_response)4, 1, 1, 0,
                                &command))
        return fail("reject invalid command fields");

    if (!pi4_sdhci_block_size_count(512, 1, &paired) ||
        paired != 0x00010200u ||
        pi4_sdhci_block_size_count(0, 1, &paired) ||
        pi4_sdhci_block_size_count(512, 0, &paired) ||
        pi4_sdhci_block_size_count(0x1000u, 1, &paired))
        return fail("paired block-size/block-count register encoding");

    if (!pi4_sdhci_replace_byte(0xaabbccddu, 1, 0x12, &paired) ||
        paired != 0xaabb12ddu ||
        !pi4_sdhci_replace_halfword(0xaabbccddu, 2, 0x1234, &paired) ||
        paired != 0x1234ccddu ||
        pi4_sdhci_replace_halfword(0, 1, 0, &paired))
        return fail("masked paired-register read/modify/write helpers");

    pi4_sdhci_unpack_response136(0x11223344u, 0x55667788u,
                                0x99aabbccu, 0xddeeff00u, response);
    if (response[0] != 0xeeff0099u || response[1] != 0xaabbcc55u ||
        response[2] != 0x66778811u || response[3] != 0x22334400u)
        return fail("R2 response register order and CRC-bit alignment");

    for (unsigned i = 0; i < 4; i++) response[i] = 0;
    set_response_bits(response, 127, 126, 1);
    set_response_bits(response, 69, 48, 0x12345u);
    if (!pi4_sdhci_csd_sector_count(response, &sectors) ||
        sectors != (0x12346ull * 1024u))
        return fail("CSD v2 capacity decoding");

    for (unsigned i = 0; i < 4; i++) response[i] = 0;
    set_response_bits(response, 127, 126, 0);
    set_response_bits(response, 83, 80, 9);
    set_response_bits(response, 73, 62, 1023);
    set_response_bits(response, 49, 47, 3);
    if (!pi4_sdhci_csd_sector_count(response, &sectors) || sectors != 32768u)
        return fail("CSD v1 capacity decoding");
    set_response_bits(response, 127, 126, 2);
    if (pi4_sdhci_csd_sector_count(response, &sectors))
        return fail("reject reserved CSD structure");

    puts("PASS Pi SDHCI clock, command, response, capacity and paired-register helpers");
    return 0;
}
