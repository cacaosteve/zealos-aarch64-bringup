#ifndef ZEAL_PI_DIAG
#include "zc/runtime.h"
#endif
/*
 * Exception + GICv3 + CNTP timer bring-up for QEMU virt @ EL1.
 */
#include <stdint.h>
#include <stddef.h>
#include "handoff.h"
#include "a64_emit.h"
#include "hc_ir.h"
#include "hc_front.h"
#include "fb_font.h"
#include "mmio_map.h"
#include "plat_pi4.h"
#if defined(ZEAL_FORCE_PI4) && !defined(ZEAL_PI_DIAG)
#include "pi4_desktop_fb.h"
#endif
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
#include "pi4_pcie_cfg.h"
#include "pi4_dma_probe.h"
#include "pi4_xhci_probe.h"
#include "pi4_xhci_dma_probe.h"
#include "pi4_xhci_dma_alias.h"
#include "pi4_fat32.h"
#include "pi4_usb_keyboard.h"
#include "pi4_usb_mouse.h"
#include "pi4_usb_bus.h"
#endif
#include "virtio_kbd.h"
#include "virtio_tablet.h"
#include "virtio_blk.h"
#ifndef ZEAL_PI_DIAG
#include "zc_source_store.h"
#endif
#include "disk_layout.h"

int zc_virtio_keyboard_ready(void) { return virtio_kbd_ready(); }
uint64_t zc_virtio_keyboard_event_count(void) { return virtio_kbd_key_event_count(); }
uint16_t zc_virtio_keyboard_last_code(void) { return virtio_kbd_last_key_code(); }
uint32_t zc_virtio_keyboard_last_value(void) { return virtio_kbd_last_key_value(); }

#include "netofdots_zc.h"
#include "lines_zc.h"
#include "minigr_zc.h"
#include "memsort_zc.h"
#include "peekplot_zc.h"
#include "offbmp_zc.h"
#include "heapstr_zc.h"
#include "catfmt_zc.h"
#include "heapque_zc.h"
#include "jobque_zc.h"
#include "jobrun_zc.h"
#include "taskspawn_zc.h"
#include "popup_zc.h"
#include "doclite_zc.h"
#include "doclib_zc.h"
#include "notes_zc.h"
#include "globshare_zc.h"
#include "life_zc.h"
#include "cartlite_zc.h"
#include "vec2lite_zc.h"
#include "angleslite_zc.h"
#include "coslite_zc.h"
#include "sqrtlite_zc.h"
#include "arglite_zc.h"
#include "commalite_zc.h"
#include "plot3lite_zc.h"
#include "tospilite_zc.h"
#include "tosutf8lite_zc.h"
#include "colorlite_zc.h"
#include "turtlelite_zc.h"
#include "filllite_zc.h"
#include "initlite_zc.h"
#include "deflite_zc.h"
#include "printlite_zc.h"
#include "msglite_zc.h"
#include "menulite_zc.h"
#include "findlite_zc.h"
#include "fslite_zc.h"
#include "setuplite_zc.h"
#include "ttlite_zc.h"
#include "buflite_zc.h"
#include "inclite_zc.h"
#include "dclite_zc.h"
#include "linedclite_zc.h"
#include "grflite_zc.h"
#include "movelite_zc.h"
#include "checkedlite_zc.h"
#include "cmplite_zc.h"
#include "forinclite_zc.h"
#include "microlite_zc.h"
#include "movestacklite_zc.h"
#include "endlite_zc.h"
#include "drawitlite_zc.h"
#include "latticelite_zc.h"
#include "looplite_zc.h"
#include "demolite_zc.h"
#include "eventlite_zc.h"
#include "playlite_zc.h"
#include "inputlite_zc.h"
#include "rightlite_zc.h"
#include "cursorlite_zc.h"
#include "uplite_zc.h"
#include "ticklite_zc.h"
#include "framelite_zc.h"
#include "plotdclite_zc.h"
#include "abortlite_zc.h"
#include "aimmovelite_zc.h"
#include "idlelite_zc.h"
#include "layerlite_zc.h"
#include "endslite_zc.h"
#include "speedlite_zc.h"
#include "midlite_zc.h"
#include "livelite_zc.h"
#include "accellite_zc.h"
#include "restartlite_zc.h"
#include "widthlite_zc.h"
#include "bothcolorlite_zc.h"
#include "menufulllite_zc.h"
#include "menubiglite_zc.h"
#include "trylite_zc.h"
#include "stepcountlite_zc.h"
#include "anglesfulllite_zc.h"
#include "braceangleslite_zc.h"
#include "bracepilite_zc.h"
#include "setmenulite_zc.h"
#include "nearlatticelite_zc.h"
#include "f64iflite_zc.h"
#include "wraplatticelite_zc.h"
#include "menulooplite_zc.h"
#include "idxalllite_zc.h"
#include "disklat_zc.h"
#include "stocklat_zc.h"
#include "depthbuflite_zc.h"
#include "depthrstlite_zc.h"
#include "depthplotlite_zc.h"
#include "depthlinelite_zc.h"
#include "ramblk_zc.h"
#include "namefile_zc.h"
#include "dirlook_zc.h"
#include "dirdel_zc.h"
#include "fopen_zc.h"
#include "fwrite_zc.h"
#include "multiblk_zc.h"
#include "redsea_zc.h"
#include "rsroot_zc.h"
#include "rsfile_zc.h"
#include "rsalloc_zc.h"
#include "rsfree_zc.h"
#include "rsmulti_zc.h"
#include "rscfile_zc.h"
#include "rscwrite_zc.h"
#include "rscseek_zc.h"
#include "rsclib_zc.h"
#include "rspersist_zc.h"
#include "runzc_zc.h"

#define PL011_DR      0x00
#define PL011_RSR     0x04 /* write clears OE/BE/PE/FE (ECR alias) */
#define PL011_FR      0x18
#define PL011_CR      0x30
#define PL011_FR_TXFF (1u << 5)
#define ZEAL_TEXT_MAX_COLS 512u
#define ZEAL_TEXT_MAX_ROWS 512u
#define ZEAL_TEXT_PLANE_COLS 100u
#define ZEAL_TEXT_PLANE_ROWS 75u
#define PL011_FR_RXFE (1u << 4)
#define PL011_FR_BUSY (1u << 3)
#define PL011_UARTEN  (1u << 0)
#define PL011_TXE     (1u << 8)
#define PL011_RXE     (1u << 9)

/* QEMU virt GICv3 */
#define GICD_BASE_PHYS 0x08000000ULL
#define GICR_BASE_PHYS 0x080A0000ULL
#define GICR_STRIDE    0x20000ULL
#define GICD_CTLR      0x0000
#define GICR_WAKER     0x0014
#define GICR_TYPER     0x0008
#define GICR_IGROUPR0  0x10080
#define GICR_ISENABLER0 0x10100
#define GICR_ICENABLER0 0x10180
#define GICR_ICPENDR0  0x10280
#define GICR_IPRIORITYR 0x10400
#define GICR_ICFGR1    0x10C04

#define TIMER_PPI 30 /* non-secure EL1 physical timer */

/*
 * ICC_*_EL1 (op0=3, op1=0, CRn=12, CRm=12):
 *   op2=0 IAR1, 1 EOIR1, 2 HPPIR1, 3 BPR1,
 *   op2=4 CTLR, 5 SRE, 6 IGRPEN0, 7 IGRPEN1
 */
#define MRS_ICC(op2, out) \
    __asm__ volatile("mrs %0, S3_0_C12_C12_" #op2 : "=r"(out))
#define MSR_ICC(op2, in) \
    __asm__ volatile("msr S3_0_C12_C12_" #op2 ", %0" ::"r"(in))

extern char exception_vectors[];

static volatile uint8_t *g_uart;
static volatile uint32_t g_timer_irqs;
static volatile uint32_t g_svc_hits;
/* 1 = use GICv2 MMIO CPU interface (Pi); 0 = GICv3 system regs (virt). */
static volatile uint32_t g_gic_v2;
/* When set, non-SVC sync advances ELR by 4 and returns (probe CNTP under UTM/HVF). */
static volatile uint32_t g_sync_recover;
static volatile uint32_t g_sync_skipped;
static volatile uint32_t g_uart_probe;
static volatile uint32_t g_uart_dead;
static volatile uint64_t g_probe_esr;
static volatile uint64_t g_probe_far;
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
static uint32_t g_pi4_link_status;
static uint32_t g_pi4_cfg_state;
static uint32_t g_pi4_cfg_id;
static uint32_t g_pi4_cfg_class_rev;
static uint32_t g_pi4_cfg_header;
static uint32_t g_pi4_cfg_command;
static uint32_t g_pi4_cfg_bars[6];
static unsigned g_pi4_cfg_bar_count;
static int g_pi4_uart_tx_ok;
static uint32_t g_pi4_xhci_state;
static uint32_t g_pi4_xhci_caps[6];
static uint64_t g_pi4_xhci_bar_cpu;
static uint64_t g_pi4_xhci_bar_va;
static uint32_t g_pi4_xhci_state_mask;
static uint32_t g_pi4_xhci_usbcmd;
static uint32_t g_pi4_xhci_usbsts;
static uint32_t g_pi4_xhci_reset_state;
static uint32_t g_pi4_xhci_reset_phase;
static uint32_t g_pi4_xhci_reset_usbcmd;
static uint32_t g_pi4_xhci_reset_usbsts;
static uint32_t g_pi4_xhci_legacy_offset;
static uint32_t g_pi4_xhci_legacy_cap;
static uint32_t g_pi4_xhci_legacy_after;
static uint32_t g_pi4_xhci_portsc[PI4_XHCI_DIAG_PORTS];
static unsigned g_pi4_xhci_port_count;
static unsigned g_pi4_xhci_port_total;
static uint64_t g_pi4_xhci_state_esr;
static uint64_t g_pi4_xhci_state_far;
static unsigned g_pi4_dma_probe_state;
static uint64_t g_pi4_dma_probe_line;
static unsigned g_pi4_xhci_dma_state;
static unsigned g_pi4_xhci_dma_events;
static unsigned g_pi4_xhci_dma_event_cursor_index;
static unsigned g_pi4_xhci_dma_event_cursor_cycle;
static unsigned g_pi4_xhci_dma_command_completions;
static uint32_t g_pi4_xhci_dma_slot_id;
static uint32_t g_pi4_xhci_dma_slot_enabled;
static unsigned g_pi4_xhci_dma_stage;
static int32_t g_pi4_xhci_dma_layout_rc;
static uint32_t g_pi4_xhci_dma_page_size;
static uint32_t g_pi4_xhci_dma_gate_failures;
static uint32_t g_pi4_xhci_dma_usbcmd;
static uint32_t g_pi4_xhci_dma_usbsts;
static uint32_t g_pi4_xhci_dma_legacy;
static uint32_t g_pi4_xhci_dma_pci_command;
static uint32_t g_pi4_xhci_dma_status;
static uint32_t g_pi4_xhci_dma_event_status;
static uint32_t g_pi4_xhci_dma_event_control;
static uint32_t g_pi4_xhci_dma_runtime_cmd;
static uint32_t g_pi4_xhci_dma_runtime_sts;
static uint32_t g_pi4_xhci_dma_run_sts;
static uint32_t g_pi4_xhci_dma_doorbell_sts;
static uint32_t g_pi4_xhci_dma_wrap_state;
static uint32_t g_pi4_xhci_dma_mfindex_start;
static uint32_t g_pi4_xhci_dma_mfindex_end;
static uint32_t g_pi4_xhci_dma_wrap_status;
static uint32_t g_pi4_xhci_dma_wrap_control;
static uint32_t g_pi4_xhci_dma_port_event_state;
static uint32_t g_pi4_xhci_dma_port_event_port;
static uint32_t g_pi4_xhci_dma_port_event_before;
static uint32_t g_pi4_xhci_dma_port_event_after;
static uint32_t g_pi4_xhci_dma_port_ready;
static uint32_t g_pi4_xhci_dma_root_portsc[PI4_XHCI_DIAG_PORTS];
static uint32_t g_pi4_xhci_dma_root_initial[PI4_XHCI_DIAG_PORTS];
static uint32_t g_pi4_xhci_dma_root_protocol[PI4_XHCI_DIAG_PORTS];
static unsigned g_pi4_xhci_dma_root_port_count;
static unsigned g_pi4_xhci_dma_root_scans, g_pi4_xhci_dma_root_wait_ms;
struct pi4_xhci_direct_root_report {
    unsigned step;
    uint32_t attempted;
    uint32_t rc[4], status[4], checks[4];
    uint32_t configuration, max_lun;
    uint64_t event_parameter, status_bus;
    uint32_t event_control;
    struct pi4_xhci_dma_config_summary config;
};
static struct pi4_xhci_direct_root_report g_pi4_xhci_direct;
struct pi4_xhci_bot_io_report {
    uint32_t rc[3], status[3], checks[3], actual[3], control[3];
    uint64_t parameter[3];
    unsigned phase;
};
struct pi4_xhci_bot_report {
    unsigned step, verified, csw_ok;
    uint32_t output_slot[4], output_in[5], output_out[5];
    uint32_t command_rc[2], command_status[2], command_control[2];
    struct pi4_xhci_bot_io_report io;
    uint8_t inquiry[PI4_XHCI_DMA_BOT_INQUIRY_BYTES];
    uint8_t csw[PI4_XHCI_DMA_BOT_CSW_BYTES];
    char vendor[9], product[17], revision[5];
};
static struct pi4_xhci_bot_report g_pi4_xhci_bot;
struct pi4_xhci_storage_command_report {
    unsigned opcode, bytes, lba, tag, transport_ok, scsi_status;
    unsigned sense_key, sense_asc, sense_ascq;
    struct pi4_xhci_bot_io_report io;
    uint8_t csw[PI4_XHCI_DMA_BOT_CSW_BYTES];
};
struct pi4_xhci_storage_report {
    unsigned step, count, repeat_ok, reads;
    uint32_t last_lba, block_bytes, crc[2], signature, first_word;
    uint64_t capacity_bytes;
    struct pi4_xhci_storage_command_report command[PI4_XHCI_DMA_BOT_STORAGE_COMMANDS];
    uint8_t first_block[PI4_XHCI_DMA_BOT_MAX_BLOCK_BYTES];
};
static struct pi4_xhci_storage_report g_pi4_xhci_storage;
static struct pi4_fat32_report g_pi4_fat32;
static struct {
    unsigned attempted, found, pattern_ok[2], repeat_ok;
    uint32_t bytes, cluster, crc[2];
    struct pi4_fat32_file_report read[2];
    uint8_t first[PI4_FAT32_FILE_MAX_BYTES], second[PI4_FAT32_FILE_MAX_BYTES];
} g_pi4_fat32_file;
static uint32_t g_pi4_xhci_dma_enable_slot_status;
static uint32_t g_pi4_xhci_dma_enable_slot_control;
static uint32_t g_pi4_xhci_dma_enable_trb_control;
static uint32_t g_pi4_xhci_dma_protocol_cap_offset;
static uint32_t g_pi4_xhci_dma_protocol_major;
static uint32_t g_pi4_xhci_dma_protocol_slot_type;
static uint32_t g_pi4_xhci_dma_address_status;
static uint32_t g_pi4_xhci_dma_address_control;
static uint32_t g_pi4_xhci_dma_device_address;
static uint32_t g_pi4_xhci_dma_device_slot_state;
static uint64_t g_pi4_xhci_dma_address_input_bus;
static uint64_t g_pi4_xhci_dma_address_output_bus;
static uint64_t g_pi4_xhci_dma_address_ep0_bus;
static uint64_t g_pi4_xhci_dma_address_dcbaa_slot;
static uint32_t g_pi4_xhci_dma_address_context_words[7];
static uint32_t g_pi4_xhci_dma_address_trb_control;
static uint32_t g_pi4_xhci_dma_transfer_status;
static uint32_t g_pi4_xhci_dma_transfer_control;
static uint64_t g_pi4_xhci_dma_transfer_parameter;
static uint32_t g_pi4_xhci_dma_transfer_checks;
static uint32_t g_pi4_xhci_dma_transfer_skipped;
static uint32_t g_pi4_xhci_dma_transfer_skipped_status;
static uint32_t g_pi4_xhci_dma_transfer_skipped_control;
static uint64_t g_pi4_xhci_dma_transfer_skipped_parameter;
static uint32_t g_pi4_xhci_dma_device_bcd_usb;
static uint32_t g_pi4_xhci_dma_device_class;
static uint32_t g_pi4_xhci_dma_device_subclass;
static uint32_t g_pi4_xhci_dma_device_protocol;
static uint32_t g_pi4_xhci_dma_device_ep0_mps;
static uint32_t g_pi4_xhci_dma_device_vendor;
static uint32_t g_pi4_xhci_dma_device_product;
static uint32_t g_pi4_xhci_dma_device_bcd_device;
static uint32_t g_pi4_xhci_dma_device_configurations;
static uint64_t g_pi4_xhci_dma_config_buffer_bus;
static uint32_t g_pi4_xhci_dma_config_total_length;
static uint32_t g_pi4_xhci_dma_config_interfaces;
static uint32_t g_pi4_xhci_dma_config_value;
static uint32_t g_pi4_xhci_dma_config_attributes;
static uint32_t g_pi4_xhci_dma_config_max_power;
static uint32_t g_pi4_xhci_dma_config_header_words[3];
static uint32_t g_pi4_xhci_dma_set_config_setup_control;
static uint32_t g_pi4_xhci_dma_set_config_status_control;
static uint64_t g_pi4_xhci_dma_set_config_event_parameter;
static uint32_t g_pi4_xhci_dma_set_config_event_status;
static uint32_t g_pi4_xhci_dma_set_config_event_control;
static uint32_t g_pi4_xhci_dma_set_config_event_checks;
static uint32_t g_pi4_xhci_dma_set_config_event_skipped;
static uint64_t g_pi4_xhci_dma_hub_buffer_bus;
static uint32_t g_pi4_xhci_dma_hub_descriptor_transfer_rc;
static uint32_t g_pi4_xhci_dma_hub_descriptor_setup[2];
static uint64_t g_pi4_xhci_dma_hub_descriptor_data_bus;
static uint32_t g_pi4_xhci_dma_hub_descriptor_data_length;
static uint32_t g_pi4_xhci_dma_hub_descriptor_data_control;
static uint32_t g_pi4_xhci_dma_hub_descriptor_status_control;
static uint32_t g_pi4_xhci_dma_hub_descriptor_validated;
static uint32_t g_pi4_xhci_dma_hub_descriptor_words[4];
static uint32_t g_pi4_xhci_dma_hub_ports;
static uint32_t g_pi4_xhci_dma_hub_characteristics;
static uint32_t g_pi4_xhci_dma_hub_power_delay;
static uint32_t g_pi4_xhci_dma_hub_current;
static uint32_t g_pi4_xhci_dma_hub_removable;
static uint32_t g_pi4_xhci_dma_hub_power_mask;
static uint32_t g_pi4_xhci_dma_hub_port_power_events;
static uint32_t g_pi4_xhci_dma_hub_port_status_events;
static uint32_t g_pi4_xhci_dma_hub_port_unpowered_mask;
static uint32_t g_pi4_xhci_dma_hub_port_connected_mask;
static uint32_t g_pi4_xhci_dma_hub_port_status[4];
static uint32_t g_pi4_xhci_dma_hub_port_change[4];
static uint32_t g_pi4_xhci_dma_hub_child_reset_state;
static uint32_t g_pi4_xhci_dma_hub_child_reset_port;
static uint32_t g_pi4_xhci_dma_hub_child_reset_request_rc;
static uint32_t g_pi4_xhci_dma_hub_child_reset_samples;
static uint32_t g_pi4_xhci_dma_hub_child_reset_status;
static uint32_t g_pi4_xhci_dma_hub_child_reset_change;
static uint32_t g_pi4_xhci_dma_hub_child_clear_rc;
static uint32_t g_pi4_xhci_dma_hub_child_final_status;
static uint32_t g_pi4_xhci_dma_hub_child_final_change;
static uint32_t g_pi4_xhci_dma_hub_config_status;
static uint32_t g_pi4_xhci_dma_hub_config_control;
static uint32_t g_pi4_xhci_dma_hub_output_slot_words[3];
static uint32_t g_pi4_xhci_dma_hub_guard_status;
static uint32_t g_pi4_xhci_dma_hub_guard_control;
static uint32_t g_pi4_xhci_dma_hub_config_input_words[8];
static uint32_t g_pi4_xhci_dma_hub_config_trb_words[4];
static uint32_t g_pi4_xhci_dma_hub_config_crcr[2];
static uint32_t g_pi4_xhci_dma_hub_config_usbsts;
static uint32_t g_pi4_xhci_dma_hub_config_db;
static uint32_t g_pi4_xhci_dma_child_slot_id;
static uint32_t g_pi4_xhci_dma_child_speed_id;
static uint32_t g_pi4_xhci_dma_child_route_string;
static uint32_t g_pi4_xhci_dma_child_enable_status;
static uint32_t g_pi4_xhci_dma_child_enable_control;
static uint32_t g_pi4_xhci_dma_child_address_status;
static uint32_t g_pi4_xhci_dma_child_address_control;
static uint32_t g_pi4_xhci_dma_child_address;
static uint32_t g_pi4_xhci_dma_child_slot_state;
static uint32_t g_pi4_xhci_dma_child_address_context_words[6];
static uint64_t g_pi4_xhci_dma_child_output_bus;
static uint64_t g_pi4_xhci_dma_child_ep0_bus;
static uint32_t g_pi4_xhci_dma_child_descriptor_transfer_rc;
static uint32_t g_pi4_xhci_dma_child_descriptor_setup_control;
static uint32_t g_pi4_xhci_dma_child_descriptor_data_control;
static uint32_t g_pi4_xhci_dma_child_descriptor_status_control;
static uint32_t g_pi4_xhci_dma_child_descriptor_validated;
static uint32_t g_pi4_xhci_dma_child_descriptor_words[2];
static uint64_t g_pi4_xhci_dma_child_descriptor_buffer_bus;
static uint64_t g_pi4_xhci_dma_child_descriptor_status_bus;
static uint32_t g_pi4_xhci_dma_child_repeat_descriptor_transfer_rc;
static uint64_t g_pi4_xhci_dma_child_repeat_descriptor_event_parameter;
static uint32_t g_pi4_xhci_dma_child_repeat_descriptor_event_status;
static uint32_t g_pi4_xhci_dma_child_repeat_descriptor_event_control;
static uint32_t g_pi4_xhci_dma_child_repeat_descriptor_event_checks;
static uint32_t g_pi4_xhci_dma_child_repeat_descriptor_setup_control;
static uint32_t g_pi4_xhci_dma_child_repeat_descriptor_data_control;
static uint32_t g_pi4_xhci_dma_child_repeat_descriptor_status_control;
static uint32_t g_pi4_xhci_dma_child_repeat_descriptor_validated;
static uint32_t g_pi4_xhci_dma_child_repeat_descriptor_observed;
static uint32_t g_pi4_xhci_dma_child_repeat_descriptor_words[2];
static uint64_t g_pi4_xhci_dma_child_repeat_descriptor_buffer_bus;
static uint64_t g_pi4_xhci_dma_child_repeat_descriptor_status_bus;
static uint32_t g_pi4_xhci_dma_child_ep0_evaluate_status;
static uint32_t g_pi4_xhci_dma_child_ep0_mps_command;
static uint32_t g_pi4_xhci_dma_child_ep0_evaluate_control;
static uint32_t g_pi4_xhci_dma_child_ep0_evaluate_mps;
static uint32_t g_pi4_xhci_dma_child_ep0_evaluate_output_mps;
static uint32_t g_pi4_xhci_dma_child_pre_evaluate_get_configuration_transfer_rc;
static uint64_t g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_parameter;
static uint32_t g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_status;
static uint32_t g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_control;
static uint32_t g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_checks;
static uint32_t g_pi4_xhci_dma_child_pre_evaluate_get_configuration_setup_control;
static uint32_t g_pi4_xhci_dma_child_pre_evaluate_get_configuration_data_control;
static uint32_t g_pi4_xhci_dma_child_pre_evaluate_get_configuration_status_control;
static uint32_t g_pi4_xhci_dma_child_pre_evaluate_get_configuration_observed;
static uint32_t g_pi4_xhci_dma_child_pre_evaluate_get_configuration_word;
static uint64_t g_pi4_xhci_dma_child_pre_evaluate_get_configuration_buffer_bus;
static uint64_t g_pi4_xhci_dma_child_pre_evaluate_get_configuration_status_bus;
static uint32_t g_pi4_xhci_dma_child_get_status_transfer_rc;
static uint64_t g_pi4_xhci_dma_child_get_status_event_parameter;
static uint32_t g_pi4_xhci_dma_child_get_status_event_status;
static uint32_t g_pi4_xhci_dma_child_get_status_event_control;
static uint32_t g_pi4_xhci_dma_child_get_status_event_checks;
static uint32_t g_pi4_xhci_dma_child_get_status_setup_control;
static uint32_t g_pi4_xhci_dma_child_get_status_data_control;
static uint32_t g_pi4_xhci_dma_child_get_status_status_control;
static uint32_t g_pi4_xhci_dma_child_get_status_observed;
static uint32_t g_pi4_xhci_dma_child_get_status_word;
static uint64_t g_pi4_xhci_dma_child_get_status_buffer_bus;
static uint64_t g_pi4_xhci_dma_child_get_status_status_bus;
static uint32_t g_pi4_xhci_dma_child_full_descriptor_transfer_rc;
static uint32_t g_pi4_xhci_dma_child_full_descriptor_setup_control;
static uint32_t g_pi4_xhci_dma_child_full_descriptor_data_control;
static uint32_t g_pi4_xhci_dma_child_full_descriptor_status_control;
static uint32_t g_pi4_xhci_dma_child_full_descriptor_validated;
static uint32_t g_pi4_xhci_dma_child_full_descriptor_observed;
static uint32_t g_pi4_xhci_dma_child_full_descriptor_words[5];
static uint64_t g_pi4_xhci_dma_child_full_descriptor_buffer_bus;
static uint64_t g_pi4_xhci_dma_child_full_descriptor_status_bus;
static uint32_t g_pi4_xhci_dma_child_config_header_transfer_rc;
static uint64_t g_pi4_xhci_dma_child_config_header_event_parameter;
static uint32_t g_pi4_xhci_dma_child_config_header_event_status;
static uint32_t g_pi4_xhci_dma_child_config_header_event_control;
static uint32_t g_pi4_xhci_dma_child_config_header_event_checks;
static uint32_t g_pi4_xhci_dma_child_config_header_setup_control;
static uint32_t g_pi4_xhci_dma_child_config_header_data_control;
static uint32_t g_pi4_xhci_dma_child_config_header_status_control;
static uint32_t g_pi4_xhci_dma_child_config_header_validated;
static uint32_t g_pi4_xhci_dma_child_config_header_observed;
static uint32_t g_pi4_xhci_dma_child_config_header_words[3];
static uint64_t g_pi4_xhci_dma_child_config_header_buffer_bus;
static uint64_t g_pi4_xhci_dma_child_config_header_status_bus;
static uint32_t g_pi4_xhci_dma_child_config_full_transfer_rc;
static uint64_t g_pi4_xhci_dma_child_config_full_event_parameter;
static uint32_t g_pi4_xhci_dma_child_config_full_event_status;
static uint32_t g_pi4_xhci_dma_child_config_full_event_control;
static uint32_t g_pi4_xhci_dma_child_config_full_event_checks;
static uint32_t g_pi4_xhci_dma_child_config_full_setup_control;
static uint32_t g_pi4_xhci_dma_child_config_full_data_control;
static uint32_t g_pi4_xhci_dma_child_config_full_status_control;
static uint32_t g_pi4_xhci_dma_child_config_full_validated;
static uint32_t g_pi4_xhci_dma_child_config_full_observed;
static uint32_t g_pi4_xhci_dma_child_config_full_words[3];
static uint64_t g_pi4_xhci_dma_child_config_full_buffer_bus;
static uint64_t g_pi4_xhci_dma_child_config_full_status_bus;
static struct pi4_xhci_dma_config_summary g_pi4_xhci_dma_child_config_summary;
static int32_t g_pi4_xhci_dma_child_bulk_guard_rc;
static uint32_t g_pi4_xhci_dma_child_bulk_guard_status;
static uint32_t g_pi4_xhci_dma_child_bulk_guard_control;
static int32_t g_pi4_xhci_dma_child_bulk_config_rc;
static uint32_t g_pi4_xhci_dma_child_bulk_config_status;
static uint32_t g_pi4_xhci_dma_child_bulk_config_control;
static uint32_t g_pi4_xhci_dma_child_bulk_contexts_verified;
static uint64_t g_pi4_xhci_dma_child_bulk_in_ring_bus;
static uint64_t g_pi4_xhci_dma_child_bulk_out_ring_bus;
static uint32_t g_pi4_xhci_dma_child_bulk_slot_words[6];
static uint32_t g_pi4_xhci_dma_child_bulk_in_words[5];
static uint32_t g_pi4_xhci_dma_child_bulk_out_words[5];
static uint32_t g_pi4_xhci_dma_child_set_config_transfer_rc;
static uint64_t g_pi4_xhci_dma_child_set_config_event_parameter;
static uint32_t g_pi4_xhci_dma_child_set_config_event_status;
static uint32_t g_pi4_xhci_dma_child_set_config_event_control;
static uint32_t g_pi4_xhci_dma_child_set_config_event_checks;
static uint32_t g_pi4_xhci_dma_child_set_config_setup_control;
static uint32_t g_pi4_xhci_dma_child_set_config_status_control;
static uint64_t g_pi4_xhci_dma_child_set_config_status_bus;
static uint32_t g_pi4_xhci_dma_child_get_config_transfer_rc;
static uint64_t g_pi4_xhci_dma_child_get_config_event_parameter;
static uint32_t g_pi4_xhci_dma_child_get_config_event_status;
static uint32_t g_pi4_xhci_dma_child_get_config_event_control;
static uint32_t g_pi4_xhci_dma_child_get_config_event_checks;
static uint32_t g_pi4_xhci_dma_child_get_config_setup_control;
static uint32_t g_pi4_xhci_dma_child_get_config_data_control;
static uint32_t g_pi4_xhci_dma_child_get_config_status_control;
static uint32_t g_pi4_xhci_dma_child_get_config_observed;
static uint32_t g_pi4_xhci_dma_child_get_config_value;
static uint64_t g_pi4_xhci_dma_child_get_config_buffer_bus;
static uint64_t g_pi4_xhci_dma_child_get_config_status_bus;
static int32_t g_pi4_xhci_dma_child_get_max_lun_transfer_rc;
static uint32_t g_pi4_xhci_dma_child_get_max_lun_prepare;
static uint64_t g_pi4_xhci_dma_child_get_max_lun_event_parameter;
static uint32_t g_pi4_xhci_dma_child_get_max_lun_event_status;
static uint32_t g_pi4_xhci_dma_child_get_max_lun_event_control;
static uint32_t g_pi4_xhci_dma_child_get_max_lun_event_checks;
static uint32_t g_pi4_xhci_dma_child_get_max_lun_setup_control;
static uint32_t g_pi4_xhci_dma_child_get_max_lun_data_control;
static uint32_t g_pi4_xhci_dma_child_get_max_lun_status_control;
static uint32_t g_pi4_xhci_dma_child_get_max_lun_observed;
static uint32_t g_pi4_xhci_dma_child_get_max_lun_value;
static uint64_t g_pi4_xhci_dma_child_get_max_lun_buffer_bus;
static uint64_t g_pi4_xhci_dma_child_get_max_lun_status_bus;
static uint32_t g_pi4_xhci_dma_hub_probe_phase;
static uint32_t g_pi4_xhci_dma_hub_probe_port;
static uint32_t g_pi4_xhci_dma_hub_request_type;
static uint32_t g_pi4_xhci_dma_hub_request;
static uint32_t g_pi4_xhci_dma_hub_request_value;
static uint32_t g_pi4_xhci_dma_hub_request_index;
static uint32_t g_pi4_xhci_dma_hub_request_length;
static uint64_t g_pi4_xhci_dma_hub_expected_status_bus;
static uint32_t g_pi4_xhci_dma_hub_status_trb_index;
static uint64_t g_pi4_xhci_dma_hub_status_buffer_bus;
static uint64_t g_pi4_xhci_dma_hub_setup_trb_bus;
static uint64_t g_pi4_xhci_dma_hub_data_trb_bus;
static uint32_t g_pi4_xhci_dma_hub_status_buffer_words[2];
static uint32_t g_pi4_xhci_dma_output_ep0_context[5];
static uint32_t g_pi4_xhci_dma_output_ep0_context_valid;
static uint32_t g_pi4_xhci_dma_output_ep0_state;
static uint32_t g_pi4_xhci_dma_output_ep0_dcs;
static uint64_t g_pi4_xhci_dma_output_ep0_dequeue_bus;
static uint32_t g_pi4_xhci_dma_stop_endpoint_rc;
static uint32_t g_pi4_xhci_dma_stop_endpoint_trb_control;
static uint32_t g_pi4_xhci_dma_stop_endpoint_status;
static uint32_t g_pi4_xhci_dma_stop_endpoint_control;
static uint32_t g_pi4_xhci_dma_stop_endpoint_transfer_events;
static uint64_t g_pi4_xhci_dma_stop_endpoint_transfer_parameter;
static uint32_t g_pi4_xhci_dma_stop_endpoint_transfer_status;
static uint32_t g_pi4_xhci_dma_stop_endpoint_transfer_control;
static uint32_t g_pi4_xhci_dma_stop_endpoint_other_events;
static uint32_t g_pi4_xhci_dma_stop_output_ep0_context_valid;
static uint32_t g_pi4_xhci_dma_stop_output_ep0_context[5];
static uint32_t g_pi4_xhci_dma_stop_output_ep0_state;
static uint32_t g_pi4_xhci_dma_stop_output_ep0_dcs;
static uint64_t g_pi4_xhci_dma_stop_output_ep0_dequeue_bus;
static uint32_t g_pi4_xhci_dma_setup_trb_control;
static uint32_t g_pi4_xhci_dma_data_trb_control;
static uint32_t g_pi4_xhci_dma_status_trb_control;
static uint64_t g_pi4_xhci_dma_control_buffer_bus;
static uint32_t g_pi4_xhci_dma_descriptor_prefix[2];
static uint64_t g_pi4_xhci_dma_port_event_parameter;
static uint32_t g_pi4_xhci_dma_port_event_status;
static uint32_t g_pi4_xhci_dma_port_event_control;
static uint32_t g_pi4_xhci_dma_setup_iman;
static uint32_t g_pi4_xhci_dma_setup_imod;
static uint32_t g_pi4_xhci_dma_crcr_lo;
static uint32_t g_pi4_xhci_dma_crcr_hi;
static uint32_t g_pi4_xhci_dma_db_flush;
static uint32_t g_pi4_xhci_dma_timeout_sts;
static uint32_t g_pi4_xhci_dma_timeout_crcr;
static uint32_t g_pi4_xhci_dma_timeout_iman;
static uint32_t g_pi4_xhci_dma_erdp_lo;
static uint32_t g_pi4_xhci_dma_erdp_hi;
static uint32_t g_pi4_xhci_dma_event_trb[4];
static uint32_t g_pi4_xhci_dma_setup_mask;
static uint32_t g_pi4_xhci_dma_setup_mismatch;
static uint32_t g_pi4_xhci_dma_bar2_state;
static uint32_t g_pi4_xhci_dma_bar2_lo;
static uint32_t g_pi4_xhci_dma_bar2_hi_original;
static uint32_t g_pi4_xhci_dma_bar2_hi_test;
static uint32_t g_pi4_xhci_dma_bar2_hi_restored;
static uint64_t g_pi4_xhci_dma_bus_bias;
static uint32_t g_pi4_xhci_dma_setup_config;
static uint32_t g_pi4_xhci_dma_setup_erstsz;
static uint64_t g_pi4_xhci_dma_setup_dcbaa;
static uint64_t g_pi4_xhci_dma_setup_erstba;
static uint64_t g_pi4_xhci_dma_setup_erdp;
static uint32_t g_pi4_xhci_dma_abort_state;
static uint32_t g_pi4_xhci_dma_abort_status;
static uint32_t g_pi4_xhci_dma_abort_control;
static uint64_t g_pi4_xhci_dma_abort_parameter;
struct pi4_pcie_readback {
    uint32_t mask;
    uint32_t win0_lo;
    uint32_t win0_hi;
    uint32_t win0_base_limit;
    uint32_t win0_base_hi;
    uint32_t win0_limit_hi;
    uint32_t inbound_bar1_lo;
    uint32_t inbound_bar1_hi;
    uint32_t inbound_bar2_lo;
    uint32_t inbound_bar2_hi;
    uint32_t inbound_bar3_lo;
    uint32_t inbound_bar3_hi;
    uint32_t misc_ctrl;
    uint32_t err_valid_before;
    uint32_t err_valid_after;
    uint32_t err_info;
    uint32_t err_addr;
    uint32_t mem_err_addr_lo;
    uint32_t mem_err_addr_hi;
    uint32_t mem_err_cause;
};
static struct pi4_pcie_readback g_pi4_pcie_readback;
#define PI4_PCIE_WIN0_LO              0x400cu
#define PI4_PCIE_WIN0_HI              0x4010u
#define PI4_PCIE_WIN0_BASE_LIMIT      0x4070u
#define PI4_PCIE_WIN0_BASE_HI         0x4080u
#define PI4_PCIE_WIN0_LIMIT_HI        0x4084u
#define PI4_PCIE_INBOUND_BAR1_LO      0x402cu
#define PI4_PCIE_INBOUND_BAR1_HI      0x4030u
#define PI4_PCIE_INBOUND_BAR2_LO      0x4034u
#define PI4_PCIE_INBOUND_BAR2_HI      0x4038u
#define PI4_PCIE_INBOUND_BAR3_LO      0x403cu
#define PI4_PCIE_INBOUND_BAR3_HI      0x4040u
#define PI4_PCIE_MISC_CTRL            0x4008u
#define PI4_PCIE_OUTB_ERR_VALID       0x6004u
#define PI4_PCIE_OUTB_ERR_INFO        0x600cu
#define PI4_PCIE_OUTB_ERR_ADDR        0x6010u
#define PI4_PCIE_OUTB_MEM_ERR_ADDR_LO 0x6018u
#define PI4_PCIE_OUTB_MEM_ERR_ADDR_HI 0x601cu
#define PI4_PCIE_OUTB_MEM_ERR_CAUSE   0x6020u
#define PI4_CFG_NOT_RUN 0u
#define PI4_CFG_READ_OK 1u
#define PI4_CFG_MMIO_FAIL 2u
#define PI4_CFG_MAP_FAIL 3u
#define PI4_CFG_NOT_LINKED 4u
#define PI4_CFG_NO_DEVICE 5u
#define PI4_CFG_FDT_SKIP 6u
#define PI4_CFG_STATUS_FAIL 7u
#define PI4_XHCI_NOT_RUN 0u
#define PI4_XHCI_CAPS_OK 1u
#define PI4_XHCI_BAD_BAR 2u
#define PI4_XHCI_MAP_FAIL 3u
#define PI4_XHCI_CMD_FAIL 4u
#define PI4_XHCI_CFG_FAIL 5u
#define PI4_XHCI_MMIO_FAIL 6u
#define PI4_XHCI_RESTORE_FAIL 7u
#define PI4_XHCI_CAPS_INVALID 8u
#endif
static uint64_t g_hhdm;
static uint64_t g_timer_ticks;
static const uint8_t *g_install_image;
static uint64_t g_install_image_size;
__attribute__((aligned(64))) static uint8_t g_install_io[65536];

uint64_t g_hc_mem[HC_IR_MEM_WORDS];
uint64_t g_hc_fp;
uint64_t g_hc_alloc;
uint64_t g_hc_glob_words;

/* Guard so a bad slot write cannot clobber the framebuffer pointer. */
static uint64_t g_fb_guard[2];

/* Framebuffer text cursor */
static volatile uint8_t *g_fb;
static uint64_t g_fb_w, g_fb_h, g_fb_pitch;
#if defined(ZEAL_FORCE_PI4) && !defined(ZEAL_PI_DIAG)
static _Alignas(64) uint32_t g_pi4_desktop_pixels[PI4_DESKTOP_WIDTH*PI4_DESKTOP_HEIGHT];
static struct { volatile uint32_t *address; struct pi4_desktop_fb plan; unsigned active; uint64_t frames; } g_pi4_scanout;
static void pi4_desktop_scanout_present(void);
static void pi4_desktop_scanout_report(void);
#endif
static uint32_t g_fb_bpp;
/* Optional indexed offscreen target used while the ZealOS task bridge builds
 * one frame. Framebuffer primitives keep their direct path outside that scope. */
static uint8_t *g_fb_render_surface;
static uint32_t *g_fb_render_rgba;
static int g_fb_render_rgba_overlay;
static uint64_t g_fb_render_stride, g_fb_render_rows;
static const uint16_t *g_fb_win_z_buf;
static const uint16_t *g_fb_win_pixel_z_buf;
static uint32_t g_fb_cx, g_fb_cy;
static uint32_t g_fb_cols, g_fb_rows;
static volatile uint32_t g_fb_console_quiet;

static int fb_console_owned_by_desktop(void) {
    if (g_fb_console_quiet)
        return 1;
#ifndef ZEAL_PI_DIAG
    return zc_source_winmgr_active() || zc_gui_terminal_active() ||
           zc_window_demo_active();
#else
    return 0;
#endif
}
static uint64_t g_zc_text_cells_drawn;
/* Bounded ZealOS-style text plane. The framebuffer remains the visible target,
 * while these cells are the source for redraws and blank-cell attributes. */
static uint32_t g_fb_text_cells[ZEAL_TEXT_MAX_COLS * ZEAL_TEXT_MAX_ROWS];
static uint8_t g_fb_text_valid[ZEAL_TEXT_MAX_COLS * ZEAL_TEXT_MAX_ROWS];
/* Retained character output for the small cooperative guest-task bridge.
 * Cells are task-local so a z-order redraw can restore them after clearing
 * the shared text plane. */
#define FB_TASK_TEXT_SLOTS 8
#define FB_SHELL_TEXT_SLOT 7
#define FB_TASK_TEXT_CELLS (ZEAL_TEXT_MAX_COLS * ZEAL_TEXT_MAX_ROWS)
static uint32_t g_fb_task_text[FB_TASK_TEXT_SLOTS][FB_TASK_TEXT_CELLS];
static uint8_t g_fb_task_text_valid[FB_TASK_TEXT_SLOTS][FB_TASK_TEXT_CELLS];
static uint16_t g_fb_task_text_x[FB_TASK_TEXT_SLOTS];
static uint16_t g_fb_task_text_y[FB_TASK_TEXT_SLOTS];

static const uint32_t g_text_palette[16] = {
    0x00000000u, 0x000000aau, 0x0000aa00u, 0x0000aaaau,
    0x00aa0000u, 0x00aa00aau, 0x00aa5500u, 0x00aaaaaau,
    0x00555555u, 0x005555ffu, 0x0055ff55u, 0x0055ffffu,
    0x00ff5555u, 0x00ff55ffu, 0x00ffff55u, 0x00ffffffu,
};
static uint32_t g_graphics_palette[16];
static int g_graphics_palette_ready;
static void fb_text_buffer_reset(void);
void zeal_fb_task_text_reset(unsigned slot);

static uint8_t fb_nearest_palette_index(uint32_t bgr) {
    uint32_t r = (bgr >> 16) & 0xffu;
    uint32_t g = (bgr >> 8) & 0xffu;
    uint32_t b = bgr & 0xffu;
    uint64_t best_distance = UINT64_MAX;
    uint8_t best = 0;
    for (uint8_t i = 0; i < 16; i++) {
        uint32_t p = g_graphics_palette_ready ? g_graphics_palette[i]
                                              : g_text_palette[i];
        int32_t dr = (int32_t)r - (int32_t)((p >> 16) & 0xffu);
        int32_t dg = (int32_t)g - (int32_t)((p >> 8) & 0xffu);
        int32_t db = (int32_t)b - (int32_t)(p & 0xffu);
        uint64_t distance = (uint64_t)(dr * dr) + (uint64_t)(dg * dg) +
                            (uint64_t)(db * db);
        if (distance < best_distance) {
            best_distance = distance;
            best = i;
        }
    }
    return best;
}

/* BCM2711 PL011 wants 32-bit MMIO; QEMU virt accepts it too. */
static void uart_write(volatile uint8_t *uart, char c) {
    unsigned spin = 0;
    volatile uint32_t *r;
    if (!uart || g_uart_dead) {
        return;
    }
    r = (volatile uint32_t *)(uintptr_t)uart;
    while ((r[PL011_FR / 4] & PL011_FR_TXFF) && spin++ < 100000u) {
        if (g_uart_dead) {
            return;
        }
    }
    if (g_uart_dead) {
        return;
    }
    r[PL011_DR / 4] = (uint32_t)(uint8_t)c;
}

/* Enable TX+RX without touching baud (keep UEFI/firmware divisor). */
static void pl011_enable_txrx(volatile uint8_t *uart) {
    volatile uint32_t *r;
    uint32_t cr;
    if (!uart) {
        return;
    }
    r = (volatile uint32_t *)(uintptr_t)uart;
    cr = r[PL011_CR / 4];
    cr |= PL011_UARTEN | PL011_TXE | PL011_RXE;
    r[PL011_CR / 4] = cr;
}

/* M200: ensure RXE, drain stale FIFO, clear RSR — Pi USB-TTL and virt serial. */
static void pl011_rx_prep(void) {
    volatile uint32_t *r;
    unsigned n = 0;
    if (!g_uart || g_uart_dead) {
        return;
    }
    pl011_enable_txrx(g_uart);
    r = (volatile uint32_t *)(uintptr_t)g_uart;
    while (!(r[PL011_FR / 4] & PL011_FR_RXFE) && n++ < 64u) {
        (void)r[PL011_DR / 4];
    }
    r[PL011_RSR / 4] = 0;
}

#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
static void uart_flush(volatile uint8_t *uart) {
    unsigned spin = 0;
    volatile uint32_t *r;
    if (!uart) {
        return;
    }
    r = (volatile uint32_t *)(uintptr_t)uart;
    while ((r[PL011_FR / 4] & PL011_FR_BUSY) && spin++ < 100000u) {
    }
}
#endif

static void uart_puts(volatile uint8_t *uart, const char *s) {
    if (!uart || !s) {
        return;
    }
    while (*s) {
        if (*s == '\n') {
            uart_write(uart, '\r');
        }
        uart_write(uart, *s++);
    }
}

static void uart_put_u64_hex(volatile uint8_t *uart, uint64_t v) {
    static const char hex[] = "0123456789abcdef";
    if (!uart) {
        return;
    }
    uart_puts(uart, "0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_write(uart, hex[(v >> i) & 0xf]);
    }
}

static void uart_put_u32_dec(volatile uint8_t *uart, uint32_t v) {
    char buf[11];
    int n = 0;
    if (!uart) {
        return;
    }
    if (v == 0) {
        uart_write(uart, '0');
        return;
    }
    while (v && n < 10) {
        buf[n++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (n--) {
        uart_write(uart, buf[n]);
    }
}

#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
static void pi4_usb_keyboard_poll(void);
static int pi4_usb_keyboard_ready(void);
static int pi4_usb_mouse_ready(void);
static int pi4_usb_bus_keyboard_ready(void);
static int pi4_usb_bus_pointer_ready(void);
static int pi4_usb_bus_sample(uint32_t, uint32_t, uint32_t *, uint32_t *, int *, int *);
static void pi4_usb_bus_poll(void);
static void pi4_usb_bus_release(void);
static void pi4_usb_bus_report(int);
static void pi4_xhci_route_text(int, const char *);
static void pi4_xhci_route_hex(int, uint32_t);
static int pi4_usb_mouse_sample(uint32_t w, uint32_t h, uint32_t *x, uint32_t *y,
                                int *buttons, int *wheel);
#endif
#include "usb_input.h"

static void fb_putpixel(uint32_t x, uint32_t y, uint32_t bgr) {
    if (!g_fb || x >= g_fb_w || y >= g_fb_h) {
        return;
    }
    if (g_fb_render_surface) {
        if (x < g_fb_render_stride && y < g_fb_render_rows) {
            g_fb_render_surface[(uint64_t)y * g_fb_render_stride + x] =
                fb_nearest_palette_index(bgr);
            /* A full-screen source compositor presents the RGBA staging
             * plane, while graph primitives normally target the indexed task
             * plane. The editor overlay must update whichever plane scans out. */
            if (g_fb_render_rgba_overlay && g_fb_render_rgba)
                g_fb_render_rgba[(uint64_t)y * g_fb_render_stride + x] =
                    bgr & 0x00ffffffu;
        }
        return;
    }
    uint32_t pxb = (g_fb_bpp + 7) / 8;
    volatile uint8_t *p = g_fb + (uint64_t)y * g_fb_pitch + (uint64_t)x * pxb;
    if (pxb >= 3) {
        p[0] = (uint8_t)(bgr & 0xff);
        p[1] = (uint8_t)((bgr >> 8) & 0xff);
        p[2] = (uint8_t)((bgr >> 16) & 0xff);
        if (pxb >= 4) {
            p[3] = 0;
        }
    }
}

/* ZealOS GrPeek: pixel color, or -1 if off-screen. */
static int64_t fb_peek(int32_t x, int32_t y) {
    if (!g_fb || x < 0 || y < 0 || (uint32_t)x >= g_fb_w || (uint32_t)y >= g_fb_h) {
        return -1;
    }
    uint32_t pxb = (g_fb_bpp + 7) / 8;
    volatile uint8_t *p = g_fb + (uint64_t)y * g_fb_pitch + (uint64_t)x * pxb;
    if (pxb < 3) {
        return -1;
    }
    return (int64_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16));
}

/* Fast scanline fill for 32bpp; falls back to putpixel otherwise. */
static void fb_fillrect(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h, uint32_t bgr) {
    if (!g_fb || w == 0 || h == 0) {
        return;
    }
    if (x0 >= g_fb_w || y0 >= g_fb_h) {
        return;
    }
    if (x0 + w > g_fb_w) {
        w = (uint32_t)(g_fb_w - x0);
    }
    if (y0 + h > g_fb_h) {
        h = (uint32_t)(g_fb_h - y0);
    }
    if (g_fb_render_surface) {
        uint8_t color = fb_nearest_palette_index(bgr);
        for (uint32_t y = 0; y < h; y++) {
            uint64_t row = (uint64_t)(y0 + y) * g_fb_render_stride + x0;
            for (uint32_t x = 0; x < w; x++)
                g_fb_render_surface[row + x] = color;
        }
        return;
    }
    uint32_t pxb = (g_fb_bpp + 7) / 8;
    if (pxb == 4) {
        uint32_t pix = bgr & 0x00ffffffu;
        for (uint32_t y = 0; y < h; y++) {
            volatile uint32_t *row =
                (volatile uint32_t *)(g_fb + (uint64_t)(y0 + y) * g_fb_pitch + (uint64_t)x0 * 4);
            for (uint32_t x = 0; x < w; x++) {
                row[x] = pix;
            }
        }
        return;
    }
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            fb_putpixel(x0 + x, y0 + y, bgr);
        }
    }
}

static void fb_clear(uint32_t bgr) {
    if (!g_fb) {
        return;
    }
    fb_text_buffer_reset();
    zeal_fb_task_text_reset(FB_SHELL_TEXT_SLOT);
    fb_fillrect(0, 0, (uint32_t)g_fb_w, (uint32_t)g_fb_h, bgr);
    g_fb_cx = 0;
    g_fb_cy = 0;
}

static void fb_draw_char(uint32_t col, uint32_t row, char ch, uint32_t fg) {
    if (!g_fb) {
        return;
    }
    unsigned char u = (unsigned char)ch;
    const uint8_t *glyph = fb_glyph8(u);
    uint32_t x0 = col * 8;
    uint32_t y0 = row * 8;
    for (uint32_t r = 0; r < 8; r++) {
        uint8_t bits = glyph[r];
        for (uint32_t c = 0; c < 8; c++) {
            if (bits & (0x80u >> c)) {
                fb_putpixel(x0 + c, y0 + r, fg);
            }
        }
    }
}

/* Present packed ZealOS text cells from the bounded text-cell plane. */
static void fb_text_present_cell(uint32_t col, uint32_t row, uint32_t packed) {
    uint32_t colors = (packed >> 8) & 0xffu;
    uint32_t fg, bg;
    unsigned char ch = (unsigned char)packed;
    if (!g_fb || col >= g_fb_cols || row >= g_fb_rows)
        return;
    if (packed & (0x20000000u | 0x40000000u))
        colors ^= 0xffu;
    fg = g_text_palette[colors & 15u];
    bg = g_text_palette[(colors >> 4) & 15u];
    fb_fillrect(col * 8u, row * 8u, 8u, 8u, bg);
    if (ch >= 32)
        fb_draw_char(col, row, (char)ch, fg);
    if (packed & 0x80000000u)
        fb_fillrect(col * 8u, row * 8u + 7u, 8u, 1u, fg);
}

static void fb_text_buffer_reset(void) {
    size_t count = (size_t)ZEAL_TEXT_MAX_COLS * ZEAL_TEXT_MAX_ROWS;
    for (size_t i = 0; i < count; i++) {
        g_fb_text_cells[i] = 0;
        g_fb_text_valid[i] = 0;
    }
}

uint64_t zeal_fb_text_span(int64_t x, int64_t y, int64_t len, uint32_t attr,
                           const void *data, int mode, int64_t left,
                           int64_t right, int64_t top, int64_t bottom,
                           uint32_t *text_base, int64_t stride) {
    uint64_t drawn = 0;
    if (!g_fb || (mode != 0 && mode != 1) || len <= 0 ||
        y < top || y > bottom || y < 0 ||
        (uint64_t)y >= g_fb_rows)
        return 0;
    if (len > 4096)
        len = 4096;
    if (x > INT64_MAX - len || x > right || x + len <= left)
        return 0;
    for (int64_t i = 0; i < len; i++) {
        int64_t col = x + i;
        if (col < left || col > right || col < 0 || (uint64_t)col >= g_fb_cols)
            continue;
        uint32_t packed = (uint32_t)attr;
        unsigned char ch = ' ';
        if (mode == 0 && data)
            ch = ((const unsigned char *)data)[i];
        else if (mode == 1 && data) {
            packed = ((const uint32_t *)data)[i];
            ch = (unsigned char)packed;
        }
        packed = (packed & 0xffffff00u) | ch;
        if ((uint64_t)col < ZEAL_TEXT_MAX_COLS &&
            (uint64_t)y < ZEAL_TEXT_MAX_ROWS) {
            size_t index = (size_t)y * ZEAL_TEXT_MAX_COLS + (size_t)col;
            g_fb_text_cells[index] = packed;
            g_fb_text_valid[index] = 1;
        }
        if (text_base && stride > col && (uint64_t)col < ZEAL_TEXT_PLANE_COLS &&
            (uint64_t)y < ZEAL_TEXT_PLANE_ROWS)
            text_base[(size_t)y * (size_t)stride + (size_t)col] = packed;
        fb_text_present_cell((uint32_t)col, (uint32_t)y, packed);
        drawn++;
    }
    g_zc_text_cells_drawn += drawn;
    return drawn;
}

uint64_t zeal_fb_text_cells_drawn(void) {
    return g_zc_text_cells_drawn;
}

uint64_t zeal_fb_text_cols(void) { return g_fb_cols; }
uint64_t zeal_fb_text_rows(void) { return g_fb_rows; }
uint64_t zeal_fb_screen_width(void) { return g_fb_w; }
uint64_t zeal_fb_screen_height(void) { return g_fb_h; }
uint64_t zeal_fb_pitch_bytes(void) { return g_fb_pitch; }
uint64_t zeal_fb_bits_per_pixel(void) { return g_fb_bpp; }
uint64_t zeal_fb_address(void) { return (uint64_t)(uintptr_t)g_fb; }

uint64_t zeal_fb_nonblack_pixels(void) {
    uint64_t count = 0;
    if (!g_fb || g_fb_bpp != 32 || g_fb_pitch < g_fb_w * sizeof(uint32_t))
        return 0;
    for (uint64_t y = 0; y < g_fb_h; y++) {
        const volatile uint32_t *row =
            (const volatile uint32_t *)(g_fb + y * g_fb_pitch);
        for (uint64_t x = 0; x < g_fb_w; x++)
            count += (row[x] & 0x00ffffffu) != 0;
    }
    return count;
}

uint64_t zeal_fb_task_frame_begin(uint8_t *surface, uint32_t *rgba,
                                  uint64_t stride, uint64_t rows) {
    if (!g_fb || !surface || stride < g_fb_w || rows < g_fb_h ||
        stride > 16384 || rows > 16384 || stride > UINT64_MAX / rows ||
        !rgba || stride > UINT64_MAX / sizeof(*rgba))
        return 0;
    g_fb_render_surface = surface;
    g_fb_render_rgba = rgba;
    g_fb_render_rgba_overlay = 0;
    g_fb_render_stride = stride;
    g_fb_render_rows = rows;
    /* ZealOS uses black as the base layer; text and task windows are composed
     * over it in the same order as the bridge's task-ring traversal. */
    for (uint64_t y = 0; y < g_fb_h; y++) {
        uint8_t *row = surface + y * stride;
        for (uint64_t x = 0; x < g_fb_w; x++)
            row[x] = 0;
        if (g_fb_bpp == 32 && g_fb_pitch >= g_fb_w * sizeof(uint32_t)) {
            uint32_t *rgba_row = rgba + y * stride;
            const volatile uint32_t *fb_row = (const volatile uint32_t *)
                (g_fb + y * g_fb_pitch);
            for (uint64_t x = 0; x < g_fb_w; x++)
                rgba_row[x] = fb_row[x];
        }
    }
    return 1;
}

void zeal_fb_task_frame_cancel(void) {
    g_fb_render_surface = NULL;
    g_fb_render_rgba = NULL;
    g_fb_render_rgba_overlay = 0;
    g_fb_render_stride = g_fb_render_rows = 0;
}

void zeal_fb_task_frame_overlay_begin(void) {
    g_fb_render_rgba_overlay = g_fb_render_surface && g_fb_render_rgba;
}

void zeal_fb_task_frame_overlay_end(void) {
    g_fb_render_rgba_overlay = 0;
}

uint64_t zeal_fb_task_frame_end(uint64_t use_rgba) {
    uint8_t *surface = g_fb_render_surface;
    uint32_t *rgba = g_fb_render_rgba;
    uint64_t stride = g_fb_render_stride, rows = g_fb_render_rows;
    if (!g_fb || !surface || !rgba ||
        (use_rgba && (g_fb_bpp != 32 ||
                      g_fb_pitch < g_fb_w * sizeof(uint32_t)))) {
        zeal_fb_task_frame_cancel();
        return 0;
    }
    zeal_fb_task_frame_cancel();
    for (uint64_t y = 0; y < g_fb_h && y < rows; y++) {
        if (use_rgba) {
            const uint32_t *src = rgba + y * stride;
            volatile uint32_t *dst = (volatile uint32_t *)
                (g_fb + y * g_fb_pitch);
            for (uint64_t x = 0; x < g_fb_w; x++) {
                uint32_t pixel = src[x];
                if (dst[x] != pixel)
                    dst[x] = pixel;
            }
        } else {
            const uint8_t *src = surface + y * stride;
            for (uint64_t x = 0; x < g_fb_w; x++) {
                uint32_t pixel = g_graphics_palette_ready
                    ? g_graphics_palette[src[x] & 0x0fu]
                    : g_text_palette[src[x] & 0x0fu];
                if (fb_peek((int32_t)x, (int32_t)y) !=
                    (int64_t)(pixel & 0x00ffffffu))
                    fb_putpixel((uint32_t)x, (uint32_t)y, pixel);
            }
        }
    }
#if defined(ZEAL_FORCE_PI4) && !defined(ZEAL_PI_DIAG)
    if (g_pi4_scanout.active) g_pi4_scanout.frames++;
    pi4_desktop_scanout_present();
#endif
    return g_fb_w * g_fb_h;
}

int64_t zeal_fb_text_pixel(int64_t x, int64_t y) {
    if (x < INT32_MIN || x > INT32_MAX || y < INT32_MIN || y > INT32_MAX)
        return -1;
    return fb_peek((int32_t)x, (int32_t)y);
}

int64_t zeal_fb_text_char(int64_t x, int64_t y, uint32_t cell,
                          int allow_border, uint32_t left_right,
                          uint32_t top_bottom, uint32_t *text_base,
                          int64_t stride) {
    int64_t left = left_right & 0xffffu;
    int64_t right = left_right >> 16;
    int64_t top = top_bottom & 0xffffu;
    int64_t bottom = top_bottom >> 16;
    if (allow_border) {
        left--;
        right++;
        top--;
        bottom++;
    }
    return zeal_fb_text_span(x, y, 1, cell, &cell, 1,
                             left, right, top, bottom, text_base, stride) == 1;
}

int64_t zeal_fb_text_fill(int64_t x, int64_t y, int64_t len, uint32_t attr,
                          uint32_t left_right, uint32_t top_bottom,
                          uint32_t *text_base, int64_t stride) {
    int64_t left = left_right & 0xffffu;
    int64_t right = left_right >> 16;
    int64_t top = top_bottom & 0xffffu;
    int64_t bottom = top_bottom >> 16;
    uint64_t drawn = 0;
    if (!g_fb || len <= 0 || y < top || y > bottom || y < 0 ||
        (uint64_t)y >= g_fb_rows)
        return -1;
    if (len > 4096)
        len = 4096;
    if (x > INT64_MAX - len || x > right || x + len <= left)
        return -1;
    for (int64_t i = 0; i < len; i++) {
        int64_t col = x + i;
        uint32_t packed = attr & 0xffffff00u;
        size_t index;
        if (col < left || col > right || col < 0 ||
            (uint64_t)col >= g_fb_cols ||
            (uint64_t)col >= ZEAL_TEXT_MAX_COLS ||
            (uint64_t)y >= ZEAL_TEXT_MAX_ROWS)
            continue;
        index = (size_t)y * ZEAL_TEXT_MAX_COLS + (size_t)col;
        if (g_fb_text_valid[index] &&
            (g_fb_text_cells[index] & 0xffu) != 0)
            continue;
        g_fb_text_cells[index] = packed;
        g_fb_text_valid[index] = 1;
        if (text_base && stride > col && (uint64_t)col < ZEAL_TEXT_PLANE_COLS &&
            (uint64_t)y < ZEAL_TEXT_PLANE_ROWS)
            text_base[(size_t)y * (size_t)stride + (size_t)col] = packed;
        fb_text_present_cell((uint32_t)col, (uint32_t)y, packed);
        drawn++;
    }
    g_zc_text_cells_drawn += drawn;
    return (int64_t)drawn;
}

uint64_t zeal_fb_text_rect(int64_t left, int64_t right, int64_t top,
                           int64_t bottom, uint32_t cell,
                           uint32_t *text_base, int64_t stride) {
    uint64_t drawn = 0;
    if (!g_fb || !g_fb_cols || !g_fb_rows || left > right || top > bottom)
        return 0;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right >= g_fb_cols) right = g_fb_cols - 1;
    if (bottom >= g_fb_rows) bottom = g_fb_rows - 1;
    if (text_base) {
        if (right >= ZEAL_TEXT_PLANE_COLS) right = ZEAL_TEXT_PLANE_COLS - 1;
        if (bottom >= ZEAL_TEXT_PLANE_ROWS) bottom = ZEAL_TEXT_PLANE_ROWS - 1;
    }
    if (left > right || top > bottom)
        return 0;
    for (int64_t row = top; row <= bottom; row++) {
        for (int64_t col = left; col <= right; col++) {
            uint32_t packed = cell;
            if ((uint64_t)col < ZEAL_TEXT_MAX_COLS &&
                (uint64_t)row < ZEAL_TEXT_MAX_ROWS) {
                size_t index = (size_t)row * ZEAL_TEXT_MAX_COLS + (size_t)col;
                g_fb_text_cells[index] = packed;
                g_fb_text_valid[index] = 1;
            }
            if (text_base && stride > col && (uint64_t)col < ZEAL_TEXT_PLANE_COLS &&
                (uint64_t)row < ZEAL_TEXT_PLANE_ROWS)
                text_base[(size_t)row * (size_t)stride + (size_t)col] = packed;
            fb_text_present_cell((uint32_t)col, (uint32_t)row, packed);
            drawn++;
        }
    }
    g_zc_text_cells_drawn += drawn;
    return drawn;
}

uint64_t zeal_fb_text_flush(uint32_t *text_base, int64_t stride, int64_t rows) {
    uint64_t drawn = 0;
    uint32_t visible_rows = g_fb_rows < ZEAL_TEXT_MAX_ROWS ? g_fb_rows : ZEAL_TEXT_MAX_ROWS;
    uint32_t cols = g_fb_cols < ZEAL_TEXT_MAX_COLS ? g_fb_cols : ZEAL_TEXT_MAX_COLS;
    if (!g_fb || !text_base || stride <= 0 || stride > ZEAL_TEXT_PLANE_COLS ||
        rows <= 0 || rows > ZEAL_TEXT_PLANE_ROWS)
        return 0;
    if ((int64_t)visible_rows > rows)
        visible_rows = (uint32_t)rows;
    if (cols > (uint64_t)stride)
        cols = (uint32_t)stride;
    if (visible_rows > ZEAL_TEXT_PLANE_ROWS)
        visible_rows = ZEAL_TEXT_PLANE_ROWS;
    for (uint32_t row = 0; row < visible_rows; row++) {
        for (uint32_t col = 0; col < cols; col++) {
            size_t index = (size_t)row * ZEAL_TEXT_MAX_COLS + col;
            g_fb_text_cells[index] = text_base[(size_t)row * (size_t)stride + col];
            g_fb_text_valid[index] = 1;
            fb_text_present_cell(col, row, g_fb_text_cells[index]);
            drawn++;
        }
    }
    return drawn;
}

uint64_t zeal_fb_text_flush_rect(uint32_t *text_base, int64_t stride,
                                 int64_t left, int64_t right,
                                 int64_t top, int64_t bottom) {
    uint64_t drawn = 0;
    if (!g_fb || !text_base || stride <= 0 || stride > ZEAL_TEXT_PLANE_COLS ||
        left > right || top > bottom)
        return 0;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right >= (int64_t)g_fb_cols) right = (int64_t)g_fb_cols - 1;
    if (bottom >= (int64_t)g_fb_rows) bottom = (int64_t)g_fb_rows - 1;
    if (right >= stride) right = stride - 1;
    if (right >= ZEAL_TEXT_PLANE_COLS) right = ZEAL_TEXT_PLANE_COLS - 1;
    if (bottom >= ZEAL_TEXT_PLANE_ROWS) bottom = ZEAL_TEXT_PLANE_ROWS - 1;
    if (left > right || top > bottom)
        return 0;
    for (int64_t row = top; row <= bottom; row++) {
        for (int64_t col = left; col <= right; col++) {
            size_t index = (size_t)row * ZEAL_TEXT_MAX_COLS + (size_t)col;
            uint32_t cell = text_base[(size_t)row * (size_t)stride + (size_t)col];
            g_fb_text_cells[index] = cell;
            g_fb_text_valid[index] = 1;
            fb_text_present_cell((uint32_t)col, (uint32_t)row, cell);
            drawn++;
        }
    }
    return drawn;
}

static int fb_task_text_bounds(int64_t left, int64_t right, int64_t top,
                               int64_t bottom) {
    return g_fb && left >= 0 && top >= 0 && left <= right && top <= bottom &&
           right < (int64_t)g_fb_cols && bottom < (int64_t)g_fb_rows &&
           right < ZEAL_TEXT_MAX_COLS && bottom < ZEAL_TEXT_MAX_ROWS;
}

static uint64_t fb_task_text_present(unsigned slot, int64_t left, int64_t top,
                                     int64_t width, int64_t height) {
    uint64_t drawn = 0;
    for (int64_t y = 0; y < height; y++) {
        for (int64_t x = 0; x < width; x++) {
            size_t local = (size_t)y * ZEAL_TEXT_MAX_COLS + (size_t)x;
            if (!g_fb_task_text_valid[slot][local])
                continue;
            int64_t col = left + x, row = top + y;
            size_t index = (size_t)row * ZEAL_TEXT_MAX_COLS + (size_t)col;
            uint32_t cell = g_fb_task_text[slot][local];
            g_fb_text_cells[index] = cell;
            g_fb_text_valid[index] = 1;
            fb_text_present_cell((uint32_t)col, (uint32_t)row, cell);
            g_zc_text_cells_drawn++;
            drawn++;
        }
    }
    return drawn;
}

/* The shell owns the full framebuffer. Scrolling it by re-rasterizing every
 * retained glyph is needlessly expensive for warning-heavy source loads, so
 * move the already-rendered pixels up one text row and paint only the new
 * bottom row. Smaller task text regions keep the general redraw path. */
static int fb_shell_text_scroll_pixels(int64_t width, int64_t height) {
    uint32_t pxb;
    uint64_t bytes, pixel_rows;
    if (!g_fb || width != (int64_t)g_fb_cols || height != (int64_t)g_fb_rows ||
        (g_fb_bpp != 24 && g_fb_bpp != 32))
        return 0;
    pxb = (g_fb_bpp + 7) / 8;
    bytes = (uint64_t)width * 8 * pxb;
    pixel_rows = (uint64_t)height * 8;
    if (pixel_rows <= 8 || bytes > g_fb_pitch)
        return 0;
    for (uint64_t y = 0; y < pixel_rows - 8; y++) {
        volatile uint8_t *dst = g_fb + y * g_fb_pitch;
        volatile uint8_t *src = g_fb + (y + 8) * g_fb_pitch;
        for (uint64_t x = 0; x < bytes; x++)
            dst[x] = src[x];
    }
    fb_fillrect(0, (uint32_t)(pixel_rows - 8), (uint32_t)(width * 8), 8,
                0x00101820u);
    return 1;
}

static uint64_t fb_task_text_present_row(unsigned slot, int64_t left,
                                         int64_t top, int64_t width,
                                         int64_t local_row) {
    uint64_t drawn = 0;
    for (int64_t x = 0; x < width; x++) {
        size_t local = (size_t)local_row * ZEAL_TEXT_MAX_COLS + (size_t)x;
        if (!g_fb_task_text_valid[slot][local])
            continue;
        int64_t col = left + x, row = top + local_row;
        size_t index = (size_t)row * ZEAL_TEXT_MAX_COLS + (size_t)col;
        uint32_t cell = g_fb_task_text[slot][local];
        g_fb_text_cells[index] = cell;
        g_fb_text_valid[index] = 1;
        fb_text_present_cell((uint32_t)col, (uint32_t)row, cell);
        g_zc_text_cells_drawn++;
        drawn++;
    }
    return drawn;
}

static void fb_task_text_scroll(unsigned slot, int64_t left, int64_t top,
                                int64_t width, int64_t height) {
    for (int64_t y = 1; y < height; y++) {
        size_t dst = (size_t)(y - 1) * ZEAL_TEXT_MAX_COLS;
        size_t src = (size_t)y * ZEAL_TEXT_MAX_COLS;
        for (int64_t x = 0; x < width; x++) {
            g_fb_task_text[slot][dst + x] = g_fb_task_text[slot][src + x];
            g_fb_task_text_valid[slot][dst + x] =
                g_fb_task_text_valid[slot][src + x];
        }
    }
    size_t last = (size_t)(height - 1) * ZEAL_TEXT_MAX_COLS;
    for (int64_t x = 0; x < width; x++) {
        g_fb_task_text[slot][last + x] = 0;
        g_fb_task_text_valid[slot][last + x] = 0;
    }
    if (slot == FB_SHELL_TEXT_SLOT && left == 0 && top == 0 &&
        fb_shell_text_scroll_pixels(width, height)) {
        for (int64_t y = 1; y < height; y++) {
            size_t dst = (size_t)(y - 1) * ZEAL_TEXT_MAX_COLS;
            size_t src = (size_t)y * ZEAL_TEXT_MAX_COLS;
            for (int64_t x = 0; x < width; x++) {
                g_fb_text_cells[dst + x] = g_fb_text_cells[src + x];
                g_fb_text_valid[dst + x] = g_fb_text_valid[src + x];
            }
        }
        size_t global_last = (size_t)(height - 1) * ZEAL_TEXT_MAX_COLS;
        for (int64_t x = 0; x < width; x++) {
            g_fb_text_cells[global_last + x] = 0;
            g_fb_text_valid[global_last + x] = 0;
        }
        (void)fb_task_text_present_row(slot, left, top, width, height - 1);
    } else {
        if (slot == FB_SHELL_TEXT_SLOT && left == 0 && top == 0)
            fb_fillrect(0, (uint32_t)((height - 1) * 8),
                        (uint32_t)(width * 8), 8, 0x00101820u);
        (void)fb_task_text_present(slot, left, top, width, height);
    }
}

void zeal_fb_task_text_reset(unsigned slot) {
    if (slot >= FB_TASK_TEXT_SLOTS)
        return;
    for (size_t i = 0; i < FB_TASK_TEXT_CELLS; i++) {
        g_fb_task_text[slot][i] = 0;
        g_fb_task_text_valid[slot][i] = 0;
    }
    g_fb_task_text_x[slot] = g_fb_task_text_y[slot] = 0;
}

uint64_t zeal_fb_task_text_write(unsigned slot, int64_t left, int64_t right,
                                 int64_t top, int64_t bottom, uint32_t attr,
                                 const char *text) {
    uint64_t written = 0;
    int64_t width, height;
    if (slot >= FB_TASK_TEXT_SLOTS || !text ||
        !fb_task_text_bounds(left, right, top, bottom))
        return 0;
    width = right - left + 1;
    height = bottom - top + 1;
    if (width > ZEAL_TEXT_MAX_COLS || height > ZEAL_TEXT_MAX_ROWS)
        return 0;
    for (size_t n = 0; text[n] && n < 4096; n++) {
        unsigned char ch = (unsigned char)text[n];
        if (ch == '\r') {
            g_fb_task_text_x[slot] = 0;
            continue;
        }
        if (ch == '\n') {
            g_fb_task_text_x[slot] = 0;
            if (++g_fb_task_text_y[slot] >= height) {
                fb_task_text_scroll(slot, left, top, width, height);
                g_fb_task_text_y[slot] = (uint16_t)(height - 1);
            }
            continue;
        }
        if (ch == '\b') {
            if (g_fb_task_text_x[slot])
                g_fb_task_text_x[slot]--;
            else if (g_fb_task_text_y[slot]) {
                g_fb_task_text_y[slot]--;
                g_fb_task_text_x[slot] = (uint16_t)(width - 1);
            }
            ch = ' ';
        }
        if (ch < 32)
            continue;
        if (g_fb_task_text_x[slot] >= width) {
            g_fb_task_text_x[slot] = 0;
            if (++g_fb_task_text_y[slot] >= height) {
                g_fb_task_text_y[slot] = (uint16_t)(height - 1);
                fb_task_text_scroll(slot, left, top, width, height);
            }
        }
        size_t local = (size_t)g_fb_task_text_y[slot] * ZEAL_TEXT_MAX_COLS +
                       g_fb_task_text_x[slot];
        uint32_t cell = ((attr & 0xffu) << 8) | ch;
        g_fb_task_text[slot][local] = cell;
        g_fb_task_text_valid[slot][local] = 1;
        size_t index = (size_t)(top + g_fb_task_text_y[slot]) * ZEAL_TEXT_MAX_COLS +
                       (size_t)(left + g_fb_task_text_x[slot]);
        g_fb_text_cells[index] = cell;
        g_fb_text_valid[index] = 1;
        fb_text_present_cell((uint32_t)(left + g_fb_task_text_x[slot]),
                             (uint32_t)(top + g_fb_task_text_y[slot]), cell);
        g_zc_text_cells_drawn++;
        g_fb_task_text_x[slot]++;
        written++;
    }
    return written;
}

uint64_t zeal_fb_task_text_compose(unsigned slot, int64_t left, int64_t right,
                                   int64_t top, int64_t bottom,
                                   uint32_t *text_base, int64_t stride,
                                   const uint16_t *z_buf, uint16_t task_z) {
    if (slot >= FB_TASK_TEXT_SLOTS ||
        !fb_task_text_bounds(left, right, top, bottom))
        return 0;
    int64_t width = right - left + 1, height = bottom - top + 1;
    if (width > ZEAL_TEXT_MAX_COLS || height > ZEAL_TEXT_MAX_ROWS)
        return 0;
    uint64_t drawn = 0;
    for (int64_t y = 0; y < height; y++) {
        for (int64_t x = 0; x < width; x++) {
            int64_t col = left + x, row = top + y;
            size_t local = (size_t)y * ZEAL_TEXT_MAX_COLS + (size_t)x;
            if (!g_fb_task_text_valid[slot][local] ||
                (z_buf && (!task_z ||
                 z_buf[(size_t)row * ZEAL_TEXT_MAX_COLS + (size_t)col] > task_z)))
                continue;
            uint32_t cell = g_fb_task_text[slot][local];
            size_t index = (size_t)row * ZEAL_TEXT_MAX_COLS + (size_t)col;
            g_fb_text_cells[index] = cell;
            g_fb_text_valid[index] = 1;
            fb_text_present_cell((uint32_t)col, (uint32_t)row, cell);
            g_zc_text_cells_drawn++;
            drawn++;
            if (text_base && stride > col &&
                (uint64_t)col < ZEAL_TEXT_PLANE_COLS &&
                (uint64_t)row < ZEAL_TEXT_PLANE_ROWS)
                text_base[(size_t)row * (size_t)stride + (size_t)col] = cell;
        }
    }
    return drawn;
}

uint64_t zeal_fb_shell_text_compose(uint32_t *text_base, int64_t stride) {
    int64_t cols = g_fb_cols < ZEAL_TEXT_PLANE_COLS
        ? g_fb_cols : ZEAL_TEXT_PLANE_COLS;
    int64_t rows = g_fb_rows < ZEAL_TEXT_PLANE_ROWS
        ? g_fb_rows : ZEAL_TEXT_PLANE_ROWS;
    if (!cols || !rows)
        return 0;
    return zeal_fb_task_text_compose(FB_SHELL_TEXT_SLOT, 0, cols - 1,
                                     0, rows - 1, text_base, stride, NULL, 0);
}

static void fb_putc(char ch) {
    uint32_t cols, rows;
    if (!g_fb || fb_console_owned_by_desktop()) {
        return;
    }
    cols = g_fb_cols < ZEAL_TEXT_MAX_COLS ? g_fb_cols : ZEAL_TEXT_MAX_COLS;
    rows = g_fb_rows < ZEAL_TEXT_MAX_ROWS ? g_fb_rows : ZEAL_TEXT_MAX_ROWS;
    if (!cols || !rows)
        return;
    char cell[2] = {ch, 0};
    (void)zeal_fb_task_text_write(FB_SHELL_TEXT_SLOT, 0, cols - 1,
                                  0, rows - 1, 0x0f, cell);
    if (ch == '\n') {
        g_fb_cx = 0;
        if (g_fb_cy + 1 < rows)
            g_fb_cy++;
        else
            g_fb_cy = rows - 1;
        return;
    }
    if (ch == '\r') {
        g_fb_cx = 0;
        return;
    }
    if (++g_fb_cx >= cols) {
        g_fb_cx = 0;
        if (g_fb_cy + 1 < rows)
            g_fb_cy++;
        else
            g_fb_cy = rows - 1;
    }
}

#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
static void pi_diag_fb_puts(const char *s) {
    while (s && *s) {
        fb_putc(*s++);
    }
}

static void pi_diag_fb_u64_hex(uint64_t value) {
    static const char hex[] = "0123456789abcdef";
    pi_diag_fb_puts("0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        fb_putc(hex[(value >> shift) & 0xf]);
    }
}

static void pi_diag_fb_u32_hex(uint32_t value) {
    static const char hex[] = "0123456789abcdef";
    pi_diag_fb_puts("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        fb_putc(hex[(value >> shift) & 0xf]);
    }
}

static int pi4_pcie_read_config32(const struct zeal_handoff *h,
                                  uint32_t link_status, unsigned offset,
                                  uint32_t *value) {
    struct pi4_pcie_cfg_read plan;
    uint64_t hhdm, index_phys, data_phys;
    volatile uint32_t *index_reg;
    volatile uint32_t *data_reg;
    uint32_t result = 0xffffffffu;

    if (!value || pi4_pcie_cfg_read_plan(h, link_status, 1, 0, 0,
                                         offset, &plan) != 0)
        return -1;
    hhdm = h->hhdm_offset;
    if (h->pi4_pcie.reg_base > UINT64_MAX - PI4_PCIE_CFG_INDEX_OFFSET ||
        h->pi4_pcie.reg_base > UINT64_MAX - plan.data_offset)
        return -2;
    index_phys = h->pi4_pcie.reg_base + PI4_PCIE_CFG_INDEX_OFFSET;
    data_phys = h->pi4_pcie.reg_base + plan.data_offset;
    if (index_phys >= (1ull << 48) || data_phys >= (1ull << 48) ||
        hhdm > UINT64_MAX - index_phys || hhdm > UINT64_MAX - data_phys)
        return -3;

    index_reg = (volatile uint32_t *)(uintptr_t)(hhdm + index_phys);
    data_reg = (volatile uint32_t *)(uintptr_t)(hhdm + data_phys);
    g_uart_probe = 1;
    g_sync_recover = 1;
    g_uart_dead = 0;
    g_probe_esr = 0;
    g_probe_far = 0;
    __asm__ volatile("str %w1, [%0]" :: "r"(index_reg), "r"(plan.index) : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
    if (!g_uart_dead) {
        __asm__ volatile("ldr %w0, [%1]" : "=r"(result) : "r"(data_reg) : "memory");
        __asm__ volatile("dsb sy" ::: "memory");
    }
    g_sync_recover = 0;
    g_uart_probe = 0;
    if (g_uart_dead) {
        g_uart_dead = 0;
        return -4;
    }
    *value = result;
    return 0;
}

static void pi4_pcie_probe_first_function(const struct zeal_handoff *h,
                                         uint32_t link_status,
                                         volatile uint8_t *log_uart) {
    uint32_t id = 0xffffffffu;
    uint32_t class_rev = 0xffffffffu;
    uint32_t header = 0xffffffffu;
    uint32_t command = 0xffffffffu;
    unsigned bar_count = 0;

    g_pi4_link_status = link_status;
    g_pi4_cfg_state = PI4_CFG_NOT_RUN;
    g_pi4_cfg_bar_count = 0;
    for (unsigned i = 0; i < 6; i++)
        g_pi4_cfg_bars[i] = 0;
    if ((link_status & (PI4_PCIE_STATUS_PHY_LINKUP | PI4_PCIE_STATUS_DL_ACTIVE)) !=
        (PI4_PCIE_STATUS_PHY_LINKUP | PI4_PCIE_STATUS_DL_ACTIVE)) {
        g_pi4_cfg_state = PI4_CFG_NOT_LINKED;
        uart_puts(log_uart, "PCIe cfg skipped: link not active\n");
        return;
    }
    if (!g_pt_pool_phys || mmio_map_2m(h->hhdm_offset, h->pi4_pcie.reg_base) != 0) {
        g_pi4_cfg_state = PI4_CFG_MAP_FAIL;
        uart_puts(log_uart, "PCIe cfg skipped: Device MMIO map failed\n");
        return;
    }

    /* Read only bus 1/device 0/function 0; never write endpoint config. */
    if (pi4_pcie_read_config32(h, link_status, 0x00, &id) != 0 ||
        pi4_pcie_read_config32(h, link_status, 0x08, &class_rev) != 0 ||
        pi4_pcie_read_config32(h, link_status, 0x0c, &header) != 0) {
        g_pi4_cfg_state = PI4_CFG_MMIO_FAIL;
        uart_puts(log_uart, "PCIe cfg read aborted ESR=");
        uart_put_u64_hex(log_uart, g_probe_esr);
        uart_puts(log_uart, " FAR=");
        uart_put_u64_hex(log_uart, g_probe_far);
        uart_puts(log_uart, "\n");
        g_uart_dead = 0;
        return;
    }
    if (id == 0xffffffffu || (id & 0xffffu) == 0xffffu) {
        g_pi4_cfg_id = id;
        g_pi4_cfg_state = PI4_CFG_NO_DEVICE;
        uart_puts(log_uart, "PCIe cfg 01:00.0 absent ID=");
        uart_put_u64_hex(log_uart, id);
        uart_puts(log_uart, "\n");
        return;
    }

    /* Header type 0 has six BARs; a PCI bridge (type 1) has only two.
     * Never report a bridge's bus-number registers as BARs. */
    unsigned header_type = (header >> 16) & 0x7fu;
    if (header_type == 0)
        bar_count = 6;
    else if (header_type == 1)
        bar_count = 2;
    if (pi4_pcie_read_config32(h, link_status, 0x04, &command) != 0) {
        g_pi4_cfg_state = PI4_CFG_MMIO_FAIL;
        uart_puts(log_uart, "PCIe command/status read aborted ESR=");
        uart_put_u64_hex(log_uart, g_probe_esr);
        uart_puts(log_uart, " FAR=");
        uart_put_u64_hex(log_uart, g_probe_far);
        uart_puts(log_uart, "\n");
        g_uart_dead = 0;
        return;
    }
    for (unsigned i = 0; i < bar_count; i++) {
        if (pi4_pcie_read_config32(h, link_status, 0x10 + i * 4,
                                   &g_pi4_cfg_bars[i]) != 0) {
            g_pi4_cfg_state = PI4_CFG_MMIO_FAIL;
            uart_puts(log_uart, "PCIe BAR read aborted ESR=");
            uart_put_u64_hex(log_uart, g_probe_esr);
            uart_puts(log_uart, " FAR=");
            uart_put_u64_hex(log_uart, g_probe_far);
            uart_puts(log_uart, "\n");
            g_uart_dead = 0;
            return;
        }
    }
    g_pi4_cfg_id = id;
    g_pi4_cfg_class_rev = class_rev;
    g_pi4_cfg_header = header;
    g_pi4_cfg_command = command;
    g_pi4_cfg_bar_count = bar_count;
    g_pi4_cfg_state = PI4_CFG_READ_OK;
    uart_puts(log_uart, "PCIe cfg 01:00.0 ID=");
    uart_put_u64_hex(log_uart, id);
    uart_puts(log_uart, " class/rev=");
    uart_put_u64_hex(log_uart, class_rev);
    uart_puts(log_uart, " header=");
    uart_put_u64_hex(log_uart, header);
    uart_puts(log_uart, " command/status=");
    uart_put_u64_hex(log_uart, command);
    uart_puts(log_uart, " (identity/BAR read)\n");
    for (unsigned i = 0; i < bar_count; i++) {
        uart_puts(log_uart, "  BAR");
        uart_write(log_uart, '0' + i);
        uart_puts(log_uart, "=");
        uart_put_u64_hex(log_uart, g_pi4_cfg_bars[i]);
        uart_puts(log_uart, "\n");
    }
}

static int pi4_pcie_write_config32(const struct zeal_handoff *h,
                                   uint32_t link_status, unsigned offset,
                                   uint32_t value) {
    struct pi4_pcie_cfg_read plan;
    uint64_t index_phys, data_phys;
    volatile uint32_t *index_reg;
    volatile uint32_t *data_reg;

    if (offset != 0x04 || value > 0xffffu ||
        pi4_pcie_cfg_read_plan(h, link_status, 1, 0, 0, offset, &plan) != 0 ||
        h->pi4_pcie.reg_base > UINT64_MAX - PI4_PCIE_CFG_INDEX_OFFSET ||
        h->pi4_pcie.reg_base > UINT64_MAX - plan.data_offset)
        return -1;
    index_phys = h->pi4_pcie.reg_base + PI4_PCIE_CFG_INDEX_OFFSET;
    data_phys = h->pi4_pcie.reg_base + plan.data_offset;
    if (index_phys >= (1ull << 48) || data_phys >= (1ull << 48) ||
        h->hhdm_offset > UINT64_MAX - index_phys ||
        h->hhdm_offset > UINT64_MAX - data_phys)
        return -2;

    index_reg = (volatile uint32_t *)(uintptr_t)(h->hhdm_offset + index_phys);
    data_reg = (volatile uint32_t *)(uintptr_t)(h->hhdm_offset + data_phys);
    g_uart_probe = 1;
    g_sync_recover = 1;
    g_uart_dead = 0;
    g_probe_esr = 0;
    g_probe_far = 0;
    __asm__ volatile("str %w1, [%0]" :: "r"(index_reg), "r"(plan.index) : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
    if (!g_uart_dead) {
        /* For command/status at 0x04, upper status bits are zero (W1C-safe). */
        __asm__ volatile("str %w1, [%0]" :: "r"(data_reg), "r"(value) : "memory");
        __asm__ volatile("dsb sy" ::: "memory");
    }
    g_sync_recover = 0;
    g_uart_probe = 0;
    if (g_uart_dead) {
        g_uart_dead = 0;
        return -3;
    }
    return 0;
}

static int pi4_xhci_read_mmio32(volatile uint8_t *base, unsigned offset,
                                unsigned span, uint32_t *value) {
    uint32_t result = 0;
    if (!base || !value || span < sizeof(uint32_t) || (offset & 3u) ||
        offset > span - sizeof(uint32_t))
        return -1;
    g_uart_probe = 1;
    g_sync_recover = 1;
    g_uart_dead = 0;
    g_probe_esr = 0;
    g_probe_far = 0;
    __asm__ volatile("ldr %w0, [%1]" : "+&r"(result) :
                     "r"((volatile uint32_t *)(base + offset)) : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
    g_sync_recover = 0;
    g_uart_probe = 0;
    if (g_uart_dead) {
        g_uart_dead = 0;
        return -2;
    }
    *value = result;
    return 0;
}

static int pi4_xhci_write_mmio32(volatile uint8_t *base, unsigned offset,
                                 unsigned span, uint32_t value) {
    if (!base || span < sizeof(uint32_t) || (offset & 3u) ||
        offset > span - sizeof(uint32_t))
        return -1;
    g_uart_probe = 1;
    g_sync_recover = 1;
    g_uart_dead = 0;
    g_probe_esr = 0;
    g_probe_far = 0;
    __asm__ volatile("str %w1, [%0]" ::
                     "r"((volatile uint32_t *)(base + offset)), "r"(value)
                     : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
    g_sync_recover = 0;
    g_uart_probe = 0;
    if (g_uart_dead) {
        g_uart_dead = 0;
        return -2;
    }
    return 0;
}

static int pi4_xhci_write_mmio64(volatile uint8_t *base, unsigned offset,
                                 unsigned span, uint64_t value) {
    if (!base || span < sizeof(uint64_t) || (offset & 7u) ||
        offset > span - sizeof(uint64_t))
        return -1;
    /* Test split writes because the previous single ARM Qword store produced
     * mismatched VL805 pointer readbacks. xHCI permits this sequence when the
     * platform cannot issue Qword accesses: low Dword first, then high. The
     * 32-bit helper includes a DSB after each device write. */
    if (pi4_xhci_write_mmio32(base, offset, span, (uint32_t)value) != 0)
        return -2;
    if (pi4_xhci_write_mmio32(base, offset + sizeof(uint32_t), span,
                              (uint32_t)(value >> 32)) != 0)
        return -2;
    return 0;
}

static int pi4_xhci_read_bar32(const struct zeal_handoff *h,
                              const struct pi4_xhci_outbound_window *window,
                              uint64_t bar_bus, uint32_t offset,
                              uint32_t *value) {
    uint64_t cpu, virt;
    if (!h || !value ||
        pi4_xhci_bar_offset_cpu(h, bar_bus, window, offset,
                                sizeof(uint32_t), &cpu) != 0 ||
        !h->hhdm_offset || h->hhdm_offset > UINT64_MAX - cpu ||
        mmio_map_2m(h->hhdm_offset, cpu) != 0)
        return -1;
    virt = h->hhdm_offset + cpu;
    return pi4_xhci_read_mmio32((volatile uint8_t *)(uintptr_t)virt,
                                0, sizeof(uint32_t), value);
}

static int pi4_xhci_write_bar32(const struct zeal_handoff *h,
                                const struct pi4_xhci_outbound_window *window,
                                uint64_t bar_bus, uint32_t offset,
                                uint32_t value) {
    uint64_t cpu, virt;
    if (!h ||
        pi4_xhci_bar_offset_cpu(h, bar_bus, window, offset,
                                sizeof(uint32_t), &cpu) != 0 ||
        !h->hhdm_offset || h->hhdm_offset > UINT64_MAX - cpu ||
        mmio_map_2m(h->hhdm_offset, cpu) != 0)
        return -1;
    virt = h->hhdm_offset + cpu;
    return pi4_xhci_write_mmio32((volatile uint8_t *)(uintptr_t)virt,
                                 0, sizeof(uint32_t), value);
}

/*
 * Request USB legacy ownership only while xHCI is halted. Bus mastering stays
 * off, and this writes only the OS Owned semaphore in USBLEGSUP. Firmware gets
 * up to one counter-second to clear BIOS Owned; no controller run/reset occurs.
 */
static void pi4_xhci_legacy_handoff(
    const struct zeal_handoff *h,
    const struct pi4_xhci_outbound_window *window,
    uint64_t bar_bus, uint32_t usbcmd, uint32_t usbsts,
    uint32_t offset, uint32_t capability) {
    uint32_t request, value;
    uint64_t frequency, start, last_poll, now, poll_interval;
    int first_poll = 1;

    g_pi4_xhci_legacy_after = capability;
    if (!pi4_xhci_handoff_safe(usbcmd, usbsts)) {
        g_pi4_xhci_state_mask |= PI4_XHCI_STATE_HANDOFF_UNSAFE;
        return;
    }
    if (pi4_xhci_legacy_claim_value(capability, &request) != 0 ||
        (request != capability &&
         pi4_xhci_write_bar32(h, window, bar_bus, offset, request) != 0)) {
        g_pi4_xhci_state_mask |= PI4_XHCI_STATE_HANDOFF_FAIL;
        g_pi4_xhci_state_esr = g_probe_esr;
        g_pi4_xhci_state_far = g_probe_far;
        return;
    }

    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(start));
    last_poll = start;
    poll_interval = frequency / 100u;
    if (!poll_interval) poll_interval = 1;
    for (;;) {
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        if (!first_poll) {
            if (!frequency || now - start >= frequency) {
                g_pi4_xhci_state_mask |= PI4_XHCI_STATE_HANDOFF_TIMEOUT;
                return;
            }
            if (now - last_poll < poll_interval) continue;
            last_poll = now;
        }
        first_poll = 0;
        if (pi4_xhci_read_bar32(h, window, bar_bus, offset, &value) != 0) {
            g_pi4_xhci_state_mask |= PI4_XHCI_STATE_HANDOFF_FAIL |
                                    PI4_XHCI_STATE_READ_FAIL;
            g_pi4_xhci_state_esr = g_probe_esr;
            g_pi4_xhci_state_far = g_probe_far;
            return;
        }
        g_pi4_xhci_legacy_after = value;
        if (!(value & PI4_XHCI_LEGACY_BIOS_OWNED)) {
            g_pi4_xhci_state_mask |= PI4_XHCI_STATE_HANDOFF_OK;
            return;
        }
    }
}

static const char *pi4_xhci_handoff_result(void) {
    if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_HANDOFF_OK)
        return "firmware-released";
    if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_HANDOFF_TIMEOUT)
        return "BIOS-owned-timeout";
    if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_HANDOFF_UNSAFE)
        return "skipped-controller-not-halted";
    if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_HANDOFF_FAIL)
        return "request-failed";
    return "not-attempted";
}

static const char *pi4_xhci_reset_result(void) {
    switch (g_pi4_xhci_reset_state) {
    case 1: return "skipped-no-handoff";
    case 2: return "skipped-unsafe";
    case 3: return "complete";
    case 4:
        return g_pi4_xhci_reset_phase == 1 ? "timeout-HCRST" : "timeout-CNR";
    case 5: return "read-write-failed";
    default: return "not-attempted";
    }
}

/* Reset only after firmware ownership is released, with PCI bus mastering off.
 * Poll HCRST then CNR for at most one counter-second; do not start the xHC. */
static void pi4_xhci_reset_halted(
    const struct zeal_handoff *h,
    const struct pi4_xhci_outbound_window *window,
    uint64_t bar_bus, const struct pi4_xhci_state_plan *plan) {
    uint32_t usbcmd, usbsts;
    uint64_t frequency, start, last_poll, now, poll_interval;
    int first_poll = 1;

    g_pi4_xhci_reset_state = 0;
    g_pi4_xhci_reset_phase = 0;
    g_pi4_xhci_reset_usbcmd = 0;
    g_pi4_xhci_reset_usbsts = 0;
    if (!(g_pi4_xhci_state_mask & PI4_XHCI_STATE_HANDOFF_OK)) {
        g_pi4_xhci_reset_state = 1;
        return;
    }
    if (pi4_xhci_read_bar32(h, window, bar_bus, plan->usbcmd_offset,
                            &usbcmd) != 0 ||
        pi4_xhci_read_bar32(h, window, bar_bus, plan->usbsts_offset,
                            &usbsts) != 0) {
        g_pi4_xhci_reset_state = 5;
        g_pi4_xhci_state_mask |= PI4_XHCI_STATE_RESET_FAIL |
                                 PI4_XHCI_STATE_READ_FAIL;
        g_pi4_xhci_state_esr = g_probe_esr;
        g_pi4_xhci_state_far = g_probe_far;
        return;
    }
    g_pi4_xhci_reset_usbcmd = usbcmd;
    g_pi4_xhci_reset_usbsts = usbsts;
    if (!pi4_xhci_reset_safe(usbcmd, usbsts, g_pi4_xhci_legacy_after,
                             (uint16_t)g_pi4_cfg_command)) {
        g_pi4_xhci_reset_state = 2;
        g_pi4_xhci_state_mask |= PI4_XHCI_STATE_RESET_UNSAFE;
        return;
    }

    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(start));
    if (!frequency) {
        g_pi4_xhci_reset_state = 5;
        g_pi4_xhci_state_mask |= PI4_XHCI_STATE_RESET_FAIL;
        return;
    }
    last_poll = start;
    poll_interval = frequency / 100u;
    if (!poll_interval) poll_interval = 1;
    g_pi4_xhci_reset_phase = 1;
    if (pi4_xhci_write_bar32(h, window, bar_bus, plan->usbcmd_offset,
                             PI4_XHCI_USBCMD_HCRST) != 0) {
        g_pi4_xhci_reset_state = 5;
        g_pi4_xhci_state_mask |= PI4_XHCI_STATE_RESET_FAIL |
                                 PI4_XHCI_STATE_READ_FAIL;
        g_pi4_xhci_state_esr = g_probe_esr;
        g_pi4_xhci_state_far = g_probe_far;
        return;
    }

    for (;;) {
        if (pi4_xhci_read_bar32(h, window, bar_bus, plan->usbcmd_offset,
                                &usbcmd) != 0) {
            g_pi4_xhci_reset_state = 5;
            g_pi4_xhci_state_mask |= PI4_XHCI_STATE_RESET_FAIL |
                                     PI4_XHCI_STATE_READ_FAIL;
            g_pi4_xhci_state_esr = g_probe_esr;
            g_pi4_xhci_state_far = g_probe_far;
            return;
        }
        if (!(usbcmd & PI4_XHCI_USBCMD_HCRST)) break;
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        if (now - start >= frequency) {
            g_pi4_xhci_reset_state = 4;
            g_pi4_xhci_state_mask |= PI4_XHCI_STATE_RESET_TIMEOUT;
            return;
        }
        if (!first_poll && now - last_poll < poll_interval) continue;
        first_poll = 0;
        last_poll = now;
    }

    g_pi4_xhci_reset_phase = 2;
    first_poll = 1;
    for (;;) {
        if (pi4_xhci_read_bar32(h, window, bar_bus, plan->usbsts_offset,
                                &usbsts) != 0) {
            g_pi4_xhci_reset_state = 5;
            g_pi4_xhci_state_mask |= PI4_XHCI_STATE_RESET_FAIL |
                                     PI4_XHCI_STATE_READ_FAIL;
            g_pi4_xhci_state_esr = g_probe_esr;
            g_pi4_xhci_state_far = g_probe_far;
            return;
        }
        if (!(usbsts & PI4_XHCI_USBSTS_CNR)) break;
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        if (now - start >= frequency) {
            g_pi4_xhci_reset_state = 4;
            g_pi4_xhci_state_mask |= PI4_XHCI_STATE_RESET_TIMEOUT;
            return;
        }
        if (!first_poll && now - last_poll < poll_interval) continue;
        first_poll = 0;
        last_poll = now;
    }

    if (pi4_xhci_read_bar32(h, window, bar_bus, plan->usbcmd_offset,
                            &usbcmd) != 0 ||
        pi4_xhci_read_bar32(h, window, bar_bus, plan->usbsts_offset,
                            &usbsts) != 0) {
        g_pi4_xhci_reset_state = 5;
        g_pi4_xhci_state_mask |= PI4_XHCI_STATE_RESET_FAIL |
                                 PI4_XHCI_STATE_READ_FAIL;
        g_pi4_xhci_state_esr = g_probe_esr;
        g_pi4_xhci_state_far = g_probe_far;
        return;
    }
    g_pi4_xhci_reset_usbcmd = usbcmd;
    g_pi4_xhci_reset_usbsts = usbsts;
    if ((usbcmd & (PI4_XHCI_USBCMD_RUN | PI4_XHCI_USBCMD_HCRST)) ||
        !(usbsts & PI4_XHCI_USBSTS_HALTED) ||
        (usbsts & PI4_XHCI_USBSTS_CNR)) {
        g_pi4_xhci_reset_state = 2;
        g_pi4_xhci_state_mask |= PI4_XHCI_STATE_RESET_UNSAFE;
        return;
    }
    g_pi4_xhci_reset_state = 3;
    g_pi4_xhci_state_mask |= PI4_XHCI_STATE_RESET_OK;
}

static void pi4_xhci_snapshot_ports(
    const struct zeal_handoff *h,
    const struct pi4_xhci_outbound_window *window,
    uint64_t bar_bus) {
    struct pi4_xhci_port_plan plan;
    uint32_t value;
    unsigned i;

    g_pi4_xhci_port_count = 0;
    g_pi4_xhci_port_total = 0;
    for (i = 0; i < PI4_XHCI_DIAG_PORTS; i++)
        g_pi4_xhci_portsc[i] = 0;
    if (pi4_xhci_portsc_plan(g_pi4_xhci_caps, &plan) != 0) {
        g_pi4_xhci_state_mask |= PI4_XHCI_STATE_READ_FAIL;
        return;
    }
    g_pi4_xhci_port_total = plan.total_ports;
    for (i = 0; i < plan.count; i++) {
        uint32_t offset = plan.first_portsc_offset +
                          i * PI4_XHCI_PORTSC_STRIDE;
        if (pi4_xhci_read_bar32(h, window, bar_bus, offset, &value) != 0) {
            g_pi4_xhci_state_mask |= PI4_XHCI_STATE_READ_FAIL;
            g_pi4_xhci_state_esr = g_probe_esr;
            g_pi4_xhci_state_far = g_probe_far;
            return;
        }
        g_pi4_xhci_portsc[i] = value;
        g_pi4_xhci_port_count++;
    }
    g_pi4_xhci_state_mask |= PI4_XHCI_STATE_PORTS;
}

/* Snapshot controller/firmware state before any optional ownership request. */
static void pi4_xhci_snapshot_state(
    const struct zeal_handoff *h,
    const struct pi4_xhci_outbound_window *window,
    uint64_t bar_bus) {
    struct pi4_xhci_state_plan plan;
    uint32_t offset;

    g_pi4_xhci_state_mask = 0;
    g_pi4_xhci_usbcmd = 0;
    g_pi4_xhci_usbsts = 0;
    g_pi4_xhci_reset_state = 0;
    g_pi4_xhci_reset_phase = 0;
    g_pi4_xhci_reset_usbcmd = 0;
    g_pi4_xhci_reset_usbsts = 0;
    g_pi4_xhci_legacy_offset = 0;
    g_pi4_xhci_legacy_cap = 0;
    g_pi4_xhci_port_count = 0;
    g_pi4_xhci_port_total = 0;
    for (unsigned i = 0; i < PI4_XHCI_DIAG_PORTS; i++)
        g_pi4_xhci_portsc[i] = 0;
    g_pi4_xhci_state_esr = 0;
    g_pi4_xhci_state_far = 0;
    if (pi4_xhci_state_offsets(g_pi4_xhci_caps, &plan) != 0) {
        g_pi4_xhci_state_mask = PI4_XHCI_STATE_READ_FAIL;
        return;
    }

    if (pi4_xhci_read_bar32(h, window, bar_bus, plan.usbcmd_offset,
                            &g_pi4_xhci_usbcmd) != 0 ||
        pi4_xhci_read_bar32(h, window, bar_bus, plan.usbsts_offset,
                            &g_pi4_xhci_usbsts) != 0) {
        g_pi4_xhci_state_mask = PI4_XHCI_STATE_READ_FAIL;
        g_pi4_xhci_state_esr = g_probe_esr;
        g_pi4_xhci_state_far = g_probe_far;
        return;
    }
    g_pi4_xhci_state_mask |= PI4_XHCI_STATE_OP_REGS;
    pi4_xhci_snapshot_ports(h, window, bar_bus);

    offset = plan.first_ext_cap_offset;
    if (!offset) {
        g_pi4_xhci_state_mask |= PI4_XHCI_STATE_NO_LEGACY;
        return;
    }
    for (unsigned i = 0; i < 64; i++) {
        uint32_t cap, next;
        if (pi4_xhci_read_bar32(h, window, bar_bus, offset, &cap) != 0) {
            g_pi4_xhci_state_mask |= PI4_XHCI_STATE_READ_FAIL;
            g_pi4_xhci_state_esr = g_probe_esr;
            g_pi4_xhci_state_far = g_probe_far;
            return;
        }
        if ((cap & 0xffu) == 1u) {
            g_pi4_xhci_legacy_offset = offset;
            g_pi4_xhci_legacy_cap = cap;
            g_pi4_xhci_legacy_after = cap;
            g_pi4_xhci_state_mask |= PI4_XHCI_STATE_LEGACY;
            pi4_xhci_legacy_handoff(h, window, bar_bus,
                                    g_pi4_xhci_usbcmd,
                                    g_pi4_xhci_usbsts, offset, cap);
            if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_HANDOFF_OK)
                pi4_xhci_reset_halted(h, window, bar_bus, &plan);
            return;
        }
        next = (cap >> 8) & 0xffu;
        if (!next) {
            g_pi4_xhci_state_mask |= PI4_XHCI_STATE_NO_LEGACY;
            return;
        }
        if (next > (PI4_XHCI_EXT_CAP_BYTES - 4u - offset) / 4u) {
            g_pi4_xhci_state_mask |= PI4_XHCI_STATE_READ_FAIL;
            return;
        }
        offset += next * 4u;
    }
    g_pi4_xhci_state_mask |= PI4_XHCI_STATE_READ_FAIL;
}

/* Read the BCM2711 root-complex translation/error registers without writes. */
static int pi4_pcie_read_mmio32(const struct zeal_handoff *h, unsigned offset,
                                uint32_t *value) {
    uint64_t phys, virt;
    uint32_t result = 0;
    volatile uint32_t *reg;

    if (!h || !value || (offset & 3u) || h->revision < 5 ||
        h->size < offsetof(struct zeal_handoff, pi4_pcie) + sizeof(h->pi4_pcie) ||
        h->pi4_pcie.reg_size < sizeof(uint32_t) ||
        offset > h->pi4_pcie.reg_size - sizeof(uint32_t) ||
        h->pi4_pcie.reg_base > UINT64_MAX - offset)
        return -1;
    phys = h->pi4_pcie.reg_base + offset;
    if (phys >= (1ull << 48) || h->hhdm_offset > UINT64_MAX - phys)
        return -1;
    virt = h->hhdm_offset + phys;
    reg = (volatile uint32_t *)(uintptr_t)virt;

    g_uart_probe = 1;
    g_sync_recover = 1;
    g_uart_dead = 0;
    g_probe_esr = 0;
    g_probe_far = 0;
    __asm__ volatile("ldr %w0, [%1]" : "+&r"(result) : "r"(reg) : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
    g_sync_recover = 0;
    g_uart_probe = 0;
    if (g_uart_dead) {
        g_uart_dead = 0;
        return -2;
    }
    *value = result;
    return 0;
}

/* Guarded 32-bit write for a bounded, save/restore PCIe bridge probe. */
static int pi4_pcie_write_mmio32(const struct zeal_handoff *h, unsigned offset,
                                 uint32_t value) {
    uint64_t phys, virt;
    volatile uint32_t *reg;

    if (!h || (offset & 3u) || h->revision < 5 ||
        h->size < offsetof(struct zeal_handoff, pi4_pcie) + sizeof(h->pi4_pcie) ||
        h->pi4_pcie.reg_size < sizeof(uint32_t) ||
        offset > h->pi4_pcie.reg_size - sizeof(uint32_t) ||
        h->pi4_pcie.reg_base > UINT64_MAX - offset)
        return -1;
    phys = h->pi4_pcie.reg_base + offset;
    if (phys >= (1ull << 48) || h->hhdm_offset > UINT64_MAX - phys)
        return -1;
    virt = h->hhdm_offset + phys;
    reg = (volatile uint32_t *)(uintptr_t)virt;

    g_uart_probe = 1;
    g_sync_recover = 1;
    g_uart_dead = 0;
    g_probe_esr = 0;
    g_probe_far = 0;
    __asm__ volatile("str %w1, [%0]" :: "r"(reg), "r"(value) : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
    g_sync_recover = 0;
    g_uart_probe = 0;
    if (g_uart_dead) {
        g_uart_dead = 0;
        return -2;
    }
    return 0;
}

static void pi4_pcie_readback_before(const struct zeal_handoff *h) {
    uint32_t value;

    g_pi4_pcie_readback = (struct pi4_pcie_readback){0};
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_WIN0_LO, &value) == 0) {
        g_pi4_pcie_readback.win0_lo = value;
        g_pi4_pcie_readback.mask |= 1u << 0;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_WIN0_HI, &value) == 0) {
        g_pi4_pcie_readback.win0_hi = value;
        g_pi4_pcie_readback.mask |= 1u << 1;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_WIN0_BASE_LIMIT, &value) == 0) {
        g_pi4_pcie_readback.win0_base_limit = value;
        g_pi4_pcie_readback.mask |= 1u << 2;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_WIN0_BASE_HI, &value) == 0) {
        g_pi4_pcie_readback.win0_base_hi = value;
        g_pi4_pcie_readback.mask |= 1u << 3;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_WIN0_LIMIT_HI, &value) == 0) {
        g_pi4_pcie_readback.win0_limit_hi = value;
        g_pi4_pcie_readback.mask |= 1u << 4;
    }
    /* BCM2711 BAR1..3 config registers define the live inbound DMA view.
     * These are read-only snapshots: do not reprogram firmware's windows. */
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_INBOUND_BAR1_LO, &value) == 0) {
        g_pi4_pcie_readback.inbound_bar1_lo = value;
        g_pi4_pcie_readback.mask |= 1u << 12;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_INBOUND_BAR1_HI, &value) == 0) {
        g_pi4_pcie_readback.inbound_bar1_hi = value;
        g_pi4_pcie_readback.mask |= 1u << 13;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_INBOUND_BAR2_LO, &value) == 0) {
        g_pi4_pcie_readback.inbound_bar2_lo = value;
        g_pi4_pcie_readback.mask |= 1u << 14;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_INBOUND_BAR2_HI, &value) == 0) {
        g_pi4_pcie_readback.inbound_bar2_hi = value;
        g_pi4_pcie_readback.mask |= 1u << 15;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_INBOUND_BAR3_LO, &value) == 0) {
        g_pi4_pcie_readback.inbound_bar3_lo = value;
        g_pi4_pcie_readback.mask |= 1u << 16;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_INBOUND_BAR3_HI, &value) == 0) {
        g_pi4_pcie_readback.inbound_bar3_hi = value;
        g_pi4_pcie_readback.mask |= 1u << 17;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_MISC_CTRL, &value) == 0) {
        g_pi4_pcie_readback.misc_ctrl = value;
        g_pi4_pcie_readback.mask |= 1u << 18;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_OUTB_ERR_VALID, &value) == 0) {
        g_pi4_pcie_readback.err_valid_before = value;
        g_pi4_pcie_readback.mask |= 1u << 5;
    }
}

static void pi4_pcie_readback_after(const struct zeal_handoff *h) {
    uint32_t value;

    if (pi4_pcie_read_mmio32(h, PI4_PCIE_OUTB_ERR_VALID, &value) == 0) {
        g_pi4_pcie_readback.err_valid_after = value;
        g_pi4_pcie_readback.mask |= 1u << 6;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_OUTB_ERR_INFO, &value) == 0) {
        g_pi4_pcie_readback.err_info = value;
        g_pi4_pcie_readback.mask |= 1u << 7;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_OUTB_ERR_ADDR, &value) == 0) {
        g_pi4_pcie_readback.err_addr = value;
        g_pi4_pcie_readback.mask |= 1u << 8;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_OUTB_MEM_ERR_ADDR_LO, &value) == 0) {
        g_pi4_pcie_readback.mem_err_addr_lo = value;
        g_pi4_pcie_readback.mask |= 1u << 9;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_OUTB_MEM_ERR_ADDR_HI, &value) == 0) {
        g_pi4_pcie_readback.mem_err_addr_hi = value;
        g_pi4_pcie_readback.mask |= 1u << 10;
    }
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_OUTB_MEM_ERR_CAUSE, &value) == 0) {
        g_pi4_pcie_readback.mem_err_cause = value;
        g_pi4_pcie_readback.mask |= 1u << 11;
    }
}

/*
 * Read xHCI capability/state through the firmware BAR. Memory Space Enable is
 * temporary; Bus Master Enable remains clear. If the controller is halted,
 * the only controller write is the OS Owned bit in USBLEGSUP. The endpoint
 * command word is restored before this diagnostic returns.
 */
static void pi4_xhci_probe_capabilities(const struct zeal_handoff *h,
                                        uint32_t link_status,
                                        volatile uint8_t *log_uart) {
    static const unsigned offsets[6] = {0x00, 0x04, 0x08, 0x10, 0x14, 0x18};
    struct pi4_xhci_outbound_window window;
    struct pi4_xhci_bar_plan bar;
    volatile uint8_t *regs;
    uint16_t original, enabled;
    uint32_t command_status, verify;
    uint64_t virt;
    uint64_t probe_esr = 0, probe_far = 0;
    int changed = 0;

    g_pi4_xhci_state = PI4_XHCI_NOT_RUN;
    g_pi4_xhci_bar_cpu = 0;
    g_pi4_xhci_bar_va = 0;
    g_pi4_xhci_state_mask = 0;
    g_pi4_xhci_usbcmd = 0;
    g_pi4_xhci_usbsts = 0;
    g_pi4_xhci_legacy_offset = 0;
    g_pi4_xhci_legacy_cap = 0;
    g_pi4_xhci_legacy_after = 0;
    g_pi4_xhci_port_count = 0;
    g_pi4_xhci_port_total = 0;
    for (unsigned i = 0; i < PI4_XHCI_DIAG_PORTS; i++)
        g_pi4_xhci_portsc[i] = 0;
    g_pi4_xhci_state_esr = 0;
    g_pi4_xhci_state_far = 0;
    for (unsigned i = 0; i < 6; i++) g_pi4_xhci_caps[i] = 0;
    if (g_pi4_cfg_state != PI4_CFG_READ_OK ||
        (g_pi4_cfg_id & 0xffffffffu) != 0x34831106u ||
        ((g_pi4_cfg_class_rev >> 8) & 0xffffffu) != 0x0c0330u ||
        g_pi4_cfg_bar_count < 2 ||
        (g_pi4_pcie_readback.mask & 0x1fu) != 0x1fu ||
        pi4_xhci_outbound_window_decode(
            g_pi4_pcie_readback.win0_lo, g_pi4_pcie_readback.win0_hi,
            g_pi4_pcie_readback.win0_base_limit,
            g_pi4_pcie_readback.win0_base_hi,
            g_pi4_pcie_readback.win0_limit_hi, &window) != 0 ||
        pi4_xhci_bar_plan(h, g_pi4_cfg_bars[0], g_pi4_cfg_bars[1],
                          &window, &bar) != 0) {
        g_pi4_xhci_state = PI4_XHCI_BAD_BAR;
        return;
    }
    g_pi4_xhci_bar_cpu = bar.cpu_base;
    if (!g_pt_pool_phys || !h->hhdm_offset ||
        h->hhdm_offset > UINT64_MAX - bar.cpu_base ||
        mmio_map_2m(h->hhdm_offset, bar.cpu_base) != 0) {
        g_pi4_xhci_state = PI4_XHCI_MAP_FAIL;
        return;
    }
    virt = h->hhdm_offset + bar.cpu_base;
    g_pi4_xhci_bar_va = virt;
    regs = (volatile uint8_t *)(uintptr_t)virt;

    original = (uint16_t)g_pi4_cfg_command;
    if (pi4_xhci_memory_command(original, &enabled) != 0) {
        g_pi4_xhci_state = PI4_XHCI_CMD_FAIL;
        return;
    }
    if (enabled != original) {
        changed = 1; /* Restore even if the guarded write reports an abort. */
        if (pi4_pcie_write_config32(h, link_status, 0x04, enabled) != 0) {
            g_pi4_xhci_state = PI4_XHCI_CFG_FAIL;
            probe_esr = g_probe_esr;
            probe_far = g_probe_far;
            goto restore;
        }
    }

    if (pi4_pcie_read_config32(h, link_status, 0x04, &verify) != 0 ||
        (uint16_t)verify != enabled ||
        ((uint16_t)verify & PI4_PCI_COMMAND_MASTER)) {
        g_pi4_xhci_state = PI4_XHCI_CFG_FAIL;
        probe_esr = g_probe_esr;
        probe_far = g_probe_far;
        goto restore;
    }
    command_status = verify;

    for (unsigned i = 0; i < 6; i++) {
        if (pi4_xhci_read_mmio32(regs, offsets[i],
                                 PI4_XHCI_CAP_BYTES,
                                 &g_pi4_xhci_caps[i]) != 0) {
            g_pi4_xhci_state = PI4_XHCI_MMIO_FAIL;
            probe_esr = g_probe_esr;
            probe_far = g_probe_far;
            goto restore;
        }
    }
    g_pi4_xhci_state = pi4_xhci_caps_valid(g_pi4_xhci_caps)
        ? PI4_XHCI_CAPS_OK : PI4_XHCI_CAPS_INVALID;
    if (g_pi4_xhci_state == PI4_XHCI_CAPS_OK)
        pi4_xhci_snapshot_state(h, &window, bar.bus_base);

restore:
    if (changed) {
        if (pi4_pcie_write_config32(h, link_status, 0x04, original) != 0 ||
            pi4_pcie_read_config32(h, link_status, 0x04, &verify) != 0 ||
            (uint16_t)verify != original) {
            g_pi4_xhci_state = PI4_XHCI_RESTORE_FAIL;
            probe_esr = g_probe_esr;
            probe_far = g_probe_far;
        }
    }
    if (g_pi4_xhci_state != PI4_XHCI_CAPS_OK &&
        g_pi4_xhci_state != PI4_XHCI_CAPS_INVALID) {
        g_probe_esr = probe_esr;
        g_probe_far = probe_far;
    }

    if (log_uart) {
        uart_puts(log_uart, "Pi xHCI cap probe state=");
        uart_put_u64_hex(log_uart, g_pi4_xhci_state);
        if (g_pi4_xhci_state == PI4_XHCI_CAPS_OK ||
            g_pi4_xhci_state == PI4_XHCI_CAPS_INVALID) {
            for (unsigned i = 0; i < 6; i++) {
                uart_puts(log_uart, " cap[");
                uart_put_u32_dec(log_uart, offsets[i]);
                uart_puts(log_uart, "]=");
                uart_put_u64_hex(log_uart, g_pi4_xhci_caps[i]);
            }
            uart_puts(log_uart, " cmd/status=");
            uart_put_u64_hex(log_uart, command_status);
            uart_puts(log_uart, "; BM stayed off, command restored\n");
            if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_OP_REGS) {
                uart_puts(log_uart, " xHCI USBCMD=");
                uart_put_u64_hex(log_uart, g_pi4_xhci_usbcmd);
                uart_puts(log_uart, " USBSTS=");
                uart_put_u64_hex(log_uart, g_pi4_xhci_usbsts);
                uart_puts(log_uart, "\n");
            }
            if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_LEGACY) {
                uart_puts(log_uart, " xHCI USBLEGSUP offset=");
                uart_put_u64_hex(log_uart, g_pi4_xhci_legacy_offset);
                uart_puts(log_uart, " initial=");
                uart_put_u64_hex(log_uart, g_pi4_xhci_legacy_cap);
                uart_puts(log_uart, " final=");
                uart_put_u64_hex(log_uart, g_pi4_xhci_legacy_after);
                uart_puts(log_uart, " handoff=");
                uart_puts(log_uart, pi4_xhci_handoff_result());
                uart_puts(log_uart, " (BME off)\n");
                uart_puts(log_uart, " xHCI HCRST=");
                uart_puts(log_uart, pi4_xhci_reset_result());
                uart_puts(log_uart, " post USBCMD/USBSTS=");
                uart_put_u64_hex(log_uart, g_pi4_xhci_reset_usbcmd);
                uart_puts(log_uart, "/");
                uart_put_u64_hex(log_uart, g_pi4_xhci_reset_usbsts);
                uart_puts(log_uart, " (BME off, R/S clear)\n");
            } else if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_NO_LEGACY) {
                uart_puts(log_uart, " xHCI USB legacy ownership capability absent\n");
            }
            if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_PORTS) {
                uart_puts(log_uart, " xHCI PORTSC count=");
                uart_put_u32_dec(log_uart, g_pi4_xhci_port_count);
                uart_puts(log_uart, "/");
                uart_put_u32_dec(log_uart, g_pi4_xhci_port_total);
                for (unsigned i = 0; i < g_pi4_xhci_port_count; i++) {
                    uart_puts(log_uart, " ");
                    uart_write(log_uart, '1' + i);
                    uart_puts(log_uart, "=");
                    uart_put_u64_hex(log_uart, g_pi4_xhci_portsc[i]);
                }
                uart_puts(log_uart, " (read-only)\n");
            }
            if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_READ_FAIL) {
                uart_puts(log_uart, " xHCI state read unavailable ESR=");
                uart_put_u64_hex(log_uart, g_pi4_xhci_state_esr);
                uart_puts(log_uart, " FAR=");
                uart_put_u64_hex(log_uart, g_pi4_xhci_state_far);
                uart_puts(log_uart, "\n");
            }
        } else {
            uart_puts(log_uart, " ESR=");
            uart_put_u64_hex(log_uart, g_probe_esr);
            uart_puts(log_uart, " FAR=");
            uart_put_u64_hex(log_uart, g_probe_far);
            uart_puts(log_uart, "\n");
        }
    }
}

/* Probe the bridge independently of PL011 so HDMI-only Pi diagnostics still
 * report the firmware link state. Endpoint discovery remains read-only; the
 * separate capability step may temporarily enable PCI memory decoding. */
static void pi4_pcie_probe_readonly(const struct zeal_handoff *h) {
    const uint32_t required = PI4_PCIE_FDT_PRESENT | PI4_PCIE_FDT_ENABLED |
                              PI4_PCIE_FDT_REG;
    uint64_t status_phys;
    volatile uint32_t *status_reg;
    uint32_t status = 0;

    g_pi4_link_status = 0;
    g_pi4_cfg_state = PI4_CFG_NOT_RUN;
    g_pi4_cfg_bar_count = 0;
    g_pi4_xhci_state = PI4_XHCI_NOT_RUN;
    if (!h || h->revision < 5 ||
        h->size < offsetof(struct zeal_handoff, pi4_pcie) + sizeof(h->pi4_pcie) ||
        (h->pi4_pcie.flags & required) != required ||
        h->pi4_pcie.reg_size < PI4_PCIE_STATUS_OFFSET + sizeof(uint32_t) ||
        h->pi4_pcie.reg_base > UINT64_MAX - PI4_PCIE_STATUS_OFFSET) {
        g_pi4_cfg_state = PI4_CFG_FDT_SKIP;
        return;
    }

    status_phys = h->pi4_pcie.reg_base + PI4_PCIE_STATUS_OFFSET;
    if (status_phys >= (1ull << 48) ||
        h->hhdm_offset > UINT64_MAX - status_phys ||
        mmio_map_2m(h->hhdm_offset, h->pi4_pcie.reg_base) != 0) {
        g_pi4_cfg_state = PI4_CFG_MAP_FAIL;
        return;
    }

    status_reg = (volatile uint32_t *)(uintptr_t)(h->hhdm_offset + status_phys);
    g_uart_probe = 1;
    g_sync_recover = 1;
    g_uart_dead = 0;
    g_probe_esr = 0;
    g_probe_far = 0;
    __asm__ volatile("ldr %w0, [%1]" : "+&r"(status) : "r"(status_reg) : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
    g_sync_recover = 0;
    g_uart_probe = 0;
    if (g_uart_dead) {
        g_pi4_cfg_state = PI4_CFG_STATUS_FAIL;
        g_uart_dead = 0;
        return;
    }

    g_pi4_link_status = status;
    pi4_pcie_probe_first_function(h, status,
                                  g_pi4_uart_tx_ok ? g_uart : NULL);
    if (g_pi4_cfg_state == PI4_CFG_READ_OK) {
        pi4_pcie_readback_before(h);
        pi4_xhci_probe_capabilities(h, status,
                                    g_pi4_uart_tx_ok ? g_uart : NULL);
        pi4_pcie_readback_after(h);
    }
}

#define PI4_DMA_CPU_NOT_RUN          0u
#define PI4_DMA_CPU_PASS             1u
#define PI4_DMA_CPU_RESET_REQUIRED  2u
#define PI4_DMA_CPU_ARENA_INVALID   3u
#define PI4_DMA_CPU_WINDOW_INVALID  4u
#define PI4_DMA_CPU_MEMMAP_INVALID  5u
#define PI4_DMA_CPU_HHDM_INVALID    6u
#define PI4_DMA_CPU_LINE_UNSUPPORTED 7u
#define PI4_DMA_CPU_READBACK_FAIL   8u

static const char *pi4_dma_cpu_result(void) {
    switch (g_pi4_dma_probe_state) {
    case PI4_DMA_CPU_PASS: return "pass";
    case PI4_DMA_CPU_RESET_REQUIRED: return "skipped-reset-gate";
    case PI4_DMA_CPU_ARENA_INVALID: return "invalid-arena";
    case PI4_DMA_CPU_WINDOW_INVALID: return "invalid-window";
    case PI4_DMA_CPU_MEMMAP_INVALID: return "invalid-memmap";
    case PI4_DMA_CPU_HHDM_INVALID: return "invalid-HHDM";
    case PI4_DMA_CPU_LINE_UNSUPPORTED: return "unsupported-cache-line";
    case PI4_DMA_CPU_READBACK_FAIL: return "readback-failed";
    default: return "not-run";
    }
}

/*
 * Exercise only CPU memory and cache maintenance in the reserved DMA arena.
 * This deliberately does not program xHCI, touch PCI config, or enable BME.
 */
static void pi4_dma_cpu_cache_probe(const struct zeal_handoff *h) {
    struct pi4_dma_probe_plan plan;
    int plan_rc;
    uint64_t ctr, line_bytes, arena_va, first_va, offset;
    volatile uint8_t *line[2];
    int mismatch = 0;

    g_pi4_dma_probe_state = PI4_DMA_CPU_NOT_RUN;
    g_pi4_dma_probe_line = 0;
    if (!(g_pi4_xhci_state_mask & PI4_XHCI_STATE_RESET_OK) ||
        (g_pi4_cfg_command & 0xffffu & PI4_PCI_COMMAND_MASTER)) {
        g_pi4_dma_probe_state = PI4_DMA_CPU_RESET_REQUIRED;
        return;
    }
    plan_rc = pi4_dma_probe_plan_make(h, &plan);
    if (plan_rc != PI4_DMA_PROBE_PLAN_OK) {
        g_pi4_dma_probe_state = plan_rc == PI4_DMA_PROBE_PLAN_ARENA
            ? PI4_DMA_CPU_ARENA_INVALID
            : plan_rc == PI4_DMA_PROBE_PLAN_WINDOW
                ? PI4_DMA_CPU_WINDOW_INVALID
                : plan_rc == PI4_DMA_PROBE_PLAN_MEMMAP
                    ? PI4_DMA_CPU_MEMMAP_INVALID : PI4_DMA_CPU_ARENA_INVALID;
        return;
    }
    if (!h->hhdm_offset || h->hhdm_offset > UINT64_MAX - plan.phys_base) {
        g_pi4_dma_probe_state = PI4_DMA_CPU_HHDM_INVALID;
        return;
    }
    if (h->hhdm_offset + plan.phys_base > UINT64_MAX - plan.size) {
        g_pi4_dma_probe_state = PI4_DMA_CPU_HHDM_INVALID;
        return;
    }

    __asm__ volatile("mrs %0, ctr_el0" : "=r"(ctr));
    line_bytes = 4ull << ((ctr >> 16) & 15u);
    if (line_bytes < 16 || line_bytes > plan.size / 2 ||
        line_bytes > 0x20000ull) {
        g_pi4_dma_probe_state = PI4_DMA_CPU_LINE_UNSUPPORTED;
        return;
    }
    arena_va = h->hhdm_offset + plan.phys_base;
    if (arena_va > UINT64_MAX - (line_bytes - 1)) {
        g_pi4_dma_probe_state = PI4_DMA_CPU_HHDM_INVALID;
        return;
    }
    first_va = (arena_va + line_bytes - 1) & ~(line_bytes - 1);
    offset = first_va - arena_va;
    if (offset > plan.size || line_bytes > (plan.size - offset) / 2) {
        g_pi4_dma_probe_state = PI4_DMA_CPU_ARENA_INVALID;
        return;
    }
    line[0] = (volatile uint8_t *)(uintptr_t)first_va;
    line[1] = (volatile uint8_t *)(uintptr_t)(first_va + line_bytes);
    g_pi4_dma_probe_line = line_bytes;

    for (unsigned n = 0; n < 2; n++) {
        for (uint64_t i = 0; i < line_bytes; i++)
            line[n][i] = (uint8_t)((i * 37u + n * 83u + 0x5au) ^ (i >> 3));
        __asm__ volatile("dc cvac, %0" :: "r"(line[n]) : "memory");
    }
    __asm__ volatile("dsb sy" ::: "memory");
    for (unsigned n = 0; n < 2; n++)
        __asm__ volatile("dc ivac, %0" :: "r"(line[n]) : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
    for (unsigned n = 0; n < 2; n++) {
        for (uint64_t i = 0; i < line_bytes; i++) {
            uint8_t want = (uint8_t)((i * 37u + n * 83u + 0x5au) ^ (i >> 3));
            if (line[n][i] != want) mismatch = 1;
        }
    }
    /* Leave the two reserved lines clean and predictable for later use. */
    for (unsigned n = 0; n < 2; n++) {
        for (uint64_t i = 0; i < line_bytes; i++) line[n][i] = 0;
        __asm__ volatile("dc cvac, %0" :: "r"(line[n]) : "memory");
    }
    __asm__ volatile("dsb sy" ::: "memory");
    g_pi4_dma_probe_state = mismatch ? PI4_DMA_CPU_READBACK_FAIL : PI4_DMA_CPU_PASS;
}

#define PI4_XHCI_DMA_NOT_RUN          0u
#define PI4_XHCI_DMA_SKIPPED_GATE     1u
#define PI4_XHCI_DMA_BAD_LAYOUT       2u
#define PI4_XHCI_DMA_MMIO_FAIL        3u
#define PI4_XHCI_DMA_PCI_ENABLE_FAIL  4u
#define PI4_XHCI_DMA_RUN_TIMEOUT      5u
#define PI4_XHCI_DMA_NOOP_TIMEOUT     6u
#define PI4_XHCI_DMA_BAD_EVENT        7u
#define PI4_XHCI_DMA_STOP_TIMEOUT     8u
#define PI4_XHCI_DMA_PCI_RESTORE_FAIL 9u
#define PI4_XHCI_DMA_NOOP_PASS        10u
#define PI4_XHCI_DMA_READBACK_FAIL    11u
#define PI4_XHCI_DMA_HSE_BEFORE_DB    12u
#define PI4_XHCI_DMA_HSE_AFTER_DB     13u
#define PI4_XHCI_DMA_INBOUND_CONFIG_FAIL 14u
#define PI4_XHCI_DMA_INBOUND_RESTORE_FAIL 15u
#define PI4_XHCI_DMA_NOOP_PAIR_PASS    16u
#define PI4_XHCI_DMA_SLOT_CYCLE_PASS   17u
#define PI4_XHCI_DMA_COMMAND_WRAP_PASS 18u
#define PI4_XHCI_DMA_ADDRESS_SKIP_PORT 19u
#define PI4_XHCI_DMA_ENABLE_SLOT_TIMEOUT 20u
#define PI4_XHCI_DMA_ENABLE_SLOT_BAD 21u
#define PI4_XHCI_DMA_ADDRESS_DEVICE_TIMEOUT 22u
#define PI4_XHCI_DMA_ADDRESS_DEVICE_BAD 23u
#define PI4_XHCI_DMA_ADDRESS_DEVICE_PASS 24u
#define PI4_XHCI_DMA_PROTOCOL_FAIL 25u
#define PI4_XHCI_DMA_ADDRESS_DEVICE_DEFAULT_PASS 26u
#define PI4_XHCI_DMA_DEVICE_DESCRIPTOR_PASS 27u
#define PI4_XHCI_DMA_CONTROL_TRANSFER_TIMEOUT 28u
#define PI4_XHCI_DMA_CONTROL_TRANSFER_BAD 29u
#define PI4_XHCI_DMA_CONFIG_HEADER_PASS 30u
#define PI4_XHCI_DMA_HUB_DESCRIPTOR_PASS 31u
#define PI4_XHCI_DMA_HUB_POWER_DELAY_FAIL 32u
#define PI4_XHCI_DMA_HUB_PORT_STATUS_FAIL 33u
#define PI4_XHCI_DMA_HUB_PORTS_PASS 34u
#define PI4_XHCI_DMA_HUB_CHILD_RESET_TIMEOUT 35u
#define PI4_XHCI_DMA_HUB_CHILD_RESET_BAD 36u
#define PI4_XHCI_DMA_HUB_CHILD_RESET_PASS 37u
#define PI4_XHCI_DMA_HUB_CHILD_NO_CONNECTED 38u
#define PI4_XHCI_DMA_HUB_CHILD_PORT_NOT_READY 39u
#define PI4_XHCI_DMA_HUB_CONFIG_TIMEOUT 40u
#define PI4_XHCI_DMA_HUB_CONFIG_BAD 41u
#define PI4_XHCI_DMA_HUB_CONTEXT_BAD 42u
#define PI4_XHCI_DMA_CHILD_ENABLE_TIMEOUT 43u
#define PI4_XHCI_DMA_CHILD_ENABLE_BAD 44u
#define PI4_XHCI_DMA_CHILD_ADDRESS_TIMEOUT 45u
#define PI4_XHCI_DMA_CHILD_ADDRESS_BAD 46u
#define PI4_XHCI_DMA_CHILD_ADDRESS_PASS 47u
#define PI4_XHCI_DMA_HUB_GUARD_TIMEOUT 48u
#define PI4_XHCI_DMA_HUB_GUARD_BAD 49u
#define PI4_XHCI_DMA_CHILD_DESCRIPTOR_PASS 50u
#define PI4_XHCI_DMA_CHILD_DESCRIPTOR_TIMEOUT 51u
#define PI4_XHCI_DMA_CHILD_DESCRIPTOR_BAD 52u
#define PI4_XHCI_DMA_CHILD_EP0_EVALUATE_TIMEOUT 53u
#define PI4_XHCI_DMA_CHILD_EP0_EVALUATE_BAD 54u
#define PI4_XHCI_DMA_CHILD_EP0_EVALUATE_PASS 55u
#define PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_TIMEOUT 56u
#define PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_BAD 57u
#define PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_PASS 58u
#define PI4_XHCI_DMA_CHILD_GET_STATUS_TIMEOUT 59u
#define PI4_XHCI_DMA_CHILD_GET_STATUS_BAD 60u
#define PI4_XHCI_DMA_CHILD_GET_STATUS_PASS 61u
#define PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_TIMEOUT 62u
#define PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_BAD 63u
#define PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_PASS 64u
#define PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_TIMEOUT 65u
#define PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_BAD 66u
#define PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_PASS 67u
#define PI4_XHCI_DMA_CHILD_CONFIG_HEADER_TIMEOUT 68u
#define PI4_XHCI_DMA_CHILD_CONFIG_HEADER_BAD 69u
#define PI4_XHCI_DMA_CHILD_CONFIG_HEADER_PASS 70u
#define PI4_XHCI_DMA_CHILD_CONFIG_FULL_TIMEOUT 71u
#define PI4_XHCI_DMA_CHILD_CONFIG_FULL_BAD 72u
#define PI4_XHCI_DMA_CHILD_CONFIG_FULL_PASS 73u
#define PI4_XHCI_DMA_CHILD_SET_CONFIG_TIMEOUT 74u
#define PI4_XHCI_DMA_CHILD_SET_CONFIG_BAD 75u
#define PI4_XHCI_DMA_CHILD_SET_CONFIG_PASS 76u
#define PI4_XHCI_DMA_CHILD_GET_CONFIG_TIMEOUT 77u
#define PI4_XHCI_DMA_CHILD_GET_CONFIG_BAD 78u
#define PI4_XHCI_DMA_CHILD_GET_CONFIG_PASS 79u
#define PI4_XHCI_DMA_CHILD_BULK_CONFIG_TIMEOUT 80u
#define PI4_XHCI_DMA_CHILD_BULK_CONFIG_BAD 81u
#define PI4_XHCI_DMA_CHILD_BULK_CONFIG_PASS 82u
#define PI4_XHCI_DMA_CHILD_BULK_SPEED_MISMATCH 83u
#define PI4_XHCI_DMA_CHILD_GET_MAX_LUN_TIMEOUT 84u
#define PI4_XHCI_DMA_CHILD_GET_MAX_LUN_BAD 85u
#define PI4_XHCI_DMA_CHILD_GET_MAX_LUN_PASS 86u
#define PI4_XHCI_DMA_ROOT_CONFIG_BAD 87u
#define PI4_XHCI_DMA_ROOT_NO_BOT 88u
#define PI4_XHCI_DMA_ROOT_SELECT_BAD 89u
#define PI4_XHCI_DMA_ROOT_MAX_LUN_BAD 90u
#define PI4_XHCI_DMA_ROOT_BOT_EP0_PASS 91u
#define PI4_XHCI_DMA_ROOT_BULK_CONFIG_BAD 92u
#define PI4_XHCI_DMA_ROOT_INQUIRY_BAD 93u
#define PI4_XHCI_DMA_ROOT_INQUIRY_PASS 94u
#define PI4_XHCI_DMA_ROOT_CAPACITY_BAD 95u
#define PI4_XHCI_DMA_ROOT_BLOCK_READ_BAD 96u
#define PI4_XHCI_DMA_ROOT_BLOCK_REPEAT_BAD 97u
#define PI4_XHCI_DMA_ROOT_BLOCK_READ_PASS 98u
#define PI4_XHCI_DMA_ROOT_FS_UNSUPPORTED 99u
#define PI4_XHCI_DMA_ROOT_FS_BAD 100u
#define PI4_XHCI_DMA_ROOT_FS_PASS 101u
#define PI4_XHCI_DMA_ROOT_FS_PARTIAL 102u
#define PI4_XHCI_DMA_ROOT_FILE_MISSING 103u
#define PI4_XHCI_DMA_ROOT_FILE_BAD 104u
#define PI4_XHCI_DMA_ROOT_FILE_PATTERN_BAD 105u
#define PI4_XHCI_DMA_ROOT_FILE_REPEAT_BAD 106u
#define PI4_XHCI_DMA_ROOT_FILE_PASS 107u
#define PI4_XHCI_DMA_LIVE_PASS 108u
#define PI4_XHCI_DMA_LIVE_BAD 109u
#define PI4_XHCI_DMA_KEYBOARD_UNSUPPORTED 110u
#define PI4_XHCI_DMA_KEYBOARD_BAD 111u
#define PI4_XHCI_DMA_KEYBOARD_READY 112u
#define PI4_XHCI_DMA_MOUSE_BAD 113u
#define PI4_XHCI_DMA_MOUSE_READY 114u
#define PI4_XHCI_DMA_MOUSE_UNSUPPORTED 115u
#define PI4_XHCI_DMA_BUS_BAD 116u
#define PI4_XHCI_DMA_BUS_READY 117u
#define PI4_XHCI_DMA_BUS_INPUT_ONLY 118u
#define PI4_XHCI_DMA_BUS_STORAGE_PENDING 119u

#define PI4_XHCI_WRAP_NOT_RUN         0u
#define PI4_XHCI_WRAP_EVENT_PASS      1u
#define PI4_XHCI_WRAP_NO_COUNTER      2u
#define PI4_XHCI_WRAP_MISSING_EVENT   3u
#define PI4_XHCI_WRAP_UNEXPECTED      4u
#define PI4_XHCI_WRAP_MMIO_FAIL       5u
#define PI4_XHCI_WRAP_NOT_OBSERVED    6u

#define PI4_XHCI_PORT_EVENT_NOT_RUN          0u
#define PI4_XHCI_PORT_EVENT_PASS             1u
#define PI4_XHCI_PORT_EVENT_NO_CONNECTED     2u
#define PI4_XHCI_PORT_EVENT_CLEAR_FAILED     3u
#define PI4_XHCI_PORT_EVENT_RESET_BUSY       4u
#define PI4_XHCI_PORT_EVENT_RESET_WRITE_FAIL 5u
#define PI4_XHCI_PORT_EVENT_TIMEOUT          6u
#define PI4_XHCI_PORT_EVENT_BAD_EVENT        7u
#define PI4_XHCI_PORT_EVENT_MMIO_FAIL        8u
#define PI4_XHCI_PORT_EVENT_NO_TIMER         9u

#define PI4_XHCI_HUB_CHILD_RESET_NOT_RUN        0u
#define PI4_XHCI_HUB_CHILD_RESET_NO_CONNECTED   1u
#define PI4_XHCI_HUB_CHILD_RESET_REQUEST_FAILED 2u
#define PI4_XHCI_HUB_CHILD_RESET_STATUS_TIMEOUT 3u
#define PI4_XHCI_HUB_CHILD_RESET_CLEAR_FAILED   4u
#define PI4_XHCI_HUB_CHILD_RESET_PASS           5u
#define PI4_XHCI_HUB_CHILD_RESET_BAD_STATUS     6u
#define PI4_XHCI_HUB_CHILD_RESET_PORT_NOT_READY 7u

static const char *pi4_xhci_dma_state_name(unsigned state) {
    switch (state) {
    case PI4_XHCI_DMA_SKIPPED_GATE: return "skipped-safety-gate";
    case PI4_XHCI_DMA_BAD_LAYOUT: return "invalid-layout";
    case PI4_XHCI_DMA_MMIO_FAIL: return "mmio-failed";
    case PI4_XHCI_DMA_PCI_ENABLE_FAIL: return "pci-enable-failed";
    case PI4_XHCI_DMA_RUN_TIMEOUT: return "run-timeout";
    case PI4_XHCI_DMA_NOOP_TIMEOUT: return "noop-timeout";
    case PI4_XHCI_DMA_BAD_EVENT: return "bad-completion-event";
    case PI4_XHCI_DMA_STOP_TIMEOUT: return "stop-timeout";
    case PI4_XHCI_DMA_PCI_RESTORE_FAIL: return "pci-restore-failed";
    case PI4_XHCI_DMA_NOOP_PASS: return "noop-pass";
    case PI4_XHCI_DMA_READBACK_FAIL: return "setup-readback-failed";
    case PI4_XHCI_DMA_HSE_BEFORE_DB: return "host-error-before-doorbell";
    case PI4_XHCI_DMA_HSE_AFTER_DB: return "host-error-after-doorbell";
    case PI4_XHCI_DMA_INBOUND_CONFIG_FAIL: return "inbound-config-failed";
    case PI4_XHCI_DMA_INBOUND_RESTORE_FAIL: return "inbound-restore-failed";
    case PI4_XHCI_DMA_NOOP_PAIR_PASS: return "noop-pair-pass";
    case PI4_XHCI_DMA_SLOT_CYCLE_PASS: return "slot-cycle-pass";
    case PI4_XHCI_DMA_COMMAND_WRAP_PASS: return "command-ring-wrap-pass";
    case PI4_XHCI_DMA_ADDRESS_SKIP_PORT: return "address-skipped-port-not-ready";
    case PI4_XHCI_DMA_ENABLE_SLOT_TIMEOUT: return "enable-slot-timeout";
    case PI4_XHCI_DMA_ENABLE_SLOT_BAD: return "enable-slot-failed";
    case PI4_XHCI_DMA_ADDRESS_DEVICE_TIMEOUT: return "address-device-timeout";
    case PI4_XHCI_DMA_ADDRESS_DEVICE_BAD: return "address-device-failed";
    case PI4_XHCI_DMA_ADDRESS_DEVICE_PASS: return "address-device-pass";
    case PI4_XHCI_DMA_ADDRESS_DEVICE_DEFAULT_PASS: return "address-device-default-pass";
    case PI4_XHCI_DMA_DEVICE_DESCRIPTOR_PASS: return "device-descriptor-prefix-pass";
    case PI4_XHCI_DMA_CONFIG_HEADER_PASS: return "config-header-pass";
    case PI4_XHCI_DMA_HUB_DESCRIPTOR_PASS: return "hub-descriptor-pass";
    case PI4_XHCI_DMA_HUB_POWER_DELAY_FAIL: return "hub-power-delay-failed";
    case PI4_XHCI_DMA_HUB_PORT_STATUS_FAIL: return "hub-port-status-failed";
    case PI4_XHCI_DMA_HUB_PORTS_PASS: return "hub-port-power-pass";
    case PI4_XHCI_DMA_HUB_CHILD_RESET_TIMEOUT: return "hub-child-reset-timeout";
    case PI4_XHCI_DMA_HUB_CHILD_RESET_BAD: return "hub-child-reset-failed";
    case PI4_XHCI_DMA_HUB_CHILD_RESET_PASS: return "hub-child-reset-pass";
    case PI4_XHCI_DMA_HUB_CHILD_NO_CONNECTED: return "hub-no-connected-child";
    case PI4_XHCI_DMA_HUB_CONFIG_TIMEOUT: return "hub-configure-timeout";
    case PI4_XHCI_DMA_HUB_CONFIG_BAD: return "hub-configure-failed";
    case PI4_XHCI_DMA_HUB_CONTEXT_BAD: return "hub-context-readback-failed";
    case PI4_XHCI_DMA_CHILD_ENABLE_TIMEOUT: return "child-enable-slot-timeout";
    case PI4_XHCI_DMA_CHILD_ENABLE_BAD: return "child-enable-slot-failed";
    case PI4_XHCI_DMA_CHILD_ADDRESS_TIMEOUT: return "child-address-timeout";
    case PI4_XHCI_DMA_CHILD_ADDRESS_BAD: return "child-address-failed";
    case PI4_XHCI_DMA_CHILD_ADDRESS_PASS: return "hub-child-address-pass";
    case PI4_XHCI_DMA_HUB_GUARD_TIMEOUT: return "hub-command-guard-timeout";
    case PI4_XHCI_DMA_HUB_GUARD_BAD: return "hub-command-guard-failed";
    case PI4_XHCI_DMA_CHILD_DESCRIPTOR_PASS:
        return "child-device-descriptor-prefix-pass";
    case PI4_XHCI_DMA_CHILD_DESCRIPTOR_TIMEOUT:
        return "child-device-descriptor-timeout";
    case PI4_XHCI_DMA_CHILD_DESCRIPTOR_BAD:
        return "child-device-descriptor-failed";
    case PI4_XHCI_DMA_CHILD_EP0_EVALUATE_TIMEOUT:
        return "child-ep0-evaluate-context-timeout";
    case PI4_XHCI_DMA_CHILD_EP0_EVALUATE_BAD:
        return "child-ep0-evaluate-context-failed";
    case PI4_XHCI_DMA_CHILD_EP0_EVALUATE_PASS:
        return "child-ep0-mps-evaluate-pass";
    case PI4_XHCI_DMA_CHILD_GET_STATUS_TIMEOUT:
        return "child-ep0-get-status-timeout";
    case PI4_XHCI_DMA_CHILD_GET_STATUS_BAD:
        return "child-ep0-get-status-failed";
    case PI4_XHCI_DMA_CHILD_GET_STATUS_PASS:
        return "child-ep0-get-status-pass";
    case PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_TIMEOUT:
        return "child-ep0-pre-evaluate-get-configuration-timeout";
    case PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_BAD:
        return "child-ep0-pre-evaluate-get-configuration-failed";
    case PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_PASS:
        return "child-ep0-pre-evaluate-get-configuration-pass";
    case PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_TIMEOUT:
        return "child-repeat-device-descriptor-timeout";
    case PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_BAD:
        return "child-repeat-device-descriptor-failed";
    case PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_PASS:
        return "child-repeat-device-descriptor-pass";
    case PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_TIMEOUT:
        return "child-device-descriptor-full-timeout";
    case PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_BAD:
        return "child-device-descriptor-full-failed";
    case PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_PASS:
        return "child-device-descriptor-full-pass";
    case PI4_XHCI_DMA_CHILD_CONFIG_HEADER_TIMEOUT:
        return "child-config-header-timeout";
    case PI4_XHCI_DMA_CHILD_CONFIG_HEADER_BAD:
        return "child-config-header-failed";
    case PI4_XHCI_DMA_CHILD_CONFIG_HEADER_PASS:
        return "child-config-header-pass";
    case PI4_XHCI_DMA_CHILD_CONFIG_FULL_TIMEOUT:
        return "child-config-descriptors-timeout";
    case PI4_XHCI_DMA_CHILD_CONFIG_FULL_BAD:
        return "child-config-descriptors-failed";
    case PI4_XHCI_DMA_CHILD_CONFIG_FULL_PASS:
        return "child-config-descriptors-pass";
    case PI4_XHCI_DMA_CHILD_SET_CONFIG_TIMEOUT:
        return "child-set-configuration-timeout";
    case PI4_XHCI_DMA_CHILD_SET_CONFIG_BAD:
        return "child-set-configuration-failed";
    case PI4_XHCI_DMA_CHILD_SET_CONFIG_PASS:
        return "child-set-configuration-pass";
    case PI4_XHCI_DMA_CHILD_GET_CONFIG_TIMEOUT:
        return "child-get-configuration-timeout";
    case PI4_XHCI_DMA_CHILD_GET_CONFIG_BAD:
        return "child-get-configuration-failed";
    case PI4_XHCI_DMA_CHILD_GET_CONFIG_PASS:
        return "child-get-configuration-pass";
    case PI4_XHCI_DMA_CHILD_BULK_CONFIG_TIMEOUT:
        return "child-bulk-endpoint-configure-timeout";
    case PI4_XHCI_DMA_CHILD_BULK_CONFIG_BAD:
        return "child-bulk-endpoint-configure-failed";
    case PI4_XHCI_DMA_CHILD_BULK_CONFIG_PASS:
        return "child-bulk-endpoint-contexts-pass";
    case PI4_XHCI_DMA_CHILD_BULK_SPEED_MISMATCH:
        return "child-bulk-speed-packet-mismatch";
    case PI4_XHCI_DMA_CHILD_GET_MAX_LUN_TIMEOUT:
        return "child-bot-get-max-lun-timeout";
    case PI4_XHCI_DMA_CHILD_GET_MAX_LUN_BAD:
        return "child-bot-get-max-lun-failed";
    case PI4_XHCI_DMA_CHILD_GET_MAX_LUN_PASS:
        return "child-bot-get-max-lun-pass";
    case PI4_XHCI_DMA_ROOT_CONFIG_BAD: return "direct-root-config-failed";
    case PI4_XHCI_DMA_ROOT_NO_BOT: return "direct-root-no-bot-interface";
    case PI4_XHCI_DMA_ROOT_SELECT_BAD: return "direct-root-select-config-failed";
    case PI4_XHCI_DMA_ROOT_MAX_LUN_BAD: return "direct-root-max-lun-failed";
    case PI4_XHCI_DMA_ROOT_BOT_EP0_PASS: return "direct-root-bot-ep0-pass";
    case PI4_XHCI_DMA_ROOT_BULK_CONFIG_BAD: return "direct-root-bulk-config-failed";
    case PI4_XHCI_DMA_ROOT_INQUIRY_BAD: return "direct-root-inquiry-failed";
    case PI4_XHCI_DMA_ROOT_INQUIRY_PASS: return "direct-root-inquiry-pass";
    case PI4_XHCI_DMA_ROOT_CAPACITY_BAD: return "direct-root-capacity-failed";
    case PI4_XHCI_DMA_ROOT_BLOCK_READ_BAD: return "direct-root-block-read-failed";
    case PI4_XHCI_DMA_ROOT_BLOCK_REPEAT_BAD: return "direct-root-block-repeat-failed";
    case PI4_XHCI_DMA_ROOT_BLOCK_READ_PASS: return "direct-root-block-read-pass";
    case PI4_XHCI_DMA_ROOT_FS_UNSUPPORTED: return "direct-root-filesystem-unsupported";
    case PI4_XHCI_DMA_ROOT_FS_BAD: return "direct-root-fat32-failed";
    case PI4_XHCI_DMA_ROOT_FS_PASS: return "direct-root-fat32-root-pass";
    case PI4_XHCI_DMA_ROOT_FS_PARTIAL: return "direct-root-fat32-root-bounded";
    case PI4_XHCI_DMA_ROOT_FILE_MISSING: return "direct-root-test-file-missing";
    case PI4_XHCI_DMA_ROOT_FILE_BAD: return "direct-root-file-read-failed";
    case PI4_XHCI_DMA_ROOT_FILE_PATTERN_BAD: return "direct-root-file-pattern-mismatch";
    case PI4_XHCI_DMA_ROOT_FILE_REPEAT_BAD: return "direct-root-file-repeat-mismatch";
    case PI4_XHCI_DMA_ROOT_FILE_PASS: return "direct-root-file-read-pass";
    case PI4_XHCI_DMA_LIVE_PASS: return "live-read-only-pass";
    case PI4_XHCI_DMA_LIVE_BAD: return "live-read-only-failed";
    case PI4_XHCI_DMA_KEYBOARD_UNSUPPORTED: return "child-boot-keyboard-not-found";
    case PI4_XHCI_DMA_KEYBOARD_BAD: return "child-boot-keyboard-failed";
    case PI4_XHCI_DMA_KEYBOARD_READY: return "child-boot-keyboard-online";
    case PI4_XHCI_DMA_MOUSE_BAD: return "child-boot-mouse-failed";
    case PI4_XHCI_DMA_MOUSE_READY: return "child-boot-mouse-online";
    case PI4_XHCI_DMA_MOUSE_UNSUPPORTED: return "child-boot-hid-not-found";
    case PI4_XHCI_DMA_BUS_BAD: return "usb2-multi-device-failed";
    case PI4_XHCI_DMA_BUS_READY: return "usb2-multi-device-online";
    case PI4_XHCI_DMA_BUS_INPUT_ONLY: return "usb2-input-online-storage-disabled";
    case PI4_XHCI_DMA_BUS_STORAGE_PENDING: return "usb2-input-online-storage-pending";
    case PI4_XHCI_DMA_CONTROL_TRANSFER_TIMEOUT: return "control-transfer-timeout";
    case PI4_XHCI_DMA_CONTROL_TRANSFER_BAD: return "control-transfer-failed";
    case PI4_XHCI_DMA_PROTOCOL_FAIL: return "port-protocol-capability-failed";
    default: return "not-run";
    }
}

static const char *pi4_xhci_dma_result(void) {
    return pi4_xhci_dma_state_name(g_pi4_xhci_dma_state);
}

static const char *pi4_xhci_dma_wrap_result(void) {
    switch (g_pi4_xhci_dma_wrap_state) {
    case PI4_XHCI_WRAP_EVENT_PASS: return "wrap-event-pass";
    case PI4_XHCI_WRAP_NO_COUNTER: return "mfindex-stalled";
    case PI4_XHCI_WRAP_MISSING_EVENT: return "wrap-event-missing";
    case PI4_XHCI_WRAP_UNEXPECTED: return "unexpected-event";
    case PI4_XHCI_WRAP_MMIO_FAIL: return "mmio-failed";
    case PI4_XHCI_WRAP_NOT_OBSERVED: return "wrap-not-observed";
    default: return "not-run";
    }
}

static const char *pi4_xhci_dma_port_event_result(void) {
    switch (g_pi4_xhci_dma_port_event_state) {
    case PI4_XHCI_PORT_EVENT_PASS: return "port-reset-event-pass";
    case PI4_XHCI_PORT_EVENT_NO_CONNECTED: return "no-connected-port";
    case PI4_XHCI_PORT_EVENT_CLEAR_FAILED: return "change-clear-failed";
    case PI4_XHCI_PORT_EVENT_RESET_BUSY: return "port-reset-busy";
    case PI4_XHCI_PORT_EVENT_RESET_WRITE_FAIL: return "port-reset-write-failed";
    case PI4_XHCI_PORT_EVENT_TIMEOUT: return "port-event-timeout";
    case PI4_XHCI_PORT_EVENT_BAD_EVENT: return "unexpected-event";
    case PI4_XHCI_PORT_EVENT_MMIO_FAIL: return "mmio-failed";
    case PI4_XHCI_PORT_EVENT_NO_TIMER: return "timer-unavailable";
    default: return "not-run";
    }
}

static const char *pi4_xhci_dma_hub_child_reset_result(void) {
    switch (g_pi4_xhci_dma_hub_child_reset_state) {
    case PI4_XHCI_HUB_CHILD_RESET_NO_CONNECTED: return "no-connected-port";
    case PI4_XHCI_HUB_CHILD_RESET_REQUEST_FAILED: return "request-failed";
    case PI4_XHCI_HUB_CHILD_RESET_STATUS_TIMEOUT: return "completion-timeout";
    case PI4_XHCI_HUB_CHILD_RESET_CLEAR_FAILED: return "change-clear-failed";
    case PI4_XHCI_HUB_CHILD_RESET_PASS: return "port-reset-pass";
    case PI4_XHCI_HUB_CHILD_RESET_BAD_STATUS: return "bad-final-status";
    case PI4_XHCI_HUB_CHILD_RESET_PORT_NOT_READY:
        return "connected-port-not-powered";
    default: return "not-run";
    }
}

static void pi4_dma_zero_bytes(uint8_t *p, uint64_t n) {
    while (n--) *p++ = 0;
}

static void pi4_dma_clean_bytes(uint8_t *p, uint64_t n) {
    uint64_t line = g_pi4_dma_probe_line;
    uintptr_t start, end;
    if (!line || (line & (line - 1)) || !n) return;
    start = (uintptr_t)p & ~(uintptr_t)(line - 1);
    end = (uintptr_t)p + (uintptr_t)n;
    for (uintptr_t a = start; a < end; a += (uintptr_t)line)
        __asm__ volatile("dc cvac, %0" :: "r"(a) : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
}

/* Discard CPU cache lines after a device has written them. Do not clean here:
 * a stale clean line must never be written back over newer DMA output. */
static void pi4_dma_invalidate_bytes(uint8_t *p, uint64_t n) {
    uint64_t line = g_pi4_dma_probe_line;
    uintptr_t start, end;
    if (!line || (line & (line - 1)) || !n) return;
    start = (uintptr_t)p & ~(uintptr_t)(line - 1);
    end = (uintptr_t)p + (uintptr_t)n;
    for (uintptr_t a = start; a < end; a += (uintptr_t)line)
        __asm__ volatile("dc ivac, %0" :: "r"(a) : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
}

static void pi4_dma_clean_invalidate_bytes(uint8_t *p, uint64_t n) {
    uint64_t line = g_pi4_dma_probe_line;
    uintptr_t start, end;
    if (!line || (line & (line - 1)) || !n) return;
    start = (uintptr_t)p & ~(uintptr_t)(line - 1);
    end = (uintptr_t)p + (uintptr_t)n;
    for (uintptr_t a = start; a < end; a += (uintptr_t)line)
        __asm__ volatile("dc civac, %0" :: "r"(a) : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
}

static int pi4_xhci_dma_wait_status(volatile uint8_t *op, unsigned offset,
                                    uint32_t mask, uint32_t expected,
                                    uint32_t *last) {
    uint64_t frequency, start, now;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(start));
    if (!frequency) return 0;
    do {
        uint32_t status;
        if (pi4_xhci_read_mmio32(op, offset, PI4_XHCI_MMIO_BYTES,
                                 &status) != 0) return 0;
        if (last) *last = status;
        if ((status & mask) == expected) return 1;
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
    } while (now - start < frequency);
    return 0;
}

static int pi4_xhci_dma_write64(volatile uint8_t *base, unsigned offset,
                                unsigned span, uint64_t value) {
    /* AC64 address fields use ordered low/high Dword writes. */
    if (g_pi4_xhci_caps[3] & 1u)
        return pi4_xhci_write_mmio64(base, offset, span, value) == 0;
    /* For a 32-bit-only xHC, the high half of address fields is unused. */
    return pi4_xhci_write_mmio32(base, offset, span, (uint32_t)value) == 0;
}

static int (*pi4_usb_event_sink)(uint64_t, uint32_t, uint32_t);
static int (*pi4_usb_bulk_stall_sink)(const struct pi4_usb_bus_stall *);
static int pi4_xhci_dma_wait_ms(uint32_t milliseconds);
static int pi4_xhci_dma_port_slot_type(volatile uint8_t *bar,
        unsigned root_port, unsigned max_ports, unsigned *protocol_major,
        unsigned *slot_type, unsigned *cap_offset);

/*
 * Controller HCRST can leave connected root ports disabled. Starting with all
 * ports disabled may stop MFINDEX, so exercise the event ring with a bounded
 * reset of the selected connected port. Allow at most one second for USB 3
 * link training before selecting a route. PRC/WRC must produce a type-34
 * Port Status Change Event; no slot or endpoint transfer is involved here.
 */
static int pi4_xhci_dma_port_reset_event_probe(
    volatile uint8_t *bar, volatile uint8_t *op, volatile uint8_t *rt,
    uint8_t *event_memory,
    uint64_t event_bus, struct pi4_xhci_dma_event_cursor *event_cursor) {
    uint64_t frequency, start, now;
    uint32_t total_ports, portsc = 0, offset, write_value, reset_change;
    unsigned port = 0;

    g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_NO_CONNECTED;
    g_pi4_xhci_dma_port_event_port = 0;
    g_pi4_xhci_dma_port_event_before = 0xffffffffu;
    g_pi4_xhci_dma_port_event_after = 0xffffffffu;
    g_pi4_xhci_dma_port_event_parameter = UINT64_MAX;
    g_pi4_xhci_dma_port_event_status = 0xffffffffu;
    g_pi4_xhci_dma_port_event_control = 0xffffffffu;

    total_ports = (g_pi4_xhci_caps[1] >> 24) & 0xffu;
    if (total_ports > PI4_XHCI_DIAG_PORTS) total_ports = PI4_XHCI_DIAG_PORTS;
    g_pi4_xhci_dma_root_port_count = total_ports;
    for (unsigned i = 0; i < total_ports; i++) {
        unsigned major = 0, slot_type = 0;
        g_pi4_xhci_dma_root_protocol[i] = 0;
        if (pi4_xhci_dma_port_slot_type(bar, i + 1u,
                (g_pi4_xhci_caps[1] >> 24) & 0xffu,
                &major, &slot_type, 0) == 0)
            g_pi4_xhci_dma_root_protocol[i] = major;
    }
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(start));
    for (unsigned scan = 0; scan <= 100u; scan++) {
        for (unsigned i = 0; i < total_ports; i++) {
            offset = PI4_XHCI_PORTSC_BASE + i * PI4_XHCI_PORTSC_STRIDE;
            if (pi4_xhci_read_mmio32(op, offset, PI4_XHCI_MMIO_BYTES,
                                     &portsc) != 0) {
                g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_MMIO_FAIL;
                return -1;
            }
            g_pi4_xhci_dma_root_portsc[i] = portsc;
            if (!scan) g_pi4_xhci_dma_root_initial[i] = portsc;
        }
        g_pi4_xhci_dma_root_scans = scan + 1u;
        port = pi4_xhci_dma_select_root_port(g_pi4_xhci_dma_root_portsc,
            g_pi4_xhci_dma_root_protocol, total_ports);
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        if (frequency && now - start <= UINT64_MAX / 1000u)
            g_pi4_xhci_dma_root_wait_ms =
                (unsigned)(((now - start) * 1000u) / frequency);
        if ((port && pi4_xhci_dma_superspeed_port_settled(
                g_pi4_xhci_dma_root_portsc[port - 1u],
                g_pi4_xhci_dma_root_protocol[port - 1u])) ||
            !frequency || now - start >= frequency || scan == 100u)
            break;
        if (!pi4_xhci_dma_wait_ms(10u)) break;
    }
    if (!port) return 0;
    portsc = g_pi4_xhci_dma_root_portsc[port - 1u];
    g_pi4_xhci_dma_port_event_port = port;
    g_pi4_xhci_dma_port_event_before = portsc;
    reset_change = g_pi4_xhci_dma_root_protocol[port - 1u] == 3u
        ? PI4_XHCI_PORTSC_WRC : pi4_xhci_dma_port_reset_change(portsc);

    offset = PI4_XHCI_PORTSC_BASE + (port - 1u) * PI4_XHCI_PORTSC_STRIDE;
    /* Clear only currently asserted RW1C changes, retaining port power. */
    for (unsigned attempt = 0; attempt < 4u; attempt++) {
        uint32_t clear = pi4_xhci_dma_portsc_clear_changes(portsc);
        if (pi4_xhci_write_mmio32(op, offset, PI4_XHCI_MMIO_BYTES,
                                  clear) != 0 ||
            pi4_xhci_read_mmio32(op, offset, PI4_XHCI_MMIO_BYTES,
                                 &portsc) != 0) {
            g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_MMIO_FAIL;
            return -1;
        }
        if (!(portsc & PI4_XHCI_PORTSC_CHANGE_MASK)) break;
    }
    if (portsc & PI4_XHCI_PORTSC_CHANGE_MASK) {
        g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_CLEAR_FAILED;
        g_pi4_xhci_dma_port_event_after = portsc;
        return 0;
    }
    if (pi4_xhci_dma_port_reset_value(portsc, &write_value) != 0) {
        g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_RESET_BUSY;
        g_pi4_xhci_dma_port_event_after = portsc;
        return 0;
    }
    if (reset_change == PI4_XHCI_PORTSC_WRC)
        write_value = (write_value & ~PI4_XHCI_PORTSC_PR) | PI4_XHCI_PORTSC_WPR;
    if (pi4_xhci_write_mmio32(op, offset, PI4_XHCI_MMIO_BYTES,
                              write_value) != 0 ||
        pi4_xhci_read_mmio32(op, offset, PI4_XHCI_MMIO_BYTES,
                             &g_pi4_xhci_dma_port_event_after) != 0) {
        g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_RESET_WRITE_FAIL;
        return 0;
    }
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(start));
    if (!frequency) {
        g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_NO_TIMER;
        return 0;
    }

    g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_TIMEOUT;
    for (;;) {
        uint8_t *entry = event_memory +
            (uint64_t)event_cursor->index * 16u;
        volatile uint32_t *event = (volatile uint32_t *)(void *)entry;
        uint32_t control;
        __asm__ volatile("dc ivac, %0" :: "r"(entry) : "memory");
        __asm__ volatile("dsb sy" ::: "memory");
        control = event[3];
        if (pi4_xhci_dma_event_ready(control, event_cursor)) {
            uint64_t parameter;
            uint32_t status;
            __asm__ volatile("dc ivac, %0" :: "r"(entry) : "memory");
            __asm__ volatile("dsb sy" ::: "memory");
            parameter = (uint64_t)event[0] | ((uint64_t)event[1] << 32);
            status = event[2];
            control = event[3];
            if (((control >> 10) & 0x3fu) == PI4_XHCI_TRB_TYPE_MFINDEX_EVT &&
                (status >> 24) == 1u) {
                g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_EVENT_PASS;
                g_pi4_xhci_dma_wrap_status = status;
                g_pi4_xhci_dma_wrap_control = control;
                g_pi4_xhci_dma_events++;
                (void)pi4_xhci_dma_event_cursor_advance(event_cursor);
                if (!pi4_xhci_dma_write64(rt, 0x38, PI4_XHCI_MMIO_BYTES,
                        pi4_xhci_dma_event_dequeue(event_bus,
                                                   event_cursor))) {
                    g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_MMIO_FAIL;
                    g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
                    return -1;
                }
                continue;
            }
            if (((control >> 10) & 0x3fu) != PI4_XHCI_TRB_TYPE_PSC_EVT ||
                (status >> 24) != 1u) {
                g_pi4_xhci_dma_port_event_parameter = parameter;
                g_pi4_xhci_dma_port_event_status = status;
                g_pi4_xhci_dma_port_event_control = control;
                g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_BAD_EVENT;
                g_pi4_xhci_dma_events++;
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_BAD_EVENT;
                return -1;
            }
            g_pi4_xhci_dma_events++;
            g_pi4_xhci_dma_port_event_parameter = parameter;
            g_pi4_xhci_dma_port_event_status = status;
            g_pi4_xhci_dma_port_event_control = control;
            (void)pi4_xhci_dma_event_cursor_advance(event_cursor);
            if (!pi4_xhci_dma_write64(rt, 0x38, PI4_XHCI_MMIO_BYTES,
                    pi4_xhci_dma_event_dequeue(event_bus, event_cursor))) {
                g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_MMIO_FAIL;
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
                return -1;
            }
            if (pi4_xhci_dma_port_event_ok(parameter, status, control, port)) {
                if (pi4_xhci_read_mmio32(op, offset, PI4_XHCI_MMIO_BYTES,
                        &g_pi4_xhci_dma_port_event_after) != 0) {
                    g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_MMIO_FAIL;
                    return -1;
                }
                if (g_pi4_xhci_dma_port_event_after & reset_change) {
                    g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_PASS;
                    /* PRC acknowledges reset completion, but device setup
                     * still needs a defined speed and an enabled U0 link.
                     * Allow a bounded settle interval before reporting it. */
                    uint64_t settle_start, settle_ticks = frequency / 10u;
                    if (!settle_ticks) settle_ticks = 1;
                    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(settle_start));
                    for (;;) {
                        if (pi4_xhci_dma_port_ready(
                                g_pi4_xhci_dma_port_event_after)) {
                            g_pi4_xhci_dma_port_ready = 1;
                            break;
                        }
                        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
                        if (now - settle_start >= settle_ticks) break;
                        if (pi4_xhci_read_mmio32(op, offset,
                                PI4_XHCI_MMIO_BYTES,
                                &g_pi4_xhci_dma_port_event_after) != 0) {
                            g_pi4_xhci_dma_port_event_state =
                                PI4_XHCI_PORT_EVENT_MMIO_FAIL;
                            g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
                            return -1;
                        }
                        __asm__ volatile("yield");
                    }
                    /* Clear PRC so the pending change state is acknowledged. */
                    if (pi4_xhci_write_mmio32(op, offset,
                        PI4_XHCI_MMIO_BYTES,
                        pi4_xhci_dma_portsc_clear_changes(
                            g_pi4_xhci_dma_port_event_after)) != 0) {
                        g_pi4_xhci_dma_port_event_state =
                            PI4_XHCI_PORT_EVENT_MMIO_FAIL;
                        g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
                        return -1;
                    }
                    return 1;
                }
            }
        }
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        if (now - start >= frequency) break;
        __asm__ volatile("yield");
    }
    if (pi4_xhci_read_mmio32(op, offset, PI4_XHCI_MMIO_BYTES,
                             &g_pi4_xhci_dma_port_event_after) != 0) {
        g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_MMIO_FAIL;
        return -1;
    }
    return 0;
}

/* Ring one already-published command and retire events for up to one second.
 * Return 1 for a successful command event, 0 for timeout, -1 for MMIO trouble,
 * and -2 for an unexpected event or unsuccessful completion code. */
static int pi4_xhci_dma_command_submit_wait(
    volatile uint8_t *op, volatile uint8_t *rt, volatile uint8_t *db,
    uint8_t *event_memory, uint64_t event_bus, uint64_t command_bus,
    unsigned command_index, unsigned expected_slot,
    struct pi4_xhci_dma_event_cursor *event_cursor,
    unsigned *returned_slot, uint32_t *completion_status,
    uint32_t *completion_control) {
    uint64_t frequency, start, now;
    uint32_t status, control;

    __asm__ volatile("dmb oshst\n\tdsb sy" ::: "memory");
    if (!event_cursor || !returned_slot || !completion_status ||
        !completion_control ||
        pi4_xhci_write_mmio32(db, 0, PI4_XHCI_MMIO_BYTES, 0) != 0 ||
        pi4_xhci_read_mmio32(db, 0, PI4_XHCI_MMIO_BYTES,
                             &g_pi4_xhci_dma_db_flush) != 0 ||
        pi4_xhci_read_mmio32(op, 4, PI4_XHCI_MMIO_BYTES,
                             &g_pi4_xhci_dma_doorbell_sts) != 0)
        return -1;
    if (g_pi4_xhci_dma_doorbell_sts & PI4_XHCI_USBSTS_HSE) return -2;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(start));
    if (!frequency) return 0;
    for (;;) {
        uint8_t *entry;
        volatile uint32_t *event;
        uint64_t parameter;
        entry = event_memory + (uint64_t)event_cursor->index * 16u;
        event = (volatile uint32_t *)(void *)entry;
        __asm__ volatile("dc ivac, %0" :: "r"(entry) : "memory");
        __asm__ volatile("dsb sy" ::: "memory");
        control = event[3];
        if (pi4_xhci_dma_event_ready(control, event_cursor)) {
            __asm__ volatile("dc ivac, %0" :: "r"(entry) : "memory");
            __asm__ volatile("dsb sy" ::: "memory");
            parameter = (uint64_t)event[0] | ((uint64_t)event[1] << 32);
            status = event[2];
            control = event[3];
            g_pi4_xhci_dma_events++;
            if (((control >> 10) & 0x3fu) == PI4_XHCI_TRB_TYPE_PSC_EVT) {
                (void)pi4_xhci_dma_event_cursor_advance(event_cursor);
                if (!pi4_xhci_dma_write64(rt, 0x38, PI4_XHCI_MMIO_BYTES,
                        pi4_xhci_dma_event_dequeue(event_bus,
                                                   event_cursor)))
                    return -1;
                continue;
            }
            if (pi4_usb_event_sink) {
                int handled = pi4_usb_event_sink(parameter, status, control);
                if (handled < 0) return -2;
                if (handled > 0) {
                    (void)pi4_xhci_dma_event_cursor_advance(event_cursor);
                    if (!pi4_xhci_dma_write64(rt, 0x38, PI4_XHCI_MMIO_BYTES,
                            pi4_xhci_dma_event_dequeue(event_bus, event_cursor))) return -1;
                    continue;
                }
            }
            if (!pi4_xhci_dma_command_event_matches(parameter, control,
                    command_bus + (uint64_t)command_index * 16u))
                return -2;
            *completion_status = status;
            *completion_control = control;
            *returned_slot = (control >> 24) & 0xffu;
            g_pi4_xhci_dma_event_status = status;
            g_pi4_xhci_dma_event_control = control;
            (void)pi4_xhci_dma_event_cursor_advance(event_cursor);
            if (!pi4_xhci_dma_write64(rt, 0x38, PI4_XHCI_MMIO_BYTES,
                    pi4_xhci_dma_event_dequeue(event_bus, event_cursor)))
                return -1;
            if ((status >> 24) != 1u ||
                (expected_slot != UINT32_MAX &&
                 *returned_slot != expected_slot))
                return -2;
            return 1;
        }
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        if (now - start >= frequency) return 0;
        __asm__ volatile("yield");
    }
}

/* Stop a stalled EP0 after its control-transfer wait expires. A Stop Endpoint
 * command reports any interrupted TD on the Transfer Event ring and commits
 * the endpoint's final dequeue pointer/DCS to the Output Endpoint Context.
 * Consume and record those events before accepting the matching command
 * completion. The wait is bounded to one architectural-counter second. */
static int pi4_xhci_dma_stop_endpoint_submit_wait(
    volatile uint8_t *op, volatile uint8_t *rt, volatile uint8_t *db,
    uint8_t *event_memory, uint64_t event_bus, uint64_t command_bus,
    unsigned command_index, unsigned slot_id, unsigned endpoint_id,
    struct pi4_xhci_dma_event_cursor *event_cursor) {
    uint64_t frequency, start, now;

    if (!op || !rt || !db || !event_memory || !event_cursor ||
        !slot_id || slot_id > 255u || !endpoint_id || endpoint_id > 31u ||
        command_index >= PI4_XHCI_DMA_RING_BYTES / 16u - 1u ||
        event_cursor->index >= PI4_XHCI_DMA_EVENT_TRBS ||
        event_cursor->cycle > 1u)
        return -1;
    __asm__ volatile("dmb oshst\n\tdsb sy" ::: "memory");
    if (pi4_xhci_write_mmio32(db, 0, PI4_XHCI_MMIO_BYTES, 0) != 0 ||
        pi4_xhci_read_mmio32(db, 0, PI4_XHCI_MMIO_BYTES,
                             &g_pi4_xhci_dma_db_flush) != 0 ||
        pi4_xhci_read_mmio32(op, 4, PI4_XHCI_MMIO_BYTES,
                             &g_pi4_xhci_dma_doorbell_sts) != 0)
        return -1;
    if (g_pi4_xhci_dma_doorbell_sts & PI4_XHCI_USBSTS_HSE) return -2;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(start));
    if (!frequency) return 0;
    for (;;) {
        uint8_t *entry = event_memory +
            (uint64_t)event_cursor->index * 16u;
        volatile uint32_t *event = (volatile uint32_t *)(void *)entry;
        uint64_t parameter;
        uint32_t status, control, type;

        __asm__ volatile("dc ivac, %0" :: "r"(entry) : "memory");
        __asm__ volatile("dsb sy" ::: "memory");
        control = event[3];
        if (pi4_xhci_dma_event_ready(control, event_cursor)) {
            __asm__ volatile("dc ivac, %0" :: "r"(entry) : "memory");
            __asm__ volatile("dsb sy" ::: "memory");
            parameter = (uint64_t)event[0] | ((uint64_t)event[1] << 32);
            status = event[2];
            control = event[3];
            type = (control >> 10) & 0x3fu;
            g_pi4_xhci_dma_events++;
            if (type == PI4_XHCI_TRB_TYPE_TRANSFER_EVT) {
                if (!g_pi4_xhci_dma_stop_endpoint_transfer_events) {
                    g_pi4_xhci_dma_stop_endpoint_transfer_parameter =
                        parameter;
                    g_pi4_xhci_dma_stop_endpoint_transfer_status = status;
                    g_pi4_xhci_dma_stop_endpoint_transfer_control = control;
                }
                g_pi4_xhci_dma_stop_endpoint_transfer_events++;
            } else if (type != PI4_XHCI_TRB_TYPE_PSC_EVT &&
                       type != PI4_XHCI_TRB_TYPE_MFINDEX_EVT &&
                       type != PI4_XHCI_TRB_TYPE_CMD_EVT) {
                g_pi4_xhci_dma_stop_endpoint_other_events++;
            }
            (void)pi4_xhci_dma_event_cursor_advance(event_cursor);
            if (!pi4_xhci_dma_write64(rt, 0x38, PI4_XHCI_MMIO_BYTES,
                    pi4_xhci_dma_event_dequeue(event_bus, event_cursor)))
                return -1;
            if (type == PI4_XHCI_TRB_TYPE_CMD_EVT &&
                pi4_xhci_dma_command_event_matches(parameter, control,
                    command_bus + (uint64_t)command_index * 16u)) {
                unsigned returned_slot = (control >> 24) & 0xffu;
                g_pi4_xhci_dma_stop_endpoint_status = status;
                g_pi4_xhci_dma_stop_endpoint_control = control;
                g_pi4_xhci_dma_event_status = status;
                g_pi4_xhci_dma_event_control = control;
                if ((status >> 24) == 1u && returned_slot == slot_id) {
                    g_pi4_xhci_dma_command_completions++;
                    return 1;
                }
                return -2;
            }
            if (type == PI4_XHCI_TRB_TYPE_CMD_EVT)
                g_pi4_xhci_dma_stop_endpoint_other_events++;
            continue;
        }
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        if (now - start >= frequency) return 0;
        __asm__ volatile("yield");
    }
}

/* Ring EP0 and look for its Transfer Event. Retire and record unrelated events
 * instead of mistaking (for example) a delayed Command Completion for the
 * transfer result. Polling remains bounded to one counter-second. */
static int pi4_xhci_dma_control_transfer_submit_wait(
    volatile uint8_t *op, volatile uint8_t *rt, volatile uint8_t *db,
    uint8_t *event_memory, uint64_t event_bus, uint64_t status_trb_bus,
    unsigned slot_id, unsigned endpoint_id,
    struct pi4_xhci_dma_event_cursor *event_cursor) {
    uint64_t frequency, start, now;
    uint32_t status, control;
    uint64_t parameter;
    uint32_t doorbell_offset;

    if (!op || !rt || !db || !event_memory || !event_cursor ||
        !slot_id || slot_id > 255u || !endpoint_id || endpoint_id > 31u ||
        event_cursor->index >= PI4_XHCI_DMA_EVENT_TRBS ||
        event_cursor->cycle > 1u ||
        slot_id > (PI4_XHCI_MMIO_BYTES - 4u) / 4u)
        return -1;
    g_pi4_xhci_dma_transfer_parameter = UINT64_MAX;
    g_pi4_xhci_dma_transfer_status = 0xffffffffu;
    g_pi4_xhci_dma_transfer_control = 0xffffffffu;
    g_pi4_xhci_dma_transfer_checks = 0;
    g_pi4_xhci_dma_transfer_skipped = 0;
    g_pi4_xhci_dma_transfer_skipped_parameter = UINT64_MAX;
    g_pi4_xhci_dma_transfer_skipped_status = 0xffffffffu;
    g_pi4_xhci_dma_transfer_skipped_control = 0xffffffffu;
    doorbell_offset = slot_id * 4u;
    __asm__ volatile("dmb oshst\n\tdsb sy" ::: "memory");
    if (pi4_xhci_write_mmio32(db, doorbell_offset, PI4_XHCI_MMIO_BYTES,
                              endpoint_id) != 0 ||
        pi4_xhci_read_mmio32(db, doorbell_offset, PI4_XHCI_MMIO_BYTES,
                             &g_pi4_xhci_dma_db_flush) != 0 ||
        pi4_xhci_read_mmio32(op, 4, PI4_XHCI_MMIO_BYTES,
                             &g_pi4_xhci_dma_doorbell_sts) != 0)
        return -1;
    if (g_pi4_xhci_dma_doorbell_sts & PI4_XHCI_USBSTS_HSE) return -2;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(start));
    if (!frequency) return 0;
    for (;;) {
        uint8_t *entry = event_memory +
            (uint64_t)event_cursor->index * 16u;
        volatile uint32_t *event = (volatile uint32_t *)(void *)entry;
        __asm__ volatile("dc ivac, %0" :: "r"(entry) : "memory");
        __asm__ volatile("dsb sy" ::: "memory");
        control = event[3];
        if (pi4_xhci_dma_event_ready(control, event_cursor)) {
            __asm__ volatile("dc ivac, %0" :: "r"(entry) : "memory");
            __asm__ volatile("dsb sy" ::: "memory");
            parameter = (uint64_t)event[0] | ((uint64_t)event[1] << 32);
            status = event[2];
            control = event[3];
            g_pi4_xhci_dma_events++;
            (void)pi4_xhci_dma_event_cursor_advance(event_cursor);
            if (!pi4_xhci_dma_write64(rt, 0x38, PI4_XHCI_MMIO_BYTES,
                    pi4_xhci_dma_event_dequeue(event_bus, event_cursor)))
                return -1;
            if (pi4_usb_event_sink) {
                int handled = pi4_usb_event_sink(parameter, status, control);
                if (handled < 0) return -2;
                if (handled > 0) continue;
            }
            if (((control >> 10) & 0x3fu) != PI4_XHCI_TRB_TYPE_TRANSFER_EVT) {
                if (!g_pi4_xhci_dma_transfer_skipped) {
                    g_pi4_xhci_dma_transfer_skipped_parameter = parameter;
                    g_pi4_xhci_dma_transfer_skipped_status = status;
                    g_pi4_xhci_dma_transfer_skipped_control = control;
                }
                g_pi4_xhci_dma_transfer_skipped++;
                continue;
            }
            g_pi4_xhci_dma_transfer_parameter = parameter;
            g_pi4_xhci_dma_transfer_status = status;
            g_pi4_xhci_dma_transfer_control = control;
            g_pi4_xhci_dma_transfer_checks =
                pi4_xhci_dma_transfer_event_check_mask(parameter, status,
                    control, status_trb_bus, slot_id, endpoint_id);
            if (g_pi4_xhci_dma_transfer_checks !=
                    PI4_XHCI_DMA_TRANSFER_CHECK_ALL)
                return -2;
            return pi4_xhci_dma_transfer_event_ok(parameter, status, control,
                status_trb_bus, slot_id, endpoint_id) ? 1 : -2;
        }
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        if (now - start >= frequency) return 0;
        __asm__ volatile("yield");
    }
}

/* Direct-root devices use their own EP0 sequence, with no USB 2 hub requests.
 * Slots/rings remain owned until the common halt/PCI-restore cleanup. */
static int pi4_xhci_dma_direct_root_request(
        volatile uint8_t *op, volatile uint8_t *rt, volatile uint8_t *db,
        uint8_t *events, uint64_t event_bus,
        struct pi4_xhci_dma_event_cursor *cursor, unsigned slot_id,
        uint8_t *ring, uint64_t ring_bus, unsigned trb_index,
        unsigned step, unsigned request_type, unsigned request,
        unsigned value, unsigned index, uint8_t *buffer,
        uint64_t buffer_bus, unsigned length) {
    uint32_t *setup = (uint32_t *)(void *)(ring + trb_index * 16u);
    uint32_t *data = setup + 4u;
    uint32_t *status = setup + (length ? 8u : 4u);
    int rc;
    if (!step || step > 4u) return -2;
    g_pi4_xhci_direct.step = step;
    g_pi4_xhci_direct.attempted |= 1u << (step - 1u);
    g_pi4_xhci_direct.rc[step - 1u] = (uint32_t)-2;
    if ((length && (!buffer || !(request_type & 0x80u))) ||
        pi4_xhci_dma_setup_stage_seed(setup, request_type, request,
            value, index, length, length ? 3u : 0u, 1u) != 0 ||
        (length && pi4_xhci_dma_data_stage_seed(data, buffer_bus,
                                                length, 1u) != 0) ||
        pi4_xhci_dma_status_stage_seed_direction(status,
                                                 length ? 0u : 1u, 1u) != 0)
        return -2;
    for (unsigned i = 0; i < length; i++) buffer[i] = 0xa5u;
    if (length) pi4_dma_clean_invalidate_bytes(buffer, length);
    pi4_dma_clean_bytes(ring, PI4_XHCI_DMA_RING_ALLOC_BYTES);
    g_pi4_xhci_direct.step = step;
    g_pi4_xhci_direct.status_bus = ring_bus +
        (uint64_t)(trb_index + (length ? 2u : 1u)) * 16u;
    rc = pi4_xhci_dma_control_transfer_submit_wait(op, rt, db, events,
        event_bus, g_pi4_xhci_direct.status_bus, slot_id, 1u, cursor);
    g_pi4_xhci_direct.rc[step - 1u] = (uint32_t)rc;
    g_pi4_xhci_direct.status[step - 1u] = g_pi4_xhci_dma_transfer_status;
    g_pi4_xhci_direct.checks[step - 1u] = g_pi4_xhci_dma_transfer_checks;
    g_pi4_xhci_direct.event_parameter = g_pi4_xhci_dma_transfer_parameter;
    g_pi4_xhci_direct.event_control = g_pi4_xhci_dma_transfer_control;
    if (rc > 0 && length) pi4_dma_invalidate_bytes(buffer, length);
    return rc;
}

static void pi4_xhci_dma_direct_root_config_probe(
        volatile uint8_t *op, volatile uint8_t *rt, volatile uint8_t *db,
        uint8_t *events, uint64_t event_bus,
        struct pi4_xhci_dma_event_cursor *cursor, unsigned slot_id,
        uint8_t *ring, uint64_t ring_bus, uint8_t *buffers,
        uint64_t buffers_bus) {
    unsigned length = g_pi4_xhci_dma_config_total_length;
    unsigned config_offset = PI4_XHCI_DMA_ROOT_CONFIG_OFFSET;
    unsigned value_offset = PI4_XHCI_DMA_ROOT_VALUE_OFFSET;
    unsigned lun_offset = PI4_XHCI_DMA_ROOT_LUN_OFFSET;
    struct pi4_xhci_dma_config_summary *summary = &g_pi4_xhci_direct.config;

    g_pi4_xhci_direct.step = 1u;
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_CONFIG_BAD;
    if (length < 9u || length > PI4_XHCI_DMA_ROOT_CONFIG_CAPACITY ||
        pi4_xhci_dma_direct_root_request(op, rt, db, events, event_bus,
            cursor, slot_id, ring, ring_bus, PI4_XHCI_DMA_ROOT_CONFIG_FULL_TRB,
            1u, 0x80u, 0x06u, 0x0200u, 0u, buffers + config_offset,
            buffers_bus + config_offset, length) <= 0 ||
        !pi4_xhci_dma_configuration_descriptor_ok(buffers + config_offset,
            length, PI4_XHCI_DMA_ROOT_CONFIG_CAPACITY, summary))
        return;
    if (!summary->bot_interface_found) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_NO_BOT;
        return;
    }
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_SELECT_BAD;
    if (pi4_xhci_dma_direct_root_request(op, rt, db, events, event_bus,
            cursor, slot_id, ring, ring_bus, PI4_XHCI_DMA_ROOT_SET_CONFIG_TRB,
            2u, 0u, 0x09u, summary->configuration_value, 0u, 0, 0, 0u) <= 0 ||
        pi4_xhci_dma_direct_root_request(op, rt, db, events, event_bus,
            cursor, slot_id, ring, ring_bus, PI4_XHCI_DMA_ROOT_GET_CONFIG_TRB,
            3u, 0x80u, 0x08u, 0u, 0u, buffers + value_offset,
            buffers_bus + value_offset, 1u) <= 0)
        return;
    g_pi4_xhci_direct.configuration = buffers[value_offset];
    if (g_pi4_xhci_direct.configuration != summary->configuration_value)
        return;
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_MAX_LUN_BAD;
    if (pi4_xhci_dma_direct_root_request(op, rt, db, events, event_bus,
            cursor, slot_id, ring, ring_bus, PI4_XHCI_DMA_ROOT_MAX_LUN_TRB,
            4u, 0xa1u, 0xfeu, 0u, summary->bot_interface_number,
            buffers + lun_offset, buffers_bus + lun_offset, 1u) <= 0)
        return;
    g_pi4_xhci_direct.max_lun = buffers[lun_offset];
    if (g_pi4_xhci_direct.max_lun > 15u) return;
    g_pi4_xhci_direct.step = 5u;
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_BOT_EP0_PASS;
}

/* A single bounded BOT phase. Reuse the event-ring/doorbell waiter, allowing
 * CC=Short Packet only on a data-IN phase and only after identity checks.
 * A registered physical-bus hook may recover an exact data-IN Stall Error
 * before CSW; all other failures use common halt/PCI restore cleanup. */
static int pi4_xhci_dma_bot_transfer(
        volatile uint8_t *op, volatile uint8_t *rt, volatile uint8_t *db,
        uint8_t *events, uint64_t event_bus,
        struct pi4_xhci_dma_event_cursor *cursor, unsigned slot_id,
        unsigned endpoint, uint8_t *ring, uint64_t ring_bus, unsigned trb_index,
        uint8_t *buffer, uint64_t buffer_bus, unsigned length,
        unsigned phase, unsigned direction_in, unsigned allow_short,
        struct pi4_xhci_dma_producer *producer,
        struct pi4_xhci_bot_io_report *report) {
    _Alignas(16) uint32_t words[4];
    uint32_t *trb = (uint32_t *)(void *)(ring + trb_index * 16u);
    unsigned actual = 0;
    int rc;
    if (!report || phase > 2u) return 0;
    report->phase = phase + 1u;
    if (pi4_xhci_dma_bulk_normal_seed(producer ? words : trb, buffer_bus, length,
                                      direction_in, producer ? producer->cycle : 1u) != 0)
        return 0;
    if (direction_in) {
        for (unsigned i = 0; i < length; i++) buffer[i] = 0xa5u;
        pi4_dma_clean_invalidate_bytes(buffer, length);
    } else {
        pi4_dma_clean_bytes(buffer, length);
    }
    if (producer) {
        if (pi4_xhci_dma_ring_enqueue(ring, ring_bus, producer, words,
                pi4_dma_clean_bytes, &trb_index) != 0) return 0;
    } else {
        pi4_dma_clean_bytes((uint8_t *)(void *)trb, 16u);
    }
    rc = pi4_xhci_dma_control_transfer_submit_wait(op, rt, db, events,
        event_bus, ring_bus + trb_index * 16u, slot_id, endpoint, cursor);
    report->rc[phase] = (uint32_t)rc;
    report->status[phase] = g_pi4_xhci_dma_transfer_status;
    report->control[phase] = g_pi4_xhci_dma_transfer_control;
    report->parameter[phase] = g_pi4_xhci_dma_transfer_parameter;
    report->checks[phase] = g_pi4_xhci_dma_transfer_checks;
    if ((rc != 1 && rc != -2) ||
        !pi4_xhci_dma_bulk_event_ok(report->parameter[phase],report->status[phase],
            report->control[phase],ring_bus+trb_index*16u,slot_id,endpoint,
            length,allow_short,&actual)) {
        struct pi4_usb_bus_stall plan;
        const uint32_t *failed=(const uint32_t *)(const void *)(ring+trb_index*16u);
        if(rc!=-2 || phase!=1u || !direction_in || !allow_short || !pi4_usb_bulk_stall_sink ||
            !pi4_usb_bus_stall_plan(report->parameter[phase],report->status[phase],
                report->control[phase],ring_bus,trb_index,slot_id,endpoint,length,
                failed[3]&1u,producer,&plan) || !pi4_usb_bulk_stall_sink(&plan))return 0;
        actual=plan.actual; /* Data phase terminated by STALL; proceed to CSW. */
    }
    report->actual[phase] = actual;
    if (direction_in) pi4_dma_invalidate_bytes(buffer, length);
    return 1;
}

static void pi4_xhci_dma_storage_probe(
        volatile uint8_t *op, volatile uint8_t *rt, volatile uint8_t *db,
        uint8_t *events, uint64_t event_bus,
        struct pi4_xhci_dma_event_cursor *cursor, unsigned slot_id,
        unsigned in_dci, unsigned out_dci,
        uint8_t *in_ring, uint64_t in_bus, uint8_t *out_ring, uint64_t out_bus,
        uint8_t *buffers, uint64_t buffers_bus);

/* Configure the direct SS bulk endpoints and retain the accepted INQUIRY
 * milestone before the separate read-only capacity/sector probe. */
static void pi4_xhci_dma_direct_root_bulk_probe(
        volatile uint8_t *op, volatile uint8_t *rt, volatile uint8_t *db,
        uint8_t *events, uint64_t event_bus,
        struct pi4_xhci_dma_event_cursor *cursor, unsigned slot_id,
        unsigned context_size, uint8_t *commands, uint64_t command_bus,
        uint8_t *input, uint64_t input_bus, uint8_t *output,
        uint8_t *in_ring, uint64_t in_bus, uint8_t *out_ring, uint64_t out_bus,
        uint8_t *buffers, uint64_t buffers_bus) {
    const struct pi4_xhci_dma_config_summary *s = &g_pi4_xhci_direct.config;
    const unsigned indices[2] = {PI4_XHCI_DMA_COMMAND_HUB_GUARD_NOOP_TRB,
                                 PI4_XHCI_DMA_COMMAND_CONFIGURE_HUB_TRB};
    uint8_t *cbw = buffers + PI4_XHCI_DMA_BOT_CBW_OFFSET;
    uint8_t *data = buffers + PI4_XHCI_DMA_BOT_DATA_OFFSET;
    uint8_t *csw = buffers + PI4_XHCI_DMA_BOT_CSW_OFFSET;
    unsigned in_dci = ((s->bot_bulk_in_address & 15u) * 2u) + 1u;
    unsigned out_dci = (s->bot_bulk_out_address & 15u) * 2u;

    g_pi4_xhci_bot.step = 1u;
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_BULK_CONFIG_BAD;
    if (!s->bot_interface_found || s->bot_bulk_in_max_burst > 15u ||
        s->bot_bulk_out_max_burst > 15u || s->bot_bulk_in_streams != 0u ||
        s->bot_bulk_out_streams != 0u ||
        pi4_xhci_dma_transfer_ring_seed(in_ring, in_bus) != 0 ||
        pi4_xhci_dma_transfer_ring_seed(out_ring, out_bus) != 0)
        return;
    pi4_dma_invalidate_bytes(output, PI4_XHCI_DMA_CONTEXT_BYTES);
    if (pi4_xhci_dma_bulk_contexts_seed(input, PI4_XHCI_DMA_CONTEXT_BYTES,
            context_size, output, PI4_XHCI_DMA_CONTEXT_BYTES,
            8u, 6u, 0x50u, s->bot_interface_alt, s->bot_interface_endpoint_count,
            s->bot_bulk_in_address, s->bot_bulk_in_attributes,
            s->bot_bulk_in_max_packet, s->bot_bulk_in_max_burst,
            s->bot_bulk_out_address, s->bot_bulk_out_attributes,
            s->bot_bulk_out_max_packet, s->bot_bulk_out_max_burst,
            4u, in_bus, out_bus) != 0)
        return;
    pi4_dma_clean_bytes(input, PI4_XHCI_DMA_CONTEXT_BYTES);
    pi4_dma_clean_bytes(in_ring, PI4_XHCI_DMA_RING_ALLOC_BYTES);
    pi4_dma_clean_bytes(out_ring, PI4_XHCI_DMA_RING_ALLOC_BYTES);
    for (unsigned i = 0; i < 2u; i++) {
        uint32_t *trb = (uint32_t *)(void *)(commands + indices[i] * 16u);
        unsigned returned_slot = 0;
        int rc;
        g_pi4_xhci_bot.step = 1u + i;
        if ((i == 0u ? pi4_xhci_dma_noop_seed(trb, 1u) :
                pi4_xhci_dma_configure_endpoint_seed(trb, input_bus, slot_id, 1u)) != 0)
            return;
        pi4_dma_clean_bytes((uint8_t *)(void *)trb, 16u);
        if (i) pi4_dma_clean_invalidate_bytes(output, PI4_XHCI_DMA_CONTEXT_BYTES);
        rc = pi4_xhci_dma_command_submit_wait(op, rt, db, events, event_bus,
            command_bus, indices[i], i ? slot_id : 0u, cursor, &returned_slot,
            &g_pi4_xhci_bot.command_status[i], &g_pi4_xhci_bot.command_control[i]);
        g_pi4_xhci_bot.command_rc[i] = (uint32_t)rc;
        if (rc <= 0) return;
        g_pi4_xhci_dma_command_completions++;
    }
    pi4_dma_invalidate_bytes(output, PI4_XHCI_DMA_CONTEXT_BYTES);
    for (unsigned i = 0; i < 4u; i++)
        g_pi4_xhci_bot.output_slot[i] = ((const uint32_t *)(const void *)output)[i];
    for (unsigned i = 0; i < 5u; i++) {
        g_pi4_xhci_bot.output_in[i] =
            ((const uint32_t *)(const void *)(output + in_dci * context_size))[i];
        g_pi4_xhci_bot.output_out[i] =
            ((const uint32_t *)(const void *)(output + out_dci * context_size))[i];
    }
    if (!pi4_xhci_dma_bulk_contexts_ok(output, PI4_XHCI_DMA_CONTEXT_BYTES,
            context_size, s->bot_bulk_in_address, s->bot_bulk_in_max_packet,
            s->bot_bulk_in_max_burst, in_bus, s->bot_bulk_out_address,
            s->bot_bulk_out_max_packet, s->bot_bulk_out_max_burst, out_bus))
        return;
    g_pi4_xhci_bot.verified = 1u;
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_INQUIRY_BAD;
    /* Configuration descriptor has been fully parsed; its DMA space can now
     * become three isolated BOT buffers without changing the summary. */
    g_pi4_xhci_bot.step = 3u;
    if (pi4_xhci_dma_bot_inquiry_cbw_seed(cbw, PI4_XHCI_DMA_BOT_CBW_BYTES,
            PI4_XHCI_DMA_BOT_INQUIRY_TAG) != 0 ||
        !pi4_xhci_dma_bot_transfer(op, rt, db, events, event_bus, cursor,
            slot_id, out_dci, out_ring, out_bus, 0u, cbw,
            buffers_bus + PI4_XHCI_DMA_BOT_CBW_OFFSET,
            PI4_XHCI_DMA_BOT_CBW_BYTES, 0u, 0u, 0u, 0, &g_pi4_xhci_bot.io))
        return;
    g_pi4_xhci_bot.step = 4u;
    if (!pi4_xhci_dma_bot_transfer(op, rt, db, events, event_bus, cursor,
            slot_id, in_dci, in_ring, in_bus, 0u, data,
            buffers_bus + PI4_XHCI_DMA_BOT_DATA_OFFSET,
            PI4_XHCI_DMA_BOT_INQUIRY_BYTES, 1u, 1u, 1u, 0, &g_pi4_xhci_bot.io))
        return;
    for (unsigned i = 0; i < PI4_XHCI_DMA_BOT_INQUIRY_BYTES; i++)
        g_pi4_xhci_bot.inquiry[i] = data[i];
    g_pi4_xhci_bot.step = 5u;
    if (!pi4_xhci_dma_bot_transfer(op, rt, db, events, event_bus, cursor,
            slot_id, in_dci, in_ring, in_bus, 1u, csw,
            buffers_bus + PI4_XHCI_DMA_BOT_CSW_OFFSET,
            PI4_XHCI_DMA_BOT_CSW_BYTES, 2u, 1u, 0u, 0, &g_pi4_xhci_bot.io))
        return;
    for (unsigned i = 0; i < PI4_XHCI_DMA_BOT_CSW_BYTES; i++)
        g_pi4_xhci_bot.csw[i] = csw[i];
    g_pi4_xhci_bot.csw_ok = pi4_xhci_dma_bot_csw_ok(csw,
        g_pi4_xhci_bot.io.actual[2], PI4_XHCI_DMA_BOT_INQUIRY_TAG,
        PI4_XHCI_DMA_BOT_INQUIRY_BYTES, g_pi4_xhci_bot.io.actual[1]);
    if (!g_pi4_xhci_bot.csw_ok ||
        !pi4_xhci_dma_bot_inquiry_ok(data, g_pi4_xhci_bot.io.actual[1]))
        return;
    pi4_xhci_dma_bot_identity_text(g_pi4_xhci_bot.vendor, data + 8u, 8u);
    pi4_xhci_dma_bot_identity_text(g_pi4_xhci_bot.product, data + 16u, 16u);
    pi4_xhci_dma_bot_identity_text(g_pi4_xhci_bot.revision, data + 32u, 4u);
    g_pi4_xhci_bot.step = 6u;
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_INQUIRY_PASS;
    pi4_xhci_dma_storage_probe(op, rt, db, events, event_bus, cursor, slot_id,
        in_dci, out_dci, in_ring, in_bus, out_ring, out_bus, buffers, buffers_bus);
}

/* Wait a descriptor-specified interval using the architectural counter,
 * rather than assuming a CPU-loop rate. */
static int pi4_xhci_dma_wait_ms(uint32_t milliseconds) {
    uint64_t frequency, start, now, ticks;
    if (!milliseconds) return 1;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    if (!frequency || frequency > UINT64_MAX / milliseconds) return 0;
    ticks = (frequency * milliseconds + 999u) / 1000u;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(start));
    for (;;) {
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        if (now - start >= ticks) return 1;
        __asm__ volatile("yield");
    }
}

struct pi4_xhci_bot_transport {
    volatile uint8_t *op, *rt, *db;
    uint8_t *events, *in_ring, *out_ring, *buffers;
    uint64_t event_bus, in_bus, out_bus, buffers_bus;
    struct pi4_xhci_dma_event_cursor *cursor;
    unsigned slot, in_dci, out_dci;
    struct pi4_xhci_dma_producer *in_producer, *out_producer;
    unsigned sequence;
    struct pi4_xhci_storage_command_report last;
};

/* Own the reserved arena, inbound translation and PCI BME for the lifetime of
 * this session. No stack-backed cursor or descriptor can outlive bootstrap. */
static struct {
    const struct zeal_handoff *handoff;
    struct pi4_xhci_bot_transport transport;
    struct pi4_xhci_dma_event_cursor events;
    struct pi4_xhci_dma_producer in, out, command;
    uint8_t *commands;
    uint64_t command_bus, next_poll, frequency;
    uint16_t original_pci;
    unsigned attempted, active, fault, stopped, boot_checks, idle_checks, kind;
    unsigned stage, command_rc, command_status, command_control;
    unsigned file_after_wrap;
} g_pi4_usb_live;

static struct {
    struct pi4_usb_keyboard_interface interface;
    struct pi4_usb_keyboard_state state;
    struct pi4_usb_mouse_state mouse_state;
    unsigned mouse, reported_activity;
    uint64_t report_at;
    struct pi4_xhci_dma_producer producer;
    uint8_t *ring, *report;
    uint64_t ring_bus, report_bus, pending, repeat_at;
    unsigned attempted, active, stage, fault, slot, dci, reports, keys;
    unsigned protocol, repeat_usage, request_rc[3], request_status[3];
    unsigned command_rc[2], command_status[2], last_status, last_control;
} g_pi4_keyboard;

static void pi4_usb_keyboard_emit_key(unsigned usage, int down) {
    unsigned code = usb_hid_key(usage);
    if (code) { input_usb_key(code, down); g_pi4_keyboard.keys++; }
    if (usage < 224u) {
        if (down) {
            uint64_t now;
            __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
            g_pi4_keyboard.repeat_usage = usage;
            g_pi4_keyboard.repeat_at = now + g_pi4_usb_live.frequency * 400u / 1000u;
        } else if (usage == g_pi4_keyboard.repeat_usage) {
            g_pi4_keyboard.repeat_usage = 0;
        }
    }
}

static struct pi4_xhci_storage_command_report *pi4_usb_last_command(
        struct pi4_xhci_bot_transport *t) {
    return t->in_producer ? &t->last :
        &g_pi4_xhci_storage.command[g_pi4_xhci_storage.count - 1u];
}

/* Append one read-only command after the INQUIRY's OUT[0], IN[0..1]. Bootstrap
 * retains up to 32 reports. The live session uses wrapping queues, increasing
 * tags, and one last-command report; it never indexes beyond that fixed log.
 * A transport error stops immediately; a valid CSW status 1 is returned for
 * the caller to diagnose with REQUEST SENSE. The optional bus hook may clear
 * a data-IN stall before CSW; no full BOT reset is issued here. */
static int pi4_xhci_dma_storage_command(struct pi4_xhci_bot_transport *t,
        unsigned opcode, uint32_t lba) {
    unsigned number = t->in_producer ? t->sequence : g_pi4_xhci_storage.count;
    unsigned bytes = opcode == 0x25u ? 8u :
        (opcode == 0x03u ? 18u : g_pi4_xhci_storage.block_bytes);
    struct pi4_xhci_storage_command_report *r;
    uint8_t *cbw = t->buffers + PI4_XHCI_DMA_BOT_CBW_OFFSET;
    uint8_t *data = t->buffers + PI4_XHCI_DMA_BOT_SECTOR_OFFSET;
    uint8_t *csw = t->buffers + PI4_XHCI_DMA_BOT_CSW_OFFSET;
    unsigned out_index = 1u + number, in_index = 2u + number * 2u;
    int rc;
    if ((!t->in_producer && number >= PI4_XHCI_DMA_BOT_STORAGE_COMMANDS) ||
        number >= UINT32_MAX - PI4_XHCI_DMA_BOT_INQUIRY_TAG - 1u ||
        (opcode != 0x25u && opcode != 0x28u && opcode != 0x03u))
        return 0;
    r = t->in_producer ? &t->last : &g_pi4_xhci_storage.command[number];
    pi4_dma_zero_bytes((uint8_t *)(void *)r, sizeof(*r));
    r->opcode = opcode; r->bytes = bytes; r->lba = lba;
    r->tag = PI4_XHCI_DMA_BOT_INQUIRY_TAG + number + 1u;
    r->scsi_status = UINT32_MAX;
    r->sense_key = r->sense_asc = r->sense_ascq = UINT32_MAX;
    for (unsigned i = 0; i < 3u; i++) {
        r->io.rc[i] = r->io.status[i] = r->io.control[i] = UINT32_MAX;
        r->io.parameter[i] = UINT64_MAX;
    }
    if (t->in_producer) { t->sequence++; out_index = in_index = 0u; }
    else g_pi4_xhci_storage.count++;
    if (opcode == 0x25u)
        rc = pi4_xhci_dma_bot_capacity10_cbw_seed(cbw,
            PI4_XHCI_DMA_BOT_CBW_BYTES, r->tag);
    else if (opcode == 0x03u)
        rc = pi4_xhci_dma_bot_sense_cbw_seed(cbw,
            PI4_XHCI_DMA_BOT_CBW_BYTES, r->tag);
    else
        rc = pi4_xhci_dma_bot_read10_cbw_seed(cbw,
            PI4_XHCI_DMA_BOT_CBW_BYTES, r->tag, lba,
            g_pi4_xhci_storage.last_lba, bytes);
    if (rc != 0 ||
        !pi4_xhci_dma_bot_transfer(t->op, t->rt, t->db, t->events, t->event_bus,
            t->cursor, t->slot, t->out_dci, t->out_ring, t->out_bus,
            out_index, cbw, t->buffers_bus + PI4_XHCI_DMA_BOT_CBW_OFFSET,
            PI4_XHCI_DMA_BOT_CBW_BYTES, 0u, 0u, 0u, t->out_producer, &r->io) ||
        !pi4_xhci_dma_bot_transfer(t->op, t->rt, t->db, t->events, t->event_bus,
            t->cursor, t->slot, t->in_dci, t->in_ring, t->in_bus,
            in_index, data, t->buffers_bus + PI4_XHCI_DMA_BOT_SECTOR_OFFSET,
            bytes, 1u, 1u, 1u, t->in_producer, &r->io) ||
        !pi4_xhci_dma_bot_transfer(t->op, t->rt, t->db, t->events, t->event_bus,
            t->cursor, t->slot, t->in_dci, t->in_ring, t->in_bus,
            in_index + 1u, csw, t->buffers_bus + PI4_XHCI_DMA_BOT_CSW_OFFSET,
            PI4_XHCI_DMA_BOT_CSW_BYTES, 2u, 1u, 0u, t->in_producer, &r->io))
        return 0;
    for (unsigned i = 0; i < PI4_XHCI_DMA_BOT_CSW_BYTES; i++) r->csw[i] = csw[i];
    r->transport_ok = pi4_xhci_dma_bot_csw_valid(csw, r->io.actual[2],
        r->tag, bytes, r->io.actual[1], &r->scsi_status);
    return r->transport_ok;
}

static int pi4_xhci_dma_storage_read_retry(struct pi4_xhci_bot_transport *t,
        unsigned opcode, uint32_t lba) {
    for (unsigned attempt = 0; attempt < 3u; attempt++) {
        struct pi4_xhci_storage_command_report *r;
        uint8_t *data = t->buffers + PI4_XHCI_DMA_BOT_SECTOR_OFFSET;
        if (!pi4_xhci_dma_storage_command(t, opcode, lba)) return 0;
        r = pi4_usb_last_command(t);
        if (r->scsi_status == 0u) return r->io.actual[1] == r->bytes;
        if (!pi4_xhci_dma_storage_command(t, 0x03u, 0u)) return 0;
        r = pi4_usb_last_command(t);
        if (r->scsi_status != 0u ||
            !pi4_xhci_dma_bot_sense_parse(data, r->io.actual[1],
                &r->sense_key, &r->sense_asc, &r->sense_ascq) ||
            !pi4_xhci_dma_bot_sense_retryable(r->sense_key, r->sense_asc, r->sense_ascq) ||
            attempt == 2u || !pi4_xhci_dma_wait_ms(100u))
            return 0;
    }
    return 0;
}

static int pi4_xhci_dma_fat32_read(void *opaque, uint32_t lba, uint8_t out[512]) {
    struct pi4_xhci_bot_transport *t = (struct pi4_xhci_bot_transport *)opaque;
    if (g_pi4_xhci_storage.block_bytes != 512u ||
        !pi4_xhci_dma_storage_read_retry(t, 0x28u, lba)) return 0;
    for (unsigned i = 0; i < 512u; i++)
        out[i] = t->buffers[PI4_XHCI_DMA_BOT_SECTOR_OFFSET + i];
    return 1;
}

/* Read only the explicitly named public test file. The host creates its
 * deterministic 1025-byte contents; the guest never modifies the stick. */
static void pi4_xhci_dma_fat32_file_probe(struct pi4_xhci_bot_transport *t) {
    g_pi4_fat32_file.attempted = 1u;
    const struct pi4_fat32_entry *file = pi4_fat32_find_file(&g_pi4_fat32,
                                                           PI4_FAT32_TEST_NAME);
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_FILE_MISSING;
    if (!file) {
        g_pi4_fat32_file.read[0].result = g_pi4_fat32.complete ?
            PI4_FAT32_FILE_NOT_FOUND : PI4_FAT32_FILE_ROOT_BOUNDED;
        return;
    }
    g_pi4_fat32_file.found = 1u;
    g_pi4_fat32_file.bytes = file->bytes;
    g_pi4_fat32_file.cluster = file->cluster;
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_FILE_PATTERN_BAD;
    if (file->bytes != PI4_FAT32_TEST_BYTES) return;
    for (unsigned pass = 0; pass < 2u; pass++) {
        struct pi4_fat32_file_report *r = &g_pi4_fat32_file.read[pass];
        uint8_t *out = pass ? g_pi4_fat32_file.second : g_pi4_fat32_file.first;
        for (unsigned i = 0; i < PI4_FAT32_FILE_MAX_BYTES; i++) out[i] = pass ? 0x5au : 0xa5u;
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_FILE_BAD;
        pi4_fat32_read_file(&g_pi4_fat32.volume, file, pi4_xhci_dma_fat32_read,
            t, out, PI4_FAT32_FILE_MAX_BYTES, r);
        if (r->result != PI4_FAT32_FILE_READ_PASS) return;
        g_pi4_fat32_file.crc[pass] = pi4_xhci_dma_crc32(out, r->bytes);
        g_pi4_fat32_file.pattern_ok[pass] = pi4_fat32_test_match(out, r->bytes) &&
            g_pi4_fat32_file.crc[pass] == PI4_FAT32_TEST_CRC32;
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_FILE_PATTERN_BAD;
        if (!g_pi4_fat32_file.pattern_ok[pass]) return;
    }
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_FILE_REPEAT_BAD;
    for (unsigned i = 0; i < file->bytes; i++)
        if (g_pi4_fat32_file.first[i] != g_pi4_fat32_file.second[i]) return;
    g_pi4_fat32_file.repeat_ok = 1u;
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_FILE_PASS;
}

static void pi4_xhci_dma_storage_probe(
        volatile uint8_t *op, volatile uint8_t *rt, volatile uint8_t *db,
        uint8_t *events, uint64_t event_bus,
        struct pi4_xhci_dma_event_cursor *cursor, unsigned slot_id,
        unsigned in_dci, unsigned out_dci,
        uint8_t *in_ring, uint64_t in_bus, uint8_t *out_ring, uint64_t out_bus,
        uint8_t *buffers, uint64_t buffers_bus) {
    struct pi4_xhci_bot_transport t = {
        .op = op, .rt = rt, .db = db, .events = events, .event_bus = event_bus,
        .cursor = cursor, .slot = slot_id, .in_dci = in_dci, .out_dci = out_dci,
        .in_ring = in_ring, .in_bus = in_bus, .out_ring = out_ring, .out_bus = out_bus,
        .buffers = buffers, .buffers_bus = buffers_bus
    };
    uint8_t *data = buffers + PI4_XHCI_DMA_BOT_SECTOR_OFFSET;
    g_pi4_xhci_storage.step = 1u;
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_CAPACITY_BAD;
    if (!pi4_xhci_dma_storage_read_retry(&t, 0x25u, 0u) ||
        !pi4_xhci_dma_bot_capacity10_parse(data, 8u,
            &g_pi4_xhci_storage.last_lba, &g_pi4_xhci_storage.block_bytes,
            &g_pi4_xhci_storage.capacity_bytes))
        return;
    for (unsigned read = 0; read < 2u; read++) {
        unsigned bytes = g_pi4_xhci_storage.block_bytes;
        g_pi4_xhci_storage.step = 2u + read;
        g_pi4_xhci_dma_state = read ? PI4_XHCI_DMA_ROOT_BLOCK_REPEAT_BAD :
                                    PI4_XHCI_DMA_ROOT_BLOCK_READ_BAD;
        if (!pi4_xhci_dma_storage_read_retry(&t, 0x28u, 0u)) return;
        g_pi4_xhci_storage.crc[read] = pi4_xhci_dma_crc32(data, bytes);
        g_pi4_xhci_storage.reads++;
        if (!read) {
            for (unsigned i = 0; i < bytes; i++) g_pi4_xhci_storage.first_block[i] = data[i];
            g_pi4_xhci_storage.signature = data[510] | ((uint32_t)data[511] << 8);
            g_pi4_xhci_storage.first_word = data[0] | ((uint32_t)data[1] << 8) |
                ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
        } else {
            /* Compare every byte, rather than relying on a checksum match. */
            for (unsigned i = 0; i < bytes; i++)
                if (data[i] != g_pi4_xhci_storage.first_block[i]) return;
        }
    }
    g_pi4_xhci_storage.repeat_ok = 1u;
    g_pi4_xhci_storage.step = 4u;
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_ROOT_BLOCK_READ_PASS;
    pi4_fat32_probe(g_pi4_xhci_storage.first_block, g_pi4_xhci_storage.last_lba,
        g_pi4_xhci_storage.block_bytes, pi4_xhci_dma_fat32_read, &t, &g_pi4_fat32);
    g_pi4_xhci_dma_state = g_pi4_fat32.result == PI4_FAT32_UNSUPPORTED ?
        PI4_XHCI_DMA_ROOT_FS_UNSUPPORTED :
        (g_pi4_fat32.result == PI4_FAT32_ROOT_PASS ? PI4_XHCI_DMA_ROOT_FS_PASS :
         (g_pi4_fat32.result == PI4_FAT32_ROOT_PARTIAL ? PI4_XHCI_DMA_ROOT_FS_PARTIAL :
                                                     PI4_XHCI_DMA_ROOT_FS_BAD));
    if (g_pi4_fat32.result == PI4_FAT32_ROOT_PASS ||
        g_pi4_fat32.result == PI4_FAT32_ROOT_PARTIAL)
        pi4_xhci_dma_fat32_file_probe(&t);
    if (g_pi4_xhci_dma_state == PI4_XHCI_DMA_ROOT_FILE_PASS) {
        /* INQUIRY consumed OUT[0], IN[0..1]; every later completed command
         * consumed one OUT and two IN slots. Adopt those exact empty queues. */
        /* Large structure assignment may emit libc memcpy in this
         * freestanding image; copy only the initialized transport handles. */
        struct pi4_xhci_bot_transport *live = &g_pi4_usb_live.transport;
        live->op = t.op; live->rt = t.rt; live->db = t.db;
        live->events = t.events; live->event_bus = t.event_bus;
        live->in_ring = t.in_ring; live->in_bus = t.in_bus;
        live->out_ring = t.out_ring; live->out_bus = t.out_bus;
        live->buffers = t.buffers; live->buffers_bus = t.buffers_bus;
        live->slot = t.slot; live->in_dci = t.in_dci; live->out_dci = t.out_dci;
        g_pi4_usb_live.events = *cursor;
        g_pi4_usb_live.transport.cursor = &g_pi4_usb_live.events;
        g_pi4_usb_live.transport.sequence = g_pi4_xhci_storage.count;
        g_pi4_usb_live.in = (struct pi4_xhci_dma_producer){
            2u + 2u * g_pi4_xhci_storage.count, 1u, 0u};
        g_pi4_usb_live.out = (struct pi4_xhci_dma_producer){
            1u + g_pi4_xhci_storage.count, 1u, 0u};
        g_pi4_usb_live.transport.in_producer = &g_pi4_usb_live.in;
        g_pi4_usb_live.transport.out_producer = &g_pi4_usb_live.out;
    }
}

/* One health transaction: command-ring No-op, then an exact repeat of the
 * accepted sector 0. Every phase has a one-second deadline; no guest writes. */
static int pi4_usb_live_check(void) {
    struct pi4_xhci_bot_transport *t = &g_pi4_usb_live.transport;
    _Alignas(16) uint32_t words[4];
    unsigned index, returned_slot;
    g_pi4_usb_live.stage = 1u;
    if (pi4_xhci_dma_noop_seed(words, g_pi4_usb_live.command.cycle) != 0 ||
        pi4_xhci_dma_ring_enqueue(g_pi4_usb_live.commands, g_pi4_usb_live.command_bus,
            &g_pi4_usb_live.command, words, pi4_dma_clean_bytes, &index) != 0)
        return 0;
    int rc = pi4_xhci_dma_command_submit_wait(t->op, t->rt, t->db, t->events,
        t->event_bus, g_pi4_usb_live.command_bus, index, 0u, t->cursor,
        &returned_slot, &g_pi4_usb_live.command_status, &g_pi4_usb_live.command_control);
    g_pi4_usb_live.command_rc = (unsigned)rc;
    if (rc != 1) return 0;
    g_pi4_xhci_dma_command_completions++;
    g_pi4_usb_live.stage = 2u;
    if (!pi4_xhci_dma_storage_read_retry(t, 0x28u, 0u)) return 0;
    g_pi4_usb_live.stage = 3u;
    for (unsigned i = 0; i < g_pi4_xhci_storage.block_bytes; i++)
        if (t->buffers[PI4_XHCI_DMA_BOT_SECTOR_OFFSET + i] !=
                g_pi4_xhci_storage.first_block[i]) return 0;
    g_pi4_xhci_dma_event_cursor_index = t->cursor->index;
    g_pi4_xhci_dma_event_cursor_cycle = t->cursor->cycle;
    g_pi4_usb_live.stage = 4u;
    return 1;
}

/* No arena is freed or reused on either success or failure. Restore the
 * bridge only after both a halted xHC and cleared PCI BME have been verified. */
static void pi4_usb_live_stop(void) {
    if (!g_pi4_usb_live.active) return;
    pi4_usb_event_sink = 0;
    pi4_usb_bulk_stall_sink = 0;
    pi4_usb_bus_release();
    if (g_pi4_keyboard.active) {
        g_pi4_keyboard.mouse_state.buttons = 0;
        const uint8_t released[8] = {0};
        g_pi4_keyboard.active = 0;
        (void)pi4_usb_keyboard_decode(&g_pi4_keyboard.state, released, 8u,
                                      pi4_usb_keyboard_emit_key);
        g_pi4_keyboard.repeat_usage = 0;
    }
    const struct zeal_handoff *h = g_pi4_usb_live.handoff;
    uint32_t verify;
    int halted = pi4_xhci_write_mmio32(g_pi4_usb_live.transport.op, 0,
        PI4_XHCI_MMIO_BYTES, 0) == 0 &&
        pi4_xhci_dma_wait_status(g_pi4_usb_live.transport.op, 4,
            PI4_XHCI_USBSTS_HALTED, PI4_XHCI_USBSTS_HALTED,
            &g_pi4_xhci_dma_status);
    int bme_off = pi4_pcie_write_config32(h, g_pi4_link_status, 0x04,
        g_pi4_usb_live.original_pci) == 0 &&
        pi4_pcie_read_config32(h, g_pi4_link_status, 0x04, &verify) == 0 &&
        (uint16_t)verify == g_pi4_usb_live.original_pci &&
        !(verify & PI4_PCI_COMMAND_MASTER);
    g_pi4_usb_live.active = 0;
    if (bme_off) g_pi4_cfg_command = verify;
    if (!halted || !bme_off) {
        g_pi4_usb_live.fault = 2u;
        g_pi4_xhci_dma_bar2_state |= 1u << 31;
        g_pi4_xhci_dma_state = halted ? PI4_XHCI_DMA_PCI_RESTORE_FAIL :
                                      PI4_XHCI_DMA_STOP_TIMEOUT;
        return;
    }
    g_pi4_xhci_dma_bar2_state |= 1u << 3;
    if (pi4_pcie_write_mmio32(h, PI4_PCIE_INBOUND_BAR2_HI,
            g_pi4_xhci_dma_bar2_hi_original) != 0 ||
        pi4_pcie_read_mmio32(h, PI4_PCIE_INBOUND_BAR2_HI, &verify) != 0 ||
        verify != g_pi4_xhci_dma_bar2_hi_original) {
        g_pi4_usb_live.fault = 3u;
        g_pi4_xhci_dma_bar2_state |= 1u << 31;
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_INBOUND_RESTORE_FAIL;
        return;
    }
    g_pi4_xhci_dma_bar2_hi_restored = verify;
    g_pi4_xhci_dma_bar2_state |= 1u << 4;
    g_pi4_usb_live.stopped = 1u;
}

/* Called only after the accepted known-file test and fully drained BOT TDs.
 * Exercise both producer cycles and >4 event laps before retaining RUN/BME. */
static int pi4_usb_live_start(const struct zeal_handoff *h, uint8_t *commands,
        uint64_t command_bus, uint16_t original_pci) {
    g_pi4_usb_live.attempted = 1u;
    g_pi4_usb_live.handoff = h;
    g_pi4_usb_live.commands = commands;
    g_pi4_usb_live.command_bus = command_bus;
    g_pi4_usb_live.original_pci = original_pci;
    g_pi4_usb_live.command = (struct pi4_xhci_dma_producer){252u, 1u, 0u};
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(g_pi4_usb_live.frequency));
    if (!g_pi4_usb_live.frequency ||
        pi4_xhci_dma_ring_skipped_prefix(commands,
            PI4_XHCI_DMA_COMMAND_ENABLE_SLOT_TRB, 1u, pi4_dma_clean_bytes) != 0)
        return 0;
    for (unsigned i = 0; i < 272u; i++) {
        if (!pi4_usb_live_check()) return 0;
        g_pi4_usb_live.boot_checks++;
    }
    g_pi4_usb_live.stage = 5u;
    pi4_xhci_dma_fat32_file_probe(&g_pi4_usb_live.transport);
    if (g_pi4_xhci_dma_state != PI4_XHCI_DMA_ROOT_FILE_PASS ||
        g_pi4_usb_live.command.wraps < 2u || g_pi4_usb_live.in.wraps < 2u ||
        g_pi4_usb_live.out.wraps < 1u || g_pi4_xhci_dma_events < 1024u)
        return 0;
    g_pi4_usb_live.file_after_wrap = 1u;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(g_pi4_usb_live.next_poll));
    g_pi4_usb_live.next_poll += g_pi4_usb_live.frequency;
    g_pi4_usb_live.stage = 6u;
    g_pi4_usb_live.kind = 1u;
    g_pi4_usb_live.active = 1u;
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_LIVE_PASS;
    return 1;
}

/* Shell-side service, independent of serial input. A fault stops future I/O
 * and follows the same halt/BME-off/translation-restore ordering. */
static int pi4_usb_live_poll(void) {
    uint64_t now;
    if (!g_pi4_usb_live.active || g_pi4_usb_live.kind != 1u) return 0;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
    if (now < g_pi4_usb_live.next_poll) return 0;
    if (!pi4_usb_live_check()) {
        g_pi4_usb_live.fault = 1u;
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_LIVE_BAD;
        pi4_usb_live_stop();
        return -1;
    }
    g_pi4_usb_live.next_poll = now + g_pi4_usb_live.frequency;
    g_pi4_usb_live.idle_checks++;
    return g_pi4_usb_live.idle_checks == 10u ? 1 : 0;
}

static int pi4_usb_keyboard_queue(void) {
    struct pi4_xhci_bot_transport *t = &g_pi4_usb_live.transport;
    _Alignas(16) uint32_t words[4];
    unsigned index;
    for (unsigned i = 0; i < 64u; i++) g_pi4_keyboard.report[i] = 0xa5u;
    pi4_dma_clean_invalidate_bytes(g_pi4_keyboard.report, 64u);
    if (pi4_xhci_dma_bulk_normal_seed(words, g_pi4_keyboard.report_bus,
            g_pi4_keyboard.mouse ? g_pi4_keyboard.interface.packet : 8u,
            1u, g_pi4_keyboard.producer.cycle) != 0 ||
        pi4_xhci_dma_ring_enqueue(g_pi4_keyboard.ring, g_pi4_keyboard.ring_bus,
            &g_pi4_keyboard.producer, words, pi4_dma_clean_bytes, &index) != 0)
        return 0;
    g_pi4_keyboard.pending = g_pi4_keyboard.ring_bus + index * 16u;
    return pi4_xhci_write_mmio32(t->db, g_pi4_keyboard.slot * 4u,
        PI4_XHCI_MMIO_BYTES, g_pi4_keyboard.dci) == 0 &&
        pi4_xhci_read_mmio32(t->db, g_pi4_keyboard.slot * 4u,
            PI4_XHCI_MMIO_BYTES, &g_pi4_xhci_dma_db_flush) == 0;
}

static int pi4_usb_keyboard_ready(void) {
    return !g_pi4_keyboard.mouse && g_pi4_keyboard.active && g_pi4_usb_live.active;
}
static int pi4_usb_mouse_ready(void) {
    return g_pi4_keyboard.mouse && g_pi4_keyboard.active && g_pi4_usb_live.active;
}
static int pi4_usb_mouse_sample(uint32_t w, uint32_t h, uint32_t *x, uint32_t *y,
                                int *buttons, int *wheel) {
    pi4_usb_keyboard_poll();
    if (!pi4_usb_mouse_ready() || !w || !h || !x || !y) return 0;
    struct pi4_usb_mouse_state *m = &g_pi4_keyboard.mouse_state;
    *x = pi4_usb_mouse_clamp(m->x, w); *y = pi4_usb_mouse_clamp(m->y, h);
    if (buttons) *buttons = (int)m->buttons;
    if (wheel) *wheel = 0; /* Device-specific boot bytes have no standard wheel. */
    return 1;
}
/* One selected USB2 child owns this event consumer: keyboard OR boot mouse. */
static void pi4_usb_keyboard_poll(void) {
    if (!g_pi4_keyboard.active || !g_pi4_usb_live.active) return;
    struct pi4_xhci_bot_transport *t = &g_pi4_usb_live.transport;
    uint32_t controller_status;
    unsigned fault = 0;
    if (pi4_xhci_read_mmio32(t->op, 4, PI4_XHCI_MMIO_BYTES,
            &controller_status) != 0 ||
        (controller_status & (PI4_XHCI_USBSTS_HALTED | PI4_XHCI_USBSTS_HSE)))
        fault = 1u;
    for (unsigned work = 0; !fault && work < PI4_XHCI_DMA_EVENT_TRBS; work++) {
        uint8_t *entry = t->events + g_pi4_usb_live.events.index * 16u;
        volatile uint32_t *event = (volatile uint32_t *)(void *)entry;
        pi4_dma_invalidate_bytes(entry, 16u);
        uint32_t control = event[3];
        if (!pi4_xhci_dma_event_ready(control, &g_pi4_usb_live.events)) break;
        __asm__ volatile("dmb oshld" ::: "memory");
        uint64_t parameter = (uint64_t)event[0] | (uint64_t)event[1] << 32;
        uint32_t status = event[2];
        g_pi4_xhci_dma_events++;
        g_pi4_keyboard.last_status = status; g_pi4_keyboard.last_control = control;
        (void)pi4_xhci_dma_event_cursor_advance(&g_pi4_usb_live.events);
        g_pi4_xhci_dma_event_cursor_index = g_pi4_usb_live.events.index;
        g_pi4_xhci_dma_event_cursor_cycle = g_pi4_usb_live.events.cycle;
        if (!pi4_xhci_dma_write64(t->rt, 0x38, PI4_XHCI_MMIO_BYTES,
                pi4_xhci_dma_event_dequeue(t->event_bus, &g_pi4_usb_live.events))) {
            fault = 2u; break;
        }
        if (((control >> 10) & 63u) == PI4_XHCI_TRB_TYPE_PSC_EVT) continue;
        unsigned actual;
        int valid = g_pi4_keyboard.mouse ?
            pi4_usb_mouse_event_ok(parameter, status, control,
                g_pi4_keyboard.pending, g_pi4_keyboard.slot, g_pi4_keyboard.dci,
                g_pi4_keyboard.interface.packet, &actual) :
            (pi4_xhci_dma_bulk_event_ok(parameter, status, control,
                g_pi4_keyboard.pending, g_pi4_keyboard.slot, g_pi4_keyboard.dci,
                8u, 1u, &actual) && actual == 8u);
        if (!g_pi4_keyboard.pending || !valid) {
            fault = 3u; break;
        }
        g_pi4_keyboard.pending = 0;
        pi4_dma_invalidate_bytes(g_pi4_keyboard.report, 64u);
        g_pi4_keyboard.reports++;
        int decoded = g_pi4_keyboard.mouse ?
            pi4_usb_mouse_decode(&g_pi4_keyboard.mouse_state, g_pi4_keyboard.report,
                actual, (uint32_t)g_fb_w, (uint32_t)g_fb_h) :
            pi4_usb_keyboard_decode(&g_pi4_keyboard.state, g_pi4_keyboard.report,
                8u, pi4_usb_keyboard_emit_key) >= 0;
        if (!decoded || !pi4_usb_keyboard_queue()) {
            fault = 4u; break;
        }
    }
    if (fault) {
        g_pi4_keyboard.fault = fault;
        g_pi4_xhci_dma_state = g_pi4_keyboard.mouse ?
            PI4_XHCI_DMA_MOUSE_BAD : PI4_XHCI_DMA_KEYBOARD_BAD;
        pi4_usb_live_stop();
        pi_diag_fb_puts("\nUSB HID stopped: fault/status/control=");
        pi_diag_fb_u32_hex(fault); pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_keyboard.last_status); pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_keyboard.last_control); pi_diag_fb_puts("\n");
        uart_puts(g_uart, "\nPi USB HID stopped: fault=");
        uart_put_u32_dec(g_uart, fault); uart_puts(g_uart, "\n");
        return;
    }
    uint64_t now;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
    if (!g_pi4_keyboard.mouse && g_pi4_keyboard.repeat_usage && now >= g_pi4_keyboard.repeat_at) {
        unsigned code = usb_hid_key(g_pi4_keyboard.repeat_usage);
        if (code) { input_usb_key(code, 1); g_pi4_keyboard.keys++; }
        g_pi4_keyboard.repeat_at = now + g_pi4_usb_live.frequency * 35u / 1000u;
    }
}

static int pi4_usb_keyboard_control(volatile uint8_t *op, volatile uint8_t *rt,
        volatile uint8_t *db, uint8_t *events, uint64_t event_bus,
        struct pi4_xhci_dma_event_cursor *cursor, unsigned slot, uint8_t *ep0,
        uint64_t ep0_bus, uint8_t *buffer, uint64_t buffer_bus, unsigned step) {
    const unsigned positions[3] = {26u, 28u, 31u};
    if (step >= 3u) return 0;
    unsigned length = step == 1u ? 1u : 0u;
    uint32_t *setup = (uint32_t *)(void *)(ep0 + positions[step] * 16u);
    uint32_t *status = setup + (length ? 8u : 4u);
    if (pi4_usb_keyboard_setup_seed(setup, step,
            g_pi4_keyboard.interface.interface, 1u) != 0 ||
        (length && pi4_xhci_dma_data_stage_seed(setup + 4u, buffer_bus, 1u, 1u) != 0) ||
        pi4_xhci_dma_status_stage_seed_direction(status, !length, 1u) != 0)
        return 0;
    *buffer = 0xa5u;
    if (length) pi4_dma_clean_invalidate_bytes(buffer, 64u);
    pi4_dma_clean_bytes(ep0, PI4_XHCI_DMA_RING_ALLOC_BYTES);
    int rc = pi4_xhci_dma_control_transfer_submit_wait(op, rt, db, events,
        event_bus, ep0_bus + (positions[step] + (length ? 2u : 1u)) * 16u,
        slot, 1u, cursor);
    g_pi4_keyboard.request_rc[step] = (unsigned)rc;
    g_pi4_keyboard.request_status[step] = g_pi4_xhci_dma_transfer_status;
    if (rc != 1) return 0;
    if (length) {
        pi4_dma_invalidate_bytes(buffer, 64u);
        g_pi4_keyboard.protocol = *buffer;
        if (*buffer != 0u) return 0;
    }
    return 1;
}

static int pi4_usb_keyboard_start(const struct zeal_handoff *h,
        volatile uint8_t *op, volatile uint8_t *rt, volatile uint8_t *db,
        uint8_t *events, uint64_t event_bus,
        struct pi4_xhci_dma_event_cursor *cursor, unsigned slot, unsigned size,
        uint8_t *commands, uint64_t command_bus, uint8_t *input, uint64_t input_bus,
        uint8_t *output, uint8_t *ep0, uint64_t ep0_bus, uint8_t *ring,
        uint64_t ring_bus, uint8_t *buffers, uint64_t buffers_bus,
        uint16_t original_pci) {
    g_pi4_xhci_dma_state = g_pi4_keyboard.mouse ?
        PI4_XHCI_DMA_MOUSE_BAD : PI4_XHCI_DMA_KEYBOARD_BAD;
    g_pi4_keyboard.protocol = UINT32_MAX;
    /* SET_IDLE is optional for a mouse and must not stall its EP0. */
    for (unsigned step = 0; step < (g_pi4_keyboard.mouse ? 2u : 3u); step++) {
        g_pi4_keyboard.stage = step + 1u;
        if (!pi4_usb_keyboard_control(op, rt, db, events, event_bus, cursor,
                slot, ep0, ep0_bus, buffers + 3072u, buffers_bus + 3072u, step)) return 0;
    }
    g_pi4_keyboard.stage = 4u;
    pi4_dma_invalidate_bytes(output, PI4_XHCI_DMA_CONTEXT_BYTES);
    int context_rc = g_pi4_keyboard.mouse ?
        pi4_usb_mouse_context_seed(input, PI4_XHCI_DMA_CONTEXT_BYTES, size,
            output, PI4_XHCI_DMA_CONTEXT_BYTES, g_pi4_xhci_dma_child_speed_id,
            &g_pi4_keyboard.interface, ring_bus) :
        pi4_usb_keyboard_context_seed(input, PI4_XHCI_DMA_CONTEXT_BYTES, size,
            output, PI4_XHCI_DMA_CONTEXT_BYTES, g_pi4_xhci_dma_child_speed_id,
            &g_pi4_keyboard.interface, ring_bus);
    if (context_rc != 0 ||
        pi4_xhci_dma_transfer_ring_seed(ring, ring_bus) != 0 ||
        /* Bootstrap has traversed command Link 255. Empty prefix must be
         * invalid for CCS=0, including entries after Configure Endpoint. */
        pi4_xhci_dma_ring_skipped_prefix(commands, 248u, 1u, pi4_dma_clean_bytes) != 0)
        return 0;
    pi4_dma_clean_bytes(input, PI4_XHCI_DMA_CONTEXT_BYTES);
    pi4_dma_clean_bytes(ring, PI4_XHCI_DMA_RING_ALLOC_BYTES);
    struct pi4_xhci_dma_producer producer = {0u, 0u, 1u};
    for (unsigned i = 0; i < 2u; i++) {
        _Alignas(16) uint32_t words[4];
        unsigned index, returned_slot;
        uint32_t control;
        pi4_dma_clean_invalidate_bytes(output, PI4_XHCI_DMA_CONTEXT_BYTES);
        if ((i ? pi4_xhci_dma_configure_endpoint_seed(words, input_bus, slot, 0u) :
                 pi4_xhci_dma_noop_seed(words, 0u)) != 0 ||
            pi4_xhci_dma_ring_enqueue(commands, command_bus, &producer, words,
                pi4_dma_clean_bytes, &index) != 0) return 0;
        int rc = pi4_xhci_dma_command_submit_wait(op, rt, db, events, event_bus,
            command_bus, index, i ? slot : 0u, cursor, &returned_slot,
            &g_pi4_keyboard.command_status[i], &control);
        g_pi4_keyboard.command_rc[i] = (unsigned)rc;
        if (rc != 1) return 0;
        g_pi4_xhci_dma_command_completions++;
    }
    g_pi4_keyboard.stage = 5u;
    pi4_dma_invalidate_bytes(output, PI4_XHCI_DMA_CONTEXT_BYTES);
    if (!pi4_usb_keyboard_context_ok(output, PI4_XHCI_DMA_CONTEXT_BYTES, size,
            g_pi4_xhci_dma_child_speed_id, &g_pi4_keyboard.interface, ring_bus)) return 0;
    g_pi4_usb_live.handoff = h; g_pi4_usb_live.original_pci = original_pci;
    g_pi4_usb_live.events = *cursor;
    g_pi4_usb_live.command = producer;
    struct pi4_xhci_bot_transport *t = &g_pi4_usb_live.transport;
    t->op = op; t->rt = rt; t->db = db; t->events = events;
    t->event_bus = event_bus; t->cursor = &g_pi4_usb_live.events;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(g_pi4_usb_live.frequency));
    if (!g_pi4_usb_live.frequency) return 0;
    g_pi4_keyboard.ring = ring; g_pi4_keyboard.ring_bus = ring_bus;
    g_pi4_keyboard.report = buffers + 3072u;
    g_pi4_keyboard.report_bus = buffers_bus + 3072u;
    g_pi4_keyboard.slot = slot;
    g_pi4_keyboard.dci = (g_pi4_keyboard.interface.endpoint & 15u) * 2u + 1u;
    g_pi4_keyboard.producer = (struct pi4_xhci_dma_producer){0u, 1u, 0u};
    g_pi4_keyboard.stage = 6u;
    if (g_pi4_keyboard.mouse) {
        if (!g_fb_w || !g_fb_h) return 0;
        g_pi4_keyboard.mouse_state.x = (uint32_t)g_fb_w / 2u;
        g_pi4_keyboard.mouse_state.y = (uint32_t)g_fb_h / 2u;
    }
    if (!pi4_usb_keyboard_queue()) return 0;
    g_pi4_keyboard.active = g_pi4_usb_live.active = 1u;
    g_pi4_usb_live.kind = 2u;
    g_pi4_keyboard.stage = 7u;
    g_pi4_xhci_dma_state = g_pi4_keyboard.mouse ?
        PI4_XHCI_DMA_MOUSE_READY : PI4_XHCI_DMA_KEYBOARD_READY;
    return 1;
}

#include "pi4_usb_bus.inc"

/* Resolve the protocol-specific Slot Type for the root port that reported
 * connection. The Supported Protocol capability is read-only MMIO; the
 * chain is bounded to the xHCI extended-capability aperture and 64 entries. */
static int pi4_xhci_dma_port_slot_type(volatile uint8_t *bar,
        unsigned root_port, unsigned max_ports, unsigned *protocol_major,
        unsigned *slot_type, unsigned *cap_offset) {
    uint32_t offset = ((g_pi4_xhci_caps[3] >> 16) & 0xffffu) * 4u;
    if (!bar || !offset || !root_port || !max_ports ||
        offset < (g_pi4_xhci_caps[0] & 0xffu) ||
        offset > PI4_XHCI_EXT_CAP_BYTES - 4u)
        return -1;
    for (unsigned i = 0; i < 64u; i++) {
        uint32_t header, port_range, slot_word, next_offset;
        int rc;
        if (pi4_xhci_read_mmio32(bar, offset, PI4_XHCI_MMIO_BYTES,
                                 &header) != 0)
            return -1;
        if ((header & 0xffu) == PI4_XHCI_EXT_CAP_SUPPORTED_PROTOCOL) {
            if (offset > PI4_XHCI_EXT_CAP_BYTES - 16u ||
                pi4_xhci_read_mmio32(bar, offset + 8u,
                    PI4_XHCI_MMIO_BYTES, &port_range) != 0 ||
                pi4_xhci_read_mmio32(bar, offset + 12u,
                    PI4_XHCI_MMIO_BYTES, &slot_word) != 0)
                return -1;
            rc = pi4_xhci_dma_protocol_cap_for_port(header, port_range,
                    slot_word, root_port, max_ports, protocol_major,
                    slot_type);
            if (rc < 0) return -1;
            if (rc > 0) {
                if (cap_offset) *cap_offset = offset;
                return 0;
            }
        }
        rc = pi4_xhci_dma_ext_cap_next(offset, header,
                PI4_XHCI_EXT_CAP_BYTES, &next_offset);
        if (rc <= 0) return -1;
        offset = next_offset;
    }
    return -1;
}

/*
 * Physical DMA bootstrap: configure a six-slot-capacity xHC with a
 * reserved DCBAA, command ring, event ring and ERST; enable PCI BME only after
 * those structures are clean; issue Enable Slot at a 64-byte-aligned CRCR
 * pointer, then Address Device with an EP0-only context before the Link TRB.
 * This isolates command/context acceptance from the already-verified ring
 * wrap path; issue consecutive EP0 device/configuration descriptor reads,
 * configure the hub, and read its class descriptor. The USB2 fallback and
 * every failed bootstrap halt and restore the original PCI command word.
 * A separate trial maps BAR2 to the 16-GiB PCIe alias and restores its
 * original high word after xHCI is stopped. The USB test reads the full
 * 18-byte device descriptor, a 9-byte configuration header, SET_CONFIGURATION,
 * a 9-byte Hub descriptor, then issues bounded downstream port power/status
 * requests. A later bounded request resets one connected, powered hub child,
 * marks the hub Slot Context, assigns that child's USB address, reads and
 * repeats its first eight Device Descriptor bytes to test EP0 continuation,
 * updates Full-Speed EP0 MPS with Evaluate Context, and reads the complete
 * 18-byte descriptor, and reads the child's complete configuration chain.
 * It then selects that configuration, verifies GET_CONFIGURATION, and adds
 * the Mass Storage BOT bulk endpoint contexts through Configure Endpoint.
 * The USB2 fallback stops at contexts; the direct SuperSpeed path additionally
 * submits read-only INQUIRY, capacity, sector and FAT32/file requests. A passing
 * known-file test can transfer ownership to the static LIVE-READ-2 session,
 * after multi-ring wrap validation. A USB2 boot-keyboard child instead starts
 * its own persistent interrupt-IN session after HID class setup succeeds.
 */
static void pi4_xhci_dma_noop_probe(const struct zeal_handoff *h) {
    struct pi4_dma_probe_plan arena, alias_arena;
    struct pi4_xhci_dma_layout layout;
    volatile uint8_t *bar, *op, *rt, *db;
    uint8_t *memory, *event_memory, *command_memory;
    uint8_t *input_context, *output_context, *ep0_ring, *control_buffer = 0;
    uint8_t *child_output_context, *child_ep0_ring;
    uint8_t *child_bulk_in_ring, *child_bulk_out_ring;
    uint64_t dcbaa_bus, scratch_array_bus, command_bus, event_bus, erst_bus;
    uint64_t input_context_bus, output_context_bus, ep0_ring_bus;
    uint64_t control_buffer_bus, child_output_bus, child_ep0_bus;
    uint64_t child_bulk_in_bus, child_bulk_out_bus;
    uint64_t frequency, start, now;
    uint32_t current_command, verify, page_size, usbcmd, usbsts, iman;
    uint32_t cap_length, db_offset, rt_offset, control, status;
    uint16_t original_pci, mmio_pci, enabled_pci;
    uint32_t *erst;
    uint64_t *dcbaa, *scratch_array;
    uint32_t rb_iman = 0xffffffffu, rb_imod = 0xffffffffu;
    uint32_t rb_config = 0xffffffffu, rb_erstsz = 0xffffffffu;
    uint32_t rb_lo = 0xffffffffu, rb_hi = 0xffffffffu;
    uint64_t rb_dcbaa = UINT64_MAX, rb_erstba = UINT64_MAX;
    uint64_t rb_erdp = UINT64_MAX;
    int pci_mse_may_be_set = 0;
    int bme_may_be_set = 0, cleanup_ok = 1;
    int bar2_may_be_set = 0;
    int plan_rc;
    struct pi4_xhci_dma_event_cursor event_cursor = {0u, 1u};
    unsigned slot_id = 0;
    unsigned child_slot_id = 0;

    if (g_pi4_usb_live.active) return;
    pi4_usb_event_sink = 0;
    pi4_usb_bulk_stall_sink = 0;
    pi4_dma_zero_bytes((uint8_t *)(void *)&g_pi4_bus, sizeof(g_pi4_bus));
    pi4_dma_zero_bytes((uint8_t *)(void *)&g_pi4_usb_live, sizeof(g_pi4_usb_live));
    pi4_dma_zero_bytes((uint8_t *)(void *)&g_pi4_keyboard, sizeof(g_pi4_keyboard));
    g_pi4_xhci_dma_root_port_count = 0;
    g_pi4_xhci_dma_root_scans = 0;
    g_pi4_xhci_dma_root_wait_ms = 0;
    pi4_dma_zero_bytes((uint8_t *)(void *)&g_pi4_xhci_direct,
                       sizeof(g_pi4_xhci_direct));
    g_pi4_xhci_direct.configuration = UINT32_MAX;
    g_pi4_xhci_direct.max_lun = UINT32_MAX;
    pi4_dma_zero_bytes((uint8_t *)(void *)&g_pi4_xhci_bot, sizeof(g_pi4_xhci_bot));
    pi4_dma_zero_bytes((uint8_t *)(void *)&g_pi4_xhci_storage,
                       sizeof(g_pi4_xhci_storage));
    pi4_dma_zero_bytes((uint8_t *)(void *)&g_pi4_fat32, sizeof(g_pi4_fat32));
    pi4_dma_zero_bytes((uint8_t *)(void *)&g_pi4_fat32_file, sizeof(g_pi4_fat32_file));
    g_pi4_xhci_storage.last_lba = g_pi4_xhci_storage.block_bytes = UINT32_MAX;
    for (unsigned i = 0; i < 2u; i++) {
        g_pi4_xhci_bot.command_rc[i] = UINT32_MAX;
        g_pi4_xhci_bot.command_status[i] = UINT32_MAX;
        g_pi4_xhci_bot.command_control[i] = UINT32_MAX;
    }
    for (unsigned i = 0; i < 3u; i++) {
        g_pi4_xhci_bot.io.rc[i] = UINT32_MAX;
        g_pi4_xhci_bot.io.status[i] = UINT32_MAX;
        g_pi4_xhci_bot.io.control[i] = UINT32_MAX;
    }
    for (unsigned i = 0; i < 4u; i++) {
        g_pi4_xhci_direct.rc[i] = UINT32_MAX;
        g_pi4_xhci_direct.status[i] = UINT32_MAX;
    }
    g_pi4_xhci_dma_state = PI4_XHCI_DMA_NOT_RUN;
    g_pi4_xhci_dma_events = 0;
    g_pi4_xhci_dma_event_cursor_index = event_cursor.index;
    g_pi4_xhci_dma_event_cursor_cycle = event_cursor.cycle;
    g_pi4_xhci_dma_command_completions = 0;
    g_pi4_xhci_dma_slot_id = 0;
    g_pi4_xhci_dma_slot_enabled = 0;
    g_pi4_xhci_dma_enable_slot_status = 0xffffffffu;
    g_pi4_xhci_dma_enable_slot_control = 0xffffffffu;
    g_pi4_xhci_dma_enable_trb_control = 0xffffffffu;
    g_pi4_xhci_dma_protocol_cap_offset = 0xffffffffu;
    g_pi4_xhci_dma_protocol_major = 0xffffffffu;
    g_pi4_xhci_dma_protocol_slot_type = 0xffffffffu;
    g_pi4_xhci_dma_address_status = 0xffffffffu;
    g_pi4_xhci_dma_address_control = 0xffffffffu;
    g_pi4_xhci_dma_device_address = 0xffffffffu;
    g_pi4_xhci_dma_device_slot_state = 0xffffffffu;
    g_pi4_xhci_dma_transfer_status = 0xffffffffu;
    g_pi4_xhci_dma_transfer_control = 0xffffffffu;
    g_pi4_xhci_dma_transfer_parameter = UINT64_MAX;
    g_pi4_xhci_dma_transfer_checks = 0;
    g_pi4_xhci_dma_transfer_skipped = 0;
    g_pi4_xhci_dma_transfer_skipped_status = 0xffffffffu;
    g_pi4_xhci_dma_transfer_skipped_control = 0xffffffffu;
    g_pi4_xhci_dma_transfer_skipped_parameter = UINT64_MAX;
    g_pi4_xhci_dma_device_bcd_usb = 0xffffffffu;
    g_pi4_xhci_dma_device_class = 0xffffffffu;
    g_pi4_xhci_dma_device_subclass = 0xffffffffu;
    g_pi4_xhci_dma_device_protocol = 0xffffffffu;
    g_pi4_xhci_dma_device_ep0_mps = 0xffffffffu;
    g_pi4_xhci_dma_device_vendor = 0xffffffffu;
    g_pi4_xhci_dma_device_product = 0xffffffffu;
    g_pi4_xhci_dma_device_bcd_device = 0xffffffffu;
    g_pi4_xhci_dma_device_configurations = 0xffffffffu;
    g_pi4_xhci_dma_config_buffer_bus = UINT64_MAX;
    g_pi4_xhci_dma_config_total_length = 0xffffffffu;
    g_pi4_xhci_dma_config_interfaces = 0xffffffffu;
    g_pi4_xhci_dma_config_value = 0xffffffffu;
    g_pi4_xhci_dma_config_attributes = 0xffffffffu;
    g_pi4_xhci_dma_config_max_power = 0xffffffffu;
    for (unsigned i = 0; i < 3; i++)
        g_pi4_xhci_dma_config_header_words[i] = 0xffffffffu;
    g_pi4_xhci_dma_set_config_setup_control = 0xffffffffu;
    g_pi4_xhci_dma_set_config_status_control = 0xffffffffu;
    g_pi4_xhci_dma_set_config_event_parameter = UINT64_MAX;
    g_pi4_xhci_dma_set_config_event_status = 0xffffffffu;
    g_pi4_xhci_dma_set_config_event_control = 0xffffffffu;
    g_pi4_xhci_dma_set_config_event_checks = 0;
    g_pi4_xhci_dma_set_config_event_skipped = 0;
    g_pi4_xhci_dma_hub_buffer_bus = UINT64_MAX;
    g_pi4_xhci_dma_hub_descriptor_transfer_rc = 0xffffffffu;
    g_pi4_xhci_dma_hub_descriptor_setup[0] = 0xffffffffu;
    g_pi4_xhci_dma_hub_descriptor_setup[1] = 0xffffffffu;
    g_pi4_xhci_dma_hub_descriptor_data_bus = UINT64_MAX;
    g_pi4_xhci_dma_hub_descriptor_data_length = 0xffffffffu;
    g_pi4_xhci_dma_hub_descriptor_data_control = 0xffffffffu;
    g_pi4_xhci_dma_hub_descriptor_status_control = 0xffffffffu;
    g_pi4_xhci_dma_hub_descriptor_validated = 0xffffffffu;
    for (unsigned i = 0; i < 4; i++)
        g_pi4_xhci_dma_hub_descriptor_words[i] = 0xffffffffu;
    g_pi4_xhci_dma_hub_ports = 0xffffffffu;
    g_pi4_xhci_dma_hub_characteristics = 0xffffffffu;
    g_pi4_xhci_dma_hub_power_delay = 0xffffffffu;
    g_pi4_xhci_dma_hub_current = 0xffffffffu;
    g_pi4_xhci_dma_hub_removable = 0xffffffffu;
    g_pi4_xhci_dma_hub_power_mask = 0xffffffffu;
    g_pi4_xhci_dma_hub_port_power_events = 0;
    g_pi4_xhci_dma_hub_port_status_events = 0;
    g_pi4_xhci_dma_hub_port_unpowered_mask = 0;
    g_pi4_xhci_dma_hub_port_connected_mask = 0;
    g_pi4_xhci_dma_hub_probe_phase = 0;
    g_pi4_xhci_dma_hub_probe_port = 0;
    g_pi4_xhci_dma_hub_request_type = 0xffffffffu;
    g_pi4_xhci_dma_hub_request = 0xffffffffu;
    g_pi4_xhci_dma_hub_request_value = 0xffffffffu;
    g_pi4_xhci_dma_hub_request_index = 0xffffffffu;
    g_pi4_xhci_dma_hub_request_length = 0xffffffffu;
    g_pi4_xhci_dma_hub_expected_status_bus = UINT64_MAX;
    g_pi4_xhci_dma_hub_setup_trb_bus = UINT64_MAX;
    g_pi4_xhci_dma_hub_data_trb_bus = UINT64_MAX;
    g_pi4_xhci_dma_output_ep0_context_valid = 0;
    g_pi4_xhci_dma_output_ep0_state = 0xffffffffu;
    g_pi4_xhci_dma_output_ep0_dcs = 0xffffffffu;
    g_pi4_xhci_dma_output_ep0_dequeue_bus = UINT64_MAX;
    g_pi4_xhci_dma_stop_endpoint_rc = UINT32_MAX;
    g_pi4_xhci_dma_stop_endpoint_trb_control = 0xffffffffu;
    g_pi4_xhci_dma_stop_endpoint_status = 0xffffffffu;
    g_pi4_xhci_dma_stop_endpoint_control = 0xffffffffu;
    g_pi4_xhci_dma_stop_endpoint_transfer_events = 0;
    g_pi4_xhci_dma_stop_endpoint_transfer_parameter = UINT64_MAX;
    g_pi4_xhci_dma_stop_endpoint_transfer_status = 0xffffffffu;
    g_pi4_xhci_dma_stop_endpoint_transfer_control = 0xffffffffu;
    g_pi4_xhci_dma_stop_endpoint_other_events = 0;
    g_pi4_xhci_dma_stop_output_ep0_context_valid = 0;
    g_pi4_xhci_dma_stop_output_ep0_state = 0xffffffffu;
    g_pi4_xhci_dma_stop_output_ep0_dcs = 0xffffffffu;
    g_pi4_xhci_dma_stop_output_ep0_dequeue_bus = UINT64_MAX;
    for (unsigned i = 0; i < 5u; i++)
        g_pi4_xhci_dma_stop_output_ep0_context[i] = 0xffffffffu;
    for (unsigned i = 0; i < 4; i++) {
        g_pi4_xhci_dma_hub_port_status[i] = 0xffffffffu;
        g_pi4_xhci_dma_hub_port_change[i] = 0xffffffffu;
    }
    g_pi4_xhci_dma_hub_child_reset_state =
        PI4_XHCI_HUB_CHILD_RESET_NOT_RUN;
    g_pi4_xhci_dma_hub_child_reset_port = 0;
    g_pi4_xhci_dma_hub_child_reset_request_rc = 0xffffffffu;
    g_pi4_xhci_dma_hub_child_reset_samples = 0;
    g_pi4_xhci_dma_hub_child_reset_status = 0xffffffffu;
    g_pi4_xhci_dma_hub_child_reset_change = 0xffffffffu;
    g_pi4_xhci_dma_hub_child_clear_rc = 0xffffffffu;
    g_pi4_xhci_dma_hub_child_final_status = 0xffffffffu;
    g_pi4_xhci_dma_hub_child_final_change = 0xffffffffu;
    g_pi4_xhci_dma_hub_config_status = 0xffffffffu;
    g_pi4_xhci_dma_hub_config_control = 0xffffffffu;
    for (unsigned i = 0; i < 3u; i++)
        g_pi4_xhci_dma_hub_output_slot_words[i] = 0xffffffffu;
    g_pi4_xhci_dma_hub_guard_status = 0xffffffffu;
    g_pi4_xhci_dma_hub_guard_control = 0xffffffffu;
    for (unsigned i = 0; i < 8u; i++)
        g_pi4_xhci_dma_hub_config_input_words[i] = 0xffffffffu;
    for (unsigned i = 0; i < 4u; i++)
        g_pi4_xhci_dma_hub_config_trb_words[i] = 0xffffffffu;
    g_pi4_xhci_dma_hub_config_crcr[0] = 0xffffffffu;
    g_pi4_xhci_dma_hub_config_crcr[1] = 0xffffffffu;
    g_pi4_xhci_dma_hub_config_usbsts = 0xffffffffu;
    g_pi4_xhci_dma_hub_config_db = 0xffffffffu;
    g_pi4_xhci_dma_child_slot_id = 0xffffffffu;
    g_pi4_xhci_dma_child_speed_id = 0xffffffffu;
    g_pi4_xhci_dma_child_route_string = 0xffffffffu;
    g_pi4_xhci_dma_child_enable_status = 0xffffffffu;
    g_pi4_xhci_dma_child_enable_control = 0xffffffffu;
    g_pi4_xhci_dma_child_address_status = 0xffffffffu;
    g_pi4_xhci_dma_child_address_control = 0xffffffffu;
    g_pi4_xhci_dma_child_address = 0xffffffffu;
    g_pi4_xhci_dma_child_slot_state = 0xffffffffu;
    for (unsigned i = 0; i < 6u; i++)
        g_pi4_xhci_dma_child_address_context_words[i] = 0xffffffffu;
    g_pi4_xhci_dma_child_output_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_ep0_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_descriptor_transfer_rc = 0xffffffffu;
    g_pi4_xhci_dma_child_descriptor_setup_control = 0xffffffffu;
    g_pi4_xhci_dma_child_descriptor_data_control = 0xffffffffu;
    g_pi4_xhci_dma_child_descriptor_status_control = 0xffffffffu;
    g_pi4_xhci_dma_child_descriptor_validated = 0xffffffffu;
    g_pi4_xhci_dma_child_descriptor_words[0] = 0xffffffffu;
    g_pi4_xhci_dma_child_descriptor_words[1] = 0xffffffffu;
    g_pi4_xhci_dma_child_descriptor_buffer_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_descriptor_status_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_repeat_descriptor_transfer_rc = 0xffffffffu;
    g_pi4_xhci_dma_child_repeat_descriptor_event_parameter = UINT64_MAX;
    g_pi4_xhci_dma_child_repeat_descriptor_event_status = 0xffffffffu;
    g_pi4_xhci_dma_child_repeat_descriptor_event_control = 0xffffffffu;
    g_pi4_xhci_dma_child_repeat_descriptor_event_checks = 0;
    g_pi4_xhci_dma_child_repeat_descriptor_setup_control = 0xffffffffu;
    g_pi4_xhci_dma_child_repeat_descriptor_data_control = 0xffffffffu;
    g_pi4_xhci_dma_child_repeat_descriptor_status_control = 0xffffffffu;
    g_pi4_xhci_dma_child_repeat_descriptor_validated = 0xffffffffu;
    g_pi4_xhci_dma_child_repeat_descriptor_observed = 0;
    g_pi4_xhci_dma_child_repeat_descriptor_words[0] = 0xffffffffu;
    g_pi4_xhci_dma_child_repeat_descriptor_words[1] = 0xffffffffu;
    g_pi4_xhci_dma_child_repeat_descriptor_buffer_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_repeat_descriptor_status_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_ep0_evaluate_status = 0xffffffffu;
    g_pi4_xhci_dma_child_ep0_mps_command = 0u;
    g_pi4_xhci_dma_child_ep0_evaluate_control = 0xffffffffu;
    g_pi4_xhci_dma_child_ep0_evaluate_mps = 0xffffffffu;
    g_pi4_xhci_dma_child_ep0_evaluate_output_mps = 0xffffffffu;
    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_transfer_rc = 0xffffffffu;
    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_parameter = UINT64_MAX;
    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_status = 0xffffffffu;
    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_control = 0xffffffffu;
    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_checks = 0;
    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_setup_control = 0xffffffffu;
    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_data_control = 0xffffffffu;
    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_status_control = 0xffffffffu;
    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_observed = 0;
    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_word = 0xffffffffu;
    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_buffer_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_status_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_get_status_transfer_rc = 0xffffffffu;
    g_pi4_xhci_dma_child_get_status_event_parameter = UINT64_MAX;
    g_pi4_xhci_dma_child_get_status_event_status = 0xffffffffu;
    g_pi4_xhci_dma_child_get_status_event_control = 0xffffffffu;
    g_pi4_xhci_dma_child_get_status_event_checks = 0;
    g_pi4_xhci_dma_child_get_status_setup_control = 0xffffffffu;
    g_pi4_xhci_dma_child_get_status_data_control = 0xffffffffu;
    g_pi4_xhci_dma_child_get_status_status_control = 0xffffffffu;
    g_pi4_xhci_dma_child_get_status_observed = 0;
    g_pi4_xhci_dma_child_get_status_word = 0xffffffffu;
    g_pi4_xhci_dma_child_get_status_buffer_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_get_status_status_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_full_descriptor_transfer_rc = 0xffffffffu;
    g_pi4_xhci_dma_child_full_descriptor_setup_control = 0xffffffffu;
    g_pi4_xhci_dma_child_full_descriptor_data_control = 0xffffffffu;
    g_pi4_xhci_dma_child_full_descriptor_status_control = 0xffffffffu;
    g_pi4_xhci_dma_child_full_descriptor_validated = 0xffffffffu;
    g_pi4_xhci_dma_child_full_descriptor_observed = 0;
    for (unsigned i = 0; i < 5u; i++)
        g_pi4_xhci_dma_child_full_descriptor_words[i] = 0xffffffffu;
    g_pi4_xhci_dma_child_full_descriptor_buffer_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_full_descriptor_status_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_config_header_transfer_rc = 0xffffffffu;
    g_pi4_xhci_dma_child_config_header_event_parameter = UINT64_MAX;
    g_pi4_xhci_dma_child_config_header_event_status = 0xffffffffu;
    g_pi4_xhci_dma_child_config_header_event_control = 0xffffffffu;
    g_pi4_xhci_dma_child_config_header_event_checks = 0;
    g_pi4_xhci_dma_child_config_header_setup_control = 0xffffffffu;
    g_pi4_xhci_dma_child_config_header_data_control = 0xffffffffu;
    g_pi4_xhci_dma_child_config_header_status_control = 0xffffffffu;
    g_pi4_xhci_dma_child_config_header_validated = 0xffffffffu;
    g_pi4_xhci_dma_child_config_header_observed = 0;
    for (unsigned i = 0; i < 3u; i++)
        g_pi4_xhci_dma_child_config_header_words[i] = 0xffffffffu;
    g_pi4_xhci_dma_child_config_header_buffer_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_config_header_status_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_config_full_transfer_rc = 0xffffffffu;
    g_pi4_xhci_dma_child_config_full_event_parameter = UINT64_MAX;
    g_pi4_xhci_dma_child_config_full_event_status = 0xffffffffu;
    g_pi4_xhci_dma_child_config_full_event_control = 0xffffffffu;
    g_pi4_xhci_dma_child_config_full_event_checks = 0;
    g_pi4_xhci_dma_child_config_full_setup_control = 0xffffffffu;
    g_pi4_xhci_dma_child_config_full_data_control = 0xffffffffu;
    g_pi4_xhci_dma_child_config_full_status_control = 0xffffffffu;
    g_pi4_xhci_dma_child_config_full_validated = 0xffffffffu;
    g_pi4_xhci_dma_child_config_full_observed = 0;
    for (unsigned i = 0; i < 3u; i++)
        g_pi4_xhci_dma_child_config_full_words[i] = 0xffffffffu;
    g_pi4_xhci_dma_child_config_full_buffer_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_config_full_status_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_config_summary =
        (struct pi4_xhci_dma_config_summary){
            .total_length = UINT32_MAX,
            .configuration_value = UINT32_MAX,
            .interface_descriptors = UINT32_MAX,
            .unique_interfaces = UINT32_MAX,
            .endpoint_descriptors = UINT32_MAX,
            .first_interface_number = UINT32_MAX,
            .first_interface_class = UINT32_MAX,
            .first_interface_subclass = UINT32_MAX,
            .first_interface_protocol = UINT32_MAX,
            .first_interface_endpoint_count = UINT32_MAX,
            .first_endpoint_address = UINT32_MAX,
            .first_endpoint_attributes = UINT32_MAX,
            .first_endpoint_max_packet = UINT32_MAX,
            .first_endpoint_interval = UINT32_MAX,
            .bot_interface_found = UINT32_MAX,
            .bot_interface_number = UINT32_MAX,
            .bot_interface_alt = UINT32_MAX,
            .bot_interface_endpoint_count = UINT32_MAX,
            .bot_bulk_in_address = UINT32_MAX,
            .bot_bulk_in_attributes = UINT32_MAX,
            .bot_bulk_in_max_packet = UINT32_MAX,
            .bot_bulk_in_interval = UINT32_MAX,
            .bot_bulk_out_address = UINT32_MAX,
            .bot_bulk_out_attributes = UINT32_MAX,
            .bot_bulk_out_max_packet = UINT32_MAX,
            .bot_bulk_out_interval = UINT32_MAX
        };
    g_pi4_xhci_dma_child_bulk_guard_rc = -1;
    g_pi4_xhci_dma_child_bulk_guard_status = 0xffffffffu;
    g_pi4_xhci_dma_child_bulk_guard_control = 0xffffffffu;
    g_pi4_xhci_dma_child_bulk_config_rc = -1;
    g_pi4_xhci_dma_child_bulk_config_status = 0xffffffffu;
    g_pi4_xhci_dma_child_bulk_config_control = 0xffffffffu;
    g_pi4_xhci_dma_child_bulk_contexts_verified = 0;
    g_pi4_xhci_dma_child_bulk_in_ring_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_bulk_out_ring_bus = UINT64_MAX;
    for (unsigned i = 0; i < 6u; i++)
        g_pi4_xhci_dma_child_bulk_slot_words[i] = 0xffffffffu;
    for (unsigned i = 0; i < 5u; i++) {
        g_pi4_xhci_dma_child_bulk_in_words[i] = 0xffffffffu;
        g_pi4_xhci_dma_child_bulk_out_words[i] = 0xffffffffu;
    }
    g_pi4_xhci_dma_child_set_config_transfer_rc = 0xffffffffu;
    g_pi4_xhci_dma_child_set_config_event_parameter = UINT64_MAX;
    g_pi4_xhci_dma_child_set_config_event_status = 0xffffffffu;
    g_pi4_xhci_dma_child_set_config_event_control = 0xffffffffu;
    g_pi4_xhci_dma_child_set_config_event_checks = 0;
    g_pi4_xhci_dma_child_set_config_setup_control = 0xffffffffu;
    g_pi4_xhci_dma_child_set_config_status_control = 0xffffffffu;
    g_pi4_xhci_dma_child_set_config_status_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_get_config_transfer_rc = 0xffffffffu;
    g_pi4_xhci_dma_child_get_config_event_parameter = UINT64_MAX;
    g_pi4_xhci_dma_child_get_config_event_status = 0xffffffffu;
    g_pi4_xhci_dma_child_get_config_event_control = 0xffffffffu;
    g_pi4_xhci_dma_child_get_config_event_checks = 0;
    g_pi4_xhci_dma_child_get_config_setup_control = 0xffffffffu;
    g_pi4_xhci_dma_child_get_config_data_control = 0xffffffffu;
    g_pi4_xhci_dma_child_get_config_status_control = 0xffffffffu;
    g_pi4_xhci_dma_child_get_config_observed = 0;
    g_pi4_xhci_dma_child_get_config_value = 0xffffffffu;
    g_pi4_xhci_dma_child_get_config_buffer_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_get_config_status_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_get_max_lun_transfer_rc = -1;
    g_pi4_xhci_dma_child_get_max_lun_prepare = 0xffffffffu;
    g_pi4_xhci_dma_child_get_max_lun_event_parameter = UINT64_MAX;
    g_pi4_xhci_dma_child_get_max_lun_event_status = 0xffffffffu;
    g_pi4_xhci_dma_child_get_max_lun_event_control = 0xffffffffu;
    g_pi4_xhci_dma_child_get_max_lun_event_checks = 0;
    g_pi4_xhci_dma_child_get_max_lun_setup_control = 0xffffffffu;
    g_pi4_xhci_dma_child_get_max_lun_data_control = 0xffffffffu;
    g_pi4_xhci_dma_child_get_max_lun_status_control = 0xffffffffu;
    g_pi4_xhci_dma_child_get_max_lun_observed = 0;
    g_pi4_xhci_dma_child_get_max_lun_value = 0xffffffffu;
    g_pi4_xhci_dma_child_get_max_lun_buffer_bus = UINT64_MAX;
    g_pi4_xhci_dma_child_get_max_lun_status_bus = UINT64_MAX;
    g_pi4_xhci_dma_setup_trb_control = 0xffffffffu;
    g_pi4_xhci_dma_data_trb_control = 0xffffffffu;
    g_pi4_xhci_dma_status_trb_control = 0xffffffffu;
    g_pi4_xhci_dma_control_buffer_bus = UINT64_MAX;
    g_pi4_xhci_dma_descriptor_prefix[0] = 0xffffffffu;
    g_pi4_xhci_dma_descriptor_prefix[1] = 0xffffffffu;
    g_pi4_xhci_dma_stage = 1; /* Entered: reserved DMA arena preflight. */
    g_pi4_xhci_dma_layout_rc = INT32_MIN;
    g_pi4_xhci_dma_page_size = 0;
    g_pi4_xhci_dma_gate_failures = 0xffffffffu;
    g_pi4_xhci_dma_usbcmd = 0xffffffffu;
    g_pi4_xhci_dma_usbsts = 0xffffffffu;
    g_pi4_xhci_dma_legacy = 0xffffffffu;
    g_pi4_xhci_dma_pci_command = 0xffffffffu;
    g_pi4_xhci_dma_status = 0;
    g_pi4_xhci_dma_event_status = 0;
    g_pi4_xhci_dma_event_control = 0;
    g_pi4_xhci_dma_setup_mask = 0;
    g_pi4_xhci_dma_setup_mismatch = 0;
    g_pi4_xhci_dma_bar2_state = 0;
    g_pi4_xhci_dma_bar2_lo = 0xffffffffu;
    g_pi4_xhci_dma_bar2_hi_original = 0xffffffffu;
    g_pi4_xhci_dma_bar2_hi_test = 0xffffffffu;
    g_pi4_xhci_dma_bar2_hi_restored = 0xffffffffu;
    g_pi4_xhci_dma_bus_bias = 0;
    g_pi4_xhci_dma_setup_iman = 0xffffffffu;
    g_pi4_xhci_dma_setup_imod = 0xffffffffu;
    g_pi4_xhci_dma_setup_config = 0xffffffffu;
    g_pi4_xhci_dma_setup_erstsz = 0xffffffffu;
    g_pi4_xhci_dma_setup_dcbaa = UINT64_MAX;
    g_pi4_xhci_dma_setup_erstba = UINT64_MAX;
    g_pi4_xhci_dma_setup_erdp = UINT64_MAX;
    g_pi4_xhci_dma_abort_state = 0;
    g_pi4_xhci_dma_abort_status = 0;
    g_pi4_xhci_dma_abort_control = 0;
    g_pi4_xhci_dma_abort_parameter = 0;
    g_pi4_xhci_dma_runtime_cmd = 0xffffffffu;
    g_pi4_xhci_dma_runtime_sts = 0xffffffffu;
    g_pi4_xhci_dma_run_sts = 0xffffffffu;
    g_pi4_xhci_dma_doorbell_sts = 0xffffffffu;
    g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_NOT_RUN;
    g_pi4_xhci_dma_mfindex_start = 0xffffffffu;
    g_pi4_xhci_dma_mfindex_end = 0xffffffffu;
    g_pi4_xhci_dma_wrap_status = 0xffffffffu;
    g_pi4_xhci_dma_wrap_control = 0xffffffffu;
    g_pi4_xhci_dma_port_event_state = PI4_XHCI_PORT_EVENT_NOT_RUN;
    g_pi4_xhci_dma_port_event_port = 0;
    g_pi4_xhci_dma_port_ready = 0;
    g_pi4_xhci_dma_port_event_before = 0xffffffffu;
    g_pi4_xhci_dma_port_event_after = 0xffffffffu;
    g_pi4_xhci_dma_port_event_parameter = UINT64_MAX;
    g_pi4_xhci_dma_port_event_status = 0xffffffffu;
    g_pi4_xhci_dma_port_event_control = 0xffffffffu;
    g_pi4_xhci_dma_crcr_lo = 0xffffffffu;
    g_pi4_xhci_dma_crcr_hi = 0xffffffffu;
    g_pi4_xhci_dma_db_flush = 0xffffffffu;
    g_pi4_xhci_dma_timeout_sts = 0xffffffffu;
    g_pi4_xhci_dma_timeout_crcr = 0xffffffffu;
    g_pi4_xhci_dma_timeout_iman = 0xffffffffu;
    g_pi4_xhci_dma_erdp_lo = 0xffffffffu;
    g_pi4_xhci_dma_erdp_hi = 0xffffffffu;
    for (unsigned i = 0; i < 4; i++)
        g_pi4_xhci_dma_event_trb[i] = 0xffffffffu;
    if (!h || g_pi4_cfg_state != PI4_CFG_READ_OK ||
        g_pi4_xhci_state != PI4_XHCI_CAPS_OK ||
        !(g_pi4_xhci_state_mask & PI4_XHCI_STATE_RESET_OK) ||
        g_pi4_dma_probe_state != PI4_DMA_CPU_PASS ||
        (g_pi4_cfg_id & 0xffffffffu) != 0x34831106u ||
        ((uint16_t)g_pi4_cfg_command & PI4_PCI_COMMAND_MASTER)) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_SKIPPED_GATE;
        return;
    }
    plan_rc = pi4_dma_probe_plan_make(h, &arena);
    if (plan_rc != PI4_DMA_PROBE_PLAN_OK) {
        g_pi4_xhci_dma_layout_rc = plan_rc;
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_BAD_LAYOUT;
        return;
    }
    g_pi4_xhci_dma_stage = 2; /* Arena accepted; validate HHDM and BAR. */
    if (!h->hhdm_offset || h->hhdm_offset > UINT64_MAX - arena.phys_base ||
        h->hhdm_offset + arena.phys_base > UINT64_MAX - arena.size) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_BAD_LAYOUT;
        return;
    }
    if (g_pi4_xhci_bar_va < h->hhdm_offset ||
        mmio_map_2m(h->hhdm_offset, g_pi4_xhci_bar_cpu) != 0) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
        return;
    }
    bar = (volatile uint8_t *)(uintptr_t)g_pi4_xhci_bar_va;
    cap_length = g_pi4_xhci_caps[0] & 0xffu;
    db_offset = g_pi4_xhci_caps[4] & ~3u;
    rt_offset = g_pi4_xhci_caps[5] & ~31u;
    if (cap_length < 0x20u || cap_length > PI4_XHCI_MMIO_BYTES - 0x3cu ||
        db_offset > PI4_XHCI_MMIO_BYTES - 4u ||
        rt_offset > PI4_XHCI_MMIO_BYTES - 0x40u) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_BAD_LAYOUT;
        return;
    }
    g_pi4_xhci_dma_stage = 3; /* BAR offsets accepted; enable MSE for MMIO. */
    op = bar + cap_length;
    rt = bar + rt_offset;
    db = bar + db_offset;
    if (pi4_pcie_read_config32(h, g_pi4_link_status, 0x04,
                               &current_command) != 0) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_PCI_ENABLE_FAIL;
        return;
    }
    original_pci = (uint16_t)current_command;
    if (pi4_xhci_memory_command(original_pci, &mmio_pci) != 0) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_SKIPPED_GATE;
        return;
    }
    if (mmio_pci != original_pci) {
        /* MSE permits CPU BAR access; BME remains clear during preflight. */
        pci_mse_may_be_set = 1;
        if (pi4_pcie_write_config32(h, g_pi4_link_status, 0x04,
                                    mmio_pci) != 0 ||
            pi4_pcie_read_config32(h, g_pi4_link_status, 0x04,
                                   &verify) != 0 ||
            (uint16_t)verify != mmio_pci ||
            ((uint16_t)verify & PI4_PCI_COMMAND_MASTER)) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_PCI_ENABLE_FAIL;
            goto preflight_restore;
        }
        g_pi4_cfg_command = verify;
    }
    if (pi4_xhci_read_mmio32(op, 0, PI4_XHCI_MMIO_BYTES, &usbcmd) != 0 ||
        pi4_xhci_read_mmio32(op, 4, PI4_XHCI_MMIO_BYTES, &usbsts) != 0 ||
        pi4_xhci_read_mmio32(op, 8, PI4_XHCI_MMIO_BYTES, &page_size) != 0) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
        goto preflight_restore;
    }
    g_pi4_xhci_dma_page_size = page_size;
    g_pi4_xhci_dma_usbcmd = usbcmd;
    g_pi4_xhci_dma_usbsts = usbsts;
    g_pi4_xhci_dma_legacy = g_pi4_xhci_legacy_after;
    g_pi4_xhci_dma_pci_command = original_pci;
    g_pi4_xhci_dma_gate_failures = 0;
    if (original_pci & PI4_PCI_COMMAND_MASTER)
        g_pi4_xhci_dma_gate_failures |= 1u << 0;
    if (usbcmd & (PI4_XHCI_USBCMD_RUN | PI4_XHCI_USBCMD_HCRST))
        g_pi4_xhci_dma_gate_failures |= 1u << 1;
    if (!(usbsts & PI4_XHCI_USBSTS_HALTED))
        g_pi4_xhci_dma_gate_failures |= 1u << 2;
    if (usbsts & PI4_XHCI_USBSTS_CNR)
        g_pi4_xhci_dma_gate_failures |= 1u << 3;
    if (g_pi4_xhci_legacy_after & PI4_XHCI_LEGACY_BIOS_OWNED)
        g_pi4_xhci_dma_gate_failures |= 1u << 4;
    if (!(g_pi4_xhci_legacy_after & PI4_XHCI_LEGACY_OS_OWNED))
        g_pi4_xhci_dma_gate_failures |= 1u << 5;
    g_pi4_xhci_dma_stage = 4; /* Operational registers read; check layout. */

    /* Keep the FDT-validated arena as the CPU allocation, but test the
     * alternate 16-GiB PCIe bus alias used by a separate Pi bare-metal xHCI
     * implementation. The exact BAR2 baseline and xHC AC64 capability are
     * mandatory; no bridge register is written until the complete DMA layout
     * has passed its halted-controller safety gates. */
    if (pi4_pcie_read_mmio32(h, PI4_PCIE_INBOUND_BAR2_LO,
                             &g_pi4_xhci_dma_bar2_lo) != 0 ||
        pi4_pcie_read_mmio32(h, PI4_PCIE_INBOUND_BAR2_HI,
                             &g_pi4_xhci_dma_bar2_hi_original) != 0) {
        g_pi4_xhci_dma_bar2_state |= 1u << 31;
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_INBOUND_CONFIG_FAIL;
        goto preflight_restore;
    }
    g_pi4_xhci_dma_bar2_state |= 1u << 0; /* Original low/high captured. */
    if (pi4_xhci_dma_alias_plan_make(&arena, g_pi4_xhci_caps,
            g_pi4_xhci_dma_bar2_lo, g_pi4_xhci_dma_bar2_hi_original,
            &alias_arena) != 0) {
        g_pi4_xhci_dma_bar2_state |= 1u << 31;
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_INBOUND_CONFIG_FAIL;
        goto preflight_restore;
    }
    g_pi4_xhci_dma_bus_bias = alias_arena.bus_base - arena.bus_base;
    g_pi4_xhci_dma_bar2_hi_test = PI4_XHCI_DMA_BAR2_HI_TEST;
    arena = alias_arena;

    g_pi4_xhci_dma_layout_rc = pi4_xhci_dma_layout_make(
            &arena, g_pi4_xhci_caps, page_size,
            usbcmd, usbsts, g_pi4_xhci_legacy_after, original_pci,
            &layout);
    if (g_pi4_xhci_dma_layout_rc != PI4_XHCI_DMA_PLAN_OK) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_BAD_LAYOUT;
        goto preflight_restore;
    }
    if (pi4_xhci_dma_enable_command(original_pci, &enabled_pci) != 0) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_SKIPPED_GATE;
        goto preflight_restore;
    }
    g_pi4_xhci_dma_stage = 5; /* Layout accepted; build DMA structures. */

    /* Apply only after all safety gates: original BAR2 low word is retained;
     * its high word and every xHCI DMA address move to the same 16-GiB alias. */
    bar2_may_be_set = 1;
    g_pi4_xhci_dma_bar2_state |= 1u << 1; /* Write attempted. */
    if (pi4_pcie_write_mmio32(h, PI4_PCIE_INBOUND_BAR2_HI,
                              g_pi4_xhci_dma_bar2_hi_test) != 0 ||
        pi4_pcie_read_mmio32(h, PI4_PCIE_INBOUND_BAR2_HI, &verify) != 0 ||
        verify != g_pi4_xhci_dma_bar2_hi_test) {
        g_pi4_xhci_dma_bar2_state |= 1u << 31;
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_INBOUND_CONFIG_FAIL;
        goto preflight_restore;
    }
    g_pi4_xhci_dma_bar2_state |= 1u << 2; /* Alias write verified. */

    memory = (uint8_t *)(uintptr_t)(h->hhdm_offset + arena.phys_base);
    event_memory = memory + layout.event_offset;
    command_memory = memory + layout.command_offset;
    input_context = memory + layout.input_context_offset;
    output_context = memory + layout.output_context_offset;
    ep0_ring = memory + layout.ep0_ring_offset;
    control_buffer = memory + layout.control_buffer_offset;
    child_output_context = memory + layout.child_output_context_offset;
    child_ep0_ring = memory + layout.child_ep0_ring_offset;
    child_bulk_in_ring = memory + layout.child_bulk_in_ring_offset;
    child_bulk_out_ring = memory + layout.child_bulk_out_ring_offset;
    dcbaa = (uint64_t *)(void *)(memory + layout.dcbaa_offset);
    scratch_array = layout.scratchpads
        ? (uint64_t *)(void *)(memory + layout.scratch_array_offset) : NULL;
    erst = (uint32_t *)(void *)(memory + layout.erst_offset);
    dcbaa_bus = arena.bus_base + layout.dcbaa_offset;
    scratch_array_bus = arena.bus_base + layout.scratch_array_offset;
    command_bus = arena.bus_base + layout.command_offset;
    event_bus = arena.bus_base + layout.event_offset;
    erst_bus = arena.bus_base + layout.erst_offset;
    input_context_bus = arena.bus_base + layout.input_context_offset;
    output_context_bus = arena.bus_base + layout.output_context_offset;
    ep0_ring_bus = arena.bus_base + layout.ep0_ring_offset;
    control_buffer_bus = arena.bus_base + layout.control_buffer_offset;
    child_output_bus = arena.bus_base + layout.child_output_context_offset;
    child_ep0_bus = arena.bus_base + layout.child_ep0_ring_offset;
    child_bulk_in_bus = arena.bus_base + layout.child_bulk_in_ring_offset;
    child_bulk_out_bus = arena.bus_base + layout.child_bulk_out_ring_offset;
    g_pi4_xhci_dma_child_bulk_in_ring_bus = child_bulk_in_bus;
    g_pi4_xhci_dma_child_bulk_out_ring_bus = child_bulk_out_bus;
    g_pi4_xhci_dma_child_output_bus = child_output_bus;
    g_pi4_xhci_dma_child_ep0_bus = child_ep0_bus;

    pi4_dma_zero_bytes(memory, layout.used_bytes);
    if (layout.scratchpads) {
        dcbaa[0] = scratch_array_bus;
        for (unsigned i = 0; i < layout.scratchpads; i++)
            scratch_array[i] = arena.bus_base + layout.scratch_pages_offset +
                               (uint64_t)i * PI4_XHCI_DMA_PAGE;
    }
    if (pi4_xhci_dma_command_ring_seed(
            command_memory, command_bus) != 0 ||
        pi4_xhci_dma_transfer_ring_seed(ep0_ring, ep0_ring_bus) != 0 ||
        pi4_xhci_dma_transfer_ring_seed(child_ep0_ring, child_ep0_bus) != 0 ||
        pi4_xhci_dma_transfer_ring_seed(child_bulk_in_ring,
                                        child_bulk_in_bus) != 0 ||
        pi4_xhci_dma_transfer_ring_seed(child_bulk_out_ring,
                                        child_bulk_out_bus) != 0) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_BAD_LAYOUT;
        goto preflight_restore;
    }
    erst[0] = (uint32_t)event_bus;
    erst[1] = (uint32_t)(event_bus >> 32);
    erst[2] = PI4_XHCI_DMA_EVENT_TRBS;
    erst[3] = 0;

    /* Clean read-only structures, and clean+invalidate the device-write ring
     * so a dirty CPU line cannot later overwrite a DMA completion. */
    pi4_dma_clean_bytes(memory, layout.used_bytes);
    pi4_dma_clean_invalidate_bytes(event_memory, PI4_XHCI_DMA_EVENT_BYTES);
    __asm__ volatile("dsb sy" ::: "memory");

    /* Initialize the complete primary interrupter before RUN. Clear only a
     * pending IP bit (RW1C), enable the interrupter as in the xHCI startup
     * sequence, keep global USBCMD.INTE explicitly off, and disable
     * moderation. IMAN.IE permits interrupt generation; it does not gate Event
     * TRB posting, so the ring remains polled and no host IRQ is enabled. Then
     * set event-ring size and dequeue pointer before ERSTBA, whose write
     * initializes the controller's event-ring state machine. */
    if (pi4_xhci_read_mmio32(rt, 0x20, PI4_XHCI_MMIO_BYTES, &iman) != 0 ||
        pi4_xhci_write_mmio32(rt, 0x20, PI4_XHCI_MMIO_BYTES,
                              (iman & 1u) | PI4_XHCI_IMAN_IE) != 0 ||
        pi4_xhci_write_mmio32(rt, 0x24, PI4_XHCI_MMIO_BYTES, 0) != 0 ||
        pi4_xhci_write_mmio32(op, 0x38, PI4_XHCI_MMIO_BYTES,
                              layout.max_slots) != 0 ||
        !pi4_xhci_dma_write64(op, 0x30, PI4_XHCI_MMIO_BYTES, dcbaa_bus) ||
        !pi4_xhci_dma_write64(op, 0x18, PI4_XHCI_MMIO_BYTES,
                (command_bus + (uint64_t)
                    PI4_XHCI_DMA_COMMAND_ENABLE_SLOT_TRB * 16u) | 1u) ||
        pi4_xhci_write_mmio32(rt, 0x28, PI4_XHCI_MMIO_BYTES, 1) != 0 ||
        !pi4_xhci_dma_write64(rt, 0x38, PI4_XHCI_MMIO_BYTES, event_bus) ||
        !pi4_xhci_dma_write64(rt, 0x30, PI4_XHCI_MMIO_BYTES, erst_bus)) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
        goto preflight_restore;
    }
    g_pi4_xhci_dma_stage = 6; /* DMA structures programmed, before BME. */
    __asm__ volatile("dsb sy" ::: "memory");

    /* Read back every ordinary setup pointer before granting bus mastering.
     * CRCR is intentionally excluded: its ring-pointer and RCS fields are
     * architecturally read-as-zero, so only the later CRR status is useful. */
    if (pi4_xhci_read_mmio32(rt, 0x20, PI4_XHCI_MMIO_BYTES,
                             &rb_iman) == 0) {
        g_pi4_xhci_dma_setup_iman = rb_iman;
        g_pi4_xhci_dma_setup_mask |= PI4_XHCI_DMA_SETUP_IMAN;
    }
    if (pi4_xhci_read_mmio32(rt, 0x24, PI4_XHCI_MMIO_BYTES,
                             &rb_imod) == 0) {
        g_pi4_xhci_dma_setup_imod = rb_imod;
        g_pi4_xhci_dma_setup_mask |= PI4_XHCI_DMA_SETUP_IMOD;
    }
    if (pi4_xhci_read_mmio32(op, 0x38, PI4_XHCI_MMIO_BYTES,
                             &rb_config) == 0) {
        g_pi4_xhci_dma_setup_config = rb_config;
        g_pi4_xhci_dma_setup_mask |= PI4_XHCI_DMA_SETUP_CONFIG;
    }
    if (pi4_xhci_read_mmio32(op, 0x30, PI4_XHCI_MMIO_BYTES,
                             &rb_lo) == 0 &&
        pi4_xhci_read_mmio32(op, 0x34, PI4_XHCI_MMIO_BYTES,
                             &rb_hi) == 0) {
        rb_dcbaa = (uint64_t)rb_lo | ((uint64_t)rb_hi << 32);
        g_pi4_xhci_dma_setup_dcbaa = rb_dcbaa;
        g_pi4_xhci_dma_setup_mask |= PI4_XHCI_DMA_SETUP_DCBAAP;
    }
    if (pi4_xhci_read_mmio32(rt, 0x28, PI4_XHCI_MMIO_BYTES,
                             &rb_erstsz) == 0) {
        g_pi4_xhci_dma_setup_erstsz = rb_erstsz;
        g_pi4_xhci_dma_setup_mask |= PI4_XHCI_DMA_SETUP_ERSTSZ;
    }
    if (pi4_xhci_read_mmio32(rt, 0x30, PI4_XHCI_MMIO_BYTES,
                             &rb_lo) == 0 &&
        pi4_xhci_read_mmio32(rt, 0x34, PI4_XHCI_MMIO_BYTES,
                             &rb_hi) == 0) {
        rb_erstba = (uint64_t)rb_lo | ((uint64_t)rb_hi << 32);
        g_pi4_xhci_dma_setup_erstba = rb_erstba;
        g_pi4_xhci_dma_setup_mask |= PI4_XHCI_DMA_SETUP_ERSTBA;
    }
    if (pi4_xhci_read_mmio32(rt, 0x38, PI4_XHCI_MMIO_BYTES,
                             &rb_lo) == 0 &&
        pi4_xhci_read_mmio32(rt, 0x3c, PI4_XHCI_MMIO_BYTES,
                             &rb_hi) == 0) {
        rb_erdp = (uint64_t)rb_lo | ((uint64_t)rb_hi << 32);
        g_pi4_xhci_dma_setup_erdp = rb_erdp;
        g_pi4_xhci_dma_setup_mask |= PI4_XHCI_DMA_SETUP_ERDP;
    }
    g_pi4_xhci_dma_setup_mismatch = pi4_xhci_dma_setup_mismatch_mask(
            g_pi4_xhci_dma_setup_mask, rb_iman, rb_imod, rb_config, rb_dcbaa,
            rb_erstsz, rb_erstba, rb_erdp,
            layout.max_slots, dcbaa_bus, 1, erst_bus, event_bus);
    if (!pi4_xhci_dma_setup_readback_ok(g_pi4_xhci_dma_setup_mask,
            rb_iman, rb_imod, rb_config, rb_dcbaa,
            rb_erstsz, rb_erstba, rb_erdp,
            layout.max_slots, dcbaa_bus, 1, erst_bus, event_bus) ||
        g_pi4_xhci_dma_setup_mismatch) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_READBACK_FAIL;
        goto preflight_restore;
    }

    bme_may_be_set = 1;
    if (pi4_pcie_write_config32(h, g_pi4_link_status, 0x04,
                                enabled_pci) != 0 ||
        pi4_pcie_read_config32(h, g_pi4_link_status, 0x04, &verify) != 0 ||
        (uint16_t)verify != enabled_pci ||
        !((uint16_t)verify & PI4_PCI_COMMAND_MASTER)) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_PCI_ENABLE_FAIL;
        goto cleanup;
    }
    g_pi4_xhci_dma_stage = 7; /* BME enabled; no controller command yet. */
    if (pi4_xhci_write_mmio32(op, 0, PI4_XHCI_MMIO_BYTES,
                              pi4_xhci_dma_run_command_value(usbcmd)) != 0) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
        goto cleanup;
    }
    g_pi4_xhci_dma_stage = 8; /* RUN requested; wait for Halted to clear. */
    if (!pi4_xhci_dma_wait_status(op, 4, PI4_XHCI_USBSTS_HALTED, 0,
                                  &g_pi4_xhci_dma_status)) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_RUN_TIMEOUT;
        goto cleanup;
    }
    g_pi4_xhci_dma_run_sts = g_pi4_xhci_dma_status;
    if (g_pi4_xhci_dma_run_sts & PI4_XHCI_USBSTS_HSE) {
        /* HSE means the xHC has already suffered a host-system access fault.
         * Do not ring a doorbell on a controller in that fatal state. */
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_HSE_BEFORE_DB;
        g_pi4_xhci_dma_stage = 15;
        goto cleanup;
    }

    /* A HCRST may leave every connected root port disabled, in which case
     * MFINDEX is allowed to stop. Reset one already-connected, powered port
     * first so the independent MFINDEX-wrap test gets a chance to observe a
     * complete counter interval. The bounded test never enables a slot or
     * transfers USB data. */
    if (pi4_xhci_dma_port_reset_event_probe(bar, op, rt, event_memory, event_bus,
                                             &event_cursor) < 0) {
        if (g_pi4_xhci_dma_state == PI4_XHCI_DMA_NOT_RUN)
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
        goto cleanup;
    }

    /* Exercise event-ring device writes independently of command fetching.
     * EWE requests a primary-interrupter MFINDEX Wrap Event. If MFINDEX wraps
     * and its event appears, the xHC has written an Event TRB through DMA even
     * before the command-ring doorbell is used. The reset above activates the
     * connected port before this full, bounded wrap interval. */
    g_pi4_xhci_dma_stage = 9; /* Running; check primary event-ring DMA. */
    if (pi4_xhci_read_mmio32(rt, 0x00, PI4_XHCI_MMIO_BYTES,
                             &g_pi4_xhci_dma_mfindex_start) != 0) {
        g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_MMIO_FAIL;
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
        goto cleanup;
    }
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(start));
    if (frequency && frequency <= UINT64_MAX / 3u) {
        uint64_t budget = frequency * 3u;
        uint32_t previous = g_pi4_xhci_dma_mfindex_start &
                            PI4_XHCI_MFINDEX_MASK;
        uint64_t wrap_tick = 0;
        int saw_counter_progress = 0, saw_wrap = 0, wrap_event = 0;
        for (;;) {
            uint8_t *entry = event_memory +
                (uint64_t)event_cursor.index * 16u;
            volatile uint32_t *event = (volatile uint32_t *)(void *)entry;
            uint32_t current;
            __asm__ volatile("dc ivac, %0" :: "r"(entry) : "memory");
            __asm__ volatile("dsb sy" ::: "memory");
            control = event[3];
            if (pi4_xhci_dma_event_ready(control, &event_cursor)) {
                status = event[2];
                __asm__ volatile("dc ivac, %0" :: "r"(entry) : "memory");
                __asm__ volatile("dsb sy" ::: "memory");
                control = event[3];
                if (pi4_xhci_dma_mfindex_event_ok(status, control)) {
                    g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_EVENT_PASS;
                    g_pi4_xhci_dma_wrap_status = status;
                    g_pi4_xhci_dma_wrap_control = control;
                    g_pi4_xhci_dma_events++;
                    (void)pi4_xhci_dma_event_cursor_advance(&event_cursor);
                    if (!pi4_xhci_dma_write64(rt, 0x38,
                            PI4_XHCI_MMIO_BYTES,
                            pi4_xhci_dma_event_dequeue(event_bus,
                                                       &event_cursor))) {
                        g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_MMIO_FAIL;
                        g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
                        goto cleanup;
                    }
                    wrap_event = 1;
                    break;
                }
                if (((control >> 10) & 0x3fu) == PI4_XHCI_TRB_TYPE_PSC_EVT) {
                    g_pi4_xhci_dma_events++;
                    (void)pi4_xhci_dma_event_cursor_advance(&event_cursor);
                    if (!pi4_xhci_dma_write64(rt, 0x38,
                            PI4_XHCI_MMIO_BYTES,
                            pi4_xhci_dma_event_dequeue(event_bus,
                                                       &event_cursor))) {
                        g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_MMIO_FAIL;
                        g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
                        goto cleanup;
                    }
                    continue;
                }
                g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_UNEXPECTED;
                g_pi4_xhci_dma_wrap_status = status;
                g_pi4_xhci_dma_wrap_control = control;
                g_pi4_xhci_dma_events++;
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_BAD_EVENT;
                goto cleanup;
            }
            if (pi4_xhci_read_mmio32(rt, 0x00, PI4_XHCI_MMIO_BYTES,
                                     &g_pi4_xhci_dma_mfindex_end) != 0) {
                g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_MMIO_FAIL;
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
                goto cleanup;
            }
            current = g_pi4_xhci_dma_mfindex_end & PI4_XHCI_MFINDEX_MASK;
            if (current != previous) saw_counter_progress = 1;
            if (pi4_xhci_dma_mfindex_wrapped(previous, current) && !saw_wrap) {
                saw_wrap = 1;
                __asm__ volatile("mrs %0, cntpct_el0" : "=r"(wrap_tick));
            }
            previous = current;
            __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
            if (saw_wrap && now - wrap_tick >= frequency / 20u) break;
            if (now - start >= budget) break;
            __asm__ volatile("yield");
        }
        if (wrap_event) {
            /* Event TRB successfully proved an xHC write to event memory. */
        } else if (saw_wrap) {
            g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_MISSING_EVENT;
        } else if (!saw_counter_progress) {
            g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_NO_COUNTER;
        } else {
            g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_NOT_OBSERVED;
        }
    } else {
        g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_NO_COUNTER;
    }
    if (g_pi4_xhci_dma_mfindex_end == 0xffffffffu &&
        pi4_xhci_read_mmio32(rt, 0x00, PI4_XHCI_MMIO_BYTES,
                             &g_pi4_xhci_dma_mfindex_end) != 0) {
        g_pi4_xhci_dma_wrap_state = PI4_XHCI_WRAP_MMIO_FAIL;
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
        goto cleanup;
    }
    /* Disable periodic wrap events after the bounded independent check. */
    if (pi4_xhci_write_mmio32(op, 0, PI4_XHCI_MMIO_BYTES,
                              (usbcmd & ~PI4_XHCI_USBCMD_EWE) |
                              PI4_XHCI_USBCMD_RUN) != 0 ||
        pi4_xhci_read_mmio32(op, 4, PI4_XHCI_MMIO_BYTES, &status) != 0) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_MMIO_FAIL;
        goto cleanup;
    }
    if (status & PI4_XHCI_USBSTS_HSE) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_HSE_BEFORE_DB;
        g_pi4_xhci_dma_stage = 15;
        goto cleanup;
    }

    /* Isolate Enable Slot from the Link-TRB cycle transition. Submit the
     * protocol-correct command at aligned ring index 248 and its initial
     * producer cycle. Later command entries remain invalid until each is
     * explicitly published, so the xHC cannot drain future No-ops before
     * the probe replaces those entries with real commands. */
    {
        unsigned protocol_major = 0, slot_type = 0;
        unsigned protocol_cap_offset = 0;
        unsigned command_index = PI4_XHCI_DMA_COMMAND_ENABLE_SLOT_TRB;
        uint8_t *enable_trb = command_memory +
                              (uint64_t)command_index * 16u;
        int command_rc;

        if (!g_pi4_xhci_dma_port_ready) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_ADDRESS_SKIP_PORT;
            goto cleanup;
        }
        if (pi4_xhci_dma_port_slot_type(bar,
                g_pi4_xhci_dma_port_event_port,
                (g_pi4_xhci_caps[1] >> 24) & 0xffu,
                &protocol_major, &slot_type,
                &protocol_cap_offset) != 0) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_PROTOCOL_FAIL;
            goto cleanup;
        }
        g_pi4_xhci_dma_protocol_cap_offset = protocol_cap_offset;
        g_pi4_xhci_dma_protocol_major = protocol_major;
        g_pi4_xhci_dma_protocol_slot_type = slot_type;
        if (pi4_xhci_dma_enable_slot_seed(
                (uint32_t *)(void *)enable_trb, slot_type, 1u) != 0) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_ENABLE_SLOT_BAD;
            goto cleanup;
        }
        g_pi4_xhci_dma_enable_trb_control =
            ((uint32_t *)(void *)enable_trb)[3];
        pi4_dma_clean_bytes(enable_trb, 16u);
        command_rc = pi4_xhci_dma_command_submit_wait(op, rt, db,
            event_memory, event_bus, command_bus, command_index, UINT32_MAX,
            &event_cursor, &slot_id,
            &g_pi4_xhci_dma_enable_slot_status,
            &g_pi4_xhci_dma_enable_slot_control);
        if (command_rc <= 0) {
            g_pi4_xhci_dma_state = command_rc == 0
                ? PI4_XHCI_DMA_ENABLE_SLOT_TIMEOUT
                : (command_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                     : PI4_XHCI_DMA_ENABLE_SLOT_BAD);
            goto cleanup;
        }
        if (!slot_id || slot_id > layout.max_slots) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_ENABLE_SLOT_BAD;
            goto cleanup;
        }
        g_pi4_xhci_dma_slot_id = slot_id;
        g_pi4_xhci_dma_slot_enabled = 1;
        g_pi4_xhci_dma_command_completions++;
        dcbaa[slot_id] = output_context_bus;
        pi4_dma_clean_bytes((uint8_t *)(void *)&dcbaa[slot_id],
                            sizeof(dcbaa[slot_id]));
        __asm__ volatile("dsb sy" ::: "memory");
    }
    /* The physical BSR=1 pre-Link run accepted Address Device with the slot
     * still in Default state. Now issue the same command at index 249/cycle 1
     * with BSR=0 to exercise USB SET_ADDRESS and the Addressed slot state. */
    if (!g_pi4_xhci_dma_port_ready) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_ADDRESS_SKIP_PORT;
        goto cleanup;
    }
    {
        unsigned context_size = pi4_xhci_dma_context_size(g_pi4_xhci_caps[3]);
        unsigned speed_id = pi4_xhci_dma_port_speed_id(
            g_pi4_xhci_dma_port_event_after);
        unsigned address_index = PI4_XHCI_DMA_COMMAND_ADDRESS_DEVICE_TRB;
        uint8_t *address_trb = command_memory + (uint64_t)address_index * 16u;
        int command_rc;

        if (!slot_id || slot_id > layout.max_slots) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_ENABLE_SLOT_BAD;
            goto cleanup;
        }
        if (pi4_xhci_dma_address_context_seed(input_context,
                PI4_XHCI_DMA_CONTEXT_BYTES, context_size, speed_id,
                g_pi4_xhci_dma_port_event_port, ep0_ring_bus) != 0) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_ADDRESS_DEVICE_BAD;
            goto cleanup;
        }
        pi4_dma_clean_bytes(input_context, (uint64_t)context_size * 3u);
        pi4_dma_clean_invalidate_bytes(output_context,
                                       PI4_XHCI_DMA_CONTEXT_BYTES);
        pi4_dma_clean_invalidate_bytes(ep0_ring,
                                       PI4_XHCI_DMA_RING_ALLOC_BYTES);
        __asm__ volatile("dsb sy" ::: "memory");

        /* Preserve the accepted EP0-only context and capture its fields. */
        {
            const uint32_t *in = (const uint32_t *)(const void *)input_context;
            const uint32_t *slot = (const uint32_t *)(const void *)
                (input_context + context_size);
            const uint32_t *ep0 = (const uint32_t *)(const void *)
                (input_context + context_size * 2u);
            g_pi4_xhci_dma_address_input_bus = input_context_bus;
            g_pi4_xhci_dma_address_output_bus = output_context_bus;
            g_pi4_xhci_dma_address_ep0_bus = ep0_ring_bus;
            g_pi4_xhci_dma_address_dcbaa_slot = dcbaa[slot_id];
            g_pi4_xhci_dma_address_context_words[0] = in[1];
            g_pi4_xhci_dma_address_context_words[1] = slot[0];
            g_pi4_xhci_dma_address_context_words[2] = slot[1];
            g_pi4_xhci_dma_address_context_words[3] = ep0[1];
            g_pi4_xhci_dma_address_context_words[4] = ep0[2];
            g_pi4_xhci_dma_address_context_words[5] = ep0[3];
            g_pi4_xhci_dma_address_context_words[6] = ep0[4];
        }

        if (pi4_xhci_dma_address_device_seed(
                (uint32_t *)(void *)address_trb,
                input_context_bus, slot_id, 1u, 0u) != 0) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_ADDRESS_DEVICE_BAD;
            goto cleanup;
        }
        g_pi4_xhci_dma_address_trb_control =
            ((uint32_t *)(void *)address_trb)[3];
        pi4_dma_clean_bytes(address_trb, 16u);
        g_pi4_xhci_dma_stage = 13; /* BSR=0, pre-Link SET_ADDRESS. */
        command_rc = pi4_xhci_dma_command_submit_wait(op, rt, db,
            event_memory, event_bus, command_bus, address_index, slot_id,
            &event_cursor, &slot_id,
            &g_pi4_xhci_dma_address_status,
            &g_pi4_xhci_dma_address_control);
        if (command_rc <= 0) {
            g_pi4_xhci_dma_state = command_rc == 0
                ? PI4_XHCI_DMA_ADDRESS_DEVICE_TIMEOUT
                : (command_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                     : PI4_XHCI_DMA_ADDRESS_DEVICE_BAD);
            goto cleanup;
        }
        g_pi4_xhci_dma_command_completions++;
        __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
        pi4_dma_invalidate_bytes(output_context,
                                 PI4_XHCI_DMA_CONTEXT_BYTES);
        g_pi4_xhci_dma_device_address =
            pi4_xhci_dma_output_usb_address(output_context);
        g_pi4_xhci_dma_device_slot_state =
            pi4_xhci_dma_output_slot_state(output_context);
        if (g_pi4_xhci_dma_device_address == 0u ||
            g_pi4_xhci_dma_device_slot_state != 2u) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_ADDRESS_DEVICE_BAD;
            goto cleanup;
        }
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_ADDRESS_DEVICE_PASS;
    }
    /* With the slot Addressed, read the full device descriptor and then the
     * configuration header. This identifies the attached device and confirms
     * that EP0 can retire consecutive control TDs. */
    {
        uint32_t *setup_trb = (uint32_t *)(void *)(ep0_ring + 0u);
        uint32_t *data_trb = (uint32_t *)(void *)(ep0_ring + 16u);
        uint32_t *status_trb = (uint32_t *)(void *)(ep0_ring + 32u);
        uint32_t *config_setup_trb = (uint32_t *)(void *)(ep0_ring + 48u);
        uint32_t *config_data_trb = (uint32_t *)(void *)(ep0_ring + 64u);
        uint32_t *config_status_trb = (uint32_t *)(void *)(ep0_ring + 80u);
        uint32_t *set_config_setup_trb = (uint32_t *)(void *)(ep0_ring + 96u);
        uint32_t *set_config_status_trb = (uint32_t *)(void *)(ep0_ring + 112u);
        uint32_t *hub_setup_trb = (uint32_t *)(void *)(ep0_ring + 128u);
        uint32_t *hub_data_trb = (uint32_t *)(void *)(ep0_ring + 144u);
        uint32_t *hub_status_trb = (uint32_t *)(void *)(ep0_ring + 160u);
        uint8_t *config_buffer = control_buffer + 64u;
        uint8_t *hub_buffer = control_buffer + 128u;
        int transfer_rc;

        if (!g_pi4_xhci_dma_device_address ||
            g_pi4_xhci_dma_device_slot_state != 2u ||
            pi4_xhci_dma_get_descriptor_setup_seed(setup_trb, 1u, 0u,
                                                   18u, 1u) != 0 ||
            pi4_xhci_dma_data_stage_seed(data_trb, control_buffer_bus, 18u,
                                         1u) != 0 ||
            pi4_xhci_dma_status_stage_seed(status_trb, 1u) != 0) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_CONTROL_TRANSFER_BAD;
            goto cleanup;
        }
        g_pi4_xhci_dma_setup_trb_control = setup_trb[3];
        g_pi4_xhci_dma_data_trb_control = data_trb[3];
        g_pi4_xhci_dma_status_trb_control = status_trb[3];
        g_pi4_xhci_dma_control_buffer_bus = control_buffer_bus;
        pi4_dma_clean_bytes(ep0_ring, PI4_XHCI_DMA_RING_ALLOC_BYTES);
        pi4_dma_clean_invalidate_bytes(control_buffer,
                                       PI4_XHCI_DMA_CONTROL_BUFFER_BYTES);
        __asm__ volatile("dsb sy" ::: "memory");
        g_pi4_xhci_dma_stage = 14; /* EP0 GET_DESCRIPTOR(device), 18 bytes. */
        transfer_rc = pi4_xhci_dma_control_transfer_submit_wait(
            op, rt, db, event_memory, event_bus,
            ep0_ring_bus + 32u, slot_id, 1u, &event_cursor);
        if (transfer_rc <= 0) {
            g_pi4_xhci_dma_state = transfer_rc == 0
                ? PI4_XHCI_DMA_CONTROL_TRANSFER_TIMEOUT
                : (transfer_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                      : PI4_XHCI_DMA_CONTROL_TRANSFER_BAD);
            goto cleanup;
        }
        __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
        pi4_dma_invalidate_bytes(control_buffer,
                                 PI4_XHCI_DMA_CONTROL_BUFFER_BYTES);
        g_pi4_xhci_dma_descriptor_prefix[0] =
            ((uint32_t *)(void *)control_buffer)[0];
        g_pi4_xhci_dma_descriptor_prefix[1] =
            ((uint32_t *)(void *)control_buffer)[1];
        if (!pi4_xhci_dma_child_device_descriptor_ok(control_buffer, 18u,
                pi4_xhci_dma_port_speed_id(g_pi4_xhci_dma_port_event_after))) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_CONTROL_TRANSFER_BAD;
            goto cleanup;
        }
        g_pi4_xhci_dma_device_bcd_usb = control_buffer[2] |
            ((uint32_t)control_buffer[3] << 8);
        g_pi4_xhci_dma_device_class = control_buffer[4];
        g_pi4_xhci_dma_device_subclass = control_buffer[5];
        g_pi4_xhci_dma_device_protocol = control_buffer[6];
        g_pi4_xhci_dma_device_ep0_mps = control_buffer[7];
        g_pi4_xhci_dma_device_vendor = control_buffer[8] |
            ((uint32_t)control_buffer[9] << 8);
        g_pi4_xhci_dma_device_product = control_buffer[10] |
            ((uint32_t)control_buffer[11] << 8);
        g_pi4_xhci_dma_device_bcd_device = control_buffer[12] |
            ((uint32_t)control_buffer[13] << 8);
        g_pi4_xhci_dma_device_configurations = control_buffer[17];

        if (pi4_xhci_dma_get_descriptor_setup_seed(config_setup_trb,
                2u, 0u, 9u, 1u) != 0 ||
            pi4_xhci_dma_data_stage_seed(config_data_trb,
                control_buffer_bus + 64u, 9u, 1u) != 0 ||
            pi4_xhci_dma_status_stage_seed(config_status_trb, 1u) != 0) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_CONTROL_TRANSFER_BAD;
            goto cleanup;
        }
        g_pi4_xhci_dma_setup_trb_control = config_setup_trb[3];
        g_pi4_xhci_dma_data_trb_control = config_data_trb[3];
        g_pi4_xhci_dma_status_trb_control = config_status_trb[3];
        g_pi4_xhci_dma_config_buffer_bus = control_buffer_bus + 64u;
        pi4_dma_clean_bytes(ep0_ring, PI4_XHCI_DMA_RING_ALLOC_BYTES);
        pi4_dma_clean_invalidate_bytes(config_buffer, 64u);
        __asm__ volatile("dsb sy" ::: "memory");
        g_pi4_xhci_dma_stage = 15; /* GET_DESCRIPTOR(configuration), header. */
        transfer_rc = pi4_xhci_dma_control_transfer_submit_wait(
            op, rt, db, event_memory, event_bus,
            ep0_ring_bus + 80u, slot_id, 1u, &event_cursor);
        if (transfer_rc <= 0) {
            g_pi4_xhci_dma_state = transfer_rc == 0
                ? PI4_XHCI_DMA_CONTROL_TRANSFER_TIMEOUT
                : (transfer_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                      : PI4_XHCI_DMA_CONTROL_TRANSFER_BAD);
            goto cleanup;
        }
        __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
        pi4_dma_invalidate_bytes(config_buffer, 64u);
        if (!pi4_xhci_dma_configuration_header_ok(config_buffer, 9u)) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_CONTROL_TRANSFER_BAD;
            goto cleanup;
        }
        g_pi4_xhci_dma_config_total_length = config_buffer[2] |
            ((uint32_t)config_buffer[3] << 8);
        g_pi4_xhci_dma_config_interfaces = config_buffer[4];
        g_pi4_xhci_dma_config_value = config_buffer[5];
        g_pi4_xhci_dma_config_attributes = config_buffer[7];
        g_pi4_xhci_dma_config_max_power = config_buffer[8];
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_CONFIG_HEADER_PASS;

        /* SuperSpeed devices are attached to independent xHCI root ports,
         * outside the USB 2 hub. Read their interface chain directly. Also
         * avoid sending Hub requests to any directly attached non-hub. */
        if (pi4_xhci_dma_port_speed_id(g_pi4_xhci_dma_port_event_after) == 4u ||
            g_pi4_xhci_dma_device_class != 9u) {
            pi4_xhci_dma_direct_root_config_probe(op, rt, db,
                event_memory, event_bus, &event_cursor, slot_id,
                ep0_ring, ep0_ring_bus, control_buffer, control_buffer_bus);
            if (g_pi4_xhci_dma_state == PI4_XHCI_DMA_ROOT_BOT_EP0_PASS &&
                pi4_xhci_dma_port_speed_id(g_pi4_xhci_dma_port_event_after) == 4u)
                pi4_xhci_dma_direct_root_bulk_probe(op, rt, db,
                    event_memory, event_bus, &event_cursor, slot_id,
                    pi4_xhci_dma_context_size(g_pi4_xhci_caps[3]),
                    command_memory, command_bus, input_context, input_context_bus,
                    output_context, child_bulk_in_ring, child_bulk_in_bus,
                    child_bulk_out_ring, child_bulk_out_bus,
                    control_buffer, control_buffer_bus);
            goto cleanup;
        }

        /* Configure the hub using the value returned by its configuration
         * header, then read its class-specific Hub descriptor. No hub port is
         * powered or reset in this pass. */
        if (pi4_xhci_dma_setup_stage_seed(set_config_setup_trb,
                0x00u, 0x09u, g_pi4_xhci_dma_config_value, 0u, 0u,
                0u, 1u) != 0 ||
            pi4_xhci_dma_status_stage_seed_direction(
                set_config_status_trb, 1u, 1u) != 0) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_CONTROL_TRANSFER_BAD;
            goto cleanup;
        }
        g_pi4_xhci_dma_set_config_setup_control =
            set_config_setup_trb[3];
        g_pi4_xhci_dma_set_config_status_control =
            set_config_status_trb[3];
        g_pi4_xhci_dma_setup_trb_control =
            g_pi4_xhci_dma_set_config_setup_control;
        g_pi4_xhci_dma_status_trb_control =
            g_pi4_xhci_dma_set_config_status_control;
        pi4_dma_clean_bytes(ep0_ring, PI4_XHCI_DMA_RING_ALLOC_BYTES);
        __asm__ volatile("dsb sy" ::: "memory");
        g_pi4_xhci_dma_stage = 16; /* SET_CONFIGURATION, no data stage. */
        transfer_rc = pi4_xhci_dma_control_transfer_submit_wait(
            op, rt, db, event_memory, event_bus,
            ep0_ring_bus + 112u, slot_id, 1u, &event_cursor);
        if (transfer_rc <= 0) {
            g_pi4_xhci_dma_state = transfer_rc == 0
                ? PI4_XHCI_DMA_CONTROL_TRANSFER_TIMEOUT
                : (transfer_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                      : PI4_XHCI_DMA_CONTROL_TRANSFER_BAD);
            goto cleanup;
        }
        g_pi4_xhci_dma_set_config_event_parameter =
            g_pi4_xhci_dma_transfer_parameter;
        g_pi4_xhci_dma_set_config_event_status =
            g_pi4_xhci_dma_transfer_status;
        g_pi4_xhci_dma_set_config_event_control =
            g_pi4_xhci_dma_transfer_control;
        g_pi4_xhci_dma_set_config_event_checks =
            g_pi4_xhci_dma_transfer_checks;
        g_pi4_xhci_dma_set_config_event_skipped =
            g_pi4_xhci_dma_transfer_skipped;

        if (pi4_xhci_dma_setup_stage_seed(hub_setup_trb,
                0xa0u, 0x06u, 0x2900u, 0u, 9u, 3u, 1u) != 0 ||
            pi4_xhci_dma_data_stage_seed(hub_data_trb,
                control_buffer_bus + 128u, 9u, 1u) != 0 ||
            pi4_xhci_dma_status_stage_seed(hub_status_trb, 1u) != 0) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_CONTROL_TRANSFER_BAD;
            goto cleanup;
        }
        g_pi4_xhci_dma_setup_trb_control = hub_setup_trb[3];
        g_pi4_xhci_dma_data_trb_control = hub_data_trb[3];
        g_pi4_xhci_dma_status_trb_control = hub_status_trb[3];
        g_pi4_xhci_dma_hub_buffer_bus = control_buffer_bus + 128u;
        g_pi4_xhci_dma_hub_descriptor_setup[0] = hub_setup_trb[0];
        g_pi4_xhci_dma_hub_descriptor_setup[1] = hub_setup_trb[1];
        g_pi4_xhci_dma_hub_descriptor_data_bus =
            (uint64_t)hub_data_trb[0] | ((uint64_t)hub_data_trb[1] << 32);
        g_pi4_xhci_dma_hub_descriptor_data_length = hub_data_trb[2];
        g_pi4_xhci_dma_hub_descriptor_data_control = hub_data_trb[3];
        g_pi4_xhci_dma_hub_descriptor_status_control = hub_status_trb[3];
        pi4_dma_clean_bytes(ep0_ring, PI4_XHCI_DMA_RING_ALLOC_BYTES);
        pi4_dma_clean_invalidate_bytes(hub_buffer, 64u);
        __asm__ volatile("dsb sy" ::: "memory");
        g_pi4_xhci_dma_stage = 17; /* Class GET_DESCRIPTOR(Hub), 9 bytes. */
        transfer_rc = pi4_xhci_dma_control_transfer_submit_wait(
            op, rt, db, event_memory, event_bus,
            ep0_ring_bus + 160u, slot_id, 1u, &event_cursor);
        g_pi4_xhci_dma_hub_descriptor_transfer_rc =
            (uint32_t)transfer_rc;
        if (transfer_rc <= 0) {
            g_pi4_xhci_dma_state = transfer_rc == 0
                ? PI4_XHCI_DMA_CONTROL_TRANSFER_TIMEOUT
                : (transfer_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                      : PI4_XHCI_DMA_CONTROL_TRANSFER_BAD);
            goto cleanup;
        }
        __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
        pi4_dma_invalidate_bytes(hub_buffer, 64u);
        g_pi4_xhci_dma_hub_descriptor_validated =
            pi4_xhci_dma_hub_descriptor_ok(hub_buffer, 9u) ? 1u : 0u;
        for (unsigned i = 0; i < 4; i++)
            g_pi4_xhci_dma_hub_descriptor_words[i] =
                ((uint32_t *)(void *)hub_buffer)[i];
        if (!g_pi4_xhci_dma_hub_descriptor_validated) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_CONTROL_TRANSFER_BAD;
            goto cleanup;
        }
        g_pi4_xhci_dma_hub_ports = hub_buffer[2];
        g_pi4_xhci_dma_hub_characteristics = hub_buffer[3] |
            ((uint32_t)hub_buffer[4] << 8);
        g_pi4_xhci_dma_hub_power_delay = hub_buffer[5];
        g_pi4_xhci_dma_hub_current = hub_buffer[6];
        g_pi4_xhci_dma_hub_removable = hub_buffer[7];
        g_pi4_xhci_dma_hub_power_mask = hub_buffer[8];
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_DESCRIPTOR_PASS;

        /* A ganged hub normally powers its whole gang from one PORT_POWER
         * request; individually switched hubs need one request per port.
         * This probe adds a port-4 retry for the observed ganged-hub mismatch. */
        unsigned hub_power_request_count =
            pi4_xhci_dma_hub_power_request_count(
                g_pi4_xhci_dma_hub_characteristics,
                g_pi4_xhci_dma_hub_ports);
        unsigned hub_power_transfer_count =
            pi4_xhci_dma_hub_power_probe_request_count(
                g_pi4_xhci_dma_hub_characteristics,
                g_pi4_xhci_dma_hub_ports);
        if (g_pi4_xhci_dma_hub_ports != 4u ||
            !hub_power_request_count) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_CONTROL_TRANSFER_BAD;
            goto cleanup;
        }
        g_pi4_xhci_dma_stage = 18; /* Hub downstream PORT_POWER feature. */
        for (unsigned i = 0; i < hub_power_transfer_count; i++) {
            unsigned port = i + 1u;
            unsigned trb_index = 11u + i * 2u;
            uint32_t *power_setup = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)trb_index * 16u);
            uint32_t *power_status = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)(trb_index + 1u) * 16u);

            g_pi4_xhci_dma_hub_probe_phase = 1u; /* PORT_POWER */
            g_pi4_xhci_dma_hub_probe_port = port;
            g_pi4_xhci_dma_hub_request_type = 0x23u;
            g_pi4_xhci_dma_hub_request = 0x03u;
            g_pi4_xhci_dma_hub_request_value =
                PI4_XHCI_HUB_FEATURE_PORT_POWER;
            g_pi4_xhci_dma_hub_request_index = port;
            g_pi4_xhci_dma_hub_request_length = 0u;
            g_pi4_xhci_dma_hub_expected_status_bus = ep0_ring_bus +
                (uint64_t)(trb_index + 1u) * 16u;
            if (pi4_xhci_dma_setup_stage_seed(power_setup, 0x23u, 0x03u,
                    PI4_XHCI_HUB_FEATURE_PORT_POWER, port, 0u, 0u, 1u) != 0 ||
                pi4_xhci_dma_status_stage_seed_direction(
                    power_status, 1u, 1u) != 0) {
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_CONTROL_TRANSFER_BAD;
                goto cleanup;
            }
            g_pi4_xhci_dma_setup_trb_control = power_setup[3];
            g_pi4_xhci_dma_status_trb_control = power_status[3];
            pi4_dma_clean_bytes(ep0_ring, PI4_XHCI_DMA_RING_ALLOC_BYTES);
            __asm__ volatile("dsb sy" ::: "memory");
            g_pi4_xhci_dma_stage = 18;
            transfer_rc = pi4_xhci_dma_control_transfer_submit_wait(
                op, rt, db, event_memory, event_bus,
                ep0_ring_bus + (uint64_t)(trb_index + 1u) * 16u,
                slot_id, 1u, &event_cursor);
            if (transfer_rc <= 0) {
                g_pi4_xhci_dma_state = transfer_rc == 0
                    ? PI4_XHCI_DMA_CONTROL_TRANSFER_TIMEOUT
                    : (transfer_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                          : PI4_XHCI_DMA_CONTROL_TRANSFER_BAD);
                goto cleanup;
            }
            g_pi4_xhci_dma_hub_port_power_events |= 1u << (port - 1u);
        }
        /* Sweep PORT_POWER across every downstream port, even though the hub
         * descriptor claims ganged switching. The last run showed only the
         * explicitly requested ports powered. Repeated power-on requests
         * distinguish descriptor mismatch from status/DMA behavior; this
         * does not clear power, reset a child, or send HID traffic. */
        if (!pi4_xhci_dma_wait_ms(g_pi4_xhci_dma_hub_power_delay * 2u)) {
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_POWER_DELAY_FAIL;
            goto cleanup;
        }

        g_pi4_xhci_dma_stage = 19; /* Hub class GET_STATUS per downstream port. */
        for (unsigned i = 0; i < 4u; i++) {
            unsigned port = i + 1u;
            unsigned trb_index = pi4_xhci_dma_hub_port_status_trb_index(
                hub_power_transfer_count, i);
            uint32_t *port_setup = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)trb_index * 16u);
            uint32_t *port_data = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)(trb_index + 1u) * 16u);
            uint32_t *port_status_trb = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)(trb_index + 2u) * 16u);
            uint8_t *port_buffer = control_buffer + 256u + i * 16u;
            uint64_t port_buffer_bus = control_buffer_bus + 256u + i * 16u;

            g_pi4_xhci_dma_hub_probe_phase = 2u; /* GET_STATUS */
            g_pi4_xhci_dma_hub_probe_port = port;
            g_pi4_xhci_dma_hub_request_type = 0xa3u;
            g_pi4_xhci_dma_hub_request = 0x00u;
            g_pi4_xhci_dma_hub_request_value = 0u;
            g_pi4_xhci_dma_hub_request_index = port;
            g_pi4_xhci_dma_hub_request_length = 4u;
            g_pi4_xhci_dma_hub_status_trb_index = trb_index;
            g_pi4_xhci_dma_hub_status_buffer_bus = port_buffer_bus;
            g_pi4_xhci_dma_hub_setup_trb_bus = ep0_ring_bus +
                (uint64_t)trb_index * 16u;
            g_pi4_xhci_dma_hub_data_trb_bus = ep0_ring_bus +
                (uint64_t)(trb_index + 1u) * 16u;
            g_pi4_xhci_dma_hub_expected_status_bus = ep0_ring_bus +
                (uint64_t)(trb_index + 2u) * 16u;
            if (pi4_xhci_dma_setup_stage_seed(port_setup, 0xa3u, 0x00u,
                    0u, port, 4u, 3u, 1u) != 0 ||
                pi4_xhci_dma_data_stage_seed(port_data, port_buffer_bus,
                    4u, 1u) != 0 ||
                pi4_xhci_dma_status_stage_seed(port_status_trb, 1u) != 0) {
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_CONTROL_TRANSFER_BAD;
                goto cleanup;
            }
            for (unsigned j = 0; j < 4u; j++) port_buffer[j] = 0;
            g_pi4_xhci_dma_setup_trb_control = port_setup[3];
            g_pi4_xhci_dma_data_trb_control = port_data[3];
            g_pi4_xhci_dma_status_trb_control = port_status_trb[3];
            pi4_dma_clean_bytes(ep0_ring, PI4_XHCI_DMA_RING_ALLOC_BYTES);
            pi4_dma_clean_invalidate_bytes(port_buffer, 64u);
            __asm__ volatile("dsb sy" ::: "memory");
            transfer_rc = pi4_xhci_dma_control_transfer_submit_wait(
                op, rt, db, event_memory, event_bus,
                ep0_ring_bus + (uint64_t)(trb_index + 2u) * 16u,
                slot_id, 1u, &event_cursor);
            if (transfer_rc <= 0) {
                g_pi4_xhci_dma_state = transfer_rc == 0
                    ? PI4_XHCI_DMA_CONTROL_TRANSFER_TIMEOUT
                    : (transfer_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                          : PI4_XHCI_DMA_CONTROL_TRANSFER_BAD);
                if (transfer_rc == 0 && slot_id) {
                    unsigned stop_index =
                        PI4_XHCI_DMA_COMMAND_STOP_ENDPOINT_TRB;
                    uint32_t *stop_trb = (uint32_t *)(void *)(command_memory +
                        (uint64_t)stop_index * 16u);
                    int stop_rc;

                    if (pi4_xhci_dma_stop_endpoint_seed(stop_trb, slot_id,
                            1u, 1u) != 0) {
                        g_pi4_xhci_dma_stop_endpoint_rc = (uint32_t)-3;
                    } else {
                        g_pi4_xhci_dma_stop_endpoint_trb_control =
                            stop_trb[3];
                        pi4_dma_clean_bytes((uint8_t *)(void *)stop_trb,
                                            16u);
                        stop_rc = pi4_xhci_dma_stop_endpoint_submit_wait(
                            op, rt, db, event_memory, event_bus,
                            command_bus, stop_index, slot_id, 1u,
                            &event_cursor);
                        g_pi4_xhci_dma_stop_endpoint_rc = (uint32_t)stop_rc;
                        if (stop_rc == 1) {
                            unsigned context_size =
                                pi4_xhci_dma_context_size(g_pi4_xhci_caps[3]);
                            const uint8_t *ep0_context = output_context +
                                                        context_size;
                            __asm__ volatile("dmb oshld\n\tdsb sy" :::
                                             "memory");
                            pi4_dma_invalidate_bytes(output_context,
                                PI4_XHCI_DMA_CONTEXT_BYTES);
                            for (unsigned j = 0; j < 5u; j++)
                                g_pi4_xhci_dma_stop_output_ep0_context[j] =
                                    ((const uint32_t *)(const void *)
                                        ep0_context)[j];
                            g_pi4_xhci_dma_stop_output_ep0_state =
                                g_pi4_xhci_dma_stop_output_ep0_context[0] &
                                7u;
                            g_pi4_xhci_dma_stop_output_ep0_dcs =
                                g_pi4_xhci_dma_stop_output_ep0_context[2] &
                                1u;
                            g_pi4_xhci_dma_stop_output_ep0_dequeue_bus =
                                ((uint64_t)
                                    g_pi4_xhci_dma_stop_output_ep0_context[3]
                                    << 32) |
                                (g_pi4_xhci_dma_stop_output_ep0_context[2] &
                                 ~0x0fu);
                            g_pi4_xhci_dma_stop_output_ep0_context_valid = 1u;
                        }
                    }
                }
                goto cleanup;
            }
            __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
            pi4_dma_invalidate_bytes(port_buffer, 64u);
            g_pi4_xhci_dma_hub_port_status[i] = port_buffer[0] |
                ((uint32_t)port_buffer[1] << 8);
            g_pi4_xhci_dma_hub_port_change[i] = port_buffer[2] |
                ((uint32_t)port_buffer[3] << 8);
            g_pi4_xhci_dma_hub_port_status_events |= 1u << i;
        }
        g_pi4_xhci_dma_hub_port_unpowered_mask =
            pi4_xhci_dma_hub_port_unpowered_mask(
                g_pi4_xhci_dma_hub_port_status,
                g_pi4_xhci_dma_hub_port_status_events);
        g_pi4_xhci_dma_hub_port_connected_mask =
            pi4_xhci_dma_hub_port_connected_mask(
                g_pi4_xhci_dma_hub_port_status,
                g_pi4_xhci_dma_hub_port_status_events);
        g_pi4_xhci_dma_state = g_pi4_xhci_dma_hub_port_unpowered_mask
            ? PI4_XHCI_DMA_HUB_PORT_STATUS_FAIL
            : PI4_XHCI_DMA_HUB_PORTS_PASS;

        /* The photos now show a connected downstream child on port 3. Reset
         * only the lowest connected port, then verify the hub's
         * C_PORT_RESET notification and clear that change bit. */
        g_pi4_xhci_dma_stage = 20;
        unsigned child_port = pi4_xhci_dma_hub_first_connected_port(
            g_pi4_xhci_dma_hub_port_connected_mask,
            g_pi4_xhci_dma_hub_ports);
        g_pi4_xhci_dma_hub_child_reset_port = child_port;
        if (!child_port) {
            g_pi4_xhci_dma_hub_child_reset_state =
                PI4_XHCI_HUB_CHILD_RESET_NO_CONNECTED;
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CHILD_NO_CONNECTED;
        } else if (!pi4_xhci_dma_hub_child_port_reset_allowed(
                       g_pi4_xhci_dma_hub_port_status,
                       g_pi4_xhci_dma_hub_port_status_events,
                       child_port)) {
            g_pi4_xhci_dma_hub_child_reset_state =
                PI4_XHCI_HUB_CHILD_RESET_PORT_NOT_READY;
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CHILD_PORT_NOT_READY;
        } else {
            unsigned child_trb_index =
                pi4_xhci_dma_hub_port_status_trb_index(
                    hub_power_transfer_count, 3u) + 3u;
            uint32_t *reset_setup = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)child_trb_index * 16u);
            uint32_t *reset_status = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)(child_trb_index + 1u) * 16u);
            unsigned status_trb_index = child_trb_index + 2u;
            uint8_t *child_buffer = control_buffer + 512u;
            uint64_t child_buffer_bus = control_buffer_bus + 512u;
            uint32_t *child_get_setup, *child_get_data, *child_get_status;
            int child_rc;

            g_pi4_xhci_dma_hub_child_reset_state =
                PI4_XHCI_HUB_CHILD_RESET_REQUEST_FAILED;
            g_pi4_xhci_dma_hub_probe_phase = 3u; /* SET_FEATURE(PORT_RESET). */
            g_pi4_xhci_dma_hub_probe_port = child_port;
            g_pi4_xhci_dma_hub_request_type = 0x23u;
            g_pi4_xhci_dma_hub_request = 0x03u;
            g_pi4_xhci_dma_hub_request_value =
                PI4_XHCI_HUB_FEATURE_PORT_RESET;
            g_pi4_xhci_dma_hub_request_index = child_port;
            g_pi4_xhci_dma_hub_request_length = 0u;
            g_pi4_xhci_dma_hub_expected_status_bus = ep0_ring_bus +
                (uint64_t)(child_trb_index + 1u) * 16u;
            if (pi4_xhci_dma_setup_stage_seed(reset_setup, 0x23u, 0x03u,
                    PI4_XHCI_HUB_FEATURE_PORT_RESET, child_port, 0u, 0u,
                    1u) != 0 ||
                pi4_xhci_dma_status_stage_seed_direction(
                    reset_status, 1u, 1u) != 0) {
                g_pi4_xhci_dma_hub_child_reset_state =
                    PI4_XHCI_HUB_CHILD_RESET_BAD_STATUS;
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CHILD_RESET_BAD;
                goto cleanup;
            }
            g_pi4_xhci_dma_setup_trb_control = reset_setup[3];
            g_pi4_xhci_dma_status_trb_control = reset_status[3];
            pi4_dma_clean_bytes(ep0_ring,
                                PI4_XHCI_DMA_RING_ALLOC_BYTES);
            __asm__ volatile("dsb sy" ::: "memory");
            child_rc = pi4_xhci_dma_control_transfer_submit_wait(
                op, rt, db, event_memory, event_bus,
                g_pi4_xhci_dma_hub_expected_status_bus,
                slot_id, 1u, &event_cursor);
            g_pi4_xhci_dma_hub_child_reset_request_rc =
                (uint32_t)child_rc;
            if (child_rc <= 0) {
                g_pi4_xhci_dma_state = child_rc == 0
                    ? PI4_XHCI_DMA_HUB_CHILD_RESET_TIMEOUT
                    : (child_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                      : PI4_XHCI_DMA_HUB_CHILD_RESET_BAD);
                goto cleanup;
            }

            g_pi4_xhci_dma_hub_child_reset_state =
                PI4_XHCI_HUB_CHILD_RESET_STATUS_TIMEOUT;
            if (!pi4_xhci_dma_wait_ms(100u)) {
                g_pi4_xhci_dma_hub_child_reset_state =
                    PI4_XHCI_HUB_CHILD_RESET_BAD_STATUS;
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CHILD_RESET_BAD;
                goto cleanup;
            }

            /* One bounded GET_STATUS sample after the USB hub reset window. */
            g_pi4_xhci_dma_hub_probe_phase = 4u; /* Reset completion status. */
            g_pi4_xhci_dma_hub_request_type = 0xa3u;
            g_pi4_xhci_dma_hub_request = 0x00u;
            g_pi4_xhci_dma_hub_request_value = 0u;
            g_pi4_xhci_dma_hub_request_index = child_port;
            g_pi4_xhci_dma_hub_request_length = 4u;
            child_get_setup = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)status_trb_index * 16u);
            child_get_data = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)(status_trb_index + 1u) * 16u);
            child_get_status = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)(status_trb_index + 2u) * 16u);
            g_pi4_xhci_dma_hub_status_trb_index = status_trb_index;
            g_pi4_xhci_dma_hub_status_buffer_bus = child_buffer_bus;
            g_pi4_xhci_dma_hub_expected_status_bus = ep0_ring_bus +
                (uint64_t)(status_trb_index + 2u) * 16u;
            if (pi4_xhci_dma_setup_stage_seed(child_get_setup, 0xa3u,
                    0x00u, 0u, child_port, 4u, 3u, 1u) != 0 ||
                pi4_xhci_dma_data_stage_seed(child_get_data,
                    child_buffer_bus, 4u, 1u) != 0 ||
                pi4_xhci_dma_status_stage_seed(child_get_status, 1u) != 0) {
                g_pi4_xhci_dma_hub_child_reset_state =
                    PI4_XHCI_HUB_CHILD_RESET_BAD_STATUS;
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CHILD_RESET_BAD;
                goto cleanup;
            }
            for (unsigned j = 0; j < 4u; j++) child_buffer[j] = 0;
            g_pi4_xhci_dma_setup_trb_control = child_get_setup[3];
            g_pi4_xhci_dma_data_trb_control = child_get_data[3];
            g_pi4_xhci_dma_status_trb_control = child_get_status[3];
            pi4_dma_clean_bytes(ep0_ring,
                                PI4_XHCI_DMA_RING_ALLOC_BYTES);
            pi4_dma_clean_invalidate_bytes(child_buffer, 64u);
            __asm__ volatile("dsb sy" ::: "memory");
            child_rc = pi4_xhci_dma_control_transfer_submit_wait(
                op, rt, db, event_memory, event_bus,
                g_pi4_xhci_dma_hub_expected_status_bus,
                slot_id, 1u, &event_cursor);
            if (child_rc <= 0) {
                g_pi4_xhci_dma_state = child_rc == 0
                    ? PI4_XHCI_DMA_HUB_CHILD_RESET_TIMEOUT
                    : (child_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                      : PI4_XHCI_DMA_HUB_CHILD_RESET_BAD);
                goto cleanup;
            }
            __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
            pi4_dma_invalidate_bytes(child_buffer, 64u);
            g_pi4_xhci_dma_hub_child_reset_status = child_buffer[0] |
                ((uint32_t)child_buffer[1] << 8);
            g_pi4_xhci_dma_hub_child_reset_change = child_buffer[2] |
                ((uint32_t)child_buffer[3] << 8);
            g_pi4_xhci_dma_hub_child_reset_samples = 1u;
            if (!pi4_xhci_dma_hub_port_reset_complete(
                    g_pi4_xhci_dma_hub_child_reset_status,
                    g_pi4_xhci_dma_hub_child_reset_change)) {
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CHILD_RESET_TIMEOUT;
                goto cleanup;
            }

            /* Clear only C_PORT_RESET, then take one final status sample. */
            child_trb_index = status_trb_index + 3u;
            reset_setup = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)child_trb_index * 16u);
            reset_status = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)(child_trb_index + 1u) * 16u);
            g_pi4_xhci_dma_hub_probe_phase = 5u; /* CLEAR_FEATURE(C_RESET). */
            g_pi4_xhci_dma_hub_request_type = 0x23u;
            g_pi4_xhci_dma_hub_request = 0x01u;
            g_pi4_xhci_dma_hub_request_value =
                PI4_XHCI_HUB_FEATURE_C_PORT_RESET;
            g_pi4_xhci_dma_hub_request_index = child_port;
            g_pi4_xhci_dma_hub_request_length = 0u;
            g_pi4_xhci_dma_hub_expected_status_bus = ep0_ring_bus +
                (uint64_t)(child_trb_index + 1u) * 16u;
            if (pi4_xhci_dma_setup_stage_seed(reset_setup, 0x23u, 0x01u,
                    PI4_XHCI_HUB_FEATURE_C_PORT_RESET, child_port,
                    0u, 0u, 1u) != 0 ||
                pi4_xhci_dma_status_stage_seed_direction(
                    reset_status, 1u, 1u) != 0) {
                g_pi4_xhci_dma_hub_child_reset_state =
                    PI4_XHCI_HUB_CHILD_RESET_CLEAR_FAILED;
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CHILD_RESET_BAD;
                goto cleanup;
            }
            g_pi4_xhci_dma_setup_trb_control = reset_setup[3];
            g_pi4_xhci_dma_status_trb_control = reset_status[3];
            pi4_dma_clean_bytes(ep0_ring,
                                PI4_XHCI_DMA_RING_ALLOC_BYTES);
            __asm__ volatile("dsb sy" ::: "memory");
            child_rc = pi4_xhci_dma_control_transfer_submit_wait(
                op, rt, db, event_memory, event_bus,
                g_pi4_xhci_dma_hub_expected_status_bus,
                slot_id, 1u, &event_cursor);
            g_pi4_xhci_dma_hub_child_clear_rc = (uint32_t)child_rc;
            if (child_rc <= 0) {
                g_pi4_xhci_dma_hub_child_reset_state =
                    PI4_XHCI_HUB_CHILD_RESET_CLEAR_FAILED;
                g_pi4_xhci_dma_state = child_rc == 0
                    ? PI4_XHCI_DMA_HUB_CHILD_RESET_TIMEOUT
                    : (child_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                      : PI4_XHCI_DMA_HUB_CHILD_RESET_BAD);
                goto cleanup;
            }

            status_trb_index = child_trb_index + 2u;
            g_pi4_xhci_dma_hub_probe_phase = 6u; /* Post-clear GET_STATUS. */
            g_pi4_xhci_dma_hub_request_type = 0xa3u;
            g_pi4_xhci_dma_hub_request = 0x00u;
            g_pi4_xhci_dma_hub_request_value = 0u;
            g_pi4_xhci_dma_hub_request_index = child_port;
            g_pi4_xhci_dma_hub_request_length = 4u;
            child_get_setup = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)status_trb_index * 16u);
            child_get_data = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)(status_trb_index + 1u) * 16u);
            child_get_status = (uint32_t *)(void *)(ep0_ring +
                (uint64_t)(status_trb_index + 2u) * 16u);
            g_pi4_xhci_dma_hub_status_trb_index = status_trb_index;
            g_pi4_xhci_dma_hub_expected_status_bus = ep0_ring_bus +
                (uint64_t)(status_trb_index + 2u) * 16u;
            if (pi4_xhci_dma_setup_stage_seed(child_get_setup, 0xa3u,
                    0x00u, 0u, child_port, 4u, 3u, 1u) != 0 ||
                pi4_xhci_dma_data_stage_seed(child_get_data,
                    child_buffer_bus, 4u, 1u) != 0 ||
                pi4_xhci_dma_status_stage_seed(child_get_status, 1u) != 0) {
                g_pi4_xhci_dma_hub_child_reset_state =
                    PI4_XHCI_HUB_CHILD_RESET_BAD_STATUS;
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CHILD_RESET_BAD;
                goto cleanup;
            }
            for (unsigned j = 0; j < 4u; j++) child_buffer[j] = 0;
            g_pi4_xhci_dma_setup_trb_control = child_get_setup[3];
            g_pi4_xhci_dma_data_trb_control = child_get_data[3];
            g_pi4_xhci_dma_status_trb_control = child_get_status[3];
            pi4_dma_clean_bytes(ep0_ring,
                                PI4_XHCI_DMA_RING_ALLOC_BYTES);
            pi4_dma_clean_invalidate_bytes(child_buffer, 64u);
            __asm__ volatile("dsb sy" ::: "memory");
            child_rc = pi4_xhci_dma_control_transfer_submit_wait(
                op, rt, db, event_memory, event_bus,
                g_pi4_xhci_dma_hub_expected_status_bus,
                slot_id, 1u, &event_cursor);
            if (child_rc <= 0) {
                g_pi4_xhci_dma_hub_child_reset_state =
                    PI4_XHCI_HUB_CHILD_RESET_BAD_STATUS;
                g_pi4_xhci_dma_state = child_rc == 0
                    ? PI4_XHCI_DMA_HUB_CHILD_RESET_TIMEOUT
                    : (child_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                      : PI4_XHCI_DMA_HUB_CHILD_RESET_BAD);
                goto cleanup;
            }
            __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
            pi4_dma_invalidate_bytes(child_buffer, 64u);
            g_pi4_xhci_dma_hub_child_final_status = child_buffer[0] |
                ((uint32_t)child_buffer[1] << 8);
            g_pi4_xhci_dma_hub_child_final_change = child_buffer[2] |
                ((uint32_t)child_buffer[3] << 8);
            if (!pi4_xhci_dma_hub_port_reset_change_cleared(
                    g_pi4_xhci_dma_hub_child_final_status,
                    g_pi4_xhci_dma_hub_child_final_change)) {
                g_pi4_xhci_dma_hub_child_reset_state =
                    PI4_XHCI_HUB_CHILD_RESET_BAD_STATUS;
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CHILD_RESET_BAD;
                goto cleanup;
            }
            g_pi4_xhci_dma_hub_child_reset_state =
                PI4_XHCI_HUB_CHILD_RESET_PASS;
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CHILD_RESET_PASS;
        }

        /* Continue only after the guarded reset succeeded. Verify the next
         * command-ring position after Address Device before attempting hub
         * configuration, separating a ring issue from a context issue. */
        if (g_pi4_xhci_dma_hub_child_reset_state ==
                PI4_XHCI_HUB_CHILD_RESET_PASS) {
            unsigned context_size = pi4_xhci_dma_context_size(
                g_pi4_xhci_caps[3]);
            unsigned hub_speed = pi4_xhci_dma_port_speed_id(
                g_pi4_xhci_dma_port_event_after);
            unsigned child_speed = pi4_xhci_dma_hub_child_speed_id(
                g_pi4_xhci_dma_hub_child_final_status);
            unsigned tt_think_time =
                (g_pi4_xhci_dma_hub_characteristics >> 5) & 3u;
            uint32_t route = pi4_xhci_dma_route_string_append(
                0u, 0u, child_port);
            uint32_t *configure_trb = (uint32_t *)(void *)(command_memory +
                (uint64_t)PI4_XHCI_DMA_COMMAND_CONFIGURE_HUB_TRB * 16u);
            uint32_t returned_slot = 0;
            int command_rc;

            g_pi4_xhci_dma_stage = 21;
            if (hub_speed != 3u || g_pi4_xhci_dma_device_class != 9u ||
                g_pi4_xhci_dma_device_protocol != 1u ||
                !child_speed || route == UINT32_MAX) {
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CONTEXT_BAD;
                goto cleanup;
            }

            {
                unsigned guard_index =
                    PI4_XHCI_DMA_COMMAND_HUB_GUARD_NOOP_TRB;
                uint32_t *guard_trb = (uint32_t *)(void *)(command_memory +
                    (uint64_t)guard_index * 16u);
                if (pi4_xhci_dma_noop_seed(guard_trb, 1u) != 0) {
                    g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_GUARD_BAD;
                    goto cleanup;
                }
                pi4_dma_clean_bytes((uint8_t *)(void *)guard_trb, 16u);
                command_rc = pi4_xhci_dma_command_submit_wait(op, rt, db,
                    event_memory, event_bus, command_bus, guard_index, 0u,
                    &event_cursor, &returned_slot,
                    &g_pi4_xhci_dma_hub_guard_status,
                    &g_pi4_xhci_dma_hub_guard_control);
                if (command_rc <= 0) {
                    g_pi4_xhci_dma_state = command_rc == 0
                        ? PI4_XHCI_DMA_HUB_GUARD_TIMEOUT
                        : (command_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                            : PI4_XHCI_DMA_HUB_GUARD_BAD);
                    goto cleanup;
                }
                g_pi4_xhci_dma_command_completions++;
            }

            /* Configure Endpoint requires the existing Slot Context with
             * only its hub fields updated. Refresh it after the EP0 traffic. */
            __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
            pi4_dma_invalidate_bytes(output_context,
                                     PI4_XHCI_DMA_CONTEXT_BYTES);
            if (pi4_xhci_dma_hub_context_seed(input_context,
                    PI4_XHCI_DMA_CONTEXT_BYTES, context_size, output_context,
                    g_pi4_xhci_dma_hub_ports, tt_think_time, 0u) != 0 ||
                pi4_xhci_dma_configure_endpoint_seed(configure_trb,
                    input_context_bus, slot_id, 1u) != 0) {
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CONTEXT_BAD;
                goto cleanup;
            }
            {
                const uint32_t *control_context =
                    (const uint32_t *)(const void *)input_context;
                const uint32_t *slot_context =
                    (const uint32_t *)(const void *)(input_context +
                                                     context_size);
                g_pi4_xhci_dma_hub_config_input_words[0] =
                    control_context[0];
                g_pi4_xhci_dma_hub_config_input_words[1] =
                    control_context[1];
                for (unsigned i = 0; i < 6u; i++)
                    g_pi4_xhci_dma_hub_config_input_words[2u + i] =
                        slot_context[i];
                for (unsigned i = 0; i < 4u; i++)
                    g_pi4_xhci_dma_hub_config_trb_words[i] =
                        configure_trb[i];
            }
            pi4_dma_clean_bytes(input_context,
                                (uint64_t)context_size * 2u);
            pi4_dma_clean_invalidate_bytes(output_context,
                                           PI4_XHCI_DMA_CONTEXT_BYTES);
            pi4_dma_clean_bytes((uint8_t *)(void *)configure_trb, 16u);
            __asm__ volatile("dsb sy" ::: "memory");
            command_rc = pi4_xhci_dma_command_submit_wait(op, rt, db,
                event_memory, event_bus, command_bus,
                PI4_XHCI_DMA_COMMAND_CONFIGURE_HUB_TRB, slot_id,
                &event_cursor, &returned_slot,
                &g_pi4_xhci_dma_hub_config_status,
                &g_pi4_xhci_dma_hub_config_control);
            if (command_rc <= 0) {
                (void)pi4_xhci_read_mmio32(op, 0x18,
                    PI4_XHCI_MMIO_BYTES,
                    &g_pi4_xhci_dma_hub_config_crcr[0]);
                (void)pi4_xhci_read_mmio32(op, 0x1c,
                    PI4_XHCI_MMIO_BYTES,
                    &g_pi4_xhci_dma_hub_config_crcr[1]);
                (void)pi4_xhci_read_mmio32(op, 0x04,
                    PI4_XHCI_MMIO_BYTES,
                    &g_pi4_xhci_dma_hub_config_usbsts);
                (void)pi4_xhci_read_mmio32(db, 0,
                    PI4_XHCI_MMIO_BYTES,
                    &g_pi4_xhci_dma_hub_config_db);
                g_pi4_xhci_dma_state = command_rc == 0
                    ? PI4_XHCI_DMA_HUB_CONFIG_TIMEOUT
                    : (command_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                        : PI4_XHCI_DMA_HUB_CONFIG_BAD);
                goto cleanup;
            }
            g_pi4_xhci_dma_command_completions++;
            __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
            pi4_dma_invalidate_bytes(output_context,
                                     PI4_XHCI_DMA_CONTEXT_BYTES);
            {
                const uint32_t *hub_slot =
                    (const uint32_t *)(const void *)output_context;
                for (unsigned i = 0; i < 3u; i++)
                    g_pi4_xhci_dma_hub_output_slot_words[i] = hub_slot[i];
            }
            if (!pi4_xhci_dma_output_hub_context_ok(output_context,
                    context_size, g_pi4_xhci_dma_hub_ports,
                    tt_think_time, 0u)) {
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_HUB_CONTEXT_BAD;
                goto cleanup;
            }

            unsigned connected = g_pi4_xhci_dma_hub_port_connected_mask & 15u;
            if (connected && (connected & (connected - 1u))) {
                (void)pi4_usb_bus_start(h, slot_id, g_pi4_xhci_dma_port_event_port,
                    pi4_xhci_dma_port_speed_id(g_pi4_xhci_dma_port_event_after),
                    context_size, connected, memory, &layout, op, rt, db,
                    event_memory, event_bus, &event_cursor, command_memory,
                    command_bus, input_context, input_context_bus, ep0_ring,
                    ep0_ring_bus, control_buffer, control_buffer_bus, original_pci);
                goto cleanup;
            }

            /* Enable a distinct child slot, publish its independent output
             * context through the DCBAA, then submit routed Address Device. */
            g_pi4_xhci_dma_stage = 22;
            if (pi4_xhci_dma_enable_slot_seed((uint32_t *)(void *)
                    (command_memory +
                     (uint64_t)PI4_XHCI_DMA_COMMAND_CHILD_ENABLE_SLOT_TRB * 16u),
                    g_pi4_xhci_dma_protocol_slot_type, 1u) != 0) {
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_ENABLE_BAD;
                goto cleanup;
            }
            {
                uint8_t *child_enable_trb = command_memory +
                    (uint64_t)PI4_XHCI_DMA_COMMAND_CHILD_ENABLE_SLOT_TRB * 16u;
                pi4_dma_clean_bytes(child_enable_trb, 16u);
                command_rc = pi4_xhci_dma_command_submit_wait(op, rt, db,
                    event_memory, event_bus, command_bus,
                    PI4_XHCI_DMA_COMMAND_CHILD_ENABLE_SLOT_TRB, UINT32_MAX,
                    &event_cursor, &child_slot_id,
                    &g_pi4_xhci_dma_child_enable_status,
                    &g_pi4_xhci_dma_child_enable_control);
            }
            if (command_rc <= 0) {
                g_pi4_xhci_dma_state = command_rc == 0
                    ? PI4_XHCI_DMA_CHILD_ENABLE_TIMEOUT
                    : (command_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                        : PI4_XHCI_DMA_CHILD_ENABLE_BAD);
                goto cleanup;
            }
            if (!child_slot_id || child_slot_id > layout.max_slots ||
                child_slot_id == slot_id) {
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_ENABLE_BAD;
                goto cleanup;
            }
            g_pi4_xhci_dma_child_slot_id = child_slot_id;
            g_pi4_xhci_dma_command_completions++;
            dcbaa[child_slot_id] = child_output_bus;
            pi4_dma_clean_bytes((uint8_t *)(void *)&dcbaa[child_slot_id],
                                sizeof(dcbaa[child_slot_id]));
            __asm__ volatile("dsb sy" ::: "memory");

            g_pi4_xhci_dma_stage = 23;
            if (pi4_xhci_dma_child_address_context_seed(input_context,
                    PI4_XHCI_DMA_CONTEXT_BYTES, context_size, child_speed,
                    g_pi4_xhci_dma_port_event_port, route, slot_id,
                    child_port, hub_speed, 0u, child_ep0_bus) != 0) {
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_ADDRESS_BAD;
                goto cleanup;
            }
            {
                const uint32_t *in =
                    (const uint32_t *)(const void *)input_context;
                const uint32_t *child_slot =
                    (const uint32_t *)(const void *)(input_context +
                                                     context_size);
                const uint32_t *child_ep0 =
                    (const uint32_t *)(const void *)(input_context +
                                                     context_size * 2u);
                g_pi4_xhci_dma_child_speed_id = child_speed;
                g_pi4_xhci_dma_child_route_string = route;
                g_pi4_xhci_dma_child_address_context_words[0] = in[1];
                g_pi4_xhci_dma_child_address_context_words[1] = child_slot[0];
                g_pi4_xhci_dma_child_address_context_words[2] = child_slot[1];
                g_pi4_xhci_dma_child_address_context_words[3] = child_slot[2];
                g_pi4_xhci_dma_child_address_context_words[4] = child_ep0[1];
                g_pi4_xhci_dma_child_address_context_words[5] = child_ep0[2];
            }
            if (pi4_xhci_dma_address_device_seed((uint32_t *)(void *)
                    (command_memory + (uint64_t)
                     PI4_XHCI_DMA_COMMAND_CHILD_ADDRESS_DEVICE_TRB * 16u),
                    input_context_bus, child_slot_id, 1u, 0u) != 0) {
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_ADDRESS_BAD;
                goto cleanup;
            }
            pi4_dma_clean_bytes(input_context,
                                (uint64_t)context_size * 3u);
            pi4_dma_clean_invalidate_bytes(child_output_context,
                                           PI4_XHCI_DMA_CONTEXT_BYTES);
            pi4_dma_clean_invalidate_bytes(child_ep0_ring,
                                           PI4_XHCI_DMA_RING_ALLOC_BYTES);
            {
                uint8_t *child_address_trb = command_memory +
                    (uint64_t)PI4_XHCI_DMA_COMMAND_CHILD_ADDRESS_DEVICE_TRB * 16u;
                pi4_dma_clean_bytes(child_address_trb, 16u);
                command_rc = pi4_xhci_dma_command_submit_wait(op, rt, db,
                    event_memory, event_bus, command_bus,
                    PI4_XHCI_DMA_COMMAND_CHILD_ADDRESS_DEVICE_TRB,
                    child_slot_id, &event_cursor, &returned_slot,
                    &g_pi4_xhci_dma_child_address_status,
                    &g_pi4_xhci_dma_child_address_control);
            }
            if (command_rc <= 0) {
                g_pi4_xhci_dma_state = command_rc == 0
                    ? PI4_XHCI_DMA_CHILD_ADDRESS_TIMEOUT
                    : (command_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                        : PI4_XHCI_DMA_CHILD_ADDRESS_BAD);
                goto cleanup;
            }
            g_pi4_xhci_dma_command_completions++;
            __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
            pi4_dma_invalidate_bytes(child_output_context,
                                     PI4_XHCI_DMA_CONTEXT_BYTES);
            g_pi4_xhci_dma_child_address =
                pi4_xhci_dma_output_usb_address(child_output_context);
            g_pi4_xhci_dma_child_slot_state =
                pi4_xhci_dma_output_slot_state(child_output_context);
            if (g_pi4_xhci_dma_child_address == 0u ||
                g_pi4_xhci_dma_child_slot_state != 2u) {
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_ADDRESS_BAD;
                goto cleanup;
            }
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_ADDRESS_PASS;

            /* Fetch the first eight bytes of the newly addressed Full-Speed
             * child's Device Descriptor using EP0's speed-default MPS (8).
             * Then update EP0 to the descriptor-reported MPS with Evaluate
             * Context before fetching all 18 bytes. */
            {
                uint32_t *child_setup =
                    (uint32_t *)(void *)(child_ep0_ring +
                        (uint64_t)PI4_XHCI_DMA_CHILD_DESCRIPTOR_PREFIX_SETUP_TRB * 16u);
                uint32_t *child_data =
                    (uint32_t *)(void *)(child_ep0_ring +
                        ((uint64_t)PI4_XHCI_DMA_CHILD_DESCRIPTOR_PREFIX_SETUP_TRB + 1u) * 16u);
                uint32_t *child_status =
                    (uint32_t *)(void *)(child_ep0_ring +
                        ((uint64_t)PI4_XHCI_DMA_CHILD_DESCRIPTOR_PREFIX_SETUP_TRB + 2u) * 16u);
                uint8_t *descriptor_buffer = control_buffer + 1024u;
                uint64_t descriptor_buffer_bus = control_buffer_bus + 1024u;
                int child_transfer_rc;

                g_pi4_xhci_dma_stage = 24;
                g_pi4_xhci_dma_child_descriptor_buffer_bus =
                    descriptor_buffer_bus;
                g_pi4_xhci_dma_child_descriptor_status_bus =
                    child_ep0_bus + 32u;
                if (pi4_xhci_dma_get_descriptor_setup_seed(child_setup,
                        1u, 0u, 8u, 1u) != 0 ||
                    pi4_xhci_dma_data_stage_seed(child_data,
                        descriptor_buffer_bus, 8u, 1u) != 0 ||
                    pi4_xhci_dma_status_stage_seed(child_status, 1u) != 0) {
                    g_pi4_xhci_dma_state =
                        PI4_XHCI_DMA_CHILD_DESCRIPTOR_BAD;
                    goto cleanup;
                }
                g_pi4_xhci_dma_child_descriptor_setup_control =
                    child_setup[3];
                g_pi4_xhci_dma_child_descriptor_data_control =
                    child_data[3];
                g_pi4_xhci_dma_child_descriptor_status_control =
                    child_status[3];
                for (unsigned i = 0; i < 8u; i++)
                    descriptor_buffer[i] = 0;
                pi4_dma_clean_bytes(child_ep0_ring,
                                    PI4_XHCI_DMA_RING_ALLOC_BYTES);
                pi4_dma_clean_invalidate_bytes(descriptor_buffer, 64u);
                __asm__ volatile("dsb sy" ::: "memory");
                child_transfer_rc =
                    pi4_xhci_dma_control_transfer_submit_wait(
                        op, rt, db, event_memory, event_bus,
                        g_pi4_xhci_dma_child_descriptor_status_bus,
                        child_slot_id, 1u, &event_cursor);
                g_pi4_xhci_dma_child_descriptor_transfer_rc =
                    (uint32_t)child_transfer_rc;
                if (child_transfer_rc <= 0) {
                    g_pi4_xhci_dma_state = child_transfer_rc == 0
                        ? PI4_XHCI_DMA_CHILD_DESCRIPTOR_TIMEOUT
                        : (child_transfer_rc == -1
                            ? PI4_XHCI_DMA_MMIO_FAIL
                            : PI4_XHCI_DMA_CHILD_DESCRIPTOR_BAD);
                    goto cleanup;
                }
                __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                pi4_dma_invalidate_bytes(descriptor_buffer, 64u);
                g_pi4_xhci_dma_child_descriptor_words[0] =
                    ((uint32_t)descriptor_buffer[0]) |
                    ((uint32_t)descriptor_buffer[1] << 8) |
                    ((uint32_t)descriptor_buffer[2] << 16) |
                    ((uint32_t)descriptor_buffer[3] << 24);
                g_pi4_xhci_dma_child_descriptor_words[1] =
                    ((uint32_t)descriptor_buffer[4]) |
                    ((uint32_t)descriptor_buffer[5] << 8) |
                    ((uint32_t)descriptor_buffer[6] << 16) |
                    ((uint32_t)descriptor_buffer[7] << 24);
                g_pi4_xhci_dma_child_descriptor_validated =
                    pi4_xhci_dma_child_device_descriptor_prefix_ok(
                        descriptor_buffer, 8u, child_speed) ? 1u : 0u;
                if (!g_pi4_xhci_dma_child_descriptor_validated) {
                    g_pi4_xhci_dma_state =
                        PI4_XHCI_DMA_CHILD_DESCRIPTOR_BAD;
                    goto cleanup;
                }
                g_pi4_xhci_dma_state =
                    PI4_XHCI_DMA_CHILD_DESCRIPTOR_PASS;

                /* Repeat the identical eight-byte Device Descriptor request
                 * before trying another request type. This distinguishes a
                 * failure to continue EP0 after one completed TD from a
                 * GET_STATUS-specific device response. */
                {
                    uint32_t *repeat_setup = (uint32_t *)(void *)(
                        child_ep0_ring + (uint64_t)
                        PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_SETUP_TRB * 16u);
                    uint32_t *repeat_data = repeat_setup + 4u;
                    uint32_t *repeat_status = repeat_setup + 8u;
                    uint8_t *repeat_buffer = control_buffer + 1216u;
                    uint64_t repeat_buffer_bus = control_buffer_bus + 1216u;

                    g_pi4_xhci_dma_stage = 25;
                    g_pi4_xhci_dma_child_repeat_descriptor_buffer_bus =
                        repeat_buffer_bus;
                    g_pi4_xhci_dma_child_repeat_descriptor_status_bus =
                        child_ep0_bus +
                        ((uint64_t)PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_SETUP_TRB +
                         2u) * 16u;
                    if (pi4_xhci_dma_get_descriptor_setup_seed(repeat_setup,
                            1u, 0u, 8u, 1u) != 0 ||
                        pi4_xhci_dma_data_stage_seed(repeat_data,
                            repeat_buffer_bus, 8u, 1u) != 0 ||
                        pi4_xhci_dma_status_stage_seed(repeat_status, 1u) != 0) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_child_repeat_descriptor_setup_control =
                        repeat_setup[3];
                    g_pi4_xhci_dma_child_repeat_descriptor_data_control =
                        repeat_data[3];
                    g_pi4_xhci_dma_child_repeat_descriptor_status_control =
                        repeat_status[3];
                    for (unsigned i = 0; i < 64u; i++)
                        repeat_buffer[i] = 0x5au;
                    pi4_dma_clean_bytes(child_ep0_ring,
                                        PI4_XHCI_DMA_RING_ALLOC_BYTES);
                    pi4_dma_clean_invalidate_bytes(repeat_buffer, 64u);
                    __asm__ volatile("dsb sy" ::: "memory");
                    child_transfer_rc =
                        pi4_xhci_dma_control_transfer_submit_wait(
                            op, rt, db, event_memory, event_bus,
                            g_pi4_xhci_dma_child_repeat_descriptor_status_bus,
                            child_slot_id, 1u, &event_cursor);
                    g_pi4_xhci_dma_child_repeat_descriptor_transfer_rc =
                        (uint32_t)child_transfer_rc;
                    g_pi4_xhci_dma_child_repeat_descriptor_event_parameter =
                        g_pi4_xhci_dma_transfer_parameter;
                    g_pi4_xhci_dma_child_repeat_descriptor_event_status =
                        g_pi4_xhci_dma_transfer_status;
                    g_pi4_xhci_dma_child_repeat_descriptor_event_control =
                        g_pi4_xhci_dma_transfer_control;
                    g_pi4_xhci_dma_child_repeat_descriptor_event_checks =
                        g_pi4_xhci_dma_transfer_checks;
                    if (child_transfer_rc <= 0) {
                        if (child_transfer_rc == -2 &&
                            (g_pi4_xhci_dma_transfer_checks &
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER)) ==
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER) &&
                            (g_pi4_xhci_dma_transfer_parameter & ~0xfull) ==
                                (g_pi4_xhci_dma_child_repeat_descriptor_status_bus &
                                 ~0xfull)) {
                            __asm__ volatile("dmb oshld\n\tdsb sy" :::
                                             "memory");
                            pi4_dma_invalidate_bytes(repeat_buffer, 64u);
                            g_pi4_xhci_dma_child_repeat_descriptor_words[0] =
                                (uint32_t)repeat_buffer[0] |
                                ((uint32_t)repeat_buffer[1] << 8) |
                                ((uint32_t)repeat_buffer[2] << 16) |
                                ((uint32_t)repeat_buffer[3] << 24);
                            g_pi4_xhci_dma_child_repeat_descriptor_words[1] =
                                (uint32_t)repeat_buffer[4] |
                                ((uint32_t)repeat_buffer[5] << 8) |
                                ((uint32_t)repeat_buffer[6] << 16) |
                                ((uint32_t)repeat_buffer[7] << 24);
                            g_pi4_xhci_dma_child_repeat_descriptor_observed = 1u;
                        }
                        g_pi4_xhci_dma_state = child_transfer_rc == 0
                            ? PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_TIMEOUT
                            : (child_transfer_rc == -1
                                ? PI4_XHCI_DMA_MMIO_FAIL
                                : PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_BAD);
                        goto cleanup;
                    }
                    __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                    pi4_dma_invalidate_bytes(repeat_buffer, 64u);
                    g_pi4_xhci_dma_child_repeat_descriptor_words[0] =
                        (uint32_t)repeat_buffer[0] |
                        ((uint32_t)repeat_buffer[1] << 8) |
                        ((uint32_t)repeat_buffer[2] << 16) |
                        ((uint32_t)repeat_buffer[3] << 24);
                    g_pi4_xhci_dma_child_repeat_descriptor_words[1] =
                        (uint32_t)repeat_buffer[4] |
                        ((uint32_t)repeat_buffer[5] << 8) |
                        ((uint32_t)repeat_buffer[6] << 16) |
                        ((uint32_t)repeat_buffer[7] << 24);
                    g_pi4_xhci_dma_child_repeat_descriptor_observed = 1u;
                    g_pi4_xhci_dma_child_repeat_descriptor_validated =
                        pi4_xhci_dma_child_device_descriptor_prefix_ok(
                            repeat_buffer, 8u, child_speed) ? 1u : 0u;
                    if (!g_pi4_xhci_dma_child_repeat_descriptor_validated) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_state =
                        PI4_XHCI_DMA_CHILD_REPEAT_DESCRIPTOR_PASS;
                }

                /* Probe a second standard device request while EP0 still has
                 * its speed-default MPS. GET_CONFIGURATION is valid in the
                 * Addressed state and distinguishes a GET_STATUS-specific
                 * response from a general post-descriptor EP0 failure. */
                {
                    uint32_t *configuration_setup = (uint32_t *)(void *)(
                        child_ep0_ring + (uint64_t)
                        PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_SETUP_TRB * 16u);
                    uint32_t *configuration_data = configuration_setup + 4u;
                    uint32_t *configuration_status = configuration_setup + 8u;
                    uint8_t *configuration_value = control_buffer + 1152u;
                    uint64_t configuration_value_bus = control_buffer_bus + 1152u;

                    g_pi4_xhci_dma_stage = 26;
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_buffer_bus =
                        configuration_value_bus;
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_status_bus =
                        child_ep0_bus +
                        ((uint64_t)PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_SETUP_TRB +
                         2u) * 16u;
                    if (pi4_xhci_dma_get_configuration_setup_seed(
                            configuration_setup, 1u) != 0 ||
                        pi4_xhci_dma_data_stage_seed(configuration_data,
                            configuration_value_bus, 1u, 1u) != 0 ||
                        pi4_xhci_dma_status_stage_seed_direction(
                            configuration_status, 0u, 1u) != 0) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_setup_control =
                        configuration_setup[3];
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_data_control =
                        configuration_data[3];
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_status_control =
                        configuration_status[3];
                    for (unsigned i = 0; i < 64u; i++)
                        configuration_value[i] = 0xa5u;
                    pi4_dma_clean_bytes(child_ep0_ring,
                                        PI4_XHCI_DMA_RING_ALLOC_BYTES);
                    pi4_dma_clean_invalidate_bytes(configuration_value, 64u);
                    __asm__ volatile("dsb sy" ::: "memory");
                    child_transfer_rc =
                        pi4_xhci_dma_control_transfer_submit_wait(
                            op, rt, db, event_memory, event_bus,
                            g_pi4_xhci_dma_child_pre_evaluate_get_configuration_status_bus,
                            child_slot_id, 1u, &event_cursor);
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_transfer_rc =
                        (uint32_t)child_transfer_rc;
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_parameter =
                        g_pi4_xhci_dma_transfer_parameter;
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_status =
                        g_pi4_xhci_dma_transfer_status;
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_control =
                        g_pi4_xhci_dma_transfer_control;
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_checks =
                        g_pi4_xhci_dma_transfer_checks;
                    if (child_transfer_rc <= 0) {
                        if (child_transfer_rc == -2 &&
                            (g_pi4_xhci_dma_transfer_checks &
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER)) ==
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER) &&
                            (g_pi4_xhci_dma_transfer_parameter & ~0xfull) ==
                                (g_pi4_xhci_dma_child_pre_evaluate_get_configuration_status_bus &
                                 ~0xfull)) {
                            __asm__ volatile("dmb oshld\n\tdsb sy" :::
                                             "memory");
                            pi4_dma_invalidate_bytes(configuration_value, 64u);
                            g_pi4_xhci_dma_child_pre_evaluate_get_configuration_word =
                                (uint32_t)configuration_value[0];
                            g_pi4_xhci_dma_child_pre_evaluate_get_configuration_observed =
                                1u;
                        }
                        g_pi4_xhci_dma_state = child_transfer_rc == 0
                            ? PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_TIMEOUT
                            : (child_transfer_rc == -1
                                ? PI4_XHCI_DMA_MMIO_FAIL
                                : PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_BAD);
                        goto cleanup;
                    }
                    __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                    pi4_dma_invalidate_bytes(configuration_value, 64u);
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_word =
                        (uint32_t)configuration_value[0];
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_observed =
                        1u;
                    /* The child has only been addressed; it has not received
                     * SET_CONFIGURATION, so the standard response is zero. */
                    if (configuration_value[0] != 0u) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_state =
                        PI4_XHCI_DMA_CHILD_PRE_EVALUATE_GET_CONFIGURATION_PASS;
                }

                /* A Full-Speed device may declare a larger EP0 packet in
                 * byte 7. xHCI requires Evaluate Context to update EP0 after
                 * the default-size first packet. The next available command
                 * is the last pre-Link entry and retains the initial cycle. */
                {
                    const uint32_t *output_ep0 =
                        pi4_xhci_dma_output_ep0_context(child_output_context,
                            PI4_XHCI_DMA_CONTEXT_BYTES, context_size);
                    uint32_t *evaluate_trb = (uint32_t *)(void *)(
                        command_memory + (uint64_t)
                        PI4_XHCI_DMA_COMMAND_CHILD_EVALUATE_CONTEXT_TRB * 16u);
                    unsigned requested_mps = 0;
                    int update_mps;

                    g_pi4_xhci_dma_stage = 27;
                    __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                    pi4_dma_invalidate_bytes(child_output_context,
                                             PI4_XHCI_DMA_CONTEXT_BYTES);
                    update_mps = pi4_xhci_dma_ep0_mps_update_plan(child_speed,
                        descriptor_buffer[7],
                        pi4_xhci_dma_output_ep0_max_packet_size(output_ep0),
                        &requested_mps);
                    g_pi4_xhci_dma_child_ep0_evaluate_mps = requested_mps;
                    if (update_mps < 0 || (update_mps &&
                        (pi4_xhci_dma_child_ep0_evaluate_context_seed(
                            input_context, PI4_XHCI_DMA_CONTEXT_BYTES,
                            context_size, child_output_context,
                            PI4_XHCI_DMA_CONTEXT_BYTES, child_speed,
                            requested_mps) != 0 ||
                        pi4_xhci_dma_evaluate_context_seed(evaluate_trb,
                            input_context_bus, child_slot_id, 1u) != 0)) ||
                        (!update_mps &&
                         pi4_xhci_dma_noop_seed(evaluate_trb, 1u) != 0)) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_EP0_EVALUATE_BAD;
                        goto cleanup;
                    }
                    /* Keep index 254 occupied even when no MPS change is
                     * required, so subsequent post-Link commands are live. */
                    g_pi4_xhci_dma_child_ep0_mps_command = update_mps
                        ? PI4_XHCI_TRB_TYPE_EVALUATE_CONTEXT
                        : PI4_XHCI_TRB_TYPE_NOOP;
                    if (update_mps)
                        pi4_dma_clean_bytes(input_context,
                                            (uint64_t)context_size * 3u);
                    pi4_dma_clean_bytes((uint8_t *)(void *)evaluate_trb, 16u);
                    __asm__ volatile("dsb sy" ::: "memory");
                    child_transfer_rc = pi4_xhci_dma_command_submit_wait(
                        op, rt, db, event_memory, event_bus, command_bus,
                        PI4_XHCI_DMA_COMMAND_CHILD_EVALUATE_CONTEXT_TRB,
                        update_mps ? child_slot_id : 0u,
                        &event_cursor, &returned_slot,
                        &g_pi4_xhci_dma_child_ep0_evaluate_status,
                        &g_pi4_xhci_dma_child_ep0_evaluate_control);
                    if (child_transfer_rc <= 0) {
                        g_pi4_xhci_dma_state = child_transfer_rc == 0
                            ? PI4_XHCI_DMA_CHILD_EP0_EVALUATE_TIMEOUT
                            : (child_transfer_rc == -1
                                ? PI4_XHCI_DMA_MMIO_FAIL
                                : PI4_XHCI_DMA_CHILD_EP0_EVALUATE_BAD);
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_command_completions++;
                    __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                    pi4_dma_invalidate_bytes(child_output_context,
                                             PI4_XHCI_DMA_CONTEXT_BYTES);
                    g_pi4_xhci_dma_child_ep0_evaluate_output_mps =
                        pi4_xhci_dma_output_ep0_max_packet_size(output_ep0);
                    if (g_pi4_xhci_dma_child_ep0_evaluate_output_mps !=
                            requested_mps) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_EP0_EVALUATE_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_state =
                        PI4_XHCI_DMA_CHILD_EP0_EVALUATE_PASS;
                }

                /* GET_STATUS is a small standard control-IN request. It
                 * checks whether the child still accepts EP0 traffic after
                 * Evaluate Context before retrying the longer descriptor. */
                {
                    uint32_t *status_setup = (uint32_t *)(void *)(
                        child_ep0_ring + (uint64_t)
                        PI4_XHCI_DMA_CHILD_POST_EVALUATE_GET_STATUS_SETUP_TRB * 16u);
                    uint32_t *status_data = status_setup + 4u;
                    uint32_t *status_stage = status_setup + 8u;
                    uint8_t *device_status = control_buffer + 1168u;
                    uint64_t device_status_bus = control_buffer_bus + 1168u;

                    g_pi4_xhci_dma_stage = 28;
                    g_pi4_xhci_dma_child_get_status_buffer_bus =
                        device_status_bus;
                    g_pi4_xhci_dma_child_get_status_status_bus =
                        child_ep0_bus +
                        ((uint64_t)PI4_XHCI_DMA_CHILD_POST_EVALUATE_GET_STATUS_SETUP_TRB +
                         2u) * 16u;
                    if (pi4_xhci_dma_get_status_setup_seed(status_setup,
                            1u) != 0 ||
                        pi4_xhci_dma_data_stage_seed(status_data,
                            device_status_bus, 2u, 1u) != 0 ||
                        pi4_xhci_dma_status_stage_seed_direction(
                            status_stage, 0u, 1u) != 0) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_GET_STATUS_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_child_get_status_setup_control =
                        status_setup[3];
                    g_pi4_xhci_dma_child_get_status_data_control =
                        status_data[3];
                    g_pi4_xhci_dma_child_get_status_status_control =
                        status_stage[3];
                    for (unsigned i = 0; i < 64u; i++)
                        device_status[i] = 0xa5u;
                    pi4_dma_clean_bytes(child_ep0_ring,
                                        PI4_XHCI_DMA_RING_ALLOC_BYTES);
                    pi4_dma_clean_invalidate_bytes(device_status, 64u);
                    __asm__ volatile("dsb sy" ::: "memory");
                    child_transfer_rc =
                        pi4_xhci_dma_control_transfer_submit_wait(
                            op, rt, db, event_memory, event_bus,
                            g_pi4_xhci_dma_child_get_status_status_bus,
                            child_slot_id, 1u, &event_cursor);
                    g_pi4_xhci_dma_child_get_status_transfer_rc =
                        (uint32_t)child_transfer_rc;
                    g_pi4_xhci_dma_child_get_status_event_parameter =
                        g_pi4_xhci_dma_transfer_parameter;
                    g_pi4_xhci_dma_child_get_status_event_status =
                        g_pi4_xhci_dma_transfer_status;
                    g_pi4_xhci_dma_child_get_status_event_control =
                        g_pi4_xhci_dma_transfer_control;
                    g_pi4_xhci_dma_child_get_status_event_checks =
                        g_pi4_xhci_dma_transfer_checks;
                    if (child_transfer_rc <= 0) {
                        if (child_transfer_rc == -2 &&
                            (g_pi4_xhci_dma_transfer_checks &
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER)) ==
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER) &&
                            (g_pi4_xhci_dma_transfer_parameter & ~0xfull) ==
                                (g_pi4_xhci_dma_child_get_status_status_bus &
                                 ~0xfull)) {
                            __asm__ volatile("dmb oshld\n\tdsb sy" :::
                                             "memory");
                            pi4_dma_invalidate_bytes(device_status, 64u);
                            g_pi4_xhci_dma_child_get_status_word =
                                (uint32_t)device_status[0] |
                                ((uint32_t)device_status[1] << 8);
                            g_pi4_xhci_dma_child_get_status_observed = 1u;
                        }
                        g_pi4_xhci_dma_state = child_transfer_rc == 0
                            ? PI4_XHCI_DMA_CHILD_GET_STATUS_TIMEOUT
                            : (child_transfer_rc == -1
                                ? PI4_XHCI_DMA_MMIO_FAIL
                                : PI4_XHCI_DMA_CHILD_GET_STATUS_BAD);
                        goto cleanup;
                    }
                    __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                    pi4_dma_invalidate_bytes(device_status, 64u);
                    g_pi4_xhci_dma_child_get_status_word =
                        (uint32_t)device_status[0] |
                        ((uint32_t)device_status[1] << 8);
                    g_pi4_xhci_dma_child_get_status_observed = 1u;
                    g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_GET_STATUS_PASS;
                }

                /* The initial and repeated descriptor TDs used indexes 0–2
                 * and 3–5. The pre/post-Evaluate GET_STATUS TDs use 6–8 and
                 * 9–11; continue without resetting the live EP0 ring. */
                {
                    uint32_t *full_setup = (uint32_t *)(void *)(
                        child_ep0_ring + (uint64_t)
                        PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_SETUP_TRB * 16u);
                    uint32_t *full_data = full_setup + 4u;
                    uint32_t *full_status = full_setup + 8u;
                    uint8_t *full_descriptor = control_buffer + 1088u;
                    uint64_t full_descriptor_bus = control_buffer_bus + 1088u;

                    g_pi4_xhci_dma_stage = 29;
                    g_pi4_xhci_dma_child_full_descriptor_buffer_bus =
                        full_descriptor_bus;
                    g_pi4_xhci_dma_child_full_descriptor_status_bus =
                        child_ep0_bus +
                        ((uint64_t)PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_SETUP_TRB +
                         2u) * 16u;
                    if (pi4_xhci_dma_get_descriptor_setup_seed(full_setup,
                            1u, 0u, 18u, 1u) != 0 ||
                        pi4_xhci_dma_data_stage_seed(full_data,
                            full_descriptor_bus, 18u, 1u) != 0 ||
                        pi4_xhci_dma_status_stage_seed(full_status, 1u) != 0) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_child_full_descriptor_setup_control =
                        full_setup[3];
                    g_pi4_xhci_dma_child_full_descriptor_data_control =
                        full_data[3];
                    g_pi4_xhci_dma_child_full_descriptor_status_control =
                        full_status[3];
                    /* A nonzero marker distinguishes an untouched DMA buffer
                     * from a device response containing zero bytes. */
                    for (unsigned i = 0; i < 64u; i++)
                        full_descriptor[i] = 0xa5u;
                    pi4_dma_clean_bytes(child_ep0_ring,
                                        PI4_XHCI_DMA_RING_ALLOC_BYTES);
                    pi4_dma_clean_invalidate_bytes(full_descriptor, 64u);
                    __asm__ volatile("dsb sy" ::: "memory");
                    child_transfer_rc =
                        pi4_xhci_dma_control_transfer_submit_wait(
                            op, rt, db, event_memory, event_bus,
                            g_pi4_xhci_dma_child_full_descriptor_status_bus,
                            child_slot_id, 1u, &event_cursor);
                    g_pi4_xhci_dma_child_full_descriptor_transfer_rc =
                        (uint32_t)child_transfer_rc;
                    if (child_transfer_rc <= 0) {
                        /* Preserve bytes when a structurally matching
                         * Transfer Event reports a non-success completion or
                         * residual length. This lets the next hardware trace
                         * separate a missing DMA write from event validation. */
                        if (child_transfer_rc == -2 &&
                            (g_pi4_xhci_dma_transfer_checks &
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER)) ==
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER) &&
                            (g_pi4_xhci_dma_transfer_parameter & ~0xfull) ==
                                (g_pi4_xhci_dma_child_full_descriptor_status_bus &
                                 ~0xfull)) {
                            __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                            pi4_dma_invalidate_bytes(full_descriptor, 64u);
                            for (unsigned i = 0; i < 5u; i++) {
                                g_pi4_xhci_dma_child_full_descriptor_words[i] =
                                    ((uint32_t)full_descriptor[i * 4u]) |
                                    ((uint32_t)full_descriptor[i * 4u + 1u] << 8) |
                                    ((uint32_t)full_descriptor[i * 4u + 2u] << 16) |
                                    ((uint32_t)full_descriptor[i * 4u + 3u] << 24);
                            }
                            g_pi4_xhci_dma_child_full_descriptor_observed = 1u;
                        }
                        g_pi4_xhci_dma_state = child_transfer_rc == 0
                            ? PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_TIMEOUT
                            : (child_transfer_rc == -1
                                ? PI4_XHCI_DMA_MMIO_FAIL
                                : PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_BAD);
                        goto cleanup;
                    }
                    __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                    pi4_dma_invalidate_bytes(full_descriptor, 64u);
                    for (unsigned i = 0; i < 5u; i++) {
                        g_pi4_xhci_dma_child_full_descriptor_words[i] =
                            ((uint32_t)full_descriptor[i * 4u]) |
                            ((uint32_t)full_descriptor[i * 4u + 1u] << 8) |
                            ((uint32_t)full_descriptor[i * 4u + 2u] << 16) |
                            ((uint32_t)full_descriptor[i * 4u + 3u] << 24);
                    }
                    g_pi4_xhci_dma_child_full_descriptor_observed = 1u;
                    g_pi4_xhci_dma_child_full_descriptor_validated =
                        pi4_xhci_dma_child_device_descriptor_ok(
                            full_descriptor, 18u, child_speed) ? 1u : 0u;
                    if (!g_pi4_xhci_dma_child_full_descriptor_validated) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_state =
                        PI4_XHCI_DMA_CHILD_FULL_DESCRIPTOR_PASS;
                }

                uint8_t *child_config = control_buffer + 1280u;
                uint64_t child_config_bus = control_buffer_bus + 1280u;
                /* The descriptor identifies the addressed child as a
                 * mass-storage-class device. Read its nine-byte Configuration
                 * Descriptor header to discover total length, interface
                 * count, and configuration value before SET_CONFIGURATION. */
                {
                    uint32_t *config_setup = (uint32_t *)(void *)(
                        child_ep0_ring + (uint64_t)
                        PI4_XHCI_DMA_CHILD_CONFIG_HEADER_SETUP_TRB * 16u);
                    uint32_t *config_data = config_setup + 4u;
                    uint32_t *config_status = config_setup + 8u;

                    g_pi4_xhci_dma_stage = 30;
                    g_pi4_xhci_dma_child_config_header_buffer_bus =
                        child_config_bus;
                    g_pi4_xhci_dma_child_config_header_status_bus =
                        child_ep0_bus +
                        ((uint64_t)PI4_XHCI_DMA_CHILD_CONFIG_HEADER_SETUP_TRB +
                         2u) * 16u;
                    if (pi4_xhci_dma_get_descriptor_setup_seed(config_setup,
                            2u, 0u, 9u, 1u) != 0 ||
                        pi4_xhci_dma_data_stage_seed(config_data,
                            child_config_bus, 9u, 1u) != 0 ||
                        pi4_xhci_dma_status_stage_seed(config_status,
                            1u) != 0) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_CONFIG_HEADER_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_child_config_header_setup_control =
                        config_setup[3];
                    g_pi4_xhci_dma_child_config_header_data_control =
                        config_data[3];
                    g_pi4_xhci_dma_child_config_header_status_control =
                        config_status[3];
                    for (unsigned i = 0; i < 64u; i++)
                        child_config[i] = 0xa5u;
                    pi4_dma_clean_bytes(child_ep0_ring,
                                        PI4_XHCI_DMA_RING_ALLOC_BYTES);
                    pi4_dma_clean_invalidate_bytes(child_config, 64u);
                    __asm__ volatile("dsb sy" ::: "memory");
                    child_transfer_rc =
                        pi4_xhci_dma_control_transfer_submit_wait(
                            op, rt, db, event_memory, event_bus,
                            g_pi4_xhci_dma_child_config_header_status_bus,
                            child_slot_id, 1u, &event_cursor);
                    g_pi4_xhci_dma_child_config_header_transfer_rc =
                        (uint32_t)child_transfer_rc;
                    g_pi4_xhci_dma_child_config_header_event_parameter =
                        g_pi4_xhci_dma_transfer_parameter;
                    g_pi4_xhci_dma_child_config_header_event_status =
                        g_pi4_xhci_dma_transfer_status;
                    g_pi4_xhci_dma_child_config_header_event_control =
                        g_pi4_xhci_dma_transfer_control;
                    g_pi4_xhci_dma_child_config_header_event_checks =
                        g_pi4_xhci_dma_transfer_checks;
                    if (child_transfer_rc <= 0) {
                        if (child_transfer_rc == -2 &&
                            (g_pi4_xhci_dma_transfer_checks &
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER)) ==
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER) &&
                            (g_pi4_xhci_dma_transfer_parameter & ~0xfull) ==
                                (g_pi4_xhci_dma_child_config_header_status_bus &
                                 ~0xfull)) {
                            __asm__ volatile("dmb oshld\n\tdsb sy" :::
                                             "memory");
                            pi4_dma_invalidate_bytes(child_config, 64u);
                            for (unsigned i = 0; i < 3u; i++)
                                g_pi4_xhci_dma_child_config_header_words[i] =
                                    (uint32_t)child_config[i * 4u] |
                                    ((uint32_t)child_config[i * 4u + 1u] << 8) |
                                    ((uint32_t)child_config[i * 4u + 2u] << 16) |
                                    ((uint32_t)child_config[i * 4u + 3u] << 24);
                            g_pi4_xhci_dma_child_config_header_observed = 1u;
                        }
                        g_pi4_xhci_dma_state = child_transfer_rc == 0
                            ? PI4_XHCI_DMA_CHILD_CONFIG_HEADER_TIMEOUT
                            : (child_transfer_rc == -1
                                ? PI4_XHCI_DMA_MMIO_FAIL
                                : PI4_XHCI_DMA_CHILD_CONFIG_HEADER_BAD);
                        goto cleanup;
                    }
                    __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                    pi4_dma_invalidate_bytes(child_config, 64u);
                    for (unsigned i = 0; i < 3u; i++)
                        g_pi4_xhci_dma_child_config_header_words[i] =
                            (uint32_t)child_config[i * 4u] |
                            ((uint32_t)child_config[i * 4u + 1u] << 8) |
                            ((uint32_t)child_config[i * 4u + 2u] << 16) |
                            ((uint32_t)child_config[i * 4u + 3u] << 24);
                    g_pi4_xhci_dma_child_config_header_observed = 1u;
                    g_pi4_xhci_dma_child_config_header_validated =
                        pi4_xhci_dma_configuration_header_ok(
                            child_config, 9u) ? 1u : 0u;
                    if (!g_pi4_xhci_dma_child_config_header_validated) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_CONFIG_HEADER_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_state =
                        PI4_XHCI_DMA_CHILD_CONFIG_HEADER_PASS;
                }

                /* The header reports the full configuration length. Fetch
                 * that bounded descriptor chain and validate its interface
                 * and endpoint structure before selecting a configuration
                 * or issuing endpoint traffic. */
                {
                    const uint32_t config_offset = 1280u;
                    const uint32_t config_capacity =
                        PI4_XHCI_DMA_CONTROL_BUFFER_BYTES - config_offset;
                    uint32_t config_length = (uint32_t)child_config[2] |
                        ((uint32_t)child_config[3] << 8);
                    uint32_t *full_setup = (uint32_t *)(void *)(
                        child_ep0_ring + (uint64_t)
                        PI4_XHCI_DMA_CHILD_CONFIG_FULL_SETUP_TRB * 16u);
                    uint32_t *full_data = full_setup + 4u;
                    uint32_t *full_status = full_setup + 8u;
                    uint64_t full_config_bus =
                        control_buffer_bus + config_offset;

                    g_pi4_xhci_dma_stage = 31;
                    g_pi4_xhci_dma_child_config_full_buffer_bus =
                        full_config_bus;
                    g_pi4_xhci_dma_child_config_full_status_bus =
                        child_ep0_bus +
                        ((uint64_t)PI4_XHCI_DMA_CHILD_CONFIG_FULL_SETUP_TRB +
                         2u) * 16u;
                    if (config_length < 9u || config_length > config_capacity ||
                        pi4_xhci_dma_get_descriptor_setup_seed(full_setup,
                            2u, 0u, config_length, 1u) != 0 ||
                        pi4_xhci_dma_data_stage_seed(full_data,
                            full_config_bus, config_length, 1u) != 0 ||
                        pi4_xhci_dma_status_stage_seed(full_status,
                            1u) != 0) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_CONFIG_FULL_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_child_config_full_setup_control =
                        full_setup[3];
                    g_pi4_xhci_dma_child_config_full_data_control =
                        full_data[3];
                    g_pi4_xhci_dma_child_config_full_status_control =
                        full_status[3];
                    for (uint32_t i = 0; i < config_length; i++)
                        child_config[i] = 0xa5u;
                    pi4_dma_clean_bytes(child_ep0_ring,
                                        PI4_XHCI_DMA_RING_ALLOC_BYTES);
                    pi4_dma_clean_invalidate_bytes(child_config,
                                                   config_length);
                    __asm__ volatile("dsb sy" ::: "memory");
                    child_transfer_rc =
                        pi4_xhci_dma_control_transfer_submit_wait(
                            op, rt, db, event_memory, event_bus,
                            g_pi4_xhci_dma_child_config_full_status_bus,
                            child_slot_id, 1u, &event_cursor);
                    g_pi4_xhci_dma_child_config_full_transfer_rc =
                        (uint32_t)child_transfer_rc;
                    g_pi4_xhci_dma_child_config_full_event_parameter =
                        g_pi4_xhci_dma_transfer_parameter;
                    g_pi4_xhci_dma_child_config_full_event_status =
                        g_pi4_xhci_dma_transfer_status;
                    g_pi4_xhci_dma_child_config_full_event_control =
                        g_pi4_xhci_dma_transfer_control;
                    g_pi4_xhci_dma_child_config_full_event_checks =
                        g_pi4_xhci_dma_transfer_checks;
                    if (child_transfer_rc <= 0) {
                        if (child_transfer_rc == -2 &&
                            (g_pi4_xhci_dma_transfer_checks &
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER)) ==
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER) &&
                            (g_pi4_xhci_dma_transfer_parameter & ~0xfull) ==
                                (g_pi4_xhci_dma_child_config_full_status_bus &
                                 ~0xfull)) {
                            __asm__ volatile("dmb oshld\n\tdsb sy" :::
                                             "memory");
                            pi4_dma_invalidate_bytes(child_config,
                                                     config_length);
                            for (unsigned i = 0; i < 3u; i++)
                                g_pi4_xhci_dma_child_config_full_words[i] =
                                    (uint32_t)child_config[i * 4u] |
                                    ((uint32_t)child_config[i * 4u + 1u] << 8) |
                                    ((uint32_t)child_config[i * 4u + 2u] << 16) |
                                    ((uint32_t)child_config[i * 4u + 3u] << 24);
                            g_pi4_xhci_dma_child_config_full_observed = 1u;
                        }
                        g_pi4_xhci_dma_state = child_transfer_rc == 0
                            ? PI4_XHCI_DMA_CHILD_CONFIG_FULL_TIMEOUT
                            : (child_transfer_rc == -1
                                ? PI4_XHCI_DMA_MMIO_FAIL
                                : PI4_XHCI_DMA_CHILD_CONFIG_FULL_BAD);
                        goto cleanup;
                    }
                    __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                    pi4_dma_invalidate_bytes(child_config, config_length);
                    for (unsigned i = 0; i < 3u; i++)
                        g_pi4_xhci_dma_child_config_full_words[i] =
                            (uint32_t)child_config[i * 4u] |
                            ((uint32_t)child_config[i * 4u + 1u] << 8) |
                            ((uint32_t)child_config[i * 4u + 2u] << 16) |
                            ((uint32_t)child_config[i * 4u + 3u] << 24);
                    g_pi4_xhci_dma_child_config_full_observed = 1u;
                    g_pi4_xhci_dma_child_config_full_validated =
                        pi4_xhci_dma_configuration_descriptor_ok(
                            child_config, config_length, config_capacity,
                            &g_pi4_xhci_dma_child_config_summary) ? 1u : 0u;
                    if (!g_pi4_xhci_dma_child_config_full_validated) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_CONFIG_FULL_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_state =
                        PI4_XHCI_DMA_CHILD_CONFIG_FULL_PASS;
                    if (!g_pi4_xhci_dma_child_config_summary.bot_interface_found) {
                        g_pi4_keyboard.attempted = 1u;
                        if (!pi4_usb_keyboard_select(child_config, config_length,
                                                     &g_pi4_keyboard.interface)) {
                            g_pi4_keyboard.mouse = 1u;
                            (void)pi4_usb_mouse_select(child_config, config_length,
                                                      &g_pi4_keyboard.interface);
                        }
                    }
                }

                /* The complete descriptor chain is validated and identifies
                 * configuration value 1. Select it with one no-data EP0
                 * request, then read GET_CONFIGURATION to verify the device
                 * accepted the state change. Do not start endpoint traffic. */
                {
                    const uint32_t get_config_offset = 2048u;
                    uint8_t *get_config_value = control_buffer +
                        get_config_offset;
                    uint64_t get_config_bus = control_buffer_bus +
                        get_config_offset;
                    uint32_t *set_setup = (uint32_t *)(void *)(
                        child_ep0_ring + (uint64_t)
                        PI4_XHCI_DMA_CHILD_SET_CONFIGURATION_SETUP_TRB * 16u);
                    uint32_t *set_status = set_setup + 4u;
                    uint32_t *get_setup = (uint32_t *)(void *)(
                        child_ep0_ring + (uint64_t)
                        PI4_XHCI_DMA_CHILD_GET_CONFIGURATION_SETUP_TRB * 16u);
                    uint32_t *get_data = get_setup + 4u;
                    uint32_t *get_status = get_setup + 8u;
                    unsigned config_value =
                        g_pi4_xhci_dma_child_config_summary.configuration_value;

                    g_pi4_xhci_dma_child_set_config_status_bus =
                        child_ep0_bus +
                        ((uint64_t)PI4_XHCI_DMA_CHILD_SET_CONFIGURATION_SETUP_TRB +
                         1u) * 16u;
                    if (!config_value || config_value > 0xffu ||
                        pi4_xhci_dma_set_configuration_setup_seed(set_setup,
                            config_value, 1u) != 0 ||
                        pi4_xhci_dma_status_stage_seed_direction(
                            set_status, 1u, 1u) != 0) {
                        g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_SET_CONFIG_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_child_set_config_setup_control =
                        set_setup[3];
                    g_pi4_xhci_dma_child_set_config_status_control =
                        set_status[3];
                    pi4_dma_clean_bytes(child_ep0_ring,
                                        PI4_XHCI_DMA_RING_ALLOC_BYTES);
                    __asm__ volatile("dsb sy" ::: "memory");
                    g_pi4_xhci_dma_stage = 32;
                    child_transfer_rc =
                        pi4_xhci_dma_control_transfer_submit_wait(
                            op, rt, db, event_memory, event_bus,
                            g_pi4_xhci_dma_child_set_config_status_bus,
                            child_slot_id, 1u, &event_cursor);
                    g_pi4_xhci_dma_child_set_config_transfer_rc =
                        (uint32_t)child_transfer_rc;
                    g_pi4_xhci_dma_child_set_config_event_parameter =
                        g_pi4_xhci_dma_transfer_parameter;
                    g_pi4_xhci_dma_child_set_config_event_status =
                        g_pi4_xhci_dma_transfer_status;
                    g_pi4_xhci_dma_child_set_config_event_control =
                        g_pi4_xhci_dma_transfer_control;
                    g_pi4_xhci_dma_child_set_config_event_checks =
                        g_pi4_xhci_dma_transfer_checks;
                    if (child_transfer_rc <= 0) {
                        g_pi4_xhci_dma_state = child_transfer_rc == 0
                            ? PI4_XHCI_DMA_CHILD_SET_CONFIG_TIMEOUT
                            : (child_transfer_rc == -1
                                ? PI4_XHCI_DMA_MMIO_FAIL
                                : PI4_XHCI_DMA_CHILD_SET_CONFIG_BAD);
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_SET_CONFIG_PASS;

                    g_pi4_xhci_dma_child_get_config_buffer_bus =
                        get_config_bus;
                    g_pi4_xhci_dma_child_get_config_status_bus =
                        child_ep0_bus +
                        ((uint64_t)PI4_XHCI_DMA_CHILD_GET_CONFIGURATION_SETUP_TRB +
                         2u) * 16u;
                    if (pi4_xhci_dma_get_configuration_setup_seed(
                            get_setup, 1u) != 0 ||
                        pi4_xhci_dma_data_stage_seed(get_data,
                            get_config_bus, 1u, 1u) != 0 ||
                        pi4_xhci_dma_status_stage_seed(get_status, 1u) != 0) {
                        g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_GET_CONFIG_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_child_get_config_setup_control =
                        get_setup[3];
                    g_pi4_xhci_dma_child_get_config_data_control =
                        get_data[3];
                    g_pi4_xhci_dma_child_get_config_status_control =
                        get_status[3];
                    *get_config_value = 0xa5u;
                    pi4_dma_clean_bytes(child_ep0_ring,
                                        PI4_XHCI_DMA_RING_ALLOC_BYTES);
                    pi4_dma_clean_invalidate_bytes(get_config_value, 64u);
                    __asm__ volatile("dsb sy" ::: "memory");
                    g_pi4_xhci_dma_stage = 33;
                    child_transfer_rc =
                        pi4_xhci_dma_control_transfer_submit_wait(
                            op, rt, db, event_memory, event_bus,
                            g_pi4_xhci_dma_child_get_config_status_bus,
                            child_slot_id, 1u, &event_cursor);
                    g_pi4_xhci_dma_child_get_config_transfer_rc =
                        (uint32_t)child_transfer_rc;
                    g_pi4_xhci_dma_child_get_config_event_parameter =
                        g_pi4_xhci_dma_transfer_parameter;
                    g_pi4_xhci_dma_child_get_config_event_status =
                        g_pi4_xhci_dma_transfer_status;
                    g_pi4_xhci_dma_child_get_config_event_control =
                        g_pi4_xhci_dma_transfer_control;
                    g_pi4_xhci_dma_child_get_config_event_checks =
                        g_pi4_xhci_dma_transfer_checks;
                    if (child_transfer_rc <= 0) {
                        if (child_transfer_rc == -2 &&
                            (g_pi4_xhci_dma_transfer_checks &
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER)) ==
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER) &&
                            (g_pi4_xhci_dma_transfer_parameter & ~0xfull) ==
                                (g_pi4_xhci_dma_child_get_config_status_bus &
                                 ~0xfull)) {
                            __asm__ volatile("dmb oshld\n\tdsb sy" :::
                                             "memory");
                            pi4_dma_invalidate_bytes(get_config_value, 64u);
                            g_pi4_xhci_dma_child_get_config_observed = 1u;
                            g_pi4_xhci_dma_child_get_config_value =
                                *get_config_value;
                        }
                        g_pi4_xhci_dma_state = child_transfer_rc == 0
                            ? PI4_XHCI_DMA_CHILD_GET_CONFIG_TIMEOUT
                            : (child_transfer_rc == -1
                                ? PI4_XHCI_DMA_MMIO_FAIL
                                : PI4_XHCI_DMA_CHILD_GET_CONFIG_BAD);
                        goto cleanup;
                    }
                    __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                    pi4_dma_invalidate_bytes(get_config_value, 64u);
                    g_pi4_xhci_dma_child_get_config_observed = 1u;
                    g_pi4_xhci_dma_child_get_config_value =
                        *get_config_value;
                    if (g_pi4_xhci_dma_child_get_config_value != config_value) {
                        g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_GET_CONFIG_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_GET_CONFIG_PASS;
                }

                if (g_pi4_keyboard.attempted) {
                    if (!g_pi4_keyboard.interface.found) {
                        g_pi4_xhci_dma_state = g_pi4_keyboard.mouse ?
                            PI4_XHCI_DMA_MOUSE_UNSUPPORTED : PI4_XHCI_DMA_KEYBOARD_UNSUPPORTED;
                        goto cleanup;
                    }
                    (void)pi4_usb_keyboard_start(h, op, rt, db, event_memory,
                        event_bus, &event_cursor, child_slot_id, context_size,
                        command_memory, command_bus, input_context, input_context_bus,
                        child_output_context, child_ep0_ring, child_ep0_bus,
                        child_bulk_in_ring, child_bulk_in_bus, control_buffer,
                        control_buffer_bus, original_pci);
                    goto cleanup;
                }

                /* GET_MAX_LUN is a read-only BOT class request over EP0. It
                 * remains safe to probe even if the endpoint descriptors
                 * later prove incompatible with the child port's speed. */
                {
                    const uint32_t max_lun_offset = 2112u;
                    uint8_t *max_lun = control_buffer + max_lun_offset;
                    uint32_t *setup = (uint32_t *)(void *)(child_ep0_ring +
                        (uint64_t)PI4_XHCI_DMA_CHILD_BOT_GET_MAX_LUN_SETUP_TRB * 16u);
                    uint32_t *data = setup + 4u;
                    uint32_t *status = setup + 8u;
                    unsigned interface_number =
                        g_pi4_xhci_dma_child_config_summary.bot_interface_number;

                    g_pi4_xhci_dma_stage = 34;
                    g_pi4_xhci_dma_child_get_max_lun_buffer_bus =
                        control_buffer_bus + max_lun_offset;
                    g_pi4_xhci_dma_child_get_max_lun_status_bus =
                        child_ep0_bus +
                        ((uint64_t)PI4_XHCI_DMA_CHILD_BOT_GET_MAX_LUN_SETUP_TRB +
                         2u) * 16u;
                    g_pi4_xhci_dma_child_get_max_lun_prepare = 0u;
                    if (!g_pi4_xhci_dma_child_config_summary.bot_interface_found) {
                        g_pi4_xhci_dma_child_get_max_lun_prepare = 1u;
                    } else if (interface_number > 0xffu) {
                        g_pi4_xhci_dma_child_get_max_lun_prepare = 2u;
                    } else if (pi4_xhci_dma_bot_get_max_lun_setup_seed(setup,
                            interface_number, 1u) != 0) {
                        g_pi4_xhci_dma_child_get_max_lun_prepare = 3u;
                    } else if (pi4_xhci_dma_data_stage_seed(data,
                            g_pi4_xhci_dma_child_get_max_lun_buffer_bus,
                            1u, 1u) != 0) {
                        g_pi4_xhci_dma_child_get_max_lun_prepare = 4u;
                    } else if (pi4_xhci_dma_status_stage_seed(status, 1u) != 0) {
                        g_pi4_xhci_dma_child_get_max_lun_prepare = 5u;
                    } else {
                        g_pi4_xhci_dma_child_get_max_lun_prepare = 6u;
                    }
                    if (g_pi4_xhci_dma_child_get_max_lun_prepare != 6u) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_GET_MAX_LUN_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_child_get_max_lun_setup_control = setup[3];
                    g_pi4_xhci_dma_child_get_max_lun_data_control = data[3];
                    g_pi4_xhci_dma_child_get_max_lun_status_control = status[3];
                    *max_lun = 0xa5u;
                    pi4_dma_clean_bytes(child_ep0_ring,
                                        PI4_XHCI_DMA_RING_ALLOC_BYTES);
                    pi4_dma_clean_invalidate_bytes(max_lun, 64u);
                    __asm__ volatile("dsb sy" ::: "memory");
                    child_transfer_rc =
                        pi4_xhci_dma_control_transfer_submit_wait(
                            op, rt, db, event_memory, event_bus,
                            g_pi4_xhci_dma_child_get_max_lun_status_bus,
                            child_slot_id, 1u, &event_cursor);
                    g_pi4_xhci_dma_child_get_max_lun_transfer_rc =
                        child_transfer_rc;
                    g_pi4_xhci_dma_child_get_max_lun_event_parameter =
                        g_pi4_xhci_dma_transfer_parameter;
                    g_pi4_xhci_dma_child_get_max_lun_event_status =
                        g_pi4_xhci_dma_transfer_status;
                    g_pi4_xhci_dma_child_get_max_lun_event_control =
                        g_pi4_xhci_dma_transfer_control;
                    g_pi4_xhci_dma_child_get_max_lun_event_checks =
                        g_pi4_xhci_dma_transfer_checks;
                    if (child_transfer_rc <= 0) {
                        if (child_transfer_rc == -2 &&
                            (g_pi4_xhci_dma_transfer_checks &
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER)) ==
                                (PI4_XHCI_DMA_TRANSFER_CHECK_TYPE |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_ENDPOINT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_SLOT |
                                 PI4_XHCI_DMA_TRANSFER_CHECK_POINTER) &&
                            (g_pi4_xhci_dma_transfer_parameter & ~0xfull) ==
                                (g_pi4_xhci_dma_child_get_max_lun_status_bus &
                                 ~0xfull)) {
                            __asm__ volatile("dmb oshld\n\tdsb sy" :::
                                             "memory");
                            pi4_dma_invalidate_bytes(max_lun, 64u);
                            g_pi4_xhci_dma_child_get_max_lun_observed = 1u;
                            g_pi4_xhci_dma_child_get_max_lun_value = *max_lun;
                        }
                        g_pi4_xhci_dma_state = child_transfer_rc == 0
                            ? PI4_XHCI_DMA_CHILD_GET_MAX_LUN_TIMEOUT
                            : (child_transfer_rc == -1
                                ? PI4_XHCI_DMA_MMIO_FAIL
                                : PI4_XHCI_DMA_CHILD_GET_MAX_LUN_BAD);
                        goto cleanup;
                    }
                    __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                    pi4_dma_invalidate_bytes(max_lun, 64u);
                    g_pi4_xhci_dma_child_get_max_lun_observed = 1u;
                    g_pi4_xhci_dma_child_get_max_lun_value = *max_lun;
                    if (*max_lun > 15u) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_GET_MAX_LUN_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_state = PI4_XHCI_DMA_CHILD_GET_MAX_LUN_PASS;
                }

                /* Do not configure an endpoint context whose advertised
                 * max-packet size is impossible at this link speed. The
                 * descriptors and GET_MAX_LUN result remain useful evidence,
                 * while avoiding a malformed Configure Endpoint command. */
                if (!pi4_xhci_dma_bulk_max_packet_valid(
                        g_pi4_xhci_dma_child_speed_id,
                        g_pi4_xhci_dma_child_config_summary.bot_bulk_in_max_packet) ||
                    !pi4_xhci_dma_bulk_max_packet_valid(
                        g_pi4_xhci_dma_child_speed_id,
                        g_pi4_xhci_dma_child_config_summary.bot_bulk_out_max_packet)) {
                    g_pi4_xhci_dma_state =
                        PI4_XHCI_DMA_CHILD_BULK_SPEED_MISMATCH;
                    goto cleanup;
                }

                /* Configure only the selected BOT interface's bulk endpoint
                 * contexts. No Bulk-Only Transport CBW, data, or CSW is sent
                 * in this bounded step. The guard No-op is placed at entry 0
                 * after the command-ring Link TRB; entry 1 stays cycle-invalid
                 * until the guard completion proves the wrapped position. */
                {
                    struct pi4_xhci_dma_config_summary *summary =
                        &g_pi4_xhci_dma_child_config_summary;
                    uint32_t *guard_trb = (uint32_t *)(void *)(command_memory +
                        (uint64_t)PI4_XHCI_DMA_COMMAND_CHILD_BULK_GUARD_NOOP_TRB * 16u);
                    uint32_t *configure_trb = (uint32_t *)(void *)(command_memory +
                        (uint64_t)PI4_XHCI_DMA_COMMAND_CHILD_BULK_CONFIGURE_TRB * 16u);
                    unsigned returned_slot = 0;
                    int bulk_rc;

                    g_pi4_xhci_dma_stage = 34;
                    if (!summary->bot_interface_found ||
                        summary->bot_interface_alt != 0u ||
                        summary->bot_interface_endpoint_count != 2u ||
                        summary->bot_bulk_in_address == UINT32_MAX ||
                        summary->bot_bulk_out_address == UINT32_MAX ||
                        pi4_xhci_dma_transfer_ring_seed(child_bulk_in_ring,
                            child_bulk_in_bus) != 0 ||
                        pi4_xhci_dma_transfer_ring_seed(child_bulk_out_ring,
                            child_bulk_out_bus) != 0) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_BULK_CONFIG_BAD;
                        goto cleanup;
                    }
                    __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                    pi4_dma_invalidate_bytes(child_output_context,
                                             PI4_XHCI_DMA_CONTEXT_BYTES);
                    if (pi4_xhci_dma_bulk_contexts_seed(input_context,
                            PI4_XHCI_DMA_CONTEXT_BYTES, context_size,
                            child_output_context, PI4_XHCI_DMA_CONTEXT_BYTES,
                            8u, 6u, 0x50u, summary->bot_interface_alt,
                            summary->bot_interface_endpoint_count,
                            summary->bot_bulk_in_address,
                            summary->bot_bulk_in_attributes,
                            summary->bot_bulk_in_max_packet, 0u,
                            summary->bot_bulk_out_address,
                            summary->bot_bulk_out_attributes,
                            summary->bot_bulk_out_max_packet, 0u,
                            g_pi4_xhci_dma_child_speed_id,
                            child_bulk_in_bus, child_bulk_out_bus) != 0 ||
                        pi4_xhci_dma_noop_seed(guard_trb, 0u) != 0) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_BULK_CONFIG_BAD;
                        goto cleanup;
                    }
                    guard_trb[4] = guard_trb[5] = guard_trb[6] = 0;
                    guard_trb[7] = 1u; /* Entry 1 invalid for PCS=0. */
                    pi4_dma_clean_bytes(input_context,
                                        PI4_XHCI_DMA_CONTEXT_BYTES);
                    pi4_dma_clean_bytes(child_bulk_in_ring,
                                        PI4_XHCI_DMA_RING_ALLOC_BYTES);
                    pi4_dma_clean_bytes(child_bulk_out_ring,
                                        PI4_XHCI_DMA_RING_ALLOC_BYTES);
                    pi4_dma_clean_bytes((uint8_t *)(void *)guard_trb, 32u);
                    __asm__ volatile("dsb sy" ::: "memory");
                    g_pi4_xhci_dma_stage = 35;
                    bulk_rc = pi4_xhci_dma_command_submit_wait(op, rt, db,
                        event_memory, event_bus, command_bus,
                        PI4_XHCI_DMA_COMMAND_CHILD_BULK_GUARD_NOOP_TRB,
                        0u, &event_cursor, &returned_slot,
                        &g_pi4_xhci_dma_child_bulk_guard_status,
                        &g_pi4_xhci_dma_child_bulk_guard_control);
                    g_pi4_xhci_dma_child_bulk_guard_rc = bulk_rc;
                    if (bulk_rc <= 0) {
                        g_pi4_xhci_dma_state = bulk_rc == 0
                            ? PI4_XHCI_DMA_CHILD_BULK_CONFIG_TIMEOUT
                            : (bulk_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                : PI4_XHCI_DMA_CHILD_BULK_CONFIG_BAD);
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_command_completions++;

                    if (pi4_xhci_dma_configure_endpoint_seed(configure_trb,
                            input_context_bus, child_slot_id, 0u) != 0) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_BULK_CONFIG_BAD;
                        goto cleanup;
                    }
                    pi4_dma_clean_bytes((uint8_t *)(void *)configure_trb, 16u);
                    pi4_dma_clean_invalidate_bytes(child_output_context,
                                                   PI4_XHCI_DMA_CONTEXT_BYTES);
                    __asm__ volatile("dsb sy" ::: "memory");
                    g_pi4_xhci_dma_stage = 36;
                    bulk_rc = pi4_xhci_dma_command_submit_wait(op, rt, db,
                        event_memory, event_bus, command_bus,
                        PI4_XHCI_DMA_COMMAND_CHILD_BULK_CONFIGURE_TRB,
                        child_slot_id, &event_cursor, &returned_slot,
                        &g_pi4_xhci_dma_child_bulk_config_status,
                        &g_pi4_xhci_dma_child_bulk_config_control);
                    g_pi4_xhci_dma_child_bulk_config_rc = bulk_rc;
                    if (bulk_rc <= 0) {
                        g_pi4_xhci_dma_state = bulk_rc == 0
                            ? PI4_XHCI_DMA_CHILD_BULK_CONFIG_TIMEOUT
                            : (bulk_rc == -1 ? PI4_XHCI_DMA_MMIO_FAIL
                                : PI4_XHCI_DMA_CHILD_BULK_CONFIG_BAD);
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_command_completions++;
                    __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                    pi4_dma_invalidate_bytes(child_output_context,
                                             PI4_XHCI_DMA_CONTEXT_BYTES);
                    {
                        const uint32_t *slot_context =
                            (const uint32_t *)(const void *)child_output_context;
                        const uint32_t *in_ep = (const uint32_t *)(const void *)(
                            child_output_context +
                            ((uint64_t)(((summary->bot_bulk_in_address & 0x0fu) * 2u) + 1u) *
                                        context_size));
                        const uint32_t *out_ep = (const uint32_t *)(const void *)(
                            child_output_context +
                            ((uint64_t)((summary->bot_bulk_out_address & 0x0fu) * 2u) *
                                        context_size));
                        for (unsigned i = 0; i < 6u; i++)
                            g_pi4_xhci_dma_child_bulk_slot_words[i] =
                                slot_context[i];
                        for (unsigned i = 0; i < 5u; i++) {
                            g_pi4_xhci_dma_child_bulk_in_words[i] = in_ep[i];
                            g_pi4_xhci_dma_child_bulk_out_words[i] = out_ep[i];
                        }
                    }
                    if (!pi4_xhci_dma_bulk_contexts_ok(
                            child_output_context, PI4_XHCI_DMA_CONTEXT_BYTES,
                            context_size, summary->bot_bulk_in_address,
                            summary->bot_bulk_in_max_packet, 0u, child_bulk_in_bus,
                            summary->bot_bulk_out_address,
                            summary->bot_bulk_out_max_packet, 0u,
                            child_bulk_out_bus)) {
                        g_pi4_xhci_dma_state =
                            PI4_XHCI_DMA_CHILD_BULK_CONFIG_BAD;
                        goto cleanup;
                    }
                    g_pi4_xhci_dma_child_bulk_contexts_verified = 1u;
                    g_pi4_xhci_dma_stage = 37;
                    g_pi4_xhci_dma_state =
                        PI4_XHCI_DMA_CHILD_BULK_CONFIG_PASS;
                }
            }
        }
    }

cleanup:
    if (g_pi4_bus.active && (g_pi4_xhci_dma_state == PI4_XHCI_DMA_BUS_READY ||
                            g_pi4_xhci_dma_state == PI4_XHCI_DMA_BUS_INPUT_ONLY ||
                            g_pi4_xhci_dma_state == PI4_XHCI_DMA_BUS_STORAGE_PENDING)) {
        g_pi4_xhci_dma_event_cursor_index = g_pi4_usb_live.events.index;
        g_pi4_xhci_dma_event_cursor_cycle = g_pi4_usb_live.events.cycle;
        (void)pi4_xhci_read_mmio32(op, 0, PI4_XHCI_MMIO_BYTES, &g_pi4_xhci_dma_runtime_cmd);
        (void)pi4_xhci_read_mmio32(op, 4, PI4_XHCI_MMIO_BYTES, &g_pi4_xhci_dma_runtime_sts);
        return;
    }
    if (g_pi4_keyboard.active &&
        (g_pi4_xhci_dma_state == PI4_XHCI_DMA_KEYBOARD_READY ||
         g_pi4_xhci_dma_state == PI4_XHCI_DMA_MOUSE_READY)) {
        g_pi4_xhci_dma_event_cursor_index = g_pi4_usb_live.events.index;
        g_pi4_xhci_dma_event_cursor_cycle = g_pi4_usb_live.events.cycle;
        (void)pi4_xhci_read_mmio32(op, 0, PI4_XHCI_MMIO_BYTES,
                                   &g_pi4_xhci_dma_runtime_cmd);
        (void)pi4_xhci_read_mmio32(op, 4, PI4_XHCI_MMIO_BYTES,
                                   &g_pi4_xhci_dma_runtime_sts);
        return;
    }
    if (g_pi4_xhci_dma_state == PI4_XHCI_DMA_ROOT_FILE_PASS) {
        int live_ok = pi4_usb_live_start(h, command_memory, command_bus, original_pci);
        event_cursor = g_pi4_usb_live.events;
        if (live_ok) {
            /* Persistent ownership: the static arena, translations and all
             * controller pointers remain valid after this stack unwinds. */
            g_pi4_xhci_dma_event_cursor_index = event_cursor.index;
            g_pi4_xhci_dma_event_cursor_cycle = event_cursor.cycle;
            (void)pi4_xhci_read_mmio32(op, 0, PI4_XHCI_MMIO_BYTES,
                                       &g_pi4_xhci_dma_runtime_cmd);
            (void)pi4_xhci_read_mmio32(op, 4, PI4_XHCI_MMIO_BYTES,
                                       &g_pi4_xhci_dma_runtime_sts);
            return;
        }
        g_pi4_usb_live.fault = 1u;
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_LIVE_BAD;
    }
    g_pi4_xhci_dma_event_cursor_index = event_cursor.index;
    g_pi4_xhci_dma_event_cursor_cycle = event_cursor.cycle;
    if (bme_may_be_set) {
        /* Preserve live state and the first event TRB before halting the xHC. */
        (void)pi4_xhci_read_mmio32(op, 0, PI4_XHCI_MMIO_BYTES,
                                   &g_pi4_xhci_dma_runtime_cmd);
        (void)pi4_xhci_read_mmio32(op, 4, PI4_XHCI_MMIO_BYTES,
                                   &g_pi4_xhci_dma_runtime_sts);
        (void)pi4_xhci_read_mmio32(op, 0x18, PI4_XHCI_MMIO_BYTES,
                                   &g_pi4_xhci_dma_crcr_lo);
        (void)pi4_xhci_read_mmio32(op, 0x1c, PI4_XHCI_MMIO_BYTES,
                                   &g_pi4_xhci_dma_crcr_hi);
        (void)pi4_xhci_read_mmio32(rt, 0x38, PI4_XHCI_MMIO_BYTES,
                                   &g_pi4_xhci_dma_erdp_lo);
        (void)pi4_xhci_read_mmio32(rt, 0x3c, PI4_XHCI_MMIO_BYTES,
                                   &g_pi4_xhci_dma_erdp_hi);
        {
            uint8_t *entry = event_memory +
                (uint64_t)event_cursor.index * 16u;
            volatile uint32_t *event = (volatile uint32_t *)(void *)entry;
            __asm__ volatile("dc ivac, %0" :: "r"(entry) : "memory");
            __asm__ volatile("dsb sy" ::: "memory");
            for (unsigned i = 0; i < 4; i++)
                g_pi4_xhci_dma_event_trb[i] = event[i];
        }
        /* Keep the reserved rings alive until the controller is halted. */
        if (pi4_xhci_write_mmio32(op, 0, PI4_XHCI_MMIO_BYTES, 0) != 0 ||
            !pi4_xhci_dma_wait_status(op, 4, PI4_XHCI_USBSTS_HALTED,
                                      PI4_XHCI_USBSTS_HALTED,
                                      &g_pi4_xhci_dma_status)) {
            cleanup_ok = 0;
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_STOP_TIMEOUT;
        } else {
            /* The controller can no longer write this buffer. Capture it on
             * failure too, so event validation and inbound DMA are separable. */
            if (g_pi4_xhci_dma_stage >= 19 && output_context) {
                unsigned context_size =
                    pi4_xhci_dma_context_size(g_pi4_xhci_caps[3]);
                const uint8_t *ep0_context = output_context + context_size;
                pi4_dma_invalidate_bytes(output_context,
                                         PI4_XHCI_DMA_CONTEXT_BYTES);
                for (unsigned i = 0; i < 5u; i++)
                    g_pi4_xhci_dma_output_ep0_context[i] =
                        ((const uint32_t *)(const void *)ep0_context)[i];
                g_pi4_xhci_dma_output_ep0_state =
                    g_pi4_xhci_dma_output_ep0_context[0] & 7u;
                g_pi4_xhci_dma_output_ep0_dcs =
                    g_pi4_xhci_dma_output_ep0_context[2] & 1u;
                g_pi4_xhci_dma_output_ep0_dequeue_bus =
                    ((uint64_t)g_pi4_xhci_dma_output_ep0_context[3] << 32) |
                    (g_pi4_xhci_dma_output_ep0_context[2] & ~0x0fu);
                g_pi4_xhci_dma_output_ep0_context_valid = 1u;
                if (g_pi4_xhci_dma_hub_probe_phase == 2u &&
                    g_pi4_xhci_dma_hub_probe_port >= 1u &&
                    g_pi4_xhci_dma_hub_probe_port <= 4u) {
                    uint8_t *port_buffer = control_buffer + 256u +
                        (g_pi4_xhci_dma_hub_probe_port - 1u) * 16u;
                    pi4_dma_invalidate_bytes(port_buffer, 64u);
                    g_pi4_xhci_dma_hub_status_buffer_words[0] =
                        ((uint32_t *)(void *)port_buffer)[0];
                    g_pi4_xhci_dma_hub_status_buffer_words[1] =
                        ((uint32_t *)(void *)port_buffer)[1];
                }
            }
            if (g_pi4_xhci_dma_stage >= 14 && control_buffer) {
                __asm__ volatile("dmb oshld\n\tdsb sy" ::: "memory");
                pi4_dma_invalidate_bytes(control_buffer,
                                         PI4_XHCI_DMA_CONTROL_BUFFER_BYTES);
                g_pi4_xhci_dma_descriptor_prefix[0] =
                    ((uint32_t *)(void *)control_buffer)[0];
                g_pi4_xhci_dma_descriptor_prefix[1] =
                    ((uint32_t *)(void *)control_buffer)[1];
            }
            if (g_pi4_xhci_dma_stage >= 15 && control_buffer) {
                uint8_t *config_buffer = control_buffer + 64u;
                const uint32_t *words =
                    (const uint32_t *)(const void *)config_buffer;
                pi4_dma_invalidate_bytes(config_buffer, 64u);
                for (unsigned i = 0; i < 3; i++)
                    g_pi4_xhci_dma_config_header_words[i] = words[i];
                if (config_buffer[0] == 9u && config_buffer[1] == 2u) {
                    g_pi4_xhci_dma_config_total_length = config_buffer[2] |
                        ((uint32_t)config_buffer[3] << 8);
                    g_pi4_xhci_dma_config_interfaces = config_buffer[4];
                    g_pi4_xhci_dma_config_value = config_buffer[5];
                    g_pi4_xhci_dma_config_attributes = config_buffer[7];
                    g_pi4_xhci_dma_config_max_power = config_buffer[8];
                }
            }
            if (g_pi4_xhci_dma_stage >= 17 && control_buffer) {
                uint8_t *hub_buffer = control_buffer + 128u;
                const uint32_t *words =
                    (const uint32_t *)(const void *)hub_buffer;
                pi4_dma_invalidate_bytes(hub_buffer, 64u);
                for (unsigned i = 0; i < 4; i++)
                    g_pi4_xhci_dma_hub_descriptor_words[i] = words[i];
                g_pi4_xhci_dma_hub_descriptor_validated =
                    pi4_xhci_dma_hub_descriptor_ok(hub_buffer, 9u) ? 1u : 0u;
                if (hub_buffer[1] == 0x29u) {
                    g_pi4_xhci_dma_hub_ports = hub_buffer[2];
                    g_pi4_xhci_dma_hub_characteristics = hub_buffer[3] |
                        ((uint32_t)hub_buffer[4] << 8);
                    g_pi4_xhci_dma_hub_power_delay = hub_buffer[5];
                    g_pi4_xhci_dma_hub_current = hub_buffer[6];
                    g_pi4_xhci_dma_hub_removable = hub_buffer[7];
                    g_pi4_xhci_dma_hub_power_mask = hub_buffer[8];
                }
            }
        }
        if (pi4_pcie_write_config32(h, g_pi4_link_status, 0x04,
                                    original_pci) != 0 ||
            pi4_pcie_read_config32(h, g_pi4_link_status, 0x04, &verify) != 0 ||
            (uint16_t)verify != original_pci ||
            ((uint16_t)verify & PI4_PCI_COMMAND_MASTER)) {
            cleanup_ok = 0;
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_PCI_RESTORE_FAIL;
        } else {
            g_pi4_cfg_command = verify;
        }
    }
    if (bar2_may_be_set) {
        if (!cleanup_ok) {
            /* Do not change inbound translation unless we proved xHCI halted
             * and PCI BME restored off. A power cycle resets the bridge. */
            g_pi4_xhci_dma_bar2_state |= 1u << 31;
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_INBOUND_RESTORE_FAIL;
        } else {
            g_pi4_xhci_dma_bar2_state |= 1u << 3; /* Restore attempted. */
            if (pi4_pcie_write_mmio32(h, PI4_PCIE_INBOUND_BAR2_HI,
                                      g_pi4_xhci_dma_bar2_hi_original) != 0 ||
                pi4_pcie_read_mmio32(h, PI4_PCIE_INBOUND_BAR2_HI,
                                     &verify) != 0 ||
                verify != g_pi4_xhci_dma_bar2_hi_original) {
                cleanup_ok = 0;
                g_pi4_xhci_dma_bar2_state |= 1u << 31;
                g_pi4_xhci_dma_state = PI4_XHCI_DMA_INBOUND_RESTORE_FAIL;
            } else {
                g_pi4_xhci_dma_bar2_hi_restored = verify;
                g_pi4_xhci_dma_bar2_state |= 1u << 4; /* Restore verified. */
            }
        }
    } else if (g_pi4_xhci_dma_bar2_state & (1u << 0)) {
        g_pi4_xhci_dma_bar2_hi_restored =
            g_pi4_xhci_dma_bar2_hi_original;
    }
    if (g_pi4_xhci_dma_state == PI4_XHCI_DMA_NOT_RUN)
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_BAD_EVENT;
    if (g_pi4_usb_live.attempted && cleanup_ok) g_pi4_usb_live.stopped = 1u;
    return;

preflight_restore:
    if (pci_mse_may_be_set &&
        (pi4_pcie_write_config32(h, g_pi4_link_status, 0x04,
                                 original_pci) != 0 ||
         pi4_pcie_read_config32(h, g_pi4_link_status, 0x04, &verify) != 0 ||
         (uint16_t)verify != original_pci ||
         ((uint16_t)verify & PI4_PCI_COMMAND_MASTER))) {
        g_pi4_xhci_dma_state = PI4_XHCI_DMA_PCI_RESTORE_FAIL;
    } else if (pci_mse_may_be_set) {
        g_pi4_cfg_command = verify;
    }
    if (bar2_may_be_set) {
        g_pi4_xhci_dma_bar2_state |= 1u << 3;
        if (pi4_pcie_write_mmio32(h, PI4_PCIE_INBOUND_BAR2_HI,
                                  g_pi4_xhci_dma_bar2_hi_original) != 0 ||
            pi4_pcie_read_mmio32(h, PI4_PCIE_INBOUND_BAR2_HI,
                                 &verify) != 0 ||
            verify != g_pi4_xhci_dma_bar2_hi_original) {
            g_pi4_xhci_dma_bar2_state |= 1u << 31;
            g_pi4_xhci_dma_state = PI4_XHCI_DMA_INBOUND_RESTORE_FAIL;
        } else {
            g_pi4_xhci_dma_bar2_hi_restored = verify;
            g_pi4_xhci_dma_bar2_state |= 1u << 4;
        }
    } else if (g_pi4_xhci_dma_bar2_state & (1u << 0)) {
        g_pi4_xhci_dma_bar2_hi_restored =
            g_pi4_xhci_dma_bar2_hi_original;
    }
}

/* Keep the selected USB route and its device identity together, so the
 * parent hub's descriptor cannot be mistaken for the downstream device. */
#if defined(ZEAL_FORCE_PI4) && !defined(ZEAL_PI_DIAG)
static void con_puts(const char *s);
#endif
static void pi4_xhci_route_text(int framebuffer, const char *text) {
#if defined(ZEAL_FORCE_PI4) && !defined(ZEAL_PI_DIAG)
    if (framebuffer) con_puts(text);
#else
    if (framebuffer) pi_diag_fb_puts(text);
#endif
    else uart_puts(g_uart, text);
}

static void pi4_xhci_route_hex(int framebuffer, uint32_t value) {
#if defined(ZEAL_FORCE_PI4) && !defined(ZEAL_PI_DIAG)
    if (framebuffer) {
        static const char hex[]="0123456789abcdef";
        char text[11]="0x00000000";
        for (unsigned i=0;i<8u;i++) text[2u+i]=hex[(value>>(28u-4u*i))&15u];
        con_puts(text);
    }
#else
    if (framebuffer) pi_diag_fb_u32_hex(value);
#endif
    else uart_put_u64_hex(g_uart, value);
}

static void pi4_usb_hid_report(int framebuffer) {
    pi4_xhci_route_text(framebuffer, g_pi4_keyboard.mouse ?
        "\nUSB mouse MOUSE-BOOT-1 selected/active/stage/fault=" :
        "\nUSB keyboard KBD-BOOT-1 selected/active/stage/fault=");
    pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.interface.found);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.active);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.stage);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.fault);
    pi4_xhci_route_text(framebuffer, "\nUSB HID interface/endpoint/packet/interval/protocol=");
    unsigned values[5] = {g_pi4_keyboard.interface.interface,
        g_pi4_keyboard.interface.endpoint, g_pi4_keyboard.interface.packet,
        g_pi4_keyboard.interface.interval, g_pi4_keyboard.protocol};
    for (unsigned i = 0; i < 5u; i++) {
        if (i) pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, values[i]);
    }
    pi4_xhci_route_text(framebuffer, g_pi4_keyboard.mouse ?
        "\nUSB mouse SET_PROTOCOL/GET_PROTOCOL rc/status=" :
        "\nUSB keyboard SET_PROTOCOL/GET_PROTOCOL/SET_IDLE rc/status=");
    for (unsigned i = 0; i < (g_pi4_keyboard.mouse ? 2u : 3u); i++) {
        if (i) pi4_xhci_route_text(framebuffer, " ");
        pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.request_rc[i]);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.request_status[i]);
    }
    pi4_xhci_route_text(framebuffer, "\nUSB HID guard/configure rc/status=");
    for (unsigned i = 0; i < 2u; i++) {
        if (i) pi4_xhci_route_text(framebuffer, " ");
        pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.command_rc[i]);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.command_status[i]);
    }
    pi4_xhci_route_text(framebuffer, g_pi4_keyboard.mouse ?
        "\nUSB mouse reports/moves/ring-wraps/event-status/control=" :
        "\nUSB keyboard reports/keys/ring-wraps/event-status/control=");
    pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.reports);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.mouse ?
        g_pi4_keyboard.mouse_state.moves : g_pi4_keyboard.keys);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.producer.wraps);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.last_status);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_keyboard.last_control);
    if (g_pi4_keyboard.mouse) {
        const struct pi4_usb_mouse_state *m = &g_pi4_keyboard.mouse_state;
        unsigned fields[] = {m->x, m->y, m->buttons, m->seen_buttons,
                             m->presses, m->releases};
        pi4_xhci_route_text(framebuffer, "\nUSB mouse x/y/buttons/seen/presses/releases=");
        for (unsigned i = 0; i < 6u; i++) {
            if (i) pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, fields[i]);
        }
    }
}

/* Print after real movement/button activity at most once per 3 seconds.
 * Idle reports do not scroll HDMI, even if a device sends them continuously. */
static void pi4_usb_mouse_activity_report(void) {
    if (!pi4_usb_mouse_ready()) return;
    const struct pi4_usb_mouse_state *m = &g_pi4_keyboard.mouse_state;
    unsigned activity = m->moves + m->presses + m->releases;
    if (activity == g_pi4_keyboard.reported_activity) return;
    uint64_t now;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
    if (now < g_pi4_keyboard.report_at) return;
    g_pi4_keyboard.report_at = now + 3u * g_pi4_usb_live.frequency;
    g_pi4_keyboard.reported_activity = activity;
    unsigned values[] = {g_pi4_keyboard.active, g_pi4_keyboard.fault, m->reports,
        m->moves, g_pi4_keyboard.producer.wraps, m->x, m->y, m->buttons,
        m->seen_buttons, m->presses, m->releases};
    for (unsigned framebuffer = 0; framebuffer < 2u; framebuffer++) {
        pi4_xhci_route_text(framebuffer,
            "\nUSB mouse live active/fault/reports/moves/wraps/x/y/buttons/seen/presses/releases=");
        for (unsigned i = 0; i < 11u; i++) {
            if (i) pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, values[i]);
        }
        pi4_xhci_route_text(framebuffer, "\n");
    }
}

static void pi4_usb_live_report(int framebuffer) {
    if (g_pi4_usb_live.kind == 2u || g_pi4_usb_live.kind == 3u) {
        pi4_xhci_route_text(framebuffer, g_pi4_usb_live.kind == 3u ?
            "\nUSB bus ownership active/fault/stopped=" :
            "\nUSB HID ownership active/fault/stopped=");
        pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.active);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.fault);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.stopped);
        return;
    }
    pi4_xhci_route_text(framebuffer, "\nUSB live LIVE-READ-2 active/fault/stopped=");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.active);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.fault);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.stopped);
    pi4_xhci_route_text(framebuffer, " boot/idle-checks=");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.boot_checks);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.idle_checks);
    pi4_xhci_route_text(framebuffer, "\nUSB live wraps command/IN/OUT/event=");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.command.wraps);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.in.wraps);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.out.wraps);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_dma_events / PI4_XHCI_DMA_EVENT_TRBS);
    pi4_xhci_route_text(framebuffer, " file-after-wrap=");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.file_after_wrap);
    pi4_xhci_route_text(framebuffer, "\nUSB live stage/command-rc/status/control=");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.stage);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.command_rc);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.command_status);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_usb_live.command_control);
    if (g_pi4_usb_live.fault) {
        const struct pi4_xhci_storage_command_report *r = &g_pi4_usb_live.transport.last;
        pi4_xhci_route_text(framebuffer, "\nUSB live last tag/phase/valid/SCSI=");
        pi4_xhci_route_hex(framebuffer, r->tag);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, r->io.phase);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, r->transport_ok);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, r->scsi_status);
        if (r->io.phase && r->io.phase <= 3u) {
            unsigned phase = r->io.phase - 1u;
            pi4_xhci_route_text(framebuffer, " rc/event/checks=");
            pi4_xhci_route_hex(framebuffer, r->io.rc[phase]);
            pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, r->io.status[phase]);
            pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, r->io.checks[phase]);
        }
    }
}

static void pi4_xhci_route_report(int framebuffer) {
    static const char *steps[4] = {
        "config-read", "set-config", "get-config", "max-lun"
    };
    const struct pi4_xhci_dma_config_summary *s = &g_pi4_xhci_direct.config;
    pi4_usb_bus_report(framebuffer);
    if (g_pi4_keyboard.attempted) pi4_usb_hid_report(framebuffer);
    if (g_pi4_usb_live.attempted) pi4_usb_live_report(framebuffer);
    pi4_xhci_route_text(framebuffer, g_pi4_bus.attempted ?
        "\nUSB route MULTI-USB2-6 selected=" : g_pi4_keyboard.attempted ?
        (g_pi4_keyboard.mouse ? "\nUSB route MOUSE-BOOT-1 selected=" :
                               "\nUSB route KBD-BOOT-1 selected=") :
        "\nUSB route LIVE-READ-2 selected=");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_dma_port_event_port);
    pi4_xhci_route_text(framebuffer, " speed=");
    pi4_xhci_route_hex(framebuffer,
        pi4_xhci_dma_port_speed_id(g_pi4_xhci_dma_port_event_after));
    pi4_xhci_route_text(framebuffer, "\nUSB discovery scans/wait-ms=");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_dma_root_scans);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_dma_root_wait_ms);
    for (unsigned i = 0; i < g_pi4_xhci_dma_root_port_count; i++) {
        pi4_xhci_route_text(framebuffer, "\nUSB root ");
        pi4_xhci_route_hex(framebuffer, i + 1u);
        pi4_xhci_route_text(framebuffer, " proto=");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_dma_root_protocol[i]);
        pi4_xhci_route_text(framebuffer, " before=");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_dma_root_initial[i]);
        pi4_xhci_route_text(framebuffer, " after=");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_dma_root_portsc[i]);
    }
    if (!g_pi4_xhci_direct.step) return;
    pi4_xhci_route_text(framebuffer, "\nUSB direct vid/pid/class=");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_dma_device_vendor);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_dma_device_product);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_dma_device_class);
    pi4_xhci_route_text(framebuffer, "\nUSB direct config bytes/interfaces/BOT=");
    pi4_xhci_route_hex(framebuffer, s->total_length);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, s->unique_interfaces);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, s->bot_interface_found);
    pi4_xhci_route_text(framebuffer, "\nUSB direct first interface num/class/sub/proto=");
    const uint32_t fields[4] = {s->first_interface_number, s->first_interface_class,
        s->first_interface_subclass, s->first_interface_protocol};
    for (unsigned i = 0; i < 4u; i++) {
        if (i) pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, fields[i]);
    }
    for (unsigned i = 0; i < 4u; i++) {
        if (!(g_pi4_xhci_direct.attempted & (1u << i))) continue;
        pi4_xhci_route_text(framebuffer, "\nUSB direct ");
        pi4_xhci_route_text(framebuffer, steps[i]);
        pi4_xhci_route_text(framebuffer, " rc/event/checks=");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_direct.rc[i]);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_direct.status[i]);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_direct.checks[i]);
    }
    pi4_xhci_route_text(framebuffer, "\nUSB direct configured/max-LUN=");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_direct.configuration);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_direct.max_lun);
    if (!g_pi4_xhci_bot.step) return;
    pi4_xhci_route_text(framebuffer, "\nUSB bulk IN addr/MPS/burst=");
    pi4_xhci_route_hex(framebuffer, s->bot_bulk_in_address);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, s->bot_bulk_in_max_packet);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, s->bot_bulk_in_max_burst);
    pi4_xhci_route_text(framebuffer, " OUT=");
    pi4_xhci_route_hex(framebuffer, s->bot_bulk_out_address);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, s->bot_bulk_out_max_packet);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, s->bot_bulk_out_max_burst);
    for (unsigned i = 0; i < 2u; i++) {
        pi4_xhci_route_text(framebuffer, i ? "\nUSB bulk configure rc/event=" :
                                             "\nUSB bulk guard rc/event=");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.command_rc[i]);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.command_status[i]);
    }
    pi4_xhci_route_text(framebuffer, "\nUSB BOT step/contexts/CSW-ok=");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.step);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.verified);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.csw_ok);
    if (!g_pi4_xhci_bot.verified && g_pi4_xhci_bot.command_rc[1] == 1u) {
        pi4_xhci_route_text(framebuffer, "\nUSB bulk output Slot=");
        for (unsigned i = 0; i < 4u; i++) {
            if (i) pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.output_slot[i]);
        }
        for (unsigned direction = 0; direction < 2u; direction++) {
            pi4_xhci_route_text(framebuffer, direction ? "\nUSB bulk output OUT=" :
                                                        "\nUSB bulk output IN=");
            const uint32_t *words = direction ? g_pi4_xhci_bot.output_out :
                                               g_pi4_xhci_bot.output_in;
            for (unsigned i = 0; i < 5u; i++) {
                if (i) pi4_xhci_route_text(framebuffer, "/");
                pi4_xhci_route_hex(framebuffer, words[i]);
            }
        }
    }
    static const char *phases[3] = {"CBW", "INQUIRY", "CSW"};
    for (unsigned i = 0; i < 3u; i++) {
        if (g_pi4_xhci_bot.step < 3u + i) continue;
        pi4_xhci_route_text(framebuffer, "\nUSB BOT ");
        pi4_xhci_route_text(framebuffer, phases[i]);
        pi4_xhci_route_text(framebuffer, " rc/event/checks/bytes=");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.io.rc[i]);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.io.status[i]);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.io.checks[i]);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.io.actual[i]);
        if (g_pi4_xhci_dma_state == PI4_XHCI_DMA_ROOT_INQUIRY_BAD &&
            g_pi4_xhci_bot.step == 3u + i) {
            pi4_xhci_route_text(framebuffer, "\nUSB BOT event ctrl/ptr-hi/ptr-lo=");
            pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.io.control[i]);
            pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, (uint32_t)(g_pi4_xhci_bot.io.parameter[i] >> 32));
            pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, (uint32_t)g_pi4_xhci_bot.io.parameter[i]);
        }
    }
    pi4_xhci_route_text(framebuffer, "\nUSB BOT CSW sig/tag/residue/status=");
    for (unsigned i = 0; i < 4u; i++) {
        uint32_t word = 0;
        for (unsigned j = 0; j < (i == 3u ? 1u : 4u); j++)
            word |= (uint32_t)g_pi4_xhci_bot.csw[i * 4u + j] << (j * 8u);
        if (i) pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, word);
    }
    if (g_pi4_xhci_bot.step >= 5u) {
        pi4_xhci_route_text(framebuffer, "\nUSB INQUIRY type/format/additional=");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.inquiry[0]);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.inquiry[3]);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_xhci_bot.inquiry[4]);
    }
    if (g_pi4_xhci_bot.step == 6u) {
        pi4_xhci_route_text(framebuffer, "\nUSB INQUIRY vendor/product/rev=");
        pi4_xhci_route_text(framebuffer, g_pi4_xhci_bot.vendor);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_text(framebuffer, g_pi4_xhci_bot.product);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_text(framebuffer, g_pi4_xhci_bot.revision);
    }
    if (!g_pi4_xhci_storage.step) return;
    pi4_xhci_route_text(framebuffer, "\nUSB storage step/commands/reads/repeat=");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_storage.step);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_storage.count);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_storage.reads);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_storage.repeat_ok);
    pi4_xhci_route_text(framebuffer, "\nUSB capacity last-LBA/block/bytes-hi/lo=");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_storage.last_lba);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_storage.block_bytes);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, (uint32_t)(g_pi4_xhci_storage.capacity_bytes >> 32));
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, (uint32_t)g_pi4_xhci_storage.capacity_bytes);
    for (unsigned command = 0; command < g_pi4_xhci_storage.count; command++) {
        const struct pi4_xhci_storage_command_report *r = &g_pi4_xhci_storage.command[command];
        pi4_xhci_route_text(framebuffer, "\nUSB SCSI op/tag/data/status/valid=");
        pi4_xhci_route_hex(framebuffer, r->opcode);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, r->tag);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, r->io.actual[1]);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, r->scsi_status);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, r->transport_ok);
        if (r->opcode == 0x03u) {
            pi4_xhci_route_text(framebuffer, "\nUSB sense key/ASC/ASCQ=");
            pi4_xhci_route_hex(framebuffer, r->sense_key);
            pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, r->sense_asc);
            pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, r->sense_ascq);
        }
        if (!r->transport_ok && r->io.phase) {
            unsigned phase = r->io.phase - 1u;
            pi4_xhci_route_text(framebuffer, "\nUSB SCSI failure phase/rc/event/checks=");
            pi4_xhci_route_hex(framebuffer, r->io.phase);
            pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, r->io.rc[phase]);
            pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, r->io.status[phase]);
            pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, r->io.checks[phase]);
            pi4_xhci_route_text(framebuffer, "\nUSB SCSI event ctrl/ptr-hi/lo=");
            pi4_xhci_route_hex(framebuffer, r->io.control[phase]);
            pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, (uint32_t)(r->io.parameter[phase] >> 32));
            pi4_xhci_route_text(framebuffer, "/");
            pi4_xhci_route_hex(framebuffer, (uint32_t)r->io.parameter[phase]);
            pi4_xhci_route_text(framebuffer, "\nUSB SCSI CSW sig/tag/residue/status=");
            for (unsigned i = 0; i < 4u; i++) {
                uint32_t word = 0;
                for (unsigned j = 0; j < (i == 3u ? 1u : 4u); j++)
                    word |= (uint32_t)r->csw[i * 4u + j] << (j * 8u);
                if (i) pi4_xhci_route_text(framebuffer, "/");
                pi4_xhci_route_hex(framebuffer, word);
            }
        }
    }
    pi4_xhci_route_text(framebuffer, "\nUSB sector0 CRC32 first/repeat=");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_storage.crc[0]);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_storage.crc[1]);
    pi4_xhci_route_text(framebuffer, " sig/first-word=");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_storage.signature);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_xhci_storage.first_word);
    if (g_pi4_fat32.result == PI4_FAT32_NOT_RUN) return;
    pi4_xhci_route_text(framebuffer, "\nUSB FAT32 result=");
    pi4_xhci_route_text(framebuffer, pi4_fat32_result_name(g_pi4_fat32.result));
    pi4_xhci_route_text(framebuffer, " MBR-types=");
    for (unsigned i = 0; i < 4u; i++) {
        if (i) pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, g_pi4_fat32.partition_types[i]);
    }
    pi4_xhci_route_text(framebuffer, "\nUSB FAT32 partition/type/start/sectors=");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.partition_index);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.partition_type);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.partition_first);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.partition_sectors);
    pi4_xhci_route_text(framebuffer, "\nUSB FAT32 label=");
    pi4_xhci_route_text(framebuffer, g_pi4_fat32.label);
    pi4_xhci_route_text(framebuffer, " spc/active-FAT/root=");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.volume.sectors_per_cluster);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.volume.active_fat);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.volume.root_cluster);
    pi4_xhci_route_text(framebuffer, "\nUSB FAT32 FAT/data/clusters=");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.volume.fat_first);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.volume.data_first);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.volume.clusters);
    pi4_xhci_route_text(framebuffer, "\nUSB FAT32 listed/complete/bounded/dir-sectors=");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.listed);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.complete);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.bounded);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.directory_sectors);
    pi4_xhci_route_text(framebuffer, "\nUSB FAT32 read-count/last-LBA=");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.reads);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32.last_read);
    for (unsigned i = 0; i < g_pi4_fat32.listed; i++) {
        const struct pi4_fat32_entry *e = &g_pi4_fat32.entry[i];
        pi4_xhci_route_text(framebuffer, "\nUSB file ");
        pi4_xhci_route_text(framebuffer, e->name);
        pi4_xhci_route_text(framebuffer, " attr/cluster/bytes=");
        pi4_xhci_route_hex(framebuffer, e->attributes);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, e->cluster);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, e->bytes);
    }
    if (!g_pi4_fat32_file.attempted) return;
    pi4_xhci_route_text(framebuffer, "\nUSB test-file " PI4_FAT32_TEST_NAME " found/cluster/bytes=");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32_file.found);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32_file.cluster);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32_file.bytes);
    for (unsigned pass = 0; pass < 2u; pass++) {
        const struct pi4_fat32_file_report *r = &g_pi4_fat32_file.read[pass];
        pi4_xhci_route_text(framebuffer, pass ? "\nUSB test-file read2=" : "\nUSB test-file read1=");
        pi4_xhci_route_text(framebuffer, pi4_fat32_file_result_name(r->result));
        pi4_xhci_route_text(framebuffer, " bytes/reads/clusters/last-LBA=");
        pi4_xhci_route_hex(framebuffer, r->bytes);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, r->reads);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, r->clusters);
        pi4_xhci_route_text(framebuffer, "/");
        pi4_xhci_route_hex(framebuffer, r->last_read);
    }
    pi4_xhci_route_text(framebuffer, "\nUSB test-file CRC32 first/repeat/expected=");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32_file.crc[0]);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32_file.crc[1]);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, PI4_FAT32_TEST_CRC32);
    pi4_xhci_route_text(framebuffer, "\nUSB test-file pattern1/pattern2/repeat-match=");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32_file.pattern_ok[0]);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32_file.pattern_ok[1]);
    pi4_xhci_route_text(framebuffer, "/");
    pi4_xhci_route_hex(framebuffer, g_pi4_fat32_file.repeat_ok);
}

/* Keep the Pi's PCIe/DMA discovery report readable without a UART adapter. */
static void pi_diag_fb_report(const struct zeal_handoff *h) {
    if (!g_fb || !h) {
        return;
    }
    /* Called below the shell banner on the first visible screen. */
    g_fb_cx = 0;
    if (h->revision < 5 ||
        h->size < offsetof(struct zeal_handoff, pi4_pcie) + sizeof(h->pi4_pcie)) {
        pi_diag_fb_puts("PI4 FDT report unavailable\n");
        return;
    }
    pi_diag_fb_puts("PI4 FDT status=");
    switch (h->pi4_pcie.fdt_status) {
    case PI4_FDT_STATUS_OK:
        pi_diag_fb_puts("ok");
        break;
    case PI4_FDT_STATUS_NO_RESPONSE:
        pi_diag_fb_puts("no response");
        break;
    case PI4_FDT_STATUS_NULL_POINTER:
        pi_diag_fb_puts("null pointer");
        break;
    case PI4_FDT_STATUS_BAD_MAGIC:
        pi_diag_fb_puts("bad magic");
        break;
    case PI4_FDT_STATUS_BAD_SIZE:
        pi_diag_fb_puts("bad totalsize");
        break;
    case PI4_FDT_STATUS_NODE_NOT_FOUND:
        pi_diag_fb_puts("PCIe node missing");
        break;
    default:
        if (h->pi4_pcie.fdt_status >= PI4_FDT_STATUS_PARSE_BASE) {
            pi_diag_fb_puts("parse error=");
            pi_diag_fb_u64_hex(h->pi4_pcie.fdt_status - PI4_FDT_STATUS_PARSE_BASE);
        } else {
            pi_diag_fb_puts("unknown");
        }
        break;
    }
    pi_diag_fb_puts("\nFDT flags=");
    pi_diag_fb_u64_hex(h->pi4_pcie.flags);
    pi_diag_fb_puts("\nPCIe reg=");
    pi_diag_fb_u64_hex(h->pi4_pcie.reg_base);
    pi_diag_fb_puts("+");
    pi_diag_fb_u64_hex(h->pi4_pcie.reg_size);
    pi_diag_fb_puts("\nPCIe mem CPU=");
    pi_diag_fb_u64_hex(h->pi4_pcie.mem_cpu_base);
    pi_diag_fb_puts("\nPCIe mem bus=");
    pi_diag_fb_u64_hex(h->pi4_pcie.mem_bus_base);
    pi_diag_fb_puts(" size=");
    pi_diag_fb_u64_hex(h->pi4_pcie.mem_size);
    pi_diag_fb_puts("\nDMA CPU=");
    pi_diag_fb_u64_hex(h->pi4_pcie.dma_cpu_base);
    pi_diag_fb_puts("\nDMA bus=");
    pi_diag_fb_u64_hex(h->pi4_pcie.dma_bus_base);
    pi_diag_fb_puts(" size=");
    pi_diag_fb_u64_hex(h->pi4_pcie.dma_size);
    if (h->revision >= 6 &&
        h->size >= offsetof(struct zeal_handoff, pi4_usb_dma) +
                   sizeof(h->pi4_usb_dma)) {
        pi_diag_fb_puts("\nDMA arena flags=");
        pi_diag_fb_u64_hex(h->pi4_usb_dma.flags);
        pi_diag_fb_puts("\nDMA arena CPU=");
        pi_diag_fb_u64_hex(h->pi4_usb_dma.phys_base);
        pi_diag_fb_puts("\nDMA arena bus=");
        pi_diag_fb_u64_hex(h->pi4_usb_dma.bus_base);
        pi_diag_fb_puts("\nDMA arena size=");
        pi_diag_fb_u64_hex(h->pi4_usb_dma.size);
    } else {
        pi_diag_fb_puts("\nDMA arena: no rev-6 reservation");
    }
    pi_diag_fb_puts("\nDMA CPU/cache=");
    pi_diag_fb_puts(pi4_dma_cpu_result());
    if (g_pi4_dma_probe_line) {
        pi_diag_fb_puts(" line=");
        pi_diag_fb_u64_hex(g_pi4_dma_probe_line);
    }
    pi_diag_fb_puts(g_pi4_usb_live.active ? " (xHCI live; DMA arena retained)" :
                                          " (CPU-only; xHCI stopped)");
    pi4_xhci_route_report(1);
    pi_diag_fb_puts("\nxHCI DMA no-op=");
    pi_diag_fb_puts(pi4_xhci_dma_result());
    pi_diag_fb_puts(" events=");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_events);
    pi_diag_fb_puts(" event-cursor index/cycle=");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_event_cursor_index);
    fb_putc('/');
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_event_cursor_cycle);
    pi_diag_fb_puts(" commands=");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_command_completions);
    pi_diag_fb_puts(" slot-id=");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_slot_id);
    pi_diag_fb_puts(" active=");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_slot_enabled);
    if (g_pi4_xhci_dma_stage >= 17) {
        pi_diag_fb_puts("\nxHCI hub probe phase/port=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_probe_phase);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_probe_port);
        pi_diag_fb_puts(" request type/request=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_request_type);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_request);
        pi_diag_fb_puts(" value/index/length=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_request_value);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_request_index);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_request_length);
        pi_diag_fb_puts(" expected Status TRB=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_hub_expected_status_bus);
        pi_diag_fb_puts(" power/status masks=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_port_power_events);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_port_status_events);
    }
    pi_diag_fb_puts("\nPCIe BAR2 lo/origHi/testHi/restoredHi/state=");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_bar2_lo);
    pi_diag_fb_puts("/");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_bar2_hi_original);
    pi_diag_fb_puts("/");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_bar2_hi_test);
    pi_diag_fb_puts("/");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_bar2_hi_restored);
    pi_diag_fb_puts("/");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_bar2_state);
    pi_diag_fb_puts("\nDMA bus alias bias=");
    pi_diag_fb_u64_hex(g_pi4_xhci_dma_bus_bias);
    pi_diag_fb_puts("\nxHCI DMA preflight stage/rc/page=");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_stage);
    pi_diag_fb_puts("/");
    pi_diag_fb_u32_hex((uint32_t)g_pi4_xhci_dma_layout_rc);
    pi_diag_fb_puts("/");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_page_size);
    pi_diag_fb_puts(" gates=");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_gate_failures);
    pi_diag_fb_puts("\nxHCI DMA cmd/sts/legacy/pci=");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_usbcmd);
    pi_diag_fb_puts("/");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_usbsts);
    pi_diag_fb_puts("/");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_legacy);
    pi_diag_fb_puts("/");
    pi_diag_fb_u32_hex(g_pi4_xhci_dma_pci_command);
    if (g_pi4_xhci_dma_stage >= 6) {
        pi_diag_fb_puts("\nxHCI setup mask/IMAN/IMOD=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_setup_mask);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_setup_iman);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_setup_imod);
        pi_diag_fb_puts("\nxHCI setup mismatch=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_setup_mismatch);
        pi_diag_fb_puts("\nxHCI setup CONFIG/ERSTSZ=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_setup_config);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_setup_erstsz);
        pi_diag_fb_puts("\nxHCI setup DCBAAP/ERSTBA/ERDP=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_setup_dcbaa);
        pi_diag_fb_puts("/");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_setup_erstba);
        pi_diag_fb_puts("/");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_setup_erdp);
    }
    if (g_pi4_xhci_dma_abort_state) {
        pi_diag_fb_puts("\nxHCI abort state/status/control=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_abort_state);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_abort_status);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_abort_control);
        pi_diag_fb_puts("\nxHCI abort parameter=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_abort_parameter);
    }
    if (g_pi4_xhci_dma_stage >= 7) {
        pi_diag_fb_puts("\nxHCI DMA live cmd/sts/CRCRlo/hi=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_runtime_cmd);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_runtime_sts);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_crcr_lo);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_crcr_hi);
        pi_diag_fb_puts("\nxHCI DMA ERDPlo/hi/event TRB=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_erdp_lo);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_erdp_hi);
        pi_diag_fb_puts("/");
        for (unsigned i = 0; i < 4; i++) {
            if (i) pi_diag_fb_puts("/");
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_event_trb[i]);
        }
    }
    if (g_pi4_xhci_dma_stage >= 8) {
        pi_diag_fb_puts("\nxHCI RUN/doorbell USBSTS=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_run_sts);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_doorbell_sts);
        pi_diag_fb_puts("\nxHCI event-ring wrap=");
        pi_diag_fb_puts(pi4_xhci_dma_wrap_result());
        pi_diag_fb_puts(" MFINDEX=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_mfindex_start);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_mfindex_end);
        pi_diag_fb_puts(" status/control=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_wrap_status);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_wrap_control);
        pi_diag_fb_puts("\nxHCI port reset event=");
        pi_diag_fb_puts(pi4_xhci_dma_port_event_result());
        pi_diag_fb_puts(" port=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_port_event_port);
        pi_diag_fb_puts(" PORTSC=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_port_event_before);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_port_event_after);
        pi_diag_fb_puts(" speed-id=");
        pi_diag_fb_u32_hex(pi4_xhci_dma_port_speed_id(
            g_pi4_xhci_dma_port_event_after));
        pi_diag_fb_puts(" ready=");
        pi_diag_fb_puts(g_pi4_xhci_dma_port_ready ? "yes" : "no");
        pi_diag_fb_puts("\nxHCI protocol-cap/major/slot-type=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_protocol_cap_offset);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_protocol_major);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_protocol_slot_type);
        pi_diag_fb_puts("\nxHCI EnableSlot TRB control=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_enable_trb_control);
        pi_diag_fb_puts("\nxHCI Enable Slot status/control=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_enable_slot_status);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_enable_slot_control);
        pi_diag_fb_puts(" Address Device status/control=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_address_status);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_address_control);
        pi_diag_fb_puts(" USB-address/slot-state=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_device_address);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_device_slot_state);
        pi_diag_fb_puts(" event parameter/status/control=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_port_event_parameter);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_port_event_status);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_port_event_control);
    }
    if (g_pi4_xhci_dma_stage >= 10) {
        pi_diag_fb_puts("\nxHCI doorbell flush readback=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_db_flush);
    }
    if (g_pi4_xhci_dma_stage >= 12) {
        pi_diag_fb_puts("\nxHCI Address DMA input/output/EP0=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_address_input_bus);
        pi_diag_fb_puts("/");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_address_output_bus);
        pi_diag_fb_puts("/");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_address_ep0_bus);
        pi_diag_fb_puts(" DCBAA[slot]=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_address_dcbaa_slot);
        pi_diag_fb_puts("\nxHCI Address ctx add/slot0/slot1/EP0d1-d4=");
        for (unsigned i = 0; i < 7; i++) {
            if (i) pi_diag_fb_puts("/");
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_address_context_words[i]);
        }
        pi_diag_fb_puts(" TRB control=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_address_trb_control);
    }
    if (g_pi4_xhci_dma_stage >= 14) {
        pi_diag_fb_puts("\nxHCI EP0 setup/data/status controls=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_setup_trb_control);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_data_trb_control);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_status_trb_control);
        pi_diag_fb_puts(" buffer=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_control_buffer_bus);
        pi_diag_fb_puts("\nxHCI Transfer Event parameter/status/control=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_transfer_parameter);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_transfer_status);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_transfer_control);
        pi_diag_fb_puts(" checks/skipped=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_transfer_checks);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_transfer_skipped);
        pi_diag_fb_puts(" first skipped param/status/control=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_transfer_skipped_parameter);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_transfer_skipped_status);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_transfer_skipped_control);
        pi_diag_fb_puts(" descriptor first8=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_descriptor_prefix[0]);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_descriptor_prefix[1]);
        pi_diag_fb_puts(" device bcd/class/sub/proto/mps=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_device_bcd_usb);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_device_class);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_device_subclass);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_device_protocol);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_device_ep0_mps);
        pi_diag_fb_puts(" vid/pid/rev/configs=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_device_vendor);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_device_product);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_device_bcd_device);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_device_configurations);
    }
    if (g_pi4_xhci_dma_stage >= 15) {
        pi_diag_fb_puts("\nxHCI config buffer/header=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_config_buffer_bus);
        fb_putc('/');
        for (unsigned i = 0; i < 3; i++) {
            if (i) fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_config_header_words[i]);
        }
        pi_diag_fb_puts(" total/interfaces/value/attr/power=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_config_total_length);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_config_interfaces);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_config_value);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_config_attributes);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_config_max_power);
    }
    if (g_pi4_xhci_dma_stage >= 16) {
        pi_diag_fb_puts("\nxHCI SET_CONFIGURATION setup/status controls=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_set_config_setup_control);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_set_config_status_control);
        pi_diag_fb_puts(" event parameter/status/control=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_set_config_event_parameter);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_set_config_event_status);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_set_config_event_control);
        pi_diag_fb_puts(" checks/skipped=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_set_config_event_checks);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_set_config_event_skipped);
    }
    if (g_pi4_xhci_dma_stage >= 17) {
        pi_diag_fb_puts("\nxHCI Hub GET_DESCRIPTOR rc/setup=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_descriptor_transfer_rc);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_descriptor_setup[0]);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_descriptor_setup[1]);
        pi_diag_fb_puts(" data bus/len/ctrl/status/valid=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_hub_descriptor_data_bus);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_descriptor_data_length);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_descriptor_data_control);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_descriptor_status_control);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_descriptor_validated);
        pi_diag_fb_puts("\nxHCI Hub descriptor buffer/words=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_hub_buffer_bus);
        fb_putc('/');
        for (unsigned i = 0; i < 4; i++) {
            if (i) fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_descriptor_words[i]);
        }
        pi_diag_fb_puts(" ports/chars/pwr-delay/current/removable/power-mask=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_ports);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_characteristics);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_power_delay);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_current);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_removable);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_power_mask);
    }
    if (g_pi4_xhci_dma_stage >= 18) {
        uint32_t powered_mask = pi4_xhci_dma_hub_port_powered_mask(
            g_pi4_xhci_dma_hub_port_status,
            g_pi4_xhci_dma_hub_port_status_events);
        pi_diag_fb_puts("\nxHCI hub power/status masks=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_port_power_events);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_port_status_events);
        pi_diag_fb_puts(" powered/unpowered/connected=");
        pi_diag_fb_u32_hex(powered_mask);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_port_unpowered_mask);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_port_connected_mask);
        pi_diag_fb_puts("\nxHCI port status/change=");
        for (unsigned i = 0; i < 4u; i++) {
            if (i) fb_putc('/');
            fb_putc('1' + i);
            fb_putc('=');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_port_status[i]);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_port_change[i]);
        }
    }
    if (g_pi4_xhci_dma_stage >= 20) {
        pi_diag_fb_puts("\nxHCI child reset=");
        pi_diag_fb_puts(pi4_xhci_dma_hub_child_reset_result());
        pi_diag_fb_puts(" port/samples/request/clear=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_child_reset_port);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_child_reset_samples);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_child_reset_request_rc);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_child_clear_rc);
        pi_diag_fb_puts("\nreset status/change=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_child_reset_status);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_child_reset_change);
        pi_diag_fb_puts(" final status/change=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_child_final_status);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_child_final_change);
    }
    if (g_pi4_xhci_dma_stage >= 21) {
        pi_diag_fb_puts("\nxHCI hub Configure Endpoint status/control=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_config_status);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_config_control);
        pi_diag_fb_puts(" guard No-op status/control=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_guard_status);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_guard_control);
        pi_diag_fb_puts(" cfg input[0..7]=");
        for (unsigned i = 0; i < 8u; i++) {
            if (i) fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_hub_config_input_words[i]);
        }
        pi_diag_fb_puts(" cfg TRB[0..3]=");
        for (unsigned i = 0; i < 4u; i++) {
            if (i) fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_config_trb_words[i]);
        }
        pi_diag_fb_puts(" CRCR/STS/DB=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_config_crcr[0]);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_config_crcr[1]);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_config_usbsts);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_config_db);
        pi_diag_fb_puts(" hub output slot=");
        for (unsigned i = 0; i < 3u; i++) {
            if (i) fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_output_slot_words[i]);
        }
        pi_diag_fb_puts("\nxHCI child speed/route/slot=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_speed_id);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_route_string);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_slot_id);
        pi_diag_fb_puts(" EnableSlot status/control=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_enable_status);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_enable_control);
        pi_diag_fb_puts(" AddressDevice status/control=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_address_status);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_address_control);
        pi_diag_fb_puts(" USB-address/slot-state=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_address);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_slot_state);
        pi_diag_fb_puts(" DMA output/EP0=");
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_child_output_bus);
        fb_putc('/');
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_child_ep0_bus);
        pi_diag_fb_puts(" ctx add/slot0-2/EP0d1-2=");
        for (unsigned i = 0; i < 6u; i++) {
            if (i) fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_address_context_words[i]);
        }
        if (g_pi4_xhci_dma_stage >= 24) {
            pi_diag_fb_puts("\nxHCI child Device Descriptor prefix rc/valid/buffer/status=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_descriptor_transfer_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_descriptor_validated);
            fb_putc('/');
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_descriptor_buffer_bus);
            fb_putc('/');
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_descriptor_status_bus);
            pi_diag_fb_puts(" setup/data/status ctrl=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_descriptor_setup_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_descriptor_data_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_descriptor_status_control);
            pi_diag_fb_puts(" bytes=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_descriptor_words[0]);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_descriptor_words[1]);
        }
        if (g_pi4_xhci_dma_stage >= 25) {
            pi_diag_fb_puts("\nxHCI child repeated Device Descriptor rc/valid/observed/event/checks/words=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_transfer_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_validated);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_observed);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_event_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_event_checks);
            pi_diag_fb_puts("/");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_words[0]);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_words[1]);
            pi_diag_fb_puts(" event-param/control=");
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_event_parameter);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_event_control);
            pi_diag_fb_puts(" buffer/status=");
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_buffer_bus);
            fb_putc('/');
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_status_bus);
            pi_diag_fb_puts(" setup/data/status ctrl=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_setup_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_data_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_repeat_descriptor_status_control);
        }
        if (g_pi4_xhci_dma_stage >= 26) {
            pi_diag_fb_puts("\nxHCI child pre-Evaluate GET_CONFIGURATION rc/event/checks=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_pre_evaluate_get_configuration_transfer_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_checks);
            pi_diag_fb_puts(" observed/word/buffer/status=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_pre_evaluate_get_configuration_observed);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_pre_evaluate_get_configuration_word);
            fb_putc('/');
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_pre_evaluate_get_configuration_buffer_bus);
            fb_putc('/');
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_pre_evaluate_get_configuration_status_bus);
            pi_diag_fb_puts(" event-param/control=");
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_parameter);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_control);
            pi_diag_fb_puts(" setup/data/status ctrl=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_pre_evaluate_get_configuration_setup_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_pre_evaluate_get_configuration_data_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_pre_evaluate_get_configuration_status_control);
        }
        if (g_pi4_xhci_dma_stage >= 27) {
            pi_diag_fb_puts("\nxHCI child EP0 Evaluate status/control requested/output MPS=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_ep0_evaluate_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_ep0_evaluate_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_ep0_evaluate_mps);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_ep0_evaluate_output_mps);
            pi_diag_fb_puts(" command-type=");
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_ep0_mps_command);
        }
        if (g_pi4_xhci_dma_stage >= 28) {
            pi_diag_fb_puts("\nxHCI child EP0 GET_STATUS rc/event/checks=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_get_status_transfer_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_get_status_event_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_get_status_event_checks);
            pi_diag_fb_puts(" observed/word/buffer/status=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_get_status_observed);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_status_word);
            fb_putc('/');
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_get_status_buffer_bus);
            fb_putc('/');
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_get_status_status_bus);
            pi_diag_fb_puts(" event-param/control=");
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_get_status_event_parameter);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_get_status_event_control);
            pi_diag_fb_puts(" setup/data/status ctrl=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_get_status_setup_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_get_status_data_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_get_status_status_control);
        }
        if (g_pi4_xhci_dma_stage >= 29) {
            pi_diag_fb_puts("\nxHCI child full Device Descriptor rc/valid/observed/buffer/status=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_full_descriptor_transfer_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_full_descriptor_validated);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_full_descriptor_observed);
            fb_putc('/');
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_full_descriptor_buffer_bus);
            fb_putc('/');
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_full_descriptor_status_bus);
            pi_diag_fb_puts(" setup/data/status ctrl=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_full_descriptor_setup_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_full_descriptor_data_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_full_descriptor_status_control);
            pi_diag_fb_puts(" descriptor words=");
            for (unsigned i = 0; i < 5u; i++) {
                if (i) fb_putc('/');
                pi_diag_fb_u32_hex(
                    g_pi4_xhci_dma_child_full_descriptor_words[i]);
            }
        }
        if (g_pi4_xhci_dma_stage >= 30) {
            pi_diag_fb_puts("\nxHCI child Configuration Header rc/valid/observed/event/checks=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_header_transfer_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_header_validated);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_header_observed);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_header_event_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_header_event_checks);
            pi_diag_fb_puts(" words=");
            for (unsigned i = 0; i < 3u; i++) {
                if (i) fb_putc('/');
                pi_diag_fb_u32_hex(
                    g_pi4_xhci_dma_child_config_header_words[i]);
            }
            pi_diag_fb_puts(" event-param/control=");
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_config_header_event_parameter);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_header_event_control);
            pi_diag_fb_puts(" buffer/status=");
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_config_header_buffer_bus);
            fb_putc('/');
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_config_header_status_bus);
            pi_diag_fb_puts(" setup/data/status ctrl=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_header_setup_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_header_data_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_header_status_control);
        }
        if (g_pi4_xhci_dma_stage >= 31) {
            const struct pi4_xhci_dma_config_summary *summary =
                &g_pi4_xhci_dma_child_config_summary;
            pi_diag_fb_puts("\nxHCI child Configuration Descriptors rc/valid/observed/event/checks=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_full_transfer_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_full_validated);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_full_observed);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_full_event_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_full_event_checks);
            pi_diag_fb_puts(" words=");
            for (unsigned i = 0; i < 3u; i++) {
                if (i) fb_putc('/');
                pi_diag_fb_u32_hex(
                    g_pi4_xhci_dma_child_config_full_words[i]);
            }
            pi_diag_fb_puts(" event-param/control=");
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_config_full_event_parameter);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_full_event_control);
            pi_diag_fb_puts(" buffer/status=");
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_config_full_buffer_bus);
            fb_putc('/');
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_config_full_status_bus);
            pi_diag_fb_puts(" setup/data/status ctrl=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_full_setup_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_full_data_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_config_full_status_control);
            pi_diag_fb_puts(" total/value/if-desc/unique/ep-desc=");
            pi_diag_fb_u32_hex(summary->total_length);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->configuration_value);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->interface_descriptors);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->unique_interfaces);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->endpoint_descriptors);
            pi_diag_fb_puts(" first-if num/class/sub/proto/eps=");
            pi_diag_fb_u32_hex(summary->first_interface_number);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->first_interface_class);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->first_interface_subclass);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->first_interface_protocol);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->first_interface_endpoint_count);
            pi_diag_fb_puts(" first-ep addr/attr/maxpacket/interval=");
            pi_diag_fb_u32_hex(summary->first_endpoint_address);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->first_endpoint_attributes);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->first_endpoint_max_packet);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->first_endpoint_interval);
            pi_diag_fb_puts(" BOT found/if/alt/eps IN addr/attr/mps OUT addr/attr/mps=");
            pi_diag_fb_u32_hex(summary->bot_interface_found);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->bot_interface_number);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->bot_interface_alt);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->bot_interface_endpoint_count);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->bot_bulk_in_address);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->bot_bulk_in_attributes);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->bot_bulk_in_max_packet);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->bot_bulk_out_address);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->bot_bulk_out_attributes);
            fb_putc('/');
            pi_diag_fb_u32_hex(summary->bot_bulk_out_max_packet);
        }
        if (g_pi4_xhci_dma_stage >= 32) {
            pi_diag_fb_puts("\nxHCI child SET_CONFIGURATION rc/event/checks=");
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_set_config_transfer_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_set_config_event_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_set_config_event_checks);
            pi_diag_fb_puts(" setup/status ctrl=");
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_set_config_setup_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_set_config_status_control);
            pi_diag_fb_puts(" event-param/control/status-bus=");
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_set_config_event_parameter);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_set_config_event_control);
            fb_putc('/');
            pi_diag_fb_u64_hex(g_pi4_xhci_dma_child_set_config_status_bus);
        }
        if (g_pi4_xhci_dma_stage >= 33) {
            pi_diag_fb_puts("\nxHCI child GET_CONFIGURATION rc/observed/value/event/checks=");
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_config_transfer_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_config_observed);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_config_value);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_config_event_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_config_event_checks);
            pi_diag_fb_puts(" setup/data/status ctrl=");
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_config_setup_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_config_data_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_config_status_control);
            pi_diag_fb_puts(" event-param/control/buffer/status=");
            pi_diag_fb_u64_hex(g_pi4_xhci_dma_child_get_config_event_parameter);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_config_event_control);
            fb_putc('/');
            pi_diag_fb_u64_hex(g_pi4_xhci_dma_child_get_config_buffer_bus);
            fb_putc('/');
            pi_diag_fb_u64_hex(g_pi4_xhci_dma_child_get_config_status_bus);
        }
        if (g_pi4_xhci_dma_stage >= 34) {
            pi_diag_fb_puts("\nxHCI child BOT GET_MAX_LUN rc/observed/value/event/checks=");
            pi_diag_fb_u32_hex(
                (uint32_t)g_pi4_xhci_dma_child_get_max_lun_transfer_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_max_lun_observed);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_max_lun_value);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_max_lun_event_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_max_lun_event_checks);
            pi_diag_fb_puts(" prepare=");
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_max_lun_prepare);
            pi_diag_fb_puts(" event-param/control=");
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_child_get_max_lun_event_parameter);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_child_get_max_lun_event_control);
            pi_diag_fb_puts(" setup/data/status ctrl=");
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_max_lun_setup_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_max_lun_data_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_get_max_lun_status_control);
            pi_diag_fb_puts(" buffer/status=");
            pi_diag_fb_u64_hex(g_pi4_xhci_dma_child_get_max_lun_buffer_bus);
            fb_putc('/');
            pi_diag_fb_u64_hex(g_pi4_xhci_dma_child_get_max_lun_status_bus);
            pi_diag_fb_puts("\nxHCI child bulk guard rc/status/control=");
            pi_diag_fb_u32_hex((uint32_t)g_pi4_xhci_dma_child_bulk_guard_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_bulk_guard_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_bulk_guard_control);
            pi_diag_fb_puts(" Configure Endpoint rc/status/control/verified=");
            pi_diag_fb_u32_hex((uint32_t)g_pi4_xhci_dma_child_bulk_config_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_bulk_config_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_bulk_config_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_bulk_contexts_verified);
            pi_diag_fb_puts(" rings IN/OUT=");
            pi_diag_fb_u64_hex(g_pi4_xhci_dma_child_bulk_in_ring_bus);
            fb_putc('/');
            pi_diag_fb_u64_hex(g_pi4_xhci_dma_child_bulk_out_ring_bus);
            pi_diag_fb_puts(" slot context=");
            for (unsigned i = 0; i < 6u; i++) {
                if (i) fb_putc('/');
                pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_bulk_slot_words[i]);
            }
            pi_diag_fb_puts(" bulk IN context=");
            for (unsigned i = 0; i < 5u; i++) {
                if (i) fb_putc('/');
                pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_bulk_in_words[i]);
            }
            pi_diag_fb_puts(" bulk OUT context=");
            for (unsigned i = 0; i < 5u; i++) {
                if (i) fb_putc('/');
                pi_diag_fb_u32_hex(g_pi4_xhci_dma_child_bulk_out_words[i]);
            }
        }
    }
    if (g_pi4_xhci_dma_stage >= 19) {
        pi_diag_fb_puts("\nxHCI current hub-status TD index/buffer=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_status_trb_index);
        fb_putc('/');
        pi_diag_fb_u64_hex(g_pi4_xhci_dma_hub_status_buffer_bus);
        pi_diag_fb_puts(" setup/data/status ctrl=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_setup_trb_control);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_data_trb_control);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_status_trb_control);
        pi_diag_fb_puts(" buffer words=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_status_buffer_words[0]);
        fb_putc('/');
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_hub_status_buffer_words[1]);
        if (g_pi4_xhci_dma_stop_endpoint_rc != UINT32_MAX) {
            pi_diag_fb_puts("\nxHCI Stop EP rc/TRB/status/control=");
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_stop_endpoint_rc);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_stop_endpoint_trb_control);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_stop_endpoint_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_stop_endpoint_control);
            pi_diag_fb_puts(" stopped/other events=");
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_stop_endpoint_transfer_events);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_stop_endpoint_other_events);
            pi_diag_fb_puts(" event param/status/control=");
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_stop_endpoint_transfer_parameter);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_stop_endpoint_transfer_status);
            fb_putc('/');
            pi_diag_fb_u32_hex(
                g_pi4_xhci_dma_stop_endpoint_transfer_control);
        }
        if (g_pi4_xhci_dma_stop_output_ep0_context_valid) {
            pi_diag_fb_puts("\nEP0 after Stop state/dequeue/DCS=");
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_stop_output_ep0_state);
            fb_putc('/');
            pi_diag_fb_u64_hex(
                g_pi4_xhci_dma_stop_output_ep0_dequeue_bus);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_stop_output_ep0_dcs);
            pi_diag_fb_puts(" context DW0..4=");
            for (unsigned i = 0; i < 5u; i++) {
                if (i) fb_putc('/');
                pi_diag_fb_u32_hex(
                    g_pi4_xhci_dma_stop_output_ep0_context[i]);
            }
        }
        if (g_pi4_xhci_dma_output_ep0_context_valid) {
            pi_diag_fb_puts("\nEP0 after global halt state/dequeue/DCS=");
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_output_ep0_state);
            fb_putc('/');
            pi_diag_fb_u64_hex(g_pi4_xhci_dma_output_ep0_dequeue_bus);
            fb_putc('/');
            pi_diag_fb_u32_hex(g_pi4_xhci_dma_output_ep0_dcs);
            pi_diag_fb_puts(" hub TD setup/data/status bus=");
            pi_diag_fb_u64_hex(g_pi4_xhci_dma_hub_setup_trb_bus);
            fb_putc('/');
            pi_diag_fb_u64_hex(g_pi4_xhci_dma_hub_data_trb_bus);
            fb_putc('/');
            pi_diag_fb_u64_hex(g_pi4_xhci_dma_hub_expected_status_bus);
            pi_diag_fb_puts("\nEP0 output context DW0..4=");
            for (unsigned i = 0; i < 5u; i++) {
                if (i) fb_putc('/');
                pi_diag_fb_u32_hex(g_pi4_xhci_dma_output_ep0_context[i]);
            }
        }
    }
    if (g_pi4_xhci_dma_stage >= 11) {
        pi_diag_fb_puts("\nxHCI timeout USBSTS/CRCR/IMAN=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_timeout_sts);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_timeout_crcr);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_timeout_iman);
    }
    if (g_pi4_xhci_dma_state == PI4_XHCI_DMA_BAD_EVENT) {
        pi_diag_fb_puts(" event status/control=");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_event_status);
        pi_diag_fb_puts("/");
        pi_diag_fb_u32_hex(g_pi4_xhci_dma_event_control);
    }
    pi_diag_fb_puts("\nPCIe link status=");
    pi_diag_fb_u64_hex(g_pi4_link_status);
    if (g_pi4_cfg_state == PI4_CFG_READ_OK) {
        pi_diag_fb_puts("\nPCI 01:00.0 ID=");
        pi_diag_fb_u64_hex(g_pi4_cfg_id);
        pi_diag_fb_puts(" class/rev=");
        pi_diag_fb_u64_hex(g_pi4_cfg_class_rev);
        pi_diag_fb_puts(" hdr=");
        pi_diag_fb_u64_hex(g_pi4_cfg_header);
        pi_diag_fb_puts("\nPCI cmd/status=");
        pi_diag_fb_u64_hex(g_pi4_cfg_command);
        for (unsigned i = 0; i < g_pi4_cfg_bar_count; i++) {
            pi_diag_fb_puts("\nBAR");
            fb_putc('0' + i);
            pi_diag_fb_puts("=");
            pi_diag_fb_u64_hex(g_pi4_cfg_bars[i]);
        }
        if (g_pi4_xhci_state == PI4_XHCI_CAPS_OK ||
            g_pi4_xhci_state == PI4_XHCI_CAPS_INVALID) {
            pi_diag_fb_puts(g_pi4_xhci_state == PI4_XHCI_CAPS_INVALID
                ? "\nxHCI caps invalid" : "\nxHCI CAP/version");
            if (g_pi4_xhci_state == PI4_XHCI_CAPS_INVALID)
                pi_diag_fb_puts(" CAP/version=");
            else
                pi_diag_fb_puts("=");
            pi_diag_fb_u64_hex(g_pi4_xhci_caps[0]);
            pi_diag_fb_puts(" HCS1=");
            pi_diag_fb_u64_hex(g_pi4_xhci_caps[1]);
            pi_diag_fb_puts(" HCS2=");
            pi_diag_fb_u64_hex(g_pi4_xhci_caps[2]);
            pi_diag_fb_puts(" HCC1=");
            pi_diag_fb_u64_hex(g_pi4_xhci_caps[3]);
            pi_diag_fb_puts("\nxHCI DB=");
            pi_diag_fb_u64_hex(g_pi4_xhci_caps[4]);
            pi_diag_fb_puts(" RTS=");
            pi_diag_fb_u64_hex(g_pi4_xhci_caps[5]);
            if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_OP_REGS) {
                pi_diag_fb_puts("\nxHCI USBCMD/USBSTS=");
                pi_diag_fb_u64_hex(g_pi4_xhci_usbcmd);
                pi_diag_fb_puts("/");
                pi_diag_fb_u64_hex(g_pi4_xhci_usbsts);
            }
            if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_LEGACY) {
                pi_diag_fb_puts("\nxHCI USBLEGSUP offset/initial/final=");
                pi_diag_fb_u64_hex(g_pi4_xhci_legacy_offset);
                pi_diag_fb_puts("/");
                pi_diag_fb_u64_hex(g_pi4_xhci_legacy_cap);
                pi_diag_fb_puts("/");
                pi_diag_fb_u64_hex(g_pi4_xhci_legacy_after);
                pi_diag_fb_puts(" handoff=");
                pi_diag_fb_puts(pi4_xhci_handoff_result());
                pi_diag_fb_puts(" HCRST=");
                pi_diag_fb_puts(pi4_xhci_reset_result());
                pi_diag_fb_puts(" post cmd/sts=");
                pi_diag_fb_u32_hex(g_pi4_xhci_reset_usbcmd);
                pi_diag_fb_puts("/");
                pi_diag_fb_u32_hex(g_pi4_xhci_reset_usbsts);
            } else if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_NO_LEGACY) {
                pi_diag_fb_puts("\nxHCI USB legacy capability: absent");
            }
            if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_PORTS) {
                pi_diag_fb_puts("\nxHCI PORTSC count=");
                pi_diag_fb_u32_hex(g_pi4_xhci_port_count);
                pi_diag_fb_puts("/");
                pi_diag_fb_u32_hex(g_pi4_xhci_port_total);
                for (unsigned i = 0; i < g_pi4_xhci_port_count; i++) {
                    pi_diag_fb_puts(" ");
                    fb_putc('1' + i);
                    pi_diag_fb_puts("=");
                    pi_diag_fb_u32_hex(g_pi4_xhci_portsc[i]);
                }
            }
            if (g_pi4_xhci_state_mask & PI4_XHCI_STATE_READ_FAIL) {
                pi_diag_fb_puts("\nxHCI state read failed ESR/FAR=");
                pi_diag_fb_u64_hex(g_pi4_xhci_state_esr);
                pi_diag_fb_puts("/");
                pi_diag_fb_u64_hex(g_pi4_xhci_state_far);
            }
            pi_diag_fb_puts("\nxHCI BAR CPU=");
            pi_diag_fb_u64_hex(g_pi4_xhci_bar_cpu);
            pi_diag_fb_puts(" VA=");
            pi_diag_fb_u64_hex(g_pi4_xhci_bar_va);
            pi_diag_fb_puts("\nPCIe win0 LO/HI/BASELIMIT=");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.win0_lo);
            pi_diag_fb_puts("/");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.win0_hi);
            pi_diag_fb_puts("/");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.win0_base_limit);
            pi_diag_fb_puts(" BASEHI/LIMHI=");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.win0_base_hi);
            pi_diag_fb_puts("/");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.win0_limit_hi);
            pi_diag_fb_puts("\nPCIe inbound BAR1 lo/hi=");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.inbound_bar1_lo);
            pi_diag_fb_puts("/");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.inbound_bar1_hi);
            pi_diag_fb_puts(" BAR2 lo/hi=");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.inbound_bar2_lo);
            pi_diag_fb_puts("/");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.inbound_bar2_hi);
            pi_diag_fb_puts("\nPCIe inbound BAR3 lo/hi=");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.inbound_bar3_lo);
            pi_diag_fb_puts("/");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.inbound_bar3_hi);
            pi_diag_fb_puts(" MISC_CTRL=");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.misc_ctrl);
            pi_diag_fb_puts("\nPCIe err valid before/after=");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.err_valid_before);
            pi_diag_fb_puts("/");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.err_valid_after);
            pi_diag_fb_puts(" info/addr=");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.err_info);
            pi_diag_fb_puts("/");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.err_addr);
            pi_diag_fb_puts("\nPCIe mem err addr hi:lo=");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.mem_err_addr_hi);
            pi_diag_fb_puts(":");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.mem_err_addr_lo);
            pi_diag_fb_puts(" cause=");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.mem_err_cause);
            pi_diag_fb_puts(" readmask=");
            pi_diag_fb_u32_hex(g_pi4_pcie_readback.mask);
        } else {
            pi_diag_fb_puts("\nxHCI caps state=");
            pi_diag_fb_u64_hex(g_pi4_xhci_state);
            if (g_pi4_xhci_state == PI4_XHCI_MMIO_FAIL ||
                g_pi4_xhci_state == PI4_XHCI_RESTORE_FAIL) {
                pi_diag_fb_puts(" ESR=");
                pi_diag_fb_u64_hex(g_probe_esr);
                pi_diag_fb_puts(" FAR=");
                pi_diag_fb_u64_hex(g_probe_far);
            }
        }
    } else if (g_pi4_cfg_state == PI4_CFG_NO_DEVICE) {
        pi_diag_fb_puts("\nPCI 01:00.0 absent ID=");
        pi_diag_fb_u64_hex(g_pi4_cfg_id);
    } else if (g_pi4_cfg_state == PI4_CFG_MMIO_FAIL) {
        pi_diag_fb_puts("\nPCI config read trapped ESR=");
        pi_diag_fb_u64_hex(g_probe_esr);
        pi_diag_fb_puts(" FAR=");
        pi_diag_fb_u64_hex(g_probe_far);
    } else if (g_pi4_cfg_state == PI4_CFG_MAP_FAIL) {
        pi_diag_fb_puts("\nPCIe probe skipped: MMIO map failed");
    } else if (g_pi4_cfg_state == PI4_CFG_NOT_LINKED) {
        pi_diag_fb_puts("\nPCI config skipped: link inactive");
    } else if (g_pi4_cfg_state == PI4_CFG_FDT_SKIP) {
        pi_diag_fb_puts("\nPCIe probe skipped: no enabled FDT reg");
    } else if (g_pi4_cfg_state == PI4_CFG_STATUS_FAIL) {
        pi_diag_fb_puts("\nPCIe status trapped ESR=");
        pi_diag_fb_u64_hex(g_probe_esr);
        pi_diag_fb_puts(" FAR=");
        pi_diag_fb_u64_hex(g_probe_far);
    } else {
        pi_diag_fb_puts("\nPCI config read not attempted");
    }
    pi_diag_fb_puts("\nPolicy: USB media read-only; live DMA arena reserved; polled native input\n");
    pi4_usb_bus_report(1);
    if (g_pi4_bus.storage_fault)
        pi_diag_fb_puts("\nInput-only test: type help/usbstatus; move/drag pointer. Photograph USB bulk output.\n");
    else if (g_pi4_bus.storage_pending)
        pi_diag_fb_puts("\nUSB storage checks start automatically after HID startup. Test help, move/drag and usbstatus.\n");
    else if (g_pi4_bus.attempted)
        pi_diag_fb_puts("\nCombined test: type help; move/drag pointer; keep all connected 55s; type usbstatus.\n");
    if (g_pi4_keyboard.attempted) {
        pi4_usb_hid_report(1);
        pi_diag_fb_puts(g_pi4_keyboard.mouse ?
            "\nMouse test: move, click both buttons; hold left and drag to paint.\n" :
            "\nKeyboard test: type help then Enter; type usbstatus then Enter.\n");
    }
}
#endif

/* Shell line editing emits BS-space-BS for terminal serials. The framebuffer
 * is a direct-pixel console, so erase the preceding cell explicitly instead
 * of painting the control bytes as visible glyphs. */
static void con_backspace(void) {
    if (g_uart) {
        uart_write(g_uart, '\b');
        uart_write(g_uart, ' ');
        uart_write(g_uart, '\b');
    }
    if (!g_fb || fb_console_owned_by_desktop()) {
        return;
    }
    (void)zeal_fb_task_text_write(FB_SHELL_TEXT_SLOT, 0,
        g_fb_cols < ZEAL_TEXT_MAX_COLS ? g_fb_cols - 1 : ZEAL_TEXT_MAX_COLS - 1,
        0, g_fb_rows < ZEAL_TEXT_MAX_ROWS ? g_fb_rows - 1 : ZEAL_TEXT_MAX_ROWS - 1,
        0x0f, "\b");
    if (g_fb_cx) {
        g_fb_cx--;
    } else if (g_fb_cy) {
        g_fb_cy--;
        g_fb_cx = g_fb_cols ? g_fb_cols - 1 : 0;
    } else {
        return;
    }
    fb_fillrect(g_fb_cx * 8, g_fb_cy * 8, 8, 8, 0x00101820u);
}

#ifndef ZEAL_PI_DIAG
/* `zls` can list the full bundled source catalog, so keep its capture larger
 * than host_print's single-call formatter limit. GuiTerminal prints this in
 * bounded chunks rather than passing the whole result to one DocPrint. */
static char gui_shell_output[32768];
static size_t gui_shell_output_len;
static int gui_shell_capture;
static int gui_shell_truncated;
static volatile int gui_window_demo_requested;
static void shell_handle(const char *line, int *done);
#endif

static void con_write(char c) {
#ifndef ZEAL_PI_DIAG
    if (gui_shell_capture) {
        /* Leave room for an explicit truncation notice. */
        if (gui_shell_output_len + 32 < sizeof(gui_shell_output)) {
            gui_shell_output[gui_shell_output_len++] = c;
            gui_shell_output[gui_shell_output_len] = 0;
        } else {
            gui_shell_truncated = 1;
        }
        return;
    }
#endif
    if (g_uart) {
        if (c == '\n') {
            uart_write(g_uart, '\r');
        }
        uart_write(g_uart, c);
    }
    fb_putc(c);
}

static void con_puts(const char *s) {
    while (*s) {
        con_write(*s++);
    }
#if defined(ZEAL_FORCE_PI4) && !defined(ZEAL_PI_DIAG)
    if (!fb_console_owned_by_desktop()) pi4_desktop_scanout_present();
#endif
}

static void fb_init_ex(const struct zeal_handoff *h, int clear) {
    if (!h->fb_addr_phys || !h->fb_width || !h->fb_pitch || h->fb_bpp < 24) {
        g_fb = NULL;
        return;
    }
    g_fb = (volatile uint8_t *)(h->hhdm_offset + h->fb_addr_phys);
    g_fb_w = h->fb_width;
    g_fb_h = h->fb_height;
    g_fb_pitch = h->fb_pitch;
    g_fb_bpp = h->fb_bpp;
    g_fb_cols = (uint32_t)(g_fb_w / 8);
    g_fb_rows = (uint32_t)(g_fb_h / 8);
    fb_text_buffer_reset();
    if (clear) {
        fb_clear(0x00101820u);
    } else {
        /* Keep firmware/Limine splash; start text near bottom. */
        g_fb_cx = 0;
        g_fb_cy = g_fb_rows > 4 ? g_fb_rows - 4 : 0;
    }
}

static void fb_init(const struct zeal_handoff *h) {
    fb_init_ex(h, 1);
}

static inline void *mmio(uint64_t phys) {
    return (void *)(uintptr_t)(g_hhdm + phys);
}

static void gic_init(void) {
    volatile uint32_t *gicd = mmio(GICD_BASE_PHYS);

    uint64_t mpidr;
    __asm__ volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    uint32_t want_aff = (uint32_t)((mpidr & 0xff)
        | (((mpidr >> 8) & 0xff) << 8)
        | (((mpidr >> 16) & 0xff) << 16)
        | (((mpidr >> 32) & 0xff) << 24));

    volatile uint32_t *gicr = NULL;
    for (uint64_t i = 0; i < 8; i++) {
        volatile uint32_t *rd = mmio(GICR_BASE_PHYS + i * GICR_STRIDE);
        uint64_t typer = ((uint64_t)rd[(GICR_TYPER / 4) + 1] << 32)
            | rd[GICR_TYPER / 4];
        if ((uint32_t)(typer >> 32) == want_aff) {
            gicr = rd;
            break;
        }
        if ((typer >> 4) & 1u) {
            break;
        }
    }
    if (!gicr) {
        gicr = mmio(GICR_BASE_PHYS + (mpidr & 0xff) * GICR_STRIDE);
    }

    uint32_t waker = gicr[GICR_WAKER / 4] & ~(1u << 1);
    gicr[GICR_WAKER / 4] = waker;
    __asm__ volatile("dsb sy");
    while (gicr[GICR_WAKER / 4] & (1u << 2)) {
    }

    /* Level-triggered PPI 30 */
    uint32_t icfgr1 = gicr[GICR_ICFGR1 / 4];
    icfgr1 &= ~(3u << ((TIMER_PPI - 16) * 2));
    gicr[GICR_ICFGR1 / 4] = icfgr1;

    gicr[GICR_ICENABLER0 / 4] = (1u << TIMER_PPI);
    gicr[GICR_ICPENDR0 / 4] = (1u << TIMER_PPI);
    __asm__ volatile("dsb sy");

    gicr[GICR_IGROUPR0 / 4] |= (1u << TIMER_PPI);
    ((volatile uint8_t *)((uintptr_t)gicr + GICR_IPRIORITYR))[TIMER_PPI] = 0x80;
    __asm__ volatile("dsb sy");
    gicr[GICR_ISENABLER0 / 4] = (1u << TIMER_PPI);
    __asm__ volatile("dsb sy");

    /* ARE_NS | EnableGrp1NS */
    gicd[GICD_CTLR / 4] = (1u << 4) | (1u << 1);
    __asm__ volatile("dsb sy");

    uint64_t sre;
    MRS_ICC(5, sre);
    sre |= 0x7;
    MSR_ICC(5, sre);
    __asm__ volatile("isb");

    __asm__ volatile("msr S3_0_C4_C6_0, %0" ::"r"(0xffull)); /* PMR */
    MSR_ICC(3, 0ull); /* BPR1 */
    MSR_ICC(7, 1ull); /* IGRPEN1 — was wrongly written as 0 before */
    __asm__ volatile("isb");

    g_gic_v2 = 0;

    if (g_uart) {
        uint64_t grp1, hppir;
        MRS_ICC(7, grp1);
        MRS_ICC(2, hppir);
        uart_puts(g_uart, "GICR ISEN=");
        uart_put_u64_hex(g_uart, gicr[GICR_ISENABLER0 / 4]);
        uart_puts(g_uart, " IGRPEN1=");
        uart_put_u64_hex(g_uart, grp1);
        uart_puts(g_uart, " HPPIR1=");
        uart_put_u64_hex(g_uart, hppir);
        uart_puts(g_uart, "\n");
    }
}

/* GIC-400 (GICv2) on Pi4 — map Device if raw HHDM aborts (FF range). */
#define GICV2_GICD_CTLR       0x000
#define GICV2_GICD_ISENABLER  0x100
#define GICV2_GICD_ICENABLER  0x180
#define GICV2_GICD_ICPENDR    0x280
#define GICV2_GICD_IPRIORITY  0x400
#define GICV2_GICD_ICFGR      0xC00
#define GICV2_GICC_CTLR       0x00
#define GICV2_GICC_PMR        0x04
#define GICV2_GICC_BPR        0x08
#define GICV2_GICC_IAR        0x0C
#define GICV2_GICC_EOIR       0x10

static uint64_t g_gicd_phys = PI4_GICD_PHYS;
static uint64_t g_gicc_phys = PI4_GICC_PHYS;

static int gicv2_probe_read(uint64_t hhdm, uint64_t gicd_phys) {
    volatile uint32_t *gicd = (volatile uint32_t *)(uintptr_t)(hhdm + gicd_phys);
    g_uart_probe = 1;
    g_sync_recover = 1;
    g_uart_dead = 0;
    (void)gicd[GICV2_GICD_CTLR / 4];
    __asm__ volatile("dsb sy");
    g_sync_recover = 0;
    g_uart_probe = 0;
    return g_uart_dead ? -1 : 0;
}

static int gicv2_init(uint64_t hhdm) {
    volatile uint32_t *gicd;
    volatile uint32_t *gicc;
    unsigned ppi = (unsigned)PI4_CNTP_PPI;

    g_gicd_phys = PI4_GICD_PHYS;
    g_gicc_phys = PI4_GICC_PHYS;

    if (gicv2_probe_read(hhdm, g_gicd_phys) != 0) {
        /* Punch Device 2MiB covering GICD+GICC (same as UART fix path). */
        if (!g_pt_pool_phys || mmio_map_2m(hhdm, PI4_GICD_PHYS) != 0) {
            return -1;
        }
        if (gicv2_probe_read(hhdm, g_gicd_phys) != 0) {
            /* Legacy ARM-local alias used by some firmware builds. */
            g_gicd_phys = 0x40041000ull;
            g_gicc_phys = 0x40042000ull;
            if (mmio_map_2m(hhdm, g_gicd_phys) != 0 ||
                gicv2_probe_read(hhdm, g_gicd_phys) != 0) {
                return -2;
            }
        }
    }

    gicd = (volatile uint32_t *)(uintptr_t)(hhdm + g_gicd_phys);
    gicc = (volatile uint32_t *)(uintptr_t)(hhdm + g_gicc_phys);

    /* Disable distributor while programming. */
    gicd[GICV2_GICD_CTLR / 4] = 0;
    __asm__ volatile("dsb sy");

    /* PPI 30: clear pending, priority, level-sensitive, enable. */
    gicd[(GICV2_GICD_ICPENDR / 4) + (ppi / 32)] = (1u << (ppi % 32));
    ((volatile uint8_t *)((uintptr_t)gicd + GICV2_GICD_IPRIORITY))[ppi] = 0xa0;
    {
        unsigned idx = ppi / 16;
        unsigned shift = (ppi % 16) * 2;
        uint32_t cfg = gicd[(GICV2_GICD_ICFGR / 4) + idx];
        cfg &= ~(3u << shift); /* level */
        gicd[(GICV2_GICD_ICFGR / 4) + idx] = cfg;
    }
    gicd[(GICV2_GICD_ICENABLER / 4) + (ppi / 32)] = (1u << (ppi % 32));
    __asm__ volatile("dsb sy");
    gicd[(GICV2_GICD_ISENABLER / 4) + (ppi / 32)] = (1u << (ppi % 32));
    __asm__ volatile("dsb sy");

    gicd[GICV2_GICD_CTLR / 4] = 1; /* EnableGrp0 */
    __asm__ volatile("dsb sy");

    gicc[GICV2_GICC_PMR / 4] = 0xff;
    gicc[GICV2_GICC_BPR / 4] = 0;
    gicc[GICV2_GICC_CTLR / 4] = 1; /* EnableGrp0 */
    __asm__ volatile("dsb sy");
    __asm__ volatile("isb");

    g_gic_v2 = 1;
    return 0;
}

static void timer_arm(void) {
    __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(0ull));
    __asm__ volatile("isb");
    __asm__ volatile("msr cntp_tval_el0, %0" ::"r"(g_timer_ticks));
    __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(1ull));
    __asm__ volatile("isb");
}

static void vectors_install(void) {
    __asm__ volatile(
        "mov x0, sp\n"
        "msr spsel, #1\n"
        "mov sp, x0\n"
        "isb\n"
        :
        :
        : "x0", "memory");
    /* PC-relative — works even before GOT relocs (belt and suspenders). */
    uint64_t vbar;
    __asm__ volatile("adr %0, exception_vectors" : "=r"(vbar));
    __asm__ volatile("msr vbar_el1, %0" ::"r"(vbar));
    __asm__ volatile("isb");
}

void exc_handle_fatal(uint64_t unused) {
    (void)unused;
    if (g_uart) {
        uart_puts(g_uart, "FATAL exception\n");
    }
    for (;;) {
        __asm__ volatile("wfi");
    }
}

/* Digits-only hex — safe if .rodata/statics are unhealthy mid-fault. */
static void uart_put_hex_raw(volatile uint8_t *uart, uint64_t v) {
    uart_write(uart, '0');
    uart_write(uart, 'x');
    for (int i = 60; i >= 0; i -= 4) {
        unsigned n = (unsigned)((v >> i) & 0xf);
        uart_write(uart, (char)(n < 10 ? '0' + n : 'a' + (n - 10)));
    }
}

void exc_handle_sync(uint64_t *frame) {
    uint64_t esr, elr, far;
    __asm__ volatile("mrs %0, esr_el1" : "=r"(esr));
    __asm__ volatile("mrs %0, elr_el1" : "=r"(elr));
    __asm__ volatile("mrs %0, far_el1" : "=r"(far));
    uint64_t ec = (esr >> 26) & 0x3f;
    if (ec == 0x15) {
        g_svc_hits++;
        if (g_uart) {
            uart_puts(g_uart, "sync: SVC ok (hits=");
            uart_put_u32_dec(g_uart, g_svc_hits);
            uart_puts(g_uart, ")\n");
        }
        return;
    }
    /* UTM/HVF may trap CNTP_* (EL1PCEN) while CNTFRQ still works. Skip insn.
     * PI-DIAG also uses this around first UART DR touches. */
    if (g_sync_recover) {
        g_sync_skipped++;
        if (g_uart_probe) {
            uint64_t esr, far;
            __asm__ volatile("mrs %0, esr_el1" : "=r"(esr));
            __asm__ volatile("mrs %0, far_el1" : "=r"(far));
            g_probe_esr = esr;
            g_probe_far = far;
            g_uart_dead = 1;
        }
        elr += 4;
        __asm__ volatile("msr elr_el1, %0" ::"r"(elr));
        return;
    }
    if (g_uart) {
        uart_puts(g_uart, "sync: ESR=");
        uart_put_hex_raw(g_uart, esr);
        uart_puts(g_uart, " EC=");
        uart_put_hex_raw(g_uart, ec);
        uart_puts(g_uart, " ELR=");
        uart_put_hex_raw(g_uart, elr);
        uart_puts(g_uart, " FAR=");
        uart_put_hex_raw(g_uart, far);
        /* The exception entry saves x29/x30 at these fixed frame slots. Keep
         * them in the fatal report so synchronous faults can be traced back
         * through the AArch64 caller chain without a debugger. */
        if (frame) {
            uart_puts(g_uart, " FP=");
            uart_put_hex_raw(g_uart, frame[29]);
            uart_puts(g_uart, " LR=");
            uart_put_hex_raw(g_uart, frame[30]);
            uart_puts(g_uart, " X2=");
            uart_put_hex_raw(g_uart, frame[2]);
            uart_puts(g_uart, " X0=");
            uart_put_hex_raw(g_uart, frame[0]);
            uart_puts(g_uart, " X1=");
            uart_put_hex_raw(g_uart, frame[1]);
            uart_puts(g_uart, " X3=");
            uart_put_hex_raw(g_uart, frame[3]);
        }
#ifndef ZEAL_PI_DIAG
        if (frame) {
            uintptr_t offset = 0;
            const char *name = zc_debug_function_for_pc((uintptr_t)elr,
                                                         &offset);
            if (name) {
                uart_puts(g_uart, " INSN=");
                uart_put_hex_raw(g_uart,
                    *(volatile const uint32_t *)(uintptr_t)elr);
                if (offset >= 128) {
                    uart_puts(g_uart, " PREV=");
                    for (int i = 32; i >= 1; --i) {
                        uart_put_hex_raw(g_uart,
                            *(volatile const uint32_t *)(uintptr_t)(elr - 4 * i));
                        uart_write(g_uart, ',');
                    }
                }
                uart_puts(g_uart, " FUNC=");
                uart_puts(g_uart, name);
                uart_puts(g_uart, "+");
                uart_put_hex_raw(g_uart, offset);
            }
        }
#endif
        uart_puts(g_uart, "\n");
    }
    for (;;) {
        __asm__ volatile("wfi");
    }
}

void exc_handle_irq(uint64_t *frame) {
    (void)frame;
    uint32_t id;

    if (g_gic_v2) {
        volatile uint32_t *gicc =
            (volatile uint32_t *)(uintptr_t)(g_hhdm + g_gicc_phys);
        id = gicc[GICV2_GICC_IAR / 4] & 0x3ffu;
    } else {
        uint64_t iar;
        MRS_ICC(0, iar);
        id = (uint32_t)(iar & 0x3ff);
    }

    if (id == TIMER_PPI || id == (uint32_t)PI4_CNTP_PPI) {
        g_timer_irqs++;
        if (g_uart) {
            uart_puts(g_uart, "irq: CNTP tick ");
            uart_put_u32_dec(g_uart, g_timer_irqs);
            uart_puts(g_uart, "\n");
        }
        timer_arm();
    } else if (g_uart && id < 1020) {
        uart_puts(g_uart, "irq: id=");
        uart_put_u32_dec(g_uart, id);
        uart_puts(g_uart, "\n");
    }

    if (id < 1020) {
        if (g_gic_v2) {
            volatile uint32_t *gicc =
                (volatile uint32_t *)(uintptr_t)(g_hhdm + g_gicc_phys);
            gicc[GICV2_GICC_EOIR / 4] = id;
        } else {
            uint64_t iar = id;
            MSR_ICC(1, iar);
        }
    }
}

static void enable_irq(void) {
    __asm__ volatile("msr daifclr, #2");
    __asm__ volatile("isb");
}

static void shell_run(const struct zeal_handoff *h);

/* Limine-mapped RWX scratch for emit-and-run (HolyC/JIT bridge). */
__attribute__((section(".jit"), aligned(64)))
static uint32_t g_jit_buf[8192]; /* was 1536 — Lattice angles[35] brace init */
__attribute__((section(".jit"), aligned(64)))
static uint32_t g_fn_code[HC_MAX_FNS][16384]; /* was 8192 — Lattice NearLattice + if(tt.w) */

static struct hc_fn_table g_fn_tbl;
static void jit_icache_flush(void *addr, size_t size) {
    uintptr_t start = (uintptr_t)addr & ~63ull;
    uintptr_t end = ((uintptr_t)addr + size + 63) & ~63ull;
    for (uintptr_t p = start; p < end; p += 64) {
        __asm__ volatile("dc cvau, %0" ::"r"(p) : "memory");
    }
    __asm__ volatile("dsb ish");
    for (uintptr_t p = start; p < end; p += 64) {
        __asm__ volatile("ic ivau, %0" ::"r"(p) : "memory");
    }
    __asm__ volatile("dsb ish");
    __asm__ volatile("isb");
}

/* Called from JITed / IR-compiled code. */
static void jit_host_puts(void) {
    con_puts("hc: host callback OK\n");
}

static uint64_t hc_host_marker(void) {
    return 0x48434F4BULL; /* "HCOK" */
}

static uint64_t hc_host_inc(uint64_t x) {
    return x + 1;
}

uint64_t hc_builtin_abs(uint64_t x) {
    int64_t v = (int64_t)x;
    return (uint64_t)(v < 0 ? -v : v);
}

/* F64 abs: clear sign bit (no FP ops; kernel stays +nofp). */
uint64_t hc_builtin_fabs(uint64_t xbits) {
    return xbits & 0x7fffffffffffffffULL;
}

/* Lattice DCFill(dc) / DCDel(dc) stubs — CDC body deferred beyond color. */
/* Minimal CDC for Lattice: color @0; depth_buf @8 (DCDepthBufAlloc). */
struct hc_cdc {
    uint64_t color;
    uint64_t depth_buf;
};
static struct hc_cdc g_hc_cdc;
/* Sparse z-buffer (tiny mcmodel — full FB BSS is too large). */
#define HC_DEPTH_MAP 2048
struct hc_depth_cell {
    uint16_t x;
    uint16_t y;
    int32_t z;
};
static struct hc_depth_cell g_hc_depth_map[HC_DEPTH_MAP];
static int g_hc_depth_on;

/* PopUpColor state (M148+); painted on FB for latticeplay (M160/M164/M165/M205/M206). */
static int g_hc_popup_i;
static int g_hc_popup_live;
static uint8_t g_hc_popup_mid = 14;  /* YELLOW — matches TurtleInit */
static uint8_t g_hc_popup_edge = 0;  /* BLACK */
/* M206: first word of last Mid/Edge PopUpColor header (survives Refresh). */
static char g_hc_popup_mid_title[9];
static char g_hc_popup_edge_title[9];

static void hc_popup_store_title(int is_edge, const char *h) {
    char *dst = is_edge ? g_hc_popup_edge_title : g_hc_popup_mid_title;
    int k;
    if (!h || !h[0]) {
        return;
    }
    for (k = 0; h[k] && h[k] != '\n' && h[k] != ' ' && k < 8; k++) {
        dst[k] = h[k];
    }
    dst[k] = 0;
}

static void hc_popup_paint_title(int which) {
    const char *lab = which ? g_hc_popup_edge_title : g_hc_popup_mid_title;
    uint32_t x0 = which ? 72u : 8u;
    uint32_t y0 = (uint32_t)g_fb_h > 40u ? (uint32_t)g_fb_h - 28u : 8u;
    uint32_t col = x0 / 8u;
    uint32_t row = (y0 >= 16u ? y0 - 16u : 0u) / 8u;
    int k;
    if (!g_fb || !lab[0]) {
        return;
    }
    fb_fillrect(x0, row * 8u, 64u, 8u, 0);
    for (k = 0; lab[k] && k < 8; k++) {
        fb_draw_char(col + (uint32_t)k, row, lab[k], 0x00E0E0E0u);
    }
}

static void hc_popup_paint_swatch(int which, uint64_t c) {
    static const uint32_t pal[16] = {
        0x00000000u, 0x000000AAu, 0x0000AA00u, 0x0000AAAAu, 0x00AA0000u, 0x00AA00AAu,
        0x00AA5500u, 0x00AAAAAAu, 0x00555555u, 0x005555FFu, 0x0055FF55u, 0x0055FFFFu,
        0x00FF5555u, 0x00FF55FFu, 0x00FFFF55u, 0x00FFFFFFu,
    };
    const char *lab = which ? "Edge" : "Mid";
    uint32_t x0 = which ? 72u : 8u;
    uint32_t y0 = (uint32_t)g_fb_h > 40u ? (uint32_t)g_fb_h - 28u : 8u;
    uint32_t col = x0 / 8u;
    uint32_t row = (y0 >= 8u ? y0 - 8u : 0u) / 8u;
    int k;
    if (!g_fb) {
        return;
    }
    fb_fillrect(x0, y0, 56u, 20u, pal[(int)c & 15]);
    fb_fillrect(x0, y0, 56u, 1u, 0x00E0E0E0u);
    fb_fillrect(x0, y0 + 19u, 56u, 1u, 0x00E0E0E0u);
    fb_fillrect(x0, y0, 1u, 20u, 0x00E0E0E0u);
    fb_fillrect(x0 + 55u, y0, 1u, 20u, 0x00E0E0E0u);
    for (k = 0; lab[k]; k++) {
        fb_draw_char(col + (uint32_t)k, row, lab[k], 0x00E0E0E0u);
    }
    hc_popup_paint_title(which);
}

/* M165–M169/M191: repaint Mid/Edge + controls hints so Cls/Refresh keep chrome. */
static void hc_popup_paint_live(void) {
    /* TempleOS 0xE9 = θ (M184 glyph); match stock Lattice HUD naming. */
    static const char hint[] = "Esc=exit Enter=restart Space=step c=color +/-=w";
    static const char hint2[] = "L-click=place R-drag=aim e=ends";
    static const char hint3[] = "arrows=d\xe9/speed 0-9=layer";
    uint32_t i;
    if (!g_hc_popup_live) {
        return;
    }
    hc_popup_paint_swatch(0, g_hc_popup_mid);
    hc_popup_paint_swatch(1, g_hc_popup_edge);
    /* Bands match GrPrint(dc, 0, 16/24/32, …) — idle DrawIt must not bury the cues. */
    if (g_fb) {
        fb_fillrect(0, 16u, 64u * 8u, 24u, 0);
        for (i = 0; hint[i]; i++) {
            fb_draw_char(i, 2u, hint[i], 0x00E0E0E0u);
        }
        for (i = 0; hint2[i]; i++) {
            fb_draw_char(i, 3u, hint2[i], 0x00E0E0E0u);
        }
        for (i = 0; hint3[i]; i++) {
            fb_draw_char(i, 4u, hint3[i], 0x00E0E0E0u);
        }
    }
}

static void hc_depth_map_clear(void) {
    int i;
    for (i = 0; i < HC_DEPTH_MAP; i++) {
        g_hc_depth_map[i].x = 0xFFFFu;
        g_hc_depth_map[i].y = 0xFFFFu;
        g_hc_depth_map[i].z = (int32_t)0x7fffffff;
    }
}

uint64_t hc_builtin_dcfill(uint64_t dc) {
    (void)dc;
    /* Lattice DCFill — clear virt FB so DiskLat/NearLattice leave a clean surface. */
    if (g_fb) {
        fb_clear(0);
    }
    /* M174: Restart must not keep stale z cells from prior strokes. */
    if (g_hc_depth_on) {
        hc_depth_map_clear();
    }
    /* M164: latticeplay Restart TurtleInit → YELLOW/BLACK; sync picker + swatches. */
    if (g_hc_popup_live) {
        g_hc_popup_i = 0;
        g_hc_popup_mid = 14;
        g_hc_popup_edge = 0;
        g_hc_popup_mid_title[0] = 0;
        g_hc_popup_edge_title[0] = 0;
        hc_popup_paint_live();
    }
    return 0;
}

uint64_t hc_builtin_dcdel(uint64_t dc) {
    (void)dc;
    return 0;
}

uint64_t hc_builtin_dcdepthbufreset(uint64_t dc) {
    struct hc_cdc *d;

    if (!dc) {
        return 0;
    }
    d = (struct hc_cdc *)(uintptr_t)dc;
    if (!d->depth_buf) {
        return 0;
    }
    hc_depth_map_clear();
    return d->depth_buf;
}

uint64_t hc_builtin_dcdepthbufalloc(uint64_t dc) {
    struct hc_cdc *d;

    if (!dc) {
        return 0;
    }
    d = (struct hc_cdc *)(uintptr_t)dc;
    g_hc_depth_on = 1;
    /* Non-null sentinel — HolyC reads dc->depth_buf; map lives in g_hc_depth_map. */
    d->depth_buf = (uint64_t)(uintptr_t)g_hc_depth_map;
    return hc_builtin_dcdepthbufreset(dc);
}

/* Return 1 if plot allowed (closer z); 0 if occluded. */
static int hc_depth_try(int32_t xi, int32_t yi, int32_t zi) {
    uint32_t h;
    int i;

    if (!g_hc_depth_on || xi < 0 || yi < 0 || xi > 0xFFFE || yi > 0xFFFE) {
        return 1;
    }
    h = (uint32_t)xi * 2654435761u ^ (uint32_t)yi * 40503u;
    for (i = 0; i < HC_DEPTH_MAP; i++) {
        uint32_t idx = (h + (uint32_t)i) % (uint32_t)HC_DEPTH_MAP;
        struct hc_depth_cell *c = &g_hc_depth_map[idx];
        if (c->x == 0xFFFFu) {
            c->x = (uint16_t)xi;
            c->y = (uint16_t)yi;
            c->z = zi;
            return 1;
        }
        if (c->x == (uint16_t)xi && c->y == (uint16_t)yi) {
            if (zi > c->z) {
                return 0;
            }
            c->z = zi;
            return 1;
        }
    }
    /* Map full — allow plot (bring-up). */
    return 1;
}

/* Non-interactive Lattice color picker (M148/M150/M160/M161/M164/M165/M204).
 * M204: HolyC string header in g_hc_mem — "Mid…" vs "Edge…" selects role
 * (fallback: call-pair odd/even). Live cycles Mid/Edge pairs after the first.
 * M160: paint FB swatches. M164: DCFill resets picker + default Mid/Edge.
 * M165: track Mid/Edge so Cls/Refresh keep swatches from entry through idle. */
uint64_t hc_builtin_popupcolor(uint64_t header) {
    const char *h = (const char *)(uintptr_t)header;
    int is_edge;
    int i;
    uint64_t c;
    /* Pair 0 stays YELLOW/BLACK for smokes + first latticeplay 'c'. */
    static const uint8_t mids[] = {14, 4, 9, 10, 12, 11, 13, 2, 5, 15};
    static const uint8_t edges[] = {0, 15, 0, 15, 0, 0, 0, 15, 0, 0};
    /* Prefer header text when it looks like a C string in g_hc_mem / ROM. */
    is_edge = -1;
    if (h) {
        uintptr_t p = (uintptr_t)h;
        if (p > 0x1000ull) {
            if (h[0] == 'E' && h[1] == 'd' && h[2] == 'g' && h[3] == 'e') {
                is_edge = 1;
            } else if (h[0] == 'M' && h[1] == 'i' && h[2] == 'd') {
                is_edge = 0;
            }
        }
    }
    i = g_hc_popup_i++;
    if (is_edge < 0) {
        is_edge = i & 1;
    }
    if (g_hc_popup_live) {
        int pair = (i / 2) % (int)(sizeof mids / sizeof mids[0]);
        c = is_edge ? (uint64_t)edges[pair] : (uint64_t)mids[pair];
    } else {
        c = is_edge ? 0ull : 14ull; /* Edge BLACK, Mid YELLOW */
    }
    if (is_edge) {
        g_hc_popup_edge = (uint8_t)(c & 15);
    } else {
        g_hc_popup_mid = (uint8_t)(c & 15);
    }
    hc_popup_paint_swatch(is_edge, c);
    /* M205/M206: store + paint header word (Refresh via paint_swatch keeps it). */
    if (g_hc_popup_live && h && (uintptr_t)h > 0x1000ull && h[0]) {
        hc_popup_store_title(is_edge, h);
        hc_popup_paint_title(is_edge);
    }
    return c;
}

/* Scripted MessageGet queue for Lattice while(TRUE) event-loop smokes. */
#define HC_MSG_QUE_MAX 16
#define HC_MSG_BIT(t) (1ULL << (uint64_t)(t))
static uint64_t g_hc_msg_type[HC_MSG_QUE_MAX];
static uint64_t g_hc_msg_a1[HC_MSG_QUE_MAX];
static uint64_t g_hc_msg_a2[HC_MSG_QUE_MAX];
static int g_hc_msg_head;
static int g_hc_msg_tail;
/* Virtio edge/move state for MessageGet when the scripted queue is empty. */
static int g_hc_msg_btn_prev;
static int g_hc_msg_rbtn_prev;
static int g_hc_msg_have_xy;
static uint32_t g_hc_msg_last_x, g_hc_msg_last_y;

static void hc_msg_que_reset(void) {
    g_hc_msg_head = 0;
    g_hc_msg_tail = 0;
    g_hc_msg_btn_prev = 0;
    g_hc_msg_rbtn_prev = 0;
    g_hc_msg_have_xy = 0;
}

/* Bring-up: MsgQuePush(type, arg1, arg2) seeds the queue for LoopLite. */
uint64_t hc_builtin_msgquepush(uint64_t type, uint64_t a1, uint64_t a2) {
    int next = (g_hc_msg_tail + 1) % HC_MSG_QUE_MAX;
    if (next == g_hc_msg_head) {
        return 0; /* full */
    }
    g_hc_msg_type[g_hc_msg_tail] = type;
    g_hc_msg_a1[g_hc_msg_tail] = a1;
    g_hc_msg_a2[g_hc_msg_tail] = a2;
    g_hc_msg_tail = next;
    return 1;
}

static void hc_msg_store_outs(uint64_t p_arg1, uint64_t p_arg2, uint64_t a1, uint64_t a2) {
    if (p_arg1) {
        *(volatile uint64_t *)(uintptr_t)p_arg1 = a1;
    }
    if (p_arg2) {
        *(volatile uint64_t *)(uintptr_t)p_arg2 = a2;
    }
}

static int uart_getc_nb(void); /* Pi MessageGet fallback; defined with PL011 RX */

/* Pop scripted event (mask-filtered), else virtio (QEMU/UTM) or UART (Pi). */
uint64_t hc_builtin_messageget(uint64_t p_arg1, uint64_t p_arg2, uint64_t mask) {
    while (g_hc_msg_head != g_hc_msg_tail) {
        uint64_t t = g_hc_msg_type[g_hc_msg_head];
        uint64_t a1 = g_hc_msg_a1[g_hc_msg_head];
        uint64_t a2 = g_hc_msg_a2[g_hc_msg_head];
        g_hc_msg_head = (g_hc_msg_head + 1) % HC_MSG_QUE_MAX;
        if (mask & HC_MSG_BIT(t)) {
            hc_msg_store_outs(p_arg1, p_arg2, a1, a2);
            return t;
        }
        /* Drop events outside mask (scripted smokes push only wanted types). */
    }

    /* KEY_DOWN/KEY_UP: virtio typed msgs when present; UART ASCII KEY_DOWN only. */
    if ((mask & (HC_MSG_BIT(2) | HC_MSG_BIT(3))) && input_keyboard_ready()) {
        uint8_t t = 0;
        uint64_t a1 = 0, a2 = 0;
        while (input_keyboard_msg_nb(&t, &a1, &a2)) {
            if (mask & HC_MSG_BIT(t)) {
                hc_msg_store_outs(p_arg1, p_arg2, a1, a2);
                return t;
            }
            /* Drop events outside mask (same as scripted queue). */
        }
    } else if ((mask & HC_MSG_BIT(2)) && !input_keyboard_ready()) {
        int c = uart_getc_nb();
        if (c >= 0) {
            /* Serial Enter is often CR; Lattice Restart is '\n'. */
            if (c == '\r') {
                c = '\n';
            }
            hc_msg_store_outs(p_arg1, p_arg2, (uint64_t)(int64_t)c, 0);
            return 2;
        }
    }
    if (input_pointer_ready() && g_fb) {
        uint32_t x = 0, y = 0;
        int btns = input_pointer_buttons();
        (void)input_pointer_xy((uint32_t)g_fb_w, (uint32_t)g_fb_h, &x, &y);
        int btn = btns & 1;
        int rbtn = (btns & 2) ? 1 : 0;
        int prev = g_hc_msg_btn_prev;
        int rprev = g_hc_msg_rbtn_prev;
        g_hc_msg_btn_prev = btn;
        g_hc_msg_rbtn_prev = rbtn;
        if ((mask & HC_MSG_BIT(5)) && btn && !prev) { /* MS_L_DOWN */
            hc_msg_store_outs(p_arg1, p_arg2, x, y);
            return 5;
        }
        if ((mask & HC_MSG_BIT(6)) && !btn && prev) { /* MS_L_UP */
            hc_msg_store_outs(p_arg1, p_arg2, x, y);
            return 6;
        }
        if ((mask & HC_MSG_BIT(7)) && rbtn && !rprev) { /* MS_R_DOWN */
            hc_msg_store_outs(p_arg1, p_arg2, x, y);
            return 7;
        }
        if ((mask & HC_MSG_BIT(8)) && !rbtn && rprev) { /* MS_R_UP */
            hc_msg_store_outs(p_arg1, p_arg2, x, y);
            return 8;
        }
        if ((mask & HC_MSG_BIT(4)) && g_hc_msg_have_xy &&
            (x != g_hc_msg_last_x || y != g_hc_msg_last_y)) { /* MS_MOVE */
            g_hc_msg_last_x = x;
            g_hc_msg_last_y = y;
            hc_msg_store_outs(p_arg1, p_arg2, x, y);
            return 4;
        }
        g_hc_msg_last_x = x;
        g_hc_msg_last_y = y;
        g_hc_msg_have_xy = 1;
    }

    hc_msg_store_outs(p_arg1, p_arg2, 0, 0);
    /* M189: live latticeplay/stock-shaped sessions yield ~60 Hz; smokes stay snappy. */
    if (g_hc_popup_live) {
        (void)hc_builtin_sleep(16);
    }
    return 0; /* MESSAGE_NULL */
}

uint64_t hc_builtin_menupush(uint64_t s) {
    (void)s;
    return 0;
}

uint64_t hc_builtin_menupop(void) {
    return 0;
}

uint64_t hc_builtin_nop0(void) {
    return 0;
}

uint64_t g_hc_fs_win_inhibit;
uint64_t g_hc_fs_draw_it;
uint64_t g_hc_fs_cur_menu;

/* Lattice CMenuEntry stubs — MenuEntryFind returns one entry per path. */
struct hc_menu_entry {
    uint64_t checked;
};
#define HC_MENU_PATH_MAX 32
#define HC_MENU_ENTRIES  12 /* Layer0..9 + Ends + fallback */
struct hc_menu_slot {
    char path[HC_MENU_PATH_MAX];
    struct hc_menu_entry ent;
};
static struct hc_menu_slot g_hc_menu_slots[HC_MENU_ENTRIES];
static int g_hc_menu_nslots;

static int hc_menu_streq(const char *a, const char *b) {
    if (!a || !b) {
        return 0;
    }
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static void hc_menu_path_copy(char *dst, const char *src, int cap) {
    int i = 0;
    if (!src || cap < 1) {
        if (cap > 0) {
            dst[0] = 0;
        }
        return;
    }
    while (src[i] && i < cap - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

uint64_t hc_builtin_menuentryfind(uint64_t menu, uint64_t path) {
    (void)menu;
    const char *p = (const char *)(uintptr_t)path;
    if (!p) {
        p = "";
    }
    for (int i = 0; i < g_hc_menu_nslots; i++) {
        if (hc_menu_streq(g_hc_menu_slots[i].path, p)) {
            return (uint64_t)(uintptr_t)&g_hc_menu_slots[i].ent;
        }
    }
    if (g_hc_menu_nslots >= HC_MENU_ENTRIES) {
        /* Reuse last slot for overflow paths. */
        hc_menu_path_copy(g_hc_menu_slots[HC_MENU_ENTRIES - 1].path, p, HC_MENU_PATH_MAX);
        return (uint64_t)(uintptr_t)&g_hc_menu_slots[HC_MENU_ENTRIES - 1].ent;
    }
    hc_menu_path_copy(g_hc_menu_slots[g_hc_menu_nslots].path, p, HC_MENU_PATH_MAX);
    g_hc_menu_slots[g_hc_menu_nslots].ent.checked = 0;
    g_hc_menu_nslots++;
    return (uint64_t)(uintptr_t)&g_hc_menu_slots[g_hc_menu_nslots - 1].ent;
}

static uint32_t hc_dc_rgb(uint64_t dc) {
    static const uint32_t pal[16] = {
        0x00000000u, 0x000000AAu, 0x0000AA00u, 0x0000AAAAu, 0x00AA0000u, 0x00AA00AAu,
        0x00AA5500u, 0x00AAAAAAu, 0x00555555u, 0x005555FFu, 0x0055FF55u, 0x0055FFFFu,
        0x00FF5555u, 0x00FF55FFu, 0x00FFFF55u, 0x00FFFFFFu,
    };
    if (!dc) {
        return 0x00FFFF00u; /* Plot3Lite bring-up yellow when dc==0 */
    }
    uint64_t c = *(volatile uint64_t *)(uintptr_t)dc;
    if (c < 16) {
        return pal[(int)c];
    }
    return (uint32_t)c;
}

static uint32_t hc_dc_rgb_at(uint64_t dc, int32_t x, int32_t y) {
    static const uint32_t pal[16] = {
        0x00000000u, 0x000000AAu, 0x0000AA00u, 0x0000AAAAu,
        0x00AA0000u, 0x00AA00AAu, 0x00AA5500u, 0x00AAAAAAu,
        0x00555555u, 0x005555FFu, 0x0055FF55u, 0x0055FFFFu,
        0x00FF5555u, 0x00FF55FFu, 0x00FFFF55u, 0x00FFFFFFu,
    };
    uint64_t color;
    uint8_t index;

    if (!dc)
        return 0x00FFFF00u;
    color = *(volatile uint64_t *)(uintptr_t)dc;
    if (color & 0x40000000ull) {
        index = (uint8_t)((color >> (((x + y) & 1) ? 16 : 0)) & 0xFF);
        return index < 16 ? pal[index] : (uint32_t)index;
    }
    if (color < 16)
        return pal[(int)color];
    return (uint32_t)color;
}

/* Lattice DCAlias(dc=NULL,task=NULL) → shared CDC stub. */
uint64_t hc_builtin_dcalias(void) {
    return (uint64_t)(uintptr_t)&g_hc_cdc;
}

/* Lattice Refresh — WinMgr-shaped call of Fs->draw_it(task, dc). */
uint64_t hc_builtin_refresh(void) {
    if (!g_hc_fs_draw_it) {
        return 0;
    }
    typedef void (*hc_draw_it_fn)(uint64_t task, uint64_t dc);
    ((hc_draw_it_fn)(uintptr_t)g_hc_fs_draw_it)(0, (uint64_t)(uintptr_t)&g_hc_cdc);
    /* M165: DrawIt/TurtleMove can cover bottom swatches — keep them on top. */
    hc_popup_paint_live();
    return 1;
}

uint64_t hc_builtin_fs_pix_width(void) {
    return g_fb_w;
}

uint64_t hc_builtin_fs_pix_height(void) {
    return g_fb_h;
}

uint64_t hc_builtin_min(uint64_t a, uint64_t b) {
    return (int64_t)a < (int64_t)b ? a : b;
}

uint64_t hc_builtin_max(uint64_t a, uint64_t b) {
    return (int64_t)a > (int64_t)b ? a : b;
}

uint64_t hc_builtin_sign(uint64_t x) {
    int64_t v = (int64_t)x;
    if (v < 0) {
        return (uint64_t)(int64_t)-1;
    }
    if (v > 0) {
        return 1;
    }
    return 0;
}

uint64_t hc_builtin_clamp(uint64_t v, uint64_t lo, uint64_t hi) {
    int64_t x = (int64_t)v, a = (int64_t)lo, b = (int64_t)hi;
    if (x < a) {
        return (uint64_t)a;
    }
    if (x > b) {
        return (uint64_t)b;
    }
    return (uint64_t)x;
}

uint64_t hc_builtin_sqr(uint64_t x) {
    int64_t v = (int64_t)x;
    return (uint64_t)(v * v);
}

uint64_t hc_builtin_putpixel(uint64_t x, uint64_t y, uint64_t c) {
    fb_putpixel((uint32_t)x, (uint32_t)y, (uint32_t)c);
    return 0;
}

static uint32_t zeal_graphics_color(uint32_t color) {
    if (color < 16)
        return g_graphics_palette_ready ? g_graphics_palette[color]
                                        : g_text_palette[color];
    return color & 0x00ffffffu;
}

void zeal_fb_graph_palette_set(const uint8_t *palette) {
    if (!palette)
        return;
    for (size_t i = 0; i < 16; i++) {
        /* ZealOS CBGR24 is a padded 32-bit struct: b, g, r, pad. */
        uint32_t b = palette[i * 4];
        uint32_t g = palette[i * 4 + 1];
        uint32_t r = palette[i * 4 + 2];
        g_graphics_palette[i] = (r << 16) | (g << 8) | b;
    }
    g_graphics_palette_ready = 1;
}

uint64_t zeal_fb_graph_color(uint32_t color) {
    return zeal_graphics_color(color);
}

void zeal_fb_set_win_z_buffer(const uint16_t *z_buf) {
    g_fb_win_z_buf = z_buf;
}

void zeal_fb_set_win_pixel_z_buffer(const uint16_t *z_buf) {
    g_fb_win_pixel_z_buf = z_buf;
}

static int fb_graph_pixel_visible(uint32_t x, uint32_t y, uint16_t task_z) {
    if (!task_z)
        return 1;
    if (g_fb_win_pixel_z_buf) {
        if (x >= 800u || y >= 600u)
            return 0;
        return g_fb_win_pixel_z_buf[(size_t)y * 800u + x] <= task_z;
    }
    if (!g_fb_win_z_buf)
        return 1;
    size_t cell = (size_t)(y / 8) * ZEAL_TEXT_MAX_COLS + (size_t)(x / 8);
    return cell < ZEAL_TEXT_MAX_COLS * ZEAL_TEXT_MAX_ROWS &&
           g_fb_win_z_buf[cell] <= task_z;
}

uint64_t zeal_fb_graph_plot(int64_t x, int64_t y, uint32_t color,
                            int64_t left, int64_t top,
                            int64_t right, int64_t bottom,
                            uint16_t task_z) {
    if (!g_fb || x < left || x > right || y < top || y > bottom ||
        x < 0 || y < 0 || (uint64_t)x >= g_fb_w || (uint64_t)y >= g_fb_h)
        return 0;
    if (!fb_graph_pixel_visible((uint32_t)x, (uint32_t)y, task_z))
        return 0;
    fb_putpixel((uint32_t)x, (uint32_t)y, zeal_graphics_color(color));
    return 1;
}

uint64_t zeal_fb_graph_rect(int64_t x, int64_t y, int64_t width, int64_t height,
                            uint32_t color, int64_t left, int64_t top,
                            int64_t right, int64_t bottom, uint16_t task_z) {
    int64_t x0, y0, x1, y1, px, py;
    if (!g_fb || width <= 0 || height <= 0 || width > 16384 || height > 16384)
        return 0;
    if (x < -16384 || x > (int64_t)g_fb_w + 16384 ||
        y < -16384 || y > (int64_t)g_fb_h + 16384)
        return 0;
    x1 = x + width - 1;
    y1 = y + height - 1;
    x0 = x > left ? x : left;
    y0 = y > top ? y : top;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > right) x1 = right;
    if (y1 > bottom) y1 = bottom;
    if (x1 >= (int64_t)g_fb_w) x1 = (int64_t)g_fb_w - 1;
    if (y1 >= (int64_t)g_fb_h) y1 = (int64_t)g_fb_h - 1;
    if (x0 > x1 || y0 > y1)
        return 0;
    color = zeal_graphics_color(color);
    for (py = y0; py <= y1; py++)
        for (px = x0; px <= x1; px++) {
            if (!fb_graph_pixel_visible((uint32_t)px, (uint32_t)py, task_z))
                continue;
            fb_putpixel((uint32_t)px, (uint32_t)py, color);
        }
    return 1;
}

uint64_t zeal_fb_graph_text(int64_t x, int64_t y, uint32_t color,
                            int64_t left, int64_t top,
                            int64_t right, int64_t bottom, const char *text,
                            uint16_t task_z) {
    int64_t origin_x = x;
    uint64_t drawn = 0;
    if (!g_fb || !text || x < -8192 || x > (int64_t)g_fb_w + 8192 ||
        y < -8192 || y > (int64_t)g_fb_h + 8192)
        return 0;
    color = zeal_graphics_color(color);
    for (size_t i = 0; text[i] && i < 4096; i++) {
        unsigned char ch = (unsigned char)text[i];
        const uint8_t *glyph;
        if (ch == '\n') {
            x = origin_x;
            y += 8;
            continue;
        }
        if (ch == '\r')
            continue;
        if (ch == '\t') {
            x += 32;
            continue;
        }
        glyph = fb_glyph8(ch);
        for (int64_t row = 0; row < 8; row++) {
            uint8_t bits = glyph[row];
            for (int64_t col = 0; col < 8; col++) {
                int64_t px = x + col, py = y + row;
                if ((bits & (0x80u >> col)) && px >= left && px <= right &&
                    py >= top && py <= bottom && px >= 0 && py >= 0 &&
                    (uint64_t)px < g_fb_w && (uint64_t)py < g_fb_h &&
                    fb_graph_pixel_visible((uint32_t)px, (uint32_t)py,
                                           task_z)) {
                    fb_putpixel((uint32_t)px, (uint32_t)py, color);
                    drawn++;
                }
            }
        }
        x += 8;
    }
    return drawn;
}

uint64_t zeal_fb_graph_line(int64_t x0, int64_t y0, int64_t x1, int64_t y1,
                            uint32_t color, int64_t step, int64_t start,
                            int64_t left, int64_t top,
                            int64_t right, int64_t bottom,
                            uint16_t task_z) {
    int64_t dx, sx, dy, sy, err, i = 0;
    uint64_t plotted = 0;
    if (!g_fb || step < 1 || start < 0 ||
        x0 < -8192 || x0 > (int64_t)g_fb_w + 8192 ||
        x1 < -8192 || x1 > (int64_t)g_fb_w + 8192 ||
        y0 < -8192 || y0 > (int64_t)g_fb_h + 8192 ||
        y1 < -8192 || y1 > (int64_t)g_fb_h + 8192)
        return 0;
    dx = x0 < x1 ? x1 - x0 : x0 - x1;
    sx = x0 < x1 ? 1 : -1;
    dy = y0 < y1 ? y0 - y1 : y1 - y0;
    sy = y0 < y1 ? 1 : -1;
    err = dx + dy;
    for (;;) {
        if (i >= start && (i - start) % step == 0 &&
            zeal_fb_graph_plot(x0, y0, color, left, top, right, bottom,
                               task_z))
            plotted++;
        if (x0 == x1 && y0 == y1)
            break;
        int64_t e2 = err * 2;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
        if (++i > 16384)
            break;
    }
    return plotted != 0;
}

/* ZealOS GrPlot(dc,x,y) or bring-up GrPlot(x,y,color).
 * Heuristic: a0 > 0xFFFF → CDC pointer (Lattice); else (x,y,color). */
uint64_t hc_builtin_grplot(uint64_t a0, uint64_t a1, uint64_t a2) {
    uint64_t x, y, c;
    if (a0 > 0xFFFFull) {
        x = a1;
        y = a2;
        c = hc_dc_rgb_at(a0, (int32_t)a1, (int32_t)a2);
    } else {
        x = a0;
        y = a1;
        c = a2;
    }
    int32_t xi = (int32_t)x, yi = (int32_t)y;
    if (xi < 0 || yi < 0 || (uint32_t)xi >= g_fb_w || (uint32_t)yi >= g_fb_h) {
        return 0;
    }
    fb_putpixel((uint32_t)xi, (uint32_t)yi, (uint32_t)c);
    return 1;
}

/* Lattice GrPlot3(dc,x,y,z): 2D project; z-test when depth_buf set. */
uint64_t hc_builtin_grplot3(uint64_t dc, uint64_t x, uint64_t y, uint64_t z) {
    int32_t xi = (int32_t)x, yi = (int32_t)y, zi = (int32_t)z;
    if (xi < 0 || yi < 0 || (uint32_t)xi >= g_fb_w || (uint32_t)yi >= g_fb_h) {
        return 0;
    }
    if (dc) {
        struct hc_cdc *d = (struct hc_cdc *)(uintptr_t)dc;
        if (d->depth_buf && !hc_depth_try(xi, yi, zi)) {
            return 0;
        }
    }
    fb_putpixel((uint32_t)xi, (uint32_t)yi, hc_dc_rgb_at(dc, xi, yi));
    return 1;
}

/* Lattice GrLine3(dc,x1,y1,z1,x2,y2,z2): 2D Bresenham + optional z-test. */
uint64_t hc_builtin_grline3(uint64_t dc, uint64_t x1, uint64_t y1, uint64_t z1,
                            uint64_t x2, uint64_t y2, uint64_t z2) {
    int32_t xa = (int32_t)x1, ya = (int32_t)y1, xb = (int32_t)x2, yb = (int32_t)y2;
    int32_t za = (int32_t)z1, zb = (int32_t)z2;
    int32_t dx = xa < xb ? xb - xa : xa - xb;
    int32_t sx = xa < xb ? 1 : -1;
    int32_t dy = ya < yb ? ya - yb : yb - ya;
    int32_t sy = ya < yb ? 1 : -1;
    int32_t err = dx + dy;
    int32_t steps;
    int32_t i;
    uint32_t col = hc_dc_rgb(dc);
    int use_depth = 0;
    int plotted = 0;

    if (dc) {
        struct hc_cdc *d = (struct hc_cdc *)(uintptr_t)dc;
        use_depth = d->depth_buf != 0;
    }
    steps = dx > -dy ? dx : -dy;
    if (steps < 1) {
        steps = 1;
    }
    i = 0;
    for (;;) {
        int32_t zi = za + (int32_t)(((int64_t)(zb - za) * i) / steps);
        if (xa >= 0 && ya >= 0 && (uint32_t)xa < g_fb_w && (uint32_t)ya < g_fb_h) {
            if (!use_depth || hc_depth_try(xa, ya, zi)) {
                fb_putpixel((uint32_t)xa, (uint32_t)ya, col);
                plotted = 1;
            }
        }
        if (xa == xb && ya == yb) {
            break;
        }
        {
            int32_t e2 = err << 1;
            if (e2 >= dy) {
                err += dy;
                xa += sx;
            }
            if (e2 <= dx) {
                err += dx;
                ya += sy;
            }
        }
        i++;
    }
    return plotted ? 1 : 0;
}

/* ZealOS GrPeek(dc,x,y) — ignore dc; color or -1. */
uint64_t hc_builtin_grpeek_dc(uint64_t dc, uint64_t x, uint64_t y) {
    (void)dc;
    return (uint64_t)fb_peek((int32_t)x, (int32_t)y);
}

/* Bring-up GrPeek(x,y); color or -1. */
uint64_t hc_builtin_grpeek(uint64_t x, uint64_t y) {
    return (uint64_t)fb_peek((int32_t)x, (int32_t)y);
}

uint64_t hc_builtin_cls(uint64_t c) {
    fb_clear((uint32_t)c);
    /* M175: latticeplay entry Cls — same stale-z clear as DCFill. */
    if (g_hc_depth_on) {
        hc_depth_map_clear();
    }
    /* M165: latticeplay entry Cls — show Mid/Edge before first 'c'. */
    hc_popup_paint_live();
    return 0;
}

uint64_t hc_builtin_fillrect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t c) {
    fb_fillrect((uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h, (uint32_t)c);
    return 0;
}

uint64_t hc_builtin_grhline(uint64_t x, uint64_t y, uint64_t w, uint64_t c) {
    fb_fillrect((uint32_t)x, (uint32_t)y, (uint32_t)w, 1, (uint32_t)c);
    return 0;
}

uint64_t hc_builtin_grvline(uint64_t x, uint64_t y, uint64_t h, uint64_t c) {
    fb_fillrect((uint32_t)x, (uint32_t)y, 1, (uint32_t)h, (uint32_t)c);
    return 0;
}

uint64_t hc_builtin_grcircle(uint64_t cx, uint64_t cy, uint64_t r, uint64_t c) {
    int32_t x0 = (int32_t)cx, y0 = (int32_t)cy, rad = (int32_t)r;
    if (rad < 0) {
        return 0;
    }
    uint32_t col = (uint32_t)c;
    int32_t x = rad, y = 0, err = 1 - rad;
    while (x >= y) {
        fb_putpixel((uint32_t)(x0 + x), (uint32_t)(y0 + y), col);
        fb_putpixel((uint32_t)(x0 + y), (uint32_t)(y0 + x), col);
        fb_putpixel((uint32_t)(x0 - y), (uint32_t)(y0 + x), col);
        fb_putpixel((uint32_t)(x0 - x), (uint32_t)(y0 + y), col);
        fb_putpixel((uint32_t)(x0 - x), (uint32_t)(y0 - y), col);
        fb_putpixel((uint32_t)(x0 - y), (uint32_t)(y0 - x), col);
        fb_putpixel((uint32_t)(x0 + y), (uint32_t)(y0 - x), col);
        fb_putpixel((uint32_t)(x0 + x), (uint32_t)(y0 - y), col);
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
    return 0;
}

uint64_t hc_builtin_grfillcircle(uint64_t cx, uint64_t cy, uint64_t r, uint64_t c) {
    int32_t x0 = (int32_t)cx, y0 = (int32_t)cy, rad = (int32_t)r;
    if (rad < 0) {
        return 0;
    }
    uint32_t col = (uint32_t)c;
    for (int32_t dy = -rad; dy <= rad; dy++) {
        int32_t dx = 0;
        while (dx * dx + dy * dy <= rad * rad) {
            dx++;
        }
        dx--;
        if (dx >= 0) {
            fb_fillrect((uint32_t)(x0 - dx), (uint32_t)(y0 + dy), (uint32_t)(2 * dx + 1), 1, col);
        }
    }
    return 0;
}

uint64_t hc_builtin_grline(uint64_t x0, uint64_t y0, uint64_t x1, uint64_t y1, uint64_t c) {
    return hc_builtin_grline6(x0, y0, x1, y1, c, 1);
}

/* ZealOS Lattice: GrLine(dc,x1,y1,x2,y2) — color from CDC. */
uint64_t hc_builtin_grline_dc(uint64_t dc, uint64_t x1, uint64_t y1, uint64_t x2, uint64_t y2) {
    return hc_builtin_grline(x1, y1, x2, y2, hc_dc_rgb(dc));
}

/* ZealOS GrLine(..., step): plot every `step` pixels along the line (start=0). */
uint64_t hc_builtin_grline6(uint64_t x0, uint64_t y0, uint64_t x1, uint64_t y1, uint64_t c,
                            uint64_t step) {
    int32_t xa = (int32_t)x0, ya = (int32_t)y0, xb = (int32_t)x1, yb = (int32_t)y1;
    int32_t dx = xa < xb ? xb - xa : xa - xb;
    int32_t sx = xa < xb ? 1 : -1;
    int32_t dy = ya < yb ? ya - yb : yb - ya; /* always ≤ 0 */
    int32_t sy = ya < yb ? 1 : -1;
    int32_t err = dx + dy;
    uint32_t col = (uint32_t)c;
    int64_t st = (int64_t)step;
    if (st < 1) {
        st = 1;
    }
    int64_t n = 0;
    for (;;) {
        if ((n % st) == 0) {
            fb_putpixel((uint32_t)xa, (uint32_t)ya, col);
        }
        if (xa == xb && ya == yb) {
            break;
        }
        int32_t e2 = err << 1;
        if (e2 >= dy) {
            err += dy;
            xa += sx;
        }
        if (e2 <= dx) {
            err += dx;
            ya += sy;
        }
        n++;
    }
    return 0;
}

uint64_t hc_builtin_putchar(uint64_t ch) {
    con_write((char)(ch & 0xff));
    return ch & 0xff;
}

uint64_t hc_builtin_print(uint64_t s) {
    const char *p = (const char *)(uintptr_t)s;
    if (!p) {
        return 0;
    }
    uint64_t n = 0;
    while (*p) {
        con_write(*p++);
        n++;
    }
    return n;
}

uint64_t hc_builtin_printat(uint64_t col, uint64_t row, uint64_t s) {
    const char *p = (const char *)(uintptr_t)s;
    if (!p || !g_fb) {
        return 0;
    }
    uint32_t c0 = (uint32_t)col, r0 = (uint32_t)row;
    uint64_t n = 0;
    while (*p) {
        char ch = *p++;
        if (ch == '\n') {
            c0 = (uint32_t)col;
            r0++;
            continue;
        }
        if (c0 < g_fb_cols && r0 < g_fb_rows) {
            fb_draw_char(c0, r0, ch, 0x00E0E0E0u);
        }
        c0++;
        n++;
    }
    return n;
}

/* Lattice: GrPrint(dc, x, y, fmt) — x/y pixels → 8px cells.
 * Wipe a fixed HUD band first so shorter Refresh lines do not leave digit soup. */
uint64_t hc_builtin_grprint(uint64_t dc, uint64_t x, uint64_t y, uint64_t fmt) {
    (void)dc;
    if (g_fb) {
        fb_fillrect((uint32_t)x, (uint32_t)y, 64u * 8u, 8u, 0);
    }
    return hc_builtin_printat(x / 8, y / 8, fmt);
}

uint64_t hc_builtin_printi64(uint64_t v) {
    int64_t x = (int64_t)v;
    char buf[24];
    int n = 0;
    int extra = 0;
    uint64_t u;
    if (x < 0) {
        con_write('-');
        extra = 1;
        u = (x == (int64_t)((uint64_t)1 << 63)) ? ((uint64_t)1 << 63) : (uint64_t)(-x);
    } else {
        u = (uint64_t)x;
    }
    if (u == 0) {
        con_write('0');
        return (uint64_t)(extra + 1);
    }
    while (u && n < 20) {
        buf[n++] = (char)('0' + (u % 10));
        u /= 10;
    }
    int digits = n;
    while (n--) {
        con_write(buf[n]);
    }
    return (uint64_t)(extra + digits);
}

uint64_t hc_builtin_strlen(uint64_t s) {
    const char *p = (const char *)(uintptr_t)s;
    uint64_t n = 0;
    if (!p) {
        return 0;
    }
    while (*p++) {
        n++;
    }
    return n;
}

uint64_t hc_builtin_strcmp(uint64_t a, uint64_t b) {
    const char *pa = (const char *)(uintptr_t)a;
    const char *pb = (const char *)(uintptr_t)b;
    if (!pa) {
        pa = "";
    }
    if (!pb) {
        pb = "";
    }
    while (*pa && *pa == *pb) {
        pa++;
        pb++;
    }
    return (uint64_t)(int64_t)((int)(unsigned char)*pa - (int)(unsigned char)*pb);
}

uint64_t hc_builtin_strcpy(uint64_t dst, uint64_t src) {
    char *d = (char *)(uintptr_t)dst;
    const char *s = (const char *)(uintptr_t)src;
    if (!d) {
        return 0;
    }
    if (!s) {
        s = "";
    }
    char *out = d;
    while ((*d++ = *s++)) {
    }
    return (uint64_t)(uintptr_t)out;
}

uint64_t hc_builtin_strcat(uint64_t dst, uint64_t src) {
    char *d = (char *)(uintptr_t)dst;
    const char *s = (const char *)(uintptr_t)src;
    if (!d) {
        return 0;
    }
    if (!s) {
        s = "";
    }
    char *out = d;
    while (*d) {
        d++;
    }
    while ((*d++ = *s++)) {
    }
    return (uint64_t)(uintptr_t)out;
}

uint64_t hc_builtin_str2i64(uint64_t s) {
    const char *p = (const char *)(uintptr_t)s;
    int64_t v = 0;
    int neg = 0;
    if (!p) {
        return 0;
    }
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p == '-') {
        neg = 1;
        p++;
    } else if (*p == '+') {
        p++;
    }
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
        while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) {
            char c = *p++;
            v <<= 4;
            if (c >= '0' && c <= '9') {
                v |= c - '0';
            } else if (c >= 'a' && c <= 'f') {
                v |= c - 'a' + 10;
            } else {
                v |= c - 'A' + 10;
            }
        }
    } else {
        while (*p >= '0' && *p <= '9') {
            v = v * 10 + (*p - '0');
            p++;
        }
    }
    return (uint64_t)(neg ? -v : v);
}

uint64_t hc_builtin_hashstr(uint64_t s) {
    const unsigned char *p = (const unsigned char *)(uintptr_t)s;
    uint64_t h = 5381;
    if (!p) {
        return h;
    }
    while (*p) {
        h = ((h << 5) + h) + *p++;
    }
    return h;
}

uint64_t hc_builtin_memcpy(uint64_t dst, uint64_t src, uint64_t n) {
    uint8_t *d = (uint8_t *)(uintptr_t)dst;
    const uint8_t *s = (const uint8_t *)(uintptr_t)src;
    if (!d || !s) {
        return dst;
    }
    for (uint64_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

uint64_t hc_builtin_memset(uint64_t dst, uint64_t val, uint64_t n) {
    uint8_t *d = (uint8_t *)(uintptr_t)dst;
    uint8_t v = (uint8_t)(val & 0xff);
    if (!d) {
        return dst;
    }
    for (uint64_t i = 0; i < n; i++) {
        d[i] = v;
    }
    return dst;
}

/* ZealOS-shaped bump heap (LIFO Free of the last chunk; Free(0) resets). */
#define HC_HEAP_BYTES (64u * 1024u)
static uint8_t g_hc_heap[HC_HEAP_BYTES] __attribute__((aligned(16)));
static uint64_t g_hc_heap_top;
static uint64_t g_hc_heap_last;

static void hc_heap_reset(void) {
    g_hc_heap_top = 0;
    g_hc_heap_last = 0;
    hc_msg_que_reset();
    g_hc_menu_nslots = 0;
    g_hc_cdc.color = 0;
    g_hc_cdc.depth_buf = 0;
    g_hc_depth_on = 0;
}

uint64_t hc_builtin_malloc(uint64_t n) {
    if (n == 0) {
        n = 1;
    }
    n = (n + 15u) & ~15u;
    if (g_hc_heap_top + n > HC_HEAP_BYTES) {
        return 0;
    }
    g_hc_heap_last = g_hc_heap_top;
    uint64_t p = (uint64_t)(uintptr_t)(g_hc_heap + g_hc_heap_top);
    g_hc_heap_top += n;
    return p;
}

uint64_t hc_builtin_free(uint64_t p) {
    if (p == 0) {
        hc_heap_reset();
        return 0;
    }
    if (p == (uint64_t)(uintptr_t)(g_hc_heap + g_hc_heap_last)) {
        g_hc_heap_top = g_hc_heap_last;
    }
    return 0;
}

uint64_t hc_builtin_strnew(uint64_t src) {
    const char *s = (const char *)(uintptr_t)src;
    uint64_t n = hc_builtin_strlen(src) + 1;
    uint64_t p = hc_builtin_malloc(n);
    if (!p) {
        return 0;
    }
    if (!s) {
        *(char *)(uintptr_t)p = 0;
        return p;
    }
    hc_builtin_memcpy(p, src, n);
    return p;
}

/* Append decimal / hex of v at *dp (advances). */
static void hc_fmt_i64(char **dp, int64_t v, int hex) {
    char tmp[24];
    int n = 0;
    uint64_t u;
    if (hex) {
        u = (uint64_t)v;
        if (u == 0) {
            tmp[n++] = '0';
        } else {
            while (u) {
                int d = (int)(u & 15u);
                tmp[n++] = (char)(d < 10 ? '0' + d : 'A' + (d - 10));
                u >>= 4;
            }
        }
    } else {
        if (v < 0) {
            *(*dp)++ = '-';
            u = (uint64_t)(-(v + 1)) + 1u;
        } else {
            u = (uint64_t)v;
        }
        if (u == 0) {
            tmp[n++] = '0';
        } else {
            while (u) {
                tmp[n++] = (char)('0' + (u % 10u));
                u /= 10u;
            }
        }
    }
    while (n > 0) {
        *(*dp)++ = tmp[--n];
    }
}

/* Format fmt with up to 5 args (F64-bits/%f or I64/%d) then GrPrint. */
uint64_t hc_builtin_grprintn(uint64_t dc, uint64_t x, uint64_t y, uint64_t fmt, uint64_t argv,
                             uint64_t nargs) {
    char buf[192];
    const char *f = (const char *)(uintptr_t)fmt;
    char *d = buf;
    char *end = buf + sizeof(buf) - 1;
    uint64_t *args = (uint64_t *)(uintptr_t)argv;
    int ai = 0;
    int narg = (int)nargs;
    if (!f) {
        f = "";
    }
    if (narg < 0) {
        narg = 0;
    }
    if (narg > 5) {
        narg = 5;
    }
    while (*f && d < end) {
        if (*f != '%') {
            *d++ = *f++;
            continue;
        }
        f++;
        if (*f == '%') {
            *d++ = '%';
            f++;
            continue;
        }
        if (ai >= narg) {
            break;
        }
        /* Optional width.prec before conversion (Lattice %5.1f). */
        int prec = 1;
        while (*f >= '0' && *f <= '9') {
            f++;
        }
        if (*f == '.') {
            f++;
            prec = 0;
            while (*f >= '0' && *f <= '9') {
                prec = prec * 10 + (*f - '0');
                f++;
            }
        }
        uint64_t av = args[ai++];
        if (*f == 'f' || *f == 'F') {
            f++;
            hc_fmt_f64_bits(&d, av, prec);
        } else if (*f == 'd') {
            f++;
            hc_fmt_i64(&d, (int64_t)av, 0);
        } else if (*f == 'X' || *f == 'x') {
            f++;
            hc_fmt_i64(&d, (int64_t)av, 1);
        } else if (*f == 'c') {
            f++;
            *d++ = (char)(av & 0xff);
        } else if (*f == 's') {
            const char *s = (const char *)(uintptr_t)av;
            f++;
            if (!s) {
                s = "";
            }
            while (*s && d < end) {
                *d++ = *s++;
            }
        } else {
            *d++ = '%';
            if (*f) {
                *d++ = *f++;
            }
        }
    }
    *d = 0;
    return hc_builtin_grprint(dc, x, y, (uint64_t)(uintptr_t)buf);
}

/* ZealOS-shaped CatPrint / StrPrint / MStrPrint — %d %s %c %X %% only; ≤2 args. */
uint64_t hc_builtin_catprint(uint64_t dst, uint64_t fmt, uint64_t a0, uint64_t a1) {
    char *d = (char *)(uintptr_t)dst;
    const char *f = (const char *)(uintptr_t)fmt;
    if (!d) {
        return 0;
    }
    if (!f) {
        f = "";
    }
    while (*d) {
        d++;
    }
    uint64_t args[2] = {a0, a1};
    int ai = 0;
    while (*f) {
        if (*f != '%') {
            *d++ = *f++;
            continue;
        }
        f++;
        if (*f == '%') {
            *d++ = '%';
            f++;
            continue;
        }
        if (ai >= 2) {
            break;
        }
        uint64_t av = args[ai++];
        if (*f == 'd') {
            hc_fmt_i64(&d, (int64_t)av, 0);
            f++;
        } else if (*f == 'X' || *f == 'x') {
            hc_fmt_i64(&d, (int64_t)av, 1);
            f++;
        } else if (*f == 'c') {
            *d++ = (char)(av & 0xff);
            f++;
        } else if (*f == 's') {
            const char *s = (const char *)(uintptr_t)av;
            if (!s) {
                s = "";
            }
            while (*s) {
                *d++ = *s++;
            }
            f++;
        } else {
            *d++ = '%';
            if (*f) {
                *d++ = *f++;
            }
        }
    }
    *d = 0;
    return dst;
}

uint64_t hc_builtin_strprint(uint64_t dst, uint64_t fmt, uint64_t a0, uint64_t a1) {
    char *d = (char *)(uintptr_t)dst;
    if (d) {
        *d = 0;
    }
    return hc_builtin_catprint(dst, fmt, a0, a1);
}

uint64_t hc_builtin_mstrprint(uint64_t fmt, uint64_t a0, uint64_t a1) {
    uint64_t p = hc_builtin_malloc(128);
    if (!p) {
        return 0;
    }
    *(char *)(uintptr_t)p = 0;
    hc_builtin_catprint(p, fmt, a0, a1);
    return p;
}

/* Indirect HolyC/JIT calls (PopUp / Spawn fp_start_addr). */
uint64_t hc_builtin_call0(uint64_t fp) {
    if (!fp) {
        return 0;
    }
    return ((uint64_t(*)(void))(uintptr_t)fp)();
}

uint64_t hc_builtin_call1(uint64_t fp, uint64_t a0) {
    if (!fp) {
        return 0;
    }
    return ((uint64_t(*)(uint64_t))(uintptr_t)fp)(a0);
}

/* 128 × 512-byte RAM disk (fallback; matches PCI/UTM RedSea 64KiB). When virtio-blk is ready, Blk* hit the image. */
#define HC_RAM_BLK_SIZE 512u
#define HC_RAM_N_BLKS   128u
static uint8_t g_ram_dsk[HC_RAM_N_BLKS * HC_RAM_BLK_SIZE] __attribute__((aligned(16)));

uint64_t hc_builtin_blkwrite(uint64_t buf, uint64_t blk, uint64_t count) {
    uint8_t *src = (uint8_t *)(uintptr_t)buf;
    if (!src || count == 0) {
        return 0;
    }
    if (virtio_blk_ready()) {
        if (blk + count > virtio_blk_capacity()) {
            return 0;
        }
        return (uint64_t)virtio_blk_write(src, blk, count);
    }
    if (blk + count > HC_RAM_N_BLKS) {
        return 0;
    }
    hc_builtin_memcpy((uint64_t)(uintptr_t)(g_ram_dsk + blk * HC_RAM_BLK_SIZE), buf,
                      count * HC_RAM_BLK_SIZE);
    return 1;
}

uint64_t hc_builtin_blkread(uint64_t buf, uint64_t blk, uint64_t count) {
    uint8_t *dst = (uint8_t *)(uintptr_t)buf;
    if (!dst || count == 0) {
        return 0;
    }
    if (virtio_blk_ready()) {
        if (blk + count > virtio_blk_capacity()) {
            return 0;
        }
        return (uint64_t)virtio_blk_read(dst, blk, count);
    }
    if (blk + count > HC_RAM_N_BLKS) {
        return 0;
    }
    hc_builtin_memcpy(buf, (uint64_t)(uintptr_t)(g_ram_dsk + blk * HC_RAM_BLK_SIZE),
                      count * HC_RAM_BLK_SIZE);
    return 1;
}

uint64_t hc_builtin_mousex(void) {
    uint32_t x = 0, y = 0;
    if (input_pointer_ready() && g_fb) {
        (void)input_pointer_xy((uint32_t)g_fb_w, (uint32_t)g_fb_h, &x, &y);
    }
    return x;
}

uint64_t hc_builtin_mousey(void) {
    uint32_t x = 0, y = 0;
    if (input_pointer_ready() && g_fb) {
        (void)input_pointer_xy((uint32_t)g_fb_w, (uint32_t)g_fb_h, &x, &y);
    }
    return y;
}

uint64_t hc_builtin_mousebtn(void) {
    if (input_pointer_ready()) {
        input_pointer_poll();
    }
    return (uint64_t)input_pointer_buttons();
}

/* Stable, single-poll tablet snapshot for the ZealC task/window bridge. */
int64_t zc_tablet_sample(void) {
    uint32_t x = 0, y = 0;
    int buttons = 0;
    if (!input_pointer_ready() || !g_fb) {
        return -1;
    }
    int wheel = 0;
    (void)input_pointer_sample((uint32_t)g_fb_w, (uint32_t)g_fb_h,
                               &x, &y, &buttons, &wheel);
    return (int64_t)x | ((int64_t)y << 16) |
           ((int64_t)(buttons & 0xff) << 32) |
           ((int64_t)(wheel & 0xff) << 40) |
           ((int64_t)input_keyboard_ctrl_down() << 48);
}

/* Tablet abs deltas (call MouseDX then MouseDY for a paired sample). */
static int32_t g_mouse_last_x = -1, g_mouse_last_y = -1;
static int32_t g_mouse_dx, g_mouse_dy;
static int g_mouse_have_dy;

static void mouse_delta_refresh(void) {
    uint32_t x = 0, y = 0;
    if (input_pointer_ready() && g_fb) {
        (void)input_pointer_xy((uint32_t)g_fb_w, (uint32_t)g_fb_h, &x, &y);
    }
    if (g_mouse_last_x < 0) {
        g_mouse_dx = 0;
        g_mouse_dy = 0;
    } else {
        g_mouse_dx = (int32_t)x - g_mouse_last_x;
        g_mouse_dy = (int32_t)y - g_mouse_last_y;
    }
    g_mouse_last_x = (int32_t)x;
    g_mouse_last_y = (int32_t)y;
    g_mouse_have_dy = 1;
}

uint64_t hc_builtin_mousedx(void) {
    mouse_delta_refresh();
    return (uint64_t)(int64_t)g_mouse_dx;
}

uint64_t hc_builtin_mousedy(void) {
    if (!g_mouse_have_dy) {
        mouse_delta_refresh();
    }
    g_mouse_have_dy = 0;
    return (uint64_t)(int64_t)g_mouse_dy;
}

uint64_t hc_builtin_fbw(void) {
    return g_fb_w;
}

uint64_t hc_builtin_fbh(void) {
    return g_fb_h;
}

static uint64_t g_hc_rand = 0x9e3779b97f4a7c15ull;

uint64_t hc_builtin_rand(void) {
    /* xorshift64* */
    uint64_t x = g_hc_rand;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    g_hc_rand = x;
    return x * 0x2545F4914F6CDD1Dull;
}

/* Peek only — do not update MessageGet edge/xy state (else edges are lost). */
static int hc_input_pending_for_sleep(void) {
    if (g_hc_msg_head != g_hc_msg_tail) {
        return 1;
    }
    if (input_keyboard_ready()) {
        if (input_keyboard_msg_pending()) {
            return 1;
        }
    } else if (g_uart) {
        volatile uint32_t *r = (volatile uint32_t *)(uintptr_t)g_uart;
        /* PL011_FR_RXFE — peek without consume. */
        if (!(r[PL011_FR / 4] & (1u << 4))) {
            return 1;
        }
    }
    if (input_pointer_ready() && g_fb) {
        uint32_t x = 0, y = 0;
        int btns = input_pointer_buttons();
        (void)input_pointer_xy((uint32_t)g_fb_w, (uint32_t)g_fb_h, &x, &y);
        int btn = btns & 1;
        int rbtn = (btns & 2) ? 1 : 0;
        if (btn != g_hc_msg_btn_prev || rbtn != g_hc_msg_rbtn_prev) {
            return 1;
        }
        if (g_hc_msg_have_xy && (x != g_hc_msg_last_x || y != g_hc_msg_last_y)) {
            return 1;
        }
    }
    return 0;
}

uint64_t hc_builtin_sleep(uint64_t ms) {
    uint64_t frq = 0, t0 = 0, now = 0, next_poll = 0;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frq));
    if (frq == 0) {
        frq = 62500000;
    }
    if (ms > 5000) {
        ms = 5000;
    }
    if (ms == 0) {
        return 0;
    }
    uint64_t ticks = (frq / 1000ull) * ms;
    uint64_t poll_every = frq / 1000ull; /* ~1 ms between input peeks */
    if (poll_every == 0) {
        poll_every = 1;
    }
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
    next_poll = t0;
    do {
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        if (now >= next_poll) {
            if (hc_input_pending_for_sleep()) {
                break;
            }
            next_poll = now + poll_every;
        }
    } while (now - t0 < ticks);
    return ms;
}

uint64_t hc_builtin_cnt(void) {
    uint64_t t = 0;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t));
    return t;
}

uint64_t hc_builtin_cntfrq(void) {
    uint64_t f = 0;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(f));
    if (f == 0) {
        f = 62500000;
    }
    return f;
}

static int hc_run_bc(const uint8_t *bc, size_t n, uint64_t *out) {
    g_hc_fp = 0;
    /* Frames start past file-scope globals. */
    g_hc_alloc = g_hc_glob_words;
    for (uint64_t i = 0; i < g_hc_glob_words && i < HC_IR_MEM_WORDS; i++) {
        g_hc_mem[i] = 0;
    }
    int bytes = hc_ir_compile(bc, n, g_jit_buf, sizeof(g_jit_buf) / 4);
    if (bytes < 0) {
        return bytes;
    }
    jit_icache_flush(g_jit_buf, (size_t)bytes);
    *out = ((uint64_t(*)(void))(void *)g_jit_buf)();
    return 0;
}

static int hc_jit_into(const uint8_t *bc, size_t n, uint32_t *out, size_t cap) {
    int bytes = hc_ir_compile(bc, n, out, cap);
    if (bytes < 0) {
        return bytes;
    }
    jit_icache_flush(out, (size_t)bytes);
    return bytes;
}

static int hc_run_src(const char *src, uint64_t *out);
static int hc_run_src_ex(const char *src, uint64_t *out, int popup_live);

/* Forward decl: RedSea load used by #include expansion (defined below). */
static int rs_load_file(const char *name, char *dst, size_t cap, size_t *out_len);
static size_t str_len(const char *s);

static char g_hc_src_exp[16384];
static char g_hc_zc_src[16384]; /* M163: DiskLat.ZC grew past 8K */
static char g_hc_src_mid[16384];

#define HC_MAX_MACROS 24
#define HC_MACRO_NAME 32
#define HC_MACRO_BODY 96

struct hc_macro {
    char name[HC_MACRO_NAME];
    char body[HC_MACRO_BODY];
    int nlen;
    int blen;
};

static struct hc_macro g_hc_macros[HC_MAX_MACROS];
static int g_hc_nmacros;

static int hc_mac_ident_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

static int hc_mac_ident_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/* If p is a #define line, record macro and return ptr past the line; else NULL. */
static const char *hc_parse_define(const char *p) {
    const char *q = p;
    while (*q == ' ' || *q == '\t') {
        q++;
    }
    if (*q != '#') {
        return NULL;
    }
    q++;
    while (*q == ' ' || *q == '\t') {
        q++;
    }
    if (!(q[0] == 'd' && q[1] == 'e' && q[2] == 'f' && q[3] == 'i' && q[4] == 'n' &&
          q[5] == 'e')) {
        return NULL;
    }
    q += 6;
    while (*q == ' ' || *q == '\t') {
        q++;
    }
    if (!hc_mac_ident_start(*q) || g_hc_nmacros >= HC_MAX_MACROS) {
        return NULL;
    }
    const char *ns = q;
    while (hc_mac_ident_char(*q)) {
        q++;
    }
    int nlen = (int)(q - ns);
    if (nlen <= 0 || nlen >= HC_MACRO_NAME) {
        return NULL;
    }
    while (*q == ' ' || *q == '\t') {
        q++;
    }
    const char *bs = q;
    while (*q && *q != '\n') {
        q++;
    }
    const char *be = q;
    while (be > bs && (be[-1] == ' ' || be[-1] == '\t' || be[-1] == '\r')) {
        be--;
    }
    int blen = (int)(be - bs);
    if (blen <= 0 || blen >= HC_MACRO_BODY) {
        return NULL;
    }
    struct hc_macro *m = &g_hc_macros[g_hc_nmacros++];
    for (int i = 0; i < nlen; i++) {
        m->name[i] = ns[i];
    }
    m->name[nlen] = 0;
    m->nlen = nlen;
    for (int i = 0; i < blen; i++) {
        m->body[i] = bs[i];
    }
    m->body[blen] = 0;
    m->blen = blen;
    if (*q == '\n') {
        q++;
    }
    return q;
}

static int hc_find_macro(const char *s, int len) {
    for (int i = 0; i < g_hc_nmacros; i++) {
        if (g_hc_macros[i].nlen == len) {
            int j;
            for (j = 0; j < len; j++) {
                if (g_hc_macros[i].name[j] != s[j]) {
                    break;
                }
            }
            if (j == len) {
                return i;
            }
        }
    }
    return -1;
}

/* Object-like #define expansion (Lattice TURTLE_SIZE / ANGLES). */
static int hc_expand_macros(const char *src, char *dst, size_t cap) {
    char tmp_a[16384];
    char tmp_b[16384];
    char *outbuf = tmp_a;
    char *inbuf = tmp_b;
    size_t slen = 0;
    while (src[slen]) {
        slen++;
        if (slen + 1 >= sizeof(tmp_a)) {
            return -1;
        }
    }
    for (size_t i = 0; i <= slen; i++) {
        inbuf[i] = src[i];
    }
    for (int pass = 0; pass < 8; pass++) {
        size_t n = 0;
        const char *p = inbuf;
        int changed = 0;
        int in_str = 0;
        while (*p) {
            if (*p == '"') {
                in_str = !in_str;
                if (n + 1 >= sizeof(tmp_a)) {
                    return -1;
                }
                outbuf[n++] = *p++;
                continue;
            }
            if (!in_str && hc_mac_ident_start(*p)) {
                const char *s = p;
                while (hc_mac_ident_char(*p)) {
                    p++;
                }
                int len = (int)(p - s);
                int mi = hc_find_macro(s, len);
                if (mi >= 0) {
                    struct hc_macro *m = &g_hc_macros[mi];
                    if (n + (size_t)m->blen + 1 >= sizeof(tmp_a)) {
                        return -1;
                    }
                    for (int i = 0; i < m->blen; i++) {
                        outbuf[n++] = m->body[i];
                    }
                    changed = 1;
                    continue;
                }
                if (n + (size_t)len + 1 >= sizeof(tmp_a)) {
                    return -1;
                }
                for (int i = 0; i < len; i++) {
                    outbuf[n++] = s[i];
                }
                continue;
            }
            if (n + 1 >= sizeof(tmp_a)) {
                return -1;
            }
            outbuf[n++] = *p++;
        }
        outbuf[n] = 0;
        if (!changed) {
            if (n + 1 > cap) {
                return -1;
            }
            for (size_t i = 0; i <= n; i++) {
                dst[i] = outbuf[i];
            }
            return 0;
        }
        /* swap for next pass */
        char *swap = inbuf;
        inbuf = outbuf;
        outbuf = swap;
    }
    return -1;
}

/* If p is an #include line, copy name and return ptr past the line; else NULL. */
static const char *hc_parse_include(const char *p, char *name, size_t nmax) {
    const char *q = p;
    char end;
    size_t i = 0;

    while (*q == ' ' || *q == '\t') {
        q++;
    }
    if (*q != '#') {
        return NULL;
    }
    q++;
    while (*q == ' ' || *q == '\t') {
        q++;
    }
    if (!(q[0] == 'i' && q[1] == 'n' && q[2] == 'c' && q[3] == 'l' && q[4] == 'u' &&
          q[5] == 'd' && q[6] == 'e')) {
        return NULL;
    }
    q += 7;
    while (*q == ' ' || *q == '\t') {
        q++;
    }
    if (*q == '"') {
        end = '"';
    } else if (*q == '<') {
        end = '>';
    } else {
        return NULL;
    }
    q++;
    while (*q && *q != end && *q != '\n' && i + 1 < nmax) {
        name[i++] = *q++;
    }
    if (*q != end || i == 0) {
        return NULL;
    }
    name[i] = 0;
    q++;
    while (*q && *q != '\n') {
        q++;
    }
    if (*q == '\n') {
        q++;
    }
    return q;
}

static int hc_expand_includes_r(const char *src, char *dst, size_t cap, size_t *out_n, int depth) {
    const char *p = src;
    char iname[40];
    char ibuf[4096];

    if (!src || !dst || !out_n || depth > 4) {
        return -1;
    }
    while (*p) {
        if (p == src || p[-1] == '\n') {
            const char *dafter = hc_parse_define(p);
            if (dafter) {
                p = dafter;
                continue;
            }
            const char *after = hc_parse_include(p, iname, sizeof(iname));
            if (after) {
                size_t n = 0;
                size_t sub = 0;
                if (rs_load_file(iname, ibuf, sizeof(ibuf), &n) != 0) {
                    return -2;
                }
                if (hc_expand_includes_r(ibuf, dst + *out_n, cap - *out_n, &sub, depth + 1) != 0) {
                    return -3;
                }
                *out_n += sub;
                if (*out_n + 1 >= cap) {
                    return -4;
                }
                p = after;
                continue;
            }
        }
        if (*out_n + 1 >= cap) {
            return -4;
        }
        dst[(*out_n)++] = *p++;
    }
    dst[*out_n] = 0;
    return 0;
}

static int hc_expand_includes(const char *src, char *dst, size_t cap) {
    size_t n = 0;
    if (!src || !dst || cap < 2) {
        return -1;
    }
    g_hc_nmacros = 0;
    if (hc_expand_includes_r(src, g_hc_src_mid, sizeof(g_hc_src_mid), &n, 0) != 0) {
        return -1;
    }
    if (hc_expand_macros(g_hc_src_mid, dst, cap) != 0) {
        return -1;
    }
    /* M192: UTF-8 θ (U+03B8=CE B8) / π (U+03C0=CF 80) → TempleOS 0xE9 / 0xE3. */
    {
        char *r = dst;
        char *w = dst;
        while (*r) {
            unsigned char a = (unsigned char)r[0];
            unsigned char b = (unsigned char)r[1];
            if (a == 0xceu && b == 0xb8u) {
                *w++ = (char)0xe9;
                r += 2;
            } else if (a == 0xcfu && b == 0x80u) {
                *w++ = (char)0xe3;
                r += 2;
            } else {
                *w++ = *r++;
            }
        }
        *w = 0;
    }
    return 0;
}

static int hc_run_src(const char *src, uint64_t *out) {
    return hc_run_src_ex(src, out, 0);
}

static int hc_run_src_ex(const char *src, uint64_t *out, int popup_live) {
    uint8_t bc[8192]; /* was 4096 — Lattice angles[35] brace init */
    int rc;
    g_hc_popup_i = 0;
    g_hc_popup_mid = 14;
    g_hc_popup_edge = 0;
    g_hc_popup_mid_title[0] = 0;
    g_hc_popup_edge_title[0] = 0;
    g_hc_popup_live = popup_live ? 1 : 0;
    g_hc_fs_draw_it = 0; /* M153: no stale Fs->draw_it across demos */
    hc_heap_reset();
    if (hc_expand_includes(src, g_hc_src_exp, sizeof(g_hc_src_exp)) != 0) {
        g_hc_popup_live = 0;
        return -50;
    }
    int n = hc_front_compile_ex(g_hc_src_exp, bc, sizeof(bc), &g_fn_tbl, g_fn_code, hc_jit_into);
    if (n < 0) {
        g_hc_popup_live = 0;
        return n;
    }
    rc = hc_run_bc(bc, (size_t)n, out);
    g_hc_fs_draw_it = 0;
    g_hc_popup_live = 0;
    /* M203: demos that skip shell_fb_ready still drop depth for the next paint. */
    if (g_hc_depth_on) {
        hc_depth_map_clear();
        g_hc_depth_on = 0;
    }
    g_hc_cdc.depth_buf = 0;
    return rc;
}

/* M155/M159/M190/M191/M193/M196: DiskLat/StockLat → live play — drop MsgQuePush scripts.
 * smoke_esc: inject one CH_ESC after Cls so check-serial ends.
 * live (!smoke_esc): inject controls GrPrint after Cls; strip idle TurtleMove (M193, stock). */
static int hc_lattice_play_src(const char *in, char *out, size_t cap, int smoke_esc) {
    size_t o = 0;
    const char *p = in;
    int saw_inj = 0;
    /* Shared live chrome; 0xE9 = θ in arrows line (M191). */
    static const char live_hints[] =
        "\n\t\tGrPrint(dc, 0, 16, \"Esc=exit Enter=restart Space=step c=color +/-=w\");"
        "\n\t\tGrPrint(dc, 0, 24, \"L-click=place R-drag=aim e=ends\");"
        "\n\t\tGrPrint(dc, 0, 32, \"arrows=d\xe9/speed 0-9=layer\");";
    if (!in || !out || cap < 8) {
        return -1;
    }
    while (*p) {
        if (p[0] == 'M' && p[1] == 's' && p[2] == 'g' && p[3] == 'Q' && p[4] == 'u' &&
            p[5] == 'e' && p[6] == 'P' && p[7] == 'u' && p[8] == 's' && p[9] == 'h') {
            p += 10;
            while (*p && *p != ';') {
                p++;
            }
            if (*p == ';') {
                p++;
            }
            while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
                p++;
            }
            continue;
        }
        /* M193: live stockplay — drop DrawIt preview TurtleMove (keep Space Step). */
        if (!smoke_esc && p[0] == 'T' && p[1] == 'u' && p[2] == 'r' && p[3] == 't' &&
            p[4] == 'l' && p[5] == 'e' && p[6] == 'M' && p[7] == 'o' && p[8] == 'v' &&
            p[9] == 'e' && p[10] == '(') {
            const char *q = p + 11;
            /* TurtleMove(dc, &t2, RED, LTRED); — DrawIt only */
            if (q[0] == 'd' && q[1] == 'c' && q[2] == ',' && q[3] == ' ' && q[4] == '&' &&
                q[5] == 't' && q[6] == '2' && q[7] == ',' && q[8] == ' ' && q[9] == 'R' &&
                q[10] == 'E' && q[11] == 'D' && q[12] == ',' && q[13] == ' ' && q[14] == 'L' &&
                q[15] == 'T' && q[16] == 'R' && q[17] == 'E' && q[18] == 'D' && q[19] == ')') {
                p = q + 20;
                if (*p == ';') {
                    p++;
                }
                while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
                    p++;
                }
                continue;
            }
        }
        if (o + 1 >= cap) {
            return -1;
        }
        out[o++] = *p++;
        if (saw_inj) {
            continue;
        }
        /* DiskLat + StockLat (M196): Cls(0); */
        if (o >= 7 && out[o - 7] == 'C' && out[o - 6] == 'l' && out[o - 5] == 's' &&
            out[o - 4] == '(' && out[o - 3] == '0' && out[o - 2] == ')' && out[o - 1] == ';') {
            const char *inj;
            size_t el = 0;
            if (smoke_esc) {
                inj = "\n\t\tMsgQuePush(MESSAGE_KEY_DOWN, CH_ESC, 0);";
            } else {
                inj = live_hints;
            }
            while (inj[el]) {
                el++;
            }
            if (o + el >= cap) {
                return -1;
            }
            for (size_t i = 0; i < el; i++) {
                out[o++] = inj[i];
            }
            saw_inj = 1;
        }
    }
    if (o >= cap) {
        return -1;
    }
    out[o] = 0;
    return 0;
}

/* I64 Abs(I64 x) { if (x < 0) return -x; return x; } */
static int hc_build_abs(uint8_t *bc, size_t cap, int64_t x) {
    size_t n = 0;
    if (cap < 48) {
        return -1;
    }
    n += hc_pack_imm64(bc + n, (uint64_t)x);
    n += hc_pack_u8op(bc + n, HC_ST_LOCAL, 0);
    n += hc_pack_u8op(bc + n, HC_LD_LOCAL, 0);
    n += hc_pack_imm64(bc + n, 0);
    bc[n++] = HC_LT;
    n += hc_pack_u8op(bc + n, HC_JNZ, 1); /* x < 0 → L1 */
    n += hc_pack_u8op(bc + n, HC_LD_LOCAL, 0);
    bc[n++] = HC_RET;
    n += hc_pack_u8op(bc + n, HC_LABEL, 1);
    n += hc_pack_u8op(bc + n, HC_LD_LOCAL, 0);
    bc[n++] = HC_NEG;
    bc[n++] = HC_RET;
    bc[n++] = HC_END;
    return (int)n;
}

/* ---- expression → IR (shell) ---- */
struct hc_expr {
    const char *p;
    uint8_t *bc;
    size_t n;
    size_t cap;
    int err;
};

static void expr_skip(struct hc_expr *e) {
    while (*e->p == ' ' || *e->p == '\t') {
        e->p++;
    }
}

static int expr_emit(struct hc_expr *e, uint8_t op) {
    if (e->n >= e->cap) {
        e->err = 1;
        return -1;
    }
    e->bc[e->n++] = op;
    return 0;
}

static int expr_imm(struct hc_expr *e, uint64_t v) {
    if (e->n + 9 > e->cap) {
        e->err = 1;
        return -1;
    }
    e->n += hc_pack_imm64(e->bc + e->n, v);
    return 0;
}

static int expr_parse_add(struct hc_expr *e);

static int expr_parse_prim(struct hc_expr *e) {
    expr_skip(e);
    if (*e->p == '(') {
        e->p++;
        if (expr_parse_add(e) < 0) {
            return -1;
        }
        expr_skip(e);
        if (*e->p != ')') {
            e->err = 1;
            return -1;
        }
        e->p++;
        return 0;
    }
    if (*e->p == '-') {
        e->p++;
        if (expr_parse_prim(e) < 0) {
            return -1;
        }
        return expr_emit(e, HC_NEG);
    }
    if (*e->p >= '0' && *e->p <= '9') {
        uint64_t v = 0;
        while (*e->p >= '0' && *e->p <= '9') {
            v = v * 10 + (uint64_t)(*e->p - '0');
            e->p++;
        }
        return expr_imm(e, v);
    }
    e->err = 1;
    return -1;
}

static int expr_parse_mul(struct hc_expr *e) {
    if (expr_parse_prim(e) < 0) {
        return -1;
    }
    for (;;) {
        expr_skip(e);
        char c = *e->p;
        if (c != '*') {
            break;
        }
        e->p++;
        if (expr_parse_prim(e) < 0) {
            return -1;
        }
        if (expr_emit(e, HC_MUL) < 0) {
            return -1;
        }
    }
    return 0;
}

static int expr_parse_add(struct hc_expr *e) {
    if (expr_parse_mul(e) < 0) {
        return -1;
    }
    for (;;) {
        expr_skip(e);
        char c = *e->p;
        if (c != '+' && c != '-') {
            break;
        }
        e->p++;
        if (expr_parse_mul(e) < 0) {
            return -1;
        }
        if (expr_emit(e, c == '+' ? HC_ADD : HC_SUB) < 0) {
            return -1;
        }
    }
    return 0;
}

static int expr_compile_run(const char *s, uint64_t *out) {
    uint8_t bc[128];
    struct hc_expr e = { .p = s, .bc = bc, .n = 0, .cap = sizeof(bc), .err = 0 };
    if (expr_parse_add(&e) < 0 || e.err) {
        return -1;
    }
    expr_skip(&e);
    if (*e.p != '\0') {
        return -1;
    }
    if (e.n + 2 > e.cap) {
        return -1;
    }
    bc[e.n++] = HC_RET;
    bc[e.n++] = HC_END;
    return hc_run_bc(bc, e.n, out);
}

static int streq(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static int str_startswith(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

static size_t str_len(const char *s) {
    size_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

static char str_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + 32);
    }
    return c;
}

/* Case-insensitive equality (RedSea names / runzc). */
static int str_ieq(const char *a, const char *b) {
    if (!a || !b) {
        return 0;
    }
    while (*a && *b) {
        if (str_lower(*a) != str_lower(*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return !*a && !*b;
}

static const char *skip_ws(const char *s) {
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    return s;
}

/* Read boot + root dir; returns 0 and fills buffers. */
static int rs_read_root(uint8_t *br, uint8_t *dir, uint64_t *out_root, uint64_t *out_sects) {
    const int64_t *w;
    uint64_t root, sects = 16;
    unsigned i;

    for (i = 0; i < 512; i++) {
        br[i] = 0;
        dir[i] = 0;
    }
    if (!hc_builtin_blkread((uint64_t)(uintptr_t)br, 0, 1)) {
        return -1;
    }
    if (br[3] != 0x88 || br[510] != 0x55 || br[511] != 0xAA) {
        return -2;
    }
    w = (const int64_t *)(br + 8);
    root = (uint64_t)w[2];
    if (w[1] > 0 && (uint64_t)w[1] <= 128) {
        sects = (uint64_t)w[1];
    }
    if (!root) {
        return -3;
    }
    if (!hc_builtin_blkread((uint64_t)(uintptr_t)dir, root, 1)) {
        return -4;
    }
    if (out_root) {
        *out_root = root;
    }
    if (out_sects) {
        *out_sects = sects;
    }
    return 0;
}

/* Case-insensitive find; returns slot or -1. */
static int rs_find_slot(uint8_t *dir, const char *name, uint64_t *out_blk, uint64_t *out_len) {
    unsigned i;
    const int64_t *w;

    for (i = 0; i < 8; i++) {
        uint8_t *e = dir + i * 64;
        if (!e[2]) {
            continue;
        }
        if (str_ieq((const char *)(e + 2), name)) {
            w = (const int64_t *)(e + 40);
            if (out_blk) {
                *out_blk = (uint64_t)w[0];
            }
            if (out_len) {
                *out_len = (uint64_t)w[1];
            }
            return (int)i;
        }
    }
    return -1;
}

/* Contiguous RedSea clusters: load root file into dst (case-insensitive). */
static int rs_load_file(const char *name, char *dst, size_t cap, size_t *out_len) {
    uint8_t br[512];
    uint8_t dir[512];
    uint8_t data[512];
    uint64_t root = 0;
    uint64_t fblk = 0, sects = 0;
    uint64_t flen = 0;
    uint64_t off;
    uint64_t blk;
    unsigned i;

    if (!name || !dst || cap < 2) {
        return -1;
    }
    if (rs_read_root(br, dir, &root, &sects) != 0) {
        return -2;
    }
    if (rs_find_slot(dir, name, &fblk, &flen) < 0) {
        return -6;
    }
    if (fblk < 3 || flen == 0 || flen >= cap ||
        fblk > sects || (flen + 511) / 512 > sects - fblk) {
        return -6;
    }
    off = 0;
    blk = fblk;
    while (off < flen) {
        uint64_t chunk = flen - off;
        if (chunk > 512) {
            chunk = 512;
        }
        for (i = 0; i < 512; i++) {
            data[i] = 0;
        }
        if (!hc_builtin_blkread((uint64_t)(uintptr_t)data, blk, 1)) {
            return -7;
        }
        for (i = 0; i < (unsigned)chunk; i++) {
            dst[off + i] = (char)data[i];
        }
        off += chunk;
        blk++;
    }
    dst[flen] = 0;
    if (out_len) {
        *out_len = (size_t)flen;
    }
    return 0;
}

/* Host RSFmt — boot+bitmap; free data clus 2..sects-1 (M145). */
static int rs_fmt_host(uint64_t sects, int64_t uid) {
    uint8_t br[512];
    uint8_t map[512];
    uint8_t z[512];
    int64_t *w;
    unsigned i;

    if (sects < 4 || sects > 128) {
        return -1;
    }
    for (i = 0; i < 512; i++) {
        br[i] = 0;
        map[i] = 0;
        z[i] = 0;
    }
    br[3] = 0x88;
    w = (int64_t *)(br + 8);
    w[0] = 0;
    w[1] = (int64_t)sects;
    w[2] = 2; /* root clus */
    w[3] = 1; /* bitmap sects */
    w[4] = uid;
    br[510] = 0x55;
    br[511] = 0xAA;
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)br, 0, 1)) {
        return -2;
    }
    map[0] = 0x07; /* clus 0 boot, 1 bitmap, 2 root reserved */
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)map, 1, 1)) {
        return -3;
    }
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)z, 2, 1)) {
        return -4;
    }
    for (i = 3; i < (unsigned)sects; i++) {
        if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)z, i, 1)) {
            return -5;
        }
    }
    return 0;
}

/* Write or replace one contiguous root file. A growing overwrite reserves a
 * different run, writes its bytes, then switches the directory entry. It never
 * extends a file into an unallocated neighbour's blocks. Smaller overwrites
 * use the existing run; they release trailing clusters after updating length. */
static int rs_put_file(const char *name, const char *src, size_t len) {
    uint8_t br[512], map[512], old_map[512], dir[512], data[512];
    uint64_t root = 0, sects = 0, fblk = 0, old_blk = 0, old_len = 0;
    uint64_t nclus, old_nclus = 0;
    int slot = -1, existing, new_run;
    unsigned i, j;
    int64_t *w;

    if (!name || !src || !len || str_len(name) > 37 || len > 65536) {
        return -1;
    }
    nclus = (len + 511) / 512;
    if (rs_read_root(br, dir, &root, &sects) != 0 || sects < 4 || sects > 128) {
        return -2;
    }
    if (!hc_builtin_blkread((uint64_t)(uintptr_t)map, 1, 1)) {
        return -7;
    }
    for (i = 0; i < 512; i++) old_map[i] = map[i];
    existing = rs_find_slot(dir, name, &old_blk, &old_len);
    if (existing >= 0) {
        old_nclus = (old_len + 511) / 512;
        if (!old_len || old_blk < 3 || old_blk + old_nclus > sects) return -3;
        slot = existing;
    } else {
        for (i = 0; i < 8; i++) if (!dir[i * 64 + 2]) { slot = (int)i; break; }
        if (slot < 0) return -6;
    }
    new_run = existing < 0 || nclus > old_nclus;
    if (new_run) {
        int found = 0;
        for (i = 3; i + nclus <= sects; i++) {
            int free_run = 1;
            for (j = 0; j < nclus; j++) {
                unsigned b = i + j;
                if (map[b >> 3] & (1u << (b & 7))) { free_run = 0; break; }
            }
            if (free_run) { fblk = i; found = 1; break; }
        }
        if (!found) return -8;
        for (j = 0; j < nclus; j++) {
            unsigned b = (unsigned)fblk + j;
            map[b >> 3] = (uint8_t)(map[b >> 3] | (1u << (b & 7)));
        }
    } else {
        fblk = old_blk;
    }

    for (j = 0; j < nclus; j++) {
        size_t off = (size_t)j * 512, chunk = len - off;
        if (chunk > 512) chunk = 512;
        for (i = 0; i < 512; i++) data[i] = 0;
        for (i = 0; i < chunk; i++) data[i] = (uint8_t)src[off + i];
        if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)data, fblk + j, 1)) return -12;
    }
    if (new_run && !hc_builtin_blkwrite((uint64_t)(uintptr_t)map, 1, 1)) return -9;
    {
        uint8_t *e = dir + (unsigned)slot * 64;
        if (existing < 0) {
            size_t nlen = str_len(name);
            for (i = 0; i < 64; i++) e[i] = 0;
            e[0] = 0x20; e[1] = 0x04;
            for (i = 0; i <= nlen; i++) e[2 + i] = (uint8_t)name[i];
        }
        w = (int64_t *)(e + 40);
        w[0] = (int64_t)fblk;
        w[1] = (int64_t)len;
        w[2] = 0;
    }
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)dir, root, 1)) {
        if (new_run) (void)hc_builtin_blkwrite((uint64_t)(uintptr_t)old_map, 1, 1);
        return -10;
    }
    if (existing >= 0 && (new_run || nclus < old_nclus)) {
        uint64_t first = new_run ? old_blk : old_blk + nclus;
        uint64_t count = new_run ? old_nclus : old_nclus - nclus;
        for (j = 0; j < count; j++) {
            unsigned b = (unsigned)(first + j);
            map[b >> 3] = (uint8_t)(map[b >> 3] & ~(1u << (b & 7)));
        }
        if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)map, 1, 1)) return -13;
    }
    return 0;
}

static int rs_file_exists(const char *name, uint64_t *out_len) {
    uint8_t br[512];
    uint8_t dir[512];
    uint64_t root = 0;
    uint64_t flen = 0;

    if (!name || rs_read_root(br, dir, &root, NULL) != 0) {
        return 0;
    }
    if (rs_find_slot(dir, name, NULL, &flen) < 0) {
        return 0;
    }
    if (out_len) {
        *out_len = flen;
    }
    return 1;
}

#ifndef ZEAL_PI_DIAG
/* The native compiler's disk: provider must never fall back to the boot RAM disk. */
static int zc_read_redsea(const char *name, char *dst, size_t cap, size_t *out_len) {
    struct zss_entry entry;
    int found = zss_find(name, &entry, NULL, NULL);
    if (found == 1) return zss_read(&entry, dst, cap, out_len);
    if (found == -3) return -3; /* Native tombstone masks the bundled archive. */
    if (found == -1) return -1;
    /* The old eight-entry RedSea catalog stays readable during migration. */
    uint64_t len = 0;
    if (!virtio_blk_ready() || str_len(name) > 37 ||
        !rs_file_exists(name, &len) || !len || len > 65536)
        return -1;
    if (out_len) *out_len = (size_t)len;
    if (!dst) return 0;
    return rs_load_file(name, dst, cap, out_len);
}

/* Enumerate readable files from the legacy eight-slot RedSea root. The
 * source archive/native catalog remains the primary listing; this adapter
 * exposes only root files that the same FileRead path can still resolve. */
int zeal_redsea_file_at(uint64_t index, char *dst, size_t cap,
                        size_t *out_size) {
    uint8_t br[512], dir[512];
    uint64_t sectors = 0;
    uint64_t visible = 0;
    unsigned i;

    if (!virtio_blk_ready() || !dst || cap < 2 ||
        rs_read_root(br, dir, NULL, &sectors) != 0)
        return 0;
    for (i = 0; i < 8; i++) {
        const uint8_t *entry = dir + i * 64;
        const int64_t *words;
        size_t name_len = 0;
        uint64_t first, length, clusters;
        struct zss_entry native_entry;
        int native_status;

        if (!entry[2] || (entry[0] & 0x10))
            continue;
        while (name_len < 38 && entry[2 + name_len])
            name_len++;
        if (!name_len || name_len >= 38)
            continue;
        /* A RedSea root entry is a filename, never a nested path. */
        for (size_t j = 0; j < name_len; j++)
            if (entry[2 + j] == '/' || entry[2 + j] == '\\')
                name_len = 0;
        if (!name_len)
            continue;
        words = (const int64_t *)(entry + 40);
        first = (uint64_t)words[0];
        length = (uint64_t)words[1];
        if (!length || length > 65536 || first < 3 || first >= sectors)
            continue;
        clusters = (length + 511) / 512;
        if (clusters > sectors - first)
            continue;
        native_status = zss_find((const char *)(entry + 2), &native_entry,
                                 NULL, NULL);
        if (native_status == -3)
            continue;
        if (visible++ != index)
            continue;
        if (name_len + 1 > cap)
            return 0;
        for (size_t j = 0; j < name_len; j++)
            dst[j] = (char)entry[2 + j];
        dst[name_len] = 0;
        if (out_size)
            *out_size = (size_t)length;
        return 1;
    }
    return 0;
}

static int zc_write_source(const char *path, const char *src, size_t len) {
    const char *name = path;
    if (!path || !src || !len || len > ZSS_MAX_FILE)
        return -1;
    if (str_startswith(name, "disk:"))
        name += 5;
    while (*name == '/')
        name++;
    if (!zss_name_ok(name, NULL))
        return -1;
    if (zss_available())
        return zss_put(name, src, len);
    /* Legacy RedSea only supports a root filename without a directory. */
    for (const char *p = name; *p; p++)
        if (*p == '/') return -1;
    if (len > 65536 || !virtio_blk_ready())
        return -1;
    return rs_put_file(name, src, len);
}
#endif

static int rs_put_file_if_absent(const char *name, const char *src, size_t len) {
    uint64_t elen = 0;
    if (rs_file_exists(name, &elen) && elen > 0) {
        return 0;
    }
    return rs_put_file(name, src, len);
}

/* Free contiguous clusters then clear dir slot (M37). */
static int rs_del_file(const char *name) {
    uint8_t br[512];
    uint8_t map[512];
    uint8_t dir[512];
    uint64_t root = 0;
    uint64_t fblk = 0;
    uint64_t flen = 0;
    uint64_t nclus;
    int slot;
    unsigned i, j;

    if (!name || str_ieq(name, ".") || str_ieq(name, "..")) {
        return -1;
    }
    if (rs_read_root(br, dir, &root, NULL) != 0) {
        return -2;
    }
    slot = rs_find_slot(dir, name, &fblk, &flen);
    if (slot < 0 || !fblk) {
        return -3;
    }
    nclus = (flen + 511) / 512;
    if (nclus < 1) {
        nclus = 1;
    }
    for (i = 0; i < 512; i++) {
        map[i] = 0;
    }
    if (!hc_builtin_blkread((uint64_t)(uintptr_t)map, 1, 1)) {
        return -4;
    }
    for (j = 0; j < (unsigned)nclus; j++) {
        unsigned b = (unsigned)(fblk + j);
        unsigned m = 1u << (b & 7);
        map[b >> 3] = (uint8_t)(map[b >> 3] & (uint8_t)~m);
    }
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)map, 1, 1)) {
        return -5;
    }
    {
        uint8_t *e = dir + (unsigned)slot * 64;
        for (i = 0; i < 64; i++) {
            e[i] = 0;
        }
    }
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)dir, root, 1)) {
        return -6;
    }
    return 0;
}

/* Rename root entry (case-insensitive find; new name stored as given). */
static int rs_rename_file(const char *oldn, const char *newn) {
    uint8_t br[512];
    uint8_t dir[512];
    uint64_t root = 0;
    int slot;
    size_t nlen;
    unsigned i;

    if (!oldn || !newn || str_ieq(oldn, ".") || str_ieq(oldn, "..")) {
        return -1;
    }
    nlen = str_len(newn);
    if (nlen == 0 || nlen > 37) {
        return -1;
    }
    if (rs_read_root(br, dir, &root, NULL) != 0) {
        return -2;
    }
    slot = rs_find_slot(dir, oldn, NULL, NULL);
    if (slot < 0) {
        return -3;
    }
    if (!str_ieq(oldn, newn) && rs_find_slot(dir, newn, NULL, NULL) >= 0) {
        return -4; /* target exists */
    }
    {
        uint8_t *e = dir + (unsigned)slot * 64;
        for (i = 0; i < 38; i++) {
            e[2 + i] = 0;
        }
        for (i = 0; i <= nlen; i++) {
            e[2 + i] = (uint8_t)newn[i];
        }
    }
    if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)dir, root, 1)) {
        return -5;
    }
    return 0;
}

static int rs_dir_print(void) {
    uint8_t br[512];
    uint8_t dir[512];
    uint64_t root = 0;
    unsigned i, n = 0;
    const int64_t *w;

    if (rs_read_root(br, dir, &root, NULL) != 0) {
        return -1;
    }
    con_puts("rsdir:\n");
    for (i = 0; i < 8; i++) {
        uint8_t *e = dir + i * 64;
        if (!e[2]) {
            continue;
        }
        w = (const int64_t *)(e + 40);
        con_puts("  ");
        con_puts((const char *)(e + 2));
        con_puts("  size=");
        uart_put_u64_hex(g_uart, (uint64_t)w[1]);
        con_puts("  blk=");
        uart_put_u64_hex(g_uart, (uint64_t)w[0]);
        con_puts("\n");
        n++;
    }
    con_puts("rsdir n=");
    uart_put_u32_dec(g_uart, n);
    con_puts("\n");
    return (int)n;
}

/* Disk catalog: seed without per-file ifs in the runzc path (M37+). */
struct rs_catalog_ent {
    const char *name;
    const char *src;
    int graphic;
};

/* M38 multi-file demo (also on disk via catalog). */
static const char ADDLIB_ZC[] = "I64 Add(I64 a, I64 b)\n{\n\treturn a + b;\n}\n";
static const char USEADD_ZC[] = "#include \"AddLib.ZC\"\n\nreturn Add(20, 22);\n";

/* Freeze catalog: #include demos + Notes (fits 8 root slots with . Keep Hi). */
static const struct rs_catalog_ent g_rs_catalog[] = {
    { "AddLib.ZC", ADDLIB_ZC, 0 },
    { "UseAdd.ZC", USEADD_ZC, 0 },
    { "DocLib.ZC", DOCLIB_ZC, 0 },
    { "Notes.ZC", NOTES_ZC, 1 },
    { "MemSort.ZC", MEMSORT_ZC, 0 },
};

static const struct rs_catalog_ent *rs_catalog_lookup(const char *name) {
    unsigned i;
    for (i = 0; i < sizeof(g_rs_catalog) / sizeof(g_rs_catalog[0]); i++) {
        if (str_ieq(name, g_rs_catalog[i].name)) {
            return &g_rs_catalog[i];
        }
    }
    return NULL;
}

static int rs_seed_catalog(void);

/* Delete one known bring-up demo when making room for the freeze catalog.
 * Never evict an arbitrary user file just because the small root is full. */
static int rs_evict_one_stale(void) {
    uint8_t br[512];
    uint8_t dir[512];
    uint64_t root = 0;
    static const char *const legacy[] = {
        "DiskLat.ZC", "Lattice.ZC", "NetOfDots.ZC", "Lines.ZC"
    };
    unsigned i;

    if (rs_read_root(br, dir, &root, NULL) != 0) {
        return -1;
    }
    for (i = 0; i < sizeof(legacy) / sizeof(legacy[0]); i++) {
        if (rs_file_exists(legacy[i], NULL)) {
            return rs_del_file(legacy[i]);
        }
    }
    return -1;
}

static unsigned rs_root_used(const uint8_t *dir) {
    unsigned i, used = 0;
    for (i = 0; i < 8; i++) {
        if (dir[i * 64 + 2]) {
            used++;
        }
    }
    return used;
}

/* Missing entries needed to run the frozen catalog and persistence smokes. */
static unsigned rs_freeze_missing(void) {
    unsigned i, missing = 0;
    for (i = 0; i < sizeof(g_rs_catalog) / sizeof(g_rs_catalog[0]); i++) {
        if (!rs_file_exists(g_rs_catalog[i].name, NULL)) {
            missing++;
        }
    }
    if (!rs_file_exists("Keep", NULL)) {
        missing++;
    }
    if (!rs_file_exists("Hi.ZC", NULL)) {
        missing++;
    }
    return missing;
}

/* Prepare a valid RedSea volume for the freeze acceptance sequence. The
 * volume has only eight root slots, so old bring-up demos may need to go if
 * both Keep and Hi.ZC are absent. Only the four named, reproducible demos
 * above are candidates; unknown files are preserved and cause a clean error. */
static int rs_prepare_freeze_catalog(void) {
    uint8_t br[512];
    uint8_t dir[512];
    uint64_t root = 0;
    unsigned used, missing;
    unsigned tries = 0;
    unsigned i;

    /* A blank/unformatted volume is left for RSCPersist/RunZC to initialize. */
    if (rs_read_root(br, dir, &root, NULL) != 0) {
        return 1;
    }
    for (;;) {
        used = rs_root_used(dir);
        missing = rs_freeze_missing();
        if (used + missing <= 8) {
            break;
        }
        if (tries++ >= 4 || rs_evict_one_stale() != 0 ||
            rs_read_root(br, dir, &root, NULL) != 0) {
            return -1;
        }
    }
    if (rs_seed_catalog() != 0) {
        return -1;
    }
    /* rs_seed_catalog is intentionally best-effort for older callers. The
     * freeze path needs a stronger guarantee before it reports success. */
    for (i = 0; i < sizeof(g_rs_catalog) / sizeof(g_rs_catalog[0]); i++) {
        if (!rs_file_exists(g_rs_catalog[i].name, NULL)) {
            return -1;
        }
    }
    return 0;
}

/* Best-effort seed: evict stale (non-catalog) files if the root is full.
 * RAM smoke and PCI RedSea are different volumes — PCI may still hold
 * NetOfDots/Lines from older milestones. */
static int rs_seed_catalog(void) {
    uint8_t br[512];
    uint8_t dir[512];
    unsigned i;

    if (rs_read_root(br, dir, NULL, NULL) != 0) {
        return -1;
    }
    for (i = 0; i < sizeof(g_rs_catalog) / sizeof(g_rs_catalog[0]); i++) {
        const struct rs_catalog_ent *e = &g_rs_catalog[i];
        uint64_t elen = 0;
        int tries;

        if (rs_file_exists(e->name, &elen) && elen > 0) {
            continue;
        }
        for (tries = 0; tries < 4; tries++) {
            if (rs_put_file(e->name, e->src, str_len(e->src)) == 0) {
                break;
            }
            if (rs_evict_one_stale() != 0) {
                break;
            }
        }
    }
    return 0;
}

/* Ensure one catalog file is on disk (for runzc / #include deps). */
static int rs_ensure_catalog_file(const char *name) {
    const struct rs_catalog_ent *e = rs_catalog_lookup(name);
    uint64_t elen = 0;
    int tries;

    if (!e) {
        return 0;
    }
    if (rs_file_exists(e->name, &elen) && elen > 0) {
        return 0;
    }
    for (tries = 0; tries < 4; tries++) {
        if (rs_put_file(e->name, e->src, str_len(e->src)) == 0) {
            return 0;
        }
        if (rs_evict_one_stale() != 0) {
            return -1;
        }
    }
    return -1;
}

static int rs_is_graphic_zc(const char *name) {
    const struct rs_catalog_ent *e = rs_catalog_lookup(name);
    return e && e->graphic;
}

static int uart_getc_nb(void) {
    volatile uint32_t *r;
    int c;
    if (!g_uart) {
        return -1;
    }
    r = (volatile uint32_t *)(uintptr_t)g_uart;
    if (r[PL011_FR / 4] & PL011_FR_RXFE) {
        return -1;
    }
    c = (int)(r[PL011_DR / 4] & 0xff);
    r[PL011_RSR / 4] = 0; /* clear OE/BE/PE/FE via ECR alias */
    return c;
}

/* UTM's serial terminal sends Return as CRLF. A shell command ends on CR,
 * then an immediately-entered zcall must not see its trailing LF as a key.
 * Keep raw UART reads unchanged for diagnostics and binary transfers. */
static int uart_text_getc_nb(void) {
    static int after_cr;
    for (;;) {
        int c = uart_getc_nb();
        if (c < 0)
            return -1;
        if (after_cr) {
            after_cr = 0;
            if (c == '\n')
                continue;
        }
        after_cr = c == '\r';
        return c;
    }
}

static int kbd_getc_nb(void) {
    int c = input_keyboard_getc_nb();
    if (c >= 0) {
        return c;
    }
    return uart_text_getc_nb();
}

#ifndef ZEAL_PI_DIAG
/* A zcall owns the input stream until it returns. Shell keystrokes have a
 * parallel virtio message record; discard those old records on handoff. */
static int zc_uart_release_pending;
static uint64_t zc_uart_release_char;
static void zc_key_reset(void) {
    uint8_t type;
    uint64_t arg1, arg2;
    zc_uart_release_pending = 0;
    while (input_keyboard_msg_nb(&type, &arg1, &arg2)) {}
    while (input_keyboard_getc_nb() >= 0) {}
}

static int zc_key_read(uint8_t *type, uint64_t *arg1, uint64_t *arg2) {
    if (zc_uart_release_pending) {
        zc_uart_release_pending = 0;
        *type = 3;
        *arg1 = zc_uart_release_char;
        *arg2 = 0;
        return 1;
    }
    if (input_keyboard_msg_nb(type, arg1, arg2)) {
        if (*type == 2 && *arg1 > 0 && *arg1 < 256)
            (void)input_keyboard_getc_nb(); /* same down event in shell char ring */
        return 1;
    }
    /* When a GUI task owns the virtio keyboard, leave UART bytes exclusively
     * to the serial shell. Polling UART here would steal characters from its
     * independent line editor while the graphical terminal is active. */
    if (zc_focus_owns_input())
        return 0;
    int c = uart_text_getc_nb();
    if (c < 0)
        return 0;
    *type = 2; /* UART has no physical key-up or scan-code data. */
    *arg1 = (uint64_t)(c == '\r' ? '\n' : c);
    *arg2 = 0;
    zc_uart_release_char = *arg1;
    zc_uart_release_pending = 1;
    return 1;
}
#endif

static int g_key_pending = -1;

uint64_t hc_builtin_keyhit(void) {
    if (g_key_pending >= 0) {
        return 1;
    }
    int c = kbd_getc_nb();
    if (c >= 0) {
        g_key_pending = c;
        return 1;
    }
    return 0;
}

uint64_t hc_builtin_getkey(void) {
    if (g_key_pending >= 0) {
        int c = g_key_pending;
        g_key_pending = -1;
        return (uint64_t)(int64_t)c;
    }
    int c = kbd_getc_nb();
    return (uint64_t)(int64_t)c; /* -1 if none */
}

static uint32_t g_tab_cx = ~0u, g_tab_cy = ~0u;
static int g_tab_prev_btn;
static uint32_t g_tab_clicks;

static void tablet_paint_stamp(uint32_t x, uint32_t y, uint32_t bgr) {
    for (int dy = -3; dy <= 3; dy++) {
        for (int dx = -3; dx <= 3; dx++) {
            if (dx * dx + dy * dy > 10) {
                continue;
            }
            int32_t px = (int32_t)x + dx;
            int32_t py = (int32_t)y + dy;
            if (px >= 0 && py >= 0) {
                fb_putpixel((uint32_t)px, (uint32_t)py, bgr);
            }
        }
    }
}

static void tablet_cursor_tick(void) {
    if (!input_pointer_ready() || !g_fb) {
        return;
    }
    uint32_t x, y;
    if (!input_pointer_xy((uint32_t)g_fb_w, (uint32_t)g_fb_h, &x, &y)) return;
    int btn = input_pointer_buttons();
    /* Once source WinMgr owns the graphical desktop it draws the software
     * arrow through gr.fp_draw_mouse. Keep this bootstrap marker out of that
     * scanout so it cannot cover the real cursor or leave stale orange pixels. */
#ifndef ZEAL_PI_DIAG
    if (zc_source_winmgr_active() || zc_gui_terminal_active() ||
        zc_window_demo_active()) {
        g_tab_prev_btn = btn;
        g_tab_cx = x;
        g_tab_cy = y;
        return;
    }
#endif
    if ((btn & 1) && !(g_tab_prev_btn & 1)) {
        tablet_paint_stamp(x, y, 0x0040C0F0u);
        g_tab_clicks++;
    } else if (btn & 1) {
        /* Drag-paint while held. */
        tablet_paint_stamp(x, y, 0x003090C0u);
    }
    if (g_tab_cx != ~0u && !(btn & 1)) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int32_t px = (int32_t)g_tab_cx + dx;
                int32_t py = (int32_t)g_tab_cy + dy;
                if (px >= 0 && py >= 0) {
                    fb_putpixel((uint32_t)px, (uint32_t)py, 0x00101010u);
                }
            }
        }
    }
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int32_t px = (int32_t)x + dx;
            int32_t py = (int32_t)y + dy;
            if (px >= 0 && py >= 0) {
                fb_putpixel((uint32_t)px, (uint32_t)py,
                            (btn & 1) ? 0x00F0F0F0u : 0x00E08020u);
            }
        }
    }
    g_tab_prev_btn = btn;
    g_tab_cx = x;
    g_tab_cy = y;
}

/* M157/M202: after Lattice/graphics demos, match shell_run dark FB (not DCFill black). */
static void shell_fb_ready(void) {
    if (g_fb && !fb_console_owned_by_desktop()) {
        fb_clear(0x00101820u);
        g_fb_cx = 0;
        g_fb_cy = 0;
    }
    /* M202: Lattice depth must not occlude later shell paint/bars. */
    if (g_hc_depth_on) {
        hc_depth_map_clear();
        g_hc_depth_on = 0;
    }
    g_hc_cdc.depth_buf = 0;
}

#ifndef ZEAL_PI_DIAG
static void zc_print_result(int64_t result) {
    char buf[20] = "0x";
    const char *hex = "0123456789abcdef";
    for (unsigned i = 0; i < 16; i++)
        buf[2+i] = hex[((uint64_t)result >> ((15-i)*4)) & 15];
    buf[18] = '\n'; buf[19] = 0;
    con_puts("zc => "); con_puts(buf);
}
static void zc_print_decimal(uint64_t value) {
    char buf[21];
    unsigned n = 0;
    do { buf[n++] = (char)('0' + value % 10); value /= 10; } while (value);
    while (n) con_write(buf[--n]);
}

void zc_usb_status(void) {
    unsigned devices = 0, hubs = 0, quarantined = 0;
    for (unsigned i = 0; i < g_usb.device_count; i++) {
        devices += !!g_usb.devices[i].present;
        hubs += g_usb.devices[i].present && g_usb.devices[i].hub_ports != 0;
        quarantined += !!g_usb.devices[i].quarantined;
    }
    con_puts("usb_input: controllers="); zc_print_decimal(g_usb.controllers);
    con_puts(" devices="); zc_print_decimal(devices);
    con_puts(" keyboard="); con_puts(usb_keyboard_ready() ? "usb" : "virtio/serial");
    con_puts(" pointer="); con_puts(usb_pointer_ready() ?
        (g_usb.pointer->kind == 3 ? "usb-tablet" : "usb-mouse") : "virtio");
    con_puts(" key_events="); zc_print_decimal(g_usb.key_events);
    con_puts(" pointer_reports="); zc_print_decimal(g_usb.pointer_reports);
    con_puts(" wheel_events="); zc_print_decimal(g_usb.wheel_events);
    con_puts(" errors="); zc_print_decimal(g_usb.errors);
    con_puts(" dma_bytes="); zc_print_decimal(usb_dma_used); con_puts("\n");
    /* Report both the decoder and last HID report, so a host-held modifier
     * can be distinguished from a guest decoder that failed to release it. */
    con_puts("keyboard_state: shift="); zc_print_decimal(g_vk.shift);
    con_puts(" ctrl="); zc_print_decimal(g_vk.ctrl);
    con_puts(" usb_modifiers="); zc_print_decimal(usb_keyboard_ready() ? g_usb.keyboard->previous[0] : 0);
    con_puts(" usb_repeat="); zc_print_decimal(usb_keyboard_ready() ? g_usb.keyboard->repeat_usage : 0);
    con_puts("\n");
    /* Report the effective pointer path even when VirtIO owns it and there
     * is no xHCI controller to contribute an `xhci: ... buttons=` line.
     * This is also useful when diagnosing UTM capture/release behavior. */
    uint32_t pointer_x = 0, pointer_y = 0;
    int pointer_buttons = 0;
    int pointer_ready = input_pointer_ready() && g_fb &&
        input_pointer_sample((uint32_t)g_fb_w, (uint32_t)g_fb_h,
                             &pointer_x, &pointer_y,
                             &pointer_buttons, NULL);
    con_puts("pointer_state: ready="); zc_print_decimal(pointer_ready);
    con_puts(" pointer_xy="); zc_print_decimal(pointer_x);
    con_puts("/"); zc_print_decimal(pointer_y);
    con_puts(" buttons="); zc_print_decimal(pointer_buttons); con_puts("\n");
    con_puts("usb_topology: hubs="); zc_print_decimal(hubs);
    con_puts(" attaches="); zc_print_decimal(g_usb.attaches);
    con_puts(" detaches="); zc_print_decimal(g_usb.detaches);
    con_puts(" rejected="); zc_print_decimal(g_usb.rejected);
    con_puts(" quarantined="); zc_print_decimal(quarantined); con_puts("\n");
    for (unsigned i = 0; i < g_usb.device_count; i++) {
        struct usb_dev *d = &g_usb.devices[i];
        if (!d->present) continue;
        con_puts("usb_device: root="); zc_print_decimal(d->port + 1);
        con_puts(" route="); zc_print_decimal(d->route);
        con_puts(" depth="); zc_print_decimal(d->depth);
        con_puts(" speed="); zc_print_decimal(d->speed);
        con_puts(" tt_slot="); zc_print_decimal(d->tt_slot);
        con_puts(" tt_port="); zc_print_decimal(d->tt_port);
        con_puts(" hub_ports="); zc_print_decimal(d->hub_ports); con_puts("\n");
    }
    for (unsigned i = 0; i < g_usb.controllers; i++) {
        struct usb_hc *hc = &g_usb.hc[i];
        con_puts("xhci: index="); zc_print_decimal(i);
        con_puts(" running="); zc_print_decimal(hc->running);
        con_puts(" error="); zc_print_decimal(hc->error);
        con_puts(" commands="); zc_print_decimal(hc->command_count);
        con_puts(" events="); zc_print_decimal(hc->event_count);
        con_puts(" port_changes="); zc_print_decimal(hc->port_changes);
        con_puts(" pointer_xy="); zc_print_decimal(g_usb.x);
        con_puts("/"); zc_print_decimal(g_usb.y);
        con_puts(" buttons="); zc_print_decimal(g_usb.buttons); con_puts("\n");
    }
}

/* zrecv uses the PL011 serial channel exclusively. Each 128-byte payload is
 * sent as one hex line; acknowledgement keeps the tiny UART FIFO from being
 * overrun by a host paste and makes a broken transfer visible. */
#define ZRECV_CHUNK 128u
static int zrecv_hex(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
static int zrecv_getc(uint64_t deadline) {
    for (;;) {
        int c = uart_getc_nb();
        if (c >= 0) return c;
        uint64_t now;
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        if ((int64_t)(now - deadline) >= 0) return -1;
        (void)hc_builtin_sleep(1);
    }
}
static int zrecv_chunk(uint8_t *dst, unsigned length, uint64_t ticks) {
    uint64_t now;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
    uint64_t deadline = now + ticks;
    int c;
    do { c = zrecv_getc(deadline); } while (c == '\r' || c == '\n');
    if (c < 0) return -2;
    for (unsigned i = 0; i < length; i++) {
        int hi = zrecv_hex(c);
        int lo = zrecv_hex(zrecv_getc(deadline));
        if (hi < 0 || lo < 0) return -1;
        dst[i] = (uint8_t)((hi << 4) | lo);
        c = zrecv_getc(deadline);
        if (c < 0) return -2;
        if (i + 1 < length && zrecv_hex(c) < 0) return -1;
    }
    return c == '\r' || c == '\n' ? 0 : -1;
}
static int zrecv_args(const char *p, char name[ZSS_MAX_NAME + 1],
                      uint32_t *length, uint32_t *crc) {
    unsigned n = 0;
    while (*p == ' ') p++;
    while (*p && *p != ' ' && *p != '\t') {
        if (n == ZSS_MAX_NAME) return -1;
        name[n++] = *p++;
    }
    name[n] = 0;
    if (!zss_name_ok(name, NULL)) return -1;
    while (*p == ' ' || *p == '\t') p++;
    if (*p < '0' || *p > '9') return -1;
    uint32_t size = 0;
    while (*p >= '0' && *p <= '9') {
        unsigned digit = (unsigned)(*p++ - '0');
        if (size > (ZSS_MAX_FILE - digit) / 10u) return -1;
        size = size * 10u + digit;
    }
    if (!size) return -1;
    while (*p == ' ' || *p == '\t') p++;
    uint32_t checksum = 0;
    for (unsigned i = 0; i < 8; i++) {
        if (!*p) return -1;
        int digit = zrecv_hex(*p++);
        if (digit < 0) return -1;
        checksum = (checksum << 4) | (uint32_t)digit;
    }
    while (*p == ' ' || *p == '\t') p++;
    if (*p) return -1;
    *length = size;
    *crc = checksum;
    return 0;
}
static void zrecv_serial(const char *args) {
    char name[ZSS_MAX_NAME + 1];
    uint32_t length, checksum;
    struct zss_stream stream;
    uint8_t bytes[ZRECV_CHUNK];
    if (zrecv_args(args, name, &length, &checksum)) {
        con_puts("zrecv: use zrecv Name.ZC <bytes> <crc32-8hex>\n"); return;
    }
    if (!g_uart) { con_puts("zrecv: serial unavailable\n"); return; }
    int status = zss_stream_begin(&stream, name, length, checksum);
    if (status) {
        con_puts(status == -2 ? "zrecv: native source partition unavailable\n" :
                 status == -3 ? "zrecv: source partition full\n" :
                                "zrecv: invalid name or source volume\n");
        return;
    }
    /* A UTM Terminal 1 user may paste chunks by hand; allow enough time to
     * locate the next line while still recovering from an abandoned upload. */
    uint64_t ticks = hc_builtin_cntfrq() * 120u;
    con_puts("zrecv: ready bytes="); zc_print_decimal(length);
    con_puts(" chunk=128\n");
    while (stream.written < length) {
        unsigned n = length - stream.written;
        if (n > ZRECV_CHUNK) n = ZRECV_CHUNK;
        int read = zrecv_chunk(bytes, n, ticks);
        if (read) {
            con_puts(read == -2 ? "zrecv: timeout; old file intact\n" :
                                  "zrecv: malformed chunk; old file intact\n");
            return;
        }
        if (zss_stream_write(&stream, bytes, n)) {
            con_puts("zrecv: disk write failed\n"); return;
        }
        con_puts("zrecv: ack "); zc_print_decimal(stream.written); con_puts("\n");
    }
    status = zss_stream_finish(&stream);
    if (status) {
        con_puts(status == -4 ? "zrecv: checksum mismatch; old file intact\n" :
                              "zrecv: disk write failed\n");
        return;
    }
    con_puts("zrecv: saved "); con_puts(name);
    con_puts(" bytes="); zc_print_decimal(length); con_puts("\n");
}
static void zss_shell_list(void) {
    uint8_t header[512];
    uint32_t tail = 0, records = 0, shown = 0;
    int scan = zss_find(NULL, NULL, &tail, &records);
    if (scan < 0) {
        con_puts(scan == -2 ? "zls: native source partition unavailable\n" :
                              "zls: damaged source catalog\n");
        return;
    }
    for (uint32_t pos = 1; pos < tail;) {
        struct zss_entry entry, latest;
        char name[ZSS_MAX_NAME + 1];
        int deleted;
        if (!zss_sector_read(pos, header) ||
            zss_decode_record(header, pos, &entry, name, &deleted)) {
            con_puts("zls: damaged source catalog\n"); return;
        }
        pos += 1u + entry.blocks;
        if (deleted) continue;
        int current = zss_find(name, &latest, NULL, NULL);
        if (current == 1 && latest.header_sector == entry.header_sector) {
            con_puts(name); con_puts(" bytes=");
            zc_print_decimal(entry.length); con_puts("\n");
            shown++;
        } else if (current == -1 || current == -2) {
            con_puts("zls: damaged source catalog\n"); return;
        }
    }
    con_puts("zls: n="); zc_print_decimal(shown);
    con_puts(" records="); zc_print_decimal(records);
    con_puts(" sectors="); zc_print_decimal(tail);
    con_puts("/"); zc_print_decimal(g_zss_sectors);
    con_puts("\n");
}
int zeal_zss_entry_at(uint32_t index, char *name, size_t name_cap,
                      size_t *size_out) {
    return zss_entry_at(index, name, name_cap, size_out);
}
uint64_t zeal_zss_catalog_revision(void) { return g_zss_catalog_revision; }
int zeal_zss_writable(void) { return zss_available(); }
#include "zc_source_editor.h"
#else
int zeal_zss_entry_at(uint32_t index, char *name, size_t name_cap,
                      size_t *size_out) {
    (void)index; (void)name; (void)name_cap; (void)size_out; return 0;
}
uint64_t zeal_zss_catalog_revision(void) { return 0; }
int zeal_zss_writable(void) { return 0; }
#endif

#ifndef ZEAL_PI_DIAG
static uint32_t zinstall_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t zinstall_u64(const uint8_t *p) {
    return (uint64_t)zinstall_u32(p) | ((uint64_t)zinstall_u32(p + 4) << 32);
}

static int zinstall_image_valid(uint64_t *sectors_out, uint32_t *extents_out,
                                uint64_t *data_start_out) {
    const uint8_t *image = g_install_image;
    uint64_t expected_sectors = ZEAL_DISK_SECTS;
    uint64_t data_start, data_bytes = 0, previous_end = 0;
    uint32_t count;
    static const uint8_t magic[8] = {'Z','I','N','S','T','0','0','1'};
    if (!image || g_install_image_size < 40 || g_install_image_size > 64ull * 1024 * 1024)
        return 0;
    for (unsigned i = 0; i < sizeof(magic); i++)
        if (image[i] != magic[i]) return 0;
    if (zinstall_u32(image + 8) != 1 || zinstall_u32(image + 12) != VB_SECT ||
        zinstall_u64(image + 16) != expected_sectors ||
        zinstall_u32(image + 28) != 16 ||
        zinstall_u64(image + 32) != g_install_image_size)
        return 0;
    count = zinstall_u32(image + 24);
    if ((uint64_t)count > (g_install_image_size - 40) / 16) return 0;
    data_start = 40ull + (uint64_t)count * 16;
    if (data_start > g_install_image_size) return 0;
    for (uint32_t i = 0; i < count; i++) {
        const uint8_t *entry = image + 40 + (uint64_t)i * 16;
        uint64_t lba = zinstall_u64(entry);
        uint32_t n = zinstall_u32(entry + 8);
        if (!n || zinstall_u32(entry + 12) != 0 || lba < previous_end ||
            lba >= expected_sectors || (uint64_t)n > expected_sectors - lba ||
            (uint64_t)n > (g_install_image_size - data_start - data_bytes) / VB_SECT)
            return 0;
        previous_end = lba + n;
        data_bytes += (uint64_t)n * VB_SECT;
    }
    if (data_start + data_bytes != g_install_image_size) return 0;
    *sectors_out = expected_sectors;
    *extents_out = count;
    *data_start_out = data_start;
    return 1;
}

static void zinstall_handle(const char *args) {
    uint64_t sectors, data_start, sector, next_report;
    uint32_t extent_count, max_batch;

    /* UTM may expose the booted ISO through a USB mass-storage device, which
     * reports the media as a disk instead of optical media.  The validated
     * installer payload is the authoritative check that this is an installer
     * boot; installed disks do not carry that module. */
    if (!zinstall_image_valid(&sectors, &extent_count, &data_start)) {
        con_puts("zinstall: installer image is unavailable; boot the ARM64 ISO\n");
        return;
    }
    if (!virtio_blk_absolute_ready() ||
        virtio_blk_full_capacity() != sectors) {
        con_puts("zinstall: attach one PCI VirtIO target disk of exactly 128 MiB\n");
        return;
    }
    if (!virtio_blk_flush_disk()) {
        con_puts("zinstall: target does not support durable flush; no install started\n");
        return;
    }
    if (!streq(args, "ERASE")) {
        con_puts("zinstall: ready; this erases the entire 128 MiB target disk. "
                 "Type zinstall ERASE to confirm.\n");
        return;
    }

    max_batch = virtio_blk_max_sectors();
    if (!max_batch || (uint64_t)max_batch * VB_SECT > sizeof(g_install_io)) {
        con_puts("zinstall: invalid VirtIO transfer window; no install started\n");
        return;
    }
    for (unsigned i = 0; i < sizeof(g_install_io); i++) g_install_io[i] = 0;

    con_puts("zinstall: erasing and installing 128 MiB; progress includes full read-back\n");
    next_report = sectors / 8;
    for (sector = 0; sector < sectors;) {
        uint32_t batch = (uint32_t)(sectors - sector);
        if (batch > max_batch) batch = max_batch;
        if (!virtio_blk_write_abs_many(g_install_io, sector, batch)) {
            con_puts("\nzinstall: erase failed at sector ");
            zc_print_decimal(sector);
            con_puts("; target may be incomplete\n");
            return;
        }
        sector += batch;
        if (sector >= next_report) {
            con_puts("zinstall: erase ");
            zc_print_decimal((sector * 100) / sectors);
            con_puts("%\n");
            next_report += sectors / 8;
        }
    }
    if (!virtio_blk_flush_disk()) {
        con_puts("zinstall: erase flush failed; target may be incomplete\n");
        return;
    }

    uint64_t payload_off = data_start;
    uint64_t written_sectors = 0;
    next_report = sectors / 8;
    for (uint32_t e = 0; e < extent_count; e++) {
        const uint8_t *entry = g_install_image + 40 + (uint64_t)e * 16;
        uint64_t lba = zinstall_u64(entry);
        uint32_t left = zinstall_u32(entry + 8);
        uint32_t done = 0;
        while (left) {
            uint32_t batch = left < max_batch ? left : max_batch;
            const uint8_t *src = g_install_image + payload_off + (uint64_t)done * VB_SECT;
            if (!virtio_blk_write_abs_many(src, lba + done, batch)) {
                con_puts("\nzinstall: image write failed at sector ");
                zc_print_decimal(lba + done);
                con_puts("; target may be incomplete\n");
                return;
            }
            done += batch;
            left -= batch;
            written_sectors += batch;
        }
        payload_off += (uint64_t)zinstall_u32(entry + 8) * VB_SECT;
        if (written_sectors >= next_report) {
            con_puts("zinstall: wrote image extents ");
            zc_print_decimal((written_sectors * 100) / sectors);
            con_puts("%\n");
            next_report += sectors / 8;
        }
    }
    if (!virtio_blk_flush_disk()) {
        con_puts("zinstall: image flush failed; do not reboot from target\n");
        return;
    }

    con_puts("zinstall: checking every sector\n");
    uint32_t extent_index = 0;
    uint64_t extent_data = data_start;
    next_report = sectors / 8;
    for (sector = 0; sector < sectors;) {
        uint32_t batch = (uint32_t)(sectors - sector);
        if (batch > max_batch) batch = max_batch;
        if (!virtio_blk_read_abs_many(g_install_io, sector, batch)) {
            con_puts("\nzinstall: read-back failed at sector ");
            zc_print_decimal(sector);
            con_puts("; target may be incomplete\n");
            return;
        }
        for (uint32_t n = 0; n < batch; n++) {
            uint64_t current = sector + n;
            while (extent_index < extent_count) {
                const uint8_t *entry = g_install_image + 40 + (uint64_t)extent_index * 16;
                uint64_t start = zinstall_u64(entry);
                uint32_t count = zinstall_u32(entry + 8);
                if (current < start + count) break;
                extent_data += (uint64_t)count * VB_SECT;
                extent_index++;
            }
            const uint8_t *actual = g_install_io + (uint64_t)n * VB_SECT;
            if (extent_index < extent_count) {
                const uint8_t *entry = g_install_image + 40 + (uint64_t)extent_index * 16;
                uint64_t start = zinstall_u64(entry);
                uint32_t count = zinstall_u32(entry + 8);
                if (current >= start && current < start + count) {
                    const uint8_t *expected = g_install_image + extent_data +
                                              (current - start) * VB_SECT;
                    for (unsigned b = 0; b < VB_SECT; b++) {
                        if (actual[b] != expected[b]) {
                            con_puts("\nzinstall: compare failed at sector ");
                            zc_print_decimal(current);
                            con_puts("; target may be incomplete\n");
                            return;
                        }
                    }
                    continue;
                }
            }
            for (unsigned b = 0; b < VB_SECT; b++) {
                if (actual[b]) {
                    con_puts("\nzinstall: nonzero gap at sector ");
                    zc_print_decimal(current);
                    con_puts("; target may be incomplete\n");
                    return;
                }
            }
        }
        sector += batch;
        if (sector >= next_report) {
            con_puts("zinstall: verified ");
            zc_print_decimal((sector * 100) / sectors);
            con_puts("%\n");
            next_report += sectors / 8;
        }
    }
    if (!virtio_blk_flush_disk()) {
        con_puts("zinstall: verification flush failed; do not reboot from target\n");
        return;
    }
    con_puts("zinstall: installed and fully read-back verified. Shut down, detach "
             "the ISO, then boot from the target disk.\n");
}
#endif

static void shell_handle(const char *line, int *done) {
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    if (*line == '\0') {
        return;
    }
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
    if (streq(line, "usbread")) { pi4_usb_bus_read(0u); return; }
    if (streq(line, "usbstatus")) {
        pi4_usb_bus_report(0); pi4_usb_bus_report(1);
#if defined(ZEAL_FORCE_PI4) && !defined(ZEAL_PI_DIAG)
        pi4_desktop_scanout_report();
#endif
        if (g_pi4_keyboard.attempted) {
            pi4_usb_hid_report(0); pi4_usb_hid_report(1);
        }
        pi4_usb_live_report(0); pi4_usb_live_report(1); con_puts("\n"); return;
    }
    if (streq(line, "usbstop")) {
        pi4_usb_bus_report(0); pi4_usb_bus_report(1);
        pi4_usb_live_stop();
        if (g_pi4_keyboard.attempted) {
            pi4_usb_hid_report(0); pi4_usb_hid_report(1);
        }
        pi4_usb_live_report(0); pi4_usb_live_report(1); con_puts("\n"); return;
    }
#endif
#ifndef ZEAL_PI_DIAG
    if (g_zedit.active) { zedit_handle(line); return; }
    if (line[0]=='z' && line[1]=='e' && line[2]=='d' && line[3]=='i' &&
        line[4]=='t' && line[5]==' ') { zedit_start(line+6); return; }
    if (streq(line, "zcheck")) { (void)zc_selftest(); return; }
    if (streq(line, "zstatus")) { zc_status(); return; }
    if (streq(line, "zinstall")) { zinstall_handle(""); return; }
    if (line[0] == 'z' && line[1] == 'i' && line[2] == 'n' &&
        line[3] == 's' && line[4] == 't' && line[5] == 'a' &&
        line[6] == 'l' && line[7] == 'l' && line[8] == ' ')
    { zinstall_handle(line + 9); return; }
    if (streq(line, "usbcheck")) {
        unsigned checked = 0;
        for (unsigned i = 0; i < g_usb.controllers; i++) {
            struct usb_hc *hc = &g_usb.hc[i];
            if (!hc->running) continue;
            for (unsigned n = 0; n < USB_TRBS * 3; n++) {
                if (usb_command(hc, 0, USB_TRB(23)) != USB_CC_OK) {
                    con_puts("usbcheck: command ring failed\n"); return;
                }
            }
            checked++;
        }
        con_puts(checked ? "usbcheck: command/event ring wrap OK\n" : "usbcheck: no running controller\n");
        return;
    }
    if (streq(line, "zreset")) {
        if (!zc_reset_session())
            con_puts("zc: native compiler ready (persistent session)\n");
        else
            con_puts("zreset: could not stop source windows; session was not reset\n");
        return;
    }
    if (streq(line, "windowdemo")) {
        if (!zc_start_window_demo())
            con_puts("windowdemo: source WinMgr running with two windows and a CDC control; drag/raise/resize; Ctrl+C returns to shell\n");
        else
            con_puts("windowdemo: failed; check serial diagnostics or use zreset\n");
        return;
    }
    if (streq(line, "windowdemostop")) {
        if (!zc_stop_window_demo())
            con_puts("windowdemo: stopped; bootstrap shell display resumed\n");
        else
            con_puts("windowdemo: stop failed; use zreset to discard the session\n");
        return;
    }
    if (streq(line, "winmgr")) {
        if (!zc_start_winmgr())
            con_puts("winmgr: upstream Window Manager running; shell remains on serial\n");
        else
            con_puts("winmgr: start failed; check serial diagnostics or reboot\n");
        return;
    }
    if (streq(line, "gterm")) {
        if (!zc_start_gui_terminal())
            con_puts("gterm: graphical terminal started; click it to type; Ctrl+C stops it\n");
        else
            con_puts("gterm: start failed; serial shell remains available\n");
        return;
    }
    if (streq(line, "gtermstop")) {
        if (!zc_stop_gui_terminal())
            con_puts("gterm: graphical terminal stopped\n");
        else
            con_puts("gterm: stop failed\n");
        return;
    }
    if (streq(line, "winmgrstop")) {
        if (!zc_stop_winmgr())
            con_puts("winmgr: stopped; bootstrap framebuffer pump resumed\n");
        else
            con_puts("winmgr: stop failed; restart the VM to reset the session\n");
        return;
    }
    if (streq(line, "zvol")) {
        if (zss_available()) {
            con_puts("zvol: source partition ready bank=");
            con_puts(g_zss_base == ZEAL_SRC_LBA_BASE ? "A" : "B");
            con_puts(" generation="); zc_print_decimal(g_zss_generation);
            con_puts(g_vb.flush_supported ? " flush=yes" : " flush=no");
            con_puts("\n");
        } else con_puts("zvol: source partition unavailable (legacy RedSea only)\n");
        return;
    }
    if (streq(line, "zgc")) {
        uint32_t before, after, kept;
        int status = zss_compact(&before, &after, &kept);
        if (status) {
            con_puts(status == -2 ? "zgc: native source partition unavailable\n" :
                     status == -3 ? "zgc: live source does not fit inactive bank\n" :
                     status == -4 ? "zgc: shadow bank or virtio flush unavailable\n" :
                     status == -5 ? "zgc: new bank written but flush failed; verify before further writes\n" :
                                    "zgc: source damaged or disk I/O failed; old bank remains active\n");
        } else {
            con_puts("zgc: OK sectors="); zc_print_decimal(before);
            con_puts("->"); zc_print_decimal(after);
            con_puts(" records="); zc_print_decimal(kept);
            con_puts(" bank="); con_puts(g_zss_base == ZEAL_SRC_LBA_BASE ? "A" : "B");
            con_puts("\n");
        }
        return;
    }
    if (streq(line, "zls")) { zss_shell_list(); return; }
    if (line[0]=='z' && line[1]=='r' && line[2]=='m' && line[3]==' ') {
        const char *name = line + 4;
        int status = zss_delete(name);
        if (status) {
            con_puts(status == -2 ? "zrm: native source partition unavailable\n" :
                     status == -3 ? "zrm: source partition full\n" :
                     status == -4 ? "zrm: missing source\n" :
                                    "zrm: invalid path or source catalog\n");
        } else {
            con_puts("zrm: deleted "); con_puts(name); con_puts("\n");
        }
        return;
    }
    if (line[0]=='z' && line[1]=='r' && line[2]=='e' && line[3]=='c' &&
        line[4]=='v' && line[5]==' ') {
        zrecv_serial(line + 6); return;
    }
    if (line[0]=='z' && line[1]=='v' && line[2]=='e' && line[3]=='r' &&
        line[4]=='i' && line[5]=='f' && line[6]=='y' && line[7]==' ') {
        struct zss_entry entry;
        const char *name = line + 8;
        if (zss_find(name, &entry, NULL, NULL) != 1 || zss_verify(&entry) != 0) {
            con_puts("zverify: missing or damaged source\n"); return;
        }
        con_puts("zverify: OK bytes=");
        zc_print_decimal(entry.length);
        con_puts("\n");
        return;
    }
    if (line[0]=='z' && line[1]=='p' && line[2]=='u' && line[3]=='t' && line[4]==' ') {
        const char *p = line + 5;
        char name[96], body[512];
        size_t nn = 0, bn = 0;
        uint8_t br[512], dir[512];
        while (*p == ' ') p++;
        while (*p && *p != ' ' && *p != '\t' && nn < sizeof(name)-1) {
            char c = *p++;
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '.' || c == '_' ||
                  c == '-' || c == '/')) {
                con_puts("zput: invalid filename\n"); return;
            }
            name[nn++] = c;
        }
        if (!nn || (*p != ' ' && *p != '\t')) {
            con_puts("zput: use zput Name.ZC <source>\n"); return;
        }
        name[nn] = 0;
        if (!zss_name_ok(name, NULL)) {
            con_puts("zput: invalid source path\n"); return;
        }
        while (*p == ' ' || *p == '\t') p++;
        while (*p && bn < sizeof(body)-1) {
            if (p[0] == '\\' && p[1] == 'n') { body[bn++] = '\n'; p += 2; }
            else if (p[0] == '\\' && p[1] == '\\') { body[bn++] = '\\'; p += 2; }
            else body[bn++] = *p++;
        }
        if (!bn || *p) { con_puts("zput: source empty or too long\n"); return; }
        body[bn] = 0;
        if (!virtio_blk_ready()) { con_puts("zput: persistent disk unavailable\n"); return; }
        if (zss_available()) {
            int err = zss_put(name, body, bn);
            if (err) {
                con_puts(err == -3 ? "zput: source partition full\n" :
                                       "zput: source partition write failed\n");
                return;
            }
            con_puts("zput: saved "); con_puts(name); con_puts("\n");
            return;
        }
        int needs_native = nn > 37;
        for (size_t i = 0; i < nn; i++) if (name[i] == '/') needs_native = 1;
        if (needs_native) {
            con_puts("zput: path needs native source partition\n"); return;
        }
        if (rs_read_root(br, dir, NULL, NULL) != 0) {
            con_puts("zput: RedSea unformatted; run rspersist first\n"); return;
        }
        int err = rs_put_file(name, body, bn);
        if (err) { con_puts("zput: RedSea write failed (full or I/O error)\n"); return; }
        con_puts("zput: saved "); con_puts(name); con_puts("\n");
        return;
    }
    if (line[0]=='z' && line[1]=='c' && line[2]==' ') {
        if (!zc_exec(line+3)) con_puts("zc: executed\n");
        return;
    }
    if (line[0]=='z' && line[1]=='l' && line[2]=='o' && line[3]=='a' && line[4]=='d' && line[5]==' ') {
        (void)zc_load(line+6); return;
    }
    if (line[0]=='z' && line[1]=='c' && line[2]=='a' && line[3]=='l' && line[4]=='l' && line[5]==' ') {
        int64_t result;
        if (!zc_call(line+6, &result)) zc_print_result(result);
        return;
    }
    if (streq(line, "fbquiet")) {
        g_fb_console_quiet = 1;
        con_puts("fbconsole: quiet (serial output remains enabled)\n");
        return;
    }
    if (streq(line, "fbshow")) {
        g_fb_console_quiet = 0;
        con_puts("fbconsole: visible\n");
        return;
    }
#endif

    if (streq(line, "help")) {
#ifndef ZEAL_PI_DIAG
        con_puts("Native compiler: zcheck | zvol | zls | zgc | zedit <path> | zrm <path> | zverify <path> | zload /path.ZC | zput Name.ZC <source> | zrecv Name.ZC <bytes> <crc32> | zload disk:Name.ZC | zc <source> | zcall Function | zstatus | zreset | zinstall [ERASE] (ISO only) | winmgr | gterm | gtermstop | winmgrstop | windowdemo | windowdemostop | fbquiet | fbshow\n");
        con_puts("Window test order: zreset -> zload /Kernel/KernelA.HH -> zload /Kernel/KernelB.HH\n");
        con_puts("  -> zload /Tests/WindowDragLive.ZC -> zcall WindowDragLiveStart; stop: zcall WindowDragLiveStop\n");
        con_puts("UTM window demo: windowdemo (drag/raise/resize; Ctrl+C exits) | windowdemostop\n");
#endif
        con_puts("UTM freeze: vblk | rspersist | rscatalog | rsdir | runzc | runzc Notes.ZC\n");
        con_puts("Lattice: nearlatticelite | disklat | lattice | latticeplay | stocklat | stockplay | depthplotlite\n");
        /* M178: eyes-on controls without reading MenuPush / DiskLat source. */
        con_puts("  latticeplay: Esc Enter Space c +/- e | L-click place | R-drag aim | arrows dth/speed | 0-9 layer\n");
        con_puts("  stockplay: Esc Enter Space c +/- e | L-click place | R-drag aim | arrows dth/speed | 0-9 layer\n");
        con_puts("  stocklat: scripted StockLat SPACE+ESC smoke (expect 15)\n");
        con_puts("Pi/serial: uartrx — wait ~2s for one PL011 RX byte (USB-TTL @ 115200)\n");
        con_puts("cmds: help|abs|sum|bars|stars|circles|bounce|paint|netofdots|lines|minigr|memsort|globshare|life|cartlite|vec2lite|angleslite|coslite|sqrtlite|arglite|commalite|plot3lite|tospilite|colorlite|turtlelite|filllite|initlite|deflite|printlite|msglite|menulite|findlite|fslite|setuplite|ttlite|buflite|inclite|dclite|linedclite|grflite|movelite|checkedlite|cmplite|forinclite|microlite|movestacklite|endlite|drawitlite|latticelite|looplite|demolite|eventlite|playlite|inputlite|rightlite|cursorlite|uplite|ticklite|framelite|plotdclite|abortlite|aimmovelite|idlelite|layerlite|endslite|speedlite|midlite|livelite|accellite|restartlite|widthlite|bothcolorlite|menufulllite|menubiglite|trylite|stepcountlite|anglesfulllite|braceangleslite|bracepilite|setmenulite|nearlatticelite|f64iflite|wraplatticelite|menulooplite|idxalllite|disklat|lattice|depthbuflite|depthrstlite|depthplotlite|depthlinelite|peekplot|offbmp|heapstr|catfmt|heapque|jobque|jobrun|spawn|popup|doclite|ramblk|namefile|dirlook|dirdel|fopen|fwrite|multiblk|redsea|rsroot|rsfile|rsalloc|rsfree|rsmulti|rscfile|rscwrite|rscseek|rsclib|rspersist|rscatalog|rsdir|rsdel|rsrename|runzc|runzc <file.ZC>|vblk|uartrx|halt|hc <src>|expr\n");
        con_puts("  hc: Print*/Str*/Mem*/Min/Max/Clamp/Sign/Sqr/Abs/Cnt/CntFrq/HashStr/Mouse*/Rand/Sleep/Gr*/Cls\n");
        con_puts("  hc: KeyHit/GetKey (Esc exits paint loops)\n");
        con_puts("  netofdots/lines/minigr/memsort/globshare/life/cartlite/vec2lite/angleslite/coslite/sqrtlite/arglite/commalite/peekplot/offbmp/heapstr/catfmt/heapque/jobque/jobrun/spawn/popup/doclite/ramblk/namefile/dirlook/dirdel/fopen/fwrite/multiblk/redsea/rsroot/rsfile/rsalloc/rsfree/rsmulti/rscfile/rscwrite/rscseek/rsclib/rspersist/runzc/vblk: upstream ZealOS demos\n");
        con_puts("  tablet: click/drag; HolyC paint via Mouse*; latticeplay place/aim\n");
        return;
    }
    if (streq(line, "uartrx")) {
        /* M200: Pi USB-TTL RX proof — wait ~2s for one byte after RX prep. */
        uint64_t frq = 0, t0 = 0, now = 0;
        int c = -1;
        pl011_rx_prep();
        con_puts("uartrx: send one byte @ 115200 (2s)...\n");
        __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frq));
        if (frq == 0) {
            frq = 62500000;
        }
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
        do {
            c = uart_getc_nb();
            if (c >= 0) {
                break;
            }
            /* M202: same Sleep peek as shell idle (not a hot cntpct spin). */
            (void)hc_builtin_sleep(1);
            __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
        } while ((now - t0) < frq * 2ull);
        if (c < 0) {
            con_puts("uartrx timeout (no RX)\n");
            return;
        }
        con_puts("uartrx got 0x");
        {
            char hx[3];
            const char *dig = "0123456789abcdef";
            hx[0] = dig[(c >> 4) & 15];
            hx[1] = dig[c & 15];
            hx[2] = 0;
            con_puts(hx);
        }
        if (c >= 32 && c < 127) {
            con_puts(" '");
            con_write((char)c);
            con_puts("'\n");
        } else {
            con_puts("\n");
        }
        return;
    }
    if (streq(line, "halt") || streq(line, "quit")) {
        *done = 1;
        return;
    }
    if (streq(line, "netofdots")) {
        uint64_t got = 0;
        if (hc_run_src(NETOFDOTS_ZC, &got) != 0) {
            con_puts("netofdots FAIL\n");
            return;
        }
        con_puts("netofdots ok (inspect FB; sleeping)\n");
        (void)hc_builtin_sleep(3000);
        return;
    }
    if (streq(line, "lines")) {
        uint64_t got = 0;
        if (hc_run_src(LINES_ZC, &got) != 0) {
            con_puts("lines FAIL\n");
            return;
        }
        con_puts("lines ok\n");
        return;
    }
    if (streq(line, "minigr")) {
        uint64_t got = 0;
        if (hc_run_src(MINIGR_ZC, &got) != 0) {
            con_puts("minigr FAIL\n");
            return;
        }
        con_puts("minigr ok\n");
        return;
    }
    if (streq(line, "memsort")) {
        uint64_t got = 0;
        if (hc_run_src(MEMSORT_ZC, &got) != 0 || got != 16) {
            con_puts("memsort FAIL\n");
            return;
        }
        con_puts("memsort ok\n");
        return;
    }
    if (streq(line, "globshare")) {
        uint64_t got = 0;
        if (hc_run_src(GLOBSHARE_ZC, &got) != 0 || got != 8) {
            con_puts("globshare FAIL\n");
            return;
        }
        con_puts("globshare ok\n");
        return;
    }
    if (streq(line, "life")) {
        uint64_t got = 0;
        if (hc_run_src(LIFE_ZC, &got) != 0 || got != 4) {
            con_puts("life FAIL\n");
            return;
        }
        con_puts("life ok\n");
        return;
    }
    if (streq(line, "cartlite")) {
        uint64_t got = 0;
        if (hc_run_src(CARTLITE_ZC, &got) != 0 || got != 14) {
            con_puts("cartlite FAIL\n");
            return;
        }
        con_puts("cartlite ok\n");
        return;
    }
    if (streq(line, "vec2lite")) {
        uint64_t got = 0;
        if (hc_run_src(VEC2LITE_ZC, &got) != 0 || got != 25) {
            con_puts("vec2lite FAIL\n");
            return;
        }
        con_puts("vec2lite ok\n");
        return;
    }
    if (streq(line, "angleslite")) {
        uint64_t got = 0;
        if (hc_run_src(ANGLESLITE_ZC, &got) != 0 || got != 12) {
            con_puts("angleslite FAIL\n");
            return;
        }
        con_puts("angleslite ok\n");
        return;
    }
    if (streq(line, "coslite")) {
        uint64_t got = 0;
        if (hc_run_src(COSLITE_ZC, &got) != 0 || got != 63) {
            con_puts("coslite FAIL\n");
            return;
        }
        con_puts("coslite ok\n");
        return;
    }
    if (streq(line, "sqrtlite")) {
        uint64_t got = 0;
        if (hc_run_src(SQRTLITE_ZC, &got) != 0 || got != 31) {
            con_puts("sqrtlite FAIL\n");
            return;
        }
        con_puts("sqrtlite ok\n");
        return;
    }
    if (streq(line, "arglite")) {
        uint64_t got = 0;
        if (hc_run_src(ARGLITE_ZC, &got) != 0 || got != 63) {
            con_puts("arglite FAIL\n");
            return;
        }
        con_puts("arglite ok\n");
        return;
    }
    if (streq(line, "commalite")) {
        uint64_t got = 0;
        if (hc_run_src(COMMALITE_ZC, &got) != 0 || got != 63) {
            con_puts("commalite FAIL\n");
            return;
        }
        con_puts("commalite ok\n");
        return;
    }
    if (streq(line, "plot3lite")) {
        uint64_t got = 0;
        if (hc_run_src(PLOT3LITE_ZC, &got) != 0 || got != 15) {
            con_puts("plot3lite FAIL\n");
            return;
        }
        con_puts("plot3lite ok\n");
        return;
    }
    if (streq(line, "tospilite")) {
        uint64_t got = 0;
        if (hc_run_src(TOSPILITE_ZC, &got) != 0 || got != 15) {
            con_puts("tospilite FAIL\n");
            return;
        }
        con_puts("tospilite ok\n");
        return;
    }
    if (streq(line, "colorlite")) {
        uint64_t got = 0;
        if (hc_run_src(COLORLITE_ZC, &got) != 0 || got != 63) {
            con_puts("colorlite FAIL\n");
            return;
        }
        con_puts("colorlite ok\n");
        return;
    }
    if (streq(line, "turtlelite")) {
        uint64_t got = 0;
        if (hc_run_src(TURTLELITE_ZC, &got) != 0 || got != 15) {
            con_puts("turtlelite FAIL\n");
            return;
        }
        con_puts("turtlelite ok\n");
        return;
    }
    if (streq(line, "filllite")) {
        uint64_t got = 0;
        if (hc_run_src(FILLLITE_ZC, &got) != 0 || got != 15) {
            con_puts("filllite FAIL\n");
            return;
        }
        con_puts("filllite ok\n");
        return;
    }
    if (streq(line, "initlite")) {
        uint64_t got = 0;
        if (hc_run_src(INITLITE_ZC, &got) != 0 || got != 15) {
            con_puts("initlite FAIL\n");
            return;
        }
        con_puts("initlite ok\n");
        return;
    }
    if (streq(line, "deflite")) {
        uint64_t got = 0;
        if (hc_run_src(DEFLITE_ZC, &got) != 0 || got != 15) {
            con_puts("deflite FAIL\n");
            return;
        }
        con_puts("deflite ok\n");
        return;
    }
    if (streq(line, "printlite")) {
        uint64_t got = 0;
        if (hc_run_src(PRINTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("printlite FAIL\n");
            return;
        }
        con_puts("printlite ok\n");
        return;
    }
    if (streq(line, "msglite")) {
        uint64_t got = 0;
        if (hc_run_src(MSGLITE_ZC, &got) != 0 || got != 15) {
            con_puts("msglite FAIL\n");
            return;
        }
        con_puts("msglite ok\n");
        return;
    }
    if (streq(line, "menulite")) {
        uint64_t got = 0;
        if (hc_run_src(MENULITE_ZC, &got) != 0 || got != 15) {
            con_puts("menulite FAIL\n");
            return;
        }
        con_puts("menulite ok\n");
        return;
    }
    if (streq(line, "findlite")) {
        uint64_t got = 0;
        if (hc_run_src(FINDLITE_ZC, &got) != 0 || got != 15) {
            con_puts("findlite FAIL\n");
            return;
        }
        con_puts("findlite ok\n");
        return;
    }
    if (streq(line, "fslite")) {
        uint64_t got = 0;
        if (hc_run_src(FSLITE_ZC, &got) != 0 || got != 15) {
            con_puts("fslite FAIL\n");
            return;
        }
        con_puts("fslite ok\n");
        return;
    }
    if (streq(line, "setuplite")) {
        uint64_t got = 0;
        if (hc_run_src(SETUPLITE_ZC, &got) != 0 || got != 15) {
            con_puts("setuplite FAIL\n");
            return;
        }
        con_puts("setuplite ok\n");
        return;
    }
    if (streq(line, "ttlite")) {
        uint64_t got = 0;
        if (hc_run_src(TTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("ttlite FAIL\n");
            return;
        }
        con_puts("ttlite ok\n");
        return;
    }
    if (streq(line, "buflite")) {
        uint64_t got = 0;
        if (hc_run_src(BUFLITE_ZC, &got) != 0 || got != 15) {
            con_puts("buflite FAIL\n");
            return;
        }
        con_puts("buflite ok\n");
        return;
    }
    if (streq(line, "inclite")) {
        uint64_t got = 0;
        if (hc_run_src(INCLITE_ZC, &got) != 0 || got != 15) {
            con_puts("inclite FAIL\n");
            return;
        }
        con_puts("inclite ok\n");
        return;
    }
    if (streq(line, "dclite")) {
        uint64_t got = 0;
        if (hc_run_src(DCLITE_ZC, &got) != 0 || got != 15) {
            con_puts("dclite FAIL\n");
            return;
        }
        con_puts("dclite ok\n");
        return;
    }
    if (streq(line, "linedclite")) {
        uint64_t got = 0;
        if (hc_run_src(LINEDCLITE_ZC, &got) != 0 || got != 15) {
            con_puts("linedclite FAIL\n");
            return;
        }
        con_puts("linedclite ok\n");
        return;
    }
    if (streq(line, "grflite")) {
        uint64_t got = 0;
        if (hc_run_src(GRFLITE_ZC, &got) != 0 || got != 15) {
            con_puts("grflite FAIL\n");
            return;
        }
        con_puts("grflite ok\n");
        return;
    }
    if (streq(line, "movelite")) {
        uint64_t got = 0;
        if (hc_run_src(MOVELITE_ZC, &got) != 0 || got != 15) {
            con_puts("movelite FAIL\n");
            return;
        }
        con_puts("movelite ok\n");
        return;
    }
    if (streq(line, "checkedlite")) {
        uint64_t got = 0;
        if (hc_run_src(CHECKEDLITE_ZC, &got) != 0 || got != 15) {
            con_puts("checkedlite FAIL\n");
            return;
        }
        con_puts("checkedlite ok\n");
        return;
    }
    if (streq(line, "cmplite")) {
        uint64_t got = 0;
        if (hc_run_src(CMPLITE_ZC, &got) != 0 || got != 15) {
            con_puts("cmplite FAIL\n");
            return;
        }
        con_puts("cmplite ok\n");
        return;
    }
    if (streq(line, "forinclite")) {
        uint64_t got = 0;
        if (hc_run_src(FORINCLITE_ZC, &got) != 0 || got != 15) {
            con_puts("forinclite FAIL\n");
            return;
        }
        con_puts("forinclite ok\n");
        return;
    }
    if (streq(line, "microlite")) {
        uint64_t got = 0;
        if (hc_run_src(MICROLITE_ZC, &got) != 0 || got != 15) {
            con_puts("microlite FAIL\n");
            return;
        }
        con_puts("microlite ok\n");
        return;
    }
    if (streq(line, "movestacklite")) {
        uint64_t got = 0;
        if (hc_run_src(MOVESTACKLITE_ZC, &got) != 0 || got != 15) {
            con_puts("movestacklite FAIL\n");
            return;
        }
        con_puts("movestacklite ok\n");
        return;
    }
    if (streq(line, "endlite")) {
        uint64_t got = 0;
        if (hc_run_src(ENDLITE_ZC, &got) != 0 || got != 15) {
            con_puts("endlite FAIL\n");
            return;
        }
        con_puts("endlite ok\n");
        return;
    }
    if (streq(line, "drawitlite")) {
        uint64_t got = 0;
        if (hc_run_src(DRAWITLITE_ZC, &got) != 0 || got != 15) {
            con_puts("drawitlite FAIL\n");
            return;
        }
        con_puts("drawitlite ok\n");
        return;
    }
    if (streq(line, "latticelite")) {
        uint64_t got = 0;
        if (hc_run_src(LATTICELITE_ZC, &got) != 0 || got != 15) {
            con_puts("latticelite FAIL\n");
            return;
        }
        con_puts("latticelite ok\n");
        return;
    }
    if (streq(line, "looplite")) {
        uint64_t got = 0;
        if (hc_run_src(LOOPLITE_ZC, &got) != 0 || got != 15) {
            con_puts("looplite FAIL\n");
            return;
        }
        con_puts("looplite ok\n");
        return;
    }
    if (streq(line, "demolite")) {
        uint64_t got = 0;
        if (hc_run_src(DEMOLITE_ZC, &got) != 0 || got != 15) {
            con_puts("demolite FAIL\n");
            return;
        }
        con_puts("demolite ok\n");
        return;
    }
    if (streq(line, "eventlite")) {
        uint64_t got = 0;
        if (hc_run_src(EVENTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("eventlite FAIL\n");
            return;
        }
        con_puts("eventlite ok\n");
        return;
    }
    if (streq(line, "playlite")) {
        uint64_t got = 0;
        if (hc_run_src(PLAYLITE_ZC, &got) != 0 || got != 15) {
            con_puts("playlite FAIL\n");
            return;
        }
        con_puts("playlite ok\n");
        return;
    }
    if (streq(line, "inputlite")) {
        uint64_t got = 0;
        if (hc_run_src(INPUTLITE_ZC, &got) != 0 || got != 31) {
            con_puts("inputlite FAIL\n");
            return;
        }
        con_puts("inputlite ok\n");
        return;
    }
    if (streq(line, "rightlite")) {
        uint64_t got = 0;
        if (hc_run_src(RIGHTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("rightlite FAIL\n");
            return;
        }
        con_puts("rightlite ok\n");
        return;
    }
    if (streq(line, "cursorlite")) {
        uint64_t got = 0;
        if (hc_run_src(CURSORLITE_ZC, &got) != 0 || got != 15) {
            con_puts("cursorlite FAIL\n");
            return;
        }
        con_puts("cursorlite ok\n");
        return;
    }
    if (streq(line, "uplite")) {
        uint64_t got = 0;
        if (hc_run_src(UPLITE_ZC, &got) != 0 || got != 15) {
            con_puts("uplite FAIL\n");
            return;
        }
        con_puts("uplite ok\n");
        return;
    }
    if (streq(line, "ticklite")) {
        uint64_t got = 0;
        if (hc_run_src(TICKLITE_ZC, &got) != 0 || got != 15) {
            con_puts("ticklite FAIL\n");
            return;
        }
        con_puts("ticklite ok\n");
        return;
    }
    if (streq(line, "framelite")) {
        uint64_t got = 0;
        if (hc_run_src(FRAMELITE_ZC, &got) != 0 || got != 15) {
            con_puts("framelite FAIL\n");
            return;
        }
        con_puts("framelite ok\n");
        return;
    }
    if (streq(line, "plotdclite")) {
        uint64_t got = 0;
        if (hc_run_src(PLOTDCLITE_ZC, &got) != 0 || got != 15) {
            con_puts("plotdclite FAIL\n");
            return;
        }
        con_puts("plotdclite ok\n");
        return;
    }
    if (streq(line, "abortlite")) {
        uint64_t got = 0;
        if (hc_run_src(ABORTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("abortlite FAIL\n");
            return;
        }
        con_puts("abortlite ok\n");
        return;
    }
    if (streq(line, "aimmovelite")) {
        uint64_t got = 0;
        if (hc_run_src(AIMMOVELITE_ZC, &got) != 0 || got != 15) {
            con_puts("aimmovelite FAIL\n");
            return;
        }
        con_puts("aimmovelite ok\n");
        return;
    }
    if (streq(line, "idlelite")) {
        uint64_t got = 0;
        if (hc_run_src(IDLELITE_ZC, &got) != 0 || got != 15) {
            con_puts("idlelite FAIL\n");
            return;
        }
        con_puts("idlelite ok\n");
        return;
    }
    if (streq(line, "layerlite")) {
        uint64_t got = 0;
        if (hc_run_src(LAYERLITE_ZC, &got) != 0 || got != 15) {
            con_puts("layerlite FAIL\n");
            return;
        }
        con_puts("layerlite ok\n");
        return;
    }
    if (streq(line, "endslite")) {
        uint64_t got = 0;
        if (hc_run_src(ENDSLITE_ZC, &got) != 0 || got != 15) {
            con_puts("endslite FAIL\n");
            return;
        }
        con_puts("endslite ok\n");
        return;
    }
    if (streq(line, "speedlite")) {
        uint64_t got = 0;
        if (hc_run_src(SPEEDLITE_ZC, &got) != 0 || got != 15) {
            con_puts("speedlite FAIL\n");
            return;
        }
        con_puts("speedlite ok\n");
        return;
    }
    if (streq(line, "midlite")) {
        uint64_t got = 0;
        if (hc_run_src(MIDLITE_ZC, &got) != 0 || got != 15) {
            con_puts("midlite FAIL\n");
            return;
        }
        con_puts("midlite ok\n");
        return;
    }
    if (streq(line, "livelite")) {
        uint64_t got = 0;
        if (hc_run_src(LIVELITE_ZC, &got) != 0 || got != 15) {
            con_puts("livelite FAIL\n");
            return;
        }
        con_puts("livelite ok\n");
        return;
    }
    if (streq(line, "accellite")) {
        uint64_t got = 0;
        if (hc_run_src(ACCELLITE_ZC, &got) != 0 || got != 15) {
            con_puts("accellite FAIL\n");
            return;
        }
        con_puts("accellite ok\n");
        return;
    }
    if (streq(line, "restartlite")) {
        uint64_t got = 0;
        if (hc_run_src(RESTARTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("restartlite FAIL\n");
            return;
        }
        con_puts("restartlite ok\n");
        return;
    }
    if (streq(line, "widthlite")) {
        uint64_t got = 0;
        if (hc_run_src(WIDTHLITE_ZC, &got) != 0 || got != 15) {
            con_puts("widthlite FAIL\n");
            return;
        }
        con_puts("widthlite ok\n");
        return;
    }
    if (streq(line, "bothcolorlite")) {
        uint64_t got = 0;
        if (hc_run_src(BOTHCOLORLITE_ZC, &got) != 0 || got != 15) {
            con_puts("bothcolorlite FAIL\n");
            return;
        }
        con_puts("bothcolorlite ok\n");
        return;
    }
    if (streq(line, "menufulllite")) {
        uint64_t got = 0;
        if (hc_run_src(MENUFULLLITE_ZC, &got) != 0 || got != 15) {
            con_puts("menufulllite FAIL\n");
            return;
        }
        con_puts("menufulllite ok\n");
        return;
    }
    if (streq(line, "menubiglite")) {
        uint64_t got = 0;
        if (hc_run_src(MENUBIGLITE_ZC, &got) != 0 || got != 15) {
            con_puts("menubiglite FAIL\n");
            return;
        }
        con_puts("menubiglite ok\n");
        return;
    }
    if (streq(line, "trylite")) {
        uint64_t got = 0;
        if (hc_run_src(TRYLITE_ZC, &got) != 0 || got != 15) {
            con_puts("trylite FAIL\n");
            return;
        }
        con_puts("trylite ok\n");
        return;
    }
    if (streq(line, "stepcountlite")) {
        uint64_t got = 0;
        if (hc_run_src(STEPCOUNTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("stepcountlite FAIL\n");
            return;
        }
        con_puts("stepcountlite ok\n");
        return;
    }
    if (streq(line, "anglesfulllite")) {
        uint64_t got = 0;
        if (hc_run_src(ANGLESFULLLITE_ZC, &got) != 0 || got != 15) {
            con_puts("anglesfulllite FAIL\n");
            return;
        }
        con_puts("anglesfulllite ok\n");
        return;
    }
    if (streq(line, "braceangleslite")) {
        uint64_t got = 0;
        if (hc_run_src(BRACEANGLESLITE_ZC, &got) != 0 || got != 15) {
            con_puts("braceangleslite FAIL\n");
            return;
        }
        con_puts("braceangleslite ok\n");
        return;
    }
    if (streq(line, "bracepilite")) {
        uint64_t got = 0;
        if (hc_run_src(BRACEPILITE_ZC, &got) != 0 || got != 15) {
            con_puts("bracepilite FAIL\n");
            return;
        }
        con_puts("bracepilite ok\n");
        return;
    }
    if (streq(line, "setmenulite")) {
        uint64_t got = 0;
        if (hc_run_src(SETMENULITE_ZC, &got) != 0 || got != 15) {
            con_puts("setmenulite FAIL\n");
            return;
        }
        con_puts("setmenulite ok\n");
        return;
    }
    if (streq(line, "nearlatticelite")) {
        uint64_t got = 0;
        if (hc_run_src(NEARLATTICELITE_ZC, &got) != 0 || got != 15) {
            con_puts("nearlatticelite FAIL\n");
            return;
        }
        shell_fb_ready();
        con_puts("nearlatticelite ok\n");
        return;
    }
    if (streq(line, "f64iflite")) {
        uint64_t got = 0;
        if (hc_run_src(F64IFLITE_ZC, &got) != 0 || got != 15) {
            con_puts("f64iflite FAIL\n");
            return;
        }
        con_puts("f64iflite ok\n");
        return;
    }
    if (streq(line, "wraplatticelite")) {
        uint64_t got = 0;
        if (hc_run_src(WRAPLATTICELITE_ZC, &got) != 0 || got != 15) {
            con_puts("wraplatticelite FAIL\n");
            return;
        }
        con_puts("wraplatticelite ok\n");
        return;
    }
    if (streq(line, "menulooplite")) {
        uint64_t got = 0;
        if (hc_run_src(MENULOOPLITE_ZC, &got) != 0 || got != 15) {
            con_puts("menulooplite FAIL\n");
            return;
        }
        con_puts("menulooplite ok\n");
        return;
    }
    if (streq(line, "idxalllite")) {
        uint64_t got = 0;
        if (hc_run_src(IDXALLLITE_ZC, &got) != 0 || got != 15) {
            con_puts("idxalllite FAIL\n");
            return;
        }
        con_puts("idxalllite ok\n");
        return;
    }
    if (streq(line, "disklat")) {
        size_t n = 0;
        uint64_t got = 0;
        if (rs_fmt_host(128, 7) != 0) {
            con_puts("disklat fmt FAIL\n");
            return;
        }
        if (rs_put_file("DiskLat.ZC", DISKLAT_ZC, str_len(DISKLAT_ZC)) != 0) {
            con_puts("disklat put FAIL\n");
            return;
        }
        if (rs_load_file("DiskLat.ZC", g_hc_zc_src, sizeof(g_hc_zc_src), &n) != 0) {
            con_puts("disklat load FAIL\n");
            return;
        }
        if (hc_run_src(g_hc_zc_src, &got) != 0 || got != 15) {
            con_puts("disklat FAIL\n");
            return;
        }
        shell_fb_ready();
        con_puts("disklat ok\n");
        return;
    }
    if (streq(line, "stocklat")) {
        uint64_t got = 0;
        /* M198: scripted StockLat smoke from the shell (no live MessageGet). */
        if (hc_run_src(STOCKLAT_ZC, &got) != 0 || got != 15) {
            con_puts("stocklat FAIL\n");
            return;
        }
        shell_fb_ready();
        con_puts("stocklat ok\n");
        return;
    }
    if (streq(line, "latticeplay")) {
        uint64_t got = 0;
        /* Live UTM Lattice: same DiskLat body, no scripted MsgQue — ESC to exit. */
        if (hc_lattice_play_src(DISKLAT_ZC, g_hc_zc_src, sizeof(g_hc_zc_src), 0) != 0) {
            con_puts("latticeplay build FAIL\n");
            return;
        }
        /* M179: serial/FB shell cue before the live loop (UTM Terminal 1). */
        con_puts("latticeplay: Esc Enter Space c +/- e | L-click place | R-drag aim | arrows dth/speed | 0-9\n");
        if (hc_run_src_ex(g_hc_zc_src, &got, 1) != 0) {
            con_puts("latticeplay FAIL\n");
            return;
        }
        shell_fb_ready();
        con_puts("latticeplay ok\n");
        return;
    }
    if (streq(line, "stockplay")) {
        uint64_t got = 0;
        /* M190: near-verbatim stock Lattice, no MsgQue; live MessageGet yield. */
        if (hc_lattice_play_src(STOCKLAT_ZC, g_hc_zc_src, sizeof(g_hc_zc_src), 0) != 0) {
            con_puts("stockplay build FAIL\n");
            return;
        }
        con_puts("stockplay: Esc Enter Space c +/- e | L-click place | R-drag aim | arrows dth/speed | 0-9\n");
        if (hc_run_src_ex(g_hc_zc_src, &got, 1) != 0) {
            con_puts("stockplay FAIL\n");
            return;
        }
        shell_fb_ready();
        con_puts("stockplay ok\n");
        return;
    }
    if (streq(line, "lattice")) {
        size_t n = 0;
        uint64_t got = 0;
        if (rs_fmt_host(128, 7) != 0) {
            con_puts("lattice fmt FAIL\n");
            return;
        }
        if (rs_put_file("Lattice.ZC", DISKLAT_ZC, str_len(DISKLAT_ZC)) != 0) {
            con_puts("lattice put FAIL\n");
            return;
        }
        if (rs_load_file("Lattice.ZC", g_hc_zc_src, sizeof(g_hc_zc_src), &n) != 0) {
            con_puts("lattice load FAIL\n");
            return;
        }
        if (hc_run_src(g_hc_zc_src, &got) != 0 || got != 15) {
            con_puts("lattice FAIL\n");
            return;
        }
        (void)rs_del_file("Lattice.ZC");
        shell_fb_ready();
        con_puts("lattice ok\n");
        return;
    }
    if (streq(line, "depthbuflite")) {
        uint64_t got = 0;
        if (hc_run_src(DEPTHBUFLITE_ZC, &got) != 0 || got != 15) {
            con_puts("depthbuflite FAIL\n");
            return;
        }
        con_puts("depthbuflite ok\n");
        return;
    }
    if (streq(line, "depthrstlite")) {
        uint64_t got = 0;
        if (hc_run_src(DEPTHRSTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("depthrstlite FAIL\n");
            return;
        }
        con_puts("depthrstlite ok\n");
        return;
    }
    if (streq(line, "depthplotlite")) {
        uint64_t got = 0;
        if (hc_run_src(DEPTHPLOTLITE_ZC, &got) != 0 || got != 15) {
            con_puts("depthplotlite FAIL\n");
            return;
        }
        con_puts("depthplotlite ok\n");
        return;
    }
    if (streq(line, "depthlinelite")) {
        uint64_t got = 0;
        if (hc_run_src(DEPTHLINELITE_ZC, &got) != 0 || got != 15) {
            con_puts("depthlinelite FAIL\n");
            return;
        }
        con_puts("depthlinelite ok\n");
        return;
    }
    if (streq(line, "peekplot")) {
        uint64_t got = 0;
        if (hc_run_src(PEEKPLOT_ZC, &got) != 0 || got == 0) {
            con_puts("peekplot FAIL\n");
            return;
        }
        con_puts("peekplot ok n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "offbmp")) {
        uint64_t got = 0;
        if (hc_run_src(OFFBMP_ZC, &got) != 0 || got == 0) {
            con_puts("offbmp FAIL\n");
            return;
        }
        con_puts("offbmp ok lit=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "heapstr")) {
        uint64_t got = 0;
        if (hc_run_src(HEAPSTR_ZC, &got) != 0 || got != 6) {
            con_puts("heapstr FAIL\n");
            return;
        }
        con_puts("heapstr ok n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "catfmt")) {
        uint64_t got = 0;
        if (hc_run_src(CATFMT_ZC, &got) != 0 || got != 12) {
            con_puts("catfmt FAIL\n");
            return;
        }
        con_puts("catfmt ok n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "heapque")) {
        uint64_t got = 0;
        if (hc_run_src(HEAPQUE_ZC, &got) != 0 || got != 150) {
            con_puts("heapque FAIL\n");
            return;
        }
        con_puts("heapque ok sum=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "jobque")) {
        uint64_t got = 0;
        if (hc_run_src(JOBQUE_ZC, &got) != 0 || got != 120) {
            con_puts("jobque FAIL\n");
            return;
        }
        con_puts("jobque ok sum=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "jobrun")) {
        uint64_t got = 0;
        if (hc_run_src(JOBRUN_ZC, &got) != 0 || got != 15) {
            con_puts("jobrun FAIL\n");
            return;
        }
        con_puts("jobrun ok sum=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "spawn")) {
        uint64_t got = 0;
        if (hc_run_src(TASKSPAWN_ZC, &got) != 0 || got != 42) {
            con_puts("spawn FAIL\n");
            return;
        }
        con_puts("spawn ok sum=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "popup")) {
        uint64_t got = 0;
        if (hc_run_src(POPUP_ZC, &got) != 0 || got != 27) {
            con_puts("popup FAIL\n");
            return;
        }
        con_puts("popup ok sum=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "doclite")) {
        uint64_t got = 0;
        if (hc_run_src(DOCLITE_ZC, &got) != 0 || got != 7) {
            con_puts("doclite FAIL\n");
            return;
        }
        con_puts("doclite ok n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "ramblk")) {
        uint64_t got = 0;
        if (hc_run_src(RAMBLK_ZC, &got) != 0 || got != 414) {
            con_puts("ramblk FAIL\n");
            return;
        }
        con_puts("ramblk ok sum=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "namefile")) {
        uint64_t got = 0;
        if (hc_run_src(NAMEFILE_ZC, &got) != 0 || got != 5) {
            con_puts("namefile FAIL\n");
            return;
        }
        con_puts("namefile ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "dirlook")) {
        uint64_t got = 0;
        if (hc_run_src(DIRLOOK_ZC, &got) != 0 || got != 4) {
            con_puts("dirlook FAIL\n");
            return;
        }
        con_puts("dirlook ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "dirdel")) {
        uint64_t got = 0;
        if (hc_run_src(DIRDEL_ZC, &got) != 0 || got != 2) {
            con_puts("dirdel FAIL\n");
            return;
        }
        con_puts("dirdel ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "fopen")) {
        uint64_t got = 0;
        if (hc_run_src(FOPEN_ZC, &got) != 0 || got != 4) {
            con_puts("fopen FAIL\n");
            return;
        }
        con_puts("fopen ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "fwrite")) {
        uint64_t got = 0;
        if (hc_run_src(FWRITE_ZC, &got) != 0 || got != 3) {
            con_puts("fwrite FAIL\n");
            return;
        }
        con_puts("fwrite ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "multiblk")) {
        uint64_t got = 0;
        if (hc_run_src(MULTIBLK_ZC, &got) != 0 || got != 2) {
            con_puts("multiblk FAIL\n");
            return;
        }
        con_puts("multiblk ok n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "redsea")) {
        uint64_t got = 0;
        if (hc_run_src(REDSEA_ZC, &got) != 0 || got != 7) {
            con_puts("redsea FAIL\n");
            return;
        }
        con_puts("redsea ok uid=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsroot")) {
        uint64_t got = 0;
        if (hc_run_src(RSROOT_ZC, &got) != 0 || got != 2) {
            con_puts("rsroot FAIL\n");
            return;
        }
        con_puts("rsroot ok clus=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsfile")) {
        uint64_t got = 0;
        if (hc_run_src(RSFILE_ZC, &got) != 0 || got != 4) {
            con_puts("rsfile FAIL\n");
            return;
        }
        con_puts("rsfile ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsalloc")) {
        uint64_t got = 0;
        if (hc_run_src(RSALLOC_ZC, &got) != 0 || got != 3) {
            con_puts("rsalloc FAIL\n");
            return;
        }
        con_puts("rsalloc ok clus=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsfree")) {
        uint64_t got = 0;
        if (hc_run_src(RSFREE_ZC, &got) != 0 || got != 3) {
            con_puts("rsfree FAIL\n");
            return;
        }
        con_puts("rsfree ok clus=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsmulti")) {
        uint64_t got = 0;
        if (hc_run_src(RSMULTI_ZC, &got) != 0 || got != 2) {
            con_puts("rsmulti FAIL\n");
            return;
        }
        con_puts("rsmulti ok n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rscfile")) {
        uint64_t got = 0;
        if (hc_run_src(RSCFILE_ZC, &got) != 0 || got != 4) {
            con_puts("rscfile FAIL\n");
            return;
        }
        con_puts("rscfile ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rscwrite")) {
        uint64_t got = 0;
        if (hc_run_src(RSCWRITE_ZC, &got) != 0 || got != 3) {
            con_puts("rscwrite FAIL\n");
            return;
        }
        con_puts("rscwrite ok len=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rscseek")) {
        uint64_t got = 0;
        if (hc_run_src(RSCSEEK_ZC, &got) != 0 || got != 4) {
            con_puts("rscseek FAIL\n");
            return;
        }
        con_puts("rscseek ok size=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsclib")) {
        uint64_t got = 0;
        if (hc_run_src(RSCLIB_ZC, &got) != 0 || got != 4) {
            con_puts("rsclib FAIL\n");
            return;
        }
        con_puts("rsclib ok size=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rspersist")) {
        uint64_t got = 0;
        if (rs_prepare_freeze_catalog() < 0) {
            con_puts("rspersist prep FAIL (root full; unknown files preserved)\n");
            return;
        }
        if (hc_run_src(RSCPERSIST_ZC, &got) != 0 || got != 4) {
            con_puts("rspersist FAIL\n");
            return;
        }
        con_puts("rspersist ok size=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "rsdir")) {
        (void)rs_seed_catalog(); /* best-effort; always list */
        if (rs_dir_print() < 0) {
            con_puts("rsdir FAIL\n");
        }
        return;
    }
    if (streq(line, "rscatalog")) {
        int prep = rs_prepare_freeze_catalog();
        if (prep != 0) {
            if (prep > 0) {
                con_puts("rscatalog FAIL (no RedSea volume)\n");
                return;
            }
            con_puts("rscatalog FAIL (root full; unknown files preserved)\n");
            return;
        }
        con_puts("rscatalog ok\n");
        (void)rs_dir_print();
        return;
    }
    if (str_startswith(line, "rsdel ")) {
        const char *name = skip_ws(line + 6);
        if (!*name || rs_del_file(name) != 0) {
            con_puts("rsdel FAIL\n");
            return;
        }
        con_puts("rsdel ok\n");
        return;
    }
    if (str_startswith(line, "rsrename ")) {
        char buf[64];
        char *oldn;
        char *newn;
        unsigned i;
        for (i = 0; i < sizeof(buf) && line[i]; i++) {
            buf[i] = line[i];
        }
        buf[i < sizeof(buf) ? i : sizeof(buf) - 1] = 0;
        oldn = (char *)skip_ws(buf + 9);
        newn = oldn;
        while (*newn && *newn != ' ' && *newn != '\t') {
            newn++;
        }
        if (!*newn) {
            con_puts("rsrename usage: rsrename old new\n");
            return;
        }
        *newn++ = 0;
        newn = (char *)skip_ws(newn);
        if (!*newn || rs_rename_file(oldn, newn) != 0) {
            con_puts("rsrename FAIL\n");
            return;
        }
        con_puts("rsrename ok\n");
        return;
    }
    if (streq(line, "runzc") || str_startswith(line, "runzc ")) {
        const char *name = "Hi.ZC";
        char src[4096];
        uint64_t got = 0;
        size_t n = 0;
        const struct rs_catalog_ent *cat;

        if (str_startswith(line, "runzc ")) {
            name = skip_ws(line + 6);
            if (!*name) {
                name = "Hi.ZC";
            }
        }

        if (rs_prepare_freeze_catalog() < 0) {
            con_puts("runzc prep FAIL (root full; unknown files preserved)\n");
            return;
        }
        if (hc_run_src(RUNZC_ZC, &got) != 0 || got != 10) {
            con_puts("runzc seed FAIL\n");
            return;
        }
        (void)rs_seed_catalog(); /* best-effort on possibly-full PCI root */
        if (str_ieq(name, "Notes.ZC")) {
            (void)rs_ensure_catalog_file("DocLib.ZC");
        }
        if (str_ieq(name, "UseAdd.ZC")) {
            (void)rs_ensure_catalog_file("AddLib.ZC");
        }
        (void)rs_ensure_catalog_file(name);
        if (rs_load_file(name, src, sizeof(src), &n) != 0) {
            con_puts("runzc load FAIL name=");
            con_puts(name);
            con_puts(" (rsdir; free a slot if catalog full)\n");
            return;
        }
        got = 0;
        cat = rs_catalog_lookup(name);
        if (str_ieq(name, "Hi.ZC")) {
            if (hc_run_src(src, &got) != 0 || got != 42) {
                con_puts("runzc JIT FAIL got=");
                uart_put_u64_hex(g_uart, got);
                con_puts("\n");
                return;
            }
            con_puts("runzc Hi.ZC => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
            return;
        }
        if (hc_run_src(src, &got) != 0) {
            con_puts("runzc JIT FAIL name=");
            con_puts(name);
            con_puts("\n");
            return;
        }
        if (cat && !cat->graphic && str_ieq(name, "MemSort.ZC") && got != 16) {
            con_puts("runzc MemSort bad => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
            return;
        }
        if (cat && !cat->graphic && str_ieq(name, "UseAdd.ZC") && got != 42) {
            con_puts("runzc UseAdd bad => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
            return;
        }
        if (cat && str_ieq(name, "Notes.ZC") && got != 14) {
            con_puts("runzc Notes bad => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
            return;
        }
        con_puts("runzc ");
        con_puts(name);
        con_puts(" ok n=");
        uart_put_u64_hex(g_uart, (uint64_t)n);
        if (rs_is_graphic_zc(name)) {
            con_puts(" (disk+JIT; inspect FB)\n");
            (void)hc_builtin_sleep(2000);
        } else {
            con_puts(" => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
        }
        return;
    }
    if (streq(line, "vblk")) {
        uint8_t buf[512];
        unsigned i;
        if (!virtio_blk_ready()) {
            con_puts("vblk none\n");
            return;
        }
        for (i = 0; i < 512; i++) {
            buf[i] = 0;
        }
        buf[0] = 0xA5;
        buf[1] = 0x5A;
        buf[2] = 'V';
        buf[3] = 'B';
        if (!virtio_blk_write(buf, 15, 1)) {
            con_puts("vblk write FAIL\n");
            return;
        }
        for (i = 0; i < 512; i++) {
            buf[i] = 0;
        }
        if (!virtio_blk_read(buf, 15, 1) || buf[0] != 0xA5 || buf[1] != 0x5A ||
            buf[2] != 'V' || buf[3] != 'B') {
            con_puts("vblk read FAIL\n");
            return;
        }
        con_puts("vblk ok cap=");
        uart_put_u64_hex(g_uart, virtio_blk_capacity());
        con_puts("\n");
        return;
    }
    if (streq(line, "stars")) {
        uint64_t got = 0;
        const char *src =
            "Cls(0x00000810);"
            "I64 i=0;"
            "while(i<120){"
            "  PutPixel(Rand()%FbW(),Rand()%FbH(),0x00E0E0F0);"
            "  i++;"
            "}"
            "PrintAt(2,1,\"stars\");"
            "return i;";
        if (hc_run_src(src, &got) != 0 || got != 120) {
            con_puts("stars FAIL\n");
            return;
        }
        con_puts("stars n=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "circles")) {
        uint64_t got = 0;
        const char *src =
            "Cls(0x00101820);"
            "GrFillCircle(200,200,50,0x004080C0);"
            "GrCircle(200,200,80,0x00E08020);"
            "GrCircle(400,220,60,0x0040C0F0);"
            "GrCircle(300,320,100,0x00C040A0);"
            "PrintAt(2,1,\"circles\");"
            "return 3;";
        if (hc_run_src(src, &got) != 0 || got != 3) {
            con_puts("circles FAIL\n");
            return;
        }
        con_puts("circles ok\n");
        return;
    }
    if (streq(line, "bars")) {
        uint64_t got = 0;
        const char *src =
            "Cls(0x00101820);"
            "FillRect(40,60,80,220,0x00C04040);"
            "FillRect(140,60,80,220,0x0040C040);"
            "FillRect(240,60,80,220,0x004040C0);"
            "GrLine(40,300,320,300,0x00E0E0E0,1);"
            "GrLine(40,60,320,300,0x00F0C040,1);"
            "PrintAt(2,1,\"bars\");"
            "return 3;";
        if (hc_run_src(src, &got) != 0 || got != 3) {
            con_puts("bars FAIL\n");
            return;
        }
        con_puts("bars ok\n");
        return;
    }
    if (streq(line, "bounce")) {
        uint64_t got = 0;
        /* Erase/redraw only the sprite — avoid full-screen Cls per frame. */
        const char *src =
            "PrintAt(2,1,\"bounce\");"
            "FillRect(20,40,380,280,0x00101820);"
            "I64 x=40;I64 y=80;I64 dx=5;I64 dy=3;I64 i=0;"
            "while(i<36){"
            "  FillRect(x,y,22,22,0x00E08020);"
            "  Sleep(8);"
            "  FillRect(x,y,22,22,0x00101820);"
            "  x=x+dx;y=y+dy;"
            "  if(x<20){x=20;dx=0-dx;}"
            "  if(x>360){x=360;dx=0-dx;}"
            "  if(y<50){y=50;dy=0-dy;}"
            "  if(y>280){y=280;dy=0-dy;}"
            "  i++;"
            "}"
            "FillRect(x,y,22,22,0x00F0C040);"
            "return i;";
        if (hc_run_src(src, &got) != 0 || got != 36) {
            con_puts("bounce FAIL\n");
            return;
        }
        con_puts("bounce frames=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "paint")) {
        uint64_t got = 0;
        /* Frame-bounded so headless script exits with dots=0 (no tablet hold). */
        const char *src =
            "Print(\"paint: drag; Esc quits\\n\");"
            "I64 i=0;I64 n=0;"
            "while(i<100){"
            "  if(KeyHit()){I64 k=GetKey();if(k==27)break;}"
            "  if(MouseBtn()){PutPixel(Clamp(MouseX(),0,799),Clamp(MouseY(),0,599),0x00E08020);n++;}"
            "  Sleep(10);"
            "  i++;"
            "}"
            "return n;";
        if (hc_run_src(src, &got) != 0) {
            con_puts("paint FAIL\n");
            return;
        }
        con_puts("paint dots=");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "abs")) {
        uint8_t bc[64];
        uint64_t got;
        int n = hc_build_abs(bc, sizeof(bc), -42);
        if (n > 0 && hc_run_bc(bc, (size_t)n, &got) == 0) {
            con_puts("abs(-42) => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
        } else {
            con_puts("abs FAIL\n");
        }
        return;
    }
    if (line[0] == 'h' && line[1] == 'c' && (line[2] == ' ' || line[2] == '\t')) {
        const char *src = line + 3;
        while (*src == ' ' || *src == '\t') {
            src++;
        }
        uint64_t got = 0;
        if (hc_run_src(src, &got) != 0) {
            con_puts("hc FAIL\n");
            return;
        }
        con_puts("hc => ");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
        return;
    }
    if (streq(line, "sum")) {
        /* SumTo(10) via IR — same as demo.hcbc */
        static const uint8_t sum_bc[] = {
            HC_IMM64, 10, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 0,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 1,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 2,
            HC_LABEL, 0,
            HC_LD_LOCAL, 2, HC_LD_LOCAL, 0, HC_LT, HC_JZ, 1,
            HC_LD_LOCAL, 1, HC_LD_LOCAL, 2, HC_ADD, HC_ST_LOCAL, 1,
            HC_LD_LOCAL, 2, HC_IMM64, 1, 0, 0, 0, 0, 0, 0, 0, HC_ADD,
            HC_ST_LOCAL, 2, HC_JMP, 0,
            HC_LABEL, 1, HC_LD_LOCAL, 1, HC_RET, HC_END,
        };
        uint64_t got;
        if (hc_run_bc(sum_bc, sizeof(sum_bc), &got) == 0) {
            con_puts("sum(10) => ");
            uart_put_u64_hex(g_uart, got);
            con_puts("\n");
        } else {
            con_puts("sum FAIL\n");
        }
        return;
    }
    uint64_t got;
    if (expr_compile_run(line, &got) == 0) {
        con_puts("=> ");
        uart_put_u64_hex(g_uart, got);
        con_puts("\n");
    } else {
        con_puts("?\n");
    }
}

#ifndef ZEAL_PI_DIAG
/* Run the same bounded shell commands from the graphical DolDoc terminal.
 * The terminal is an app task, so destructive/session-control commands and
 * serial-only transfer/editor flows stay on the serial bootstrap shell. */
int64_t zc_gui_shell_exec(int64_t *a) {
    const char *line = (const char *)(uintptr_t)a[0];
    const char *command = line;
    char command_buf[256];
    int done = 0;
    size_t command_len = 0;
    size_t i;

    if (!line)
        return (int64_t)(uintptr_t)"graphical shell: command is too long\n";
    while (*command == ' ' || *command == '\t')
        command++;
    if (!*command)
        return (int64_t)(uintptr_t)"";
    while (command_len <= 255 && command[command_len])
        command_len++;
    if (command_len > 255)
        return (int64_t)(uintptr_t)"graphical shell: command is too long\n";
    while (command_len && (command[command_len - 1] == ' ' ||
                           command[command_len - 1] == '\t'))
        command_len--;
    if (!command_len)
        return (int64_t)(uintptr_t)"";
    for (i = 0; i < command_len; i++)
        command_buf[i] = command[i];
    command_buf[command_len] = 0;
    command = command_buf;
    /* Match the visible OS menu's Status label at the shell prompt too. */
    if (streq(command, "status"))
        command = "zstatus";
    if (streq(command, "help"))
        return (int64_t)(uintptr_t)
            "Graphical commands: status (zstatus) | zvol | zls | zgc | zverify <path> | zload <path> | zput <name> <source> | zrm <name> | edit <path> | zc <source> | zcall <function> | zinstall [ERASE] (ISO only) | windowdemo | latticeplay | stockplay | paint\n"
#ifdef ZEAL_FORCE_PI4
            "Pi USB diagnostics: usbstatus | usbread (media remains read-only).\n"
#endif
            "OS menu: hold the pointer button, drag, then release on an item.\n"
            "Lattice controls: Esc exit | Enter restart | Space step | c colors | L-click place | R-drag aim\n"
            "Type ZealC directly, for example: 40+2;\n";
    if (streq(command, "windowdemo")) {
        if (zc_window_demo_active() || gui_window_demo_requested)
            return (int64_t)(uintptr_t)"windowdemo: already running or starting\n";
        /* Manager teardown mutates the root task ring. Defer it to the
         * bootstrap shell, then return normally to the terminal's task. */
        gui_window_demo_requested = 1;
        return (int64_t)(uintptr_t)"windowdemo: starting; terminal will hide\n";
    }
    if (!(
#ifdef ZEAL_FORCE_PI4
          streq(command, "usbstatus") || streq(command, "usbread") ||
#endif
          streq(command, "zstatus") ||
          streq(command, "zvol") || streq(command, "zls") ||
          streq(command, "zgc") ||
          streq(command, "zinstall") || str_startswith(command, "zinstall ") ||
          streq(command, "latticeplay") || streq(command, "stockplay") ||
          str_startswith(command, "zverify ") ||
          str_startswith(command, "zload ") ||
          str_startswith(command, "zcall ") ||
          streq(command, "paint") ||
          str_startswith(command, "zput ") ||
          str_startswith(command, "zrm ") ||
          str_startswith(command, "zc ")))
        return 0; /* Feed unrecognized input to the original UserTaskCont. */

    gui_shell_output_len = 0;
    gui_shell_output[0] = 0;
    gui_shell_truncated = 0;
    gui_shell_capture = 1;
    shell_handle(command, &done);
    gui_shell_capture = 0;
    if (gui_shell_truncated && gui_shell_output_len + 1 < sizeof(gui_shell_output)) {
        static const char suffix[] = "\n[output truncated]\n";
        size_t n = sizeof(suffix) - 1;
        if (n > sizeof(gui_shell_output) - gui_shell_output_len - 1)
            n = sizeof(gui_shell_output) - gui_shell_output_len - 1;
        for (size_t i = 0; i < n; i++)
            gui_shell_output[gui_shell_output_len + i] = suffix[i];
        gui_shell_output_len += n;
        gui_shell_output[gui_shell_output_len] = 0;
    }
    if (!gui_shell_output_len)
        return (int64_t)(uintptr_t)"\n";
    return (int64_t)(uintptr_t)gui_shell_output;
}
#endif

static void shell_run(const struct zeal_handoff *h) {
    (void)h;
    /* Interactive by default: no auto-script, no timeout.
     * Automated demos belong in jit_smoke / make run-serial kill, not here. */
    /* M200: RXE + drain before input (Pi USB-TTL / virt serial). */
    pl011_rx_prep();
    /* M151: wipe FB smoke residue (bars/lines) so UTM shows a clean shell. */
    /* Boot starts the source desktop before the bootstrap shell. Do not erase
     * its first frame or paint the serial shell banner over the desktop. */
    if (g_fb && !fb_console_owned_by_desktop()) {
        fb_clear(0x00101820u);
        g_fb_cx = 0;
        g_fb_cy = 0;
    }
    con_puts("\nZealOS aarch64 shell (HolyC-IR exprs)\n");
    /* M152: short FB banner; full cmd list via `help` (800x600 wraps badly). */
    con_puts("type: help | vblk | rscatalog | runzc Notes.ZC | uartrx | halt\n");
    con_puts("      hc <src> | bars | paint | nearlatticelite | disklat | latticeplay | stocklat | stockplay\n");
#ifndef ZEAL_PI_DIAG
    con_puts("native: zcheck | zload /Kernel/QuickSort.ZC | zc <source> | zstatus\n");
#else
    pi_diag_fb_report(h);
    con_puts("\nPi USB: usbstatus | usbread | usbstop; boot keyboard or serial input\n");
#endif
    con_puts("> ");
    char line[512];
    unsigned len = 0;
    int done = 0;
    while (!done) {
        int gui_has_keyboard = 0;
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
        int live_event = pi4_usb_live_poll();
        if (live_event) {
            con_puts(live_event > 0 ? "\nUSB live: 10 idle checks passed; controller remains online" :
                                    "\nUSB live: stopped after health-check failure");
            pi4_usb_live_report(0); pi4_usb_live_report(1); con_puts("\n> ");
        }
#endif
#ifndef ZEAL_PI_DIAG
        zc_idle_step();
        if (gui_window_demo_requested) {
            gui_window_demo_requested = 0;
            if (!zc_start_window_demo())
                con_puts("windowdemo: source WinMgr running with two windows and a CDC control; drag/raise/resize; Ctrl+C returns to shell\n");
            else
                con_puts("windowdemo: failed; check serial diagnostics or use zreset\n");
        }
        gui_has_keyboard = zc_focus_owns_input();
        if (gui_has_keyboard) {
            /* A focused GUI task owns normal keys, with Ctrl+C retained as a
             * keyboard escape to return to the serial bootstrap shell. The
             * serial terminal has its own UART stream, so keep polling that
             * while the VM's virtio keyboard belongs to the GUI. */
            if ((zc_window_demo_active() || zc_gui_terminal_active()) &&
                input_keyboard_take_ctrl_c()) {
                if (zc_window_demo_active()) {
                    if (zc_stop_window_demo() == 0)
                        con_puts("\nwindowdemo: stopped with Ctrl+C; shell input resumed\n> ");
                    else
                        con_puts("\nwindowdemo: Ctrl+C teardown reported an error; shell input resumed\n> ");
                } else if (zc_stop_gui_terminal() == 0)
                    con_puts("\ngterm: stopped with Ctrl+C; shell input resumed\n> ");
                else
                    con_puts("\ngterm: Ctrl+C teardown reported an error; shell input resumed\n> ");
                len = 0;
                continue;
            }
        }
#endif
        int c = gui_has_keyboard ? uart_text_getc_nb() : kbd_getc_nb();
        tablet_cursor_tick();
#ifdef ZEAL_PI_DIAG
        pi4_usb_mouse_activity_report();
#endif
        if (c < 0) {
            /* M201: yield ~1 ms (Sleep peeks UART/virtio) instead of busy-spin. */
            (void)hc_builtin_sleep(1);
            continue;
        }
        if (c == '\r' || c == '\n') {
            con_puts("\n");
            line[len] = 0;
            if (len) {
                shell_handle(line, &done);
            }
            if (!done) {
#ifndef ZEAL_PI_DIAG
                con_puts(g_zedit.active ? "edit> " : "> ");
#else
                con_puts("> ");
#endif
            }
            len = 0;
            continue;
        }
        if (c == 0x7f || c == 0x08) {
            if (len) {
                len--;
                con_backspace();
            }
            continue;
        }
        if (c >= 32 && c < 127 && len + 1 < sizeof(line)) {
            line[len++] = (char)c;
            con_write((char)c);
        }
    }
#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
    pi4_usb_live_stop();
#endif
}

static int jit_smoke(void) {
    uint8_t bc[80];
    size_t n = 0;
    uint64_t got = 0;

    /* M200: PL011 RX path — enable/drain then idle (empty FIFO). */
    pl011_rx_prep();
    if (uart_getc_nb() >= 0) {
        uart_puts(g_uart, "hc: uart RX idle FAIL\n");
        return -253;
    }
    uart_puts(g_uart, "hc: uart RX idle OK\n");

    /* Aiwnios-shaped BL relative call (a64_emit slice). */
    {
        uint32_t *p = g_jit_buf;
        *p++ = a64_stp_pre(30, 31, 31, -16); /* save LR */
        *p++ = a64_bl(3);                    /* call +3 words → movz */
        *p++ = a64_ldp_post(30, 31, 31, 16); /* restore LR */
        *p++ = a64_ret();
        *p++ = a64_movz_x(0, 42, 0);
        *p++ = a64_ret();
        size_t bytes = (size_t)((p - g_jit_buf) * 4);
        jit_icache_flush(g_jit_buf, bytes);
        got = ((uint64_t(*)(void))(void *)g_jit_buf)();
        if (got != 42) {
            uart_puts(g_uart, "hc: BL slice FAIL\n");
            return -36;
        }
        uart_puts(g_uart, "hc: BL slice => ");
        uart_put_u64_hex(g_uart, got);
        uart_puts(g_uart, "\n");
    }

    n += hc_pack_imm64(bc + n, 0x1000);
    n += hc_pack_imm64(bc + n, 0x20);
    bc[n++] = HC_ADD;
    n += hc_pack_imm64(bc + n, 0xC0DE);
    bc[n++] = HC_ADD;
    bc[n++] = HC_RET;
    bc[n++] = HC_END;
    if (hc_run_bc(bc, n, &got) != 0 || got != 0x1000ull + 0x20ull + 0xC0DEull) {
        uart_puts(g_uart, "hc: Foo FAIL\n");
        return -1;
    }
    uart_puts(g_uart, "hc: IR Foo => ");
    uart_put_u64_hex(g_uart, got);
    uart_puts(g_uart, "\n");

    n = 0;
    n += hc_pack_imm64(bc + n, 3);
    n += hc_pack_imm64(bc + n, 7);
    bc[n++] = HC_MUL;
    n += hc_pack_imm64(bc + n, 1);
    bc[n++] = HC_SUB;
    bc[n++] = HC_RET;
    bc[n++] = HC_END;
    if (hc_run_bc(bc, n, &got) != 0 || got != 20) {
        return -2;
    }
    uart_puts(g_uart, "hc: IR MulSub => ");
    uart_put_u64_hex(g_uart, got);
    uart_puts(g_uart, "\n");

    n = 0;
    n += hc_pack_call0(bc + n, (uint64_t)(uintptr_t)jit_host_puts);
    n += hc_pack_call0(bc + n, (uint64_t)(uintptr_t)hc_host_marker);
    bc[n++] = HC_RET;
    bc[n++] = HC_END;
    if (hc_run_bc(bc, n, &got) != 0 || got != 0x48434F4BULL) {
        return -3;
    }
    uart_puts(g_uart, "hc: IR Call => ");
    uart_put_u64_hex(g_uart, got);
    uart_puts(g_uart, "\n");

    /* Abs(-5) with locals+branches — packed offline (same layout as host test) */
    {
        static const uint8_t abs_bc[] = {
            /* IMM -5 */ HC_IMM64, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            /* ST_LOCAL 0 */ HC_ST_LOCAL, 0,
            /* LD_LOCAL 0 */ HC_LD_LOCAL, 0,
            /* IMM 0 */ HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0,
            HC_LT,
            /* JNZ 1 */ HC_JNZ, 1,
            /* LD_LOCAL 0; RET */ HC_LD_LOCAL, 0, HC_RET,
            /* LABEL 1 */ HC_LABEL, 1,
            /* LD; NEG; RET */ HC_LD_LOCAL, 0, HC_NEG, HC_RET,
            HC_END,
        };
        int rc = hc_run_bc(abs_bc, sizeof(abs_bc), &got);
        if (rc != 0 || got != 5) {
            uart_puts(g_uart, "hc: Abs(-5) FAIL rc=");
            uart_put_u32_dec(g_uart, rc < 0 ? (uint32_t)(-rc) : 0);
            uart_puts(g_uart, " got=");
            uart_put_u64_hex(g_uart, got);
            uart_puts(g_uart, " n=");
            uart_put_u32_dec(g_uart, (uint32_t)sizeof(abs_bc));
            uart_puts(g_uart, "\n");
            return -4;
        }
    }
    uart_puts(g_uart, "hc: Abs(-5) => ");
    uart_put_u64_hex(g_uart, got);
    uart_puts(g_uart, "\n");

    {
        static const uint8_t abs7_bc[] = {
            HC_IMM64, 7, 0, 0, 0, 0, 0, 0, 0,
            HC_ST_LOCAL, 0,
            HC_LD_LOCAL, 0,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0,
            HC_LT,
            HC_JNZ, 1,
            HC_LD_LOCAL, 0, HC_RET,
            HC_LABEL, 1,
            HC_LD_LOCAL, 0, HC_NEG, HC_RET,
            HC_END,
        };
        if (hc_run_bc(abs7_bc, sizeof(abs7_bc), &got) != 0 || got != 7) {
            uart_puts(g_uart, "hc: Abs(7) FAIL\n");
            return -5;
        }
    }
    uart_puts(g_uart, "hc: Abs(7) => ");
    uart_put_u64_hex(g_uart, got);
    uart_puts(g_uart, "\n");

    /* SumTo(5): s=0; i=0; while (i < 5) { s += i; i++; } → 10 */
    {
        static const uint8_t sum_bc[] = {
            HC_IMM64, 5, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 0,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 1,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 2,
            HC_LABEL, 0,
            HC_LD_LOCAL, 2, HC_LD_LOCAL, 0, HC_LT, HC_JZ, 1,
            HC_LD_LOCAL, 1, HC_LD_LOCAL, 2, HC_ADD, HC_ST_LOCAL, 1,
            HC_LD_LOCAL, 2, HC_IMM64, 1, 0, 0, 0, 0, 0, 0, 0, HC_ADD,
            HC_ST_LOCAL, 2, HC_JMP, 0,
            HC_LABEL, 1, HC_LD_LOCAL, 1, HC_RET, HC_END,
        };
        if (hc_run_bc(sum_bc, sizeof(sum_bc), &got) != 0 || got != 10) {
            uart_puts(g_uart, "hc: SumTo FAIL\n");
            return -6;
        }
        uart_puts(g_uart, "hc: SumTo(5) => ");
        uart_put_u64_hex(g_uart, got);
        uart_puts(g_uart, "\n");
    }

    /* Pointer: *(&slot0) = 0xAABB; return *(&slot0); */
    {
        static const uint8_t ptr_bc[] = {
            HC_SLOT_ADDR, 0,
            HC_IMM64, 0xBB, 0xAA, 0, 0, 0, 0, 0, 0,
            HC_STORE,
            HC_SLOT_ADDR, 0, HC_LOAD, HC_RET, HC_END,
        };
        g_hc_mem[0] = 0;
        if (hc_run_bc(ptr_bc, sizeof(ptr_bc), &got) != 0 || got != 0xAABB) {
            uart_puts(g_uart, "hc: Ptr FAIL got=");
            uart_put_u64_hex(g_uart, got);
            uart_puts(g_uart, "\n");
            return -7;
        }
        uart_puts(g_uart, "hc: Ptr => ");
        uart_put_u64_hex(g_uart, got);
        uart_puts(g_uart, "\n");
    }

    /* HolyC subset frontend */
    {
        uint64_t got2 = 0;
        if (hc_run_src("I64 s=0;I64 i=0;while(i<10){s=s+i;i=i+1;}return s;", &got2) != 0 ||
            got2 != 45) {
            uart_puts(g_uart, "hc: Front Sum FAIL\n");
            return -8;
        }
        uart_puts(g_uart, "hc: Front SumTo(10) => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=3;if(x<2)return 1;else return 9;", &got2) != 0 || got2 != 9) {
            uart_puts(g_uart, "hc: Front If FAIL\n");
            return -9;
        }
        uart_puts(g_uart, "hc: Front If => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 add(I64 a,I64 b){return a+b;}return add(20,22);", &got2) != 0 ||
            got2 != 42) {
            uart_puts(g_uart, "hc: Front Fn FAIL\n");
            return -11;
        }
        uart_puts(g_uart, "hc: Front Fn add => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=5;I64 p=&x;*p=7;return x;", &got2) != 0 || got2 != 7) {
            uart_puts(g_uart, "hc: Front Ptr FAIL\n");
            return -12;
        }
        uart_puts(g_uart, "hc: Front &/* => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 s=0;for(I64 i=0;i<10;i=i+1){s=s+i;}return s;", &got2) != 0 ||
            got2 != 45) {
            uart_puts(g_uart, "hc: Front For FAIL\n");
            return -13;
        }
        uart_puts(g_uart, "hc: Front For => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 s=0;I64 i=0;while(1){if(i>=5)break;s=s+i;i=i+1;}return s;",
                       &got2) != 0 ||
            got2 != 10) {
            uart_puts(g_uart, "hc: Front Break FAIL\n");
            return -14;
        }
        uart_puts(g_uart, "hc: Front Break => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 s=0;for(I64 i=0;i<10;i=i+1){if(i<5)continue;s=s+i;}return s;",
                       &got2) != 0 ||
            got2 != 35) {
            uart_puts(g_uart, "hc: Front Cont FAIL\n");
            return -15;
        }
        uart_puts(g_uart, "hc: Front Cont => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a[4];a[0]=3;a[1]=4;a[2]=5;return a[0]+a[1]+a[2];", &got2) != 0 ||
            got2 != 12) {
            uart_puts(g_uart, "hc: Front Arr FAIL\n");
            return -16;
        }
        uart_puts(g_uart, "hc: Front Arr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a[5];I64 s=0;for(I64 i=0;i<5;i=i+1){a[i]=i;}for(I64 j=0;j<5;j=j+1){"
                       "s=s+a[j];}return s;",
                       &got2) != 0 ||
            got2 != 10) {
            uart_puts(g_uart, "hc: Front ArrFor FAIL\n");
            return -17;
        }
        uart_puts(g_uart, "hc: Front ArrFor => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U8 b[4];b[0]=65;b[1]=2;b[2]=3;return b[0]+b[1]+b[2];", &got2) != 0 ||
            got2 != 70) {
            uart_puts(g_uart, "hc: Front U8 FAIL\n");
            return -18;
        }
        uart_puts(g_uart, "hc: Front U8 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return \"Az\"[0]+\"Az\"[1];", &got2) != 0 || got2 != (uint64_t)('A' + 'z')) {
            uart_puts(g_uart, "hc: Front Str FAIL\n");
            return -19;
        }
        uart_puts(g_uart, "hc: Front Str => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=2;switch(x){case 1:return 10;case 2:return 20;default:return 30;}",
                       &got2) != 0 ||
            got2 != 20) {
            uart_puts(g_uart, "hc: Front Switch FAIL\n");
            return -20;
        }
        uart_puts(g_uart, "hc: Front Switch => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=9;switch(x){case 1:return 1;default:return 99;}", &got2) != 0 ||
            got2 != 99) {
            uart_puts(g_uart, "hc: Front SwitchDef FAIL\n");
            return -21;
        }
        uart_puts(g_uart, "hc: Front SwitchDef => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class Point{I64 x;I64 y;};Point p;p.x=3;p.y=4;return p.x+p.y;",
                       &got2) != 0 ||
            got2 != 7) {
            uart_puts(g_uart, "hc: Front Class FAIL\n");
            return -22;
        }
        uart_puts(g_uart, "hc: Front Class => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class S{U8 a;I64 b;};S s;s.a=5;s.b=10;return s.a+s.b;", &got2) !=
                0 ||
            got2 != 15) {
            uart_puts(g_uart, "hc: Front ClassU8 FAIL\n");
            return -23;
        }
        uart_puts(g_uart, "hc: Front ClassU8 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class In{I64 v;};class Out{In i;I64 z;};Out o;o.i.v=5;o.z=2;return o.i.v+o.z;",
                       &got2) != 0 ||
            got2 != 7) {
            uart_puts(g_uart, "hc: Front Nest FAIL\n");
            return -24;
        }
        uart_puts(g_uart, "hc: Front Nest => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class P{I64 x;};P p;P *q=&p;q->x=9;return p.x;", &got2) != 0 ||
            got2 != 9) {
            uart_puts(g_uart, "hc: Front Arrow FAIL\n");
            return -25;
        }
        uart_puts(g_uart, "hc: Front Arrow => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 s=0;I64 i=0;do{s=s+i;i=i+1;}while(i<5);return s;", &got2) != 0 ||
            got2 != 10) {
            uart_puts(g_uart, "hc: Front Do FAIL\n");
            return -26;
        }
        uart_puts(g_uart, "hc: Front Do => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=3;x+=4;x*=2;x-=1;return x;", &got2) != 0 || got2 != 13) {
            uart_puts(g_uart, "hc: Front AsgOp FAIL\n");
            return -27;
        }
        uart_puts(g_uart, "hc: Front AsgOp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a=0;I64 b=5;return (a&&b)||(b==5);", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front Logic FAIL\n");
            return -28;
        }
        uart_puts(g_uart, "hc: Front Logic => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return (20/3)*3+(20%3);", &got2) != 0 || got2 != 20) {
            uart_puts(g_uart, "hc: Front DivMod FAIL\n");
            return -29;
        }
        uart_puts(g_uart, "hc: Front DivMod => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=0;return !x + !(x+1);", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front Not FAIL\n");
            return -30;
        }
        uart_puts(g_uart, "hc: Front Not => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=2;return x==2?40:7;", &got2) != 0 || got2 != 40) {
            uart_puts(g_uart, "hc: Front Tern FAIL\n");
            return -31;
        }
        uart_puts(g_uart, "hc: Front Tern => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return 0x10+0x2A;", &got2) != 0 || got2 != 0x3a) {
            uart_puts(g_uart, "hc: Front Hex FAIL\n");
            return -32;
        }
        uart_puts(g_uart, "hc: Front Hex => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return 'A'+59;", &got2) != 0 || got2 != 124) {
            uart_puts(g_uart, "hc: Front Char FAIL\n");
            return -33;
        }
        uart_puts(g_uart, "hc: Front Char => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=5;x++;x++;x--;return x;", &got2) != 0 || got2 != 6) {
            uart_puts(g_uart, "hc: Front Inc FAIL\n");
            return -34;
        }
        uart_puts(g_uart, "hc: Front Inc => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return (-8)>>2;", &got2) != 0 || got2 != (uint64_t)(int64_t)-2) {
            uart_puts(g_uart, "hc: Front Asr FAIL\n");
            return -35;
        }
        uart_puts(g_uart, "hc: Front Asr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=0;goto skip;x=99;skip:return x+3;", &got2) != 0 ||
            got2 != 3) {
            uart_puts(g_uart, "hc: Front Goto FAIL\n");
            return -37;
        }
        uart_puts(g_uart, "hc: Front Goto => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a=2,b=3,c=4;return a+b+c;", &got2) != 0 || got2 != 9) {
            uart_puts(g_uart, "hc: Front Multi FAIL\n");
            return -38;
        }
        uart_puts(g_uart, "hc: Front Multi => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return Abs(-11);", &got2) != 0 || got2 != 11) {
            uart_puts(g_uart, "hc: Front BuiltinAbs FAIL\n");
            return -39;
        }
        uart_puts(g_uart, "hc: Front BuiltinAbs => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("PutPixel(10,20,0x00FF00);return 1;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front PutPixel FAIL\n");
            return -40;
        }
        uart_puts(g_uart, "hc: Front PutPixel => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class Pt{I64 x;I64 y;};return sizeof(I64)+sizeof(U8)+sizeof(Pt);",
                       &got2) != 0 ||
            got2 != 8 + 1 + 16) {
            uart_puts(g_uart, "hc: Front Sizeof FAIL\n");
            return -41;
        }
        uart_puts(g_uart, "hc: Front Sizeof => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("Cls(0x101010);return 2;", &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Front Cls FAIL\n");
            return -42;
        }
        uart_puts(g_uart, "hc: Front Cls => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("FillRect(5,5,8,6,0x00C04020);return 3;", &got2) != 0 || got2 != 3) {
            uart_puts(g_uart, "hc: Front FillRect FAIL\n");
            return -43;
        }
        uart_puts(g_uart, "hc: Front FillRect => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return StrLen(\"Hello\");", &got2) != 0 || got2 != 5) {
            uart_puts(g_uart, "hc: Front StrLen FAIL\n");
            return -44;
        }
        uart_puts(g_uart, "hc: Front StrLen => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("PrintAt(2,2,\"Zeal\");return Print(\"ok\");", &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Front Print FAIL\n");
            return -45;
        }
        uart_puts(g_uart, "hc: Front Print => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a=Rand();I64 b=Rand();return a!=b;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front Rand FAIL\n");
            return -46;
        }
        uart_puts(g_uart, "hc: Front Rand => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Mouse* returns coords even if tablet idle (0,0). */
        if (hc_run_src("return MouseX()+MouseY()+MouseBtn()>=0;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front Mouse FAIL\n");
            return -47;
        }
        uart_puts(g_uart, "hc: Front Mouse => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return MouseDX()+MouseDY()==0;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front MouseD FAIL\n");
            return -61;
        }
        uart_puts(g_uart, "hc: Front MouseD => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("Sleep(2);return 8;", &got2) != 0 || got2 != 8) {
            uart_puts(g_uart, "hc: Front Sleep FAIL\n");
            return -48;
        }
        uart_puts(g_uart, "hc: Front Sleep => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Mini paint: a few samples; paints if button held. */
        if (hc_run_src("I64 i=0;while(i<4){if(MouseBtn())PutPixel(MouseX(),MouseY(),0x00F0C040);"
                       "i++;Sleep(1);}return i;",
                       &got2) != 0 ||
            got2 != 4) {
            uart_puts(g_uart, "hc: Front Paint FAIL\n");
            return -49;
        }
        uart_puts(g_uart, "hc: Front Paint => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return KeyHit()==0;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front KeyHit FAIL\n");
            return -50;
        }
        uart_puts(g_uart, "hc: Front KeyHit => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return GetKey()<0;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front GetKey FAIL\n");
            return -51;
        }
        uart_puts(g_uart, "hc: Front GetKey => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("GrLine(10,10,60,45,0x00FFFFFF,1);return 1;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front GrLine FAIL\n");
            return -52;
        }
        uart_puts(g_uart, "hc: Front GrLine => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("GrPlot(3,4,0x00AABBCC);return GrPeek(3,4);", &got2) != 0 ||
            got2 != 0x00AABBCC) {
            uart_puts(g_uart, "hc: Front GrPeek FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -93;
        }
        uart_puts(g_uart, "hc: Front GrPeek => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return GrPeek(-1,0);", &got2) != 0 || (int64_t)got2 != -1) {
            uart_puts(g_uart, "hc: Front GrPeekOOB FAIL\n");
            return -94;
        }
        if (hc_run_src("GrHLine(10,50,80,0x00FF8080);GrVLine(50,10,80,0x0080FF80);return 2;",
                       &got2) != 0 ||
            got2 != 2) {
            uart_puts(g_uart, "hc: Front GrHV FAIL\n");
            return -53;
        }
        uart_puts(g_uart, "hc: Front GrHV => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("Cls(0x00101820);return 1;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front ClsFast FAIL\n");
            return -54;
        }
        uart_puts(g_uart, "hc: Front ClsFast => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("enum {A,B=5,C};enum Color{RED=1,GREEN,BLUE};return A+B+C+RED+GREEN+BLUE;",
                       &got2) != 0 ||
            got2 != (0 + 5 + 6 + 1 + 2 + 3)) {
            uart_puts(g_uart, "hc: Front Enum FAIL\n");
            return -55;
        }
        uart_puts(g_uart, "hc: Front Enum => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return StrCmp(\"Zeal\",\"Zeal\");", &got2) != 0 || got2 != 0) {
            uart_puts(g_uart, "hc: Front StrCmp FAIL\n");
            return -56;
        }
        uart_puts(g_uart, "hc: Front StrCmp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return StrCmp(\"A\",\"B\")<0;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front StrCmpLt FAIL\n");
            return -57;
        }
        uart_puts(g_uart, "hc: Front StrCmpLt => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return StrCmp(\"Zeal\",\"Zeal\")==0 && StrCmp(\"A\",\"B\")<0;", &got2) != 0 ||
            got2 != 1) {
            uart_puts(g_uart, "hc: Front LandCmp FAIL\n");
            return -58;
        }
        uart_puts(g_uart, "hc: Front LandCmp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U8 d[4];MemCpy(d,\"Hi\",3);return d[0]+d[1]+d[2];", &got2) != 0 ||
            got2 != (uint64_t)('H' + 'i' + 0)) {
            uart_puts(g_uart, "hc: Front MemCpy FAIL\n");
            return -59;
        }
        uart_puts(g_uart, "hc: Front MemCpy => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U8 d[3];MemSet(d,7,3);return d[0]+d[1]+d[2];", &got2) != 0 || got2 != 21) {
            uart_puts(g_uart, "hc: Front MemSet FAIL\n");
            return -60;
        }
        uart_puts(g_uart, "hc: Front MemSet => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return Str2I64(\"-42\")+Str2I64(\"0x10\");", &got2) != 0 ||
            got2 != (uint64_t)(int64_t)(-42 + 0x10)) {
            uart_puts(g_uart, "hc: Front Str2I64 FAIL\n");
            return -62;
        }
        uart_puts(g_uart, "hc: Front Str2I64 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return PrintI64(-7);", &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Front PrintI64 FAIL\n");
            return -63;
        }
        uart_puts(g_uart, "hc: Front PrintI64 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a=Cnt();Sleep(1);return Cnt()>a;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front Cnt FAIL\n");
            return -64;
        }
        uart_puts(g_uart, "hc: Front Cnt => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return Min(3,9)+Max(-2,5);", &got2) != 0 || got2 != 8) {
            uart_puts(g_uart, "hc: Front MinMax FAIL\n");
            return -65;
        }
        uart_puts(g_uart, "hc: Front MinMax => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 a(I64 x){return x+1;}I64 b(I64 x){return x+2;}I64 c(I64 x){return x+3;}"
                       "I64 d(I64 x){return x+4;}I64 e(I64 x){return x+5;}"
                       "return a(1)+b(1)+c(1)+d(1)+e(1);",
                       &got2) != 0 ||
            got2 != (2 + 3 + 4 + 5 + 6)) {
            uart_puts(g_uart, "hc: Front Fns8 FAIL\n");
            return -66;
        }
        uart_puts(g_uart, "hc: Front Fns8 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return Sign(-9)+Sign(0)+Sign(4);", &got2) != 0 ||
            got2 != (uint64_t)(int64_t)(-1 + 0 + 1)) {
            uart_puts(g_uart, "hc: Front Sign FAIL\n");
            return -67;
        }
        uart_puts(g_uart, "hc: Front Sign => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return Clamp(-5,0,10)+Clamp(3,0,10)+Clamp(99,0,10);", &got2) != 0 ||
            got2 != 13) {
            uart_puts(g_uart, "hc: Front Clamp FAIL\n");
            return -68;
        }
        uart_puts(g_uart, "hc: Front Clamp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return HashStr(\"Zeal\")!=HashStr(\"OS\") && HashStr(\"Zeal\")==HashStr(\"Zeal\");",
                       &got2) != 0 ||
            got2 != 1) {
            uart_puts(g_uart, "hc: Front HashStr FAIL\n");
            return -69;
        }
        uart_puts(g_uart, "hc: Front HashStr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return Sqr(-6);", &got2) != 0 || got2 != 36) {
            uart_puts(g_uart, "hc: Front Sqr FAIL\n");
            return -70;
        }
        uart_puts(g_uart, "hc: Front Sqr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 mix(I64 a,I64 b,I64 c,I64 d){return a+b*2+c*3+d*4;}return mix(1,2,3,4);",
                       &got2) != 0 ||
            got2 != (1 + 4 + 9 + 16)) {
            uart_puts(g_uart, "hc: Front Fn4 FAIL\n");
            return -71;
        }
        uart_puts(g_uart, "hc: Front Fn4 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 f5(I64 a,I64 b,I64 c,I64 d,I64 e){return a+b+c+d+e;}return f5(1,2,3,4,5);",
                       &got2) != 0 ||
            got2 != 15) {
            uart_puts(g_uart, "hc: Front Fn5 FAIL\n");
            return -72;
        }
        uart_puts(g_uart, "hc: Front Fn5 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U32 x=40;U32 y=2;return x+y;", &got2) != 0 || got2 != 42) {
            uart_puts(g_uart, "hc: Front U32 FAIL\n");
            return -73;
        }
        uart_puts(g_uart, "hc: Front U32 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U32 x=0xFFFFFFFF;x=x+1;return x;", &got2) != 0 || got2 != 0) {
            uart_puts(g_uart, "hc: Front U32Wrap FAIL\n");
            return -73;
        }
        uart_puts(g_uart, "hc: Front U32Wrap => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U32 a[3];a[0]=1;a[1]=2;a[2]=3;return a[0]+a[1]+a[2];", &got2) != 0 ||
            got2 != 6) {
            uart_puts(g_uart, "hc: Front U32Arr FAIL\n");
            return -73;
        }
        uart_puts(g_uart, "hc: Front U32Arr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class S{U32 a;U32 b;};S s;s.a=3;s.b=4;return s.a+s.b+sizeof(S);",
                       &got2) != 0 ||
            got2 != 15) {
            /* sizeof(S)=8 after align; 3+4+8=15 */
            uart_puts(g_uart, "hc: Front U32Struct FAIL\n");
            return -73;
        }
        uart_puts(g_uart, "hc: Front U32Struct => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("Bool t=1;Bool f=0;return t&&!f;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front Bool FAIL\n");
            return -74;
        }
        uart_puts(g_uart, "hc: Front Bool => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return sizeof(U32)+sizeof(Bool)+sizeof(U8);", &got2) != 0 || got2 != 13) {
            uart_puts(g_uart, "hc: Front SizeofU FAIL\n");
            return -75;
        }
        uart_puts(g_uart, "hc: Front SizeofU => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 nest(I64 x){I64 y=x+1;return y;}I64 a=5;return nest(a)+a;", &got2) !=
                0 ||
            got2 != 11) {
            uart_puts(g_uart, "hc: Front FrameNest FAIL\n");
            return -82;
        }
        uart_puts(g_uart, "hc: Front FrameNest => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 g;I64 inc(){g=g+1;return g;}g=0;return inc()+inc();", &got2) != 0 ||
            got2 != 3) {
            uart_puts(g_uart, "hc: Front Glob FAIL\n");
            return -149;
        }
        uart_puts(g_uart, "hc: Front Glob => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(GLOBSHARE_ZC, &got2) != 0 || got2 != 8) {
            uart_puts(g_uart, "hc: Upstream GlobShare FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -150;
        }
        uart_puts(g_uart, "hc: Upstream GlobShare => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(LIFE_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream Life FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -151;
        }
        uart_puts(g_uart, "hc: Upstream Life => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("F64 a=1.5;F64 b=2.5;return ToI64(a+b);", &got2) != 0 ||
            got2 != 4) {
            uart_puts(g_uart, "hc: Front F64 FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -152;
        }
        uart_puts(g_uart, "hc: Front F64 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return ToI64(-2.0);", &got2) != 0 ||
            got2 != (uint64_t)(int64_t)-2) {
            uart_puts(g_uart, "hc: Front F64Neg FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -154;
        }
        uart_puts(g_uart, "hc: Front F64Neg => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return ((-2.0)<=2.0)+((-2.0)<0.0);", &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Front F64Cmp FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -156;
        }
        uart_puts(g_uart, "hc: Front F64Cmp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("F64 x;x=-2.0;x=x+0.5;return ToI64(x*2.0);", &got2) != 0 ||
            got2 != (uint64_t)(int64_t)-3) {
            /* -1.5*2 = -3 */
            uart_puts(g_uart, "hc: Front F64Step FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -157;
        }
        uart_puts(g_uart, "hc: Front F64Step => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        got2 = 0;
        if (hc_run_src(
                "I64 n;F64 x;n=0;x=-2.0;while(x<=2.0){n=n+1;x=x+0.5;}return n;", &got2) != 0 ||
            got2 != 9) {
            uart_puts(g_uart, "hc: Front F64While FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -155;
        }
        uart_puts(g_uart, "hc: Front F64While => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(CARTLITE_ZC, &got2) != 0 || got2 != 14) {
            uart_puts(g_uart, "hc: Upstream CartLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -153;
        }
        uart_puts(g_uart, "hc: Upstream CartLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return (1.0!=2.0)+(3.0==3.0);", &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Front F64Ne FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -164;
        }
        uart_puts(g_uart, "hc: Front F64Ne => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(VEC2LITE_ZC, &got2) != 0 || got2 != 25) {
            uart_puts(g_uart, "hc: Upstream Vec2Lite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -165;
        }
        uart_puts(g_uart, "hc: Upstream Vec2Lite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ANGLESLITE_ZC, &got2) != 0 || got2 != 12) {
            uart_puts(g_uart, "hc: Upstream AnglesLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -166;
        }
        uart_puts(g_uart, "hc: Upstream AnglesLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(COSLITE_ZC, &got2) != 0 || got2 != 63) {
            uart_puts(g_uart, "hc: Upstream CosLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -167;
        }
        uart_puts(g_uart, "hc: Upstream CosLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(SQRTLITE_ZC, &got2) != 0 || got2 != 31) {
            uart_puts(g_uart, "hc: Upstream SqrtLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -168;
        }
        uart_puts(g_uart, "hc: Upstream SqrtLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ARGLITE_ZC, &got2) != 0 || got2 != 63) {
            uart_puts(g_uart, "hc: Upstream ArgLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -169;
        }
        uart_puts(g_uart, "hc: Upstream ArgLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(COMMALITE_ZC, &got2) != 0 || got2 != 63) {
            uart_puts(g_uart, "hc: Upstream CommaLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -170;
        }
        uart_puts(g_uart, "hc: Upstream CommaLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(PLOT3LITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream Plot3Lite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -171;
        }
        uart_puts(g_uart, "hc: Upstream Plot3Lite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TOSPILITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream TosPiLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -172;
        }
        uart_puts(g_uart, "hc: Upstream TosPiLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TOSUTF8LITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream TosUtf8Lite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -252;
        }
        uart_puts(g_uart, "hc: Upstream TosUtf8Lite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(COLORLITE_ZC, &got2) != 0 || got2 != 63) {
            uart_puts(g_uart, "hc: Upstream ColorLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -173;
        }
        uart_puts(g_uart, "hc: Upstream ColorLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TURTLELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream TurtleLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -174;
        }
        uart_puts(g_uart, "hc: Upstream TurtleLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FILLLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream FillLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -175;
        }
        uart_puts(g_uart, "hc: Upstream FillLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(INITLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream InitLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -176;
        }
        uart_puts(g_uart, "hc: Upstream InitLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DEFLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DefLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -177;
        }
        uart_puts(g_uart, "hc: Upstream DefLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(PRINTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream PrintLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -178;
        }
        uart_puts(g_uart, "hc: Upstream PrintLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MSGLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MsgLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -179;
        }
        uart_puts(g_uart, "hc: Upstream MsgLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MENULITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MenuLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -180;
        }
        uart_puts(g_uart, "hc: Upstream MenuLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FINDLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream FindLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -181;
        }
        uart_puts(g_uart, "hc: Upstream FindLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FSLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream FsLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -182;
        }
        uart_puts(g_uart, "hc: Upstream FsLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(SETUPLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream SetupLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -183;
        }
        uart_puts(g_uart, "hc: Upstream SetupLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream TtLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -184;
        }
        uart_puts(g_uart, "hc: Upstream TtLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(BUFLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream BufLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -185;
        }
        uart_puts(g_uart, "hc: Upstream BufLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(INCLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream IncLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -186;
        }
        uart_puts(g_uart, "hc: Upstream IncLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DCLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DcLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -187;
        }
        uart_puts(g_uart, "hc: Upstream DcLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(LINEDCLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream LineDcLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -188;
        }
        uart_puts(g_uart, "hc: Upstream LineDcLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(GRFLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream GrfLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -189;
        }
        uart_puts(g_uart, "hc: Upstream GrfLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MOVELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MoveLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -190;
        }
        uart_puts(g_uart, "hc: Upstream MoveLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(CHECKEDLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream CheckedLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -191;
        }
        uart_puts(g_uart, "hc: Upstream CheckedLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(CMPLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream CmpLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -192;
        }
        uart_puts(g_uart, "hc: Upstream CmpLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FORINCLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream ForIncLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -193;
        }
        uart_puts(g_uart, "hc: Upstream ForIncLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MICROLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MicroLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -194;
        }
        uart_puts(g_uart, "hc: Upstream MicroLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MOVESTACKLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MoveStackLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -195;
        }
        uart_puts(g_uart, "hc: Upstream MoveStackLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ENDLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream EndLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -196;
        }
        uart_puts(g_uart, "hc: Upstream EndLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DRAWITLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DrawItLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -197;
        }
        uart_puts(g_uart, "hc: Upstream DrawItLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(LATTICELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream LatticeLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -198;
        }
        uart_puts(g_uart, "hc: Upstream LatticeLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(LOOPLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream LoopLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -199;
        }
        uart_puts(g_uart, "hc: Upstream LoopLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DEMOLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DemoLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -200;
        }
        uart_puts(g_uart, "hc: Upstream DemoLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(EVENTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream EventLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -201;
        }
        uart_puts(g_uart, "hc: Upstream EventLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(PLAYLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream PlayLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -202;
        }
        uart_puts(g_uart, "hc: Upstream PlayLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(INPUTLITE_ZC, &got2) != 0 || got2 != 31) {
            uart_puts(g_uart, "hc: Upstream InputLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -203;
        }
        uart_puts(g_uart, "hc: Upstream InputLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RIGHTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream RightLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -205;
        }
        uart_puts(g_uart, "hc: Upstream RightLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(CURSORLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream CursorLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -206;
        }
        uart_puts(g_uart, "hc: Upstream CursorLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(UPLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream UpLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -207;
        }
        uart_puts(g_uart, "hc: Upstream UpLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TICKLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream TickLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -208;
        }
        uart_puts(g_uart, "hc: Upstream TickLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FRAMELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream FrameLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -209;
        }
        uart_puts(g_uart, "hc: Upstream FrameLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(PLOTDCLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream PlotDcLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -210;
        }
        uart_puts(g_uart, "hc: Upstream PlotDcLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ABORTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream AbortLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -211;
        }
        uart_puts(g_uart, "hc: Upstream AbortLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(AIMMOVELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream AimMoveLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -212;
        }
        uart_puts(g_uart, "hc: Upstream AimMoveLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(IDLELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream IdleLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -213;
        }
        uart_puts(g_uart, "hc: Upstream IdleLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(LAYERLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream LayerLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -214;
        }
        uart_puts(g_uart, "hc: Upstream LayerLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ENDSLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream EndsLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -215;
        }
        uart_puts(g_uart, "hc: Upstream EndsLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(SPEEDLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream SpeedLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -216;
        }
        uart_puts(g_uart, "hc: Upstream SpeedLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MIDLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MidLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -217;
        }
        uart_puts(g_uart, "hc: Upstream MidLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(LIVELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream LiveLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -218;
        }
        uart_puts(g_uart, "hc: Upstream LiveLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ACCELLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream AccelLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -219;
        }
        uart_puts(g_uart, "hc: Upstream AccelLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RESTARTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream RestartLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -220;
        }
        uart_puts(g_uart, "hc: Upstream RestartLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(WIDTHLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream WidthLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -221;
        }
        uart_puts(g_uart, "hc: Upstream WidthLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(BOTHCOLORLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream BothColorLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -222;
        }
        uart_puts(g_uart, "hc: Upstream BothColorLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* M204: header string selects Edge/Mid (Edge-first must be BLACK, not YELLOW). */
        if (hc_run_src("return PopUpColor(\"Edge Color\\n\\n\");", &got2) != 0 || got2 != 0) {
            uart_puts(g_uart, "hc: PopUpColor Edge FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -254;
        }
        uart_puts(g_uart, "hc: PopUpColor Edge => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return PopUpColor(\"Mid Color\\n\\n\");", &got2) != 0 || got2 != 14) {
            uart_puts(g_uart, "hc: PopUpColor Mid FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -255;
        }
        uart_puts(g_uart, "hc: PopUpColor Mid => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MENUFULLLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MenuFullLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -223;
        }
        uart_puts(g_uart, "hc: Upstream MenuFullLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MENUBIGLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MenuBigLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -224;
        }
        uart_puts(g_uart, "hc: Upstream MenuBigLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TRYLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream TryLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -225;
        }
        uart_puts(g_uart, "hc: Upstream TryLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(STEPCOUNTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream StepCountLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -226;
        }
        uart_puts(g_uart, "hc: Upstream StepCountLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(ANGLESFULLLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream AnglesFullLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -227;
        }
        uart_puts(g_uart, "hc: Upstream AnglesFullLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(BRACEANGLESLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream BraceAnglesLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -228;
        }
        uart_puts(g_uart, "hc: Upstream BraceAnglesLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(BRACEPILITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream BracePiLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -229;
        }
        uart_puts(g_uart, "hc: Upstream BracePiLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(SETMENULITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream SetMenuLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -230;
        }
        uart_puts(g_uart, "hc: Upstream SetMenuLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(NEARLATTICELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream NearLatticeLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -231;
        }
        uart_puts(g_uart, "hc: Upstream NearLatticeLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(F64IFLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream F64IfLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -232;
        }
        uart_puts(g_uart, "hc: Upstream F64IfLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(WRAPLATTICELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream WrapLatticeLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -233;
        }
        uart_puts(g_uart, "hc: Upstream WrapLatticeLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MENULOOPLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream MenuLoopLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -234;
        }
        uart_puts(g_uart, "hc: Upstream MenuLoopLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(IDXALLLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream IdxAllLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -235;
        }
        uart_puts(g_uart, "hc: Upstream IdxAllLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Lattice MessageGet mask precedence: 1<<2|1<<5 == 36 (not 96). */
        if (hc_run_src("return 1<<MESSAGE_KEY_DOWN|1<<MESSAGE_MS_L_DOWN;", &got2) != 0 ||
            got2 != 36) {
            uart_puts(g_uart, "hc: Front MsgMask FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -204;
        }
        uart_puts(g_uart, "hc: Front MsgMask => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 f(I64 x){I64 y=x;I64 *p=&y;return *p;}return f(9);", &got2) != 0 ||
            got2 != 9) {
            uart_puts(g_uart, "hc: Front FramePtr FAIL\n");
            return -83;
        }
        uart_puts(g_uart, "hc: Front FramePtr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* ChatGPT UTM acceptance compiler checks (hex display). */
        if (hc_run_src("return sizeof(U32);", &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Accept sizeof(U32) FAIL\n");
            return -84;
        }
        uart_puts(g_uart, "hc: Accept sizeof(U32) => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U32 x=0xFFFFFFFF;x=x+1;return x;", &got2) != 0 || got2 != 0) {
            uart_puts(g_uart, "hc: Accept U32 wrap FAIL\n");
            return -85;
        }
        uart_puts(g_uart, "hc: Accept U32 wrap => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 f(I64 x){return x+1;}I64 a=5;return f(2)+a;", &got2) != 0 ||
            got2 != 8) {
            uart_puts(g_uart, "hc: Accept frame call FAIL\n");
            return -86;
        }
        uart_puts(g_uart, "hc: Accept frame call => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return true&&!false;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front TrueFalse FAIL\n");
            return -76;
        }
        uart_puts(g_uart, "hc: Front TrueFalse => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return NULL==0;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front NULL FAIL\n");
            return -77;
        }
        uart_puts(g_uart, "hc: Front NULL => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 x=7;I64 *p=&x;*p=9;return x+(p==NULL?0:1);", &got2) != 0 || got2 != 10) {
            uart_puts(g_uart, "hc: Front I64Ptr FAIL\n");
            return -78;
        }
        uart_puts(g_uart, "hc: Front I64Ptr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("const I64 k=6;return k*k;", &got2) != 0 || got2 != 36) {
            uart_puts(g_uart, "hc: Front Const FAIL\n");
            return -79;
        }
        uart_puts(g_uart, "hc: Front Const => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("volatile I64 x=3;return x+1;", &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Front Volatile FAIL\n");
            return -80;
        }
        uart_puts(g_uart, "hc: Front Volatile => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class A{I64 a;};class B{I64 b;};class C{I64 c;};class D{I64 d;};"
                       "class E{I64 e;};A a;B b;C c;D d;E e;a.a=1;b.b=2;c.c=3;d.d=4;e.e=5;"
                       "return a.a+b.b+c.c+d.d+e.e;",
                       &got2) != 0 ||
            got2 != 15) {
            uart_puts(g_uart, "hc: Front Classes8 FAIL\n");
            return -81;
        }
        uart_puts(g_uart, "hc: Front Classes8 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("class V{I64 a;I64 b;I64 c;I64 d;I64 e;};V v;v.a=1;v.b=2;v.c=3;v.d=4;v.e=5;"
                       "return v.a+v.b+v.c+v.d+v.e;",
                       &got2) != 0 ||
            got2 != 15) {
            uart_puts(g_uart, "hc: Front Members5 FAIL\n");
            return -82;
        }
        uart_puts(g_uart, "hc: Front Members5 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U8 b=0;U8 *p=&b;*p=65;return b+*p;", &got2) != 0 || got2 != 130) {
            uart_puts(g_uart, "hc: Front U8Ptr FAIL\n");
            return -83;
        }
        uart_puts(g_uart, "hc: Front U8Ptr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("union U{I64 w;U8 b;};U u;u.w=0x41;return u.b+sizeof(U);", &got2) != 0 ||
            got2 != (0x41 + 8)) {
            uart_puts(g_uart, "hc: Front Union FAIL\n");
            return -84;
        }
        uart_puts(g_uart, "hc: Front Union => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("GrCircle(100,100,20,0x00FFFFFF);return 1;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front GrCircle FAIL\n");
            return -85;
        }
        uart_puts(g_uart, "hc: Front GrCircle => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("GrFillCircle(50,50,10,0x00FF0000);return 2;", &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Front GrFillCircle FAIL\n");
            return -86;
        }
        uart_puts(g_uart, "hc: Front GrFillCircle => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return CntFrq()>1000;", &got2) != 0 || got2 != 1) {
            uart_puts(g_uart, "hc: Front CntFrq FAIL\n");
            return -87;
        }
        uart_puts(g_uart, "hc: Front CntFrq => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("return FbW();", &got2) != 0 || got2 == 0) {
            uart_puts(g_uart, "hc: Front FbWH FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -88;
        }
        uart_puts(g_uart, "hc: Front FbWH => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/NetOfDots.ZC (embedded from that file at build). */
        if (hc_run_src(NETOFDOTS_ZC, &got2) != 0) {
            uart_puts(g_uart, "hc: Upstream NetOfDots FAIL\n");
            return -89;
        }
        uart_puts(g_uart, "hc: Upstream NetOfDots => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/Lines.ZC (Clamp/Sign/Rand/FbW/KeyHit loop). */
        if (hc_run_src(LINES_ZC, &got2) != 0) {
            uart_puts(g_uart, "hc: Upstream Lines FAIL\n");
            return -90;
        }
        uart_puts(g_uart, "hc: Upstream Lines => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/MiniGr.ZC (MiniGrLib lecture slice). */
        if (hc_run_src(MINIGR_ZC, &got2) != 0) {
            uart_puts(g_uart, "hc: Upstream MiniGr FAIL\n");
            return -91;
        }
        uart_puts(g_uart, "hc: Upstream MiniGr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/MemSort.ZC (RadixSort-inspired counting sort). */
        if (hc_run_src(MEMSORT_ZC, &got2) != 0 || got2 != 16) {
            uart_puts(g_uart, "hc: Upstream MemSort FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -92;
        }
        uart_puts(g_uart, "hc: Upstream MemSort => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/PeekPlot.ZC (Life-inspired GrPeek/GrPlot). */
        if (hc_run_src(PEEKPLOT_ZC, &got2) != 0 || got2 == 0) {
            uart_puts(g_uart, "hc: Upstream PeekPlot FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -95;
        }
        uart_puts(g_uart, "hc: Upstream PeekPlot => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Front MAlloc / Free + U8* index. */
        if (hc_run_src("U8 *p=MAlloc(8);if(!p)return 0;MemSet(p,3,8);"
                       "I64 s=p[0]+p[7];p[0]=9;s=s+p[0];Free(p);return s;",
                       &got2) != 0 ||
            got2 != 15) {
            uart_puts(g_uart, "hc: Front MAlloc FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -96;
        }
        uart_puts(g_uart, "hc: Front MAlloc => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/OffBmp.ZC (MAlloc offscreen + GrPlot blit). */
        if (hc_run_src(OFFBMP_ZC, &got2) != 0 || got2 == 0) {
            uart_puts(g_uart, "hc: Upstream OffBmp FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -97;
        }
        uart_puts(g_uart, "hc: Upstream OffBmp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Front StrNew / StrCpy / StrCat. */
        if (hc_run_src("U8 *a=StrNew(\"Zeal\");U8 *b=MAlloc(8);StrCpy(b,a);StrCat(b,\"OS\");"
                       "I64 n=StrLen(b)+(StrCmp(b,\"ZealOS\")==0);Free(0);return n;",
                       &got2) != 0 ||
            got2 != 7) {
            uart_puts(g_uart, "hc: Front StrNew FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -98;
        }
        uart_puts(g_uart, "hc: Front StrNew => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/HeapStr.ZC. */
        if (hc_run_src(HEAPSTR_ZC, &got2) != 0 || got2 != 6) {
            uart_puts(g_uart, "hc: Upstream HeapStr FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -99;
        }
        uart_puts(g_uart, "hc: Upstream HeapStr => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Front CatPrint / MStrPrint. */
        if (hc_run_src("U8 *s=MStrPrint(\"%s-%d\",\"Zeal\",8);"
                       "CatPrint(s,\"/%s\",\"OS\",0);I64 n=StrLen(s);"
                       "I64 ok=StrCmp(s,\"Zeal-8/OS\")==0;Free(0);return n+ok;",
                       &got2) != 0 ||
            got2 != 10) {
            uart_puts(g_uart, "hc: Front CatPrint FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -100;
        }
        uart_puts(g_uart, "hc: Front CatPrint => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/CatFmt.ZC. */
        if (hc_run_src(CATFMT_ZC, &got2) != 0 || got2 != 12) {
            uart_puts(g_uart, "hc: Upstream CatFmt FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -101;
        }
        uart_puts(g_uart, "hc: Upstream CatFmt => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/HeapQue.ZC (QueueInit/Insert sum). */
        if (hc_run_src(HEAPQUE_ZC, &got2) != 0 || got2 != 150) {
            uart_puts(g_uart, "hc: Upstream HeapQue FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -102;
        }
        uart_puts(g_uart, "hc: Upstream HeapQue => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* Compiles/runs upstream/JobQue.ZC (QueueRemove + job sum). */
        if (hc_run_src(JOBQUE_ZC, &got2) != 0 || got2 != 120) {
            uart_puts(g_uart, "hc: Upstream JobQue FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -103;
        }
        uart_puts(g_uart, "hc: Upstream JobQue => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(JOBRUN_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream JobRun FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -104;
        }
        uart_puts(g_uart, "hc: Upstream JobRun => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(TASKSPAWN_ZC, &got2) != 0 || got2 != 42) {
            uart_puts(g_uart, "hc: Upstream TaskSpawn FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -105;
        }
        uart_puts(g_uart, "hc: Upstream TaskSpawn => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("I64 inc(I64 x){return x+1;}return Call1(&inc,41);", &got2) != 0 ||
            got2 != 42) {
            uart_puts(g_uart, "hc: Front Call1 FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -106;
        }
        uart_puts(g_uart, "hc: Front Call1 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(POPUP_ZC, &got2) != 0 || got2 != 27) {
            uart_puts(g_uart, "hc: Upstream PopUp FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -107;
        }
        uart_puts(g_uart, "hc: Upstream PopUp => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DOCLITE_ZC, &got2) != 0 || got2 != 7) {
            uart_puts(g_uart, "hc: Upstream DocLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -108;
        }
        uart_puts(g_uart, "hc: Upstream DocLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src("U8 b[8];MemSet(b,0,8);b[0]=1;b[1]=2;BlkWrite(b,1,1);"
                       "MemSet(b,0,8);BlkRead(b,1,1);return b[0]+b[1];",
                       &got2) != 0 ||
            got2 != 3) {
            uart_puts(g_uart, "hc: Front Blk FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -109;
        }
        uart_puts(g_uart, "hc: Front Blk => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RAMBLK_ZC, &got2) != 0 || got2 != 414) {
            uart_puts(g_uart, "hc: Upstream RamBlk FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -110;
        }
        uart_puts(g_uart, "hc: Upstream RamBlk => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(NAMEFILE_ZC, &got2) != 0 || got2 != 5) {
            uart_puts(g_uart, "hc: Upstream NameFile FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -111;
        }
        uart_puts(g_uart, "hc: Upstream NameFile => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DIRLOOK_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream DirLook FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -112;
        }
        uart_puts(g_uart, "hc: Upstream DirLook => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DIRDEL_ZC, &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Upstream DirDel FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -113;
        }
        uart_puts(g_uart, "hc: Upstream DirDel => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FOPEN_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream FOpen FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -114;
        }
        uart_puts(g_uart, "hc: Upstream FOpen => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(FWRITE_ZC, &got2) != 0 || got2 != 3) {
            uart_puts(g_uart, "hc: Upstream FWrite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -115;
        }
        uart_puts(g_uart, "hc: Upstream FWrite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(MULTIBLK_ZC, &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Upstream MultiBlk FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -116;
        }
        uart_puts(g_uart, "hc: Upstream MultiBlk => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(REDSEA_ZC, &got2) != 0 || got2 != 7) {
            uart_puts(g_uart, "hc: Upstream RedSea FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -117;
        }
        uart_puts(g_uart, "hc: Upstream RedSea => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSROOT_ZC, &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Upstream RSRoot FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -118;
        }
        uart_puts(g_uart, "hc: Upstream RSRoot => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSFILE_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSFile FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -119;
        }
        uart_puts(g_uart, "hc: Upstream RSFile => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSALLOC_ZC, &got2) != 0 || got2 != 3) {
            uart_puts(g_uart, "hc: Upstream RSAlloc FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -120;
        }
        uart_puts(g_uart, "hc: Upstream RSAlloc => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSFREE_ZC, &got2) != 0 || got2 != 3) {
            uart_puts(g_uart, "hc: Upstream RSFree FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -121;
        }
        uart_puts(g_uart, "hc: Upstream RSFree => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSMULTI_ZC, &got2) != 0 || got2 != 2) {
            uart_puts(g_uart, "hc: Upstream RSMulti FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -122;
        }
        uart_puts(g_uart, "hc: Upstream RSMulti => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSCFILE_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSCFile FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -123;
        }
        uart_puts(g_uart, "hc: Upstream RSCFile => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSCWRITE_ZC, &got2) != 0 || got2 != 3) {
            uart_puts(g_uart, "hc: Upstream RSCWrite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -124;
        }
        uart_puts(g_uart, "hc: Upstream RSCWrite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSCSEEK_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSCSeek FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -125;
        }
        uart_puts(g_uart, "hc: Upstream RSCSeek => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSCLIB_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSCLib FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -126;
        }
        uart_puts(g_uart, "hc: Upstream RSCLib => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSCPERSIST_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSCPersist FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -127;
        }
        uart_puts(g_uart, "hc: Upstream RSCPersist => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RSCPERSIST_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSCPersist2 FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -128;
        }
        uart_puts(g_uart, "hc: Upstream RSCPersist2 => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(RUNZC_ZC, &got2) != 0 || got2 != 10) {
            uart_puts(g_uart, "hc: Upstream RunZC seed FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -129;
        }
        uart_puts(g_uart, "hc: Upstream RunZC seed => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        {
            char src[2048];
            size_t n = 0;
            if (rs_load_file("Hi.ZC", src, sizeof(src), &n) != 0) {
                uart_puts(g_uart, "hc: Upstream RunZC load FAIL\n");
                return -130;
            }
            if (hc_run_src(src, &got2) != 0 || got2 != 42) {
                uart_puts(g_uart, "hc: Upstream RunZC JIT FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -131;
            }
            uart_puts(g_uart, "hc: Upstream RunZC Hi.ZC => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        /* Rename/del while root still has free slots (catalog fills all 8). */
        if (rs_put_file("Tmp.ZC", "return 7;", 9) != 0) {
            uart_puts(g_uart, "hc: Upstream rs put Tmp FAIL\n");
            return -141;
        }
        if (rs_rename_file("tmp.zc", "Tmp2.ZC") != 0) {
            uart_puts(g_uart, "hc: Upstream rsrename FAIL\n");
            return -142;
        }
        {
            char src[64];
            size_t n = 0;
            if (rs_load_file("TMP2.zc", src, sizeof(src), &n) != 0 ||
                hc_run_src(src, &got2) != 0 || got2 != 7) {
                uart_puts(g_uart, "hc: Upstream rsrename load FAIL\n");
                return -143;
            }
            uart_puts(g_uart, "hc: Upstream rsrename ok => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        if (rs_del_file("tmp2.zc") != 0 || rs_file_exists("Tmp2.ZC", NULL)) {
            uart_puts(g_uart, "hc: Upstream rsdel FAIL\n");
            return -144;
        }
        uart_puts(g_uart, "hc: Upstream rsdel ok\n");
        /* M115/M145: Lattice compose from RedSea (ephemeral; not freeze catalog). */
        if (rs_fmt_host(128, 7) != 0) {
            uart_puts(g_uart, "hc: Upstream DiskLat fmt FAIL\n");
            return -245;
        }
        if (rs_put_file("DiskLat.ZC", DISKLAT_ZC, str_len(DISKLAT_ZC)) != 0) {
            uart_puts(g_uart, "hc: Upstream DiskLat put FAIL\n");
            return -236;
        }
        {
            size_t n = 0;
            if (rs_load_file("DiskLat.ZC", g_hc_zc_src, sizeof(g_hc_zc_src), &n) != 0) {
                uart_puts(g_uart, "hc: Upstream DiskLat load FAIL\n");
                return -237;
            }
            if (hc_run_src(g_hc_zc_src, &got2) != 0 || got2 != 15) {
                uart_puts(g_uart, "hc: Upstream DiskLat FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -238;
            }
            uart_puts(g_uart, "hc: Upstream DiskLat => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        /* Regress the preserved UTM layout: a legacy DiskLat file occupies
         * blocks 3..18, leaving the freeze catalog above block 18. The
         * persistence demos must allocate using the volume's 128-block size,
         * retain the legacy file, and create Keep/Hi.ZC in later free blocks. */
        if (rs_seed_catalog() != 0 ||
            !rs_file_exists("DiskLat.ZC", NULL) ||
            !rs_file_exists("MemSort.ZC", NULL)) {
            uart_puts(g_uart, "hc: Upstream RSPreserve fixture FAIL\n");
            return -249;
        }
        if (hc_run_src(RSCPERSIST_ZC, &got2) != 0 || got2 != 4 ||
            hc_run_src(RSCPERSIST_ZC, &got2) != 0 || got2 != 4) {
            uart_puts(g_uart, "hc: Upstream RSPreserve Keep FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -250;
        }
        if (hc_run_src(RUNZC_ZC, &got2) != 0 || got2 != 10) {
            uart_puts(g_uart, "hc: Upstream RSPreserve Hi.ZC FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -251;
        }
        if (!rs_file_exists("DiskLat.ZC", NULL) ||
            !rs_file_exists("Keep", NULL) ||
            !rs_file_exists("Hi.ZC", NULL)) {
            uart_puts(g_uart, "hc: Upstream RSPreserve data lost\n");
            return -252;
        }
        uart_puts(g_uart, "hc: Upstream RSPreserve high alloc => 0x4\n");

        /* Model the older on-disk root: '.', DiskLat, then all five catalog
         * files. Keep+Hi need two slots, so preparation may remove only the
         * named legacy demo and must retain every catalog file. */
        if (rs_fmt_host(128, 8) != 0) {
            uart_puts(g_uart, "hc: Upstream RSPreserve slots fmt FAIL\n");
            return -253;
        }
        {
            uint8_t rootdir[512];
            int64_t *w;
            unsigned i;
            for (i = 0; i < sizeof(rootdir); i++) {
                rootdir[i] = 0;
            }
            rootdir[2] = '.';
            w = (int64_t *)(rootdir + 40);
            w[0] = 2;
            w[1] = 1;
            if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)rootdir, 2, 1)) {
                uart_puts(g_uart, "hc: Upstream RSPreserve dot FAIL\n");
                return -254;
            }
        }
        if (rs_put_file("DiskLat.ZC", DISKLAT_ZC, str_len(DISKLAT_ZC)) != 0 ||
            rs_seed_catalog() != 0 || !rs_file_exists("DiskLat.ZC", NULL)) {
            uart_puts(g_uart, "hc: Upstream RSPreserve slots fixture FAIL\n");
            return -255;
        }
        if (rs_prepare_freeze_catalog() != 0 ||
            rs_file_exists("DiskLat.ZC", NULL) ||
            !rs_file_exists("MemSort.ZC", NULL)) {
            uart_puts(g_uart, "hc: Upstream RSPreserve migration FAIL\n");
            return -256;
        }
        if (hc_run_src(RSCPERSIST_ZC, &got2) != 0 || got2 != 4 ||
            hc_run_src(RSCPERSIST_ZC, &got2) != 0 || got2 != 4 ||
            hc_run_src(RUNZC_ZC, &got2) != 0 || got2 != 10) {
            uart_puts(g_uart, "hc: Upstream RSPreserve slots smokes FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -257;
        }
        if (rs_file_exists("DiskLat.ZC", NULL) ||
            !rs_file_exists("AddLib.ZC", NULL) ||
            !rs_file_exists("UseAdd.ZC", NULL) ||
            !rs_file_exists("DocLib.ZC", NULL) ||
            !rs_file_exists("Notes.ZC", NULL) ||
            !rs_file_exists("MemSort.ZC", NULL) ||
            !rs_file_exists("Keep", NULL) || !rs_file_exists("Hi.ZC", NULL)) {
            uart_puts(g_uart, "hc: Upstream RSPreserve slots data FAIL\n");
            return -258;
        }
        uart_puts(g_uart, "hc: Upstream RSPreserve root slots => 0x4\n");

        /* A non-demo file must survive when the root cannot reserve Keep and
         * Hi.ZC after catalog seeding. Preparation should refuse, not delete. */
        if (rs_fmt_host(128, 9) != 0) {
            uart_puts(g_uart, "hc: Upstream RSPreserve unknown fmt FAIL\n");
            return -259;
        }
        {
            uint8_t rootdir[512];
            int64_t *w;
            unsigned i;
            for (i = 0; i < sizeof(rootdir); i++) {
                rootdir[i] = 0;
            }
            rootdir[2] = '.';
            w = (int64_t *)(rootdir + 40);
            w[0] = 2;
            w[1] = 1;
            if (!hc_builtin_blkwrite((uint64_t)(uintptr_t)rootdir, 2, 1) ||
                rs_put_file("User.ZC", "return 9;", 9) != 0 ||
                rs_seed_catalog() != 0) {
                uart_puts(g_uart, "hc: Upstream RSPreserve unknown fixture FAIL\n");
                return -260;
            }
        }
        if (rs_prepare_freeze_catalog() >= 0 ||
            !rs_file_exists("User.ZC", NULL) ||
            !rs_file_exists("AddLib.ZC", NULL) ||
            !rs_file_exists("MemSort.ZC", NULL)) {
            uart_puts(g_uart, "hc: Upstream RSPreserve unknown data FAIL\n");
            return -261;
        }
        uart_puts(g_uart, "hc: Upstream RSPreserve unknown preserved => 0x1\n");
        (void)rs_del_file("DiskLat.ZC");
        /* M125: same compose under Lattice.ZC name (runzc-shaped; not freeze catalog). */
        if (rs_fmt_host(128, 7) != 0) {
            uart_puts(g_uart, "hc: Upstream Lattice.ZC fmt FAIL\n");
            return -246;
        }
        if (rs_put_file("Lattice.ZC", DISKLAT_ZC, str_len(DISKLAT_ZC)) != 0) {
            uart_puts(g_uart, "hc: Upstream Lattice.ZC put FAIL\n");
            return -241;
        }
        {
            size_t n = 0;
            if (rs_load_file("Lattice.ZC", g_hc_zc_src, sizeof(g_hc_zc_src), &n) != 0) {
                uart_puts(g_uart, "hc: Upstream Lattice.ZC load FAIL\n");
                return -242;
            }
            if (hc_run_src(g_hc_zc_src, &got2) != 0 || got2 != 15) {
                uart_puts(g_uart, "hc: Upstream Lattice.ZC FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -243;
            }
            uart_puts(g_uart, "hc: Upstream Lattice.ZC => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        (void)rs_del_file("Lattice.ZC");
        /* M155: LatticePlay = DiskLat without scripted MsgQue; ESC-only for smoke. */
        {
            if (hc_lattice_play_src(DISKLAT_ZC, g_hc_zc_src, sizeof(g_hc_zc_src), 1) != 0) {
                uart_puts(g_uart, "hc: Upstream LatticePlay build FAIL\n");
                return -247;
            }
            if (hc_run_src(g_hc_zc_src, &got2) != 0) {
                uart_puts(g_uart, "hc: Upstream LatticePlay FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -248;
            }
            uart_puts(g_uart, "hc: Upstream LatticePlay => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        /* M182: stock Demo/Graphics/Lattice.ZC shape (θ/dθ_idx glyphs; scripted Esc). */
        if (hc_run_src(STOCKLAT_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream StockLat FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -249;
        }
        uart_puts(g_uart, "hc: Upstream StockLat => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* M190: StockPlay = StockLat without MsgQue; ESC-only smoke. */
        {
            if (hc_lattice_play_src(STOCKLAT_ZC, g_hc_zc_src, sizeof(g_hc_zc_src), 1) != 0) {
                uart_puts(g_uart, "hc: Upstream StockPlay build FAIL\n");
                return -250;
            }
            if (hc_run_src_ex(g_hc_zc_src, &got2, 1) != 0) {
                uart_puts(g_uart, "hc: Upstream StockPlay FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -251;
            }
            uart_puts(g_uart, "hc: Upstream StockPlay => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        if (hc_run_src(DEPTHBUFLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DepthBufLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -239;
        }
        uart_puts(g_uart, "hc: Upstream DepthBufLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DEPTHRSTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DepthRstLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -240;
        }
        uart_puts(g_uart, "hc: Upstream DepthRstLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DEPTHPLOTLITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DepthPlotLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -244;
        }
        uart_puts(g_uart, "hc: Upstream DepthPlotLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        if (hc_run_src(DEPTHLINELITE_ZC, &got2) != 0 || got2 != 15) {
            uart_puts(g_uart, "hc: Upstream DepthLineLite FAIL got=");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
            return -245;
        }
        uart_puts(g_uart, "hc: Upstream DepthLineLite => ");
        uart_put_u64_hex(g_uart, got2);
        uart_puts(g_uart, "\n");
        /* M35–M39: catalog + #include + Notes (no NetOfDots/Lines — slot budget). */
        if (rs_seed_catalog() != 0) {
            uart_puts(g_uart, "hc: Upstream rscatalog FAIL\n");
            return -132;
        }
        {
            char src[4096];
            size_t n = 0;
            if (rs_load_file("memsort.zc", src, sizeof(src), &n) != 0 || n < 500) {
                uart_puts(g_uart, "hc: Upstream RunZC load MemSort FAIL\n");
                return -139;
            }
            if (hc_run_src(src, &got2) != 0 || got2 != 16) {
                uart_puts(g_uart, "hc: Upstream RunZC MemSort FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -140;
            }
            uart_puts(g_uart, "hc: Upstream RunZC MemSort.ZC => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, "\n");
        }
        {
            char src[4096];
            size_t n = 0;
            if (rs_load_file("UseAdd.ZC", src, sizeof(src), &n) != 0) {
                uart_puts(g_uart, "hc: Upstream UseAdd load FAIL\n");
                return -145;
            }
            if (hc_run_src(src, &got2) != 0 || got2 != 42) {
                uart_puts(g_uart, "hc: Upstream UseAdd FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -146;
            }
            uart_puts(g_uart, "hc: Upstream UseAdd.ZC => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, " (#include)\n");
        }
        {
            char src[4096];
            size_t n = 0;
            if (rs_load_file("Notes.ZC", src, sizeof(src), &n) != 0) {
                uart_puts(g_uart, "hc: Upstream Notes load FAIL\n");
                return -147;
            }
            if (hc_run_src(src, &got2) != 0 || got2 != 14) {
                uart_puts(g_uart, "hc: Upstream Notes FAIL got=");
                uart_put_u64_hex(g_uart, got2);
                uart_puts(g_uart, "\n");
                return -148;
            }
            uart_puts(g_uart, "hc: Upstream Notes.ZC => ");
            uart_put_u64_hex(g_uart, got2);
            uart_puts(g_uart, " (DocLib#include)\n");
        }
        {
            int nd = rs_dir_print();
            if (nd < 3) {
                uart_puts(g_uart, "hc: Upstream rsdir FAIL n=");
                uart_put_u32_dec(g_uart, (uint32_t)(nd < 0 ? 0 : nd));
                uart_puts(g_uart, "\n");
                return -138;
            }
            uart_puts(g_uart, "hc: Upstream rsdir ok\n");
        }
    }
    {
        uint8_t bc[32];
        size_t n = 0;
        n += hc_pack_imm64(bc + n, 41);
        n += hc_pack_call1(bc + n, (uint64_t)(uintptr_t)hc_host_inc);
        bc[n++] = HC_RET;
        bc[n++] = HC_END;
        if (hc_run_bc(bc, n, &got) != 0 || got != 42) {
            uart_puts(g_uart, "hc: CALL1 FAIL\n");
            return -10;
        }
        uart_puts(g_uart, "hc: CALL1 => ");
        uart_put_u64_hex(g_uart, got);
        uart_puts(g_uart, "\n");
    }

    {
        char grown[700], readback[701];
        size_t n = 0;
        unsigned i;
        for (i = 0; i < sizeof(grown); i++) grown[i] = (char)('A' + i % 26);
        if (rs_fmt_host(16, 0x5a) != 0 ||
            rs_put_file("Grow.ZC", grown, 400) != 0 ||
            rs_put_file("Guard.ZC", "keep", 4) != 0 ||
            rs_put_file("Grow.ZC", grown, sizeof(grown)) != 0 ||
            rs_load_file("Guard.ZC", readback, sizeof(readback), &n) != 0 ||
            n != 4 || !streq(readback, "keep") ||
            rs_load_file("Grow.ZC", readback, sizeof(readback), &n) != 0 ||
            n != sizeof(grown)) {
            uart_puts(g_uart, "hc: RedSea safe grow FAIL\n");
            return -201;
        }
        for (i = 0; i < sizeof(grown); i++) {
            if (grown[i] != readback[i]) {
                uart_puts(g_uart, "hc: RedSea safe grow FAIL\n");
                return -202;
            }
        }
        if (rs_fmt_host(6, 0x5b) != 0 ||
            rs_put_file("Base.ZC", grown, 400) != 0 ||
            rs_put_file("Guard.ZC", "keep", 4) != 0 ||
            rs_put_file("Base.ZC", grown, sizeof(grown)) != -8 ||
            rs_load_file("Base.ZC", readback, sizeof(readback), &n) != 0 || n != 400 ||
            rs_load_file("Guard.ZC", readback, sizeof(readback), &n) != 0 ||
            n != 4 || !streq(readback, "keep")) {
            uart_puts(g_uart, "hc: RedSea full-grow rollback FAIL\n");
            return -203;
        }
        uart_puts(g_uart, "hc: RedSea safe grow OK\n");
    }
    uart_puts(g_uart, "hc IR OK (front+host)\n");
    return 0;
}

#define HCBC_MAGIC 0x43424348u /* "HCBC" le */

static void bc_module_run(const struct zeal_handoff *h) {
    /* Always smoke SumTo(10) from a static blob first (isolates module map issues). */
    {
        static const uint8_t sum10[] = {
            HC_IMM64, 10, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 0,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 1,
            HC_IMM64, 0, 0, 0, 0, 0, 0, 0, 0, HC_ST_LOCAL, 2,
            HC_LABEL, 0,
            HC_LD_LOCAL, 2, HC_LD_LOCAL, 0, HC_LT, HC_JZ, 1,
            HC_LD_LOCAL, 1, HC_LD_LOCAL, 2, HC_ADD, HC_ST_LOCAL, 1,
            HC_LD_LOCAL, 2, HC_IMM64, 1, 0, 0, 0, 0, 0, 0, 0, HC_ADD,
            HC_ST_LOCAL, 2, HC_JMP, 0,
            HC_LABEL, 1, HC_LD_LOCAL, 1, HC_RET, HC_END,
        };
        uint64_t got = 0;
        if (hc_run_bc(sum10, sizeof(sum10), &got) != 0 || got != 45) {
            uart_puts(g_uart, "bc: static SumTo FAIL\n");
            return;
        }
        uart_puts(g_uart, "bc: static SumTo OK\n");
    }

    if (!h->bc_virt || h->bc_size < 16) {
        uart_puts(g_uart, "bc: no module\n");
        return;
    }
    const uint8_t *raw = (const uint8_t *)(uintptr_t)h->bc_virt;
    uint32_t magic = (uint32_t)raw[0] | ((uint32_t)raw[1] << 8) |
                     ((uint32_t)raw[2] << 16) | ((uint32_t)raw[3] << 24);
    if (magic != HCBC_MAGIC) {
        uart_puts(g_uart, "bc: bad magic\n");
        return;
    }
    uint32_t off = (uint32_t)raw[4] | ((uint32_t)raw[5] << 8) |
                   ((uint32_t)raw[6] << 16) | ((uint32_t)raw[7] << 24);
    uint32_t len = (uint32_t)raw[8] | ((uint32_t)raw[9] << 8) |
                   ((uint32_t)raw[10] << 16) | ((uint32_t)raw[11] << 24);
    uint64_t expect = 0;
    for (int b = 0; b < 8; b++) {
        expect |= (uint64_t)raw[12 + b] << (8 * b);
    }
    uart_puts(g_uart, "bc: mod off=");
    uart_put_u32_dec(g_uart, off);
    uart_puts(g_uart, " len=");
    uart_put_u32_dec(g_uart, len);
    uart_puts(g_uart, "\n");
    if ((uint64_t)off + len > h->bc_size) {
        uart_puts(g_uart, "bc: truncated\n");
        return;
    }
    /* Copy out of module mapping before JIT (avoid execute/alias quirks). */
    uint8_t tmp[128];
    if (len > sizeof(tmp)) {
        uart_puts(g_uart, "bc: too big\n");
        return;
    }
    for (uint32_t i = 0; i < len; i++) {
        tmp[i] = raw[off + i];
    }
    uint64_t got = 0;
    if (hc_run_bc(tmp, len, &got) != 0) {
        uart_puts(g_uart, "bc: compile/run FAIL\n");
        return;
    }
    uart_puts(g_uart, "bc: module => ");
    uart_put_u64_hex(g_uart, got);
    if (expect && got != expect) {
        uart_puts(g_uart, " WANT ");
        uart_put_u64_hex(g_uart, expect);
        uart_puts(g_uart, " FAIL\n");
        return;
    }
    uart_puts(g_uart, " OK\n");
}

/* Paint a strip + title if Limine handed us a framebuffer. */
static void fb_smoke(const struct zeal_handoff *h) {
    (void)h;
    if (!g_fb) {
        uart_puts(g_uart, "fb: none\n");
        return;
    }
    uint32_t pxb = (g_fb_bpp + 7) / 8;
    uint64_t y0 = g_fb_h / 2;
    for (uint64_t x = 0; x < g_fb_w; x++) {
        volatile uint8_t *px = g_fb + y0 * g_fb_pitch + x * pxb;
        if (pxb >= 4) {
            px[0] = 0xC0;
            px[1] = 0x90;
            px[2] = 0x20;
            px[3] = 0x00;
        } else if (pxb == 3) {
            px[0] = 0xC0;
            px[1] = 0x90;
            px[2] = 0x20;
        }
    }
    con_puts("fb: ");
    uart_put_u32_dec(g_uart, (uint32_t)g_fb_w);
    con_puts("x");
    uart_put_u32_dec(g_uart, (uint32_t)g_fb_h);
    con_puts(" text OK\n");
}

#if defined(ZEAL_FORCE_PI4) && !defined(ZEAL_PI_DIAG)
static void pi4_desktop_scanout_present(void) {
    if (!g_pi4_scanout.active) return;
    const struct pi4_desktop_fb *p=&g_pi4_scanout.plan;
    pi4_desktop_fb_present(g_pi4_scanout.address,g_pi4_desktop_pixels,p);
    /* Firmware scanout can read noncoherent RAM. Publish only the viewport;
     * no invalidation can discard CPU-written pixels before they are cleaned. */
    for (unsigned y=p->top;y<p->top+PI4_DESKTOP_HEIGHT*p->scale;y++)
        pi4_dma_clean_bytes((uint8_t *)(uintptr_t)g_pi4_scanout.address+
            (uint64_t)y*p->pitch+p->left*4u,PI4_DESKTOP_WIDTH*p->scale*4u);
}
static void pi4_desktop_scanout_report(void) {
    if (!g_pi4_scanout.active) return;
    const struct pi4_desktop_fb *p=&g_pi4_scanout.plan;
    con_puts("\nPI-DESKTOP-2 physical width/height/pitch/scale/frames=");
    unsigned values[]={p->width,p->height,p->pitch,p->scale,(uint32_t)g_pi4_scanout.frames};
    for (unsigned i=0;i<5u;i++) {
        if(i)con_puts("/");
        pi4_xhci_route_hex(1,values[i]);
    }
    con_puts("\nPI-DESKTOP-2 logical surface: 800x600 pitch 3200\n");
}
static int pi4_desktop_prepare(void) {
    struct pi4_desktop_fb p;
    if (!g_fb || !pi4_desktop_fb_plan(g_fb_w,g_fb_h,g_fb_pitch,g_fb_bpp,&p))
        return 0;
    g_pi4_scanout.address=(volatile uint32_t *)(uintptr_t)g_fb;
    g_pi4_scanout.plan=p;
    /* Clear active physical pixels once; preserve firmware scanline padding. */
    for (unsigned y=0;y<p.height;y++) {
        volatile uint32_t *row=g_pi4_scanout.address+(uint64_t)y*(p.pitch/4u);
        for (unsigned x=0;x<p.width;x++) row[x]=0;
    }
    pi4_dma_clean_bytes((uint8_t *)(uintptr_t)g_pi4_scanout.address,(uint64_t)p.pitch*p.height);
    g_fb=(volatile uint8_t *)(void *)g_pi4_desktop_pixels;
    g_fb_w=PI4_DESKTOP_WIDTH; g_fb_h=PI4_DESKTOP_HEIGHT; g_fb_pitch=PI4_DESKTOP_WIDTH*4u;
    g_fb_cols=PI4_DESKTOP_WIDTH/8u; g_fb_rows=PI4_DESKTOP_HEIGHT/8u;
    g_fb_cx=g_fb_cy=0u;
    if (g_pi4_bus.mouse<g_pi4_bus.count) {
        g_pi4_bus.device[g_pi4_bus.mouse].mouse.x=PI4_DESKTOP_WIDTH/2u;
        g_pi4_bus.device[g_pi4_bus.mouse].mouse.y=PI4_DESKTOP_HEIGHT/2u;
    }
    fb_text_buffer_reset(); fb_clear(0u);
    g_pi4_scanout.active=1u;
    pi4_desktop_scanout_report();
    return 1;
}
#endif

#ifndef ZEAL_PI_DIAG
static __attribute__((always_inline)) inline void native_desktop_start(const struct zeal_handoff *h) {
    zc_set_source_reader(zc_read_redsea);
#ifdef ZEAL_FORCE_PI4
    zc_set_source_writer(NULL); /* Pi media remains read-only in this gate. */
#else
    zc_set_source_writer(zc_write_source);
#endif
    zc_set_key_reader(zc_key_read, zc_key_reset);
    if (h->revision >= 4 &&
        h->size >= offsetof(struct zeal_handoff, boot_media_type) +
                       sizeof(h->boot_media_type))
        zc_set_boot_media(h->boot_media_type);
    if (h->revision >= 3 && h->size >= offsetof(struct zeal_handoff, boot_media_type))
        zc_init((void *)(uintptr_t)h->sources_virt, h->sources_size, con_puts);
#ifdef ZEAL_FORCE_PI4
    con_puts("\nPI-DESKTOP-2: native ZealC / source graphics startup\n");
#endif
    if (h->revision >= 3 && h->size >= offsetof(struct zeal_handoff, boot_media_type) &&
        zc_start_graphics() == 0) {
        /* Source loading writes useful diagnostics to the bootstrap console.
         * Retire that retained shell plane before the desktop takes ownership,
         * or every later compositor frame will keep restoring the boot log
         * underneath windows. The serial UART retains the full diagnostics. */
        if (g_fb)
            fb_clear(0x00101820u);
        if (zc_start_winmgr() == 0) {
            uart_puts(g_uart, "zc: source Window Manager started at boot\n");
            if (zc_start_gui_terminal() == 0)
            {
                uart_puts(g_uart, "zc: graphical terminal started at boot; serial shell remains available\n");
                if (zc_start_home_startup() != 0)
                    uart_puts(g_uart, "zc: HomeSys Once startup could not be queued\n");
            }
            else
                uart_puts(g_uart, "zc: graphical terminal auto-start failed; type gterm to retry\n");
        } else
            con_puts("zc: source Window Manager auto-start failed; use winmgr to retry\n");
    }
}
#endif

void kernel_entry(const struct zeal_handoff *h) {
    if (!h || h->magic0 != ZEAL_HANDOFF_MAGIC_0 || h->magic1 != ZEAL_HANDOFF_MAGIC_1) {
        for (;;) {
            __asm__ volatile("wfi");
        }
    }

    if (h->revision >= 7 &&
        h->size >= offsetof(struct zeal_handoff, install_image_size) +
                       sizeof(h->install_image_size)) {
        g_install_image = (const uint8_t *)(uintptr_t)h->install_image_virt;
        g_install_image_size = h->install_image_size;
    }

    int is_pi = (h->flags & ZEAL_FLAG_PLAT_PI4) != 0;
#ifdef ZEAL_FORCE_PI4
    is_pi = 1;
#endif

    g_hhdm = h->hhdm_offset;
    g_uart = NULL;

    /* Enable FP/SIMD at EL1 so JIT F64 ops (FADD/SCVTF/…) don't undef-insn. */
    {
        uint64_t cpacr;
        __asm__ volatile("mrs %0, cpacr_el1" : "=r"(cpacr));
        cpacr |= (3ull << 20); /* FPEN = 0b11 */
        __asm__ volatile("msr cpacr_el1, %0\n\tisb" ::"r"(cpacr));
    }

    /* Consume the explicit page-table reservation from the Limine handoff. */
    {
        uint64_t pool = 0;
        for (uint32_t i = 0; i < h->mem_count; i++) {
            if (h->mem[i].type != ZEAL_MEM_PAGETABLE ||
                h->mem[i].length != 8ull * 4096ull ||
                (h->mem[i].base & 0xfffull)) {
                continue;
            }
            pool = h->mem[i].base;
            break;
        }
        if (pool) {
            mmio_pt_set_pool(pool);
        }
    }

#if defined(ZEAL_PI_DIAG) || defined(ZEAL_FORCE_PI4)
    if (is_pi) {
        /*
         * PI-DIAG-2: prove EL1 + PL011 TX. Magenta = entry; green = map.
         * Still no GIC / virtio / JIT.
         */
        fb_init_ex(h, 0);
        if (g_fb && g_fb_h > 140) {
            fb_fillrect(0, 120, (uint32_t)(g_fb_w > 600 ? 600 : g_fb_w), 20, 0x00C000C0u);
        }
        con_puts("KENTRY PI-DIAG-2\n");
        vectors_install();

        if (h->uart_phys) {
            /*
             * Prefer Limine's existing HHDM mapping first (no PT surgery).
             * Our Device punch was causing Address Size Faults on L2.
             */
            g_uart = (volatile uint8_t *)(h->hhdm_offset + h->uart_phys);
            if (g_fb && g_fb_h > 160) {
                fb_fillrect(0, 144, (uint32_t)(g_fb_w > 600 ? 600 : g_fb_w), 12, 0x0040C040u);
            }
        }

        if (g_uart) {
            uint32_t bw = (uint32_t)(g_fb_w > 600 ? 600 : g_fb_w);
            int fr_ok = 0, dr_ok = 0;
            /* Colors as 0x00BBGGRR (low byte = FB byte0 = Blue). */
            const uint32_t COL_OK = 0x0000E000u;   /* green */
            const uint32_t COL_BAD = 0x00E00000u;  /* red */
            const uint32_t COL_INFO = 0x00E0E000u; /* yellow */
            const uint32_t COL_DFSC = 0x00E000E0u; /* magenta */

            /* --- Probe A: FR read (32-bit) --- */
            g_uart_probe = 1;
            g_sync_recover = 1;
            g_uart_dead = 0;
            g_probe_esr = 0;
            g_probe_far = 0;
            {
                volatile uint32_t *r = (volatile uint32_t *)(uintptr_t)g_uart;
                (void)r[PL011_FR / 4];
                __asm__ volatile("dsb sy");
            }
            fr_ok = !g_uart_dead;
            g_sync_recover = 0;
            g_uart_probe = 0;
            if (g_fb && g_fb_h > 180) {
                fb_fillrect(0, 168, bw, 10, fr_ok ? COL_OK : COL_BAD);
            }

            /* If raw HHDM aborted, punch Device mapping and retry FR once. */
            if (!fr_ok && g_pt_pool_phys) {
                if (mmio_map_2m(h->hhdm_offset, h->uart_phys) == 0) {
                    g_uart = (volatile uint8_t *)(h->hhdm_offset + h->uart_phys);
                    g_uart_probe = 1;
                    g_sync_recover = 1;
                    g_uart_dead = 0;
                    g_probe_esr = 0;
                    {
                        volatile uint32_t *r = (volatile uint32_t *)(uintptr_t)g_uart;
                        (void)r[PL011_FR / 4];
                        __asm__ volatile("dsb sy");
                    }
                    fr_ok = !g_uart_dead;
                    g_sync_recover = 0;
                    g_uart_probe = 0;
                    if (g_fb && g_fb_h > 180) {
                        /* overwrite FR bar: green if Device retry worked */
                        fb_fillrect(0, 168, bw, 10, fr_ok ? COL_OK : COL_BAD);
                    }
                }
            }

            /* --- Probe B: DR byte write --- */
            g_uart_probe = 1;
            g_sync_recover = 1;
            g_uart_dead = 0;
            if (!fr_ok) {
                /* keep first ESR */
            } else {
                g_probe_esr = 0;
                g_probe_far = 0;
            }
            g_uart[PL011_DR] = (uint8_t)'!';
            __asm__ volatile("dsb sy");
            dr_ok = !g_uart_dead;
            g_sync_recover = 0;
            g_uart_probe = 0;
            if (g_fb && g_fb_h > 190) {
                fb_fillrect(0, 180, bw, 10, dr_ok ? COL_OK : COL_BAD);
            }

            if (fr_ok && dr_ok) {
                g_pi4_uart_tx_ok = 1;
                g_uart_dead = 0;
                pl011_enable_txrx(g_uart);
                uart_puts(g_uart, "\nhello from EL");
                uart_write(g_uart, (char)('0' + (int)h->boot_el));
                uart_puts(g_uart, " (PI-DIAG-2)\n");
                uart_flush(g_uart);
                if (h->revision >= 5 &&
                    h->size >= offsetof(struct zeal_handoff, pi4_pcie) +
                               sizeof(h->pi4_pcie)) {
                    uart_puts(g_uart, "PI4 FDT status=");
                    uart_put_u64_hex(g_uart, h->pi4_pcie.fdt_status);
                    uart_puts(g_uart, " PCIe flags=");
                    uart_put_u64_hex(g_uart, h->pi4_pcie.flags);
                    uart_puts(g_uart, " regs=");
                    uart_put_u64_hex(g_uart, h->pi4_pcie.reg_base);
                    uart_puts(g_uart, "/");
                    uart_put_u64_hex(g_uart, h->pi4_pcie.reg_size);
                    uart_puts(g_uart, " mem=");
                    uart_put_u64_hex(g_uart, h->pi4_pcie.mem_cpu_base);
                    uart_puts(g_uart, "<-bus:");
                    uart_put_u64_hex(g_uart, h->pi4_pcie.mem_bus_base);
                    uart_puts(g_uart, "/");
                    uart_put_u64_hex(g_uart, h->pi4_pcie.mem_size);
                    uart_puts(g_uart, " dma=");
                    uart_put_u64_hex(g_uart, h->pi4_pcie.dma_cpu_base);
                    uart_puts(g_uart, "<-bus:");
                    uart_put_u64_hex(g_uart, h->pi4_pcie.dma_bus_base);
                    uart_puts(g_uart, "/");
                    uart_put_u64_hex(g_uart, h->pi4_pcie.dma_size);
                    uart_puts(g_uart, " (read-only discovery)\n");

                    if (h->revision >= 6 &&
                        h->size >= offsetof(struct zeal_handoff, pi4_usb_dma) +
                                       sizeof(h->pi4_usb_dma)) {
                        uart_puts(g_uart, "Pi USB DMA arena flags=");
                        uart_put_u64_hex(g_uart, h->pi4_usb_dma.flags);
                        uart_puts(g_uart, " phys=");
                        uart_put_u64_hex(g_uart, h->pi4_usb_dma.phys_base);
                        uart_puts(g_uart, " bus=");
                        uart_put_u64_hex(g_uart, h->pi4_usb_dma.bus_base);
                        uart_puts(g_uart, " size=");
                        uart_put_u64_hex(g_uart, h->pi4_usb_dma.size);
                        uart_puts(g_uart, " (reserved only; xHCI off)\n");
                    } else {
                        uart_puts(g_uart, "Pi USB DMA arena: no rev-6 reservation\n");
                    }

                }
                if (g_fb && g_fb_h > 300) {
                    fb_fillrect(0, 280, bw, 12, 0x00E0E0E0u); /* white = UART OK */
                }
                con_puts("UART TX OK\n");
            } else {
                uint32_t ec = (uint32_t)((g_probe_esr >> 26) & 0x3f);
                uint32_t dfsc = (uint32_t)(g_probe_esr & 0x3f);
                if (g_fb && g_fb_h > 310) {
                    uint32_t w = 40u + (ec * 8u);
                    if (w > bw) {
                        w = bw;
                    }
                    fb_fillrect(0, 280, w, 12, COL_INFO);
                    uint32_t w2 = 40u + (dfsc * 10u);
                    if (w2 > bw) {
                        w2 = bw;
                    }
                    fb_fillrect(0, 294, w2, 10, COL_DFSC);
                }
                con_puts("UART MMIO abort — continue soft timer\n");
            }
        } else {
            if (g_fb && g_fb_h > 180) {
                fb_fillrect(0, 168, (uint32_t)(g_fb_w > 600 ? 600 : g_fb_w), 14, 0x00E00000u);
            }
            con_puts("UART none — continue soft timer\n");
        }

        /* Keep HDMI PCIe/FDT diagnostics useful even when UART is absent or
         * its probe fails. The endpoint remains read-only and bus mastering
         * and xHCI are deliberately untouched. */
        pi4_pcie_probe_readonly(h);
        pi4_dma_cpu_cache_probe(h);
        pi4_xhci_dma_noop_probe(h);
        pi4_pcie_readback_after(h);
        uart_puts(g_uart, "Pi DMA CPU/cache=");
        uart_puts(g_uart, pi4_dma_cpu_result());
        if (g_pi4_dma_probe_line) {
            uart_puts(g_uart, " line=");
            uart_put_u64_hex(g_uart, g_pi4_dma_probe_line);
        }
        uart_puts(g_uart, g_pi4_usb_live.active ? " (xHCI live; DMA arena retained)\n" :
                                               " (CPU-only; xHCI stopped)\n");
        pi4_xhci_route_report(0);
        uart_puts(g_uart, "\nPi xHCI DMA no-op=");
        uart_puts(g_uart, pi4_xhci_dma_result());
        uart_puts(g_uart, " events=");
        uart_put_u32_dec(g_uart, g_pi4_xhci_dma_events);
        uart_puts(g_uart, " PCIe BAR2 lo/origHi/testHi/restoredHi/state=");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_bar2_lo);
        uart_puts(g_uart, "/");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_bar2_hi_original);
        uart_puts(g_uart, "/");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_bar2_hi_test);
        uart_puts(g_uart, "/");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_bar2_hi_restored);
        uart_puts(g_uart, "/");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_bar2_state);
        uart_puts(g_uart, " DMA alias bias=");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_bus_bias);
        uart_puts(g_uart, " final PCI cmd/status=");
        uart_put_u64_hex(g_uart, g_pi4_cfg_command);
        uart_puts(g_uart, " commands=");
        uart_put_u32_dec(g_uart, g_pi4_xhci_dma_command_completions);
        uart_puts(g_uart, " slot-id=");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_slot_id);
        uart_puts(g_uart, " active=");
        uart_put_u32_dec(g_uart, g_pi4_xhci_dma_slot_enabled);
        if (g_pi4_xhci_dma_stage >= 17) {
            uart_puts(g_uart, "\nHub probe phase/port=");
            uart_put_u32_dec(g_uart, g_pi4_xhci_dma_hub_probe_phase);
            uart_puts(g_uart, "/");
            uart_put_u32_dec(g_uart, g_pi4_xhci_dma_hub_probe_port);
            uart_puts(g_uart, " request type/request/value/index/length=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_request_type);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_request);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_request_value);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_request_index);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_request_length);
            uart_puts(g_uart, " expected Status TRB=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_expected_status_bus);
            uart_puts(g_uart, " power/status masks=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_port_power_events);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_port_status_events);
            uart_puts(g_uart, " unpowered mask=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_port_unpowered_mask);
            uart_puts(g_uart, " connected mask=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_port_connected_mask);
            uart_puts(g_uart, "\n");
        }
        if (g_pi4_xhci_dma_stage >= 20) {
            uart_puts(g_uart, "Hub child reset=");
            uart_puts(g_uart, pi4_xhci_dma_hub_child_reset_result());
            uart_puts(g_uart, " port/samples/request/clear=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_child_reset_port);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_child_reset_samples);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_child_reset_request_rc);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_child_clear_rc);
            uart_puts(g_uart, " reset status/change=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_child_reset_status);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_child_reset_change);
            uart_puts(g_uart, " final status/change=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_child_final_status);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_child_final_change);
            uart_puts(g_uart, "\n");
        }
        if (g_pi4_xhci_dma_stage >= 21) {
            uart_puts(g_uart, "Hub Configure Endpoint status/control=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_config_status);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_config_control);
            uart_puts(g_uart, " guard No-op status/control=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_guard_status);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_guard_control);
            uart_puts(g_uart, " cfg input[0..7]=");
            for (unsigned i = 0; i < 8u; i++) {
                if (i) uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_hub_config_input_words[i]);
            }
            uart_puts(g_uart, " cfg TRB[0..3]=");
            for (unsigned i = 0; i < 4u; i++) {
                if (i) uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_hub_config_trb_words[i]);
            }
            uart_puts(g_uart, " CRCR/STS/DB=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_config_crcr[0]);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_config_crcr[1]);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_config_usbsts);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_config_db);
            uart_puts(g_uart, " output slot=");
            for (unsigned i = 0; i < 3u; i++) {
                if (i) uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_hub_output_slot_words[i]);
            }
            uart_puts(g_uart, "\nChild speed/route/slot=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_child_speed_id);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_child_route_string);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_child_slot_id);
            uart_puts(g_uart, " EnableSlot status/control=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_child_enable_status);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_child_enable_control);
            uart_puts(g_uart, " AddressDevice status/control=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_child_address_status);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_child_address_control);
            uart_puts(g_uart, " USB-address/slot-state=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_child_address);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_child_slot_state);
            uart_puts(g_uart, " DMA output/EP0=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_child_output_bus);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_child_ep0_bus);
            uart_puts(g_uart, " ctx add/slot0-2/EP0d1-2=");
            for (unsigned i = 0; i < 6u; i++) {
                if (i) uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_address_context_words[i]);
            }
            if (g_pi4_xhci_dma_stage >= 24) {
                uart_puts(g_uart,
                    "\nChild Device Descriptor prefix rc/valid/buffer/status=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_descriptor_transfer_rc);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_descriptor_validated);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_descriptor_buffer_bus);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_descriptor_status_bus);
                uart_puts(g_uart, " setup/data/status ctrl=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_descriptor_setup_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_descriptor_data_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_descriptor_status_control);
                uart_puts(g_uart, " bytes=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_descriptor_words[0]);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_descriptor_words[1]);
            }
            if (g_pi4_xhci_dma_stage >= 25) {
                uart_puts(g_uart,
                    "\nChild repeated Device Descriptor rc/valid/observed/event/checks/words=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_transfer_rc);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_validated);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_observed);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_event_status);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_event_checks);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_words[0]);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_words[1]);
                uart_puts(g_uart, " event-param/control=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_event_parameter);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_event_control);
                uart_puts(g_uart, " buffer/status=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_buffer_bus);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_status_bus);
                uart_puts(g_uart, " setup/data/status ctrl=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_setup_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_data_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_repeat_descriptor_status_control);
            }
            if (g_pi4_xhci_dma_stage >= 26) {
                uart_puts(g_uart,
                    "\nChild pre-Evaluate GET_CONFIGURATION rc/event/checks=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_transfer_rc);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_status);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_checks);
                uart_puts(g_uart, " observed/word/buffer/status=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_observed);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_word);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_buffer_bus);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_status_bus);
                uart_puts(g_uart, " event-param/control=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_parameter);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_event_control);
                uart_puts(g_uart, " setup/data/status ctrl=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_setup_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_data_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_pre_evaluate_get_configuration_status_control);
            }
            if (g_pi4_xhci_dma_stage >= 27) {
                uart_puts(g_uart,
                    "\nChild EP0 Evaluate status/control requested/output MPS=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_ep0_evaluate_status);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_ep0_evaluate_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_ep0_evaluate_mps);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_ep0_evaluate_output_mps);
                uart_puts(g_uart, " command-type=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_ep0_mps_command);
            }
            if (g_pi4_xhci_dma_stage >= 28) {
                uart_puts(g_uart,
                    "\nChild EP0 GET_STATUS rc/event/checks=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_status_transfer_rc);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_status_event_status);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_status_event_checks);
                uart_puts(g_uart, " observed/word/buffer/status=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_status_observed);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_status_word);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_status_buffer_bus);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_status_status_bus);
                uart_puts(g_uart, " event-param/control=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_status_event_parameter);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_status_event_control);
                uart_puts(g_uart, " setup/data/status ctrl=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_status_setup_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_status_data_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_status_status_control);
            }
            if (g_pi4_xhci_dma_stage >= 29) {
                uart_puts(g_uart,
                    "\nChild full Device Descriptor rc/valid/observed/buffer/status=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_full_descriptor_transfer_rc);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_full_descriptor_validated);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_full_descriptor_observed);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_full_descriptor_buffer_bus);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_full_descriptor_status_bus);
                uart_puts(g_uart, " descriptor words=");
                for (unsigned i = 0; i < 5u; i++) {
                    if (i) uart_puts(g_uart, "/");
                    uart_put_u64_hex(g_uart,
                        g_pi4_xhci_dma_child_full_descriptor_words[i]);
                }
            }
            if (g_pi4_xhci_dma_stage >= 30) {
                uart_puts(g_uart,
                    "\nChild Configuration Header rc/valid/observed/event/checks=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_header_transfer_rc);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_header_validated);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_header_observed);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_header_event_status);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_header_event_checks);
                uart_puts(g_uart, " words=");
                for (unsigned i = 0; i < 3u; i++) {
                    if (i) uart_puts(g_uart, "/");
                    uart_put_u64_hex(g_uart,
                        g_pi4_xhci_dma_child_config_header_words[i]);
                }
                uart_puts(g_uart, " event-param/control=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_header_event_parameter);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_header_event_control);
                uart_puts(g_uart, " buffer/status=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_header_buffer_bus);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_header_status_bus);
                uart_puts(g_uart, " setup/data/status ctrl=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_header_setup_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_header_data_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_header_status_control);
            }
            if (g_pi4_xhci_dma_stage >= 31) {
                const struct pi4_xhci_dma_config_summary *summary =
                    &g_pi4_xhci_dma_child_config_summary;
                uart_puts(g_uart,
                    "\nChild Configuration Descriptors rc/valid/observed/event/checks=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_full_transfer_rc);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_full_validated);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_full_observed);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_full_event_status);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_config_full_event_checks);
                uart_puts(g_uart, " words=");
                for (unsigned i = 0; i < 3u; i++) {
                    if (i) uart_puts(g_uart, "/");
                    uart_put_u64_hex(g_uart,
                        g_pi4_xhci_dma_child_config_full_words[i]);
                }
                uart_puts(g_uart, " total/value/if-desc/unique/ep-desc=");
                uart_put_u64_hex(g_uart, summary->total_length);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->configuration_value);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->interface_descriptors);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->unique_interfaces);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->endpoint_descriptors);
                uart_puts(g_uart, " first-if num/class/sub/proto/eps=");
                uart_put_u64_hex(g_uart, summary->first_interface_number);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->first_interface_class);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->first_interface_subclass);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->first_interface_protocol);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    summary->first_interface_endpoint_count);
                uart_puts(g_uart, " first-ep addr/attr/maxpacket/interval=");
                uart_put_u64_hex(g_uart, summary->first_endpoint_address);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->first_endpoint_attributes);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    summary->first_endpoint_max_packet);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    summary->first_endpoint_interval);
                uart_puts(g_uart,
                    " BOT found/if/alt/eps IN addr/attr/mps OUT addr/attr/mps=");
                uart_put_u64_hex(g_uart, summary->bot_interface_found);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->bot_interface_number);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->bot_interface_alt);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->bot_interface_endpoint_count);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->bot_bulk_in_address);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->bot_bulk_in_attributes);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->bot_bulk_in_max_packet);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->bot_bulk_out_address);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->bot_bulk_out_attributes);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart, summary->bot_bulk_out_max_packet);
            }
            if (g_pi4_xhci_dma_stage >= 32) {
                uart_puts(g_uart,
                    "\nChild SET_CONFIGURATION rc/event/checks=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_set_config_transfer_rc);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_set_config_event_status);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_set_config_event_checks);
                uart_puts(g_uart, " setup/status ctrl=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_set_config_setup_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_set_config_status_control);
                uart_puts(g_uart, " event-param/control/status-bus=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_set_config_event_parameter);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_set_config_event_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_set_config_status_bus);
            }
            if (g_pi4_xhci_dma_stage >= 33) {
                uart_puts(g_uart,
                    "\nChild GET_CONFIGURATION rc/observed/value/event/checks=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_config_transfer_rc);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_config_observed);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_config_value);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_config_event_status);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_config_event_checks);
                uart_puts(g_uart, " setup/data/status ctrl=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_config_setup_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_config_data_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_config_status_control);
                uart_puts(g_uart, " event-param/control/buffer/status=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_config_event_parameter);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_config_event_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_config_buffer_bus);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_config_status_bus);
            }
            if (g_pi4_xhci_dma_stage >= 34) {
                uart_puts(g_uart,
                    "\nChild BOT GET_MAX_LUN rc/observed/value/event/checks=");
                uart_put_u64_hex(g_uart,
                    (uint32_t)g_pi4_xhci_dma_child_get_max_lun_transfer_rc);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_max_lun_observed);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_max_lun_value);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_max_lun_event_status);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_max_lun_event_checks);
                uart_puts(g_uart, " prepare=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_max_lun_prepare);
                uart_puts(g_uart, " event-param/control=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_max_lun_event_parameter);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_max_lun_event_control);
                uart_puts(g_uart, " setup/data/status ctrl=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_max_lun_setup_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_max_lun_data_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_max_lun_status_control);
                uart_puts(g_uart, " buffer/status=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_max_lun_buffer_bus);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_get_max_lun_status_bus);
                uart_puts(g_uart,
                    "\nChild bulk guard rc/status/control=");
                uart_put_u64_hex(g_uart,
                    (uint32_t)g_pi4_xhci_dma_child_bulk_guard_rc);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_bulk_guard_status);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_bulk_guard_control);
                uart_puts(g_uart,
                    " Configure Endpoint rc/status/control/verified=");
                uart_put_u64_hex(g_uart,
                    (uint32_t)g_pi4_xhci_dma_child_bulk_config_rc);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_bulk_config_status);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_bulk_config_control);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_bulk_contexts_verified);
                uart_puts(g_uart, " rings IN/OUT=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_bulk_in_ring_bus);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_child_bulk_out_ring_bus);
                uart_puts(g_uart, " slot context=");
                for (unsigned i = 0; i < 6u; i++) {
                    if (i) uart_puts(g_uart, "/");
                    uart_put_u64_hex(g_uart,
                        g_pi4_xhci_dma_child_bulk_slot_words[i]);
                }
                uart_puts(g_uart, " bulk IN context=");
                for (unsigned i = 0; i < 5u; i++) {
                    if (i) uart_puts(g_uart, "/");
                    uart_put_u64_hex(g_uart,
                        g_pi4_xhci_dma_child_bulk_in_words[i]);
                }
                uart_puts(g_uart, " bulk OUT context=");
                for (unsigned i = 0; i < 5u; i++) {
                    if (i) uart_puts(g_uart, "/");
                    uart_put_u64_hex(g_uart,
                        g_pi4_xhci_dma_child_bulk_out_words[i]);
                }
            }
            uart_puts(g_uart, "\n");
        }
        uart_puts(g_uart, " port-speed-id=");
        uart_put_u32_dec(g_uart, pi4_xhci_dma_port_speed_id(
            g_pi4_xhci_dma_port_event_after));
        uart_puts(g_uart, " port-ready=");
        uart_put_u32_dec(g_uart, g_pi4_xhci_dma_port_ready);
        uart_puts(g_uart, "\nxHCI protocol-cap/major/slot-type=");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_protocol_cap_offset);
        uart_puts(g_uart, "/");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_protocol_major);
        uart_puts(g_uart, "/");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_protocol_slot_type);
        uart_puts(g_uart, "\nxHCI EnableSlot TRB control=");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_enable_trb_control);
        uart_puts(g_uart, "\nxHCI EnableSlot status/control=");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_enable_slot_status);
        uart_puts(g_uart, "/");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_enable_slot_control);
        uart_puts(g_uart, " AddressDevice status/control=");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_address_status);
        uart_puts(g_uart, "/");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_address_control);
        uart_puts(g_uart, " USB-address/slot-state=");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_device_address);
        uart_puts(g_uart, "/");
        uart_put_u64_hex(g_uart, g_pi4_xhci_dma_device_slot_state);
        uart_puts(g_uart, " (Address Device issues SET_ADDRESS)\n");
        if (g_pi4_xhci_dma_stage >= 14) {
            uart_puts(g_uart, "EP0 GET_DESCRIPTOR setup/data/status=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_setup_trb_control);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_data_trb_control);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_status_trb_control);
            uart_puts(g_uart, " buffer=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_control_buffer_bus);
            uart_puts(g_uart, "\nTransfer Event parameter/status/control=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_transfer_parameter);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_transfer_status);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_transfer_control);
            uart_puts(g_uart, " checks/skipped=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_transfer_checks);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_transfer_skipped);
            uart_puts(g_uart, " first skipped param/status/control=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_transfer_skipped_parameter);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_transfer_skipped_status);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_transfer_skipped_control);
            uart_puts(g_uart, " descriptor first8=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_descriptor_prefix[0]);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_descriptor_prefix[1]);
            uart_puts(g_uart, " device bcd/class/sub/proto/mps=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_device_bcd_usb);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_device_class);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_device_subclass);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_device_protocol);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_device_ep0_mps);
            uart_puts(g_uart, " vid/pid/rev/configs=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_device_vendor);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_device_product);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_device_bcd_device);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_device_configurations);
            uart_puts(g_uart, "\n");
        }
        if (g_pi4_xhci_dma_stage >= 15) {
            uart_puts(g_uart, "Configuration header buffer/words=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_config_buffer_bus);
            for (unsigned i = 0; i < 3; i++) {
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_config_header_words[i]);
            }
            uart_puts(g_uart, " total/interfaces/value/attr/power=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_config_total_length);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_config_interfaces);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_config_value);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_config_attributes);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_config_max_power);
            uart_puts(g_uart, "\n");
        }
        if (g_pi4_xhci_dma_stage >= 16) {
            uart_puts(g_uart, "SET_CONFIGURATION setup/status controls=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_set_config_setup_control);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_set_config_status_control);
            uart_puts(g_uart, " event parameter/status/control=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_set_config_event_parameter);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_set_config_event_status);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_set_config_event_control);
            uart_puts(g_uart, " checks/skipped=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_set_config_event_checks);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_set_config_event_skipped);
            uart_puts(g_uart, "\n");
        }
        if (g_pi4_xhci_dma_stage >= 17) {
            uart_puts(g_uart, "Hub GET_DESCRIPTOR rc/setup=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_descriptor_transfer_rc);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_descriptor_setup[0]);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_descriptor_setup[1]);
            uart_puts(g_uart, " data bus/len/ctrl/status/valid=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_descriptor_data_bus);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_descriptor_data_length);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_descriptor_data_control);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_descriptor_status_control);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_descriptor_validated);
            uart_puts(g_uart, "\nHub descriptor buffer/words=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_buffer_bus);
            for (unsigned i = 0; i < 4; i++) {
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_hub_descriptor_words[i]);
            }
            uart_puts(g_uart,
                " ports/chars/pwr-delay/current/removable/power-mask=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_ports);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_characteristics);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_power_delay);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_current);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_removable);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_hub_power_mask);
            uart_puts(g_uart, "\n");
        }
        if (g_pi4_xhci_dma_stage >= 18) {
            uart_puts(g_uart, "Hub port power/status events=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_port_power_events);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_hub_port_status_events);
            uart_puts(g_uart, " status/change=");
            for (unsigned i = 0; i < 4u; i++) {
                if (i) uart_puts(g_uart, "/");
                uart_write(g_uart, (char)('1' + i));
                uart_puts(g_uart, "=");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_hub_port_status[i]);
                uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_hub_port_change[i]);
            }
            uart_puts(g_uart, "\n");
        }
        if (g_pi4_xhci_dma_stop_endpoint_rc != UINT32_MAX) {
            uart_puts(g_uart, "Stop Endpoint rc/TRB/status/control=");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_stop_endpoint_rc);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_stop_endpoint_trb_control);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_stop_endpoint_status);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart, g_pi4_xhci_dma_stop_endpoint_control);
            uart_puts(g_uart, " transfer-events/other=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_stop_endpoint_transfer_events);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_stop_endpoint_other_events);
            uart_puts(g_uart, " event param/status/control=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_stop_endpoint_transfer_parameter);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_stop_endpoint_transfer_status);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_stop_endpoint_transfer_control);
            uart_puts(g_uart, "\n");
        }
        if (g_pi4_xhci_dma_stop_output_ep0_context_valid) {
            uart_puts(g_uart, "EP0 after Stop state/dequeue/DCS=");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_stop_output_ep0_state);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_stop_output_ep0_dequeue_bus);
            uart_puts(g_uart, "/");
            uart_put_u64_hex(g_uart,
                g_pi4_xhci_dma_stop_output_ep0_dcs);
            uart_puts(g_uart, " ctx=");
            for (unsigned i = 0; i < 5u; i++) {
                if (i) uart_puts(g_uart, "/");
                uart_put_u64_hex(g_uart,
                    g_pi4_xhci_dma_stop_output_ep0_context[i]);
            }
            uart_puts(g_uart, "\n");
        }

        /*
         * PI-DIAG-3: soft CNTP (poll ISTATUS). No GICv2 yet — proves timer
         * regs before distributor/CPU interface bring-up.
         */
        {
            uint32_t bw = (uint32_t)(g_fb_w > 600 ? 600 : g_fb_w);
            uint64_t frq;
            uint32_t soft_ticks = 0;
            const uint32_t COL_TICK = 0x0000E000u; /* green */
            const uint32_t COL_DONE = 0x00E0E000u; /* yellow */

            con_puts("PI-DIAG-3 soft CNTP\n");
            uart_puts(g_uart, "PI-DIAG-3 soft CNTP (no GIC)\n");

            __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frq));
            if (frq == 0) {
                frq = 54000000; /* Pi4 typical fallback */
            }
            g_timer_ticks = frq / 4; /* ~4 Hz */
            uart_puts(g_uart, "CNTFRQ=");
            uart_put_u64_hex(g_uart, frq);
            uart_puts(g_uart, "\n");

            g_sync_recover = 1;
            g_sync_skipped = 0;
            timer_arm();
            {
                uint32_t trapped = g_sync_skipped;
                g_sync_recover = 0;
                if (trapped) {
                    uart_puts(g_uart, "CNTP trapped — skip\n");
                    con_puts("CNTP trapped\n");
                    if (g_fb && g_fb_h > 330) {
                        fb_fillrect(0, 310, bw, 12, 0x00E00000u);
                    }
                } else {
                    uint64_t t0;
                    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
                    while (soft_ticks < 3) {
                        uint64_t ctl, now;
                        __asm__ volatile("mrs %0, cntp_ctl_el0" : "=r"(ctl));
                        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
                        if ((ctl & 4u) != 0 && (now - t0) > (frq / 8)) {
                            soft_ticks++;
                            uart_puts(g_uart, "soft tick ");
                            uart_put_u32_dec(g_uart, soft_ticks);
                            uart_puts(g_uart, "\n");
                            if (g_fb && g_fb_h > 310 + soft_ticks * 14) {
                                fb_fillrect(0, 300 + soft_ticks * 14, bw, 10, COL_TICK);
                            }
                            timer_arm();
                            __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
                        }
                        /* Busy-wait — no IRQ/WFI until GICv2. */
                        for (volatile unsigned s = 0; s < 1000u; s++) {
                        }
                    }
                    uart_puts(g_uart, "soft CNTP OK\n");
                    con_puts("soft CNTP OK\n");
                    if (g_fb && g_fb_h > 360) {
                        fb_fillrect(0, 350, bw, 14, COL_DONE);
                    }
                    __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(0ull));

                    /* PI-DIAG-4: GICv2 + CNTP IRQ (raw HHDM). */
                    {
                        const uint32_t COL_IRQ = 0x0000E0E0u; /* cyan */
                        const uint32_t COL_GIC_BAD = 0x00E00000u;
                        int gic_ok = 0;
                        uint32_t spins = 0;

                        con_puts("PI-DIAG-4 GICv2\n");
                        uart_puts(g_uart, "PI-DIAG-4 GICv2+CNTP IRQ\n");
                        g_timer_irqs = 0;
                        if (gicv2_init(h->hhdm_offset) != 0) {
                            uart_puts(g_uart, "GICv2 map/probe FAIL\n");
                            con_puts("GICv2 FAIL\n");
                            if (g_fb && g_fb_h > 380) {
                                fb_fillrect(0, 370, bw, 12, COL_GIC_BAD);
                            }
                        } else {
                            gic_ok = 1;
                            uart_puts(g_uart, "GICv2 @ ");
                            uart_put_u64_hex(g_uart, g_gicd_phys);
                            uart_puts(g_uart, "\n");
                            if (g_fb && g_fb_h > 380) {
                                /* orange = GIC probe OK, waiting for IRQs */
                                fb_fillrect(0, 370, bw, 10, 0x000080E0u);
                            }
                            timer_arm();
                            enable_irq();
                            uart_puts(g_uart, "GIC+CNTP armed\n");
                            while (g_timer_irqs < 3 && spins < 50000000u) {
                                __asm__ volatile("wfi");
                                spins++;
                                /* Escape if IRQs never arrive — soft poll. */
                                if ((spins & 0xffffu) == 0) {
                                    uint64_t ctl;
                                    __asm__ volatile("mrs %0, cntp_ctl_el0" : "=r"(ctl));
                                    if ((ctl & 4u) && g_timer_irqs == 0) {
                                        /* Pending but no IRQ delivery */
                                        break;
                                    }
                                }
                            }
                            __asm__ volatile("msr daifset, #2");
                            __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(0ull));

                            uart_puts(g_uart, "irq ticks=");
                            uart_put_u32_dec(g_uart, g_timer_irqs);
                            uart_puts(g_uart, "\n");

                            if (g_fb && g_fb_h > 380) {
                                uint32_t n = g_timer_irqs > 3 ? 3 : g_timer_irqs;
                                for (uint32_t i = 1; i <= n; i++) {
                                    fb_fillrect(0, 370 + i * 14, bw, 10, COL_IRQ);
                                }
                                if (g_timer_irqs >= 3) {
                                    fb_fillrect(0, 420, bw, 14, 0x00E0E0E0u); /* white = IRQ OK */
                                    con_puts("GICv2 IRQ OK\n");
                                    uart_puts(g_uart, "GICv2 IRQ OK - entering shell\n");
                                    /* Stop timer IRQs before interactive input. */
                                    __asm__ volatile("msr daifset, #2");
                                    __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(0ull));
                                    __asm__ volatile("isb");
                                    pl011_rx_prep();
#ifndef ZEAL_PI_DIAG
                                    con_puts("\nPI-DESKTOP-2: accepted Pi hardware startup\n");
                                    input_poll(); /* Runs the accepted automatic USB probe. */
                                    pi4_usb_bus_report(1);
                                    if (!h->sources_virt || !h->sources_size)
                                        con_puts("PI-DESKTOP-2: missing source archive; native desktop skipped\n");
                                    else if (!pi4_usb_bus_keyboard_ready() ||
                                             !pi4_usb_bus_pointer_ready() ||
                                             !g_pi4_bus.storage_ready)
                                        con_puts("PI-DESKTOP-2: combined USB gate failed; native desktop skipped\n");
                                    else if (!pi4_desktop_prepare())
                                        con_puts("PI-DESKTOP-2: unsupported framebuffer; use diagnostic recovery\n");
                                    else
                                        native_desktop_start(h);
#endif
                                    shell_run(h);
                                    for (;;) {
                                        __asm__ volatile("wfi");
                                    }
                                } else if (gic_ok) {
                                    fb_fillrect(0, 420, bw, 14, COL_GIC_BAD);
                                    con_puts("GICv2 no IRQ - halt\n");
                                }
                            }
                        }
                    }
                }
            }
        }

        for (;;) {
            __asm__ volatile("wfi");
        }
    }
#endif

    if (h->uart_phys) {
        if (is_pi) {
            if (mmio_map_2m(h->hhdm_offset, h->uart_phys) == 0) {
                g_uart = (volatile uint8_t *)(h->hhdm_offset + h->uart_phys);
            }
        } else {
            g_uart = (volatile uint8_t *)(h->hhdm_offset + h->uart_phys);
        }
    }

    /* ---- QEMU / UTM virt path (full bring-up) ---- */
    fb_init(h);

    uart_puts(g_uart, "\n==============================\n");
    uart_puts(g_uart, "kernel_entry (module + vectors + CNTP)\n");
    uart_puts(g_uart, is_pi ? "platform: Pi4\n" : "platform: virt\n");
    uart_puts(g_uart, "zealos-aarch64-bringup / handoff OK\n");
    uart_puts(g_uart, "boot_el: ");
    if (g_uart) {
        uart_write(g_uart, (char)('0' + (int)h->boot_el));
    }
    uart_puts(g_uart, "\nHHDM: ");
    uart_put_u64_hex(g_uart, h->hhdm_offset);
    uart_puts(g_uart, "\n");

    vectors_install();
    uart_puts(g_uart, "VBAR_EL1 installed\n");

    __asm__ volatile("svc #0");

    uint64_t frq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frq));
    if (frq == 0) {
        frq = 62500000;
    }
    g_timer_ticks = frq / 4;
    uart_puts(g_uart, "CNTFRQ=");
    uart_put_u64_hex(g_uart, frq);
    uart_puts(g_uart, "  timer ~4Hz\n");

    if (!is_pi) {
        gic_init();
        {
            uint64_t sre, grp1;
            MRS_ICC(5, sre);
            MRS_ICC(7, grp1);
            uart_puts(g_uart, "ICC_SRE=");
            uart_put_u64_hex(g_uart, sre);
            uart_puts(g_uart, " IGRPEN1=");
            uart_put_u64_hex(g_uart, grp1);
            uart_puts(g_uart, "\n");
        }

        g_sync_recover = 1;
        g_sync_skipped = 0;
        timer_arm();
        uint32_t timer_trapped = g_sync_skipped;
        g_sync_recover = 0;

        uint32_t soft_ticks = 0;
        if (timer_trapped) {
            uart_puts(g_uart, "timer: CNTP trapped (UTM/HVF) — skip IRQ smoke\n");
        } else {
            enable_irq();
            uart_puts(g_uart, "GIC+CNTP armed, collecting ticks...\n");

            uint64_t t0;
            __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
            while (g_timer_irqs < 3 && soft_ticks < 3) {
                uint64_t ctl;
                __asm__ volatile("mrs %0, cntp_ctl_el0" : "=r"(ctl));
                if ((ctl & 4u) && g_timer_irqs == 0) {
                    uint64_t now;
                    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
                    if (now - t0 > frq) {
                        soft_ticks++;
                        uart_puts(g_uart, "tick (ISTATUS) ");
                        uart_put_u32_dec(g_uart, soft_ticks);
                        uart_puts(g_uart, "\n");
                        timer_arm();
                        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(t0));
                    }
                }
                if (g_timer_irqs + soft_ticks >= 3) {
                    break;
                }
                __asm__ volatile("wfi");
            }

            uart_puts(g_uart, "timer OK (irq=");
            uart_put_u32_dec(g_uart, g_timer_irqs);
            uart_puts(g_uart, " soft=");
            uart_put_u32_dec(g_uart, soft_ticks);
            uart_puts(g_uart, " svc=");
            uart_put_u32_dec(g_uart, g_svc_hits);
            uart_puts(g_uart, ")\n");

            g_sync_recover = 1;
            __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(0ull));
            g_sync_recover = 0;
            __asm__ volatile("msr daifset, #2");
        }
    }

    uart_puts(g_uart, "HolyC-IR → aarch64 JIT...\n");
    if (jit_smoke() != 0) {
        uart_puts(g_uart, "hc IR FAIL\n");
        uart_puts(g_uart, "hc IR FAIL — halted (fix run-serial / check-serial)\n");
        for (;;) {
            __asm__ volatile("wfi");
        }
    }

    (void)g_fb_guard;
    fb_smoke(h);

    if (!is_pi) {
        uart_puts(g_uart, "virtio-kbd: probe...\n");
        if (mmio_map_virtio_window(h->hhdm_offset) != 0) {
            uart_puts(g_uart, "virtio-kbd: mmio map FAIL\n");
        }
        if (virtio_kbd_init(h->hhdm_offset)) {
            uart_puts(g_uart, "virtio-kbd: OK @ ");
            uart_put_u64_hex(g_uart, virtio_kbd_mmio_phys());
            uart_puts(g_uart, "\n");
        } else {
            uart_puts(g_uart, "virtio-kbd: none (serial only)\n");
        }
        if (virtio_tablet_init(h->hhdm_offset)) {
            uart_puts(g_uart, "virtio-tablet: OK @ ");
            uart_put_u64_hex(g_uart, virtio_tablet_mmio_phys());
            uart_puts(g_uart, "\n");
        } else {
            uart_puts(g_uart, "virtio-tablet: none\n");
        }
        if (virtio_blk_init(h->hhdm_offset)) {
            uart_puts(g_uart, "virtio-blk: OK @ ");
            uart_put_u64_hex(g_uart, virtio_blk_mmio_phys());
            uart_puts(g_uart, " cap=");
            uart_put_u64_hex(g_uart, virtio_blk_capacity());
            uart_puts(g_uart, g_vb.xport == VB_XPORT_PCI ? " pci\n" : " mmio\n");
            uart_puts(g_uart, "virtio-blk: rw OK\n");
#ifndef ZEAL_PI_DIAG
            uart_puts(g_uart, zss_available() ? "source-volume: ready\n" :
                                              "source-volume: none\n");
#endif
        } else {
            uart_puts(g_uart, "virtio-blk: none (RAM Blk* only) dbg=");
            uart_put_u64_hex(g_uart, (uint64_t)(uint32_t)g_vb_pci_dbg);
            uart_puts(g_uart, "\n");
        }
    }

    if (!is_pi) {
        usb_init(h->hhdm_offset, (unsigned)g_fb_w, (unsigned)g_fb_h, input_usb_key);
        input_poll();
    }

    bc_module_run(h);
#ifndef ZEAL_PI_DIAG
    native_desktop_start(h);
#endif
    shell_run(h);

    uart_puts(g_uart, "tablet clicks=");
    uart_put_u32_dec(g_uart, g_tab_clicks);
    uart_puts(g_uart, "\n");
    uart_puts(g_uart, "halting\n");
    uart_puts(g_uart, "==============================\n");

    for (;;) {
        __asm__ volatile("wfi");
    }
}
