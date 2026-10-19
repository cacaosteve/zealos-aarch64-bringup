#include "runtime.h"
#include "aiwn_except.h"
#include "aiwn_hash.h"
#include "aiwn_lexparser.h"
#include "aiwn_mem.h"
#include "../fb_font.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int64_t TempleOS_CallNOnStack(uint64_t stack_top, void (*fptr)(),
                              int64_t argc, int64_t *argv);
extern int zeal_zss_entry_at(uint32_t index, char *name, size_t name_cap,
                             size_t *size_out);
extern uint64_t zeal_zss_catalog_revision(void);
extern int zeal_zss_writable(void);

/* ZealOS KernelA.HH currently defines EXT_EXTS_NUM as five. Keep a stable
 * pointer/table pair for its kernel-owned function dispatch slots; guest
 * modules populate these slots as they load. */
static void *guest_ext_entries[5];
static void **guest_ext_table = guest_ext_entries;
static uint32_t guest_boot_source; /* BOOT_SRC_NULL until a boot handoff arrives. */
void zc_set_boot_media(uint32_t media_type) {
    /* Pinned KernelA.HH: HARDDRIVE=3, DVD=4, RAM=2. No x86 boot-block
     * offset exists on the EFI path, so the high half remains zero. */
    guest_boot_source = media_type == 0 ? 3 : media_type == 1 ? 4 : media_type == 2 ? 2 : 0;
}
static uint64_t guest_fb_addr, guest_fb_width, guest_fb_height, guest_fb_pitch;
static uint64_t guest_fb_bpp;
static uint64_t guest_text_globals[16];
/* The first graphics slice uses the kernel's compact 8x8 ASCII font as the
 * guest CTextGlobals.font table. Entries are U64 so unchanged ZealOS code can
 * index text.font[character] with its original ABI. */
static uint64_t guest_gr_font[256];
static uint64_t guest_gr_colors[256];
static int guest_gr_tables_ready;
static void *guest_fp_set_std_palette;
static void *guest_fp_update_ctrls;

extern void hc_fmt_f64_bits(char **dp, uint64_t bits, int prec);
extern uint64_t hc_builtin_exp(uint64_t xbits);
extern uint64_t hc_builtin_sqrt(uint64_t xbits);
extern uint64_t hc_builtin_cos(uint64_t xbits);
extern uint64_t hc_builtin_sin(uint64_t xbits);

extern uint64_t zeal_fb_text_span(int64_t x, int64_t y, int64_t len,
                                  uint32_t attr, const void *data, int mode,
                                  int64_t left, int64_t right,
                                  int64_t top, int64_t bottom,
                                  uint32_t *text_base, int64_t stride);
extern uint64_t zeal_fb_text_cells_drawn(void);
extern int64_t zeal_fb_text_pixel(int64_t x, int64_t y);
extern int64_t zeal_fb_text_char(int64_t x, int64_t y, uint32_t cell,
                                 int allow_border, uint32_t left_right,
                                 uint32_t top_bottom, uint32_t *text_base,
                                 int64_t stride);
extern int64_t zeal_fb_text_fill(int64_t x, int64_t y, int64_t len,
                                 uint32_t attr, uint32_t left_right,
                                 uint32_t top_bottom, uint32_t *text_base,
                                 int64_t stride);
extern uint64_t zeal_fb_text_rect(int64_t left, int64_t right, int64_t top,
                                  int64_t bottom, uint32_t cell,
                                  uint32_t *text_base, int64_t stride);
extern uint64_t zeal_fb_text_flush(uint32_t *text_base, int64_t stride,
                                   int64_t rows);
extern uint64_t zeal_fb_text_flush_rect(uint32_t *text_base, int64_t stride,
                                        int64_t left, int64_t right,
                                        int64_t top, int64_t bottom);
extern uint64_t zeal_fb_graph_plot(int64_t x, int64_t y, uint32_t color,
                                   int64_t left, int64_t top,
                                   int64_t right, int64_t bottom,
                                   uint16_t task_z);
extern uint64_t zeal_fb_graph_color(uint32_t color);
extern void zeal_fb_set_win_z_buffer(const uint16_t *z_buf);
extern uint64_t zeal_fb_graph_rect(int64_t x, int64_t y,
                                   int64_t width, int64_t height,
                                   uint32_t color, int64_t left, int64_t top,
                                   int64_t right, int64_t bottom,
                                   uint16_t task_z);
extern uint64_t zeal_fb_graph_text(int64_t x, int64_t y, uint32_t color,
                                   int64_t left, int64_t top,
                                   int64_t right, int64_t bottom,
                                   const char *text, uint16_t task_z);
extern uint64_t zeal_fb_graph_line(int64_t x0, int64_t y0,
                                   int64_t x1, int64_t y1, uint32_t color,
                                   int64_t step, int64_t start,
                                   int64_t left, int64_t top,
                                   int64_t right, int64_t bottom,
                                   uint16_t task_z);
extern int64_t zc_tablet_sample(void);

/* Separate arenas from legacy hc_* so its per-demo reset cannot invalidate
 * modules loaded here. Both use splitting/coalescing and arbitrary-order Free. */
#define DATA_BYTES (24u * 1024u * 1024u)
#define CODE_BYTES (8u * 1024u * 1024u)
#define BLOCK_MAGIC 0x5a43424c4f434b31ull
struct block {
    uint64_t magic;
    size_t size, requested;
    struct block *prev, *next;
    CHeapCtrl *owner;
    uint64_t used, pad;
};
_Static_assert(sizeof(struct block) % 16 == 0, "allocator alignment");
static unsigned char data_arena[DATA_BYTES] __attribute__((aligned(16)));
static unsigned char code_arena[CODE_BYTES] __attribute__((section(".jit"), aligned(4096)));
static struct block *arenas[2];
/* Keep a separate address-ordered list of free blocks. First-fit allocation
 * stays byte-for-byte compatible with the original arena layout, while
 * avoiding a walk across every live compiler allocation on each request.
 * Free blocks reuse owner/pad for their free-list links. */
static struct block *arena_free_heads[2];
static CTask task;
CTask *Fs;
static CHeapCtrl data_heap, code_heap;
static zc_output_fn output_cb;
static zc_key_event_fn key_reader;
static zc_key_reset_fn key_reset;
static const unsigned char *source_archive;
static size_t archive_size;
static zc_source_read_fn disk_reader;
static zc_source_write_fn disk_writer;
static jmp_buf guard;
static int guarded, ready, failed;
static jmp_buf compiler_recovery_guard;
static int compiler_recovery_active;
static uint64_t compiler_recovery_errors;
static int64_t compiler_recovery_exception;
#define GUEST_EXCEPT_COMPILER INT64_C(0x72656c69706d6f43)
#define GUEST_EXCEPT_BREAK INT64_C(0x6b61657242)
static int window_demo_loaded;
static int window_demo_restore_gui_terminal;
static int gui_terminal_active;
static void *guest_keydev_put_key;
static void *guest_doc_recalc_stub;
static int gui_terminal_modules_loaded;

const char *zc_debug_function_for_pc(uintptr_t pc, uintptr_t *offset) {
    CHashTable *table;
    CHashFun *best = NULL;
    uintptr_t best_pc = 0;
    uintptr_t code_lo = (uintptr_t)code_arena;
    uintptr_t code_hi = code_lo + sizeof(code_arena);
    if (offset)
        *offset = 0;
    if (!Fs || pc < code_lo || pc >= code_hi)
        return NULL;
    for (table = Fs->hash_table; table; table = table->next) {
        int64_t i;
        if (!table->body || table->mask < 0 || table->mask > 65535)
            continue;
        for (i = 0; i <= table->mask; ++i) {
            CHash *entry;
            for (entry = table->body[i]; entry; entry = entry->next) {
                if ((entry->type & HTT_FUN) &&
                    ((CHashFun *)entry)->fun_ptr) {
                    uintptr_t start = (uintptr_t)((CHashFun *)entry)->fun_ptr;
                    if (start >= code_lo && start <= pc && start > best_pc) {
                        best = (CHashFun *)entry;
                        best_pc = start;
                    }
                }
            }
        }
    }
    if (!best)
        return NULL;
    if (offset)
        *offset = pc - best_pc;
    return best->base.base.str;
}
static size_t loaded_modules;
static FILE files[16];
/* Kernel/KMain.ZC builds this table before KMathB is used. The current
 * freestanding bootstrap has no upstream startup sequence, so provide its
 * data binding here. Index 309 is 10^0; KMathB uses indices 1 through 617.
 * KMain allocates 617 elements but writes element 617; reserve all 618 here. */
static double pow10_values[618];
static double *pow10_I64_host = pow10_values;
/* Guest CTask storage remains separate from Aiwnios compiler CTask state.
 * Each child compiler table inherits its parent's names, matching pinned
 * Spawn, while these fibers still implement only cooperative scheduling. */
#define GUEST_APP_TASKS 9 /* root plus eight user-spawned children */
#define GUEST_EXECUTIVE GUEST_APP_TASKS /* reserved CPU-0 job server */
#define GUEST_SYSTEM (GUEST_APP_TASKS + 1) /* original ServerTaskCont */
#define GUEST_TASKS (GUEST_APP_TASKS + 2)
#define GUEST_STACK_BYTES (64u * 1024u)
#define GUEST_STACK_MAX_BYTES (2u * 1024u * 1024u)
#define GUEST_JOBS_MAX 128u
#define GUEST_JOB_DEPTH_MAX 8u
#define GUEST_MESSAGE_MS_MOVE 4
#define GUEST_INPUT_FILTER_TASK (1u << 5) /* TASKf_INPUT_FILTER_TASK */
/* KernelA.HH CJob: keep the queue links and message fields at their original
 * offsets so guest code may inspect the pending queue through CTask. */
struct guest_job {
    struct guest_job *next, *last;
    void *ctrl;
    int64_t job_code, flags, message_code;
    void *addr, *fun_arg, *aux_str;
    int64_t aux1, aux2, res;
    void *spawned_task, *master_task;
};
struct guest_active_job {
    struct guest_active_job *previous;
    struct guest_job *job;
};
_Static_assert(sizeof(struct guest_job) == 112 &&
               offsetof(struct guest_job, message_code) == 40 &&
               offsetof(struct guest_job, aux_str) == 64 &&
               offsetof(struct guest_job, aux1) == 72 &&
               offsetof(struct guest_job, res) == 88 &&
               offsetof(struct guest_job, spawned_task) == 96 &&
               offsetof(struct guest_job, master_task) == 104,
               "pinned CJob layout");
enum guest_state { GUEST_UNUSED, GUEST_RUNNABLE, GUEST_DONE };
enum guest_entry {
    GUEST_ENTRY_NO_ARG, GUEST_ENTRY_SPAWN, GUEST_ENTRY_EXECUTIVE,
    GUEST_ENTRY_INPUT_FILTER, GUEST_ENTRY_SYSTEM
};
enum guest_wait {
    GUEST_WAIT_NONE, GUEST_WAIT_BIRTH, GUEST_WAIT_DEATH,
    GUEST_WAIT_IDLE, GUEST_WAIT_MESSAGE, GUEST_WAIT_SERVER, GUEST_WAIT_JOB,
    GUEST_WAIT_TIMER, GUEST_WAIT_IDLE_TIMEOUT, GUEST_WAIT_MESSAGE_TIMEOUT
};
struct guest_unwind;
struct guest_exception {
    struct guest_exception *next;
    struct guest_exception *caught_previous;
    unsigned depth;
    uint8_t previous_catch;
    int64_t previous_ch;
    struct guest_unwind *unwind;
    uint64_t irq_flags, irq_stack[8];
    unsigned irq_depth;
    jmp_buf context;
};
/* Native scopes participate in guest exception ordering. A guest catch keeps
 * scopes active at its SysTry; scopes entered afterward must unwind first.
 * A landing pad handles cleanup in its own C frame; cleanup-only scopes can
 * be retired directly while the throwing native frame is still on-stack. */
struct guest_unwind {
    struct guest_unwind *previous;
    struct guest_exception *top, *caught;
    void (*cleanup)(struct guest_unwind *);
    jmp_buf *landing;
    volatile int64_t exception;
    int64_t previous_ch;
    uint8_t previous_catch;
    uint64_t irq_flags, irq_stack[8];
    unsigned irq_depth;
};
struct source_context;
struct user_compiler;
struct guest_fiber {
    uint64_t words[2048];
    uint64_t heap_words[256];
    CHeapCtrl heap_owner;
    CTask compiler_task;
    CHeapCtrl compiler_heap, compiler_code_heap;
    void *prompt_cc;
    struct source_context *source_context;
    struct user_compiler *user_compilers;
    void *user_prompt_reader;
    void *user_output_writer;
    int user_output_suppressed, user_diagnostic_active;
    size_t user_diagnostic_length;
    char user_diagnostic[2048];
    uint64_t user_statements;
    jmp_buf context;
    unsigned char stack[GUEST_STACK_BYTES] __attribute__((aligned(16)));
    unsigned char *large_stack;
    size_t stack_size;
    void *entry, *data;
    int64_t result;
    enum guest_state state;
    unsigned started;
    enum guest_entry entry_kind;
    enum guest_wait wait_kind;
    void *wait_slot;
    void *wait_target;
    int64_t wait_number;
    uint64_t wait_deadline;
    int kill_requested;
    int ending;
    int key_wait_active;
    int source_job_active;
    uint64_t irq_flags_stack[8];
    unsigned irq_flags_depth;
    /* Live IF bit while this fiber is descheduled. Job.ZC may Yield under
     * PUSHFD/CLI; each fiber keeps its own mask across cooperative switches. */
    uint64_t irq_flags_live;
    int64_t last_polled_key_code;
    struct guest_active_job *active_jobs;
    struct guest_exception *except_top, *except_caught;
    struct guest_unwind *unwind;
    jmp_buf throw_context;
    jmp_buf end_context;
};
static struct guest_fiber guest_fibers[GUEST_TASKS];
/* Custom stacks use a separate owner so finish_guest can reclaim a task's
 * ordinary heap without freeing the stack it is currently running on. */
static CHeapCtrl guest_stack_heap;
static unsigned guest_current, guest_completed;
static uint64_t guest_task_serial;
static unsigned guest_job_count;
static struct guest_job *guest_tracked_jobs[GUEST_JOBS_MAX];
/* Parsing and code emission share one CPU and cannot switch tasks. Emitted
 * source code may switch after compilation finishes. */
static int guest_task_exe_active;
/* Only the CPU-0 executive runs call jobs. Unlike source compilation, a
 * precompiled callback may park on its own fiber stack and resume later. */
static int guest_job_call_active;
static int guest_background_dispatch;
static unsigned guest_idle_frame_divider;
static uint64_t guest_task_frame_count;
static CHashFun *guest_winmgr_tick_fun;
/* Keep source graphics updates inside the staged framebuffer transaction. */
static int guest_task_frame_active;
static int guest_task_frame_rgba_ready;
static void set_guest_task(void *p);
static int64_t guest_mp_count = 1;
static void *guest_cpu_structs;
static void *guest_kbd_state;
static void *guest_autocomplete_state;
static void *guest_task_being_screen_updated;
static void *guest_screencast_state;
static struct guest_counts {
    int64_t jiffies, timer, time_stamp_freq, time_stamp_kHz_freq;
    int64_t time_stamp_freq_initial;
    uint8_t time_stamp_calibrated;
} guest_counts;
static uint64_t guest_tS_bits;
_Static_assert(offsetof(struct guest_counts, time_stamp_freq) == 16 &&
               sizeof(struct guest_counts) == 48, "pinned counts layout");
/* KernelB's pinned CProgress layout; populated independently by graphics
 * compatibility tests until the native progress service is ported. */
struct guest_progress {
    int64_t val, max;
    double t0, tf;
    uint8_t desc[48];
};
static struct guest_progress guest_progresses[4];
/* KernelB exposes the elapsed-time origin of each progress bar separately
 * from its CProgress record; Win.ZC reads these when saving progress timing. */
static double guest_progress_t0[4];
_Static_assert(offsetof(struct guest_progress, desc) == 32 &&
               sizeof(struct guest_progress) == 80,
               "pinned CProgress layout");
/* KernelB: CSema sys_semas[SEMA_SEMAS_NUM]; each CSema is one cache line. */
#define GUEST_SEMA_NUM 21
#define GUEST_SEMA_STRIDE 128
static uint8_t guest_sys_semas[GUEST_SEMA_NUM * GUEST_SEMA_STRIDE];
/* Pinned KernelB.HH exposes this as SYS_FOCUS_TASK. NULL preserves the
 * bootstrap convention that the task actively scanning owns device input. */
static void *guest_focus_task;
/* Live keyboard routing diagnostics: distinguish device events from events
 * posted to the focused guest task and consumed by the graphical terminal. */
static uint64_t guest_keyboard_route_polls;
static uint64_t guest_keyboard_route_reads;
static uint64_t guest_keyboard_route_posts;
static uint64_t guest_keyboard_route_drops;
static int64_t guest_keyboard_route_last_type;
static int64_t guest_keyboard_route_last_arg1;
static int64_t guest_keyboard_route_last_arg2;
static int64_t guest_parent_off, guest_number_off, guest_name_off, guest_flags_off;
static int64_t guest_gs_off;
static int64_t guest_display_flags_off;
static int64_t guest_win_left_off, guest_win_right_off;
static int64_t guest_win_top_off, guest_win_bottom_off, guest_text_attr_off;
static int64_t guest_next_task_off, guest_last_task_off;
static int64_t guest_next_sibling_off, guest_last_sibling_off;
static int64_t guest_next_child_off, guest_last_child_off;
static int64_t guest_next_ctrl_off, guest_last_ctrl_off;
static int64_t guest_next_ode_off, guest_last_ode_off;
static int64_t guest_popup_off;
static int64_t guest_win_inhibit_off;
static int64_t guest_data_heap_off, guest_code_heap_off;
static int64_t guest_answer_off, guest_answer_type_off, guest_answer_time_off;
static int64_t guest_new_answer_off;
static int64_t guest_end_cb_off;
static int64_t guest_input_filter_off, guest_next_filter_off, guest_server_ctrl_off;
static int64_t guest_heap_sig_off, guest_heap_used_off, guest_heap_task_off;
static int64_t guest_except_ch_off, guest_catch_except_off;
static int64_t guest_hash_table_off;
static int64_t guest_cur_dir_off;
static char **guest_home_dir_ptr;
static int64_t guest_next_cc_off, guest_last_cc_off;
static int64_t guest_last_fun_off, guest_fun_size, guest_fun_name_off;
static int64_t guest_fun_type_off, guest_fun_flags_off;
static int64_t guest_fun_argc_off, guest_fun_code_off;
static int64_t guest_cc_size, guest_cc_flags_off, guest_cc_opts_off;
static int64_t guest_cc_files_off, guest_cc_errors_off, guest_cc_token_off;
static int64_t guest_lex_size, guest_lex_buf_off, guest_lex_ptr_off;
static int64_t guest_lex_line_off, guest_lex_flags_off, guest_lex_name_off;
static int64_t guest_lex_depth_off, guest_lex_start_off;
static int64_t guest_lex_doc_off, guest_lex_entry_off;
static void *guest_prompt_context(void);
static int guest_task_bound;
static int source_winmgr_active;
extern uint64_t zeal_fb_text_cols(void), zeal_fb_text_rows(void);
extern uint64_t zeal_fb_screen_width(void), zeal_fb_screen_height(void);
extern uint64_t zeal_fb_pitch_bytes(void), zeal_fb_bits_per_pixel(void);
extern uint64_t zeal_fb_address(void);
extern uint64_t zeal_fb_nonblack_pixels(void);
extern uint64_t zeal_fb_task_frame_begin(uint8_t *surface, uint32_t *rgba,
                                         uint64_t stride, uint64_t rows);
extern uint64_t zeal_fb_task_frame_end(uint64_t use_rgba);
extern void zeal_fb_task_frame_cancel(void);
extern void zeal_fb_task_text_reset(unsigned slot);
extern uint64_t zeal_fb_task_text_write(unsigned slot, int64_t left,
                                         int64_t right, int64_t top,
                                         int64_t bottom, uint32_t attr,
                                         const char *text);
extern uint64_t zeal_fb_task_text_compose(unsigned slot, int64_t left,
                                           int64_t right, int64_t top,
                                           int64_t bottom, uint32_t *text_base,
                                           int64_t stride,
                                           const uint16_t *z_buf,
                                           uint16_t task_z);
extern uint64_t zeal_fb_shell_text_compose(uint32_t *text_base,
                                            int64_t stride);
static void guest_task_text_output(const char *text);
static uint32_t *guest_flags(struct guest_fiber *f);
static void guest_raise(int64_t code) __attribute__((noreturn));
static void guest_unwind_all(void);
static int64_t host_rflags_get(int64_t *a);
static int64_t host_rflags_set(int64_t *a);
static int64_t execute_source_result(const char *path, const char *src,
                                     int return_expr, int record_answer);
extern void __clear_cache(void *, void *);
extern int64_t zc_gui_shell_exec(int64_t *);
void zc_fail(const char *s) __attribute__((noreturn));

void zc_output(const char *s) {
    if (output_cb)
        output_cb(s);
    if (guest_task_bound && s) {
        struct guest_fiber *f = &guest_fibers[guest_current];
        if (f->user_diagnostic_active) {
            size_t n = strlen(s), avail = sizeof(f->user_diagnostic) - 1 - f->user_diagnostic_length;
            if (n > avail) n = avail;
            memcpy(f->user_diagnostic + f->user_diagnostic_length, s, n);
            f->user_diagnostic_length += n;
            f->user_diagnostic[f->user_diagnostic_length] = 0;
        }
    }
}
void zc_set_source_reader(zc_source_read_fn read) { disk_reader = read; }
void zc_set_source_writer(zc_source_write_fn write) { disk_writer = write; }
void zc_set_key_reader(zc_key_event_fn read, zc_key_reset_fn reset) {
    key_reader = read;
    key_reset = reset;
}
void zc_fail(const char *s) {
    compiler_recovery_active = 0;
    if (guest_task_bound && guest_fibers[guest_current].irq_flags_depth) {
        struct guest_fiber *f = &guest_fibers[guest_current];
        if (f->irq_flags_stack[0] & (1u << 9))
            __asm__ volatile("msr daifclr, #2\n\tisb" ::: "memory");
        else
            __asm__ volatile("msr daifset, #2\n\tisb" ::: "memory");
        f->irq_flags_depth = 0;
    }
    zc_output("zc: ");
    zc_output(s);
    zc_output("\n");
    failed = 1;
    if (guarded) {
        /* A failed guest call may be running on a fiber stack. The guard is
         * on the shell's stack, so restore its task pointer before unwinding. */
        if (guest_task_bound) {
            guest_current = 0;
            Fs = &task;
            __asm__ volatile("msr tpidr_el1, %0\n\tisb" : : "r"(guest_fibers[0].words) : "memory");
        }
        longjmp(guard, 1);
    }
    for (;;)
        __asm__ volatile("wfi");
}
void AIWNIOS_throw(uint64_t code) {
    (void)code;
    if (compiler_recovery_active) {
        compiler_recovery_exception = GUEST_EXCEPT_COMPILER;
        compiler_recovery_active = 0;
        longjmp(compiler_recovery_guard, 1);
    }
    zc_fail("compile failed; use zreset to discard the session");
}
void DoNothing(void) {
    uintptr_t return_pc = (uintptr_t)__builtin_return_address(0);
    uintptr_t offset = 0;
    const char *function = zc_debug_function_for_pc(return_pc, &offset);
    if (function)
        printf("zc: unresolved external call from %s+0x%lx\n", function,
               (unsigned long)offset);
    else
        printf("zc: unresolved external call from pc=0x%lx\n",
               (unsigned long)return_pc);
    zc_fail("unresolved external call");
}

static void arena_init(unsigned i, void *p, size_t size) {
    arenas[i] = p;
    *arenas[i] = (struct block){.magic = BLOCK_MAGIC, .size = size - sizeof(struct block)};
    arena_free_heads[i] = arenas[i];
}
static struct block *free_prev(const struct block *b) {
    return (struct block *)(uintptr_t)b->owner;
}
static struct block *free_next(const struct block *b) {
    return (struct block *)(uintptr_t)b->pad;
}
static void free_list_remove(unsigned a, struct block *b) {
    struct block *prev = free_prev(b), *next = free_next(b);
    if (prev)
        prev->pad = (uint64_t)(uintptr_t)next;
    else
        arena_free_heads[a] = next;
    if (next)
        next->owner = (CHeapCtrl *)prev;
    b->owner = NULL;
    b->pad = 0;
}
static void free_list_replace(unsigned a, struct block *old, struct block *replacement) {
    struct block *prev = free_prev(old), *next = free_next(old);
    replacement->owner = (CHeapCtrl *)prev;
    replacement->pad = (uint64_t)(uintptr_t)next;
    if (prev)
        prev->pad = (uint64_t)(uintptr_t)replacement;
    else
        arena_free_heads[a] = replacement;
    if (next)
        next->owner = (CHeapCtrl *)replacement;
}
static void free_list_insert(unsigned a, struct block *b) {
    struct block *prev = NULL, *next = arena_free_heads[a];
    uintptr_t address = (uintptr_t)b;
    while (next && (uintptr_t)next < address) {
        prev = next;
        next = free_next(next);
    }
    b->owner = (CHeapCtrl *)prev;
    b->pad = (uint64_t)(uintptr_t)next;
    if (prev)
        prev->pad = (uint64_t)(uintptr_t)b;
    else
        arena_free_heads[a] = b;
    if (next)
        next->owner = (CHeapCtrl *)b;
}
static CHeapCtrl *heap_for(void *t) {
    /* Aiwnios parser allocations use the currently compiling task; queued
     * jobs below pass &data_heap explicitly because they can outlive it. */
    if (!t || t == Fs)
        return Fs && Fs->heap ? Fs->heap : &data_heap;
    return t;
}
static void sync_guest_heap(CHeapCtrl *owner) {
    if (!guest_task_bound)
        return;
    for (unsigned i = 0; i < GUEST_TASKS; i++) {
        struct guest_fiber *f = &guest_fibers[i];
        if (owner == &f->heap_owner && f->state != GUEST_UNUSED) {
            *(int64_t *)((unsigned char *)f->heap_words + guest_heap_used_off) = owner->used_u8s;
            return;
        }
    }
}
void *__AIWNIOS_MAlloc(int64_t count, void *t) {
    if (count < 0 || (uint64_t)count > DATA_BYTES) {
        static char msg[96];
        snprintf(msg, sizeof(msg),
                 "allocation size out of range (%lld)%s",
                 (long long)count,
                 ((uint64_t)count >> 48) == 0xfff8ull ? " nan-bits" : "");
        zc_fail(msg);
    }
    CHeapCtrl *owner = heap_for(t);
    unsigned a = owner->is_code_heap ? 1 : 0;
    size_t n = ((size_t)count + 15) & ~(size_t)15;
    if (!n)
        n = 16;
    for (struct block *b = arena_free_heads[a]; b; b = free_next(b)) {
        if (b->size < n)
            continue;
        if (b->size >= n + sizeof(*b) + 16) {
            struct block *tail = (void *)((unsigned char *)(b + 1) + n);
            *tail = (struct block){
                .magic = BLOCK_MAGIC, .size = b->size - n - sizeof(*b), .prev = b, .next = b->next};
            if (tail->next)
                tail->next->prev = tail;
            b->next = tail;
            b->size = n;
            free_list_replace(a, b, tail);
        } else {
            free_list_remove(a, b);
        }
        b->used = 1;
        b->requested = (size_t)count;
        b->owner = owner;
        b->pad = 0;
        owner->used_u8s += (int64_t)b->size;
        sync_guest_heap(owner);
        return b + 1;
    }
    {
        size_t free_total = 0, largest = 0, free_count = 0;
        char msg[256];
        for (struct block *b = arena_free_heads[a]; b && free_count < 65536;
             b = free_next(b)) {
            free_total += b->size;
            if (b->size > largest)
                largest = b->size;
            free_count++;
        }
        snprintf(msg, sizeof(msg),
                 "%s (request=%lld aligned=%zu owner_used=%lld free=%zu largest=%zu blocks=%zu)",
                 a ? "code arena exhausted" : "data arena exhausted",
                 (long long)count, n, (long long)owner->used_u8s,
                 free_total, largest, free_count);
        zc_fail(msg);
    }
}
static struct block *get_block(void *p) {
    uintptr_t v = (uintptr_t)p;
    int valid = (v >= (uintptr_t)data_arena + sizeof(struct block) &&
                 v < (uintptr_t)data_arena + DATA_BYTES) ||
                (v >= (uintptr_t)code_arena + sizeof(struct block) &&
                 v < (uintptr_t)code_arena + CODE_BYTES);
    if (!valid || v % 16)
        zc_fail("invalid heap pointer");
    struct block *b = (struct block *)p - 1;
    if (b->magic != BLOCK_MAGIC || !b->used)
        zc_fail("invalid or duplicate free");
    return b;
}
static struct block *release(struct block *b) {
    CHeapCtrl *owner = b->owner;
    unsigned a = (uintptr_t)b >= (uintptr_t)code_arena &&
                 (uintptr_t)b < (uintptr_t)code_arena + CODE_BYTES;
    owner->used_u8s -= (int64_t)b->size;
    sync_guest_heap(owner);
    b->used = 0;
    b->owner = NULL;
    b->requested = 0;
    b->pad = 0;
    free_list_insert(a, b);
    if (b->next && !b->next->used) {
        struct block *n = b->next;
        free_list_remove(a, n);
        b->size += sizeof(*n) + n->size;
        b->next = n->next;
        n->magic = 0;
        if (b->next)
            b->next->prev = b;
    }
    if (b->prev && !b->prev->used) {
        struct block *p = b->prev;
        free_list_remove(a, b);
        p->size += sizeof(*b) + b->size;
        p->next = b->next;
        b->magic = 0;
        if (p->next)
            p->next->prev = p;
        b = p;
    }
    return b;
}
void __AIWNIOS_Free(void *p) {
    if (p)
        release(get_block(p));
}
void *__AIWNIOS_CAlloc(int64_t n, void *t) {
    void *p = __AIWNIOS_MAlloc(n, t);
    memset(p, 0, (size_t)n);
    return p;
}
char *__AIWNIOS_StrDup(char *s, void *t) {
    size_t n = strlen(s) + 1;
    char *p = __AIWNIOS_MAlloc((int64_t)n, t);
    memcpy(p, s, n);
    return p;
}
int64_t MSize(void *p) { return p ? (int64_t)get_block(p)->requested : 0; }
CHeapCtrl *HeapCtrlInit(CHeapCtrl *h, CTask *t, int64_t code) {
    if (!h)
        h = __AIWNIOS_CAlloc(sizeof(*h), NULL);
    h->is_code_heap = (int32_t)code;
    h->mem_task = t;
    return h;
}
void HeapCtrlDel(CHeapCtrl *h) {
    for (unsigned a = 0; a < 2; a++)
        for (struct block *b = arenas[a]; b;)
            b = (b->used && b->owner == h) ? release(b)->next : b->next;
    __AIWNIOS_Free(h);
}
void *malloc(size_t n) { return __AIWNIOS_MAlloc((int64_t)n, NULL); }
void *calloc(size_t n, size_t s) {
    if (s && n > SIZE_MAX / s)
        zc_fail("allocation overflow");
    return __AIWNIOS_CAlloc((int64_t)(n * s), NULL);
}
void free(void *p) { __AIWNIOS_Free(p); }
void *realloc(void *p, size_t n) {
    if (!n) {
        free(p);
        return NULL;
    }
    void *q = malloc(n);
    if (p) {
        size_t old = (size_t)MSize(p);
        memcpy(q, p, n < old ? n : old);
        free(p);
    }
    return q;
}

/* Deterministic ustar archive, supplied as a Limine module. No guest file
 * content is generated by this reader; source bytes are copied unchanged. */
static int normal_path(const char *src, char out[128]) {
    size_t n = 0;
    while (*src) {
        while (*src == '/')
            src++;
        const char *start = src;
        while (*src && *src != '/')
            src++;
        size_t len = (size_t)(src - start);
        if (!len)
            break;
        if (len == 1 && start[0] == '.')
            continue;
        if (len == 2 && start[0] == '.' && start[1] == '.') {
            while (n && out[n - 1] != '/')
                n--;
            if (n)
                n--;
            continue;
        }
        if (n + len + 2 > 128)
            return -1;
        if (n)
            out[n++] = '/';
        memcpy(out + n, start, len);
        n += len;
    }
    out[n] = 0;
    return 0;
}
/* The bootstrap source volume has one namespace, without ZealOS drive
 * letters. Keep compiler archive lookup root-relative; resolve application
 * paths against the calling task only at the public file API boundary. */
static const char *guest_current_dir(void) {
    const char *dir = guest_task_bound
        ? *(char **)((unsigned char *)guest_fibers[guest_current].words + guest_cur_dir_off)
        : NULL;
    return dir ? dir : "/";
}
static const char *guest_home_suffix(const char *path) {
    const char *suffix = NULL;
    for (const char *part = path; *part;) {
        while (*part == '/') part++;
        if (*part == '~' && (!part[1] || part[1] == '/')) suffix = part + 1;
        while (*part && *part != '/') part++;
    }
    return suffix;
}
static int guest_path(const char *path, char out[134]) {
    char joined[256], expanded[256], name[128];
    if (!path || !*path)
        return -1;
    int disk = !strncmp(path, "disk:", 5);
    const char *src = path + (disk ? 5 : 0);
    /* Boot/home drive roots both name the sole source volume. This does not
     * introduce other drive letters or mounted-volume services. */
    if (!disk && ((!strncmp(src, "::", 2) || !strncmp(src, "~:", 2)) &&
                  (!src[2] || src[2] == '/'))) {
        src += 2;
        if (!*src) src = "/";
    }
    size_t length = strlen(src);
    if (length >= 128 || strchr(src, ':') || strchr(src, '\\'))
        return -1;
    if (!disk && src[0] != '/') {
        const char *dir = guest_current_dir();
        size_t dir_len = strlen(dir);
        if (dir_len + length + 2 > sizeof(joined))
            return -1;
        memcpy(joined, dir, dir_len);
        joined[dir_len] = '/';
        memcpy(joined + dir_len + 1, src, length + 1);
        src = joined;
    }
    /* DiskStrA's standalone '~' component resets the path to home, even
     * after earlier components. HomeSet stores a canonical absolute path in
     * the real guest blkdev globals; do not cache the string it can replace. */
    const char *suffix = guest_home_suffix(src);
    if (suffix) {
        const char *home = guest_home_dir_ptr ? *guest_home_dir_ptr : NULL;
        if (!home || *home != '/' || strchr(home, ':') ||
            strlen(home) + strlen(suffix) + 1 > sizeof(expanded))
            return -1;
        size_t home_len = strlen(home);
        memcpy(expanded, home, home_len);
        memcpy(expanded + home_len, suffix, strlen(suffix) + 1);
        src = expanded;
    }
    if (normal_path(src, name))
        return -1;
    size_t prefix = disk ? 5 : 0;
    if (disk)
        memcpy(out, "disk:", prefix);
    out[prefix] = '/';
    memcpy(out + prefix + 1, name, strlen(name) + 1);
    return 0;
}
static int64_t host_source_path(int64_t *a) {
    char path[134];
    if (guest_path((const char *)(uintptr_t)a[0], path))
        return 0;
    CHeapCtrl *owner = guest_task_bound ? &guest_fibers[guest_current].heap_owner : &data_heap;
    return (int64_t)(uintptr_t)__AIWNIOS_StrDup(path, owner);
}
static int octal(const unsigned char *p, size_t n, size_t *out) {
    size_t r = 0;
    int seen = 0;
    for (size_t i = 0; i < n; i++) {
        if (!p[i] || p[i] == ' ') {
            if (seen)
                break;
            else
                continue;
        }
        if (p[i] < '0' || p[i] > '7' || r > SIZE_MAX / 8)
            return -1;
        r = r * 8 + (p[i] - '0');
        seen = 1;
    }
    *out = r;
    return 0;
}
static const unsigned char *source_find(const char *path, size_t *size) {
    if (!source_archive)
        return NULL;
    char name[128];
    if (normal_path(path, name))
        return NULL;
    for (size_t off = 0; off <= archive_size && archive_size - off >= 512;) {
        const unsigned char *h = source_archive + off;
        if (!*h)
            return NULL;
        size_t n, check;
        if (!memchr(h, 0, 100))
            return NULL;
        if (memcmp(h + 257, "ustar", 5) || octal(h + 124, 12, &n) || octal(h + 148, 8, &check))
            return NULL;
        size_t sum = 0;
        for (unsigned i = 0; i < 512; i++)
            sum += i >= 148 && i < 156 ? ' ' : h[i];
        if (sum != check || n > archive_size - off - 512)
            return NULL;
        if ((h[156] == '0' || !h[156]) && !strcmp(name, (const char *)h)) {
            *size = n;
            return h + 512;
        }
        if (n > SIZE_MAX - 511)
            return NULL;
        size_t padded = (n + 511) & ~(size_t)511;
        if (padded > archive_size - off - 512)
            return NULL;
        off += 512 + padded;
    }
    return NULL;
}
struct source_list_entry {
    char path[256];
    size_t size;
};
struct source_overlay_entry {
    char name[96];
    size_t size;
    int status;
};
static struct source_list_entry source_list_cache[4096];
static struct source_overlay_entry source_overlay_cache[4096];
static size_t source_list_cache_count;
static size_t source_overlay_cache_count;
static int source_list_cache_ready;
static uint64_t source_list_cache_revision;
static void source_list_cache_add(const char *path, size_t size) {
    size_t length = strlen(path);
    if (source_list_cache_count >=
            sizeof(source_list_cache) / sizeof(source_list_cache[0]) ||
        length + 1 > sizeof(source_list_cache[0].path))
        return;
    struct source_list_entry *entry =
        &source_list_cache[source_list_cache_count++];
    memcpy(entry->path, path, length + 1);
    entry->size = size;
}
static void source_list_cache_build(void) {
    char path[256];
    size_t size = 0;
    source_overlay_cache_count = 0;
    for (uint32_t index = 0;
         source_overlay_cache_count <
             sizeof(source_overlay_cache) / sizeof(source_overlay_cache[0]);
         index++) {
        char name[96];
        int status = zeal_zss_entry_at(index, name, sizeof(name), &size);
        if (!status)
            break;
        struct source_overlay_entry *entry =
            &source_overlay_cache[source_overlay_cache_count++];
        size_t length = strlen(name);
        memcpy(entry->name, name, length + 1);
        entry->size = size;
        entry->status = status;
    }
    source_list_cache_count = 0;
    if (source_archive) {
        for (size_t off = 0; off <= archive_size &&
                             archive_size - off >= 512;) {
            const unsigned char *h = source_archive + off;
            if (!*h) break;
            size_t n, check, name_len = 0, prefix_len = 0;
            while (name_len < 100 && h[name_len]) name_len++;
            while (prefix_len < 155 && h[345 + prefix_len]) prefix_len++;
            if (name_len == 100 || prefix_len == 155 ||
                memcmp(h + 257, "ustar", 5) ||
                octal(h + 124, 12, &n) || octal(h + 148, 8, &check) ||
                n > archive_size - off - 512)
                break;
            size_t sum = 0;
            for (unsigned i = 0; i < 512; i++)
                sum += i >= 148 && i < 156 ? ' ' : h[i];
            if (sum != check) break;
            if (h[156] == '0' || !h[156]) {
                size_t total_len = name_len + (prefix_len ? prefix_len + 1 : 0);
                if (total_len + 1 > sizeof(path)) break;
                if (prefix_len) {
                    memcpy(path, h + 345, prefix_len);
                    path[prefix_len] = '/';
                }
                memcpy(path + (prefix_len ? prefix_len + 1 : 0), h, name_len);
                path[total_len] = 0;
                const char *native_path = path[0] == '/' ? path + 1 : path;
                int status = 0;
                size_t native_size = 0;
                for (size_t i = 0; i < source_overlay_cache_count; i++) {
                    if (!strcmp(source_overlay_cache[i].name, native_path)) {
                        status = source_overlay_cache[i].status;
                        native_size = source_overlay_cache[i].size;
                        break;
                    }
                }
                if (status != -3) {
                    size = status == 1 ? native_size : n;
                    source_list_cache_add(path, size);
                }
            }
            if (n > SIZE_MAX - 511) break;
            size_t padded = (n + 511) & ~(size_t)511;
            if (padded > archive_size - off - 512) break;
            off += 512 + padded;
        }
    }
    for (size_t index = 0; index < source_overlay_cache_count; index++) {
        const struct source_overlay_entry *overlay =
            &source_overlay_cache[index];
        if (overlay->status != 1)
            continue;
        size_t native_length = strlen(overlay->name);
        if (native_length + 1 > sizeof(path)) continue;
        memcpy(path, overlay->name, native_length + 1);
        size_t archive_size = 0;
        if (!source_find(path, &archive_size))
            source_list_cache_add(path, overlay->size);
    }
    source_list_cache_ready = 1;
    source_list_cache_revision = zeal_zss_catalog_revision();
}
static void source_list_cache_ensure(void) {
    if (!source_list_cache_ready ||
        source_list_cache_revision != zeal_zss_catalog_revision())
        source_list_cache_build();
}
static int64_t host_source_file_at(int64_t *a) {
    if (a[0] < 0 || a[2] <= 0)
        return 0;
    if (a[0] == 0 || !source_list_cache_ready ||
        source_list_cache_revision != zeal_zss_catalog_revision())
        source_list_cache_build();
    if ((uint64_t)a[0] >= source_list_cache_count)
        return 0;
    const struct source_list_entry *entry = &source_list_cache[a[0]];
    size_t length = strlen(entry->path);
    if (length + 1 > (size_t)a[2])
        return 0;
    memcpy((char *)(uintptr_t)a[1], entry->path, length + 1);
    if (a[3])
        *(int64_t *)(uintptr_t)a[3] = (int64_t)entry->size;
    return 1;
}
static int source_dir_exists(const char *path) {
    char name[128];
    char prefix[130];
    if (!path || normal_path(path, name))
        return 0;
    if (!*name)
        return 1;
    size_t name_len = strlen(name);
    if (name_len + 1 >= sizeof(prefix))
        return 0;
    memcpy(prefix, name, name_len);
    prefix[name_len] = '/';
    prefix[name_len + 1] = 0;
    if (source_archive) {
        for (size_t off = 0; off <= archive_size && archive_size - off >= 512;) {
            const unsigned char *h = source_archive + off;
            if (!*h) break;
            size_t n, check;
            if (!memchr(h, 0, 100) || memcmp(h + 257, "ustar", 5) ||
                octal(h + 124, 12, &n) || octal(h + 148, 8, &check) ||
                n > archive_size - off - 512)
                break;
            size_t sum = 0;
            for (unsigned i = 0; i < 512; i++)
                sum += i >= 148 && i < 156 ? ' ' : h[i];
            if (sum != check) break;
            if (h[156] == '5') {
                size_t dir_len = strlen((const char *)h);
                if (!strcmp(name, (const char *)h) ||
                    (dir_len == name_len + 1 && h[name_len] == '/' &&
                     !memcmp(name, h, name_len)))
                    return 1;
            }
            if (n > SIZE_MAX - 511) break;
            size_t padded = (n + 511) & ~(size_t)511;
            if (padded > archive_size - off - 512) break;
            off += 512 + padded;
        }
    }
    source_list_cache_ensure();
    for (size_t i = 0; i < source_list_cache_count; i++)
        if (!strncmp(source_list_cache[i].path, prefix, name_len + 1))
            return 1;
    return 0;
}
static int64_t host_file_dir_exists(int64_t *a) {
    char path[134];
    if (guest_path((const char *)(uintptr_t)a[0], path))
        return 0;
    return source_dir_exists(path + (!strncmp(path, "disk:", 5) ? 5 : 0));
}
static int64_t host_source_writable(int64_t *a) {
    char path[134];
    if (guest_path((const char *)(uintptr_t)a[0], path)) return 0;
    const char *name = path + (!strncmp(path, "disk:", 5) ? 6 : 1);
    return *name && strlen(name) <= 95 && zeal_zss_writable();
}
static int disk_name(const char *path, char name[128]) {
    if (strncmp(path, "disk:", 5) || normal_path(path + 5, name)) return -1;
    size_t n = strlen(name);
    if (!n || n > 95) return -1;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)name[i];
        if (c < 33 || c > 126 || c == ':' || c == '\\') return -1;
    }
    return 0;
}
static char *disk_copy(const char *path, size_t *length, int *tombstoned) {
    char name[128];
    size_t n = 0, got = 0;
    if (tombstoned)
        *tombstoned = 0;
    if (disk_name(path, name) || !disk_reader)
        return NULL;
    int status = disk_reader(name, NULL, 0, &n);
    if (status == -3 && tombstoned)
        *tombstoned = 1;
    if (status || !n || n > 4u * 1024u * 1024u)
        return NULL;
    char *src = malloc(n + 1);
    if (!src || disk_reader(name, src, n + 1, &got) || got != n || memchr(src, 0, n)) {
        free(src);
        return NULL;
    }
    src[n] = 0;
    *length = n;
    return src;
}
static const unsigned char *source_file_find(const char *path, size_t *length,
                                              char **owned) {
    const unsigned char *data;
    int tombstoned = 0;
    *owned = NULL;
    if (!path)
        return NULL;
    if (!strncmp(path, "disk:", 5)) {
        *owned = disk_copy(path, length, NULL);
        return (const unsigned char *)*owned;
    }
    // Absolute ZealOS paths refer to the writable source volume first, then
    // fall back to the bundled source archive. This lets Ed reopen saved edits
    // instead of silently displaying the original bundled copy.
    char name[128];
    if (!normal_path(path, name) && strlen(name) <= 95) {
        char disk_path[101] = "disk:";
        memcpy(disk_path + 5, name, strlen(name) + 1);
        *owned = disk_copy(disk_path, length, &tombstoned);
        if (*owned)
            return (const unsigned char *)*owned;
        if (tombstoned)
            return NULL;
    }
    data = source_find(path, length);
    return data;
}
static int64_t host_file_read(int64_t *a) {
    char path[134];
    size_t length = 0;
    char *owned = NULL;
    if (a[1]) *(int64_t *)(uintptr_t)a[1] = 0;
    if (a[2]) *(int64_t *)(uintptr_t)a[2] = 0;
    if (guest_path((const char *)(uintptr_t)a[0], path))
        return 0;
    const unsigned char *data = source_file_find(path, &length, &owned);
    if (!data || length > INT64_MAX || length == SIZE_MAX) {
        free(owned);
        return 0;
    }
    CHeapCtrl *owner = guest_task_bound ? &guest_fibers[guest_current].heap_owner : &data_heap;
    char *contents = __AIWNIOS_MAlloc(length + 1, owner);
    if (!contents) {
        free(owned);
        return 0;
    }
    memcpy(contents, data, length);
    contents[length] = 0;
    if (a[1]) *(int64_t *)(uintptr_t)a[1] = (int64_t)length;
    free(owned);
    return (int64_t)(uintptr_t)contents;
}
static int64_t host_file_write(int64_t *a) {
    char path[134];
    const char *src = (const char *)(uintptr_t)a[1];
    int64_t length = a[2];
    if (guest_path((const char *)(uintptr_t)a[0], path) ||
        !src || length <= 0 || !disk_writer ||
        disk_writer(path, src, (size_t)length))
        return -1;
    return length;
}
FILE *fopen(const char *path, const char *mode) {
    if (strcmp(mode, "rb") && strcmp(mode, "r"))
        return NULL;
    /* The compiler already qualifies ordinary include names against their
     * source file. Only expand guest home/drive aliases here, so those names
     * work in #include as well as FileRead/ExeFile without double-joining cwd. */
    char resolved[134];
    if (guest_home_suffix(path) || !strncmp(path, "::", 2) || !strncmp(path, "~:", 2)) {
        if (guest_path(path, resolved)) return NULL;
        path = resolved;
    }
    size_t n;
    char *owned = NULL;
    const unsigned char *data;
    if (!strncmp(path, "disk:", 5)) {
        owned = disk_copy(path, &n, NULL);
        data = (const unsigned char *)owned;
    } else {
        data = source_file_find(path, &n, &owned);
    }
    if (!data)
        return NULL;
    for (unsigned i = 0; i < 16; i++)
        if (!files[i].used) {
            files[i] = (FILE){.data = (const char *)data, .owned = owned, .size = n, .used = 1};
            return &files[i];
        }
    free(owned);
    return NULL;
}
int fclose(FILE *f) {
    free(f->owned);
    f->owned = NULL;
    f->used = 0;
    return 0;
}
int fseek(FILE *f, long off, int whence) {
    int64_t base = whence == SEEK_SET   ? 0
                   : whence == SEEK_CUR ? (int64_t)f->pos
                   : whence == SEEK_END ? (int64_t)f->size
                                        : -1;
    if (base < 0 || off < -base || off > (int64_t)f->size - base)
        return -1;
    f->pos = (size_t)(base + off);
    return 0;
}
long ftell(FILE *f) { return (long)f->pos; }
size_t fread(void *d, size_t w, size_t n, FILE *f) {
    if (!w)
        return 0;
    size_t avail = (f->size - f->pos) / w;
    if (n > avail)
        n = avail;
    memcpy(d, f->data + f->pos, n * w);
    f->pos += n * w;
    return n;
}

static CHeapCtrl *guest_allocation_owner(int64_t selector) {
    if (guest_task_bound) {
        if (!selector)
            return &guest_fibers[guest_current].heap_owner;
        for (unsigned i = 0; i < GUEST_TASKS; i++) {
            struct guest_fiber *f = &guest_fibers[i];
            if (f->state == GUEST_RUNNABLE &&
                ((void *)(uintptr_t)selector == f->words ||
                 (void *)(uintptr_t)selector == f->heap_words))
                return &f->heap_owner;
        }
        zc_fail("allocation target is not a live guest task or heap");
    }
    return &data_heap;
}
static int64_t host_malloc(int64_t *a) {
    return (int64_t)(uintptr_t)__AIWNIOS_MAlloc(a[0], guest_allocation_owner(a[1]));
}
static int64_t host_system_task(int64_t *a) {
    (void)a;
    if (!guest_task_bound)
        return 0;
    unsigned i = guest_fibers[GUEST_SYSTEM].state == GUEST_RUNNABLE ? GUEST_SYSTEM : 0;
    return (int64_t)(uintptr_t)guest_fibers[i].words;
}
static int64_t host_sqrt(int64_t *a) {
    return (int64_t)hc_builtin_sqrt((uint64_t)a[0]);
}
static int64_t host_exp(int64_t *a) {
    return (int64_t)hc_builtin_exp((uint64_t)a[0]);
}
static int64_t host_sin_f64(int64_t *a) {
    return (int64_t)hc_builtin_sin((uint64_t)a[0]);
}
static int64_t host_cos_f64(int64_t *a) {
    return (int64_t)hc_builtin_cos((uint64_t)a[0]);
}
static double host_f64_from_bits(uint64_t bits) {
    double value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}
static uint64_t host_f64_to_bits(double value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}
/* Freestanding natural log for the pinned _LN import. Reduce x=2^e*m
 * to sqrt(1/2)<=m<=sqrt(2), then ln(m)=2*(z+z^3/3+...). The fixed
 * series is below binary64 precision; no hosted libm or x87 is required. */
static int64_t host_ln(int64_t *a) {
    uint64_t bits = (uint64_t)a[0], magnitude = bits & UINT64_C(0x7fffffffffffffff);
    if (magnitude > UINT64_C(0x7ff0000000000000))
        return (int64_t)(bits | UINT64_C(0x0008000000000000));
    if (!magnitude)
        return (int64_t)UINT64_C(0xfff0000000000000);
    if (bits >> 63)
        return (int64_t)UINT64_C(0x7ff8000000000000);
    if (magnitude == UINT64_C(0x7ff0000000000000))
        return (int64_t)bits;
    int exponent = 0;
    if (!(bits >> 52)) {
        bits = host_f64_to_bits(host_f64_from_bits(bits) * 0x1p52);
        exponent = -52;
    }
    exponent += (int)(bits >> 52) - 1023;
    double m = host_f64_from_bits((bits & UINT64_C(0x000fffffffffffff)) |
                                 UINT64_C(0x3ff0000000000000));
    /* Center on 1 so values just below 1 do not subtract two ln(2)s. */
    if (m > 0x1.6a09e667f3bcdp0) {
        m *= 0.5;
        exponent++;
    }
    double z = (m - 1.0) / (m + 1.0), z2 = z * z, term = z, sum = z;
    for (unsigned n = 3; n <= 51; n += 2) {
        term *= z2;
        sum += term / (double)n;
    }
    return (int64_t)host_f64_to_bits(2.0 * sum + exponent * 0x1.62e42fefa39efp-1);
}
static double host_sqrt_value(double value) {
    return host_f64_from_bits(hc_builtin_sqrt(host_f64_to_bits(value)));
}
/* KernelB's _D3_* imports use a pointer-based CD3 ABI.  The guest compiler's
 * C-symbol bridge passes each ZealOS word unchanged, including F64 bit
 * patterns, so keep these adapters word-oriented and preserve aliasing (for
 * example D3Add(&v, &v, &w)). */
typedef struct { double x, y, z; } host_cd3;
static int64_t host_d3_add(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    const host_cd3 *left = (const host_cd3 *)(uintptr_t)a[1];
    const host_cd3 *right = (const host_cd3 *)(uintptr_t)a[2];
    double x = left->x + right->x, y = left->y + right->y,
           z = left->z + right->z;
    out->x = x; out->y = y; out->z = z;
    return a[0];
}
static int64_t host_d3_add_equ(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    const host_cd3 *right = (const host_cd3 *)(uintptr_t)a[1];
    out->x += right->x; out->y += right->y; out->z += right->z;
    return a[0];
}
static int64_t host_d3_copy(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    const host_cd3 *in = (const host_cd3 *)(uintptr_t)a[1];
    double x = in->x, y = in->y, z = in->z;
    out->x = x; out->y = y; out->z = z;
    return a[0];
}
static int64_t host_d3_cross(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    const host_cd3 *left = (const host_cd3 *)(uintptr_t)a[1];
    const host_cd3 *right = (const host_cd3 *)(uintptr_t)a[2];
    double x = left->y * right->z - left->z * right->y;
    double y = left->z * right->x - left->x * right->z;
    double z = left->x * right->y - left->y * right->x;
    out->x = x; out->y = y; out->z = z;
    return a[0];
}
static int64_t host_d3_dist_sqr(int64_t *a) {
    const host_cd3 *left = (const host_cd3 *)(uintptr_t)a[0];
    const host_cd3 *right = (const host_cd3 *)(uintptr_t)a[1];
    double x = left->x - right->x, y = left->y - right->y,
           z = left->z - right->z;
    return (int64_t)host_f64_to_bits(x * x + y * y + z * z);
}
static int64_t host_d3_dist(int64_t *a) {
    return (int64_t)host_f64_to_bits(host_sqrt_value(
        host_f64_from_bits((uint64_t)host_d3_dist_sqr(a))));
}
static int64_t host_d3_div(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    const host_cd3 *in = (const host_cd3 *)(uintptr_t)a[1];
    double divisor = host_f64_from_bits((uint64_t)a[2]);
    double x = in->x / divisor, y = in->y / divisor, z = in->z / divisor;
    out->x = x; out->y = y; out->z = z;
    return a[0];
}
static int64_t host_d3_div_equ(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    double divisor = host_f64_from_bits((uint64_t)a[1]);
    out->x /= divisor; out->y /= divisor; out->z /= divisor;
    return a[0];
}
static int64_t host_d3_dot(int64_t *a) {
    const host_cd3 *left = (const host_cd3 *)(uintptr_t)a[0];
    const host_cd3 *right = (const host_cd3 *)(uintptr_t)a[1];
    return (int64_t)host_f64_to_bits(left->x * right->x +
                                     left->y * right->y +
                                     left->z * right->z);
}
static int64_t host_d3_equ(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    out->x = host_f64_from_bits((uint64_t)a[1]);
    out->y = host_f64_from_bits((uint64_t)a[2]);
    out->z = host_f64_from_bits((uint64_t)a[3]);
    return a[0];
}
static int64_t host_d3_mul(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    double scale = host_f64_from_bits((uint64_t)a[1]);
    const host_cd3 *in = (const host_cd3 *)(uintptr_t)a[2];
    double x = scale * in->x, y = scale * in->y, z = scale * in->z;
    out->x = x; out->y = y; out->z = z;
    return a[0];
}
static int64_t host_d3_mul_equ(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    double scale = host_f64_from_bits((uint64_t)a[1]);
    out->x *= scale; out->y *= scale; out->z *= scale;
    return a[0];
}
static int64_t host_d3_norm_sqr(int64_t *a) {
    const host_cd3 *v = (const host_cd3 *)(uintptr_t)a[0];
    return (int64_t)host_f64_to_bits(v->x * v->x + v->y * v->y + v->z * v->z);
}
static int64_t host_d3_norm(int64_t *a) {
    return (int64_t)host_f64_to_bits(host_sqrt_value(
        host_f64_from_bits((uint64_t)host_d3_norm_sqr(a))));
}
static int64_t host_d3_sub(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    const host_cd3 *left = (const host_cd3 *)(uintptr_t)a[1];
    const host_cd3 *right = (const host_cd3 *)(uintptr_t)a[2];
    double x = left->x - right->x, y = left->y - right->y,
           z = left->z - right->z;
    out->x = x; out->y = y; out->z = z;
    return a[0];
}
static int64_t host_d3_sub_equ(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    const host_cd3 *right = (const host_cd3 *)(uintptr_t)a[1];
    out->x -= right->x; out->y -= right->y; out->z -= right->z;
    return a[0];
}
static int64_t host_d3_unit(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    double norm = host_sqrt_value(out->x * out->x + out->y * out->y + out->z * out->z);
    if (norm != 0) { out->x /= norm; out->y /= norm; out->z /= norm; }
    return a[0];
}
static int64_t host_d3_zero(int64_t *a) {
    host_cd3 *out = (host_cd3 *)(uintptr_t)a[0];
    out->x = out->y = out->z = 0;
    return a[0];
}
static int64_t host_low_pass1(int64_t *a) {
    double alpha = host_f64_from_bits((uint64_t)a[0]);
    double y0 = host_f64_from_bits((uint64_t)a[1]);
    double y = host_f64_from_bits((uint64_t)a[2]);
    double dt = host_f64_from_bits((uint64_t)a[3]);
    double decay = host_f64_from_bits(
        hc_builtin_exp((uint64_t)(int64_t)(-alpha * dt)));
    double result = y0 * decay + y * (1.0 - decay);
    uint64_t bits;
    memcpy(&bits, &result, sizeof(bits));
    return (int64_t)bits;
}
static int64_t host_clamp_f64(int64_t *a) {
    double value = host_f64_from_bits((uint64_t)a[0]);
    double lo = host_f64_from_bits((uint64_t)a[1]);
    double hi = host_f64_from_bits((uint64_t)a[2]);
    uint64_t bits;
    value = value < lo ? lo : value > hi ? hi : value;
    memcpy(&bits, &value, sizeof(bits));
    return (int64_t)bits;
}
static int64_t host_max_f64(int64_t *a) {
    double left = host_f64_from_bits((uint64_t)a[0]);
    double right = host_f64_from_bits((uint64_t)a[1]);
    uint64_t bits;
    double result = left > right ? left : right;
    memcpy(&bits, &result, sizeof(bits));
    return (int64_t)bits;
}
static int64_t host_calloc(int64_t *a) {
    return (int64_t)(uintptr_t)__AIWNIOS_CAlloc(a[0], guest_allocation_owner(a[1]));
}
static int64_t host_free(int64_t *a) {
    free((void *)(uintptr_t)a[0]);
    return 0;
}
static int64_t host_msize(int64_t *a) { return MSize((void *)(uintptr_t)a[0]); }
static int64_t host_copy(int64_t *a) {
    return (int64_t)(uintptr_t)memcpy((void *)(uintptr_t)a[0], (void *)(uintptr_t)a[1],
                                      (size_t)a[2]);
}
/* ARM64 replacement for GrAsm.ZC's _DC_BLOT_COLOR4. Each U64 in the source
 * holds eight packed 4-bit pixels. The destination stores one byte per eight
 * pixels in each of four color bitplanes; the cache avoids rewriting unchanged
 * groups. */
static int64_t host_dc_blot_color4(int64_t *a) {
    uint8_t *dst = (uint8_t *)(uintptr_t)a[0];
    uint64_t *img = (uint64_t *)(uintptr_t)a[1];
    uint64_t *cache = (uint64_t *)(uintptr_t)a[2];
    if (a[3] < 0 || (uint64_t)a[3] > SIZE_MAX / 4 ||
        (a[3] && (!dst || !img || !cache)))
        return -1;
    size_t count = (size_t)a[3];
    for (size_t i = 0; i < count; i++) {
        uint64_t pixels = img[i];
        if (pixels == cache[i])
            continue;
        cache[i] = pixels;
        for (unsigned plane = 0; plane < 4; plane++) {
            uint8_t packed = 0;
            for (unsigned pixel = 0; pixel < 8; pixel++)
                packed |= (uint8_t)(((pixels >> (pixel * 8 + plane)) & 1) << pixel);
            dst[(size_t)plane * count + i] = packed;
        }
    }
    return 0;
}
/* ARM64 replacement for GrAsm.ZC's x86 GrRopEquU8NoClipping. The original
 * lookup tables expand one glyph bit into an eight-pixel mask and one text
 * attribute into eight copies of a palette index. This scalar version keeps
 * the same row-stride/transparent-foreground behavior without x86 assembly. */
static int64_t host_gr_rop_equ_u8_no_clipping(int64_t *a) {
    uint64_t packed = (uint64_t)a[0];
    uint8_t *dst = (uint8_t *)(uintptr_t)a[1];
    int64_t stride = a[2];
    if (!dst || stride <= 0 || stride > (1 << 24))
        return -1;

    unsigned ch = (unsigned)(packed & 0xffu);
    uint8_t fg = (uint8_t)guest_gr_colors[(packed >> 8) & 0xffu];
    const uint8_t *rows = ch >= 0x80
        ? fb_glyph8(ch) : (const uint8_t *)&guest_gr_font[ch];
    int underline = (packed & 0x80000000u) != 0;
    for (unsigned y = 0; y < 8; y++) {
        uint8_t bits = rows[y];
        if (underline && y == 7)
            bits = 0xff;
        if (!bits)
            continue;
        for (unsigned x = 0; x < 8; x++)
            if (bits & (0x80u >> x))
                dst[(size_t)y * (size_t)stride + x] = fg;
    }
    return 0;
}

static void init_guest_gr_tables(void) {
    if (guest_gr_tables_ready)
        return;
    for (unsigned ch = 0; ch < 256; ch++) {
        unsigned font_ch = ch;
        if (font_ch < 32) {
            guest_gr_font[ch] = 0;
            continue;
        }
        if (font_ch > 127)
            font_ch = '?';
        memcpy(&guest_gr_font[ch], g_font8[font_ch - 32], 8);
    }
    /* GrUpdateTextFG masks the attribute to the low foreground nibble before
     * calling GrRopEquU8NoClipping. The renderer's
     * intermediate surface is palette-indexed; foreground-only stores use
     * that four-bit color until GrInit/display palette setup is ported. */
    for (unsigned attr = 0; attr < 256; attr++)
        guest_gr_colors[attr] = attr & 0x0f;
    guest_gr_tables_ready = 1;
}
static int64_t host_compare(int64_t *a) {
    if (a[2] < 0)
        return 0;
    return memcmp((const void *)(uintptr_t)a[0],
                  (const void *)(uintptr_t)a[1], (size_t)a[2]);
}
static int64_t host_define_mirror(int64_t *a) {
    const char *name = (const char *)(uintptr_t)a[0];
    const char *value = (const char *)(uintptr_t)a[1];
    CHashDefineStr *define;
    if (!name || !value)
        return 0;
    define = (CHashDefineStr *)HashFind((char *)name, Fs->hash_table,
                                         HTT_DEFINE_STR, 1);
    if (define) {
        A_FREE(define->data);
        define->data = A_STRDUP((char *)value, NULL);
    } else {
        define = A_CALLOC(sizeof(*define), NULL);
        define->base.str = A_STRDUP((char *)name, NULL);
        define->base.type = HTT_DEFINE_STR;
        define->data = A_STRDUP((char *)value, NULL);
        HashAdd(&define->base, Fs->hash_table);
    }
    return 0;
}
static int64_t host_set(int64_t *a) {
    return (int64_t)(uintptr_t)memset((void *)(uintptr_t)a[0], (int)a[1], (size_t)a[2]);
}
static int64_t host_memset_u32(int64_t *a) {
    uint32_t *dst = (uint32_t *)(uintptr_t)a[0];
    uint32_t value = (uint32_t)a[1];
    int64_t count = a[2];
    if (count < 0 || (uint64_t)count > SIZE_MAX / sizeof(*dst))
        return 0;
    for (int64_t i = 0; i < count; i++)
        dst[i] = value;
    return (int64_t)(uintptr_t)dst;
}
static int64_t host_puts(int64_t *a) {
    const char *text = (const char *)(uintptr_t)a[0];
    zc_output((void *)text);
    guest_task_text_output(text);
    return 0;
}
static void guest_task_text_output(const char *text) {
    if (!guest_task_bound || !text || guest_current >= GUEST_TASKS ||
        guest_fibers[guest_current].state != GUEST_RUNNABLE)
        return;
    struct guest_fiber *f = &guest_fibers[guest_current];
    /* Parser diagnostics are replayed only after the checkpoint commits or
     * rolls back. Doc mutations must never be undone by compiler recovery. */
    if (f->user_diagnostic_active) return;
    /* Both the pinned InputFilterTask (a normal Spawn) and the native filter
     * publish TASKf_INPUT_FILTER_TASK. Their formatted output is StdIn for
     * the linked recipient, through the original KeyDev message producer. */
    if (*guest_flags(f) & GUEST_INPUT_FILTER_TASK) {
        CHashFun *put = (CHashFun *)HashFind(
            "KDInputFilterPutS", Fs->hash_table, HTT_FUN, 1);
        if (put && put->fun_ptr && put->argc == 1 &&
            !(put->base.base.type & HTF_EXTERN)) {
            FFI_CALL_TOS_1(put->fun_ptr, (int64_t)(uintptr_t)text);
            return;
        }
    }
    if (f->user_output_writer && !f->user_output_suppressed) {
        FFI_CALL_TOS_1(f->user_output_writer, (int64_t)(uintptr_t)text);
        return;
    }
    unsigned char *task = (unsigned char *)guest_fibers[guest_current].words;
    int64_t left = *(int64_t *)(task + guest_win_left_off);
    int64_t right = *(int64_t *)(task + guest_win_right_off);
    int64_t top = *(int64_t *)(task + guest_win_top_off);
    int64_t bottom = *(int64_t *)(task + guest_win_bottom_off);
    uint32_t attr = *(uint8_t *)(task + guest_text_attr_off);
    (void)zeal_fb_task_text_write(guest_current, left, right, top, bottom,
                                  attr, text);
}
static int64_t host_swap(int64_t *a) {
    int64_t *x = (void *)(uintptr_t)a[0], *y = (void *)(uintptr_t)a[1], v = *x;
    *x = *y;
    *y = v;
    return v;
}
/* GenFFIBinding passes X0 = argv[]; raw CHash* APIs must not be bound directly. */
static int64_t host_hash_table_new(int64_t *a) {
    return (int64_t)(uintptr_t)HashTableNew(a[0], (void *)(uintptr_t)a[1]);
}
static int64_t host_hash_table_del(int64_t *a) {
    if (a[0])
        HashTableDel((CHashTable *)(uintptr_t)a[0]);
    return 0;
}
static int64_t host_hash_find(int64_t *a) {
    return (int64_t)(uintptr_t)HashFind((char *)(uintptr_t)a[0],
                                        (CHashTable *)(uintptr_t)a[1], a[2],
                                        a[3]);
}
static int64_t host_hash_add(int64_t *a) {
    HashAdd((CHash *)(uintptr_t)a[0], (CHashTable *)(uintptr_t)a[1]);
    return 0;
}
static uint32_t *guest_text_plane(int64_t address, int64_t stride,
                                  int64_t rows) {
    uintptr_t p = (uintptr_t)address;
    size_t bytes;
    int in_data, in_code;
    if (!p || p % _Alignof(uint32_t) || stride <= 0 || stride > 512 ||
        rows <= 0 || rows > 512 ||
        (uint64_t)stride * (uint64_t)rows > SIZE_MAX / sizeof(uint32_t))
        return NULL;
    bytes = (size_t)stride * (size_t)rows * sizeof(uint32_t);
    if (p > UINTPTR_MAX - bytes)
        return NULL;
    in_data = p >= (uintptr_t)data_arena &&
              p + bytes <= (uintptr_t)data_arena + sizeof(data_arena);
    in_code = p >= (uintptr_t)code_arena &&
              p + bytes <= (uintptr_t)code_arena + sizeof(code_arena);
    return in_data || in_code ? (uint32_t *)p : NULL;
}
static uint8_t *guest_graphics_surface(void) {
    CHashGlblVar *global = (CHashGlblVar *)HashFind(
        "bootstrap_graphics_pixels", Fs->hash_table, HTT_GLBL_VAR, 1);
    uintptr_t p;
    size_t bytes = 800u * 600u;
    if (!global || !global->data_addr)
        return NULL;
    p = (uintptr_t)global->data_addr;
    if (p > UINTPTR_MAX - bytes)
        return NULL;
    if ((p >= (uintptr_t)data_arena &&
         p + bytes <= (uintptr_t)data_arena + sizeof(data_arena)) ||
        (p >= (uintptr_t)code_arena &&
         p + bytes <= (uintptr_t)code_arena + sizeof(code_arena)))
        return (uint8_t *)p;
    return NULL;
}
static uint32_t *guest_graphics_rgba_surface(void) {
    CHashGlblVar *global = (CHashGlblVar *)HashFind(
        "bootstrap_present_raw", Fs->hash_table, HTT_GLBL_VAR, 1);
    uintptr_t p;
    size_t bytes = 800u * 600u * sizeof(uint32_t);
    if (!global || !global->data_addr)
        return NULL;
    p = (uintptr_t)global->data_addr;
    if (p > UINTPTR_MAX - bytes)
        return NULL;
    if ((p >= (uintptr_t)data_arena &&
         p + bytes <= (uintptr_t)data_arena + sizeof(data_arena)) ||
        (p >= (uintptr_t)code_arena &&
         p + bytes <= (uintptr_t)code_arena + sizeof(code_arena)))
        return (uint32_t *)p;
    return NULL;
}
static int guest_source_full_screen_active(void) {
    CHashGlblVar *global = (CHashGlblVar *)HashFind(
        "bootstrap_gr_fullscreen_active", Fs->hash_table, HTT_GLBL_VAR, 1);
    return global && global->data_addr && *(uint64_t *)global->data_addr;
}
static int64_t guest_task_frame_run(void) {
    if (guest_task_frame_active)
        return 0;
    uint8_t *surface = guest_graphics_surface();
    uint32_t *rgba = guest_graphics_rgba_surface();
    CHashFun *present;
    uint64_t presented;
    if (!guest_winmgr_tick_fun)
        guest_winmgr_tick_fun = (CHashFun *)HashFind(
            "BootstrapWinMgrTick", Fs->hash_table, HTT_FUN, 1);
    if (!surface || !rgba || !guest_winmgr_tick_fun ||
        !guest_winmgr_tick_fun->fun_ptr || guest_winmgr_tick_fun->argc ||
        (guest_winmgr_tick_fun->base.base.type & HTF_EXTERN) ||
        !zeal_fb_task_frame_begin(surface, rgba, 800, 600))
        return 0;
    set_guest_task(guest_fibers[0].words);
    guest_task_frame_active = 1;
    guest_task_frame_rgba_ready = 0;
    /* TaskBridge's frame helpers are emitted before WinZBufUpdate's body.
     * Resolve the completed source function here so geometry/focus changes
     * cannot leave the previous full-screen window's occlusion cells behind. */
    CHashFun *z_update = (CHashFun *)HashFind(
        "WinZBufUpdate", Fs->hash_table, HTT_FUN, 1);
    if (z_update && z_update->fun_ptr && !z_update->argc &&
        !(z_update->base.base.type & HTF_EXTERN))
        FFI_CALL_TOS_0(z_update->fun_ptr);
    FFI_CALL_TOS_0(guest_winmgr_tick_fun->fun_ptr);
    /* The source incremental screen pass must finish in the staging buffer
     * before scanout. Running it after frame_end caused visible pixel-by-pixel
     * updates when overlapping windows changed beneath one another. */
    present = (CHashFun *)HashFind(
        "BootstrapGrPresent", Fs->hash_table, HTT_FUN, 1);
    if (present && present->fun_ptr && !present->argc &&
        !(present->base.base.type & HTF_EXTERN))
        FFI_CALL_TOS_0(present->fun_ptr);
    presented = zeal_fb_task_frame_end(guest_source_full_screen_active() ||
                                       guest_task_frame_rgba_ready) != 0;
    if (presented)
        guest_task_frame_count++;
    guest_task_frame_active = 0;
    guest_task_frame_rgba_ready = 0;
    return presented;
}
static int64_t host_task_frame_run(int64_t *a) {
    unsigned caller = guest_current;
    int had_guest_task = guest_task_bound;
    (void)a;
    /* The compositor is rooted in the shell task. When WinMgrSleep calls
     * into it, switch the scheduler identity and ZealOS TLS together. */
    if (had_guest_task) {
        guest_current = 0;
        Fs = &task;
        set_guest_task(guest_fibers[0].words);
    }
    int64_t presented = guest_task_frame_run();
    if (had_guest_task && caller < GUEST_TASKS) {
        guest_current = caller;
        Fs = caller ? &guest_fibers[caller].compiler_task : &task;
        set_guest_task(guest_fibers[caller].words);
    }
    return presented;
}
static int64_t host_gr_text_update(int64_t *a) {
    CHashFun *background, *foreground;
    (void)a;
    if (!Fs || !Fs->hash_table)
        return 0;
    background = (CHashFun *)HashFind("GrUpdateTextBG", Fs->hash_table,
                                     HTT_FUN, 1);
    foreground = (CHashFun *)HashFind("GrUpdateTextFG", Fs->hash_table,
                                      HTT_FUN, 1);
    if (!background || !background->fun_ptr || background->argc ||
        (background->base.base.type & HTF_EXTERN) ||
        !foreground || !foreground->fun_ptr || foreground->argc ||
        (foreground->base.base.type & HTF_EXTERN))
        return 0;
    FFI_CALL_TOS_0(background->fun_ptr);
    FFI_CALL_TOS_0(foreground->fun_ptr);
    return 1;
}
static int64_t host_gr_screen_update(int64_t *a) {
    CHashFun *update;
    CHashFun *palette_set;
    CHashGlblVar *raw_screen;
    CHashGlblVar *standard_palette;
    CHashGlblVar *palette;
    (void)a;
    if (!Fs || !Fs->hash_table || !guest_fb_addr || guest_fb_width != 800 ||
        guest_fb_height != 600 || guest_fb_pitch != 800 * sizeof(uint32_t) ||
        guest_fb_bpp != 32)
        return 0;
    raw_screen = (CHashGlblVar *)HashFind(
        "bootstrap_present_raw", Fs->hash_table, HTT_GLBL_VAR, 1);
    if (!raw_screen || !raw_screen->data_addr)
        return 0;
    /* Convert through the validated staging plane and keep source scanout
     * writes staged while a host frame is in progress. */
    guest_text_globals[2] = (uint64_t)(uintptr_t)raw_screen->data_addr;
    guest_text_globals[3] = guest_task_frame_active
        ? (uint64_t)(uintptr_t)raw_screen->data_addr
        : guest_fb_addr;
    guest_text_globals[9] = guest_fb_pitch * guest_fb_height;
    {
        palette_set = (CHashFun *)HashFind("GrPaletteSet", Fs->hash_table,
                                           HTT_FUN, 1);
        standard_palette = (CHashGlblVar *)HashFind(
            "gr32_palette_std", Fs->hash_table, HTT_GLBL_VAR, 1);
        palette = (CHashGlblVar *)HashFind("gr_palette", Fs->hash_table,
                                           HTT_GLBL_VAR, 1);
        if (!palette || !palette->data_addr)
            return 0;
        /* Keep an already-selected ZealOS palette; supply the normal palette
         * only when the incremental startup path has left it all-zero. */
        uint32_t *colors = (uint32_t *)palette->data_addr;
        int has_color = 0;
        for (size_t i = 0; i < 16; i++)
            has_color |= colors[i] != 0;
        if (!has_color) {
            if (!palette_set || !palette_set->fun_ptr ||
                palette_set->argc != 1 ||
                (palette_set->base.base.type & HTF_EXTERN) ||
                !standard_palette || !standard_palette->data_addr)
                return 0;
            FFI_CALL_TOS_1(palette_set->fun_ptr,
                           (int64_t)(uintptr_t)standard_palette->data_addr);
        }
    }
    update = (CHashFun *)HashFind("GrUpdateScreen32", Fs->hash_table,
                                  HTT_FUN, 1);
    if (!update || !update->fun_ptr || update->argc ||
        (update->base.base.type & HTF_EXTERN))
        return 0;
    FFI_CALL_TOS_0(update->fun_ptr);
    /* The upstream globals persist between calls. Restore the physical
     * framebuffer alias after the staged update so direct diagnostics and
     * non-frame source calls retain the normal Limine framebuffer contract. */
    if (guest_task_frame_active) {
        guest_task_frame_rgba_ready = 1;
        guest_text_globals[3] = guest_fb_addr;
    }
    return 1;
}
static int64_t host_gr_update_task_win(int64_t *a) {
    CHashFun *draw;
    CHashFun *redraw;
    CHashFun *terminal_state;
    if (!Fs || !Fs->hash_table || !a[0])
        return 0;
    draw = (CHashFun *)HashFind("GrUpdateTaskWin", Fs->hash_table,
                                HTT_FUN, 1);
    if (!draw || !draw->fun_ptr || draw->argc != 1 ||
        (draw->base.base.type & HTF_EXTERN))
        return 0;
    FFI_CALL_TOS_1(draw->fun_ptr, a[0]);
    /* The graphics modules are loaded before DolDoc during boot. Keep the
     * normal GrUpdateTaskWin path, then explicitly run the fully loaded
     * document renderer for every registered user terminal. This makes their
     * visible frames independent of early forward stubs. */
    if (gui_terminal_active) {
        terminal_state = (CHashFun *)HashFind(
            "BootstrapGuiTerminalState", Fs->hash_table, HTT_FUN, 1);
        redraw = (CHashFun *)HashFind("BootstrapGuiTerminalRedraw",
                                     Fs->hash_table, HTT_FUN, 1);
        if (terminal_state && terminal_state->fun_ptr && terminal_state->argc == 1 &&
            !(terminal_state->base.base.type & HTF_EXTERN) &&
            FFI_CALL_TOS_1(terminal_state->fun_ptr, a[0]) &&
            redraw && redraw->fun_ptr && redraw->argc == 1 &&
            !(redraw->base.base.type & HTF_EXTERN))
            FFI_CALL_TOS_1(redraw->fun_ptr, a[0]);
    }
    return 1;
}
static int64_t host_gr_update_tasks(int64_t *a) {
    CHashFun *update;
    (void)a;
    if (!Fs || !Fs->hash_table)
        return 0;
    update = (CHashFun *)HashFind("GrUpdateTasks", Fs->hash_table,
                                  HTT_FUN, 1);
    if (!update || !update->fun_ptr || update->argc ||
        (update->base.base.type & HTF_EXTERN))
        return 0;
    FFI_CALL_TOS_0(update->fun_ptr);
    return 1;
}
static int64_t host_gr_screen_full_update(int64_t *a) {
    CHashFun *update;
    (void)a;
    if (!Fs || !Fs->hash_table)
        return 0;
    update = (CHashFun *)HashFind("GrUpdateScreen", Fs->hash_table,
                                  HTT_FUN, 1);
    if (!update || !update->fun_ptr || update->argc ||
        (update->base.base.type & HTF_EXTERN))
        return 0;
    FFI_CALL_TOS_0(update->fun_ptr);
    return 1;
}
static int64_t host_ode_update_task(int64_t *a) {
    CHashFun *update;
    if (!Fs || !Fs->hash_table || !a[0])
        return 0;
    update = (CHashFun *)HashFind("ODEsUpdate", Fs->hash_table, HTT_FUN, 1);
    if (!update || !update->fun_ptr || update->argc != 1 ||
        (update->base.base.type & HTF_EXTERN))
        return 0;
    FFI_CALL_TOS_1(update->fun_ptr, a[0]);
    return 1;
}
static int64_t host_gr_update_task_odes(int64_t *a) {
    CHashFun *update;
    CHashFun *ode_update;
    if (!Fs || !Fs->hash_table || !a[0])
        return 0;
    /* GrScreen may be loaded before MathODE. Its ODE wrapper cannot run until
     * the source integrator exists; calling the wrapper early resolves its
     * optional bridge from an incomplete source graph. */
    ode_update = (CHashFun *)HashFind("ODEsUpdate", Fs->hash_table,
                                      HTT_FUN, 1);
    if (!ode_update || !ode_update->fun_ptr || ode_update->argc != 1 ||
        (ode_update->base.base.type & HTF_EXTERN))
        return 0;
    update = (CHashFun *)HashFind("GrUpdateTaskODEs", Fs->hash_table,
                                  HTT_FUN, 1);
    if (!update || !update->fun_ptr || update->argc != 1 ||
        (update->base.base.type & HTF_EXTERN))
        return 0;
    FFI_CALL_TOS_1(update->fun_ptr, a[0]);
    return 1;
}
static int64_t host_doc_recalc(int64_t *a) {
    CHashFun *fun = (CHashFun *)HashFind(
        "DocRecalc", Fs->hash_table, HTT_FUN, 1);
    if (!fun || !fun->fun_ptr || fun->fun_ptr == guest_doc_recalc_stub ||
        fun->argc != 2 || (fun->base.base.type & HTF_EXTERN))
        return 0;
    return FFI_CALL_TOS_2(fun->fun_ptr, a[0], a[1]);
}
static int64_t host_doc_update_task_docs(int64_t *a) {
    if (gui_terminal_active) {
        CHashFun *available = (CHashFun *)HashFind(
            "BootstrapGuiTerminalDocumentsAvailable", Fs->hash_table, HTT_FUN, 1);
        if (available && available->fun_ptr && available->argc == 1 &&
            !(available->base.base.type & HTF_EXTERN) &&
            !FFI_CALL_TOS_1(available->fun_ptr, a[0]))
            return 1;
    }
    CHashFun *fun = (CHashFun *)HashFind(
        "DocUpdateTaskDocs", Fs->hash_table, HTT_FUN, 1);
    if (!fun || !fun->fun_ptr || fun->argc != 1 ||
        (fun->base.base.type & HTF_EXTERN))
        return 0;
    (void)FFI_CALL_TOS_1(fun->fun_ptr, a[0]);
    return 1;
}
static int64_t host_active_gr_globals(int64_t *a) {
    CHashGlblVar *global;
    (void)a;
    if (!Fs || !Fs->hash_table)
        return 0;
    global = (CHashGlblVar *)HashFind("gr", Fs->hash_table, HTT_GLBL_VAR, 1);
    return global && global->data_addr
               ? (int64_t)(uintptr_t)global->data_addr
               : 0;
}
static int64_t host_active_mouse_globals(int64_t *a) {
    CHashGlblVar *global;
    (void)a;
    if (!Fs || !Fs->hash_table)
        return 0;
    global = (CHashGlblVar *)HashFind("mouse", Fs->hash_table, HTT_GLBL_VAR, 1);
    return global && global->data_addr
               ? (int64_t)(uintptr_t)global->data_addr
               : 0;
}
static int64_t host_active_winmgr_globals(int64_t *a) {
    CHashGlblVar *global;
    (void)a;
    if (!Fs || !Fs->hash_table)
        return 0;
    global = (CHashGlblVar *)HashFind("winmgr", Fs->hash_table,
                                     HTT_GLBL_VAR, 1);
    return global && global->data_addr
               ? (int64_t)(uintptr_t)global->data_addr
               : 0;
}
static int64_t host_set_winmgr_task(int64_t *a) {
    CHashGlblVar *global;
    int updated = 0;
    if (!Fs || !Fs->hash_table)
        return 0;
    /* TaskBridge is loaded at both KernelC and graphics stages. Functions
     * retain the data slot present when they were compiled; changing only
     * the newest slot leaves older hit-testing/frame code using sys_task as
     * its manager sentinel, so it can raise the background over applications. */
    for (int64_t instance = 1; (global = (CHashGlblVar *)HashFind(
             "sys_winmgr_task", Fs->hash_table, HTT_GLBL_VAR, instance)); instance++) {
        if (global->data_addr) {
            *(void **)global->data_addr = (void *)(uintptr_t)a[0];
            updated = 1;
        }
    }
    return updated;
}
static int64_t host_win_mouse_update(int64_t *a) {
    CHashFun *update;
    (void)a;
    if (!Fs || !Fs->hash_table)
        return 0;
    update = (CHashFun *)HashFind("WinMouseUpdate", Fs->hash_table,
                                  HTT_FUN, 1);
    if (!update || !update->fun_ptr || update->argc ||
        (update->base.base.type & HTF_EXTERN))
        return 0;
    FFI_CALL_TOS_0(update->fun_ptr);
    return 1;
}
static CMemberLst *guest_class_member(CHashClass *cls, const char *name) {
    for (CMemberLst *member = cls ? cls->members_lst : NULL; member;
         member = member->next) {
        if (member->str && !strcmp(member->str, name))
            return member;
    }
    return cls && cls->base_class ? guest_class_member(cls->base_class, name)
                                  : NULL;
}
static int guest_store_i64_member(void *data, CHashClass *cls,
                                  CMemberLst *member, int64_t value) {
    int64_t size;
    if (!data || !cls || !member || !member->member_class ||
        member->off < 0 || member->off > cls->sz)
        return 0;
    size = member->member_class->sz;
    if (size <= 0 || size > (int64_t)sizeof(value) ||
        member->off > cls->sz - size)
        return 0;
    memcpy((uint8_t *)data + member->off, &value, (size_t)size);
    return 1;
}
static int64_t host_mouse_state_update(int64_t *a) {
    CHashGlblVar *global;
    CHashGlblVar *hard_global;
    CMemberLst *pos, *pos_x, *pos_y, *pos_text, *text_x, *text_y, *timestamp;
    CMemberLst *installed;
    CHashClass *cls, *pos_cls, *text_cls;
    int64_t pos_x_off, pos_y_off, text_x_off, text_y_off;
    uint8_t *data;
    if (!Fs || !Fs->hash_table)
        return 0;
    global = (CHashGlblVar *)HashFind("mouse", Fs->hash_table,
                                      HTT_GLBL_VAR, 1);
    if (!global || !global->data_addr)
        return 0;
    pos = guest_class_member(global->var_class, "pos");
    pos_text = guest_class_member(global->var_class, "pos_text");
    timestamp = guest_class_member(global->var_class, "timestamp");
    pos_x = pos ? guest_class_member(pos->member_class, "x") : NULL;
    pos_y = pos ? guest_class_member(pos->member_class, "y") : NULL;
    text_x = pos_text ? guest_class_member(pos_text->member_class, "x") : NULL;
    text_y = pos_text ? guest_class_member(pos_text->member_class, "y") : NULL;
    if (!pos_x || !pos_y || !text_x || !text_y || !timestamp)
        return 0;
    cls = global->var_class;
    pos_cls = pos ? pos->member_class : NULL;
    text_cls = pos_text ? pos_text->member_class : NULL;
    if (!cls || !pos_cls || !text_cls || pos->off < 0 || pos_text->off < 0 ||
        timestamp->off < 0 || pos_cls->sz < (int64_t)sizeof(int64_t) * 2 ||
        text_cls->sz < (int64_t)sizeof(int64_t) * 2 ||
        pos->off > cls->sz - pos_cls->sz ||
        pos_text->off > cls->sz - text_cls->sz ||
        timestamp->off > cls->sz - (int64_t)sizeof(int64_t) ||
        pos_x->off < 0 || pos_x->off > pos_cls->sz - (int64_t)sizeof(int64_t) ||
        pos_y->off < 0 || pos_y->off > pos_cls->sz - (int64_t)sizeof(int64_t) ||
        text_x->off < 0 || text_x->off > text_cls->sz - (int64_t)sizeof(int64_t) ||
        text_y->off < 0 || text_y->off > text_cls->sz - (int64_t)sizeof(int64_t))
        return 0;
    pos_x_off = pos->off + pos_x->off;
    pos_y_off = pos->off + pos_y->off;
    text_x_off = pos_text->off + text_x->off;
    text_y_off = pos_text->off + text_y->off;
    data = global->data_addr;
    *(int64_t *)(data + pos_x_off) = a[0];
    *(int64_t *)(data + pos_y_off) = a[1];
    *(int64_t *)(data + text_x_off) = a[2];
    *(int64_t *)(data + text_y_off) = a[3];
    *(int64_t *)(data + timestamp->off) = a[4];
    hard_global = (CHashGlblVar *)HashFind("mouse_hard", Fs->hash_table,
                                           HTT_GLBL_VAR, 1);
    if (hard_global && hard_global->data_addr) {
        installed = guest_class_member(hard_global->var_class, "installed");
        guest_store_i64_member(hard_global->data_addr,
                               hard_global->var_class, installed, 1);
    }
    return 1;
}
static int64_t host_mouse_buttons_update(int64_t *a) {
    CHashGlblVar *global;
    CMemberLst *scan_code;
    int64_t value;
    if (!Fs || !Fs->hash_table)
        return 0;
    global = (CHashGlblVar *)HashFind("kbd", Fs->hash_table,
                                     HTT_GLBL_VAR, 1);
    if (!global || !global->data_addr || !global->var_class ||
        global->var_class->sz < (int64_t)sizeof(value))
        return 0;
    scan_code = guest_class_member(global->var_class, "scan_code");
    if (!scan_code || scan_code->off < 0 ||
        scan_code->off > global->var_class->sz - (int64_t)sizeof(value))
        return 0;
    memcpy(&value, (uint8_t *)global->data_addr + scan_code->off,
           sizeof(value));
    value = (a[0] ? (int64_t)((uint64_t)value | (1ull << 16))
                  : (int64_t)((uint64_t)value & ~(1ull << 16)));
    value = (a[1] ? (int64_t)((uint64_t)value | (1ull << 17))
                  : (int64_t)((uint64_t)value & ~(1ull << 17)));
    return guest_store_i64_member(global->data_addr, global->var_class,
                                  scan_code, value);
}
static int64_t host_fb_text_span(int64_t *a) {
    uint32_t lr = (uint32_t)a[6], tb = (uint32_t)a[7];
    uint32_t *plane = guest_text_plane(a[8], a[9], 75);
    if (!plane || a[9] != 100)
        return 0;
    return (int64_t)zeal_fb_text_span(a[0], a[1], a[2], (uint32_t)a[3],
                                      (const void *)(uintptr_t)a[4], (int)a[5],
                                      lr & 0xffffu, lr >> 16,
                                      tb & 0xffffu, tb >> 16, plane, a[9]);
}
static int64_t host_fb_text_cells_drawn(int64_t *a) {
    (void)a;
    return (int64_t)zeal_fb_text_cells_drawn();
}
static int64_t host_fb_text_pixel(int64_t *a) {
    return zeal_fb_text_pixel(a[0], a[1]);
}
static int64_t host_fb_text_char(int64_t *a) {
    uint32_t *plane = guest_text_plane(a[6], a[7], 75);
    if (!plane || a[7] != 100)
        return 0;
    return zeal_fb_text_char(a[0], a[1], (uint32_t)a[2], (int)a[3],
                             (uint32_t)a[4], (uint32_t)a[5], plane, a[7]);
}
static int64_t host_fb_text_fill(int64_t *a) {
    uint32_t *plane = guest_text_plane(a[6], a[7], 75);
    if (!plane || a[7] != 100)
        return -1;
    return zeal_fb_text_fill(a[0], a[1], a[2], (uint32_t)a[3],
                             (uint32_t)a[4], (uint32_t)a[5], plane, a[7]);
}
static int64_t host_fb_text_rect(int64_t *a) {
    uint32_t *plane = guest_text_plane(a[5], a[6], 75);
    if (!plane || a[6] != 100)
        return 0;
    return (int64_t)zeal_fb_text_rect(a[0], a[1], a[2], a[3],
                                      (uint32_t)a[4], plane, a[6]);
}
static int64_t host_fb_text_flush(int64_t *a) {
    uint32_t *plane = guest_text_plane(a[0], a[1], a[2]);
    if (!plane || a[1] != 100 || a[2] != 75)
        return 0;
    return (int64_t)zeal_fb_text_flush(plane, a[1], a[2]);
}
static int64_t host_fb_text_flush_rect(int64_t *a) {
    uint32_t *plane = guest_text_plane(a[0], a[1], 75);
    if (!plane || a[1] != 100)
        return 0;
    return (int64_t)zeal_fb_text_flush_rect(plane, a[1], a[2], a[3], a[4], a[5]);
}
static int64_t host_fb_graph_plot(int64_t *a) {
    return (int64_t)zeal_fb_graph_plot(a[0], a[1], (uint32_t)a[2],
                                       a[3], a[4], a[5], a[6],
                                       (uint16_t)a[7]);
}
static int64_t host_fb_graph_color(int64_t *a) {
    return (int64_t)zeal_fb_graph_color((uint32_t)a[0]);
}
static int64_t host_fb_set_win_z_buffer(int64_t *a) {
    zeal_fb_set_win_z_buffer((const uint16_t *)(uintptr_t)a[0]);
    return 0;
}
static int64_t host_fb_graph_rect(int64_t *a) {
    return (int64_t)zeal_fb_graph_rect(a[0], a[1], a[2], a[3],
                                       (uint32_t)a[4], a[5], a[6], a[7], a[8],
                                       (uint16_t)a[9]);
}
static int64_t host_fb_graph_text(int64_t *a) {
    return (int64_t)zeal_fb_graph_text(a[0], a[1], (uint32_t)a[2],
                                       a[3], a[4], a[5], a[6],
                                       (const char *)(uintptr_t)a[7],
                                       (uint16_t)a[8]);
}
static int64_t host_fb_graph_line(int64_t *a) {
    return (int64_t)zeal_fb_graph_line(a[0], a[1], a[2], a[3],
                                       (uint32_t)a[4], a[5], a[6],
                                       a[7], a[8], a[9], a[10],
                                       (uint16_t)a[11]);
}
static int64_t host_fb_screen_width(int64_t *a) {
    (void)a;
    return (int64_t)zeal_fb_screen_width();
}
static int64_t host_fb_screen_height(int64_t *a) {
    (void)a;
    return (int64_t)zeal_fb_screen_height();
}
static void set_guest_task(void *p) {
    __asm__ volatile("msr tpidr_el1, %0\n\tisb" : : "r"(p) : "memory");
}
static int64_t host_fs(int64_t *a) {
    void *p;
    (void)a;
    __asm__ volatile("mrs %0, tpidr_el1" : "=r"(p));
    if (!guest_task_bound || p != guest_fibers[guest_current].words ||
        guest_fibers[guest_current].state != GUEST_RUNNABLE)
        zc_fail("current guest task is unavailable");
    return (int64_t)(uintptr_t)p;
}
/* CCPU stand-in for KeyDev Ctrl-Alt hooks; only idle_task is consulted. */
static uint8_t guest_cpu[512];
static int64_t host_gs(int64_t *a) {
    (void)a;
    return (int64_t)(uintptr_t)(guest_cpu_structs ? guest_cpu_structs
                                                  : guest_cpu);
}
static void guest_unwind_enter(struct guest_unwind *scope,
                                void (*cleanup)(struct guest_unwind *),
                                jmp_buf *landing) {
    struct guest_fiber *f = &guest_fibers[guest_current];
    *scope = (struct guest_unwind){
        .previous = f->unwind, .top = f->except_top, .caught = f->except_caught,
        .cleanup = cleanup, .landing = landing,
        .previous_ch = *(int64_t *)((unsigned char *)f->words + guest_except_ch_off),
        .previous_catch = *(uint8_t *)((unsigned char *)f->words + guest_catch_except_off)
    };
    scope->irq_flags = (uint64_t)host_rflags_get(NULL);
    scope->irq_depth = f->irq_flags_depth;
    memcpy(scope->irq_stack, f->irq_flags_stack, sizeof(scope->irq_stack));
    f->unwind = scope;
}
static void guest_unwind_flags(struct guest_unwind *scope) {
    struct guest_fiber *f = &guest_fibers[guest_current];
    f->irq_flags_depth = scope->irq_depth;
    memcpy(f->irq_flags_stack, scope->irq_stack, sizeof(scope->irq_stack));
    int64_t flags = (int64_t)scope->irq_flags;
    f->irq_flags_live = scope->irq_flags;
    host_rflags_set(&flags);
}
static void guest_unwind_leave(struct guest_unwind *scope) {
    struct guest_fiber *f = &guest_fibers[guest_current];
    if (f->unwind != scope)
        zc_fail("native exception scope order mismatch");
    /* A scope being abandoned cannot leave pads whose saved stacks belong
     * to it. Well-formed normal returns already removed these pads. */
    while (f->except_top != scope->top) {
        struct guest_exception *pad = f->except_top;
        if (!pad) zc_fail("native exception scope lost its guest handler");
        f->except_top = pad->next;
        free(pad);
    }
    while (f->except_caught != scope->caught) {
        struct guest_exception *pad = f->except_caught;
        if (!pad) zc_fail("native exception scope lost its outer catch");
        f->except_caught = pad->caught_previous;
        free(pad);
    }
    f->unwind = scope->previous;
    if (scope->cleanup)
        scope->cleanup(scope);
}
static void guest_unwind_handled(struct guest_unwind *scope, int64_t code) {
    struct guest_fiber *f = &guest_fibers[guest_current];
    *(int64_t *)((unsigned char *)f->words + guest_except_ch_off) =
        scope->caught ? scope->previous_ch : code;
    *(uint8_t *)((unsigned char *)f->words + guest_catch_except_off) =
        scope->caught ? scope->previous_catch : 0;
}
static void guest_unwind_abandon(struct guest_unwind *scope) {
    guest_unwind_leave(scope);
    guest_unwind_flags(scope);
}
static void guest_unwind_all(void) {
    if (guest_task_bound)
        while (guest_fibers[guest_current].unwind)
            guest_unwind_abandon(guest_fibers[guest_current].unwind);
}
static int64_t host_sys_try(int64_t *a) {
    (void)a;
    if (!guest_task_bound || guest_fibers[guest_current].state != GUEST_RUNNABLE)
        zc_fail("SysTry outside a guest task");
    struct guest_fiber *f = &guest_fibers[guest_current];
    struct guest_exception *pad = __AIWNIOS_CAlloc(sizeof(*pad), &f->heap_owner);
    unsigned top_depth = f->except_top ? f->except_top->depth : 0;
    unsigned caught_depth = f->except_caught ? f->except_caught->depth : 0;
    pad->depth = (top_depth > caught_depth ? top_depth : caught_depth) + 1;
    pad->previous_catch = *(uint8_t *)((unsigned char *)f->words + guest_catch_except_off);
    pad->previous_ch = *(int64_t *)((unsigned char *)f->words + guest_except_ch_off);
    pad->next = f->except_top;
    pad->unwind = f->unwind;
    pad->irq_flags = (uint64_t)host_rflags_get(NULL);
    pad->irq_depth = f->irq_flags_depth;
    memcpy(pad->irq_stack, f->irq_flags_stack, sizeof(pad->irq_stack));
    f->except_top = pad;
    return (int64_t)(uintptr_t)pad->context;
}
static int64_t host_sys_untry(int64_t *a) {
    (void)a;
    struct guest_fiber *f = &guest_fibers[guest_current];
    struct guest_exception *pad = f->except_top;
    if (!guest_task_bound || !pad)
        zc_fail("SysUntry without SysTry");
    f->except_top = pad->next;
    __AIWNIOS_Free(pad);
    return 0;
}
static void guest_raise(int64_t code) {
    struct guest_fiber *f = &guest_fibers[guest_current];
    struct guest_exception *pad = guest_task_bound ? f->except_top : NULL;
    struct guest_unwind *target = pad ? pad->unwind : NULL;
    while (guest_task_bound && f->unwind != target) {
        struct guest_unwind *scope = f->unwind;
        if (!scope)
            zc_fail("guest catch has an abandoned native scope");
        if (scope->landing) {
            if (compiler_recovery_active) {
                /* A preprocessor expression escaped its guest handler.
                 * Restore the parser checkpoint before freeing its objects. */
                compiler_recovery_exception = code;
                compiler_recovery_active = 0;
                longjmp(compiler_recovery_guard, 1);
            }
            scope->exception = code;
            longjmp(*scope->landing, 1);
        }
        guest_unwind_abandon(scope);
    }
    if (!pad)
        zc_fail("uncaught guest exception");
    /* A catch already unwound past the next handler cannot remain active.
     * Keep an outer catch when a nested try inside it throws. */
    while (f->except_caught && f->except_caught->depth >= pad->depth) {
        struct guest_exception *old = f->except_caught;
        f->except_caught = old->caught_previous;
        __AIWNIOS_Free(old);
    }
    f->except_top = pad->next;
    pad->caught_previous = f->except_caught;
    f->except_caught = pad;
    *(int64_t *)((unsigned char *)f->words + guest_except_ch_off) = code;
    *(uint8_t *)((unsigned char *)f->words + guest_catch_except_off) = 0;
    memcpy(f->throw_context, pad->context, sizeof(f->throw_context));
    f->irq_flags_depth = pad->irq_depth;
    memcpy(f->irq_flags_stack, pad->irq_stack, sizeof(pad->irq_stack));
    int64_t flags = (int64_t)pad->irq_flags;
    f->irq_flags_live = pad->irq_flags;
    host_rflags_set(&flags);
    longjmp(f->throw_context, 1);
}
static int64_t host_guest_throw(int64_t *a) {
    guest_raise(a[0]);
}
static int64_t host_end_catch(int64_t *a) {
    (void)a;
    struct guest_fiber *f = &guest_fibers[guest_current];
    struct guest_exception *pad = guest_task_bound ? f->except_caught : NULL;
    if (!pad)
        zc_fail("EndCatch without a catch");
    uint8_t *caught = (uint8_t *)f->words + guest_catch_except_off;
    if (!*caught) {
        int64_t code = *(int64_t *)((unsigned char *)f->words + guest_except_ch_off);
        int64_t args[2] = {code, 0};
        host_guest_throw(args);
    }
    *caught = pad->caught_previous ? pad->previous_catch : 0;
    if (pad->caught_previous)
        *(int64_t *)((unsigned char *)f->words + guest_except_ch_off) = pad->previous_ch;
    f->except_caught = pad->caught_previous;
    __AIWNIOS_Free(pad);
    return 0;
}
#define GUEST_SUSPENDED (1u << 2) /* TASKf_SUSPENDED in pinned KernelA.HH */
#define GUEST_KILL_TASK (1u << 1) /* TASKf_KILL_TASK */
#define GUEST_IDLE (1u << 3) /* TASKf_IDLE in pinned KernelA.HH */
#define GUEST_CMD_LINE_PROMPT (1u << 4) /* TASKf_CMD_LINE_PROMPT */
#define GUEST_FILTER_INPUT (1u << 6)
#define GUEST_AWAITING_MESSAGE (1u << 9)
#define GUEST_WIF_SELF_KEY_DESC (1u << 12) /* WIf_SELF_KEY_DESC in KernelA.HH */
#define GUEST_SCF_KEY_DESC (1ull << 31) /* SCf_KEY_DESC in KernelA.HH */
#define GUEST_JOBF_WAKE_MASTER (1u << 0)
#define GUEST_JOBF_FOCUS_MASTER (1u << 1)
#define GUEST_JOBF_EXIT_ON_COMPLETE (1u << 2)
#define GUEST_JOBF_DONT_FILTER (1u << 3)
#define GUEST_JOBF_HIGHEST_PRIORITY (1u << 4)
#define GUEST_JOBF_DONE (1u << 5)
#define GUEST_JOBF_DISPATCHED (1u << 6)
#define GUEST_JOBF_FREE_ON_COMPLETE (1u << 7)
#define GUEST_JOBF_ADD_TO_QUE (1u << 8)
/* Internal requests may bypass root UI messages; normal TaskExe keeps FIFO. */
#define GUEST_JOBF_SYSTEM_REQUEST (UINT64_C(1) << 62)
#define GUEST_JOBT_TEXT_INPUT 0
#define GUEST_JOBT_MESSAGE 1
#define GUEST_JOBT_EXE_STR 2
#define GUEST_JOBT_CALL 3
#define GUEST_JOBT_SPAWN_TASK 4
static uint32_t *guest_flags(struct guest_fiber *f) {
    return (uint32_t *)((unsigned char *)f->words + guest_flags_off);
}
static int dispatch_blocks_current(void) {
    return guest_task_exe_active ||
           (guest_task_bound && guest_fibers[guest_current].source_job_active) ||
           (guest_job_call_active && guest_current == GUEST_EXECUTIVE);
}
static int valid_guest_task(void *task);
static struct guest_fiber *find_guest_task(void *task);
static struct guest_fiber *guest_popup_parent(struct guest_fiber *child) {
    void *parent_task = *(void **)((unsigned char *)child->words + guest_parent_off);
    struct guest_fiber *parent = find_guest_task(parent_task);
    if (!parent || !valid_guest_task(parent_task) ||
        *(void **)((unsigned char *)parent->words + guest_popup_off) != child->words)
        return NULL;
    return parent;
}
static struct guest_fiber *guest_popup_child(struct guest_fiber *parent) {
    void *child_task = *(void **)((unsigned char *)parent->words + guest_popup_off);
    struct guest_fiber *child = find_guest_task(child_task);
    return child && valid_guest_task(child_task) &&
           *(void **)((unsigned char *)child->words + guest_parent_off) == parent->words
        ? child : NULL;
}
static int64_t host_task_yield(int64_t *a);
static int64_t host_time_cal(int64_t *a);
static int64_t host_rflags_get(int64_t *a);
static int64_t host_rflags_set(int64_t *a);
static int64_t host_jobs_handler(int64_t *a);
static int64_t host_jobs_handler_mode(int64_t *a, int system_only);
static uint64_t counter_now(void);
static uint64_t deadline_after_milliseconds(uint64_t milliseconds);
static int64_t poll_guest_keys(void);
static void guest_executive_loop(void) __attribute__((noreturn));
static void guest_input_filter_loop(void);
static struct guest_job *guest_job_head(struct guest_fiber *f) {
    return (void *)((unsigned char *)f->words + guest_server_ctrl_off);
}
static struct guest_job *guest_done_head(struct guest_fiber *f) {
    return (void *)((unsigned char *)f->words + guest_server_ctrl_off + 16);
}
static void guest_job_insert(struct guest_job *head, struct guest_job *job) {
    job->next = head;
    job->last = head->last;
    head->last->next = job;
    head->last = job;
}
static void guest_job_unlink(struct guest_job *job) {
    job->last->next = job->next;
    job->next->last = job->last;
    job->next = job->last = NULL;
}
/* KernelB's queue intrinsics use the same first two pointer fields for
 * CQueue, CJob, and the embedded CJobCtrl sentinels. */
struct guest_queue { struct guest_queue *next, *last; };
static int64_t host_queue_init(int64_t *a) {
    struct guest_queue *head = (void *)(uintptr_t)a[0];
    if (!head)
        zc_fail("QueueInit requires a head");
    head->next = head->last = head;
    return 0;
}
static int64_t host_queue_insert(int64_t *a) {
    struct guest_queue *entry = (void *)(uintptr_t)a[0];
    struct guest_queue *pred = (void *)(uintptr_t)a[1];
    if (!entry || !pred || !pred->next || pred->next->last != pred)
        zc_fail("QueueInsert requires a linked predecessor");
    entry->next = pred->next;
    entry->last = pred;
    pred->next->last = entry;
    pred->next = entry;
    return 0;
}
static int64_t host_queue_insert_rev(int64_t *a) {
    struct guest_queue *entry = (void *)(uintptr_t)a[0];
    struct guest_queue *succ = (void *)(uintptr_t)a[1];
    if (!entry || !succ || !succ->last || succ->last->next != succ)
        zc_fail("QueueInsertRev requires a linked successor");
    entry->last = succ->last;
    entry->next = succ;
    succ->last->next = entry;
    succ->last = entry;
    return 0;
}
static int64_t host_queue_remove(int64_t *a) {
    struct guest_queue *entry = (void *)(uintptr_t)a[0];
    if (!entry || !entry->next || !entry->last || entry->next == entry ||
        entry->last->next != entry || entry->next->last != entry)
        zc_fail("QueueRemove requires a linked entry");
    entry->last->next = entry->next;
    entry->next->last = entry->last;
    entry->next = entry->last = NULL;
    return 0;
}
static void guest_job_track(struct guest_job *job) {
    for (unsigned i = 0; i < GUEST_JOBS_MAX; i++) {
        if (!guest_tracked_jobs[i]) {
            guest_tracked_jobs[i] = job;
            guest_job_count++;
            return;
        }
    }
    zc_fail("guest tracked job table full");
}
static int guest_job_untrack(struct guest_job *job) {
    for (unsigned i = 0; i < GUEST_JOBS_MAX; i++) {
        if (guest_tracked_jobs[i] == job) {
            guest_tracked_jobs[i] = NULL;
            guest_job_count--;
            return 1;
        }
    }
    return 0;
}
static void guest_job_delete(struct guest_job *job) {
    (void)guest_job_untrack(job);
    if (job->aux_str)
        __AIWNIOS_Free(job->aux_str);
    __AIWNIOS_Free(job);
}
static int64_t host_job_del(int64_t *a) {
    struct guest_job *job = (void *)(uintptr_t)a[0];
    if (!job)
        zc_fail("JobDel requires a job");
    if (job->next || job->last)
        zc_fail("JobDel requires a detached job");
    if (MSize(job) != sizeof(*job))
        zc_fail("JobDel requires a guest job allocation");
    guest_job_delete(job);
    return 0;
}
static void guest_jobs_clear(struct guest_fiber *f) {
    for (unsigned done = 0; done < 2; done++) {
        struct guest_job *head = done ? guest_done_head(f) : guest_job_head(f);
        while (head->next != head) {
            struct guest_job *job = head->next;
            guest_job_unlink(job);
            guest_job_delete(job);
        }
    }
}
static struct guest_job *guest_job_find(void *request) {
    /* JobsHandler unlinks a job before invoking it. A task that yields while
     * executing its source or callback leaves that request in flight. */
    for (unsigned i = 0; i < GUEST_TASKS; i++) {
        if (guest_fibers[i].state != GUEST_RUNNABLE)
            continue;
        for (struct guest_active_job *active = guest_fibers[i].active_jobs;
             request && active; active = active->previous)
            if (active->job == request)
                return request;
        for (unsigned done = 0; done < 2; done++) {
            struct guest_job *head = done ? guest_done_head(&guest_fibers[i])
                                          : guest_job_head(&guest_fibers[i]);
            for (struct guest_job *job = head->next; job != head; job = job->next)
                if (job == request)
                    return job;
        }
    }
    return NULL;
}
static int wait_ready(struct guest_fiber *f) {
    if (f->wait_kind == GUEST_WAIT_NONE)
        return 1;
    if (f->wait_kind == GUEST_WAIT_TIMER)
        return (int64_t)(counter_now() - f->wait_deadline) >= 0;
    if (f->wait_kind == GUEST_WAIT_MESSAGE_TIMEOUT) {
        void *target = *(void **)f->wait_slot;
        struct guest_fiber *server = find_guest_task(target);
        if (!valid_guest_task(target))
            return 1;
        for (unsigned hops = 0; server && hops < GUEST_TASKS; hops++) {
            struct guest_job *head = guest_job_head(server);
            if (head->next != head)
                return 1;
            server = guest_popup_parent(server);
        }
        return (int64_t)(counter_now() - f->wait_deadline) >= 0;
    }
    if (f->wait_kind == GUEST_WAIT_JOB) {
        if (!f->wait_target) {
            struct guest_job *head = guest_done_head(f);
            return head->next != head;
        }
        struct guest_job *job = guest_job_find(f->wait_target);
        return !job || !!(job->flags & GUEST_JOBF_DONE);
    }
    void *target = *(void **)f->wait_slot;
    int valid = valid_guest_task(target);
    if (f->wait_kind == GUEST_WAIT_DEATH)
        return !valid;
    if (f->wait_kind == GUEST_WAIT_MESSAGE) {
        struct guest_fiber *server = find_guest_task(target);
        if (!valid)
            return 1;
        for (unsigned hops = 0; server && hops < GUEST_TASKS; hops++) {
            struct guest_job *head = guest_job_head(server);
            if (head->next != head)
                return 1;
            server = guest_popup_parent(server);
        }
        return 0;
    }
    if (f->wait_kind == GUEST_WAIT_SERVER) {
        struct guest_fiber *server = find_guest_task(target);
        return !valid || guest_job_head(server)->next != guest_job_head(server);
    }
    if (f->wait_kind == GUEST_WAIT_IDLE ||
        f->wait_kind == GUEST_WAIT_IDLE_TIMEOUT) {
        if (f->wait_kind == GUEST_WAIT_IDLE_TIMEOUT &&
            (int64_t)(counter_now() - f->wait_deadline) >= 0)
            return 1;
        if (!valid)
            return 1;
        void *filter = *(void **)((unsigned char *)target + guest_input_filter_off);
        struct guest_fiber *input = find_guest_task(filter);
        if (filter == f->words || !input || !valid_guest_task(filter))
            return 1;
        unsigned char *ctrl = (unsigned char *)filter + guest_server_ctrl_off;
        return *(void **)ctrl == ctrl && (*guest_flags(input) & GUEST_IDLE) &&
               (!f->wait_number || (*guest_flags(input) & GUEST_CMD_LINE_PROMPT));
    }
    return valid && *(int64_t *)((unsigned char *)target + guest_number_off) != f->wait_number;
}
static int guest_timer_pending(void) {
    for (unsigned i = 0; i < GUEST_TASKS; i++)
        if (guest_fibers[i].state == GUEST_RUNNABLE &&
            (guest_fibers[i].wait_kind == GUEST_WAIT_TIMER ||
             guest_fibers[i].wait_kind == GUEST_WAIT_IDLE_TIMEOUT ||
             guest_fibers[i].wait_kind == GUEST_WAIT_MESSAGE_TIMEOUT) &&
            !(*guest_flags(&guest_fibers[i]) & GUEST_SUSPENDED))
            return 1;
    return 0;
}
static unsigned next_guest(unsigned from) {
    for (unsigned step = 1; step <= GUEST_TASKS; step++) {
        unsigned i = (from + step) % GUEST_TASKS;
        if (guest_fibers[i].state == GUEST_RUNNABLE &&
            (guest_fibers[i].kill_requested ||
             (!(*guest_flags(&guest_fibers[i]) & GUEST_SUSPENDED) &&
              wait_ready(&guest_fibers[i]))))
            return i;
    }
    return 0; /* The root remains runnable while the shell is executing. */
}
static void enter_guest(unsigned next) __attribute__((noreturn));
static void enter_guest(unsigned next) {
    if (next != guest_current) {
        struct guest_fiber *cur = &guest_fibers[guest_current];
        struct guest_fiber *nxt = &guest_fibers[next];
        cur->irq_flags_live = (uint64_t)host_rflags_get(NULL);
        int64_t restore = (int64_t)nxt->irq_flags_live;
        host_rflags_set(&restore);
    }
    guest_current = next;
    Fs = next ? &guest_fibers[next].compiler_task : &task;
    set_guest_task(guest_fibers[next].words);
    longjmp(guest_fibers[next].context, 1);
}
static CHeapCtrl *code_owner(void *p) {
    uintptr_t address = (uintptr_t)p;
    for (struct block *b = arenas[1]; b; b = b->next) {
        uintptr_t start = (uintptr_t)(b + 1);
        if (b->used && address >= start && address - start < b->requested)
            return b->owner;
    }
    return NULL;
}
static int code_lives_with_parent(void *entry, void *parent) {
    CHeapCtrl *owner = code_owner(entry);
    if (owner == &code_heap)
        return 1;
    for (unsigned hop = 0; hop < GUEST_TASKS && parent; hop++) {
        struct guest_fiber *f = find_guest_task(parent);
        if (!f)
            break;
        if (owner == &f->compiler_code_heap)
            return 1;
        parent = *(void **)((unsigned char *)f->words + guest_parent_off);
    }
    return 0;
}
static void *guest_child_head(void *parent) {
    return (unsigned char *)parent + guest_next_child_off - guest_next_sibling_off;
}
static int valid_child_link(void *link, void *head, void *parent) {
    if (link == head)
        return 1;
    struct guest_fiber *child = find_guest_task(link);
    return child && *(void **)((unsigned char *)child->words + guest_parent_off) == parent;
}
static void guest_trampoline(void) __attribute__((noreturn));
static void init_guest_fiber(unsigned i, void *parent, const char *name) {
    struct guest_fiber *f = &guest_fibers[i];
    unsigned char *words = (unsigned char *)f->words;
    unsigned char *heap = (unsigned char *)f->heap_words;
    f->state = GUEST_RUNNABLE;
    *(void **)words = words;
    memcpy(words + 8, "TskS", 4);
    *(void **)(words + guest_next_task_off) = words;
    *(void **)(words + guest_last_task_off) = words;
    *(void **)(words + guest_next_sibling_off) = words;
    *(void **)(words + guest_last_sibling_off) = words;
    /* KTask.ZC uses a synthetic CTask whose sibling links overlap these
     * two child-head fields. Preserve that exact sentinel representation. */
    void *child_head = guest_child_head(words);
    *(void **)(words + guest_next_child_off) = child_head;
    *(void **)(words + guest_last_child_off) = child_head;
    *(void **)(words + guest_next_ctrl_off) = words + guest_next_ctrl_off;
    *(void **)(words + guest_last_ctrl_off) = words + guest_next_ctrl_off;
    *(void **)(words + guest_next_cc_off) = words + guest_next_cc_off;
    *(void **)(words + guest_last_cc_off) = words + guest_next_cc_off;
    *(void **)(words + guest_parent_off) = parent;
    const char *dir = parent
        ? *(char **)((unsigned char *)parent + guest_cur_dir_off) : NULL;
    *(char **)(words + guest_cur_dir_off) =
        __AIWNIOS_StrDup((char *)(dir ? dir : "/"), &f->heap_owner);
    if (parent && guest_gs_off >= 0)
        *(void **)(words + guest_gs_off) =
            *(void **)((unsigned char *)parent + guest_gs_off);
    *(void **)(words + guest_input_filter_off) = words;
    *(void **)(words + guest_next_filter_off) = words;
    *(void **)(words + guest_next_ode_off) = words + guest_next_ode_off;
    *(void **)(words + guest_last_ode_off) = words + guest_next_ode_off;
    *(uint32_t *)(words + guest_display_flags_off) = parent
        ? *(uint32_t *)((unsigned char *)parent + guest_display_flags_off)
        : (1u << 1); /* DISPLAYf_NOT_RAW */
    uint64_t cols = zeal_fb_text_cols(), rows = zeal_fb_text_rows();
    if (!cols || cols > 512) cols = 100;
    if (!rows || rows > 512) rows = 75;
    *(int64_t *)(words + guest_win_left_off) = 0;
    *(int64_t *)(words + guest_win_right_off) = (int64_t)cols - 1;
    *(int64_t *)(words + guest_win_top_off) = 0;
    *(int64_t *)(words + guest_win_bottom_off) = (int64_t)rows - 1;
    *(uint8_t *)(words + guest_text_attr_off) = 0x0f;
    zeal_fb_task_text_reset(i);
    unsigned char *ctrl = words + guest_server_ctrl_off;
    *(void **)(ctrl + 0) = ctrl;
    *(void **)(ctrl + 8) = ctrl;
    *(void **)(ctrl + 16) = ctrl + 16;
    *(void **)(ctrl + 24) = ctrl + 16;
    *(int64_t *)(words + guest_number_off) = i ? (int64_t)++guest_task_serial : 0;
    *(void **)(words + guest_data_heap_off) = heap;
    *(void **)(words + guest_code_heap_off) = heap;
    /* Guest DEFINE/FramePtr tables are separate from the Aiwnios compiler
     * hash. Inherit the parent's definitions just as compiler names do;
     * DolDoc callbacks need the root's ST_DOC_CMDS and related lists.
     * HashTableNew's CHash/CHashTable layout matches ZealOS. */
    CHashTable *guest_hash = HashTableNew(1024, &f->heap_owner);
    if (parent)
        guest_hash->next = *(CHashTable **)((unsigned char *)parent +
                                           guest_hash_table_off);
    *(void **)(words + guest_hash_table_off) = guest_hash;
    memcpy(heap + guest_heap_sig_off, "HcSV", 4);
    *(void **)(heap + guest_heap_task_off) = words;
    f->irq_flags_live = 1u << 9; /* IF enabled until CLI/RFlagsSet. */
    if (i) {
        struct guest_fiber *parent_fiber = parent ? find_guest_task(parent) : NULL;
        CTask *parent_compiler = parent_fiber && parent_fiber != &guest_fibers[0]
            ? &parent_fiber->compiler_task : &task;
        f->compiler_task.heap = HeapCtrlInit(&f->compiler_heap, &f->compiler_task, 0);
        f->compiler_task.code_heap = HeapCtrlInit(&f->compiler_code_heap,
                                                  &f->compiler_task, 1);
        f->compiler_task.hash_table = HashTableNew(1024, &f->compiler_heap);
        f->compiler_task.hash_table->next = parent_compiler->hash_table;
    }
    if (name) {
        unsigned char *dst = words + guest_name_off;
        for (unsigned j = 0; j < 31 && name[j]; j++)
            dst[j] = (unsigned char)name[j];
    }
}
static void init_guest_context(struct guest_fiber *f) {
    f->context[11] = (uintptr_t)guest_trampoline; /* saved x30 */
    unsigned char *stack = f->large_stack ? f->large_stack : f->stack;
    size_t stack_size = f->large_stack ? f->stack_size : sizeof(f->stack);
    f->context[12] = (uintptr_t)(stack + stack_size); /* saved SP */
    __asm__ volatile("mrs %0, fpcr" : "=r"(f->context[22]));
    __asm__ volatile("mrs %0, fpsr" : "=r"(f->context[23]));
}
static struct guest_fiber *new_guest_fiber(void *entry, void *data, void *parent,
                                           const char *name, enum guest_entry kind,
                                           size_t stack_size) {
    if (!guest_task_bound ||
        !find_guest_task(parent) ||
        (kind != GUEST_ENTRY_INPUT_FILTER && !code_lives_with_parent(entry, parent)))
        return NULL;
    /* The saved AArch64 SP must remain 16-byte aligned even when a caller
     * requests a byte count that is not itself aligned. */
    if (stack_size > SIZE_MAX - 15)
        return NULL;
    stack_size = (stack_size + 15) & ~(size_t)15;
    unsigned first = kind == GUEST_ENTRY_SYSTEM ? GUEST_SYSTEM : 1;
    unsigned end = kind == GUEST_ENTRY_SYSTEM ? GUEST_SYSTEM + 1 : GUEST_APP_TASKS;
    for (unsigned i = first; i < end; i++) {
        struct guest_fiber *f = &guest_fibers[i];
        if (f->state == GUEST_RUNNABLE)
            continue;
        /* A finished fiber cannot release its own stack while executing on
         * it. Slot reuse runs on another fiber, so this is a safe point. */
        if (f->large_stack)
            __AIWNIOS_Free(f->large_stack);
        memset(f, 0, sizeof(*f));
        if (stack_size > GUEST_STACK_BYTES) {
            f->large_stack = __AIWNIOS_MAlloc((int64_t)stack_size,
                                               &guest_stack_heap);
            f->stack_size = stack_size;
        }
        f->entry = entry;
        f->data = data;
        f->entry_kind = kind;
        init_guest_fiber(i, parent, name);
        void *root = guest_fibers[0].words;
        void *tail = *(void **)((unsigned char *)root + guest_last_task_off);
        *(void **)((unsigned char *)f->words + guest_next_task_off) = root;
        *(void **)((unsigned char *)f->words + guest_last_task_off) = tail;
        *(void **)((unsigned char *)tail + guest_next_task_off) = f->words;
        *(void **)((unsigned char *)root + guest_last_task_off) = f->words;
        /* Match Sched.ZC TaskQueueInsChild: insert before the oldest child,
         * with the parent's embedded synthetic sibling as the ring head. */
        void *head = guest_child_head(parent);
        void *pred = *(void **)((unsigned char *)parent + guest_last_child_off);
        if (!valid_child_link(pred, head, parent))
            zc_fail("guest child ring corrupted");
        void *last = *(void **)((unsigned char *)pred + guest_last_sibling_off);
        if (!valid_child_link(last, head, parent) ||
            *(void **)((unsigned char *)last + guest_next_sibling_off) != pred)
            zc_fail("guest child ring corrupted");
        *(void **)((unsigned char *)last + guest_next_sibling_off) = f->words;
        *(void **)((unsigned char *)pred + guest_last_sibling_off) = f->words;
        *(void **)((unsigned char *)f->words + guest_last_sibling_off) = last;
        *(void **)((unsigned char *)f->words + guest_next_sibling_off) = pred;
        init_guest_context(f);
        return f;
    }
    return NULL;
}
static struct guest_fiber *ensure_guest_executive(void) {
    struct guest_fiber *f = &guest_fibers[GUEST_EXECUTIVE];
    if (f->state == GUEST_RUNNABLE)
        return f;
    memset(f, 0, sizeof(*f));
    f->entry_kind = GUEST_ENTRY_EXECUTIVE;
    init_guest_fiber(GUEST_EXECUTIVE, guest_fibers[0].words, "Executive");
    init_guest_context(f);
    return f;
}
static int64_t host_system_task_start(int64_t *a) {
    if (!guest_task_bound || guest_current || guest_task_exe_active ||
        guest_task_frame_active || code_owner((void *)(uintptr_t)a[0]) != &code_heap)
        return 0;
    struct guest_fiber *f = &guest_fibers[GUEST_SYSTEM];
    if (f->state == GUEST_RUNNABLE)
        return 42;
    CHashGlblVar *system = (CHashGlblVar *)HashFind("sys_task", task.hash_table,
                                                   HTT_GLBL_VAR, 1);
    if (!system || !system->data_addr)
        return 0;
    f = new_guest_fiber((void *)(uintptr_t)a[0], NULL, guest_fibers[0].words,
                       "System", GUEST_ENTRY_SYSTEM, GUEST_STACK_BYTES);
    if (!f)
        return 0;
    /* The bootstrap root already owns the kernel/global namespace inherited
     * by running tasks. System shares that persistent namespace and compiler
     * arenas, rather than creating a child-local table invisible to siblings.
     * Its guest heap, compiler context stack, queues and execution stack are
     * separate. The system task cannot exit or be killed during this session. */
    free(f->compiler_task.hash_table->body);
    free(f->compiler_task.hash_table);
    f->compiler_task.hash_table = task.hash_table;
    f->compiler_task.heap = &data_heap;
    f->compiler_task.code_heap = &code_heap;
    CHashTable **mirror = (CHashTable **)((unsigned char *)f->words + guest_hash_table_off);
    free((*mirror)->body);
    free(*mirror);
    *mirror = *(CHashTable **)((unsigned char *)guest_fibers[0].words + guest_hash_table_off);
    *(uint32_t *)((unsigned char *)f->words + guest_win_inhibit_off) = UINT32_MAX;
    *(uint32_t *)((unsigned char *)f->words + guest_display_flags_off) &= ~(1u << 0);
    *(void **)system->data_addr = f->words;
    return 42;
}
static void reclaim_guest_heap(CHeapCtrl *owner) {
    for (unsigned a = 0; a < 2; a++)
        for (struct block *b = arenas[a]; b;)
            b = (b->used && b->owner == owner) ? release(b)->next : b->next;
}
static int guest_has_children(struct guest_fiber *parent) {
    for (unsigned i = 1; i < GUEST_TASKS; i++)
        if (guest_fibers[i].state == GUEST_RUNNABLE &&
            *(void **)((unsigned char *)guest_fibers[i].words + guest_parent_off) == parent->words)
            return 1;
    return 0;
}
static struct guest_fiber *guest_job_waiter(struct guest_job *job) {
    struct guest_fiber *master = find_guest_task(job->master_task);
    if (master)
        return master;
    for (unsigned i = 0; i < GUEST_TASKS; i++)
        if (guest_fibers[i].state == GUEST_RUNNABLE &&
            guest_fibers[i].wait_kind == GUEST_WAIT_JOB &&
            guest_fibers[i].wait_target == job)
            return &guest_fibers[i];
    return NULL;
}
static void cancel_active_job(struct guest_job *job) {
    struct guest_fiber *master = find_guest_task(job->master_task);
    if (master && (job->flags & GUEST_JOBF_FOCUS_MASTER) &&
        !(*(uint32_t *)((unsigned char *)master->words + guest_win_inhibit_off) & 1u))
        guest_focus_task = master->words;
    struct guest_fiber *recipient = guest_job_waiter(job);
    if (job->job_code != GUEST_JOBT_EXE_STR ||
        (job->flags & GUEST_JOBF_FREE_ON_COMPLETE) || !recipient) {
        guest_job_delete(job);
        return;
    }
    /* An exiting server cannot finish its source expression. Publish a
     * canceled zero result so an existing JobResGet waiter can progress. */
    job->res = 0;
    job->flags |= GUEST_JOBF_DONE;
    job->ctrl = (unsigned char *)recipient->words + guest_server_ctrl_off;
    guest_job_insert(guest_done_head(recipient), job);
}
static void user_compilers_clear(struct guest_fiber *f);
static void finish_guest(unsigned i, int64_t result) {
    struct guest_fiber *f = &guest_fibers[i];
    if (f->state != GUEST_RUNNABLE)
        return;
    if (guest_has_children(f))
        zc_fail("task ended before its children");
    struct guest_fiber *popup_parent = guest_popup_parent(f);
    if (popup_parent)
        *(void **)((unsigned char *)popup_parent->words + guest_popup_off) = NULL;
    if (i != GUEST_EXECUTIVE) {
        void *parent = *(void **)((unsigned char *)f->words + guest_parent_off);
        if (!valid_guest_task(parent))
            zc_fail("guest task parent vanished");
        void *head = guest_child_head(parent);
        void *sibling_prev = *(void **)((unsigned char *)f->words + guest_last_sibling_off);
        void *sibling_next = *(void **)((unsigned char *)f->words + guest_next_sibling_off);
        if (!valid_child_link(sibling_prev, head, parent) ||
            !valid_child_link(sibling_next, head, parent) ||
            *(void **)((unsigned char *)sibling_prev + guest_next_sibling_off) != f->words ||
            *(void **)((unsigned char *)sibling_next + guest_last_sibling_off) != f->words)
            zc_fail("guest child ring corrupted");
        *(void **)((unsigned char *)sibling_prev + guest_next_sibling_off) = sibling_next;
        *(void **)((unsigned char *)sibling_next + guest_last_sibling_off) = sibling_prev;
        *(void **)((unsigned char *)f->words + guest_next_sibling_off) = f->words;
        *(void **)((unsigned char *)f->words + guest_last_sibling_off) = f->words;
        void *previous = *(void **)((unsigned char *)f->words + guest_last_task_off);
        void *next = *(void **)((unsigned char *)f->words + guest_next_task_off);
        if (!valid_guest_task(previous) || !valid_guest_task(next) ||
            *(void **)((unsigned char *)previous + guest_next_task_off) != f->words ||
            *(void **)((unsigned char *)next + guest_last_task_off) != f->words)
            zc_fail("guest task ring corrupted");
        *(void **)((unsigned char *)previous + guest_next_task_off) = next;
        *(void **)((unsigned char *)next + guest_last_task_off) = previous;
        *(void **)((unsigned char *)f->words + guest_next_task_off) = f->words;
        *(void **)((unsigned char *)f->words + guest_last_task_off) = f->words;
    }
    f->state = GUEST_DONE;
    if (guest_focus_task == f->words)
        guest_focus_task = NULL;
    if (f->entry_kind == GUEST_ENTRY_INPUT_FILTER) {
        void *parent_task = *(void **)((unsigned char *)f->words + guest_parent_off);
        struct guest_fiber *parent = find_guest_task(parent_task);
        if (parent) {
            /* The bootstrap supports one filter per server. Preserve keys
             * that arrived behind its text jobs when the filter exits. */
            struct guest_job *head = guest_job_head(f);
            for (struct guest_job *job = head->next, *next; job != head; job = next) {
                next = job->next;
                if (job->job_code == GUEST_JOBT_MESSAGE) {
                    guest_job_unlink(job);
                    job->ctrl = (unsigned char *)parent->words + guest_server_ctrl_off;
                    guest_job_insert(guest_job_head(parent), job);
                    *guest_flags(parent) &= ~(GUEST_IDLE | GUEST_AWAITING_MESSAGE);
                }
            }
            *(void **)((unsigned char *)parent->words + guest_input_filter_off) = parent->words;
            *(void **)((unsigned char *)parent->words + guest_next_filter_off) = parent->words;
            *guest_flags(parent) &= ~GUEST_FILTER_INPUT;
        }
    }
    f->result = result;
    for (struct guest_active_job *active = f->active_jobs; active;
         active = active->previous)
        cancel_active_job(active->job);
    f->active_jobs = NULL;
    f->except_top = f->except_caught = NULL;
    f->unwind = NULL;
    guest_jobs_clear(f);
    user_compilers_clear(f);
    reclaim_guest_heap(&f->heap_owner);
    if (i) {
        reclaim_guest_heap(&f->compiler_code_heap);
        reclaim_guest_heap(&f->compiler_heap);
        f->compiler_task.hash_table = NULL;
    }
    memset((unsigned char *)f->words + 8, 0, 4); /* no longer a valid CTask */
    guest_completed++;
}
static void end_guest_current(int64_t result) __attribute__((noreturn));
static int64_t host_task_yield(int64_t *a);
static void end_guest_current(int64_t result) {
    unsigned me = guest_current;
    struct guest_fiber *f = &guest_fibers[me];
    if (!me || me == GUEST_EXECUTIVE || me == GUEST_SYSTEM || f->state != GUEST_RUNNABLE)
        zc_fail("cannot end root, executive or system task");
    if (f->ending) {
        guest_unwind_all();
        longjmp(f->end_context, 1); /* Exit inside a callback resumes teardown. */
    }
    f->ending = 1;
    f->kill_requested = 0;
    f->wait_kind = GUEST_WAIT_NONE;
    *guest_flags(f) &= ~(GUEST_KILL_TASK | GUEST_SUSPENDED | GUEST_AWAITING_MESSAGE);
    guest_unwind_all();
    (void)setjmp(f->end_context);
    for (;;) {
        void **slot = (void **)((unsigned char *)f->words + guest_end_cb_off);
        void *callback = *slot;
        if (!callback)
            break;
        if (!code_lives_with_parent(callback, f->words))
            zc_fail("task end callback has no live code owner");
        *slot = NULL; /* An Exit inside the callback cannot invoke it twice. */
        (void)FFI_CALL_TOS_0(callback);
    }
    while (guest_has_children(f)) {
        for (unsigned child = 1; child < GUEST_TASKS; child++) {
            struct guest_fiber *dependent = &guest_fibers[child];
            if (dependent->state == GUEST_RUNNABLE &&
                *(void **)((unsigned char *)dependent->words + guest_parent_off) == f->words &&
                !dependent->ending) {
                dependent->kill_requested = 1;
                *guest_flags(dependent) |= GUEST_KILL_TASK;
            }
        }
        (void)host_task_yield(NULL);
    }
    finish_guest(me, result);
    enter_guest(next_guest(me));
}
static void guest_trampoline(void) {
    unsigned me = guest_current;
    guest_fibers[me].started = 1;
    if (guest_fibers[me].entry_kind == GUEST_ENTRY_EXECUTIVE)
        guest_executive_loop();
    if (guest_fibers[me].kill_requested)
        end_guest_current(-1);
    if (guest_fibers[me].entry_kind == GUEST_ENTRY_INPUT_FILTER) {
        guest_input_filter_loop();
        end_guest_current(0);
    }
    int64_t result = guest_fibers[me].entry_kind == GUEST_ENTRY_SPAWN
                         ? FFI_CALL_TOS_1(guest_fibers[me].entry,
                                          (int64_t)(uintptr_t)guest_fibers[me].data)
                         : FFI_CALL_TOS_0(guest_fibers[me].entry);
    end_guest_current(result);
}
static int64_t host_task_spawn(int64_t *a) {
    struct guest_fiber *f = new_guest_fiber((void *)(uintptr_t)a[0], NULL,
                                            guest_fibers[guest_current].words, NULL,
                                            GUEST_ENTRY_NO_ARG,
                                            GUEST_STACK_BYTES);
    return f ? (int64_t)(f - guest_fibers) : -1;
}
static int64_t host_call_n_on_stack(int64_t *a) {
    uintptr_t stack_low = (uintptr_t)a[0];
    uintptr_t stack_top = (uintptr_t)a[1] & ~(uintptr_t)15;
    int64_t argc = a[3];
    int64_t *argv = (int64_t *)(uintptr_t)a[4];
    uint64_t spill_bytes;
    if (!stack_low || stack_top <= stack_low || !a[2] || argc < 0 || argc > 1024 ||
        (argc && !argv))
        return -1;
    spill_bytes = argc > 8 ? (uint64_t)(argc - 8) * 8 : 0;
    spill_bytes = (spill_bytes + 15) & ~UINT64_C(15);
    if (stack_top - stack_low < spill_bytes)
        return -1;
    return TempleOS_CallNOnStack(stack_top, (void (*)(void))(uintptr_t)a[2],
                                 argc, argv);
}
static struct guest_fiber *find_guest_task(void *task) {
    for (unsigned i = 0; i < GUEST_TASKS; i++)
        if (guest_fibers[i].words == task && guest_fibers[i].state == GUEST_RUNNABLE)
            return &guest_fibers[i];
    return NULL;
}
static int64_t host_cd(int64_t *a) {
    const char *requested = (const char *)(uintptr_t)a[0];
    char path[134];
    if (!guest_task_bound)
        return 0;
    if (requested && !*requested)
        return 1;
    /* Cd(NULL) selects home, as in DiskDirB. Directory creation is still
     * unavailable; missing paths fail without changing the current dir. */
    if (guest_path(requested ? requested : "~", path))
        return 0;
    const char *dir = path + (!strncmp(path, "disk:", 5) ? 5 : 0);
    if (!source_dir_exists(dir))
        return 0;
    struct guest_fiber *f = &guest_fibers[guest_current];
    char **current = (char **)((unsigned char *)f->words + guest_cur_dir_off);
    char *next = __AIWNIOS_StrDup((char *)dir, &f->heap_owner);
    free(*current);
    *current = next;
    return 1;
}
static int64_t host_dir_cur(int64_t *a) {
    struct guest_fiber *task = a[0] ? find_guest_task((void *)(uintptr_t)a[0])
        : guest_task_bound ? &guest_fibers[guest_current] : NULL;
    struct guest_fiber *owner = a[1] ? find_guest_task((void *)(uintptr_t)a[1])
        : guest_task_bound ? &guest_fibers[guest_current] : NULL;
    if (!task || !owner)
        return 0;
    char *dir = *(char **)((unsigned char *)task->words + guest_cur_dir_off);
    return dir ? (int64_t)(uintptr_t)__AIWNIOS_StrDup(dir, &owner->heap_owner) : 0;
}
static int64_t host_sound(int64_t *a) {
    /* SettingsPop silences the PC speaker. There is no audio device on this
     * platform yet; do not pretend that nonzero notes can be played. */
    if (a[0])
        zc_fail("Sound: audio output is not available on ARM64");
    return 0;
}
static int64_t host_fb_task_text_compose(int64_t *a) {
    struct guest_fiber *f = find_guest_task((void *)(uintptr_t)a[0]);
    uint32_t *plane = guest_text_plane(a[1], a[2], 75);
    if (!f || !plane || a[2] != 100)
        return 0;
    unsigned char *task = (unsigned char *)f->words;
    return (int64_t)zeal_fb_task_text_compose(
        (unsigned)(f - guest_fibers),
        *(int64_t *)(task + guest_win_left_off),
        *(int64_t *)(task + guest_win_right_off),
        *(int64_t *)(task + guest_win_top_off),
        *(int64_t *)(task + guest_win_bottom_off), plane, a[2],
        (const uint16_t *)(uintptr_t)a[3], (uint16_t)a[4]);
}
static int64_t host_fb_shell_text_compose(int64_t *a) {
    uint32_t *plane = guest_text_plane(a[0], a[1], 75);
    if (!plane || a[1] != 100)
        return 0;
    return (int64_t)zeal_fb_shell_text_compose(plane, a[1]);
}
static int focused_key_wait_pending(void) {
    struct guest_fiber *focus = find_guest_task(guest_focus_task);
    return key_reader && focus && focus->key_wait_active &&
           focus->wait_kind == GUEST_WAIT_MESSAGE &&
           !(*guest_flags(focus) & GUEST_SUSPENDED);
}
static int valid_guest_task(void *task) {
    struct guest_fiber *f = find_guest_task(task);
    return f && *(void **)f->words == f->words &&
           !memcmp((unsigned char *)f->words + 8, "TskS", 4);
}
static int64_t host_task_focus_next(int64_t *a) {
    (void)a;
    /* Walk the application ring; the hidden System server inhibits focus.
     * The CPU executive is outside this ring. */
    void *root = guest_fibers[0].words;
    void *start = valid_guest_task(guest_focus_task) &&
                  guest_focus_task != guest_fibers[GUEST_EXECUTIVE].words
        ? guest_focus_task : root;
    void *task = *(void **)((unsigned char *)start + guest_next_task_off);
    guest_focus_task = NULL;
    for (unsigned hops = 0; hops < GUEST_TASKS; hops++) {
        if (!valid_guest_task(task) || task == guest_fibers[GUEST_EXECUTIVE].words)
            zc_fail("guest task ring corrupted");
        if (!(*(uint32_t *)((unsigned char *)task + guest_win_inhibit_off) & 1u)) {
            guest_focus_task = task;
            return 0;
        }
        if (task == start)
            break;
        task = *(void **)((unsigned char *)task + guest_next_task_off);
    }
    return 0;
}
static int64_t host_spawn(int64_t *a) {
    void *entry = (void *)(uintptr_t)a[0];
    void *parent = a[4] ? (void *)(uintptr_t)a[4] : guest_fibers[0].words;
    /* UTM exposes one guest CPU, so explicit CPU 0 and the upstream default
     * (-1, any CPU) have identical scheduling semantics here. */
    if (!guest_task_bound || (a[3] != -1 && a[3] != 0) ||
        a[5] < 0 || (uint64_t)a[5] > GUEST_STACK_MAX_BYTES ||
        a[6] != (1 << 8) ||
        !find_guest_task(parent))
        return 0;
    size_t stack_size = a[5] ? (size_t)a[5] : GUEST_STACK_BYTES;
    if (stack_size < GUEST_STACK_BYTES)
        stack_size = GUEST_STACK_BYTES;
    struct guest_fiber *f = new_guest_fiber(entry, (void *)(uintptr_t)a[1], parent,
                                            (const char *)(uintptr_t)a[2], GUEST_ENTRY_SPAWN,
                                            stack_size);
    return (int64_t)(uintptr_t)(f ? f->words : NULL);
}
static int64_t host_task_validate(int64_t *a) {
    return valid_guest_task((void *)(uintptr_t)a[0]);
}
static int64_t host_is_suspended(int64_t *a) {
    void *task = a[0] ? (void *)(uintptr_t)a[0] : guest_fibers[guest_current].words;
    struct guest_fiber *f = find_guest_task(task);
    return f && !!(*guest_flags(f) & GUEST_SUSPENDED);
}
static int64_t host_suspend(int64_t *a) {
    void *task = a[0] ? (void *)(uintptr_t)a[0] : guest_fibers[guest_current].words;
    struct guest_fiber *f = find_guest_task(task);
    if (!f || f == &guest_fibers[0] || f == &guest_fibers[GUEST_SYSTEM])
        return 0; /* The shell and system server must remain schedulable. */
    uint32_t *flags = guest_flags(f);
    int64_t old = !!(*flags & GUEST_SUSPENDED);
    if (a[1])
        *flags |= GUEST_SUSPENDED;
    else
        *flags &= ~GUEST_SUSPENDED;
    return old;
}
static int64_t host_kill(int64_t *a) {
    struct guest_fiber *f = find_guest_task((void *)(uintptr_t)a[0]);
    if (!f || f == &guest_fibers[0] || f == &guest_fibers[GUEST_EXECUTIVE] ||
        f == &guest_fibers[GUEST_SYSTEM] || a[2])
        return 0; /* Protect the three session services. */
    unsigned i = (unsigned)(f - guest_fibers);
    if (dispatch_blocks_current()) {
        void *ancestor = guest_fibers[guest_current].words;
        for (unsigned hop = 0; hop < GUEST_TASKS && ancestor; hop++) {
            if (ancestor == f->words)
                zc_fail("kill current or ancestor during job dispatch is unsupported");
            struct guest_fiber *current = find_guest_task(ancestor);
            ancestor = current ? *(void **)((unsigned char *)current->words + guest_parent_off)
                               : NULL;
        }
    }
    if (f->ending)
        return 1; /* The callback already started; DeathWait waits for teardown. */
    int has_callback = !!*(void **)((unsigned char *)f->words + guest_end_cb_off);
    if (has_callback || guest_has_children(f)) {
        if (dispatch_blocks_current())
            zc_fail("kill with task teardown during job dispatch is unsupported");
        f->kill_requested = 1;
        *guest_flags(f) |= GUEST_KILL_TASK;
        if (i == guest_current)
            end_guest_current(-1);
        if (a[1])
            while (f->state == GUEST_RUNNABLE &&
                   (f->kill_requested || !has_callback))
                (void)host_task_yield(NULL);
        return 1;
    }
    finish_guest(i, -1);
    if (i == guest_current)
        enter_guest(next_guest(i));
    return 1; /* Termination is synchronous, so wait=TRUE is already met. */
}
static int64_t host_task_yield(int64_t *a) {
    (void)a;
    /* A frame can borrow root's identity while running on WinMgr's stack.
     * Saving a scheduler context here would replace root's suspended shell
     * context with that borrowed stack. Let DocRecalc's short lock deadline
     * advance and bail out; the owning task can finish after the frame. */
    if (guest_task_frame_active) {
        if (guest_focus_task)
            poll_guest_keys();
        (void)host_time_cal(NULL);
        return 0;
    }
    if (guest_task_exe_active)
        zc_fail("yield during job dispatch is unsupported");
    unsigned me = guest_current;
    struct guest_fiber *self = &guest_fibers[me];
    /* Root's fallback jobs remain available to module-only sessions. The
     * dedicated source server dispatches its ordinary work in ServerTaskCont;
     * a yielding system job admits only nested internal Sys requests. Never
     * enter the parser while it is compiling. */
    int serve_system = !me || (me == GUEST_SYSTEM &&
                              (self->source_job_active || self->active_jobs));
    uint32_t was_idle = *guest_flags(self) & GUEST_IDLE;
    if (serve_system) {
        int64_t handle[2] = {0, 0};
        (void)host_jobs_handler_mode(handle, self->source_job_active || self->active_jobs);
        // Only the module-only root fallback borrows IDLE while yielding.
        // A real server with an in-flight job must keep TaskWait blocked.
        if (!me)
            *guest_flags(self) |= GUEST_IDLE;
    }
    /* Only poll device input for the explicitly focused task. A merely
     * waiting background task must not steal serial-shell input. */
    if (guest_focus_task)
        poll_guest_keys();
    int park_server = !me ? 0 : self->wait_kind == GUEST_WAIT_NONE &&
        (*guest_flags(self) & GUEST_AWAITING_MESSAGE) &&
        guest_job_head(self)->next == guest_job_head(self);
    if (park_server) {
        self->wait_target = self->words;
        self->wait_slot = &self->wait_target;
        self->wait_kind = GUEST_WAIT_SERVER;
    }
    unsigned next = next_guest(me);
    if (next != me && !setjmp(self->context))
        enter_guest(next);
    /* A child can queue Sys work and suspend itself in the slice above.
     * Pump before returning to root's task-wait loop, which may otherwise
     * see no runnable child and stop before dispatching the request. */
    if (serve_system) {
        int64_t handle[2] = {0, 0};
        (void)host_jobs_handler_mode(handle, self->source_job_active || self->active_jobs);
        *guest_flags(self) = (*guest_flags(self) & ~GUEST_IDLE) | was_idle;
    }
    if (self->kill_requested && !self->ending)
        end_guest_current(-1);
    if (park_server)
        self->wait_kind = GUEST_WAIT_NONE;
    return 0;
}
static int64_t host_task_exit(int64_t *a) {
    (void)a;
    if (guest_task_exe_active ||
        (guest_job_call_active && guest_current == GUEST_EXECUTIVE))
        zc_fail("Exit during compilation or executive callback is unsupported");
    if (guest_current == GUEST_SYSTEM)
        guest_raise(GUEST_EXCEPT_BREAK);
    end_guest_current(0);
}
static int64_t guest_wait(enum guest_wait kind, void *slot, int64_t number) {
    if (!guest_task_bound || !slot)
        zc_fail("task wait needs a task-pointer slot");
    struct guest_fiber *self = &guest_fibers[guest_current];
    self->wait_kind = kind;
    self->wait_slot = slot;
    self->wait_number = number;
    while (!wait_ready(self)) {
        if (!guest_current && !next_guest(0)) {
            if (!guest_timer_pending() && !focused_key_wait_pending())
                zc_fail("task wait cannot progress");
            __asm__ volatile("yield");
        }
        host_task_yield(NULL);
    }
    self->wait_kind = GUEST_WAIT_NONE;
    return kind == GUEST_WAIT_BIRTH
               ? *(int64_t *)((unsigned char *)*(void **)slot + guest_number_off)
               : 0;
}
static int64_t host_birth_wait(int64_t *a) {
    return guest_wait(GUEST_WAIT_BIRTH, (void *)(uintptr_t)a[0], a[1]);
}
static int64_t host_death_wait(int64_t *a) {
    void **slot = (void **)(uintptr_t)a[0];
    if (!slot)
        zc_fail("DeathWait needs a task-pointer slot");
    if (a[1]) {
        void *target = *slot;
        struct guest_fiber *f = find_guest_task(target);
        if (f && target != guest_fibers[guest_current].words &&
            !f->started && !(*guest_flags(f) & GUEST_SUSPENDED)) {
            /* Give a newly spawned target its first cooperative slice before
             * requesting exit. This lets its entry install task_end_cb (the
             * guest equivalent of accepting the upstream EXIT message).
             * Without it, an immediate DeathWait(TRUE) could synchronously
             * discard the never-started fiber and skip that callback. */
            while (f->state == GUEST_RUNNABLE && !f->started)
                (void)host_task_yield(NULL);
        }
        if (valid_guest_task(target)) {
            /* Upstream asks the target's idle command server to run Exit;
             * this bootstrap has no server command loop for every task.
             * Reuse the task teardown path instead: it runs end callbacks,
             * kills descendants, and only returns once the target is dead. */
            int64_t kill_args[3] = {(int64_t)(uintptr_t)target, 1, 0};
            if (!host_kill(kill_args))
                zc_fail("DeathWait could not request task exit");
        }
    }
    return guest_wait(GUEST_WAIT_DEATH, slot, 0);
}
static int64_t host_task_wait(int64_t *a) {
    void *target = a[0] ? (void *)(uintptr_t)a[0] : guest_fibers[guest_current].words;
    if (!valid_guest_task(target))
        return 0;
    struct guest_fiber *self = &guest_fibers[guest_current];
    self->wait_target = target;
    (void)guest_wait(GUEST_WAIT_IDLE, &self->wait_target, !!a[1]);
    return 0;
}
static int guest_task_idle(void *target, int cmd_line_prompt) {
    if (!valid_guest_task(target))
        return 1;
    void *filter = *(void **)((unsigned char *)target + guest_input_filter_off);
    struct guest_fiber *input = find_guest_task(filter);
    if (filter == guest_fibers[guest_current].words || !input ||
        !valid_guest_task(filter))
        return 1;
    unsigned char *ctrl = (unsigned char *)filter + guest_server_ctrl_off;
    return *(void **)ctrl == ctrl && (*guest_flags(input) & GUEST_IDLE) &&
           (!cmd_line_prompt || (*guest_flags(input) & GUEST_CMD_LINE_PROMPT));
}
static int64_t host_task_wait_timeout(int64_t *a) {
    void *target = a[0] ? (void *)(uintptr_t)a[0]
                        : guest_fibers[guest_current].words;
    int64_t timeout_ms = a[1];
    if (timeout_ms < 0)
        zc_fail("TaskWaitTimeout requires a nonnegative timeout");
    if (!valid_guest_task(target) || guest_task_idle(target, !!a[2]))
        return 1;
    struct guest_fiber *self = &guest_fibers[guest_current];
    self->wait_target = target;
    self->wait_slot = &self->wait_target;
    self->wait_number = !!a[2];
    self->wait_deadline = deadline_after_milliseconds((uint64_t)timeout_ms);
    self->wait_kind = GUEST_WAIT_IDLE_TIMEOUT;
    for (;;) {
        if (guest_task_idle(target, !!a[2])) {
            self->wait_kind = GUEST_WAIT_NONE;
            return 1;
        }
        if ((int64_t)(counter_now() - self->wait_deadline) >= 0) {
            self->wait_kind = GUEST_WAIT_NONE;
            return 0;
        }
        if (!guest_current && !next_guest(0) && !guest_timer_pending())
            zc_fail("timed task wait cannot progress");
        (void)host_task_yield(NULL);
        __asm__ volatile("yield");
    }
}
static struct guest_job *post_guest_message(void *server_task, void *master,
                                            int64_t code, int64_t arg1, int64_t arg2,
                                            int64_t flags) {
    struct guest_fiber *server = find_guest_task(server_task);
    if (!server || (master && !valid_guest_task(master)))
        return NULL;
    if (*(void **)((unsigned char *)server->words + guest_popup_off) &&
        !(*guest_flags(server) & GUEST_FILTER_INPUT))
        return NULL;
    if (code == INT64_MIN)
        zc_fail("message code out of range");
    for (unsigned hops = 0; hops < GUEST_TASKS &&
            (*guest_flags(server) & GUEST_FILTER_INPUT) &&
            !(flags & GUEST_JOBF_DONT_FILTER); hops++) {
        void *next = *(void **)((unsigned char *)server->words + guest_next_filter_off);
        server = find_guest_task(next);
        if (!server)
            return NULL;
        if (hops + 1 == GUEST_TASKS)
            zc_fail("input filter cycle");
    }
    int64_t first_code = code < 0 ? -code : code;
    int64_t second_code = code < 0 ? first_code + 1 : -1;
    if (first_code >= 64 || second_code >= 64)
        zc_fail("message code out of range");
    unsigned needed = code < 0 ? 2 : 1;
    /* Pointer motion is a latest-state notification, unlike button/key
     * transitions. If the recipient has not consumed the immediately
     * preceding move, replace its coordinates instead of allowing high-rate
     * tablet input to exhaust the bounded shared guest-job pool. Do not scan
     * past another event: that preserves all input ordering barriers. */
    if (code == GUEST_MESSAGE_MS_MOVE && needed == 1) {
        struct guest_job *head = guest_job_head(server);
        struct guest_job *last = head->last;
        if (last != head && last->job_code == GUEST_JOBT_MESSAGE &&
            last->message_code == GUEST_MESSAGE_MS_MOVE &&
            last->master_task == master && last->flags == flags) {
            last->aux1 = arg1;
            last->aux2 = arg2;
            *guest_flags(server) &= ~(GUEST_IDLE | GUEST_AWAITING_MESSAGE);
            return last;
        }
    }
    if (guest_job_count > GUEST_JOBS_MAX - needed) {
        /* Motion is replaceable state, so preserve the task and its queued
         * key/button transitions if unrelated traffic has filled the pool. */
        if (code == GUEST_MESSAGE_MS_MOVE && needed == 1)
            return NULL;
        zc_fail("guest message queue full");
    }
    struct guest_job *first = NULL;
    for (unsigned i = 0; i < needed; i++) {
        struct guest_job *job = __AIWNIOS_CAlloc(sizeof(*job), &data_heap);
        job->ctrl = (unsigned char *)server->words + guest_server_ctrl_off;
        job->job_code = GUEST_JOBT_MESSAGE;
        job->flags = flags;
        job->message_code = i ? second_code : first_code;
        job->aux1 = arg1;
        job->aux2 = arg2;
        job->master_task = master;
        guest_job_insert(guest_job_head(server), job);
        guest_job_track(job);
        if (!first)
            first = job;
    }
    *guest_flags(server) &= ~(GUEST_IDLE | GUEST_AWAITING_MESSAGE);
    /* Pinned TaskResetAwaitingMessage wakes each linked popup when a parent
     * receives a message. Keep IDLE until that popup actually scans it. */
    for (unsigned hops = 0; hops < GUEST_TASKS; hops++) {
        server = guest_popup_child(server);
        if (!server)
            break;
        *guest_flags(server) &= ~GUEST_AWAITING_MESSAGE;
    }
    return first;
}
static int64_t host_task_message(int64_t *a) {
    return (int64_t)(uintptr_t)post_guest_message((void *)(uintptr_t)a[0],
        (void *)(uintptr_t)a[1], a[2], a[3], a[4], a[5]);
}
static int64_t host_message_post(int64_t *a) {
    void *task = (void *)(uintptr_t)a[0];
    struct guest_fiber *server = find_guest_task(task);
    if (!server)
        return 0;
    int64_t flags = a[4];
    if (*guest_flags(server) & GUEST_INPUT_FILTER_TASK) {
        task = *(void **)((unsigned char *)task + guest_input_filter_off);
        flags |= GUEST_JOBF_DONT_FILTER;
    }
    (void)post_guest_message(task, NULL, a[1], a[2], a[3], flags);
    return 0;
}
static int64_t host_message_scan(int64_t *a) {
    void *task = a[3] ? (void *)(uintptr_t)a[3] : guest_fibers[guest_current].words;
    struct guest_fiber *server = find_guest_task(task);
    /* Until a focus/window router exists, the task reading its own queue
     * owns live keyboard input. Remote queue inspection never steals it. */
    if (server == &guest_fibers[guest_current])
        poll_guest_keys();
    /* Pinned SerialDev/Message.ZC calls JobsHandler before inspecting the
     * current task's message queue. A MessageGet server can therefore run a
     * queued TaskExe without a separate explicit JobsHandler call. */
    if (server == &guest_fibers[guest_current] &&
        !dispatch_blocks_current()) {
        int64_t handle[2] = {0, (int64_t)(uintptr_t)task};
        (void)host_jobs_handler(handle);
        server = find_guest_task(task);
    }
    for (unsigned hops = 0; server && hops < GUEST_TASKS; hops++) {
        struct guest_job *head = guest_job_head(server);
        while (head->next != head) {
            struct guest_job *job = head->next;
            if (job->job_code != GUEST_JOBT_MESSAGE)
                break; /* A future JobsHandler owns non-message requests. */
            int64_t code = job->message_code, arg1 = job->aux1, arg2 = job->aux2;
            guest_job_unlink(job);
            guest_job_delete(job);
            /* SerialDev/Message.ZC discards key-description events when the
             * recipient inhibits them, even if the caller's mask includes
             * MESSAGE_KEY_DOWN. Keep the copied scan code after deletion. */
            if (code == 2 && ((uint64_t)arg2 & GUEST_SCF_KEY_DESC) &&
                (*(uint32_t *)((unsigned char *)server->words + guest_win_inhibit_off) &
                 GUEST_WIF_SELF_KEY_DESC))
                continue;
            if (code > 0 && code < 64 && ((uint64_t)a[2] & (1ull << code))) {
                if (a[0]) *(int64_t *)(uintptr_t)a[0] = arg1;
                if (a[1]) *(int64_t *)(uintptr_t)a[1] = arg2;
                return code;
            }
        }
        server = guest_popup_parent(server);
    }
    if (a[0]) *(int64_t *)(uintptr_t)a[0] = 0;
    if (a[1]) *(int64_t *)(uintptr_t)a[1] = 0;
    return 0;
}
static int64_t poll_guest_keys(void) {
    if (!key_reader)
        return 0;
    guest_keyboard_route_polls++;
    void *task = guest_focus_task;
    /* The shell retains its keyboard while it is idle. Only an explicitly
     * focused child may take device input during background dispatch. */
    if (guest_background_dispatch) {
        if (!valid_guest_task(task)) {
            guest_focus_task = NULL;
            return 0;
        }
        if (task == guest_fibers[0].words)
            return 0;
    }
    if (!valid_guest_task(task)) {
        guest_focus_task = NULL;
        task = guest_fibers[guest_current].words;
    }
    int64_t last_code = 0;
    for (unsigned i = 0; i < 32; i++) {
        uint8_t type;
        uint64_t ch, sc;
        if (!key_reader(&type, &ch, &sc))
            break;
        guest_keyboard_route_reads++;
        guest_keyboard_route_last_type = type;
        guest_keyboard_route_last_arg1 = (int64_t)ch;
        guest_keyboard_route_last_arg2 = (int64_t)sc;
        if (type != 2 && type != 3)
            continue;
        if (post_guest_message(task, NULL,
                               type, (int64_t)ch, (int64_t)sc, 0))
        {
            guest_keyboard_route_posts++;
            last_code = type;
        }
        else
            guest_keyboard_route_drops++;
    }
    return last_code;
}
static int64_t host_kbd_mouse_handler(int64_t *a) {
    if (a[1])
        zc_fail("KbdMouseHandler mouse polling is unsupported");
    guest_fibers[guest_current].last_polled_key_code = a[0]
        ? poll_guest_keys() : 0;
    return 0;
}
static int64_t host_zc_tablet_sample(int64_t *a) {
    (void)a;
    return zc_tablet_sample();
}
static int64_t host_kbd_messages_queue(int64_t *a) {
    (void)a;
    struct guest_fiber *f = &guest_fibers[guest_current];
    int64_t result = f->last_polled_key_code;
    f->last_polled_key_code = 0;
    return result;
}
static int64_t host_put_key(int64_t *a) {
    if (!guest_task_bound ||
        (*(uint32_t *)((unsigned char *)guest_fibers[guest_current].words +
                       guest_display_flags_off) &
         (1u << 2)) ||
        ((uint64_t)a[1] & GUEST_SCF_KEY_DESC))
        return 0;
    /* Message.ZC's KeyGet is compiled before KeyDev.ZC and retains this
     * bootstrap echo target. Once the graphical task installs its document
     * device, honor that real output chain (including editor/navigation keys)
     * instead of sending its input to the hidden bootstrap text console. */
    if (guest_keydev_put_key &&
        (*guest_flags(&guest_fibers[guest_current]) & GUEST_INPUT_FILTER_TASK)) {
        FFI_CALL_TOS_2(guest_keydev_put_key, a[0], a[1]);
        return 0;
    }
    if (gui_terminal_active && guest_keydev_put_key) {
        CHashFun *terminal = (CHashFun *)HashFind(
            "BootstrapGuiTerminalState", Fs->hash_table, HTT_FUN, 1);
        if (terminal && terminal->fun_ptr && terminal->argc == 1 &&
            !(terminal->base.base.type & HTF_EXTERN) &&
            FFI_CALL_TOS_1(terminal->fun_ptr,
                           (int64_t)(uintptr_t)guest_fibers[guest_current].words)) {
            FFI_CALL_TOS_2(guest_keydev_put_key, a[0], a[1]);
            return 0;
        }
    }
    if (a[0] >= 32 && a[0] < 127) {
        char shown[2] = {(char)a[0], 0};
        zc_output(shown);
        guest_task_text_output(shown);
    }
    return 0;
}
static int64_t host_str_len(int64_t *a) {
    const char *s = (const char *)(uintptr_t)a[0];
    if (!s)
        zc_fail("StrLen source is null");
    for (size_t n = 0; n <= 1024 * 1024; n++)
        if (!s[n])
            return (int64_t)n;
    zc_fail("StrLen exceeds 1 MiB");
    return 0;
}
static int64_t host_key_scan(int64_t *a) {
    int64_t ch = 0, sc = 0;
    int64_t scan[4] = {(int64_t)(uintptr_t)&ch, (int64_t)(uintptr_t)&sc,
                       1ll << 2, 0};
    int64_t found = host_message_scan(scan) == 2;
    if (a[0]) *(int64_t *)(uintptr_t)a[0] = ch;
    if (a[1]) *(int64_t *)(uintptr_t)a[1] = sc;
    if (found && a[2]) {
        int64_t key[2] = {ch, sc};
        host_put_key(key);
    }
    return found;
}
static int64_t host_key_get(int64_t *a) {
    if (a[2])
        zc_fail("KeyGet raw cursor is not implemented");
    /* Emitted source may wait for a key, as MessageGet already does. Keep
     * the non-switching compile phase and executive callback protected. */
    if (guest_task_exe_active ||
        (guest_job_call_active && guest_current == GUEST_EXECUTIVE))
        zc_fail("KeyGet during compilation or executive callback is unsupported");
    int64_t ch = 0, sc = 0;
    int64_t scan[3] = {(int64_t)(uintptr_t)&ch, (int64_t)(uintptr_t)&sc, 0};
    while (!host_key_scan(scan)) {
        struct guest_fiber *self = &guest_fibers[guest_current];
        if (guest_current) {
            self->wait_target = self->words;
            self->wait_slot = &self->wait_target;
            self->wait_kind = GUEST_WAIT_MESSAGE;
        }
        self->key_wait_active = 1;
        (void)host_task_yield(NULL);
        self->key_wait_active = 0;
        self->wait_kind = GUEST_WAIT_NONE;
        __asm__ volatile("yield");
    }
    if (a[0]) *(int64_t *)(uintptr_t)a[0] = sc;
    if (a[1]) {
        int64_t key[2] = {ch, sc};
        host_put_key(key);
    }
    return ch;
}
static int64_t host_message_get(int64_t *a) {
    void *task = a[3] ? (void *)(uintptr_t)a[3] : guest_fibers[guest_current].words;
    struct guest_fiber *server = find_guest_task(task);
    int device_key_wait = key_reader && task == guest_fibers[guest_current].words &&
        ((uint64_t)a[2] & ((1ull << 2) | (1ull << 3)));
    if (!server)
        zc_fail("message target unavailable");
    *guest_flags(server) &= ~GUEST_IDLE;
    for (;;) {
        int64_t code = host_message_scan(a);
        if (code) {
            *guest_flags(server) &= ~GUEST_IDLE;
            return code;
        }
        *guest_flags(server) |= GUEST_IDLE;
        struct guest_fiber *self = &guest_fibers[guest_current];
        self->wait_target = task;
        self->wait_slot = &self->wait_target;
        self->wait_kind = GUEST_WAIT_MESSAGE;
        if (!guest_current && !next_guest(0) && !device_key_wait)
            zc_fail("message wait cannot progress");
        self->key_wait_active = device_key_wait;
        host_task_yield(NULL);
        self->key_wait_active = 0;
        self->wait_kind = GUEST_WAIT_NONE;
        if (device_key_wait)
            __asm__ volatile("yield");
        server = find_guest_task(task);
        if (!server)
            zc_fail("message target ended while waiting");
    }
}
static int64_t host_message_get_timeout(int64_t *a) {
    void *task = a[3] ? (void *)(uintptr_t)a[3]
                      : guest_fibers[guest_current].words;
    struct guest_fiber *server = find_guest_task(task);
    int64_t timeout_ms = a[4];
    int device_key_wait = key_reader && task == guest_fibers[guest_current].words &&
        ((uint64_t)a[2] & ((1ull << 2) | (1ull << 3)));
    if (!server)
        zc_fail("message target unavailable");
    if (timeout_ms < 0)
        zc_fail("MessageGetTimeout requires a nonnegative timeout");
    struct guest_fiber *self = &guest_fibers[guest_current];
    self->wait_target = task;
    self->wait_slot = &self->wait_target;
    self->wait_deadline = deadline_after_milliseconds((uint64_t)timeout_ms);
    for (;;) {
        *guest_flags(server) &= ~GUEST_IDLE;
        int64_t code = host_message_scan(a);
        if (code) {
            *guest_flags(server) &= ~GUEST_IDLE;
            self->wait_kind = GUEST_WAIT_NONE;
            self->key_wait_active = 0;
            return code;
        }
        *guest_flags(server) |= GUEST_IDLE;
        if ((int64_t)(counter_now() - self->wait_deadline) >= 0) {
            self->wait_kind = GUEST_WAIT_NONE;
            self->key_wait_active = 0;
            return 0;
        }
        self->wait_kind = GUEST_WAIT_MESSAGE_TIMEOUT;
        self->key_wait_active = device_key_wait;
        if (!guest_current && !next_guest(0) && !guest_timer_pending() &&
            !device_key_wait)
            zc_fail("timed message wait cannot progress");
        (void)host_task_yield(NULL);
        self->key_wait_active = 0;
        server = find_guest_task(task);
        if (!server) {
            self->wait_kind = GUEST_WAIT_NONE;
            zc_fail("message target ended while waiting");
        }
    }
}
static int64_t host_flush_messages(int64_t *a) {
    int64_t scan[4] = {0, 0, ~1ll, a[0]};
    int64_t count = 0;
    while (host_message_scan(scan))
        count++;
    return count;
}
static int64_t host_message_post_wait(int64_t *a) {
    host_message_post(a);
    int64_t wait[2] = {a[0], 0};
    return host_task_wait(wait);
}
static int64_t host_message_self(int64_t *a) {
    int64_t post[5] = {(int64_t)(uintptr_t)guest_fibers[guest_current].words,
                       a[0], a[1], a[2], a[3]};
    return host_message_post(post);
}
struct dispatch_scope {
    struct guest_unwind unwind;
    int compiling, callbacks, source_jobs;
};
static void dispatch_scope_cleanup(struct guest_unwind *scope) {
    struct dispatch_scope *dispatch = (struct dispatch_scope *)scope;
    guest_task_exe_active = dispatch->compiling;
    guest_job_call_active = dispatch->callbacks;
    guest_fibers[guest_current].source_job_active = dispatch->source_jobs;
    guest_unwind_flags(scope);
}
static void dispatch_scope_enter(struct dispatch_scope *dispatch, jmp_buf *landing) {
    dispatch->compiling = guest_task_exe_active;
    dispatch->callbacks = guest_job_call_active;
    dispatch->source_jobs = guest_fibers[guest_current].source_job_active;
    guest_unwind_enter(&dispatch->unwind, dispatch_scope_cleanup, landing);
}
static void guest_input_filter_loop(void) {
    struct guest_fiber *filter = &guest_fibers[guest_current];
    struct guest_job *head = guest_job_head(filter);
    for (unsigned handled = 0; handled < GUEST_JOBS_MAX; handled++) {
        /* Upstream InputFilterTask runs JobsHandler before looking for the
         * next text job. Keep routed key messages with the parent so they
         * cannot block a queued executable job in this bounded filter. */
        struct guest_job *job = head->next;
        if (job == head)
            break;
        if (job->job_code == GUEST_JOBT_MESSAGE) {
            void *parent_task = *(void **)((unsigned char *)filter->words + guest_parent_off);
            struct guest_fiber *parent = find_guest_task(parent_task);
            if (!parent)
                break;
            guest_job_unlink(job);
            job->ctrl = (unsigned char *)parent->words + guest_server_ctrl_off;
            guest_job_insert(guest_job_head(parent), job);
            *guest_flags(parent) &= ~(GUEST_IDLE | GUEST_AWAITING_MESSAGE);
            continue;
        }
        if (job->job_code != GUEST_JOBT_TEXT_INPUT) {
            int64_t args[2] = {0, 0};
            if (!host_jobs_handler(args))
                break;
            continue;
        }
        guest_job_unlink(job);
        *guest_flags(filter) &= ~GUEST_IDLE;
        struct guest_active_job active = {filter->active_jobs, job};
        filter->active_jobs = &active;
        struct dispatch_scope dispatch;
        jmp_buf landing;
        dispatch_scope_enter(&dispatch, &landing);
        int raised = setjmp(landing);
        if (!raised) {
            filter->source_job_active++;
            guest_task_exe_active = 1;
            (void)execute_source_result("<TaskText>", job->aux_str, 1, 1);
        }
        guest_unwind_leave(&dispatch.unwind);
        if (raised)
            guest_unwind_handled(&dispatch.unwind, dispatch.unwind.exception);
        filter->active_jobs = active.previous;
        guest_job_delete(job);
    }
}
static int64_t host_task_text(int64_t *a) {
    void *server_task = (void *)(uintptr_t)a[0];
    struct guest_fiber *server = find_guest_task(server_task);
    void *master = (void *)(uintptr_t)a[1];
    const char *source = (const char *)(uintptr_t)a[2];
    uint64_t flags = (uint64_t)a[3];
    /* The synthetic filter supports one server link; other job modes still
     * require the upstream input/Doc runtime. */
    if (!server || server->entry_kind == GUEST_ENTRY_INPUT_FILTER || !source ||
        (*(void **)((unsigned char *)server->words + guest_popup_off) &&
         !(*guest_flags(server) & GUEST_FILTER_INPUT)) ||
        (master && !valid_guest_task(master)) ||
        (flags & ~(uint64_t)GUEST_JOBF_HIGHEST_PRIORITY) ||
        dispatch_blocks_current())
        return 0;
    size_t length = 0;
    while (length <= 4096 && source[length])
        length++;
    if (length > 4096)
        zc_fail("TaskText text exceeds 4096 bytes");
    if (guest_job_count == GUEST_JOBS_MAX)
        zc_fail("guest job queue full");
    /* Pinned TaskText copies data before waiting for an old filter. The
     * source pointer may refer to memory owned by that filter. */
    struct guest_job *job = __AIWNIOS_CAlloc(sizeof(*job), &data_heap);
    char *copy = __AIWNIOS_CAlloc(length + 1, &data_heap);
    memcpy(copy, source, length);
    job->job_code = GUEST_JOBT_TEXT_INPUT;
    job->flags = (int64_t)flags;
    job->aux_str = copy;
    job->master_task = master;
    /* Reserve this job before TaskWait can dispatch and change the queue. */
    guest_job_track(job);
    void *filter_task = *(void **)((unsigned char *)server->words + guest_next_filter_off);
    if ((flags & GUEST_JOBF_HIGHEST_PRIORITY) && filter_task != server_task) {
        int64_t wait[2] = {(int64_t)(uintptr_t)server_task, 0};
        (void)host_task_wait(wait);
        server = find_guest_task(server_task);
        if (!server) {
            guest_job_delete(job);
            return 0;
        }
        filter_task = *(void **)((unsigned char *)server->words + guest_next_filter_off);
        if (filter_task != server_task) {
            /* A live idle filter needs the upstream multi-filter ring. */
            guest_job_delete(job);
            return 0;
        }
    }
    struct guest_fiber *filter;
    if (filter_task == server->words) {
        filter = new_guest_fiber(NULL, NULL, server->words, "Input Filter",
                                 GUEST_ENTRY_INPUT_FILTER, GUEST_STACK_BYTES);
        if (!filter) {
            guest_job_delete(job);
            return 0;
        }
        *(void **)((unsigned char *)server->words + guest_input_filter_off) = filter->words;
        *(void **)((unsigned char *)server->words + guest_next_filter_off) = filter->words;
        *(void **)((unsigned char *)filter->words + guest_input_filter_off) = server->words;
        *(void **)((unsigned char *)filter->words + guest_next_filter_off) = server->words;
        *guest_flags(server) |= GUEST_FILTER_INPUT;
        *guest_flags(filter) |= GUEST_INPUT_FILTER_TASK;
    } else {
        filter = find_guest_task(filter_task);
        if (!filter || filter->entry_kind != GUEST_ENTRY_INPUT_FILTER ||
            *(void **)((unsigned char *)filter->words + guest_parent_off) != server_task) {
            guest_job_delete(job);
            return 0;
        }
    }
    job->ctrl = (unsigned char *)filter->words + guest_server_ctrl_off;
    guest_job_insert(guest_job_head(filter), job);
    *guest_flags(filter) &= ~(GUEST_IDLE | GUEST_AWAITING_MESSAGE);
    return (int64_t)(uintptr_t)job;
}
static int64_t host_task_exe_mode(int64_t *a, int system_request) {
    struct guest_fiber *server = find_guest_task((void *)(uintptr_t)a[0]);
    void *master = (void *)(uintptr_t)a[1];
    const char *source = (const char *)(uintptr_t)a[2];
    uint64_t flags = (uint64_t)a[3];
    if (!server || !source ||
        (!(system_request && (server == &guest_fibers[0] ||
                             server == &guest_fibers[GUEST_SYSTEM])) &&
         *(void **)((unsigned char *)server->words + guest_popup_off) &&
         !(*guest_flags(server) & GUEST_FILTER_INPUT)) ||
        (master && !valid_guest_task(master)) ||
        (flags & ~(uint64_t)(GUEST_JOBF_WAKE_MASTER |
                             GUEST_JOBF_FOCUS_MASTER |
                             GUEST_JOBF_EXIT_ON_COMPLETE |
                             GUEST_JOBF_FREE_ON_COMPLETE)) ||
        ((flags & GUEST_JOBF_WAKE_MASTER) &&
         (master != guest_fibers[guest_current].words || master == server->words)) ||
        ((flags & GUEST_JOBF_EXIT_ON_COMPLETE) &&
         (server == &guest_fibers[0] || server == &guest_fibers[GUEST_SYSTEM])))
        return 0;
    size_t length = 0;
    while (length <= 4096 && source[length])
        length++;
    if (length > 4096)
        zc_fail("TaskExe text exceeds 4096 bytes");
    if (guest_job_count == GUEST_JOBS_MAX)
        zc_fail("guest job queue full");
    struct guest_job *job = __AIWNIOS_CAlloc(sizeof(*job), &data_heap);
    char *copy = __AIWNIOS_CAlloc(length + 1, &data_heap);
    memcpy(copy, source, length);
    job->ctrl = (unsigned char *)server->words + guest_server_ctrl_off;
    job->job_code = GUEST_JOBT_EXE_STR;
    job->flags = (int64_t)(flags | (system_request ? GUEST_JOBF_SYSTEM_REQUEST : 0));
    job->aux_str = copy;
    job->master_task = master;
    guest_job_insert(guest_job_head(server), job);
    guest_job_track(job);
    *guest_flags(server) &= ~(GUEST_IDLE | GUEST_AWAITING_MESSAGE);
    if (flags & GUEST_JOBF_WAKE_MASTER) {
        struct guest_fiber *self = &guest_fibers[guest_current];
        self->wait_target = job;
        (void)guest_wait(GUEST_WAIT_JOB, &self->wait_target, 0);
    }
    return (int64_t)(uintptr_t)job;
}
static int64_t host_task_exe(int64_t *a) {
    return host_task_exe_mode(a, 0);
}
static int64_t host_job_queue(int64_t *a) {
    void *callback = (void *)(uintptr_t)a[0];
    uint64_t flags = (uint64_t)a[3];
    int spawn = a[4] == GUEST_JOBT_SPAWN_TASK;
    void *parent = a[6] ? (void *)(uintptr_t)a[6] : guest_fibers[0].words;
    /* Queued callbacks may outlive their submitter. Root-owned code stays
     * live for this compiler session; child-owned code would need pinning. */
    if (!guest_task_bound || code_owner(callback) != &code_heap || a[2] != 0 ||
        (spawn ? (flags != GUEST_JOBF_ADD_TO_QUE || a[7] < 0 ||
                  (uint64_t)a[7] > GUEST_STACK_MAX_BYTES ||
                  !valid_guest_task(parent))
               : (a[4] != GUEST_JOBT_CALL ||
                  (flags & ~(uint64_t)GUEST_JOBF_FREE_ON_COMPLETE) ||
                  a[5] || a[6] || a[7])))
        return 0;
    size_t name_len = 0;
    const char *name = (const char *)(uintptr_t)a[5];
    if (spawn && name) {
        while (name_len <= 4096 && name[name_len])
            name_len++;
        if (name_len > 4096)
            zc_fail("SpawnQueue name exceeds 4096 bytes");
    }
    if (guest_job_count == GUEST_JOBS_MAX)
        zc_fail("guest job queue full");
    struct guest_fiber *server = ensure_guest_executive();
    struct guest_job *job = __AIWNIOS_CAlloc(sizeof(*job), &data_heap);
    job->ctrl = (unsigned char *)server->words + guest_server_ctrl_off;
    job->job_code = a[4];
    job->flags = (int64_t)flags;
    job->addr = callback;
    job->fun_arg = (void *)(uintptr_t)a[1];
    if (spawn) {
        if (name) {
            job->aux_str = __AIWNIOS_CAlloc(name_len + 1, &data_heap);
            memcpy(job->aux_str, name, name_len);
        }
        job->aux1 = (int64_t)(uintptr_t)parent;
        job->aux2 = a[7]; /* SpawnQueue's stack_size travels in CJob.aux2. */
    }
    guest_job_insert(guest_job_head(server), job);
    guest_job_track(job);
    *guest_flags(server) &= ~(GUEST_IDLE | GUEST_AWAITING_MESSAGE);
    return (int64_t)(uintptr_t)job;
}
static int64_t host_spawn_queue(int64_t *a) {
    if (guest_current == GUEST_EXECUTIVE)
        return 0; /* Waiting on this executive would deadlock. */
    int64_t queued[8] = {a[0], a[1], a[3], a[6], GUEST_JOBT_SPAWN_TASK,
                         a[2], a[4], a[5]};
    struct guest_job *job = (void *)(uintptr_t)host_job_queue(queued);
    if (!job)
        return 0;
    struct guest_fiber *self = &guest_fibers[guest_current];
    self->wait_target = job;
    *guest_flags(self) |= GUEST_IDLE;
    (void)guest_wait(GUEST_WAIT_JOB, &self->wait_target, 0);
    *guest_flags(self) &= ~GUEST_IDLE;
    job = guest_job_find(job);
    if (!job || !(job->flags & GUEST_JOBF_DONE))
        zc_fail("SpawnQueue request ended before completion");
    void *created = job->spawned_task;
    guest_job_unlink(job);
    guest_job_delete(job);
    return (int64_t)(uintptr_t)created;
}
static int64_t host_job_res_scan(int64_t *a) {
    void *request = (void *)(uintptr_t)a[0];
    struct guest_job *job = NULL;
    if (request) {
        job = guest_job_find(request);
        if (job && !(job->flags & GUEST_JOBF_DONE))
            job = NULL;
    } else {
        struct guest_job *head = guest_done_head(&guest_fibers[guest_current]);
        if (head->next != head)
            job = head->next;
    }
    if (!job) {
        if (a[1]) *(int64_t *)(uintptr_t)a[1] = 0;
        return 0;
    }
    int64_t result = job->res;
    guest_job_unlink(job);
    guest_job_delete(job);
    if (a[1]) *(int64_t *)(uintptr_t)a[1] = result;
    return 1;
}
static int64_t host_job_res_get(int64_t *a) {
    void *request = (void *)(uintptr_t)a[0];
    struct guest_fiber *self = &guest_fibers[guest_current];
    for (;;) {
        struct guest_job *job = request ? guest_job_find(request) : NULL;
        struct guest_job *head = guest_done_head(self);
        if (request ? (job && (job->flags & GUEST_JOBF_DONE)) : head->next != head)
            break;
        if (request && !job)
            zc_fail("job request ended before completion");
        if (guest_current == GUEST_EXECUTIVE)
            zc_fail("executive cannot wait for its own job");
        self->wait_target = request;
        self->wait_kind = GUEST_WAIT_JOB;
        if (!guest_current && !next_guest(0)) {
            if (!guest_timer_pending() && !focused_key_wait_pending())
                zc_fail("job wait cannot progress");
            __asm__ volatile("yield");
        }
        host_task_yield(NULL);
        self->wait_kind = GUEST_WAIT_NONE;
    }
    int64_t result = 0;
    int64_t scan[2] = {(int64_t)(uintptr_t)request, (int64_t)(uintptr_t)&result};
    if (!host_job_res_scan(scan))
        zc_fail("job result vanished");
    return result;
}
static int64_t host_jobs_handler_mode(int64_t *a, int system_only) {
    void *task = a[1] ? (void *)(uintptr_t)a[1] : guest_fibers[guest_current].words;
    if (task != guest_fibers[guest_current].words)
        zc_fail("JobsHandler requires the current task");
    if ((guest_task_exe_active &&
         !(system_only && guest_current == GUEST_SYSTEM)) ||
        (guest_job_call_active && guest_current == GUEST_EXECUTIVE))
        zc_fail("job dispatch during compilation or executive callback is unsupported");
    struct guest_fiber *server = &guest_fibers[guest_current];
    unsigned depth = 0;
    for (struct guest_active_job *active = server->active_jobs; active;
         active = active->previous)
        if (++depth == GUEST_JOB_DEPTH_MAX)
            zc_fail("nested job dispatch depth exceeded");
    struct guest_job *head = guest_job_head(server);
    int64_t count = 0;
    while (head->next != head) {
        struct guest_job *job = head->next;
        /* Nested system requests bypass unrelated jobs/messages while their
         * enclosing source body yields. Normal server dispatch keeps FIFO.
         * Root-only sessions retain their UI-message bypass for Sys. */
        if (system_only || (server == &guest_fibers[0] &&
                            job->job_code == GUEST_JOBT_MESSAGE)) {
            while (job != head && !(job->flags & GUEST_JOBF_SYSTEM_REQUEST))
                job = job->next;
            if (job == head)
                break;
        }
        if (job->job_code != GUEST_JOBT_EXE_STR && job->job_code != GUEST_JOBT_CALL &&
            job->job_code != GUEST_JOBT_SPAWN_TASK)
            break;
        if (count == GUEST_JOBS_MAX)
            zc_fail("JobsHandler dispatch budget exceeded");
        guest_job_unlink(job);
        struct guest_active_job active = {server->active_jobs, job};
        server->active_jobs = &active;
        job->flags |= GUEST_JOBF_DISPATCHED;
        *guest_flags(server) &= ~GUEST_IDLE;
        struct dispatch_scope dispatch;
        jmp_buf landing;
        dispatch_scope_enter(&dispatch, &landing);
        int task_exe_active_before = guest_task_exe_active;
        unsigned source_job_active_before = server->source_job_active;
        int raised = setjmp(landing);
        int service_exit = server == &guest_fibers[GUEST_SYSTEM] &&
                           (job->flags & GUEST_JOBF_EXIT_ON_COMPLETE);
        if (raised || service_exit) {
            job->res = 0;
        } else if (job->job_code == GUEST_JOBT_EXE_STR) {
            server->source_job_active++;
            guest_task_exe_active = 1;
            job->res = execute_source_result("<TaskExe>", job->aux_str, 1, 1);
            guest_task_exe_active = 0;
            server->source_job_active--;
        } else if (job->job_code == GUEST_JOBT_CALL) {
            guest_job_call_active = 1;
            job->res = FFI_CALL_TOS_1(job->addr, (int64_t)(uintptr_t)job->fun_arg);
            guest_job_call_active = 0;
        } else {
            struct guest_fiber *child = valid_guest_task((void *)(uintptr_t)job->aux1)
                ? new_guest_fiber(job->addr, job->fun_arg,
                    (void *)(uintptr_t)job->aux1,
                    job->aux_str ? job->aux_str : "Unnamed", GUEST_ENTRY_SPAWN,
                    job->aux2 ? (size_t)job->aux2 : GUEST_STACK_BYTES)
                : NULL;
            job->spawned_task = child ? child->words : NULL;
        }
        /* A source job may call Sys recursively on the dedicated System
         * task. Its nested request has its own dispatch scope, so restore the
         * enclosing job's state after a normal return or exception unwind. */
        guest_task_exe_active = task_exe_active_before;
        server->source_job_active = source_job_active_before;
        guest_unwind_leave(&dispatch.unwind);
        if (raised)
            guest_unwind_handled(&dispatch.unwind, dispatch.unwind.exception);
        server->active_jobs = active.previous;
        int exit_on_complete = !!(job->flags & GUEST_JOBF_EXIT_ON_COMPLETE);
        struct guest_fiber *master = find_guest_task(job->master_task);
        /* Pinned JobRunOne restores focus after dispatch, unless the master
         * has WIf_SELF_FOCUS set in win_inhibit. */
        if (master && (job->flags & GUEST_JOBF_FOCUS_MASTER) &&
            !(*(uint32_t *)((unsigned char *)master->words + guest_win_inhibit_off) & 1u))
            guest_focus_task = master->words;
        /* Pinned TaskExe suspends its master for WAKE_MASTER. Native
         * TaskExe waits on the job instead; completing either form must
         * make that same master runnable again. */
        if (master && (job->flags & GUEST_JOBF_WAKE_MASTER))
            *guest_flags(master) &= ~GUEST_SUSPENDED;
        if ((job->flags & GUEST_JOBF_FREE_ON_COMPLETE) ||
            (job->master_task && !master)) {
            guest_job_delete(job);
        } else {
            struct guest_fiber *owner = master ? master : server;
            job->flags |= GUEST_JOBF_DONE;
            guest_job_insert(guest_done_head(owner), job);
        }
        count++;
        if (exit_on_complete && !service_exit)
            end_guest_current(0);
    }
    return count;
}
static int64_t host_jobs_handler(int64_t *a) {
    return host_jobs_handler_mode(a, 0);
}
static int64_t host_server_jobs_handler(int64_t *a) {
    struct guest_unwind flags;
    guest_unwind_enter(&flags, guest_unwind_flags, NULL);
    (void)host_rflags_set(a);
    int64_t args[2] = {a[0], 0};
    int64_t result = host_jobs_handler_mode(args, 0);
    guest_unwind_leave(&flags);
    return result;
}
static void guest_executive_loop(void) {
    for (;;) {
        int64_t args[2] = {0, 0};
        (void)host_jobs_handler(args);
        struct guest_fiber *self = &guest_fibers[guest_current];
        *guest_flags(self) |= GUEST_IDLE | GUEST_AWAITING_MESSAGE;
        (void)host_task_yield(NULL);
    }
}
static int64_t host_task_run(int64_t *a) {
    (void)a;
    if (!guest_task_bound || guest_current)
        return -1;
    unsigned before = guest_completed;
    for (;;) {
        unsigned next = next_guest(0);
        if (!next) {
            if (guest_timer_pending()) {
                __asm__ volatile("yield");
                continue;
            }
            for (unsigned i = 1; i < GUEST_APP_TASKS; i++)
                if (guest_fibers[i].state == GUEST_RUNNABLE)
                    return -2; /* Live tasks are suspended or waiting. */
            return (int64_t)(guest_completed - before);
        }
        host_task_yield(NULL);
    }
}
static int64_t host_task_result(int64_t *a) {
    unsigned i = (unsigned)a[0];
    if (a[0] <= 0 || i >= GUEST_TASKS || guest_fibers[i].state != GUEST_DONE)
        return -1;
    return guest_fibers[i].result;
}
static unsigned char *bit_byte(int64_t *a) {
    return (unsigned char *)(uintptr_t)a[0] + (a[1] >> 3);
}
static int64_t host_bt(int64_t *a) {
    unsigned char mask = (unsigned char)(1u << ((uint64_t)a[1] & 7));
    return !!(__atomic_load_n(bit_byte(a), __ATOMIC_SEQ_CST) & mask);
}
static int64_t host_lbts(int64_t *a) {
    unsigned char mask = (unsigned char)(1u << ((uint64_t)a[1] & 7));
    return !!(__atomic_fetch_or(bit_byte(a), mask, __ATOMIC_SEQ_CST) & mask);
}
static int64_t host_lbtr(int64_t *a) {
    unsigned char mask = (unsigned char)(1u << ((uint64_t)a[1] & 7));
    return !!(__atomic_fetch_and(bit_byte(a), (unsigned char)~mask, __ATOMIC_SEQ_CST) & mask);
}
static int64_t host_lbtc(int64_t *a) {
    unsigned char mask = (unsigned char)(1u << ((uint64_t)a[1] & 7));
    return !!(__atomic_fetch_xor(bit_byte(a), mask, __ATOMIC_SEQ_CST) & mask);
}
static int64_t host_bts(int64_t *a) {
    /* Non-locked Bts; same bit math as LBts on this single-CPU bridge. */
    return host_lbts(a);
}
static int64_t host_btr(int64_t *a) {
    return host_lbtr(a);
}
static int64_t host_btc(int64_t *a) {
    return host_lbtc(a);
}
static int64_t host_bsf(int64_t *a) {
    uint64_t v = (uint64_t)a[0];
    return v ? (int64_t)__builtin_ctzll(v) : -1;
}
static int64_t host_bsr(int64_t *a) {
    uint64_t v = (uint64_t)a[0];
    return v ? (int64_t)(63 - __builtin_clzll(v)) : -1;
}
static int64_t host_sqr_i64(int64_t *a) {
    int64_t i = a[0];
    return i * i;
}
static int64_t host_dist_sqr_i64(int64_t *a) {
    /* Graphics coordinates are framebuffer-bounded in this port. Keep the
     * calculation in unsigned space to avoid C signed-overflow UB. */
    int64_t dx, dy;
    if (__builtin_sub_overflow(a[0], a[2], &dx) ||
        __builtin_sub_overflow(a[1], a[3], &dy))
        return INT64_MAX;
    uint64_t ax = dx < 0 ? (uint64_t)(-(dx + 1)) + 1 : (uint64_t)dx;
    uint64_t ay = dy < 0 ? (uint64_t)(-(dy + 1)) + 1 : (uint64_t)dy;
    if (ax && ax > UINT32_MAX) return INT64_MAX;
    if (ay && ay > UINT32_MAX) return INT64_MAX;
    uint64_t xx = ax * ax, yy = ay * ay;
    if (xx > INT64_MAX - yy) return INT64_MAX;
    return (int64_t)(xx + yy);
}
static int64_t host_clamp_i64(int64_t *a) {
    return a[0] < a[1] ? a[1] : a[0] > a[2] ? a[2] : a[0];
}
static int64_t host_max_i64(int64_t *a) {
    return a[0] > a[1] ? a[0] : a[1];
}
static int64_t host_swap_u16(int64_t *a) {
    uint16_t *x = (uint16_t *)(uintptr_t)a[0];
    uint16_t *y = (uint16_t *)(uintptr_t)a[1];
    if (x && y) {
        uint16_t value = *x;
        *x = *y;
        *y = value;
    }
    return 0;
}
static int64_t host_endian_u64(int64_t *a) {
    uint64_t v = (uint64_t)a[0];
    v = ((v & 0x00000000FFFFFFFFull) << 32) | ((v & 0xFFFFFFFF00000000ull) >> 32);
    v = ((v & 0x0000FFFF0000FFFFull) << 16) | ((v & 0xFFFF0000FFFF0000ull) >> 16);
    v = ((v & 0x00FF00FF00FF00FFull) << 8) | ((v & 0xFF00FF00FF00FF00ull) >> 8);
    return (int64_t)v;
}
static int64_t host_lbequal(int64_t *a) {
    unsigned char mask = (unsigned char)(1u << ((uint64_t)a[1] & 7));
    if (a[2])
        return !!(__atomic_fetch_or(bit_byte(a), mask, __ATOMIC_SEQ_CST) & mask);
    return !!(__atomic_fetch_and(bit_byte(a), (unsigned char)~mask,
                                 __ATOMIC_SEQ_CST) & mask);
}
static int64_t host_bequal(int64_t *a) {
    /* Non-locked BEqual; same bit math as LBEqual on this single-CPU bridge. */
    return host_lbequal(a);
}
/* The x86 RFLAGS interrupt-enable bit is the only flag the cooperative
 * ARM64 task bridge can preserve. Do not expose ARM DAIF bits as x86 flags. */
static int64_t host_rflags_get(int64_t *a) {
    uint64_t daif;
    (void)a;
    __asm__ volatile("mrs %0, daif" : "=r"(daif) :: "memory");
    return 2 | ((daif & (1u << 7)) ? 0 : (1u << 9));
}
static int64_t host_rflags_set(int64_t *a) {
    if (a[0] & (1u << 9))
        __asm__ volatile("msr daifclr, #2" ::: "memory");
    else
        __asm__ volatile("msr daifset, #2" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
    return 0;
}
static int64_t host_rflags_push(int64_t *a) {
    (void)a;
    if (!guest_task_bound)
        zc_fail("RFlagsPush requires a guest task");
    struct guest_fiber *f = &guest_fibers[guest_current];
    if (f->irq_flags_depth == 8)
        zc_fail("RFlagsPush stack full");
    f->irq_flags_stack[f->irq_flags_depth++] = (uint64_t)host_rflags_get(NULL);
    return 0;
}
static int64_t host_rflags_pop(int64_t *a) {
    (void)a;
    if (!guest_task_bound || !guest_fibers[guest_current].irq_flags_depth)
        zc_fail("RFlagsPop without RFlagsPush");
    struct guest_fiber *f = &guest_fibers[guest_current];
    int64_t flags = (int64_t)f->irq_flags_stack[--f->irq_flags_depth];
    return host_rflags_set(&flags);
}
static int64_t host_tsc(int64_t *a) {
    uint64_t ticks;
    (void)a;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(ticks));
    return (int64_t)ticks;
}
static uint64_t counter_now(void) {
    uint64_t ticks;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(ticks));
    return ticks;
}
static uint64_t deadline_after_milliseconds(uint64_t milliseconds) {
    uint64_t frequency, seconds, ticks, remainder, fractional, per_ms;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    if (!frequency)
        zc_fail("generic timer frequency unavailable");
    seconds = milliseconds / 1000;
    remainder = milliseconds % 1000;
    if (seconds > (uint64_t)INT64_MAX / frequency)
        zc_fail("timed wait duration too long");
    ticks = seconds * frequency;
    per_ms = frequency / 1000;
    if (remainder && per_ms > (UINT64_MAX - ticks) / remainder)
        zc_fail("timed wait duration too long");
    fractional = per_ms * remainder +
        (frequency % 1000) * remainder / 1000;
    if (fractional > (uint64_t)INT64_MAX - ticks)
        zc_fail("timed wait duration too long");
    ticks += fractional;
    if (!ticks)
        ticks = 1;
    return counter_now() + ticks;
}
static int64_t host_monotonic_ns(int64_t *a) {
    (void)a;
    uint64_t frequency;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    if (!frequency)
        zc_fail("generic timer frequency unavailable");
    uint64_t ticks = counter_now();
    uint64_t seconds = ticks / frequency;
    uint64_t remainder = ticks % frequency;
    if (seconds > (uint64_t)INT64_MAX / 1000000000ULL)
        zc_fail("monotonic clock overflow");
    uint64_t ns = seconds * 1000000000ULL +
        remainder * 1000000000ULL / frequency;
    if (ns > (uint64_t)INT64_MAX)
        zc_fail("monotonic clock overflow");
    return (int64_t)ns;
}
static int64_t host_timer_seconds_bits(int64_t *a) {
    double seconds = (double)host_monotonic_ns(a) * 0.000000001;
    uint64_t bits;
    memcpy(&bits, &seconds, sizeof(bits));
    guest_tS_bits = bits;
    return (int64_t)bits;
}
static int64_t host_time_cal(int64_t *a) {
    (void)a;
    /* Source TimeCal updates both the millisecond jiffy count and F64 tS. */
    (void)host_timer_seconds_bits(NULL);
    guest_counts.jiffies = host_monotonic_ns(NULL) / 1000000;
    return guest_counts.jiffies;
}
static int64_t host_fifo_i64_flush(int64_t *a) {
    /* Pinned CFifoI64 is {buf, mask, in_ptr, out_ptr}. Upstream flushes by
     * advancing the consumer to the producer; a null FIFO is a valid empty
     * keyboard state during early bring-up. */
    int64_t *fifo = (int64_t *)(uintptr_t)a[0];
    if (fifo)
        fifo[3] = fifo[2];
    return 0;
}
static int64_t host_sleep_until(int64_t *a) {
    uint64_t frequency;
    uint64_t milliseconds, seconds, ticks, deadline;
    int64_t now_jiffies = host_monotonic_ns(NULL) / 1000000;
    guest_counts.jiffies = now_jiffies;
    if (a[0] <= now_jiffies) {
        /* Real ZealOS can preempt a late window-manager refresh. This
         * cooperative scheduler must still offer a task slice when a frame
         * has already missed its deadline, or WinMgr can render forever
         * without letting the shell or input-owning terminal run. */
        if (guest_task_bound && guest_current && !guest_task_frame_active) {
            struct guest_fiber *self = &guest_fibers[guest_current];
            uint32_t was_idle = *guest_flags(self) & GUEST_IDLE;
            *guest_flags(self) |= GUEST_IDLE;
            (void)host_task_yield(NULL);
            *guest_flags(self) = (*guest_flags(self) & ~GUEST_IDLE) | was_idle;
        }
        return 0;
    }
    milliseconds = (uint64_t)(a[0] - now_jiffies);
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    if (!frequency)
        zc_fail("generic timer frequency unavailable");
    seconds = milliseconds / 1000;
    if (seconds > (uint64_t)INT64_MAX / frequency)
        zc_fail("SleepUntil deadline too far away");
    ticks = seconds * frequency + (milliseconds % 1000) * frequency / 1000;
    if (ticks > (uint64_t)INT64_MAX)
        zc_fail("SleepUntil deadline too far away");
    deadline = counter_now() + ticks;
    if (guest_task_exe_active) {
        /* Source compilation cannot yield while the shared compiler is active. */
        while ((int64_t)(counter_now() - deadline) < 0)
            __asm__ volatile("yield");
    } else {
        struct guest_fiber *self = &guest_fibers[guest_current];
        uint32_t was_idle = *guest_flags(self) & GUEST_IDLE;
        *guest_flags(self) |= GUEST_IDLE;
        self->wait_deadline = deadline;
        self->wait_kind = GUEST_WAIT_TIMER;
        while (!wait_ready(self)) {
            (void)host_task_yield(NULL);
            guest_counts.jiffies = host_monotonic_ns(NULL) / 1000000;
            __asm__ volatile("yield");
        }
        self->wait_kind = GUEST_WAIT_NONE;
        *guest_flags(self) = (*guest_flags(self) & ~GUEST_IDLE) | was_idle;
    }
    guest_counts.jiffies = host_monotonic_ns(NULL) / 1000000;
    return 0;
}
static int64_t host_sleep(int64_t *a) {
    if (a[0] <= 0)
        return guest_task_exe_active
            ? 0 : host_task_yield(NULL);
    uint64_t frequency;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    if (!frequency)
        zc_fail("generic timer frequency unavailable");
    uint64_t milliseconds = (uint64_t)a[0];
    uint64_t seconds = milliseconds / 1000;
    if (seconds > (uint64_t)INT64_MAX / frequency)
        zc_fail("Sleep duration too long");
    uint64_t ticks = seconds * frequency +
        (milliseconds % 1000) * frequency / 1000;
    if (ticks > (uint64_t)INT64_MAX)
        zc_fail("Sleep duration too long");
    if (!ticks)
        ticks = 1;
    uint64_t deadline = counter_now() + ticks;
    if (guest_task_exe_active) {
        /* The shared compiler cannot yield while dispatching a job. Keep
         * Sleep usable there, but run this interval without task switches. */
        while ((int64_t)(counter_now() - deadline) < 0)
            __asm__ volatile("yield");
        return 0;
    }
    struct guest_fiber *self = &guest_fibers[guest_current];
    uint32_t was_idle = *guest_flags(self) & GUEST_IDLE;
    *guest_flags(self) |= GUEST_IDLE;
    self->wait_deadline = deadline;
    self->wait_kind = GUEST_WAIT_TIMER;
    while (!wait_ready(self)) {
        (void)host_task_yield(NULL);
        __asm__ volatile("yield");
    }
    self->wait_kind = GUEST_WAIT_NONE;
    *guest_flags(self) = (*guest_flags(self) & ~GUEST_IDLE) | was_idle;
    return 0;
}
static uint8_t zeal_answer_type(int64_t raw_type) {
    switch (raw_type) {
    case RT_I8i: return 4;
    case RT_U8i: return 5;
    case RT_I16i: return 6;
    case RT_U16i: return 7;
    case RT_I32i: return 8;
    case RT_U32i: return 9;
    case RT_I64i: case RT_PTR: return 10;
    case RT_U64i: return 11;
    case RT_F64: return 14;
    default: return 3; /* ZealOS RT_U0 for statements without a value. */
    }
}
static int64_t host_to_f64(int64_t *a) {
    double value = (double)a[0];
    int64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}
static int64_t host_to_i64(int64_t *a) {
    double value;
    memcpy(&value, &a[0], sizeof(value));
    if (!(value >= -0x1p63 && value < 0x1p63))
        zc_fail("ToI64 argument is not a finite I64 value");
    return (int64_t)value;
}
static int64_t host_abs_i64(int64_t *a) {
    /* KernelB declares AbsI64 as _intern IC_ABS_I64; Aiwnios has no that opcode. */
    return a[0] < 0 ? -a[0] : a[0];
}
static int64_t host_abs_f64(int64_t *a) {
    /* Preserve NaN payloads while applying IEEE-754 absolute value. */
    return (int64_t)((uint64_t)a[0] & UINT64_C(0x7fffffffffffffff));
}
static int64_t host_to_bool(int64_t *a) {
    /* KernelB declares ToBool as _intern IC_TO_BOOL; bind host until call sites
     * lower to the IC_TO_BOOL opcode. */
    return a[0] != 0;
}
static int64_t host_to_upper(int64_t *a) {
    int64_t ch = a[0] & 0xff;
    if (ch >= 'a' && ch <= 'z')
        ch = ch - 'a' + 'A';
    return ch;
}
static int64_t host_ctrl_alt_cb_set(int64_t *a) {
    /* Startup can register window hotkeys before the native key-device
     * dispatcher exists. Registration is intentionally inert at this stage. */
    (void)a;
    return 0;
}
static int task_field(CHashClass *cls, const char *name, int64_t size, int64_t expected_off) {
    CMemberLst *m = MemberFind((char *)name, cls);
    int64_t count = m && m->dim.total_cnt > 0 ? m->dim.total_cnt : 1;
    return m && m->member_class && m->member_class->sz > 0 &&
           m->member_class->sz <= size && m->member_class->sz * count == size &&
           m->off >= 0 && m->off <= cls->sz - size &&
           (expected_off < 0 || m->off == expected_off);
}
static void bind_guest_task(void) {
    CHashClass *cls = (CHashClass *)HashFind("CTask", Fs->hash_table, HTT_CLASS, 1);
    CHashClass *heap = (CHashClass *)HashFind("CHeapCtrl", Fs->hash_table, HTT_CLASS, 1);
    CHashClass *ctrl = (CHashClass *)HashFind("CJobCtrl", Fs->hash_table, HTT_CLASS, 1);
    CHashClass *job = (CHashClass *)HashFind("CJob", Fs->hash_table, HTT_CLASS, 1);
    CHashClass *counts = (CHashClass *)HashFind("CCountsGlobals", Fs->hash_table, HTT_CLASS, 1);
    CHashClass *cc = (CHashClass *)HashFind("CCompCtrl", Fs->hash_table, HTT_CLASS, 1);
    CHashClass *lf = (CHashClass *)HashFind("CLexFile", Fs->hash_table, HTT_CLASS, 1);
    CHashClass *fun_cls = (CHashClass *)HashFind("CHashFun", Fs->hash_table, HTT_CLASS, 1);
    CHashFun *fun = (CHashFun *)HashFind("Fs", Fs->hash_table, HTT_FUN, 1);
    if (!cls || !heap || !ctrl || !job || !counts || !cc || !lf || !fun_cls || !fun || !fun->import_name || strcmp(fun->import_name, "IC_FS") ||
        fun->argc || fun->return_class != cls + 1 ||
        cls->sz <= 0 || cls->sz > (int64_t)sizeof(guest_fibers[0].words) ||
        heap->sz <= 0 || heap->sz > (int64_t)sizeof(guest_fibers[0].heap_words) ||
        !task_field(cls, "addr", 8, 0) ||
        !task_field(cls, "task_signature", 4, 8) ||
        !task_field(cls, "win_inhibit", 4, 12) ||
        !task_field(cls, "task_flags", 4, 24) ||
        !task_field(cls, "display_flags", 4, 28) ||
        !task_field(cls, "rand_seed", 8, -1) ||
        !task_field(cls, "parent_task", 8, -1) ||
        !task_field(cls, "gs", 8, -1) ||
        !task_field(cls, "next_task", 8, -1) ||
        !task_field(cls, "last_task", 8, -1) ||
        !task_field(cls, "next_sibling_task", 8, -1) ||
        !task_field(cls, "last_sibling_task", 8, -1) ||
        !task_field(cls, "next_child_task", 8, -1) ||
        !task_field(cls, "last_child_task", 8, -1) ||
        !task_field(cls, "next_ctrl", 8, -1) ||
        !task_field(cls, "last_ctrl", 8, -1) ||
        !task_field(cls, "popup_task", 8, -1) ||
        !task_field(cls, "last_input_filter_task", 8, -1) ||
        !task_field(cls, "next_input_filter_task", 8, -1) ||
        !task_field(cls, "next_ode", 8, -1) ||
        !task_field(cls, "last_ode", 8, -1) ||
        !task_field(cls, "server_ctrl", 40, -1) ||
        !task_field(cls, "task_num", 8, -1) ||
        !task_field(cls, "task_name", 32, -1) ||
        !task_field(cls, "win_left", 8, -1) ||
        !task_field(cls, "win_right", 8, -1) ||
        !task_field(cls, "win_top", 8, -1) ||
        !task_field(cls, "win_bottom", 8, -1) ||
        !task_field(cls, "text_attr", 1, -1) ||
        !task_field(cls, "code_heap", 8, -1) ||
        !task_field(cls, "data_heap", 8, -1) ||
        !task_field(cls, "answer", 8, -1) ||
        !task_field(cls, "answer_type", 1, -1) ||
        !task_field(cls, "answer_time", 8, -1) ||
        !task_field(cls, "new_answer", 1, -1) ||
        !task_field(cls, "task_end_cb", 8, -1) ||
        !task_field(cls, "except_ch", 8, -1) ||
        !task_field(cls, "catch_except", 1, -1) ||
        !task_field(cls, "hash_table", 8, -1) ||
        !task_field(cls, "cur_dir", 8, -1) ||
        !task_field(cls, "next_cc", 8, -1) ||
        !task_field(cls, "last_cc", 8, -1) ||
        !task_field(cls, "last_fun", 8, -1) ||
        !task_field(heap, "hc_signature", 4, 8) ||
        !task_field(heap, "used_u8s", 8, -1) ||
        !task_field(heap, "mem_task", 8, -1))
        zc_fail("pinned task/heap layout or Fs signature mismatch");
    if (cc->sz < 16 || cc->sz > 4096 || lf->sz < 8 || lf->sz > 256 ||
        !task_field(cc, "next", 8, 0) || !task_field(cc, "last", 8, 8) ||
        !task_field(cc, "flags", 8, -1) || !task_field(cc, "opts", 8, -1) ||
        !task_field(cc, "lex_include_stack", 8, -1) ||
        !task_field(cc, "error_count", 8, -1) ||
        !task_field(cc, "token", 8, -1) ||
        !task_field(lf, "next", 8, 0) || !task_field(lf, "buf", 8, -1) ||
        !task_field(lf, "buf_ptr", 8, -1) || !task_field(lf, "line_num", 8, -1) ||
        !task_field(lf, "flags", 8, -1) || !task_field(lf, "full_name", 8, -1) ||
        !task_field(lf, "depth", 4, -1) || !task_field(lf, "doc", 8, -1) ||
        !task_field(lf, "cur_entry", 8, -1))
        zc_fail("pinned compiler context layout mismatch");
    guest_next_cc_off = MemberFind("next_cc", cls)->off;
    guest_last_cc_off = MemberFind("last_cc", cls)->off;
    if (fun_cls->sz < 16 || fun_cls->sz > 1024 ||
        !task_field(fun_cls, "str", 8, -1) ||
        !task_field(fun_cls, "type", 4, -1) ||
        !task_field(fun_cls, "flags", 2, -1) ||
        !task_field(fun_cls, "arg_count", 4, -1) ||
        !task_field(fun_cls, "exe_addr", 8, -1))
        zc_fail("pinned last-function layout mismatch");
    guest_last_fun_off = MemberFind("last_fun", cls)->off;
    guest_fun_size = fun_cls->sz;
    guest_fun_name_off = MemberFind("str", fun_cls)->off;
    guest_fun_type_off = MemberFind("type", fun_cls)->off;
    guest_fun_flags_off = MemberFind("flags", fun_cls)->off;
    guest_fun_argc_off = MemberFind("arg_count", fun_cls)->off;
    guest_fun_code_off = MemberFind("exe_addr", fun_cls)->off;
    if (guest_last_cc_off != guest_next_cc_off + 8)
        zc_fail("pinned compiler queue sentinel layout mismatch");
    guest_cc_size = cc->sz;
    guest_cc_flags_off = MemberFind("flags", cc)->off;
    guest_cc_opts_off = MemberFind("opts", cc)->off;
    guest_cc_files_off = MemberFind("lex_include_stack", cc)->off;
    guest_cc_errors_off = MemberFind("error_count", cc)->off;
    guest_cc_token_off = MemberFind("token", cc)->off;
    guest_lex_size = lf->sz;
    guest_lex_buf_off = MemberFind("buf", lf)->off;
    guest_lex_ptr_off = MemberFind("buf_ptr", lf)->off;
    guest_lex_line_off = MemberFind("line_num", lf)->off;
    guest_lex_flags_off = MemberFind("flags", lf)->off;
    guest_lex_name_off = MemberFind("full_name", lf)->off;
    guest_lex_depth_off = MemberFind("depth", lf)->off;
    if (!task_field(lf, "line_start", 8, -1))
        zc_fail("pinned lexer line-start layout mismatch");
    guest_lex_start_off = MemberFind("line_start", lf)->off;
    guest_lex_doc_off = MemberFind("doc", lf)->off;
    guest_lex_entry_off = MemberFind("cur_entry", lf)->off;
    if (ctrl->sz != 40 || !task_field(ctrl, "next_waiting", 8, 0) ||
        !task_field(ctrl, "last_waiting", 8, 8) ||
        !task_field(ctrl, "next_done", 8, 16) ||
        !task_field(ctrl, "last_done", 8, 24))
        zc_fail("pinned job control layout mismatch");
    if (job->sz != sizeof(struct guest_job) ||
        !task_field(job, "next", 8, 0) || !task_field(job, "last", 8, 8) ||
        !task_field(job, "ctrl", 8, 16) || !task_field(job, "job_code", 8, 24) ||
        !task_field(job, "flags", 8, 32) || !task_field(job, "message_code", 8, 40) ||
        !task_field(job, "aux_str", 8, 64) ||
        !task_field(job, "aux1", 8, 72) || !task_field(job, "aux2", 8, 80) ||
        !task_field(job, "res", 8, 88) ||
        !task_field(job, "spawned_task", 8, 96) ||
        !task_field(job, "master_task", 8, 104))
        zc_fail("pinned message job layout mismatch");
    if (counts->sz < 41 || counts->sz > (int64_t)sizeof(guest_counts) ||
        !task_field(counts, "time_stamp_freq", 8, 16) ||
        !task_field(counts, "time_stamp_freq_initial", 8, 32) ||
        !task_field(counts, "time_stamp_calibrated", 1, 40))
        zc_fail("pinned counts layout mismatch");
    guest_parent_off = MemberFind("parent_task", cls)->off;
    guest_gs_off = MemberFind("gs", cls)->off;
    guest_next_task_off = MemberFind("next_task", cls)->off;
    guest_last_task_off = MemberFind("last_task", cls)->off;
    guest_next_sibling_off = MemberFind("next_sibling_task", cls)->off;
    guest_last_sibling_off = MemberFind("last_sibling_task", cls)->off;
    guest_next_child_off = MemberFind("next_child_task", cls)->off;
    guest_last_child_off = MemberFind("last_child_task", cls)->off;
    guest_next_ctrl_off = MemberFind("next_ctrl", cls)->off;
    guest_last_ctrl_off = MemberFind("last_ctrl", cls)->off;
    if (guest_next_child_off < guest_next_sibling_off ||
        guest_last_child_off - guest_next_child_off !=
        guest_last_sibling_off - guest_next_sibling_off ||
        guest_last_sibling_off - guest_next_sibling_off != 8)
        zc_fail("pinned task child sentinel layout mismatch");
    guest_popup_off = MemberFind("popup_task", cls)->off;
    guest_input_filter_off = MemberFind("last_input_filter_task", cls)->off;
    guest_next_filter_off = MemberFind("next_input_filter_task", cls)->off;
    guest_next_ode_off = MemberFind("next_ode", cls)->off;
    guest_last_ode_off = MemberFind("last_ode", cls)->off;
    guest_server_ctrl_off = MemberFind("server_ctrl", cls)->off;
    guest_number_off = MemberFind("task_num", cls)->off;
    guest_name_off = MemberFind("task_name", cls)->off;
    guest_flags_off = MemberFind("task_flags", cls)->off;
    guest_display_flags_off = MemberFind("display_flags", cls)->off;
    guest_win_left_off = MemberFind("win_left", cls)->off;
    guest_win_right_off = MemberFind("win_right", cls)->off;
    guest_win_top_off = MemberFind("win_top", cls)->off;
    guest_win_bottom_off = MemberFind("win_bottom", cls)->off;
    guest_text_attr_off = MemberFind("text_attr", cls)->off;
    guest_win_inhibit_off = MemberFind("win_inhibit", cls)->off;
    guest_data_heap_off = MemberFind("data_heap", cls)->off;
    guest_code_heap_off = MemberFind("code_heap", cls)->off;
    guest_answer_off = MemberFind("answer", cls)->off;
    guest_answer_type_off = MemberFind("answer_type", cls)->off;
    guest_answer_time_off = MemberFind("answer_time", cls)->off;
    guest_new_answer_off = MemberFind("new_answer", cls)->off;
    guest_end_cb_off = MemberFind("task_end_cb", cls)->off;
    guest_except_ch_off = MemberFind("except_ch", cls)->off;
    guest_catch_except_off = MemberFind("catch_except", cls)->off;
    guest_hash_table_off = MemberFind("hash_table", cls)->off;
    guest_cur_dir_off = MemberFind("cur_dir", cls)->off;
    guest_heap_sig_off = MemberFind("hc_signature", heap)->off;
    guest_heap_used_off = MemberFind("used_u8s", heap)->off;
    guest_heap_task_off = MemberFind("mem_task", heap)->off;
    memset(guest_fibers, 0, sizeof(guest_fibers));
    memset(guest_progresses, 0, sizeof(guest_progresses));
    memset(guest_progress_t0, 0, sizeof(guest_progress_t0));
    guest_current = guest_completed = 0;
    guest_task_serial = 0;
    guest_job_count = 0;
    guest_task_exe_active = 0;
    guest_job_call_active = 0;
    guest_focus_task = NULL;
    uint64_t frequency;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    if (!frequency || frequency > INT64_MAX)
        zc_fail("generic timer frequency unavailable");
    guest_counts.time_stamp_freq = (int64_t)frequency;
    guest_counts.time_stamp_kHz_freq = (int64_t)(frequency / 1000);
    guest_counts.time_stamp_freq_initial = (int64_t)frequency;
    guest_counts.time_stamp_calibrated = 1;
    init_guest_fiber(0, NULL, "Root");
    set_guest_task(guest_fibers[0].words);
    guest_task_bound = 1;
    PrsBindCSymbol("Fs", host_fs, 0);
    PrsBindCSymbol("Gs", host_gs, 0);
    PrsBindCSymbol("mp_count", &guest_mp_count, 0);
    PrsBindCSymbol("sys_focus_task", &guest_focus_task, 0);
    PrsBindCSymbol("sys_progresses", guest_progresses, 0);
    PrsBindCSymbol("progress1_t0", &guest_progress_t0[0], 0);
    PrsBindCSymbol("progress2_t0", &guest_progress_t0[1], 0);
    PrsBindCSymbol("progress3_t0", &guest_progress_t0[2], 0);
    PrsBindCSymbol("progress4_t0", &guest_progress_t0[3], 0);
    PrsBindCSymbol("Bt", host_bt, 2);
    PrsBindCSymbol("Bts", host_bts, 2);
    PrsBindCSymbol("Btr", host_btr, 2);
    PrsBindCSymbol("Btc", host_btc, 2);
    PrsBindCSymbol("Bsf", host_bsf, 1);
    PrsBindCSymbol("Bsr", host_bsr, 1);
    PrsBindCSymbol("SqrI64", host_sqr_i64, 1);
    PrsBindCSymbol("EndianU64", host_endian_u64, 1);
    PrsBindCSymbol("StrLen", host_str_len, 1);
    PrsBindCSymbol("LBts", host_lbts, 2);
    PrsBindCSymbol("LBtr", host_lbtr, 2);
    PrsBindCSymbol("LBtc", host_lbtc, 2);
    PrsBindCSymbol("LBEqual", host_lbequal, 3);
    PrsBindCSymbol("BEqual", host_bequal, 3);
    PrsBindCSymbol("QueueInit", host_queue_init, 1);
    PrsBindCSymbol("QueueInsert", host_queue_insert, 2);
    PrsBindCSymbol("QueueInsertRev", host_queue_insert_rev, 2);
    PrsBindCSymbol("QueueRemove", host_queue_remove, 1);
    PrsBindCSymbol("RFlagsGet", host_rflags_get, 0);
    PrsBindCSymbol("RFlagsSet", host_rflags_set, 1);
    PrsBindCSymbol("TSCGet", host_tsc, 0);
    PrsBindCSymbol("ZcMonotonicNs", host_monotonic_ns, 0);
    PrsBindCSymbol("ToF64", host_to_f64, 1);
    PrsBindCSymbol("ToI64", host_to_i64, 1);
    PrsBindCSymbol("AbsI64", host_abs_i64, 1);
    PrsBindCSymbol("Abs", host_abs_f64, 1);
    PrsBindCSymbol("D3Add", host_d3_add, 3);
    PrsBindCSymbol("D3AddEqu", host_d3_add_equ, 2);
    PrsBindCSymbol("D3Copy", host_d3_copy, 2);
    PrsBindCSymbol("D3Cross", host_d3_cross, 3);
    PrsBindCSymbol("D3Dist", host_d3_dist, 2);
    PrsBindCSymbol("D3DistSqr", host_d3_dist_sqr, 2);
    PrsBindCSymbol("D3Div", host_d3_div, 3);
    PrsBindCSymbol("D3DivEqu", host_d3_div_equ, 2);
    PrsBindCSymbol("D3Dot", host_d3_dot, 2);
    PrsBindCSymbol("D3Equ", host_d3_equ, 4);
    PrsBindCSymbol("D3Mul", host_d3_mul, 3);
    PrsBindCSymbol("D3MulEqu", host_d3_mul_equ, 2);
    PrsBindCSymbol("D3Norm", host_d3_norm, 1);
    PrsBindCSymbol("D3NormSqr", host_d3_norm_sqr, 1);
    PrsBindCSymbol("D3Sub", host_d3_sub, 3);
    PrsBindCSymbol("D3SubEqu", host_d3_sub_equ, 2);
    PrsBindCSymbol("D3Unit", host_d3_unit, 1);
    PrsBindCSymbol("D3Zero", host_d3_zero, 1);
    PrsBindCSymbol("ToBool", host_to_bool, 1);
    PrsBindCSymbol("ToUpper", host_to_upper, 1);
    PrsBindCSymbol("CtrlAltCBSet", host_ctrl_alt_cb_set, 5);
    PrsBindCSymbol("SwapI64", host_swap, 2);
    PrsBindCSymbol("sys_semas", guest_sys_semas, 0);
}

/* Keep the last compiled function in the calling guest task, as COCCompile
 * does upstream. Aiwnios CHashFun is a different layout: publish a stable,
 * task-owned pinned-size view with the fields needed by LastFun and callers
 * inspecting its name/arity/code. The remaining compiler metadata is not
 * implemented by this view and must not point at incompatible native data. */
static void guest_function_complete(CHashFun *fun) {
    if (!guest_task_bound)
        return;
    struct guest_fiber *f = &guest_fibers[guest_current];
    unsigned char *mirror = __AIWNIOS_CAlloc(guest_fun_size, &f->heap_owner);
    *(char **)(mirror + guest_fun_name_off) = fun->base.base.str;
    *(uint32_t *)(mirror + guest_fun_type_off) = 0x40; /* pinned HTT_FUN */
    *(uint16_t *)(mirror + guest_fun_flags_off) =
        (fun->base.flags & CLSF_VARGS) ? 1u << 14 : 0;
    *(uint32_t *)(mirror + guest_fun_argc_off) = (uint32_t)fun->argc;
    *(void **)(mirror + guest_fun_code_off) = fun->fun_ptr;
    *(void **)((unsigned char *)f->words + guest_last_fun_off) = mirror;
}
static int64_t host_last_fun(int64_t *a) {
    if (!guest_task_bound)
        return 0;
    unsigned char *fun = *(unsigned char **)(
        (unsigned char *)guest_fibers[guest_current].words + guest_last_fun_off);
    if (!fun)
        return 0;
    void *entry = *(void **)(fun + guest_fun_code_off);
    if (!entry)
        return 0;
    if (a[0] < 0 || a[0] > 1024 || (a[0] && !a[1])) {
        zc_output("LastFun requires 0..1024 raw argument words\n");
        return 0;
    }
    /* Like the pinned assembly, this forwards raw words, not type-converted
     * values or default arguments. Variadic entries take explicit argc/argv
     * words. The caller must match the compiled function's ABI. Aiwnios also
     * returns F64 bits in X0, preserving LastFun's I64 return contract. */
    return TempleOS_CallN((void (*)(void))entry, a[0], (int64_t *)(uintptr_t)a[1]);
}

/* Guest CCompCtrl objects retain the pinned layout, while the native parser
 * owns its separate CCmpCtrl. Only the queue, flags, opts, error count and
 * include-file metadata are exposed here; native IR/hash fields are not cast
 * to the incompatible upstream compiler structures. */
struct source_context {
    struct source_context *previous;
    unsigned char *mirror;
    unsigned char *option_target;
    int owned;
};
static void **user_doc_state(CLexFile *file);
static unsigned char *guest_context_new(int64_t flags) {
    struct guest_fiber *f = &guest_fibers[guest_current];
    unsigned char *head = (unsigned char *)f->words + guest_next_cc_off;
    unsigned char *tail = *(unsigned char **)(head + 8);
    unsigned char *cc = __AIWNIOS_CAlloc(guest_cc_size, &f->heap_owner);
    *(void **)cc = head;
    *(void **)(cc + 8) = tail;
    *(void **)tail = cc;
    *(void **)(head + 8) = cc;
    *(int64_t *)(cc + guest_cc_flags_off) = flags | 0x008;
    /* CompCtrlNew's defaults, inherited only from this task's outer cc. */
    *(int64_t *)(cc + guest_cc_opts_off) = tail == head ? (1 << 2) | (1 << 5)
        : *(int64_t *)(tail + guest_cc_opts_off);
    return cc;
}
static void *guest_prompt_context(void) {
    if (!guest_task_bound)
        return NULL;
    struct guest_fiber *f = &guest_fibers[guest_current];
    if (!f->prompt_cc)
        f->prompt_cc = guest_context_new(0x001 | 0x002 | 0x004);
    return f->prompt_cc;
}
static void guest_context_files_clear(unsigned char *cc) {
    unsigned char **head = (unsigned char **)(cc + guest_cc_files_off);
    while (*head) {
        unsigned char *file = *head;
        *head = *(unsigned char **)file;
        free(*(void **)(file + guest_lex_name_off));
        free(*(void **)(file + guest_lex_start_off));
        free(file);
    }
}
static void guest_context_sync(CLexer *lex) {
    if (!guest_task_bound || !guest_fibers[guest_current].source_context)
        return;
    unsigned char *cc = guest_fibers[guest_current].source_context->mirror;
    guest_context_files_clear(cc);
    unsigned char **next = (unsigned char **)(cc + guest_cc_files_off);
    int depth = -1;
    for (CLexFile *file = lex->file; file; file = file->last)
        depth += !!file->is_file;
    for (CLexFile *file = lex->file; file; file = file->last) {
        unsigned char *mirror = __AIWNIOS_CAlloc(guest_lex_size,
            &guest_fibers[guest_current].heap_owner);
        *(void **)(mirror + guest_lex_buf_off) = file->text;
        *(void **)(mirror + guest_lex_ptr_off) = file->text + (file->port_read ? 0 : file->pos);
        *(int64_t *)(mirror + guest_lex_line_off) = file->ln + 1;
        *(int64_t *)(mirror + guest_lex_flags_off) = file->port_read ? 1 : file->is_file ? 0 : 2;
        if (file->port_read) {
            /* The reader's private data starts with its original document
             * and current entry, used by ExeCmdLine's source-aware title. */
            void **input = user_doc_state(file);
            *(void **)(mirror + guest_lex_doc_off) = input[0];
            *(void **)(mirror + guest_lex_entry_off) = input[1];
        }
        *(void **)(mirror + guest_lex_name_off) =
            __AIWNIOS_StrDup(file->filename, &guest_fibers[guest_current].heap_owner);
        *(int32_t *)(mirror + guest_lex_depth_off) = depth < 0 ? 0 : depth;
        /* ExeCmdLine copies STR_LEN-1 bytes for the title, even for a short
         * final line. Give it padded task-owned storage, never an unchecked
         * pointer into the lexer's possibly tiny input allocation. */
        char *line = __AIWNIOS_CAlloc(STR_LEN, &guest_fibers[guest_current].heap_owner);
        size_t start = file->port_read ? 0 : file->pos;
        while (start && file->text[start - 1] != '\n') start--;
        for (size_t i = 0; i < STR_LEN - 1 && file->text[start + i] &&
                           file->text[start + i] != '\n'; i++)
            line[i] = file->text[start + i];
        *(void **)(mirror + guest_lex_start_off) = line;
        depth -= !!file->is_file;
        *next = mirror;
        next = (unsigned char **)mirror;
    }
}
static void guest_context_enter(struct source_context *context, CLexer *lex,
                                int interactive, int64_t flags) {
    memset(context, 0, sizeof(*context));
    if (!guest_task_bound)
        return;
    struct guest_fiber *f = &guest_fibers[guest_current];
    context->previous = f->source_context;
    context->owned = !interactive || context->previous != NULL || f->user_compilers != NULL;
    /* Legacy `zc` commands entered by the line editor are statements of the
     * same interactive session. Their temporary compiler inherits its flags
     * and exports committed option changes back to the waiting source cc. */
    if (interactive && !context->previous && f->user_compilers) {
        unsigned char *head = (unsigned char *)f->words + guest_next_cc_off;
        unsigned char *tail = *(unsigned char **)(head + 8);
        if (tail != head) context->option_target = tail;
    }
    context->mirror = context->owned ? guest_context_new(flags) : guest_prompt_context();
    f->source_context = context;
    lex->port_options = (int64_t *)(context->mirror + guest_cc_opts_off);
    lex->port_sync = guest_context_sync;
    guest_context_sync(lex);
}
static void guest_context_leave(struct source_context *context) {
    if (!context->mirror)
        return;
    struct guest_fiber *f = &guest_fibers[guest_current];
    if (context->option_target)
        *(int64_t *)(context->option_target + guest_cc_opts_off) =
            *(int64_t *)(context->mirror + guest_cc_opts_off);
    guest_context_files_clear(context->mirror);
    if (context->owned) {
        unsigned char *next = *(unsigned char **)context->mirror;
        unsigned char *last = *(unsigned char **)(context->mirror + 8);
        *(void **)last = next;
        *(void **)(next + 8) = last;
        free(context->mirror);
    }
    f->source_context = context->previous;
}

/* Parsing/emission cannot yield. Keep a statement checkpoint outside the
 * managed heaps: parser failures can leave half-built hash entries, changed
 * existing externs and freed/reused allocations. Restoring both arenas also
 * restores those links and import patches. Commit before running emitted
 * code, so prior statements and other tasks' runtime work are never undone.
 * This costs 24 MiB of fixed recovery storage; boot module loading does not
 * take checkpoints. Only one parser can own this buffer on CPU 0. */
static unsigned char compiler_saved_data[DATA_BYTES] __attribute__((aligned(16)));
static unsigned char compiler_saved_code[CODE_BYTES] __attribute__((aligned(16)));
static CTask compiler_saved_task;
static int compiler_saved_switch_depth, compiler_saved_fun_depth;
static size_t compiler_saved_bytes[2];
static struct block *compiler_saved_free_heads[2];
static uintptr_t compiler_saved_sp;
static void *compiler_saved_last_fun;
static struct guest_exception *compiler_saved_except_top, *compiler_saved_except_caught;
static int64_t compiler_saved_except_ch;
static uint8_t compiler_saved_catch_except;
static int64_t compiler_saved_heap_used[3 + 3 * GUEST_TASKS];
static CHeapCtrl *compiler_checkpoint_heap(unsigned i) {
    if (i == 0) return &data_heap;
    if (i == 1) return &code_heap;
    if (i == 2) return &guest_stack_heap;
    i -= 3;
    struct guest_fiber *f = &guest_fibers[i / 3];
    if (i % 3 == 0) return &f->heap_owner;
    if (i % 3 == 1) return &f->compiler_heap;
    return &f->compiler_code_heap;
}
static void compiler_checkpoint_save(void) {
    __asm__ volatile("mov %0, sp" : "=r"(compiler_saved_sp));
    for (unsigned a = 0; a < 2; a++) {
        compiler_saved_free_heads[a] = arena_free_heads[a];
        size_t span = 0;
        for (struct block *b = arenas[a]; b; b = b->next)
            span = (unsigned char *)(b + 1) - (unsigned char *)arenas[a] +
                   (b->used ? b->size : 0);
        /* Include every existing block header and live payload. The last
         * free block's unused tail cannot affect the restored allocation
         * chain, so copying those bytes serves no recovery purpose. */
        compiler_saved_bytes[a] = span;
        memcpy(a ? compiler_saved_code : compiler_saved_data, arenas[a], span);
    }
    memcpy(&compiler_saved_task, Fs, sizeof(compiler_saved_task));
    compiler_saved_last_fun = guest_task_bound
        ? *(void **)((unsigned char *)guest_fibers[guest_current].words + guest_last_fun_off)
        : NULL;
    if (guest_task_bound) {
        struct guest_fiber *f = &guest_fibers[guest_current];
        compiler_saved_except_top = f->except_top;
        compiler_saved_except_caught = f->except_caught;
        compiler_saved_except_ch = *(int64_t *)((unsigned char *)f->words + guest_except_ch_off);
        compiler_saved_catch_except = *(uint8_t *)((unsigned char *)f->words + guest_catch_except_off);
    }
    compiler_saved_switch_depth = aiwnios_switch_depth;
    compiler_saved_fun_depth = aiwnios_fun_depth;
    for (unsigned i = 0; i < 3 + 3 * GUEST_TASKS; i++)
        compiler_saved_heap_used[i] = compiler_checkpoint_heap(i)->used_u8s;
}
static void compiler_checkpoint_restore(void) {
    size_t copied = 0;
    /* A custom fiber stack is allocated in the data arena. Restoring its
     * payload would overwrite live native/JIT frames, including this one.
     * Walk the saved headers, retaining stack contents while restoring all
     * allocator metadata and compiler/application allocations around them. */
    for (struct block *b = (struct block *)compiler_saved_data; b;) {
        size_t offset = (unsigned char *)b - compiler_saved_data;
        uintptr_t payload_address = (uintptr_t)data_arena + offset + sizeof(*b);
        int active_stack = compiler_saved_sp >= payload_address &&
                           compiler_saved_sp - payload_address < b->size;
        if (b->used && (b->owner == &guest_stack_heap || active_stack)) {
            size_t payload = offset + sizeof(*b);
            memcpy(data_arena + copied, compiler_saved_data + copied,
                   payload - copied);
            copied = payload + b->size;
        }
        b = b->next ? (struct block *)(compiler_saved_data +
             ((unsigned char *)b->next - data_arena)) : NULL;
    }
    memcpy(data_arena + copied, compiler_saved_data + copied,
           compiler_saved_bytes[0] - copied);
    memcpy(code_arena, compiler_saved_code, compiler_saved_bytes[1]);
    arena_free_heads[0] = compiler_saved_free_heads[0];
    arena_free_heads[1] = compiler_saved_free_heads[1];
    memcpy(Fs, &compiler_saved_task, sizeof(compiler_saved_task));
    if (guest_task_bound)
        *(void **)((unsigned char *)guest_fibers[guest_current].words + guest_last_fun_off) =
            compiler_saved_last_fun;
    if (guest_task_bound) {
        struct guest_fiber *f = &guest_fibers[guest_current];
        f->except_top = compiler_saved_except_top;
        f->except_caught = compiler_saved_except_caught;
        *(int64_t *)((unsigned char *)f->words + guest_except_ch_off) = compiler_saved_except_ch;
        *(uint8_t *)((unsigned char *)f->words + guest_catch_except_off) = compiler_saved_catch_except;
    }
    aiwnios_switch_depth = compiler_saved_switch_depth;
    aiwnios_fun_depth = compiler_saved_fun_depth;
    for (unsigned i = 0; i < 3 + 3 * GUEST_TASKS; i++) {
        CHeapCtrl *h = compiler_checkpoint_heap(i);
        h->used_u8s = compiler_saved_heap_used[i];
        sync_guest_heap(h);
    }
    __clear_cache(code_arena, code_arena + sizeof(code_arena));
}
static void compiler_source_cleanup(CCmpCtrl *cc, CLexer *lex) {
    CodeCtrlPop(cc);
    CmpCtrlDel(cc);
    /* Includes and macro expansions can belong to Fs's heap rather than
     * lex->hc. A failed parse has not consumed their normal EOF cleanup. */
    while (lex->file) {
        CLexFile *file = lex->file;
        lex->file = file->last;
        free(file->text);
        free(file->dir);
        free(file->filename);
        free(file);
    }
    LexerDel(lex);
}
struct source_execution {
    struct guest_unwind unwind;
    CLexer *lex;
    CCmpCtrl *cc;
    struct source_context context;
    char *volatile code;
    int previous_compiling;
    int registered;
};
static void source_execution_cleanup(struct guest_unwind *scope) {
    struct source_execution *execution = (struct source_execution *)scope;
    guest_task_exe_active = execution->previous_compiling;
    free(execution->code);
    guest_context_leave(&execution->context);
    compiler_source_cleanup(execution->cc, execution->lex);
}
static void source_execution_leave(struct source_execution *execution) {
    if (execution->registered)
        guest_unwind_leave(&execution->unwind);
    else
        source_execution_cleanup(&execution->unwind);
}
/* recover: 0 = boot/fatal parser, 1 = ExePutS Compiler/Break catch,
 *          2 = ExePutS2, restoring native parser state before guest rethrow. */
static int64_t execute_source_mode(const char *path, const char *src,
                                   int return_expr, int record_answer,
                                   const char *initial_dir, int64_t guest_flags, int just_load,
                                   int recover, int *error) {
    CLexer *lex = LexerNew((char *)path, (char *)src);
    lex->file->is_file = 1;
    char dir[128];
    if (normal_path(path, dir))
        zc_fail("source path too long");
    char *slash = strrchr(dir, '/');
    if (slash)
        *slash = 0;
    else
        strcpy(dir, !strncmp(path, "disk:", 5) ? "disk:" : ".");
    if (initial_dir || path[0] == '<') {
        const char *source_dir = initial_dir ? initial_dir : guest_current_dir();
        if (strlen(source_dir) >= sizeof(dir))
            zc_fail("current directory is too long");
        strcpy(dir, source_dir);
    }
    lex->file->dir = __AIWNIOS_StrDup(dir, NULL);
    /* ExePutS and interactive text execute Cd between statements. Actual
     * include files retain their own source directory in the lexer stack. */
    if (initial_dir || path[0] == '<') {
        lex->file->port_dir = guest_current_dir;
        lex->port_defer_statement_lex = 1;
    }
    CCmpCtrl *cc = CmpCtrlNew(lex);
    cc->flags |= CCF_STRINGS_ON_HEAP;
    CodeCtrlPush(cc);
    struct source_execution execution = {
        .lex = lex, .cc = cc, .previous_compiling = guest_task_exe_active,
        .registered = guest_task_bound
    };
    guest_context_enter(&execution.context, lex, !strcmp(path, "<shell>"), guest_flags);
    lex->port_fun_complete = guest_function_complete;
    int previous_compiling = guest_task_exe_active;
    int started = 0;
    int64_t result = 0;
    if (error)
        *error = 0;
    jmp_buf landing;
    if (execution.registered) {
        guest_unwind_enter(&execution.unwind, source_execution_cleanup, &landing);
        if (setjmp(landing)) {
            int64_t code = execution.unwind.exception;
            lex->port_statement_depth = 0;
            lex->port_lex_pending = 0;
            source_execution_leave(&execution);
            guest_unwind_flags(&execution.unwind);
            if (recover == 1 && (code == GUEST_EXCEPT_COMPILER || code == GUEST_EXCEPT_BREAK)) {
                guest_unwind_handled(&execution.unwind, code);
                if (error) *error = 1;
                return 0;
            }
            guest_raise(code);
        }
    }
    for (;;) {
        if (recover) {
            compiler_checkpoint_save();
            if (setjmp(compiler_recovery_guard)) {
                compiler_recovery_active = 0;
                lex->port_statement_depth = 0;
                lex->port_lex_pending = 0;
                compiler_checkpoint_restore();
                guest_task_exe_active = previous_compiling;
                int64_t code = compiler_recovery_exception;
                if (execution.context.mirror)
                    (*(int64_t *)(execution.context.mirror + guest_cc_errors_off))++;
                source_execution_leave(&execution);
                if (execution.registered)
                    guest_unwind_flags(&execution.unwind);
                compiler_recovery_errors++;
                if (error)
                    *error = 1;
                zc_output("zc: compile failed; current statement discarded\n");
                if (recover == 2 || (code != GUEST_EXCEPT_COMPILER && code != GUEST_EXCEPT_BREAK))
                    guest_raise(code);
                if (execution.registered)
                    guest_unwind_handled(&execution.unwind, code);
                return 0;
            }
            compiler_recovery_active = 1;
        }
        guest_task_exe_active = 1;
        if (!started) {
            Lex(lex);
            started = 1;
        } else if (lex->port_lex_pending) {
            lex->port_lex_pending = 0;
            Lex(lex);
        }
        if (!PrsStmt(cc)) {
            if (lex->cur_tok) {
                printf("zc: unconsumed token=%ld at %s:%ld:%ld name=%s\n",
                       (long)lex->cur_tok, lex->file ? lex->file->filename : "<eof>",
                       lex->file ? (long)lex->file->ln + 1 : 0,
                       lex->file ? (long)lex->file->col + 1 : 0, lex->string);
                AIWNIOS_throw(0);
            }
            compiler_recovery_active = 0;
            guest_task_exe_active = previous_compiling;
            break;
        }
        CRPN *first = (CRPN *)cc->code_ctrl->ir_code->next;
        int has_statement = first != (CRPN *)cc->code_ctrl->ir_code &&
                            first->type != IC_NOP;
        int explicit_return = first != (CRPN *)cc->code_ctrl->ir_code &&
                              first->type == IC_RET;
        int64_t raw_type = RT_U0;
        if ((record_answer || return_expr) && first != (CRPN *)cc->code_ctrl->ir_code) {
            if (first->type == IC_RET)
                first = (CRPN *)first->base.next;
            if (first != (CRPN *)cc->code_ctrl->ir_code && first->type != IC_NOP)
                raw_type = AssignRawTypeToNode(cc, first);
        }
        /* The Aiwnios file loader discards each top-level expression. ExePrint
         * returns its last value, so give each TaskExe statement a return.
         * Definition-only and control-flow statements supply no expression
         * operand. Wrapping a final IC_LABEL in IC_RET consumes that label,
         * leaving its branches unbound. Explicit returns already have RET. */
        if (return_expr && !explicit_return && raw_type > RT_UNDEF) {
            CRPN *ret = __AIWNIOS_CAlloc(sizeof(*ret), NULL);
            ret->type = IC_RET;
            QueIns((CQue *)ret, cc->code_ctrl->ir_code);
        }
        char *code = Compile(cc, NULL, NULL, NULL);
        execution.code = code;
        compiler_recovery_active = 0;
        guest_task_exe_active = previous_compiling;
        if (code) {
            if (lex->port_sync)
                lex->port_sync(lex);
            uint64_t start = record_answer ? counter_now() : 0;
            int source_job = guest_task_bound &&
                             guest_fibers[guest_current].source_job_active;
            if (source_job)
                guest_task_exe_active = 0;
            /* Aiwnios may attach initialization work to an IC_NOP
             * declaration, so still execute it. Its otherwise empty RET and
             * an #include's optional semicolon have no new expression value. */
            if (!just_load) {
                int64_t value = FFI_CALL_TOS_0(code);
                if (!return_expr || has_statement)
                    result = value;
            }
            if (source_job)
                guest_task_exe_active = 1;
            uint64_t end = record_answer ? counter_now() : 0;
            free(code);
            execution.code = NULL;
            if (!just_load && has_statement && record_answer && guest_task_bound) {
                unsigned char *words = (unsigned char *)guest_fibers[guest_current].words;
                uint64_t frequency;
                __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
                double elapsed = frequency ? (double)(end - start) / (double)frequency : 0.0;
                *(int64_t *)(words + guest_answer_off) = result;
                words[guest_answer_type_off] = zeal_answer_type(raw_type);
                memcpy(words + guest_answer_time_off, &elapsed, sizeof(elapsed));
                words[guest_new_answer_off] = 1;
            }
        }
        CodeCtrlPop(cc);
        CodeCtrlPush(cc);
    }
    source_execution_leave(&execution);
    return just_load ? 1 : result;
}
#include "user_compiler.h"

static int64_t execute_source_result(const char *path, const char *src,
                                     int return_expr, int record_answer) {
    return execute_source_mode(path, src, return_expr, record_answer, NULL, 0, 0, 1, NULL);
}
struct source_call_scope {
    struct guest_unwind unwind;
    int previous_compiling, previous_jobs;
};
static void source_call_cleanup(struct guest_unwind *scope) {
    struct source_call_scope *call = (struct source_call_scope *)scope;
    guest_task_exe_active = call->previous_compiling;
    guest_fibers[guest_current].source_job_active = call->previous_jobs;
}
static int64_t host_exe_puts_mode(int64_t *a, int throwing) {
    const char *source = (const char *)(uintptr_t)a[0];
    const char *filename = (const char *)(uintptr_t)a[1];
    char path[134];
    /* CMain's ExeFile wrapper supplies a short #include string. File contents
     * are read by the lexer and retain their actual include directories. The
     * wrapper's filename is a diagnostic label; its initial include resolves
     * from the calling task's directory, rather than that label's parent. */
    const int64_t supported = 0x001 | 0x008 | 0x040;
    if (!guest_task_bound || !source)
        zc_fail("ExePutS requires source and a guest task");
    if (a[2] & ~supported)
        zc_fail("ExePutS compiler flags are not supported by the ARM64 bootstrap");
    if (a[3])
        zc_fail("ExePutS hash-table contexts are not supported by the ARM64 bootstrap");
    if (guest_task_exe_active)
        zc_fail("ExePutS during compilation is unsupported");
    size_t length = 0;
    while (length <= 4096 && source[length])
        length++;
    if (length > 4096)
        zc_fail("ExePutS text exceeds 4096 bytes; use ExeFile for source files");
    if (filename && guest_path(filename, path))
        zc_fail("ExePutS filename is invalid");
    struct guest_fiber *f = &guest_fibers[guest_current];
    struct source_call_scope call = {
        .previous_compiling = guest_task_exe_active,
        .previous_jobs = f->source_job_active
    };
    guest_unwind_enter(&call.unwind, source_call_cleanup, NULL);
    f->source_job_active++;
    guest_task_exe_active = 1;
    int64_t result = execute_source_mode(filename ? path : throwing ? "<ExePutS2>" : "<ExePutS>",
        source, 1, 1, guest_current_dir(), a[2], !!(a[2] & 0x040), throwing ? 2 : 1, NULL);
    guest_unwind_leave(&call.unwind);
    return result;
}
static int64_t host_exe_puts(int64_t *a) {
    return host_exe_puts_mode(a, 0);
}
static int64_t host_exe_puts2(int64_t *a) {
    int64_t args[4] = {a[0], a[1], a[2], 0};
    return host_exe_puts_mode(args, 1);
}
static size_t format_text(char *output, size_t capacity, const char *format,
                          int64_t argc, const int64_t *argv, int strip_markers);
static int64_t host_sys(int64_t *a) {
    if (!guest_task_bound ||
        (guest_task_exe_active && guest_current != GUEST_SYSTEM) ||
        (guest_current == GUEST_EXECUTIVE && guest_job_call_active))
        zc_fail("Sys requires a cooperative execution context");
    char source[4097];
    format_text(source, sizeof(source), (const char *)(uintptr_t)a[0],
                a[1], (const int64_t *)(uintptr_t)a[2], 0);
    struct guest_fiber *server = guest_fibers[GUEST_SYSTEM].state == GUEST_RUNNABLE
        ? &guest_fibers[GUEST_SYSTEM] : &guest_fibers[0];
    int self = server == &guest_fibers[guest_current];
    int64_t request[4] = {
        (int64_t)(uintptr_t)server->words,
        (int64_t)(uintptr_t)guest_fibers[guest_current].words,
        (int64_t)(uintptr_t)source,
        self ? 0 : GUEST_JOBF_WAKE_MASTER
    };
    int64_t job = host_task_exe_mode(request, 1);
    if (!job)
        return 0;
    if (self) {
        int64_t handle[2] = {0, 0};
        (void)host_jobs_handler_mode(handle, 1);
    }
    int64_t result = 0, scan[2] = {job, (int64_t)(uintptr_t)&result};
    if (!host_job_res_scan(scan))
        zc_fail("system request did not complete");
    return result;
}
static int64_t host_exe_print_mode(int64_t *a, int throwing) {
    char source[4097];
    format_text(source, sizeof(source), (const char *)(uintptr_t)a[0],
                a[1], (const int64_t *)(uintptr_t)a[2], 0);
    int64_t args[4] = {(int64_t)(uintptr_t)source, 0, 0, 0};
    return host_exe_puts_mode(args, throwing);
}
static int64_t host_exe_print(int64_t *a) {
    return host_exe_print_mode(a, 0);
}
static int64_t host_exe_print2(int64_t *a) {
    return host_exe_print_mode(a, 1);
}
static int64_t host_print(int64_t *a);
/* Bounded formatter shared by Print, source generation and StrPrintJoin.
 * ExePrint and graphical Print preserve source display markers; StrPrintJoin
 * keeps its text-oriented behavior. Empty generated string statements remain
 * valid source. */
static size_t format_text(char *output, size_t capacity, const char *format,
                          int64_t argc, const int64_t *argv, int strip_markers) {
    size_t used = 0;
    int64_t arg = 0;
    if (!format || argc < 0 || argc > 64 || (argc && !argv))
        zc_fail("format arguments are invalid");
#define FMT_BYTE(ch) do { \
    if (used == capacity - 1) zc_fail("formatted text exceeds buffer"); \
    output[used++] = (char)(ch); \
} while (0)
    for (size_t i = 0;;) {
        if (!format[i]) break;
        if (strip_markers && format[i] == '$' && format[i + 1] == '$') {
            /* Text console has no DolDoc style state; drop $$...$$ markers. */
            i += 2;
            while (format[i] && !(format[i] == '$' && format[i + 1] == '$'))
                i++;
            if (format[i] == '$' && format[i + 1] == '$')
                i += 2;
            continue;
        }
        /* Aiwnios string decoding can normalize doubled DolDoc delimiters
         * from source text to single-dollar tags before calling Print. */
        if (strip_markers && (!strncmp(format + i, "$PT$", 4) ||
            !strncmp(format + i, "$FG$", 4))) {
            i += 4;
            continue;
        }
        if (format[i] != '%') {
            FMT_BYTE(format[i++]);
            continue;
        }
        if (!format[i + 1])
            zc_fail("format ends with percent");
        size_t spec = i + 1;
        int precision = 1;
        int has_precision = 0;
        int left = 0, truncate = 0, escape_dollars = 0;
        size_t width = 0;
        /* ZealOS height repeat, used unchanged by DocType after inserting a
         * file. Repeat the whole packed character value, not just its low
         * byte. Check the total before looping so zero/huge counts are bounded. */
        if (format[spec] == 'h' && format[spec + 1] == '*' &&
            (format[spec + 2] == 'c' || format[spec + 2] == 'C')) {
            if (arg + 2 > argc) zc_fail("format needs repeat and character arguments");
            int64_t count = argv[arg++];
            uint64_t packed = (uint64_t)argv[arg++];
            unsigned length = 0;
            while (length < 8 && (unsigned char)(packed >> (length * 8))) length++;
            if (count > 0 && length) {
                if ((uint64_t)count > (capacity - 1 - used) / length)
                    zc_fail("formatted text exceeds buffer");
                for (int64_t repeat = 0; repeat < count; repeat++)
                    for (unsigned byte = 0; byte < length; byte++)
                        FMT_BYTE(packed >> (byte * 8));
            }
            i = spec + 3;
            continue;
        }
        if (format[spec] == '-') { left = 1; spec++; }
        /* String width/truncation is needed by the original border callbacks.
         * Numeric width remains outside this formatter's current subset. */
        if (format[spec] == '*') {
            if (arg >= argc) zc_fail("format needs a width argument");
            int64_t requested = argv[arg++];
            if (requested < 0) {
                left = 1;
                width = (uint64_t)(-(requested + 1)) + 1;
            } else width = (uint64_t)requested;
            spec++;
        } else while (format[spec] >= '0' && format[spec] <= '9') {
            if (width < capacity) width = width * 10 + (format[spec] - '0');
            spec++;
        }
        if (width >= capacity) zc_fail("format width exceeds buffer");
        if (format[spec] == '.') {
            has_precision = 1;
            precision = 0;
            spec++;
            while (format[spec] >= '0' && format[spec] <= '9') {
                if (precision < 100)
                    precision = precision * 10 + (format[spec] - '0');
                spec++;
            }
        }
        if (format[spec] == 't') { truncate = 1; spec++; }
        /* Aiwnios can normalize %$$Q to %$Q while decoding source strings. */
        if (format[spec] == '$') {
            size_t escaped = spec + 1;
            if (format[escaped] == '$') escaped++;
            if (format[escaped] == 'Q') { spec = escaped; escape_dollars = 1; }
        }
        char code = format[spec];
        if (!code)
            zc_fail("format ends with incomplete conversion");
        i = spec + 1;
        if (code == '%') {
            FMT_BYTE('%');
            continue;
        }
        if (code != 'c' && code != 'C' && code != 's' && code != 'Q' &&
            code != 'S' && code != 'Z' &&
            code != 'd' && code != 'x' && code != 'X' &&
            code != 'f' && code != 'F')
        {
            printf("zc: unsupported format conversion '%c' in %s\n", code, format);
            zc_fail("format code is unsupported");
        }
        if (arg >= argc)
            zc_fail("format needs another argument");
        if (code == 'c' || code == 'C') {
            /* ZealOS %c/%C print the nonzero bytes of a packed I64 value. */
            uint64_t packed = (uint64_t)argv[arg++];
            for (unsigned byte = 0; byte < 8; byte++) {
                unsigned char ch = (unsigned char)(packed >> (byte * 8));
                if (!ch) break;
                FMT_BYTE(ch);
            }
        } else if (code == 's' || code == 'Q' || code == 'S' || code == 'Z') {
            size_t field_start = used;
            const char *s;
            if (code == 'Z') {
                if (arg + 2 > argc) zc_fail("format needs index and define arguments");
                CHashFun *sub = (CHashFun *)HashFind("DefineSub", Fs->hash_table, HTT_FUN, 1);
                if (!sub || !sub->fun_ptr || sub->argc != 2 || (sub->base.base.type & HTF_EXTERN))
                    zc_fail("format %Z requires DefineSub");
                s = (const char *)(uintptr_t)FFI_CALL_TOS_2(sub->fun_ptr, argv[arg], argv[arg + 1]);
                arg += 2;
                if (!s) s = "";
            } else if (code == 'S') {
                CHashFun *define = (CHashFun *)HashFind("Define", Fs->hash_table, HTT_FUN, 1);
                if (!define || !define->fun_ptr || define->argc != 1 ||
                    (define->base.base.type & HTF_EXTERN))
                    zc_fail("format %S requires Define");
                s = (const char *)(uintptr_t)FFI_CALL_TOS_1(define->fun_ptr, argv[arg]);
                arg++;
                if (!s) s = "";
            } else s = (const char *)(uintptr_t)argv[arg++];
            if (!s) zc_fail("format %s argument is null");
            for (size_t j = 0; s[j]; j++) {
                if (truncate && width && used - field_start >= width) break;
                if (code == 'Q') {
                    switch (s[j]) {
                    case '"': FMT_BYTE('\\'); FMT_BYTE('"'); break;
                    case '\\': FMT_BYTE('\\'); FMT_BYTE('\\'); break;
                    case '\n': FMT_BYTE('\\'); FMT_BYTE('n'); break;
                    case '\r': FMT_BYTE('\\'); FMT_BYTE('r'); break;
                    case '\t': FMT_BYTE('\\'); FMT_BYTE('t'); break;
                    case '$':
                        if (escape_dollars) { FMT_BYTE('\\'); FMT_BYTE('x'); FMT_BYTE('2'); FMT_BYTE('4'); }
                        else { FMT_BYTE('$'); FMT_BYTE('$'); }
                        break;
                    default: FMT_BYTE(s[j]); break;
                    }
                } else
                    FMT_BYTE(s[j]);
            }
            size_t length = used - field_start;
            if (truncate && width && length > width) used = field_start + (length = width);
            if (width > length) {
                size_t pad = width - length;
                if (pad > capacity - 1 - used) zc_fail("formatted text exceeds buffer");
                if (!left) {
                    memmove(output + field_start + pad, output + field_start, length);
                    memset(output + field_start, ' ', pad);
                    used += pad;
                } else while (pad--) FMT_BYTE(' ');
            }
        } else if (code == 'f' || code == 'F') {
            char number[64];
            char *end = number;
            hc_fmt_f64_bits(&end, (uint64_t)argv[arg++],
                            has_precision ? precision : 1);
            *end = 0;
            for (const char *p = number; *p; p++)
                FMT_BYTE(*p);
        } else if (code == 'x' || code == 'X') {
            static const char lower_digits[] = "0123456789abcdef";
            static const char upper_digits[] = "0123456789ABCDEF";
            const char *digits = code == 'X' ? upper_digits : lower_digits;
            uint64_t value = (uint64_t)argv[arg++];
            char reversed[16];
            size_t n = 0;
            do {
                reversed[n++] = digits[value & 0xF];
                value >>= 4;
            } while (value && n < sizeof(reversed));
            while (n)
                FMT_BYTE(reversed[--n]);
        } else {
            int64_t value = argv[arg++];
            char digits[32];
            size_t n = 0;
            uint64_t mag;
            if (value < 0) {
                FMT_BYTE('-');
                mag = (uint64_t)(-(value + 1)) + 1;
            } else
                mag = (uint64_t)value;
            do {
                digits[n++] = (char)('0' + (mag % 10));
                mag /= 10;
            } while (mag && n < sizeof(digits));
            while (n)
                FMT_BYTE(digits[--n]);
        }
    }
#undef FMT_BYTE
    output[used] = 0;
    return used;
}
static int64_t host_str_print_join(int64_t *a) {
    char *dst = (char *)(uintptr_t)a[0];
    const char *format = (const char *)(uintptr_t)a[1];
    int64_t argc = a[2];
    const int64_t *argv = (const int64_t *)(uintptr_t)a[3];
    char stack[4097];
    size_t used = format_text(stack, sizeof(stack), format, argc, argv, 1);
    if (!dst) {
        CHeapCtrl *owner = guest_task_bound ? &guest_fibers[guest_current].heap_owner
                                            : &data_heap;
        dst = __AIWNIOS_MAlloc((int64_t)used + 1, owner);
        if (!dst)
            zc_fail("StrPrintJoin allocation failed");
    }
    memcpy(dst, stack, used + 1);
    return (int64_t)(uintptr_t)dst;
}
static int64_t host_print(int64_t *a) {
    const char *format = (const char *)(uintptr_t)a[0];
    int64_t argc = a[1];
    const int64_t *argv = (const int64_t *)(uintptr_t)a[2];
    char output[4097];
    format_text(output, sizeof(output), format, argc, argv, 0);
    zc_output(output);
    guest_task_text_output(output);
    return 0;
}
static int64_t host_call(int64_t *a) {
    if (!a[0])
        return -1;
    return FFI_CALL_TOS_0((void *)(uintptr_t)a[0]);
}
static int64_t host_call_ext_str(int64_t *a) {
    const char *name = (const char *)(uintptr_t)a[0];
    CHashFun *fun;
    if (!name || !Fs || !Fs->hash_table)
        return -1;
    fun = (CHashFun *)HashFind((char *)name, Fs->hash_table, HTT_FUN, 1);
    if (!fun || fun->argc || !fun->fun_ptr || (fun->base.base.type & HTF_EXTERN))
        return -1;
    return FFI_CALL_TOS_0(fun->fun_ptr);
}
static int execute_source(const char *path, const char *src) {
    (void)execute_source_mode(path, src, 0, 0, NULL, 0, 0, 0, NULL);
    return 0;
}
static int load_inner(const char *path);
struct source_load_scope {
    struct guest_unwind unwind;
    char *source;
};
static void source_load_cleanup(struct guest_unwind *scope) {
    free(((struct source_load_scope *)scope)->source);
}
static int load_source_path(const char *path, int merged, int recover) {
    size_t n;
    char *src;
    if (!strncmp(path, "disk:", 5)) {
        src = disk_copy(path, &n, NULL);
    } else {
        char *owned = NULL;
        const unsigned char *s = merged ? source_file_find(path, &n, &owned)
                                        : source_find(path, &n);
        src = owned ? owned : s ? malloc(n + 1) : NULL;
        if (src && !owned) {
            memcpy(src, s, n);
            src[n] = 0;
        }
    }
    if (!src) {
        printf("zc: source not found: %s\n", path);
        return -1;
    }
    struct source_load_scope load = {.source = src};
    int registered = guest_task_bound;
    if (registered)
        guest_unwind_enter(&load.unwind, source_load_cleanup, NULL);
    if (!strcmp(path, "/System/TaskBridge.ZC")) {
        PrsBindCSymbol("ZcSystemTask", host_system_task, 0);
        /* KernelB-only callers need the text declaration before TaskBridge
         * is compiled. Re-declaring it inside the module creates a fresh,
         * unbound external slot before its function bodies are emitted. */
        if (!HashFind("text", Fs->hash_table, HTT_GLBL_VAR, 1))
            execute_source("<TaskBridge text>", "extern CTextGlobals text;");
        guest_text_globals[4] = zeal_fb_screen_height() / 8;
        guest_text_globals[5] = zeal_fb_screen_width() / 8;
        PrsBindCSymbol("text", guest_text_globals, 0);
    }
    int compile_error = 0;
    (void)execute_source_mode(path, src, 0, 0, NULL, 0, 0, recover, &compile_error);
    if (compile_error) {
        if (registered) guest_unwind_leave(&load.unwind);
        else free(src);
        return -1;
    }
    if (!strcmp(path, "/System/TaskBridge.ZC")) {
        CHashFun *recalc = (CHashFun *)HashFind("DocRecalc", Fs->hash_table,
                                               HTT_FUN, 1);
        guest_doc_recalc_stub = recalc ? recalc->fun_ptr : NULL;
        PrsBindCSymbol("ZcDocRecalc", host_doc_recalc, 2);
        /* TaskBridge declares the generic file API before defining DocRead /
         * DocWrite.  Bind it as soon as those declarations exist, just like
         * the bridge helpers below, so calls compiled in TaskBridge resolve. */
        PrsBindCSymbol("FileRead", host_file_read, 3);
        PrsBindCSymbol("FileWrite", host_file_write, 5);
        PrsBindCSymbol("ZcSys", host_sys, 3);
        PrsBindCSymbol("ZcSystemTask", host_system_task, 0);
        /* PopUp's server bridge probes the source-backed desktop loop before
         * UserTask.ZC is loaded, so make dynamic source calls available at
         * the initial TaskBridge load as well. */
        PrsBindCSymbol("ZcCallExtStr", host_call_ext_str, 1);
        PrsBindCSymbol("ZcSourceDirExists", host_file_dir_exists, 1);
        PrsBindCSymbol("ZcSourcePath", host_source_path, 1);
        PrsBindCSymbol("ZcSourceWritable", host_source_writable, 1);
        PrsBindCSymbol("Cd", host_cd, 2);
        PrsBindCSymbol("DirCur", host_dir_cur, 2);
        CHashGlblVar *blkdev = (CHashGlblVar *)HashFind(
            "blkdev", Fs->hash_table, HTT_GLBL_VAR, 1);
        CHashClass *blkdev_class = (CHashClass *)HashFind(
            "CBlkDevGlobals", Fs->hash_table, HTT_CLASS, 1);
        CMemberLst *home = blkdev_class ? MemberFind("home_dir", blkdev_class) : NULL;
        if (!blkdev || !blkdev->data_addr || !home || home->off < 0 ||
            home->off + 8 > blkdev_class->sz)
            zc_fail("pinned home directory globals missing");
        guest_home_dir_ptr = (char **)((unsigned char *)blkdev->data_addr + home->off);
        PrsBindCSymbol("ZcSourceFileAt", host_source_file_at, 4);
        PrsBindCSymbol("LowPass1", host_low_pass1, 4);
        /* These functions were pre-registered before TaskBridge introduced
         * their extern declarations, so bind them again now that the symbols
         * and relocation slots exist. */
        PrsBindCSymbol("ZcMouseStateUpdate", host_mouse_state_update, 5);
        PrsBindCSymbol("ZcMouseButtonsUpdate", host_mouse_buttons_update, 2);
        PrsBindCSymbol("ZcActiveGrGlobals", host_active_gr_globals, 0);
        PrsBindCSymbol("ZcActiveMouseGlobals", host_active_mouse_globals, 0);
        PrsBindCSymbol("ZcActiveWinMgrGlobals", host_active_winmgr_globals, 0);
        PrsBindCSymbol("ZcSetWinMgrTask", host_set_winmgr_task, 1);
        PrsBindCSymbol("ExePrint2", host_exe_print2, 3);
        PrsBindCSymbol("ExePutS", host_exe_puts, 4);
        PrsBindCSymbol("ExePutS2", host_exe_puts2, 3);
        PrsBindCSymbol("LastFun", host_last_fun, 2);
    }
    if (!strcmp(path, "/Kernel/Job.ZC"))
        execute_source("<system execution bridge>",
            "I64 Sys(U8 *format,...){return ZcSys(format,argc,argv);}");
    if (!strcmp(path, "/System/ServerTask.ZC")) {
        PrsBindCSymbol("ZcSystemTaskStart", host_system_task_start, 1);
        PrsBindCSymbol("ZcServerJobsHandler", host_server_jobs_handler, 1);
    }
    if (!strcmp(path, "/System/UserTask.ZC")) {
        PrsBindCSymbol("ZcCallExtStr", host_call_ext_str, 1);
        PrsBindCSymbol("ZcUserCompCtrlNew", host_user_compiler_new, 2);
        PrsBindCSymbol("ZcUserCompCtrlDel", host_user_compiler_del, 1);
        PrsBindCSymbol("ZcUserLex", host_user_lex, 1);
        PrsBindCSymbol("ZcUserStatement2Bin", host_user_statement, 2);
        PrsBindCSymbol("ZcUserCodeFree", host_user_code_free, 2);
        PrsBindCSymbol("ZcUserPromptSet", host_user_prompt_set, 1);
        PrsBindCSymbol("ZcUserOutputSet", host_user_output_set, 1);
        PrsBindCSymbol("ZcUserPutExcept", host_user_put_except, 0);
        PrsBindCSymbol("ZcUserInputComplete", host_user_input_complete, 1);
    }
    if (!strcmp(path, "/System/GuiTerminal.ZC"))
        PrsBindCSymbol("ZcGuiShellExec", host_gui_shell_exec, 1);
    if (!strcmp(path, "/System/DolDoc/DocCompiler.ZC")) {
        CHashClass *input = (CHashClass *)HashFind("CArmDocInput", Fs->hash_table, HTT_CLASS, 1);
        if (!input || !task_field(input, "doc", 8, 0) ||
            !task_field(input, "entry", 8, 8) || !task_field(input, "line", 8, 16))
            zc_fail("private DolDoc input layout mismatch");
        PrsBindCSymbol("ZcUserDocAttach", host_user_doc_attach, 5);
    }
    if (!strcmp(path, "/Kernel/KeyDev.ZC")) {
        CHashFun *put = (CHashFun *)HashFind("PutKey", Fs->hash_table, HTT_FUN, 1);
        if (put && put->fun_ptr && put->argc == 2 &&
            !(put->base.base.type & HTF_EXTERN))
            guest_keydev_put_key = put->fun_ptr;
    }
    if (!strcmp(path, "/Kernel/KTask.ZC")) {
        PrsBindCSymbol("ZcCallExtStr", host_call_ext_str, 1);
        PrsBindCSymbol("ZcTaskValidate", host_task_validate, 1);
        PrsBindCSymbol("ZcTaskKill", host_kill, 3);
        PrsBindCSymbol("ZcSpawn", host_spawn, 7);
    }
    /* Source units can introduce fresh extern declarations after KernelC.HH
     * has been parsed. Rebind the timer hook after each unit so those calls
     * resolve to the same host implementation as the kernel declaration. */
    PrsBindCSymbol("SleepUntil", host_sleep_until, 1);
    PrsBindCSymbol("Clamp", host_clamp_f64, 3);
    PrsBindCSymbol("Max", host_max_f64, 2);
    PrsBindCSymbol("MaxI64", host_max_i64, 2);
    if (!strcmp(path, "/Kernel/KernelC.HH")) {
        PrsBindCSymbol("FileRead", host_file_read, 3);
        PrsBindCSymbol("FileWrite", host_file_write, 5);
        PrsBindCSymbol("Cd", host_cd, 2);
        PrsBindCSymbol("DirCur", host_dir_cur, 2);
        PrsBindCSymbol("Sound", host_sound, 1);
        /* counts is declared here, not in KernelB.HH; bind after declaration
         * so the generic extern-data binder can resolve the symbol. */
        CHashGlblVar *spawned_tasks = (CHashGlblVar *)HashFind(
            "sys_num_spawned_tasks", Fs->hash_table, HTT_GLBL_VAR, 1);
        if (!spawned_tasks)
            zc_fail("KernelC task serial declaration missing");
        PrsBindCSymbol("sys_num_spawned_tasks", &guest_task_serial, 0);
        if (!spawned_tasks->data_addr)
            zc_fail("KernelC task serial binding failed");
        PrsBindCSymbol("counts", &guest_counts, 0);
        PrsBindCSymbol("tS", &guest_tS_bits, 0);
        PrsBindCSymbol("TimeCal", host_time_cal, 0);
        PrsBindCSymbol("FifoI64Flush", host_fifo_i64_flush, 1);
        CHashClass *kbd = (CHashClass *)HashFind("CKbdStateGlobals",
                                                  Fs->hash_table, HTT_CLASS, 1);
        if (!kbd || kbd->sz <= 0 || kbd->sz > (1 << 20))
            zc_fail("invalid CKbdStateGlobals layout");
        guest_kbd_state = calloc(1, kbd->sz);
        if (!guest_kbd_state)
            zc_fail("keyboard compatibility storage allocation failed");
        PrsBindCSymbol("kbd", guest_kbd_state, 0);
        CHashClass *autocomplete = (CHashClass *)HashFind(
            "CAutoCompleteGlobals", Fs->hash_table, HTT_CLASS, 1);
        if (!autocomplete || autocomplete->sz <= 0 || autocomplete->sz > (1 << 20))
            zc_fail("invalid CAutoCompleteGlobals layout");
        guest_autocomplete_state = calloc(1, autocomplete->sz);
        if (!guest_autocomplete_state)
            zc_fail("autocomplete compatibility storage allocation failed");
        PrsBindCSymbol("ac", guest_autocomplete_state, 0);
        PrsBindCSymbol("sys_task_being_screen_updated",
                       &guest_task_being_screen_updated, 0);
        CHashClass *screencast = (CHashClass *)HashFind(
            "CScreenCastGlobals", Fs->hash_table, HTT_CLASS, 1);
        if (!screencast || screencast->sz <= 0 || screencast->sz > (1 << 20))
            zc_fail("invalid CScreenCastGlobals layout");
        guest_screencast_state = calloc(1, screencast->sz);
        if (!guest_screencast_state)
            zc_fail("screencast compatibility storage allocation failed");
        PrsBindCSymbol("screencast", guest_screencast_state, 0);
        PrsBindCSymbol("fp_set_std_palette", &guest_fp_set_std_palette, 0);
        /* KTask's floating-point control hook is optional, matching the
         * upstream null check before dispatch. Keep null storage available
         * to source modules that declare it as extern data. */
        PrsBindCSymbol("fp_update_ctrls", &guest_fp_update_ctrls, 0);
        CHashClass *text = (CHashClass *)HashFind("CTextGlobals", Fs->hash_table,
                                                   HTT_CLASS, 1);
        if (!text || text->sz < 48 ||
            !task_field(text, "rows", 8, 32) ||
            !task_field(text, "cols", 8, 40))
            zc_fail("pinned CTextGlobals layout mismatch");
        guest_text_globals[4] = guest_fb_height / 8;
        guest_text_globals[5] = guest_fb_width / 8;
        init_guest_gr_tables();
        guest_text_globals[6] = (uint64_t)(uintptr_t)guest_gr_font;
        PrsBindCSymbol("text", guest_text_globals, 0);
        /* KernelC redeclares the bootstrap heap calls with ZealOS's real
         * CTask* parameter types and _MALLOC/_FREE import names. Rebind
         * after those declarations so the ARM backend sees callable host
         * implementations instead of unresolved x86 import labels. */
    PrsBindCSymbol("MAlloc", host_malloc, 2);
        PrsBindCSymbol("CAlloc", host_calloc, 2);
        PrsBindCSymbol("Free", host_free, 1);
        PrsBindCSymbol("MSize", host_msize, 1);
        PrsBindCSymbol("MSize2", host_msize, 1);
        /* KernelC declares keyboard shortcut registration after KernelB has
         * bound the guest task bridge. Keep early Win.ZC initialization from
         * dispatching into the unresolved-symbol sentinel. */
        PrsBindCSymbol("CtrlAltCBSet", host_ctrl_alt_cb_set, 5);
        /* SpriteRaster references CTextGlobals.text.font. Load it only after
         * KernelC declares and binds that global. */
        if (load_inner("/System/Gr/SpriteRaster.ZC"))
            zc_fail("sprite raster bridge source missing");
    }
    if (!strcmp(path, "/Kernel/KernelB.HH")) {
        bind_guest_task();
        PrsBindCSymbol("sys_boot_src", &guest_boot_source, 0);
        /* ZealOS's Call invokes a zero-argument JIT entry point. Rebind it
         * after KernelB declares the _CALL import. */
        PrsBindCSymbol("Call", host_call, 1);
        CHashClass *cpu = (CHashClass *)HashFind("CCPU", Fs->hash_table,
                                                  HTT_CLASS, 1);
        if (!cpu || cpu->sz <= 0 || cpu->sz > (1 << 20))
            zc_fail("invalid CCPU layout");
        CMemberLst *cpu_addr = MemberFind("addr", cpu);
        CMemberLst *cpu_num = MemberFind("num", cpu);
        CMemberLst *cpu_exec = MemberFind("executive_task", cpu);
        if (!task_field(cpu, "addr", 8, 0) ||
            !task_field(cpu, "num", 8, 8) ||
            !task_field(cpu, "executive_task", 8, -1) ||
            !task_field(cpu, "idle_task", 8, -1) ||
            !cpu_addr || !cpu_num || !cpu_exec)
            zc_fail("pinned CCPU layout mismatch");
        guest_cpu_structs = calloc(128, cpu->sz);
        if (!guest_cpu_structs)
            zc_fail("CCPU compatibility storage allocation failed");
        *(void **)guest_cpu_structs = guest_cpu_structs;
        *(int64_t *)((unsigned char *)guest_cpu_structs + cpu_num->off) = 0;
        *(void **)((unsigned char *)guest_cpu_structs + cpu_exec->off) =
            guest_fibers[0].words;
        // MultiProc.ZC assigns sys_task->gs = cpu_structs before it spawns
        // normal tasks; KTask.ZC then copies parent->gs into each child.
        *(void **)((unsigned char *)guest_fibers[0].words + guest_gs_off) =
            guest_cpu_structs;
        PrsBindCSymbol("cpu_structs", &guest_cpu_structs, 0);
        PrsBindCSymbol("ext", &guest_ext_table, 0);
        /* KernelB exposes the firmware framebuffer contract to ZealOS source. */
        guest_fb_width = zeal_fb_screen_width();
        guest_fb_height = zeal_fb_screen_height();
        guest_fb_pitch = zeal_fb_pitch_bytes();
        guest_fb_bpp = zeal_fb_bits_per_pixel();
        guest_fb_addr = zeal_fb_address();
        guest_text_globals[3] = guest_fb_addr;
        guest_text_globals[4] = guest_fb_height / 8;
        guest_text_globals[5] = guest_fb_width / 8;
        guest_text_globals[8] = (guest_fb_pitch / 4) * guest_fb_height;
        guest_text_globals[9] = guest_text_globals[8] * sizeof(uint32_t);
        PrsBindCSymbol("sys_framebuffer_addr", &guest_fb_addr, 0);
        PrsBindCSymbol("sys_framebuffer_width", &guest_fb_width, 0);
        PrsBindCSymbol("sys_framebuffer_height", &guest_fb_height, 0);
        PrsBindCSymbol("sys_framebuffer_pitch", &guest_fb_pitch, 0);
        PrsBindCSymbol("sys_framebuffer_bpp", &guest_fb_bpp, 0);
        if (load_inner("/System/TaskBridge.ZC"))
            zc_fail("task bridge source missing");
        PrsBindCSymbol("ZcGrTextUpdate", host_gr_text_update, 0);
        PrsBindCSymbol("ZcWinMouseUpdate", host_win_mouse_update, 0);
        PrsBindCSymbol("ZcGrScreenUpdate", host_gr_screen_update, 0);
        PrsBindCSymbol("ZcGrScreenFullUpdate", host_gr_screen_full_update, 0);
        PrsBindCSymbol("ZcGrUpdateTasks", host_gr_update_tasks, 0);
        PrsBindCSymbol("ZcGrUpdateTaskWin", host_gr_update_task_win, 1);
        PrsBindCSymbol("ZcODEsUpdate", host_ode_update_task, 1);
        PrsBindCSymbol("ZcGrUpdateTaskODEs", host_gr_update_task_odes, 1);
        PrsBindCSymbol("DistSqrI64", host_dist_sqr_i64, 4);
        PrsBindCSymbol("SwapU16", host_swap_u16, 2);
        PrsBindCSymbol("ClampI64", host_clamp_i64, 3);
        PrsBindCSymbol("ZcDefineMirror", host_define_mirror, 2);
        /* The bridge declares counts for KernelB-stage task tests. KernelC
         * later rebinds the same object after its own declaration is loaded. */
        PrsBindCSymbol("counts", &guest_counts, 0);
        PrsBindCSymbol("MemCompare", host_compare, 3);
        PrsBindCSymbol("MemCopy", host_copy, 3);
        PrsBindCSymbol("MemSet", host_set, 3);
        PrsBindCSymbol("MemSetU32", host_memset_u32, 3);
        PrsBindCSymbol("Sqrt", host_sqrt, 1);
        PrsBindCSymbol("Exp", host_exp, 1);
        PrsBindCSymbol("Ln", host_ln, 1);
        PrsBindCSymbol("ZcSinF64", host_sin_f64, 1);
        PrsBindCSymbol("ZcCosF64", host_cos_f64, 1);
        PrsBindCSymbol("ZcMonotonicNs", host_monotonic_ns, 0);
        PrsBindCSymbol("ZcTryEnter", host_sys_try, 0);
        PrsBindCSymbol("ZcTryLeave", host_sys_untry, 0);
        PrsBindCSymbol("ZcTryCatchEnd", host_end_catch, 0);
        PrsBindCSymbolNaked("AIWNIOS_SetJmp", zc_setjmp, 1);
        PrsBindCSymbol("throw", host_guest_throw, 2);
        PrsBindCSymbol("ExePrint", host_exe_print, 3);
        PrsBindCSymbol("ExePrint2", host_exe_print2, 3);
        PrsBindCSymbol("Print", host_print, 3);
        PrsBindCSymbol("ZcGuiShellExec", host_gui_shell_exec, 1);
        PrsBindCSymbol("StrPrintJoin", host_str_print_join, 4);
        PrsBindCSymbol("PutKey", host_put_key, 2);
        PrsBindCSymbol("StrLen", host_str_len, 1);
        PrsBindCSymbol("RFlagsPush", host_rflags_push, 0);
        PrsBindCSymbol("RFlagsPop", host_rflags_pop, 0);
        PrsBindCSymbol("KbdMouseHandler", host_kbd_mouse_handler, 2);
        PrsBindCSymbol("ZcTabletSample", host_zc_tablet_sample, 0);
        PrsBindCSymbol("KbdMessagesQueue", host_kbd_messages_queue, 0);
        PrsBindCSymbol("Spawn", host_spawn, 7);
        PrsBindCSymbol("Yield", host_task_yield, 0);
        PrsBindCSymbol("TaskFocusNext", host_task_focus_next, 0);
        PrsBindCSymbol("TaskValidate", host_task_validate, 1);
        PrsBindCSymbol("Suspend", host_suspend, 2);
        PrsBindCSymbol("IsSuspended", host_is_suspended, 1);
        PrsBindCSymbol("Kill", host_kill, 3);
        PrsBindCSymbol("BirthWait", host_birth_wait, 2);
        PrsBindCSymbol("DeathWait", host_death_wait, 2);
        PrsBindCSymbol("TaskWait", host_task_wait, 2);
        PrsBindCSymbol("ZcTaskWaitTimeout", host_task_wait_timeout, 3);
        PrsBindCSymbol("Sleep", host_sleep, 1);
        PrsBindCSymbol("Exit", host_task_exit, 0);
        PrsBindCSymbol("TaskEndNow", host_task_exit, 0);
        PrsBindCSymbol("TaskMessage", host_task_message, 6);
        PrsBindCSymbol("MessagePost", host_message_post, 5);
        PrsBindCSymbol("MessagePostWait", host_message_post_wait, 5);
        PrsBindCSymbol("Message", host_message_self, 4);
        PrsBindCSymbol("JobDel", host_job_del, 1);
        PrsBindCSymbol("MessageScan", host_message_scan, 4);
        PrsBindCSymbol("MessageGet", host_message_get, 4);
        PrsBindCSymbol("ZcMessageGetTimeout", host_message_get_timeout, 5);
        PrsBindCSymbol("FlushMessages", host_flush_messages, 1);
        PrsBindCSymbol("KeyScan", host_key_scan, 3);
        PrsBindCSymbol("KeyGet", host_key_get, 3);
        PrsBindCSymbol("TaskExe", host_task_exe, 4);
        PrsBindCSymbol("TaskText", host_task_text, 4);
        PrsBindCSymbol("JobQueue", host_job_queue, 8);
        PrsBindCSymbol("SpawnQueue", host_spawn_queue, 7);
        PrsBindCSymbol("JobsHandler", host_jobs_handler, 2);
        PrsBindCSymbol("JobResScan", host_job_res_scan, 2);
        PrsBindCSymbol("JobResGet", host_job_res_get, 1);
        /* ZealOS CHash/CHashTable layout matches Aiwnios; guest DEFINE entries
         * use ZealOS HTT_* bits on the guest Fs->hash_table. */
        PrsBindCSymbol("HashTableNew", host_hash_table_new, 2);
        PrsBindCSymbol("HashTableDel", host_hash_table_del, 1);
        PrsBindCSymbol("HashFind", host_hash_find, 4);
        PrsBindCSymbol("HashAdd", host_hash_add, 2);
    }
    if (registered) guest_unwind_leave(&load.unwind);
    else free(src);
    loaded_modules++;
    printf("zc: loaded %s\n", path);
    return 0;
}
static int load_inner(const char *path) {
    return load_source_path(path, 0, 0);
}
/* Shell commands are the final recoverable guest-exception boundary. Keep
 * the native guard from pointing at an abandoned C stack after a guest throw.
 * Host invariant failures still invalidate the session through zc_fail. */
struct command_scope {
    struct guest_unwind unwind;
    struct guest_fiber *owner;
};
static void command_cleanup(struct guest_unwind *scope) {
    (void)scope;
    guarded = 0;
}
static void command_enter(struct command_scope *command, jmp_buf *landing) {
    command->owner = guest_task_bound ? &guest_fibers[guest_current] : NULL;
    if (command->owner)
        guest_unwind_enter(&command->unwind, command_cleanup, landing);
}
static void command_leave(struct command_scope *command) {
    if (command->owner)
        guest_unwind_leave(&command->unwind);
    guarded = 0;
}
static int command_exception(struct command_scope *command) {
    int64_t code = command->unwind.exception;
    guest_unwind_abandon(&command->unwind);
    guest_unwind_handled(&command->unwind, code);
    printf("zc: uncaught guest exception 0x%016lx\n", (unsigned long)code);
    return -1;
}
static void command_failed(struct command_scope *command) {
    /* zc_fail restored the root task before jumping here. Never walk a
     * failed parser's scopes, or another fiber's stacks, in that context. */
    if (command->owner)
        command->owner->unwind = command->unwind.previous;
    guarded = 0;
}
int zc_exec(const char *src) {
    if (compiler_recovery_active) {
        zc_output("zc: source compilation during parsing is unsupported\n");
        return -1;
    }
    if (!ready || failed) {
        zc_output("zc: session unavailable; use zreset\n");
        return -1;
    }
    struct command_scope command;
    jmp_buf landing;
    command_enter(&command, &landing);
    if (command.owner && setjmp(landing))
        return command_exception(&command);
    guarded = 1;
    if (setjmp(guard)) {
        command_failed(&command);
        return -1;
    }
    if (key_reset)
        key_reset();
    /* The bootstrap can compile before KernelA/KernelB bind a guest CTask.
     * Once bound, interactive zc input follows ExeCmdLine's per-statement
     * answer path. Module loads keep their separate load-only path. */
    int compile_error = 0;
    (void)execute_source_mode("<shell>", src, guest_task_bound, guest_task_bound,
                              NULL, 0x001 | 0x002 | 0x004, 0, 1, &compile_error);
    command_leave(&command);
    return compile_error ? -1 : 0;
}
static int zc_load_mode(const char *path, int recover) {
    if (compiler_recovery_active) {
        zc_output("zc: source compilation during parsing is unsupported\n");
        return -1;
    }
    char resolved[134];
    int relative = path && path[0] != '/' && strncmp(path, "disk:", 5);
    if (!ready || failed) {
        zc_output("zc: session unavailable; use zreset\n");
        return -1;
    }
    struct command_scope command;
    jmp_buf landing;
    command_enter(&command, &landing);
    if (command.owner && setjmp(landing))
        return command_exception(&command);
    guarded = 1;
    if (setjmp(guard)) {
        command_failed(&command);
        return -1;
    }
    if (relative) {
        if (guest_path(path, resolved)) {
            command_leave(&command);
            zc_output("zc: invalid source path\n");
            return -1;
        }
        path = resolved;
    }
    /* Canonical boot/module paths retain their pinned archive semantics;
     * relative application loads use the same merged view as FileRead. */
    int r = load_source_path(path, relative, recover);
    command_leave(&command);
    return r;
}
int zc_load(const char *path) {
    return zc_load_mode(path, 1);
}
int zc_call(const char *name, int64_t *out) {
    if (!ready || failed)
        return -1;
    CHashFun *f = (CHashFun *)HashFind((char *)name, Fs->hash_table, HTT_FUN, 1);
    if (!f || f->argc || !f->fun_ptr || (f->base.base.type & HTF_EXTERN)) {
        printf("zc: no zero-argument function %s\n", name);
        return -1;
    }
    if (!f->return_class || f->return_class->raw_type < RT_I8i ||
        f->return_class->raw_type > RT_U64i) {
        zc_output("zc: zcall requires an integer return type\n");
        return -1;
    }
    struct command_scope command;
    jmp_buf landing;
    command_enter(&command, &landing);
    if (command.owner && setjmp(landing))
        return command_exception(&command);
    guarded = 1;
    if (setjmp(guard)) {
        command_failed(&command);
        return -1;
    }
    if (guest_task_bound && !guest_fibers[guest_current].source_context)
        (void)guest_prompt_context();
    if (key_reset)
        key_reset();
    int64_t r = FFI_CALL_TOS_0(f->fun_ptr);
    if (out)
        *out = r;
    command_leave(&command);
    return 0;
}
void zc_idle_step(void) {
    if (!ready || failed || !guest_task_bound || guest_current ||
        guest_task_exe_active)
        return;
    guarded = 1;
    if (setjmp(guard)) {
        guest_background_dispatch = 0;
        zeal_fb_task_frame_cancel();
        guarded = 0;
        return;
    }
    if (guest_focus_task || next_guest(0)) {
        guest_background_dispatch = 1;
        (void)host_task_yield(NULL);
        guest_background_dispatch = 0;
    }
    // The interactive shell sleeps about 1 ms between polls. Refreshing each
    // 16 idle steps gives ZealC window controls an input/render loop without
    // consuming one of the guest's limited cooperative task slots.
    if (++guest_idle_frame_divider >= 16) {
        guest_idle_frame_divider = 0;
        if (!guest_winmgr_tick_fun) {
            guest_winmgr_tick_fun = (CHashFun *)HashFind(
                "BootstrapWinMgrTick", Fs->hash_table, HTT_FUN, 1);
        }
        /* The bootstrap shell writes directly to scanout. Keep the upstream
         * manager's empty background from replacing it unless an app window
         * or the diagnostic demo is active. */
        if ((window_demo_loaded || gui_terminal_active) &&
            guest_winmgr_tick_fun && guest_winmgr_tick_fun->fun_ptr &&
            !guest_winmgr_tick_fun->argc &&
            !(guest_winmgr_tick_fun->base.base.type & HTF_EXTERN)) {
            (void)guest_task_frame_run();
        }
    }
    guarded = 0;
}
int zc_focus_owns_input(void) {
    return ready && !failed && guest_task_bound &&
           guest_focus_task != guest_fibers[0].words &&
           valid_guest_task(guest_focus_task);
}
void zc_init(const void *archive, size_t size, zc_output_fn output) {
    if (archive) {
        source_archive = archive;
        archive_size = size;
    }
    if (output)
        output_cb = output;
    ready = failed = loaded_modules = 0;
    window_demo_loaded = 0;
    window_demo_restore_gui_terminal = 0;
    gui_terminal_active = 0;
    guest_keydev_put_key = NULL;
    guest_doc_recalc_stub = NULL;
    guest_home_dir_ptr = NULL;
    gui_terminal_modules_loaded = 0;
    /* A lexer exception can longjmp out of #assert while the Aiwnios parser
     * depth counters are nonzero. zreset starts a new compiler session, so
     * discard that abandoned parse state before loading the bootstrap again. */
    aiwnios_switch_depth = 0;
    aiwnios_fun_depth = 0;
    compiler_recovery_active = 0;
    compiler_recovery_errors = 0;
    guest_task_bound = 0;
    source_winmgr_active = 0;
    guest_current = guest_completed = 0;
    guest_job_count = 0;
    memset(guest_tracked_jobs, 0, sizeof(guest_tracked_jobs));
    guest_task_exe_active = 0;
    guest_job_call_active = 0;
    guest_background_dispatch = 0;
    guest_idle_frame_divider = 0;
    guest_task_frame_count = 0;
    guest_keyboard_route_polls = 0;
    guest_keyboard_route_reads = 0;
    guest_keyboard_route_posts = 0;
    guest_keyboard_route_drops = 0;
    guest_keyboard_route_last_type = 0;
    guest_keyboard_route_last_arg1 = 0;
    guest_keyboard_route_last_arg2 = 0;
    guest_winmgr_tick_fun = NULL;
    guest_focus_task = NULL;
    /* The shared text object can point into the session's staging buffers.
     * Drop those addresses before reclaiming/reusing the compiler arenas. */
    memset(guest_text_globals, 0, sizeof(guest_text_globals));
    memset(guest_fibers, 0, sizeof(guest_fibers));
    set_guest_task(NULL);
    memset(&task, 0, sizeof(task));
    memset(&data_heap, 0, sizeof(data_heap));
    memset(&code_heap, 0, sizeof(code_heap));
    memset(&guest_stack_heap, 0, sizeof(guest_stack_heap));
    memset(files, 0, sizeof(files));
    arena_init(0, data_arena, sizeof(data_arena));
    arena_init(1, code_arena, sizeof(code_arena));
    Fs = &task;
    task.heap = &data_heap;
    task.code_heap = &code_heap;
    code_heap.is_code_heap = 1;
    guarded = 1;
    if (setjmp(guard)) {
        guarded = 0;
        return;
    }
    task.hash_table = HashTableNew(1024, NULL);
    if (load_inner("/System/Bootstrap.ZC")) {
        guarded = 0;
        return;
    }
    pow10_values[309] = 1.0;
    for (int i = 1; i <= 308; i++) {
        pow10_values[309 + i] = pow10_values[308 + i] * 10.0;
        pow10_values[309 - i] = pow10_values[310 - i] / 10.0;
    }
    PrsBindCSymbol("pow10_I64", &pow10_I64_host, 0);
    const struct {
        const char *name;
        int64_t (*fn)(int64_t *);
        int arity;
    } bindings[] = {{"MAlloc", host_malloc, 2}, {"CAlloc", host_calloc, 2},
                    {"ZcSystemTask", host_system_task, 0},
                    {"Call", host_call, 1}, {"ZcCallExtStr", host_call_ext_str, 1},
                    {"ZcTaskValidate", host_task_validate, 1},
                    {"ZcTaskKill", host_kill, 3},
                    {"ZcSpawn", host_spawn, 7},
                    {"Free", host_free, 1},  {"MSize", host_msize, 1},
                    /* MSize2 is ZealOS internal size; alias requested size for now. */
                    {"MSize2", host_msize, 1},
                    {"MemCopy", host_copy, 3},  {"MemSet", host_set, 3}, {"SwapI64", host_swap, 2},
                    {"DCBlotColor4", host_dc_blot_color4, 4},
                    {"GrRopEquU8NoClipping", host_gr_rop_equ_u8_no_clipping, 3},
                    {"Puts", host_puts, 1},
                    {"ZcTaskSpawn", host_task_spawn, 1},
                    {"ZcCallNOnStack", host_call_n_on_stack, 5},
                    {"ZcTaskYield", host_task_yield, 0}, {"ZcTaskRun", host_task_run, 0},
                    {"ZcTaskResult", host_task_result, 1},
                    {"ZcTaskFrameRun", host_task_frame_run, 0},
                    {"ZcMouseStateUpdate", host_mouse_state_update, 5},
                    {"ZcMouseButtonsUpdate", host_mouse_buttons_update, 2},
                    {"ZcFbTextSpan", host_fb_text_span, 10},
                    {"ZcFbTextCellsDrawn", host_fb_text_cells_drawn, 0},
                    {"ZcFbTextPixel", host_fb_text_pixel, 2},
                    {"ZcFbTextChar", host_fb_text_char, 8},
                    {"ZcFbTextFill", host_fb_text_fill, 8},
                    {"ZcFbTextRect", host_fb_text_rect, 7},
                    {"ZcFbTextFlush", host_fb_text_flush, 3},
                    {"ZcDocUpdateTaskDocs", host_doc_update_task_docs, 1},
                    {"ZcDocRecalc", host_doc_recalc, 2},
                    {"ZcActiveGrGlobals", host_active_gr_globals, 0},
                    {"ZcActiveMouseGlobals", host_active_mouse_globals, 0},
                    {"ZcActiveWinMgrGlobals", host_active_winmgr_globals, 0},
                    {"ZcSetWinMgrTask", host_set_winmgr_task, 1},
                    {"ZcFbTextFlushRect", host_fb_text_flush_rect, 6},
                    {"ZcFbGraphPlot", host_fb_graph_plot, 8},
                    {"ZcFbGraphColor", host_fb_graph_color, 1},
                    {"ZcFbSetWinZBuffer", host_fb_set_win_z_buffer, 1},
                    {"ZcFbGraphRect", host_fb_graph_rect, 10},
                    {"ZcFbGraphText", host_fb_graph_text, 9},
                    {"ZcFbGraphLine", host_fb_graph_line, 12},
                    {"ZcFbScreenWidth", host_fb_screen_width, 0},
                    {"ZcFbScreenHeight", host_fb_screen_height, 0},
                    {"ZcFbTaskTextCompose", host_fb_task_text_compose, 5},
                    {"ZcFbShellTextCompose", host_fb_shell_text_compose, 2},
                    {"ExePrint", host_exe_print, 3},
                    {"ExePrint2", host_exe_print2, 3}};
    for (size_t i = 0; i < sizeof(bindings) / sizeof(*bindings); i++)
        PrsBindCSymbol((char *)bindings[i].name, (void *)bindings[i].fn, bindings[i].arity);
    __clear_cache(code_arena, code_arena + sizeof(code_arena));
    ready = 1;
    guarded = 0;
    zc_output("zc: native compiler ready (persistent session)\n");
}
int zc_start_graphics(void) {
    static const char *const modules[] = {
        "/Kernel/KernelA.HH",
        "/Kernel/KernelB.HH",
        "/Kernel/KernelC.HH",
        "/Kernel/KMathB.ZC",
        "/System/TaskBridge.ZC",
        "/System/Externs.ZC",
        /* GrUpdateTaskODEs is part of the normal source frame path. Load its
         * implementation during boot so task-owned ODEs advance without an
         * application having to load MathODE.ZC manually. */
        "/System/Math/MathODE.ZC",
        "/System/Gr/GrInitA.ZC",
        "/System/Gr/Gr.HH",
        "/System/Gr/SpriteBridge.ZC",
        "/System/Gr/GrExterns.ZC",
        "/System/Gr/GrGlobals.ZC",
        "/System/Gr/GrPalette.ZC",
        "/System/Gr/GrDC.ZC",
        "/System/Gr/GrBitMap.ZC",
        "/System/Menu.ZC",
        "/System/Win.ZC",
        "/System/WinMgr.ZC",
        "/System/Gr/WinMgrBoot.ZC",
        "/System/Gr/GrScreen.ZC",
        "/System/Gr/GrBootBridge.ZC",
    };
    const unsigned char *archive = source_archive;
    size_t archive_len = archive_size;
    zc_output_fn output = output_cb;
    CHashGlblVar *winmgr_task;
    CHashGlblVar *system_task;

    /* KernelA/B bind the guest CTask model while these modules load, so the
     * bootstrap session is expected to enter here before guest_task_bound. */
    if (!ready || failed)
        return -1;
    zc_output("zc: loading upstream ZealOS graphics sources\n");
    guarded = 1;
    if (setjmp(guard)) {
        guarded = 0;
        zc_output("zc: upstream graphics source load failed; returning to bootstrap shell\n");
        zc_init(archive, archive_len, output);
        return -1;
    }
    for (size_t i = 0; i < sizeof(modules) / sizeof(*modules); i++) {
        if (load_inner(modules[i])) {
            guarded = 0;
            zc_output("zc: upstream graphics source missing; returning to bootstrap shell\n");
            zc_init(archive, archive_len, output);
            return -1;
        }
    }
    /* GrInit's normal startup selects a palette before applications can save
     * it. The incremental source boot must do that too, rather than relying
     * on the fallback presenter's lazy initialization of an all-zero palette. */
    execute_source("<graphics startup>", "PaletteSetStd(FALSE);");
    /* Verify the source ODE pipeline in the same compiler hash that loaded
     * the modules. Later shell calls may run with a task-local Fs/hash table. */
    const char *const ode_symbols[] = {
        "ODENew", "ODEsUpdate", "GrUpdateTaskODEs"
    };
    for (size_t i = 0; i < sizeof(ode_symbols) / sizeof(*ode_symbols); i++) {
        CHashFun *fun = (CHashFun *)HashFind((char *)ode_symbols[i],
                                             Fs->hash_table, HTT_FUN, 1);
        if (!fun || !fun->fun_ptr || (fun->base.base.type & HTF_EXTERN)) {
            guarded = 0;
            zc_output("zc: boot graphics ODE hook missing; returning to bootstrap shell\n");
            zc_init(archive, archive_len, output);
            return -1;
        }
    }
    zc_output("zc: boot graphics ODE hooks ready\n");
    winmgr_task = (CHashGlblVar *)HashFind("sys_winmgr_task", Fs->hash_table,
                                           HTT_GLBL_VAR, 1);
    system_task = (CHashGlblVar *)HashFind("sys_task", Fs->hash_table,
                                           HTT_GLBL_VAR, 1);
    if (!winmgr_task || !winmgr_task->data_addr ||
        !system_task || !system_task->data_addr ||
        !*(void **)system_task->data_addr) {
        guarded = 0;
        zc_output("zc: upstream graphics task root unavailable; returning to bootstrap shell\n");
        zc_init(archive, archive_len, output);
        return -1;
    }
    /* WinMgrTask is not spawned yet. Give its unchanged renderer the existing
     * shell task ring until the real WinMgr task lifecycle is brought up. */
    if (!*(void **)winmgr_task->data_addr)
        *(void **)winmgr_task->data_addr = *(void **)system_task->data_addr;
    guarded = 0;
    zc_output("zc: upstream ZealOS graphics sources loaded\n");
    return 0;
}
int zc_start_winmgr(void) {
    int64_t result = 0;
    if (!ready || failed ||
        zc_call("BootstrapGrContextFlagsInit", &result) != 0 ||
        result != 0x2a)
        return -1;
    result = 0;
    if (!ready || failed || zc_call("BootstrapWinMgrStart", &result) != 0)
        return -1;
    if (result != 0x2a)
        return -1;
    source_winmgr_active = 1;
    return 0;
}
int zc_start_gui_terminal(void) {
    static const char *const modules[] = {
        "/Kernel/SerialDev/Message.ZC",
        "/Kernel/Job.ZC",
        "/Kernel/KeyDev.ZC",
        "/System/DolDoc/DocBin.ZC",
        "/System/DolDoc/DocNew.ZC",
        "/System/DolDoc/DocPutKey.ZC",
        "/System/DolDoc/DocPutS.ZC",
        "/System/DolDoc/DocDblBuf.ZC",
        "/System/DolDoc/DocTerm.ZC",
    };
    static const char *const doc_modules[] = {
        "/System/DolDoc/DocInit.ZC",
        "/System/DolDoc/DocPlain.ZC",
        "/System/DolDoc/DocRecalcLib.ZC",
        "/System/DolDoc/DocHighlight.ZC",
        "/System/DolDoc/DocRecalc.ZC",
        /* DocPutS was first compiled to break DolDoc's load cycle. Recompile
         * after DocPlain defines ParseDollarCmd so DocLoad/DocPrint retain the
         * real parser, including welcome text, links, colors and saved flags. */
        "/System/DolDoc/DocPutS.ZC",
        /* Make the pinned document/file/editor stack available to the normal
         * graphical shell instead of leaving Ed as a source-load-only path. */
        "/System/DolDoc/DocFile.ZC",
        "/System/DolDoc/DocRun.ZC",
        "/System/DolDoc/DocForm.ZC",
        "/System/DolDoc/DocGet.ZC",
        "/System/DolDoc/DocChar.ZC",
        "/System/DolDoc/DocFind.ZC",
        "/System/DolDoc/DocLink.ZC",
        "/System/DolDoc/DocEd.ZC",
        "/System/DolDoc/DocPopUp.ZC",
        "/System/DolDoc/DocClipBoard.ZC",
        /* Rebind the terminal's RIGHT_AINCLUDE action to real SysFile. */
        "/System/DolDoc/DocTerm.ZC",
        "/System/TaskSettings.ZC",
        "/System/ServerTask.ZC",
        "/System/UserTask.ZC",
        /* Bind CtrlAltT to the original User body instead of the early stub.
         * KeyDevInit is performed once on root's persistent stack below. */
        "/Kernel/KeyDev.ZC",
        "/System/SourceDir.ZC",
        "/System/SourceHome.ZC",
        "/System/VersionScalars.ZC",
        "/System/DolDoc/DocCompiler.ZC",
        "/System/DolDoc/DocTree.ZC",
        "/System/Registry.ZC",
        "/System/SourceRegistry.ZC",
        /* The normal key handler calls these services; load their upstream
         * implementations before rebinding DocPutKey below. */
        "/System/AutoComplete/ACFill.ZC",
        "/System/AutoComplete/ACTask.ZC",
        "/System/AutoComplete/ACInit.ZC",
        /* Direct calls retain the function present at compile time. The
         * first DocPutKey load breaks DolDoc's dependency cycle, but binds
         * EdCharIns/EdCursorLeft/etc. to bootstrap stand-ins. Recompile after
         * their real definitions exist, before installing KDDocPutKey. */
        "/System/DolDoc/DocPutKey.ZC",
        "/System/SourcePrompt.ZC",
        "/System/SourceTraining.ZC",
        "/System/HomeStartUp.ZC",
        "/System/GuiTerminal.ZC",
    };
    int64_t result = 0;
    if (!ready || failed || gui_terminal_active || !zc_source_winmgr_active())
        return gui_terminal_active ? 0 : -1;
    if (!gui_terminal_modules_loaded) {
        for (size_t i = 0; i < sizeof(modules) / sizeof(*modules); i++)
            if (zc_load_mode(modules[i], 0) != 0)
                return -1;
        if (zc_call("BootstrapDefinesLoad", &result) != 0)
            return -1;
        for (size_t i = 0; i < sizeof(doc_modules) / sizeof(*doc_modules); i++)
            if (zc_load_mode(doc_modules[i], 0) != 0)
                return -1;
        gui_terminal_modules_loaded = 1;
    }
    zc_output("zc: graphical terminal starting system task\n");
    if (zc_call("BootstrapSystemTaskStart", &result) != 0 || result != 42)
        return -1;
    zc_output("zc: graphical terminal starting registry\n");
    if (zc_call("BootstrapRegistryStart", &result) != 0 || result != 42)
        return -1;
    zc_output("zc: graphical terminal initializing devices\n");
    if (zc_call("BootstrapGuiTerminalDevicesInit", &result) != 0 || result != 42)
        return -1;
    result = 0;
    zc_output("zc: graphical terminal spawning task\n");
    if (zc_call("BootstrapGuiTerminalStart", &result) != 0 || result != 0x2a)
        return -1;
    zc_output("zc: graphical terminal task spawned\n");
    /* BootstrapGuiTerminalStart assigns VM keyboard focus to the visible
     * graphical shell. The serial shell remains available through UART. */
    gui_terminal_active = 1;
    return 0;
}
int zc_start_home_startup(void) {
    int64_t result = 0;
    if (!ready || failed || !gui_terminal_active || !source_winmgr_active)
        return -1;
    result = 0;
    if (zc_call("BootstrapGuiTerminalHomeStart", &result) != 0 || result != 0x2a)
        return -1;
    return 0;
}
int zc_stop_gui_terminal(void) {
    int64_t result = 0;
    if (!ready || failed || !gui_terminal_active ||
        zc_call("BootstrapGuiTerminalStop", &result) != 0 || result != 0x2a)
        return -1;
    gui_terminal_active = 0;
    guest_focus_task = NULL;
    return 0;
}
int zc_gui_terminal_active(void) {
    return gui_terminal_active;
}
int zc_stop_winmgr(void) {
    int64_t result = 0;
    if (!ready || failed)
        return -1;
    /* A native-only session has no source graphics modules to call into.
     * Reset the source-manager flag with each session, and make stop
     * idempotent so later zreset commands do not look up an absent hook. */
    if (!source_winmgr_active)
        return 0;
    if (zc_call("BootstrapWinMgrStop", &result) != 0)
        return -1;
    if (result != 0x2a)
        return -1;
    source_winmgr_active = 0;
    return 0;
}
int zc_source_winmgr_active(void) {
    return ready && !failed && source_winmgr_active;
}
static int zc_window_demo_restore_terminal(void) {
    if (!window_demo_restore_gui_terminal)
        return 0;
    window_demo_restore_gui_terminal = 0;
    int64_t result = 0;
    if (zc_call("BootstrapGuiTerminalShow", &result) != 0 || result != 0x2a) {
        zc_output("windowdemo: graphical terminal could not be shown; type gtermstop then gterm to retry\n");
        return -1;
    }
    return 0;
}
int zc_start_window_demo(void) {
    int64_t result = 0;
    if (!ready || failed)
        return -1;
    if (window_demo_loaded) {
        zc_output("windowdemo: already loaded; stop it before restarting\n");
        return -1;
    }
    if (gui_terminal_active) {
        if (zc_call("BootstrapGuiTerminalHide", &result) != 0 || result != 0x2a)
            return -1;
        window_demo_restore_gui_terminal = 1;
    }
    if (zc_stop_winmgr() != 0) {
        zc_output("windowdemo: could not stop the source window manager\n");
        (void)zc_window_demo_restore_terminal();
        return -1;
    }
    if (zc_call("BootstrapFrameUseBridge", &result) != 0 || result != 0x2a) {
        zc_output("windowdemo: could not select the framebuffer bridge\n");
        (void)zc_start_winmgr();
        (void)zc_window_demo_restore_terminal();
        return -1;
    }
    if (zc_load("/Tests/WindowDragLive.ZC") != 0) {
        (void)zc_call("BootstrapFrameUseSource", NULL);
        zc_output("windowdemo: could not load the WindowDragLive fixture\n");
        (void)zc_start_winmgr();
        (void)zc_window_demo_restore_terminal();
        return -1;
    }
    window_demo_loaded = 1;
    if (zc_call("WindowDragLiveStart", &result) != 0 || result != 0x2a) {
        (void)zc_call("WindowDragLiveStop", NULL);
        (void)zc_call("BootstrapFrameUseSource", NULL);
        window_demo_loaded = 0;
        zc_output("windowdemo: WindowDragLiveStart failed\n");
        (void)zc_start_winmgr();
        (void)zc_window_demo_restore_terminal();
        return -1;
    }
    if (zc_start_winmgr() != 0) {
        (void)zc_call("WindowDragLiveStop", NULL);
        (void)zc_call("BootstrapFrameUseSource", NULL);
        window_demo_loaded = 0;
        zc_output("windowdemo: source window manager restart failed; fixture stopped\n");
        (void)zc_start_winmgr();
        (void)zc_window_demo_restore_terminal();
        return -1;
    }
    return 0;
}
int zc_stop_window_demo(void) {
    if (!window_demo_loaded)
        return 0;
    /* Stop the periodic source renderer before tearing down its task windows. */
    int failed_stop = zc_stop_winmgr() != 0;
    if (zc_call("WindowDragLiveStop", NULL) != 0)
        failed_stop = 1;
    if (zc_call("BootstrapFrameUseSource", NULL) != 0)
        failed_stop = 1;
    if (zc_start_winmgr() != 0)
        failed_stop = 1;
    window_demo_loaded = 0;
    if (zc_window_demo_restore_terminal() != 0)
        failed_stop = 1;
    return failed_stop ? -1 : 0;
}
int zc_window_demo_active(void) {
    return window_demo_loaded;
}
int zc_reset_session(void) {
    if (ready && !failed) {
        if (window_demo_loaded) {
            // Do not recreate a terminal task while the compiler session is
            // being torn down below.
            window_demo_restore_gui_terminal = 0;
            if (zc_stop_window_demo() != 0)
                return -1;
        }
        if (gui_terminal_active && zc_stop_gui_terminal() != 0)
            return -1;
        if (zc_stop_winmgr() != 0)
            return -1;
    }
    zc_init(NULL, 0, NULL);
    return ready && !failed ? 0 : -1;
}
void zc_status(void) {
    size_t used[2] = {0}, blocks[2] = {0};
    int64_t gui_terminal_window = 0;
    int64_t gui_key_events = 0, gui_last_char = 0, gui_last_scan = 0;
    int64_t gui_editor_active = 0;
    int64_t gui_editor_raw = -1;
    struct guest_fiber *keyboard_focus = ready
        ? find_guest_task(guest_focus_task) : NULL;
    uint64_t scanout_nonblack = zeal_fb_nonblack_pixels();
    for (unsigned a = 0; a < 2; a++)
        for (struct block *b = arenas[a]; b; b = b->next)
            if (b->used) {
                used[a] += b->size;
                blocks[a]++;
            }
    printf("zc: ready=%d failed=%d modules=%lu data=%lu/%u code=%lu/%u allocations=%lu\n", ready,
           failed, (unsigned long)loaded_modules, (unsigned long)used[0], DATA_BYTES,
           (unsigned long)used[1], CODE_BYTES, (unsigned long)(blocks[0] + blocks[1]));
    printf("compiler_recovery: errors=%lu active=%d\n",
           (unsigned long)compiler_recovery_errors, compiler_recovery_active);
    printf("desktop: winmgr=%d gterm=%d windowdemo=%d presented_frames=%lu\n",
           source_winmgr_active, gui_terminal_active, window_demo_loaded,
           (unsigned long)guest_task_frame_count);
    struct guest_fiber *system_server = &guest_fibers[GUEST_SYSTEM];
    printf("system_server: running=%d separate=%d idle=%d queued=%d\n",
           system_server->state == GUEST_RUNNABLE,
           system_server->state == GUEST_RUNNABLE,
           system_server->state == GUEST_RUNNABLE && !!(*guest_flags(system_server) & GUEST_IDLE),
           system_server->state == GUEST_RUNNABLE &&
               guest_job_head(system_server)->next != guest_job_head(system_server));
    printf("keyboard_focus=%s\n", keyboard_focus
           ? (const char *)((unsigned char *)keyboard_focus->words + guest_name_off)
           : "none");
    if (keyboard_focus)
        printf("user_command: original_loop=%d statements=%lu\n",
               keyboard_focus->user_prompt_reader != NULL,
               (unsigned long)keyboard_focus->user_statements);
    printf("virtio_keyboard: ready=%d key_events=%lu last_code=%u last_value=%lu\n",
           zc_virtio_keyboard_ready(),
           (unsigned long)zc_virtio_keyboard_event_count(),
           (unsigned)zc_virtio_keyboard_last_code(),
           (unsigned long)zc_virtio_keyboard_last_value());
    zc_usb_status();
    printf("keyboard_route: polls=%lu read=%lu posted=%lu dropped=%lu last=%ld/%ld/%ld\n",
           (unsigned long)guest_keyboard_route_polls,
           (unsigned long)guest_keyboard_route_reads,
           (unsigned long)guest_keyboard_route_posts,
           (unsigned long)guest_keyboard_route_drops,
           (long)guest_keyboard_route_last_type,
           (long)guest_keyboard_route_last_arg1,
           (long)guest_keyboard_route_last_arg2);
    if (gui_terminal_active) {
        (void)zc_call("BootstrapGuiTerminalKeyEventCount", &gui_key_events);
        (void)zc_call("BootstrapGuiTerminalLastKeyChar", &gui_last_char);
        (void)zc_call("BootstrapGuiTerminalLastScan", &gui_last_scan);
        (void)zc_call("BootstrapGuiTerminalEditorActive", &gui_editor_active);
        (void)zc_call("BootstrapGuiTerminalEditorRaw", &gui_editor_raw);
        printf("gui_keyboard: key_events=%ld last_char=%ld last_scan=0x%02lx\n",
               (long)gui_key_events, (long)gui_last_char,
               (unsigned long)gui_last_scan);
        printf("gui_editor_active=%ld raw=%ld\n",
               (long)gui_editor_active, (long)gui_editor_raw);
    }
    if (gui_terminal_active &&
        zc_call("BootstrapGuiTerminalWindowState", &gui_terminal_window) == 0)
        printf("gterm_window=0x%08lx task/show/display_doc/put_doc/border_doc/geometry\n",
               (unsigned long)gui_terminal_window);
    printf("scanout: nonblack_pixels=%lu of %lu\n",
           (unsigned long)scanout_nonblack,
           (unsigned long)(zeal_fb_screen_width() * zeal_fb_screen_height()));
}
int zc_selftest(void) {
    int64_t result = 0;
    zc_init(NULL, 0, NULL);
    if (!ready || zc_load("/Kernel/QuickSort.ZC") || zc_load("/Tests/SortCaller.ZC") ||
        zc_call("SortChecks", &result) || result != 63) {
        printf("zc: compatibility FAIL result=%ld\n", (long)result);
        return -1;
    }
    if (zc_call("NextRun", &result) || result != 2 || zc_load("/Tests/Session.ZC") ||
        zc_call("SessionChecks", &result) || result != 127) {
        printf("zc: session FAIL result=%ld\n", (long)result);
        return -1;
    }
    if (zc_load("/Tests/Capacity.ZC") || zc_call("CapacityChecks", &result) || result != 648) {
        printf("zc: capacity FAIL result=%ld\n", (long)result);
        return -1;
    }
    zc_output("zc: upstream QuickSort + persistent modules OK\n");
    zc_status();
    return 0;
}
