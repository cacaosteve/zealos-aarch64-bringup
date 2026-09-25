/*
 * ZealOS aarch64 bring-up: Limine protocol hello.
 * Success = serial "hello from EL1" (+ optional FB stripe).
 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_bootloader_info_request bootloader_info_request = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_stack_size_request stack_size_request = {
    .id = LIMINE_STACK_SIZE_REQUEST_ID,
    .revision = 0,
    .stack_size = 65536,
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

#define PL011_UART0_PHYS 0x09000000ULL
#define PL011_DR         0x00
#define PL011_FR         0x18
#define PL011_FR_TXFF    (1u << 5)

static void halt(void) {
    for (;;) {
        __asm__ volatile("wfi");
    }
}

static void uart_write(volatile uint8_t *uart, char c) {
    while (uart[PL011_FR] & PL011_FR_TXFF) {
    }
    uart[PL011_DR] = (uint8_t)c;
}

static void uart_puts(volatile uint8_t *uart, const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_write(uart, '\r');
        }
        uart_write(uart, *s++);
    }
}

static void uart_put_u64_hex(volatile uint8_t *uart, uint64_t v) {
    static const char hex[] = "0123456789abcdef";
    uart_puts(uart, "0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_write(uart, hex[(v >> i) & 0xf]);
    }
}

static uint64_t current_el(void) {
    uint64_t el;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(el));
    return (el >> 2) & 3;
}

static void fill_fb_stripe(struct limine_framebuffer *fb) {
    if (!fb || fb->bpp != 32) {
        return;
    }
    uint32_t *pixels = fb->address;
    uint64_t pitch32 = fb->pitch / 4;
    uint32_t green = 0xFF00FF00u;
    uint32_t dark = 0xFF001100u;
    for (uint64_t y = 0; y < fb->height; y++) {
        for (uint64_t x = 0; x < fb->width; x++) {
            pixels[y * pitch32 + x] = (y < fb->height / 8) ? green : dark;
        }
    }
}

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        halt();
    }

    if (framebuffer_request.response &&
        framebuffer_request.response->framebuffer_count > 0) {
        fill_fb_stripe(framebuffer_request.response->framebuffers[0]);
    }

    uint64_t hhdm = hhdm_request.response ? hhdm_request.response->offset : 0;
    volatile uint8_t *uart = (volatile uint8_t *)(hhdm + PL011_UART0_PHYS);

    uart_puts(uart, "\n==============================\n");
    uart_puts(uart, "hello from EL");
    uart_write(uart, (char)('0' + (int)current_el()));
    uart_puts(uart, "\n");
    uart_puts(uart, "zealos-aarch64-bringup / Limine\n");

    if (bootloader_info_request.response) {
        uart_puts(uart, "bootloader: ");
        uart_puts(uart, bootloader_info_request.response->name);
        uart_puts(uart, " ");
        uart_puts(uart, bootloader_info_request.response->version);
        uart_puts(uart, "\n");
    }

    uart_puts(uart, "HHDM offset: ");
    uart_put_u64_hex(uart, hhdm);
    uart_puts(uart, "\n");

    if (memmap_request.response) {
        uart_puts(uart, "memmap entries: ");
        uart_put_u64_hex(uart, memmap_request.response->entry_count);
        uart_puts(uart, "\n");
    }

    if (framebuffer_request.response &&
        framebuffer_request.response->framebuffer_count > 0) {
        uart_puts(uart, "painted green stripe — bring-up OK\n");
    } else {
        uart_puts(uart, "no framebuffer (serial-only OK)\n");
    }

    uart_puts(uart, "halting (wfi loop)\n");
    uart_puts(uart, "==============================\n");
    halt();
}
