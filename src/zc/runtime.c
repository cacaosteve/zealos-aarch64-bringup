#include "runtime.h"
#include "aiwn_except.h"
#include "aiwn_hash.h"
#include "aiwn_lexparser.h"
#include "aiwn_mem.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void hc_fmt_f64_bits(char **dp, uint64_t bits, int prec);

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
                                   int64_t right, int64_t bottom);
extern uint64_t zeal_fb_graph_rect(int64_t x, int64_t y,
                                   int64_t width, int64_t height,
                                   uint32_t color, int64_t left, int64_t top,
                                   int64_t right, int64_t bottom);
extern uint64_t zeal_fb_graph_text(int64_t x, int64_t y, uint32_t color,
                                   int64_t left, int64_t top,
                                   int64_t right, int64_t bottom,
                                   const char *text);
extern uint64_t zeal_fb_graph_line(int64_t x0, int64_t y0,
                                   int64_t x1, int64_t y1, uint32_t color,
                                   int64_t step, int64_t start,
                                   int64_t left, int64_t top,
                                   int64_t right, int64_t bottom);

/* Separate arenas from legacy hc_* so its per-demo reset cannot invalidate
 * modules loaded here. Both use splitting/coalescing and arbitrary-order Free. */
#define DATA_BYTES (16u * 1024u * 1024u)
#define CODE_BYTES (4u * 1024u * 1024u)
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
static CTask task;
CTask *Fs;
static CHeapCtrl data_heap, code_heap;
static zc_output_fn output_cb;
static zc_key_event_fn key_reader;
static zc_key_reset_fn key_reset;
static const unsigned char *source_archive;
static size_t archive_size;
static zc_source_read_fn disk_reader;
static jmp_buf guard;
static int guarded, ready, failed;
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
#define GUEST_APP_TASKS 5 /* root plus four user-spawned children */
#define GUEST_EXECUTIVE 5 /* reserved CPU-0 job server */
#define GUEST_TASKS 6
#define GUEST_STACK_BYTES (64u * 1024u)
#define GUEST_JOBS_MAX 128u
#define GUEST_JOB_DEPTH_MAX 8u
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
    GUEST_ENTRY_INPUT_FILTER
};
enum guest_wait {
    GUEST_WAIT_NONE, GUEST_WAIT_BIRTH, GUEST_WAIT_DEATH,
    GUEST_WAIT_IDLE, GUEST_WAIT_MESSAGE, GUEST_WAIT_SERVER, GUEST_WAIT_JOB,
    GUEST_WAIT_TIMER
};
struct guest_exception {
    struct guest_exception *next;
    struct guest_exception *caught_previous;
    unsigned depth;
    uint8_t previous_catch;
    int64_t previous_ch;
    jmp_buf context;
};
struct guest_fiber {
    uint64_t words[2048];
    uint64_t heap_words[256];
    CHeapCtrl heap_owner;
    CTask compiler_task;
    CHeapCtrl compiler_heap, compiler_code_heap;
    jmp_buf context;
    unsigned char stack[GUEST_STACK_BYTES] __attribute__((aligned(16)));
    void *entry, *data;
    int64_t result;
    enum guest_state state;
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
    jmp_buf throw_context;
    jmp_buf end_context;
};
static struct guest_fiber guest_fibers[GUEST_TASKS];
static unsigned guest_current, guest_completed;
static uint64_t guest_task_serial;
static unsigned guest_job_count;
/* Parsing and code emission share one CPU and cannot switch tasks. Emitted
 * source code may switch after compilation finishes. */
static int guest_task_exe_active;
/* Only the CPU-0 executive runs call jobs. Unlike source compilation, a
 * precompiled callback may park on its own fiber stack and resume later. */
static int guest_job_call_active;
static int guest_background_dispatch;
static int64_t guest_mp_count = 1;
static struct guest_counts {
    int64_t jiffies, timer, time_stamp_freq, time_stamp_kHz_freq;
    int64_t time_stamp_freq_initial;
    uint8_t time_stamp_calibrated;
} guest_counts;
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
static int64_t guest_parent_off, guest_number_off, guest_name_off, guest_flags_off;
static int64_t guest_display_flags_off;
static int64_t guest_win_left_off, guest_win_right_off;
static int64_t guest_win_top_off, guest_win_bottom_off, guest_text_attr_off;
static int64_t guest_next_task_off, guest_last_task_off;
static int64_t guest_next_sibling_off, guest_last_sibling_off;
static int64_t guest_next_child_off, guest_last_child_off;
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
static int guest_task_bound;
extern uint64_t zeal_fb_text_cols(void), zeal_fb_text_rows(void);
extern uint64_t zeal_fb_screen_width(void), zeal_fb_screen_height(void);
extern void zeal_fb_task_text_reset(unsigned slot);
extern uint64_t zeal_fb_task_text_write(unsigned slot, int64_t left,
                                         int64_t right, int64_t top,
                                         int64_t bottom, uint32_t attr,
                                         const char *text);
extern uint64_t zeal_fb_task_text_compose(unsigned slot, int64_t left,
                                           int64_t right, int64_t top,
                                           int64_t bottom, uint32_t *text_base,
                                           int64_t stride);
extern uint64_t zeal_fb_shell_text_compose(uint32_t *text_base,
                                            int64_t stride);
static void guest_task_text_output(const char *text);
static int64_t execute_source_result(const char *path, const char *src,
                                     int return_expr, int record_answer);
extern void __clear_cache(void *, void *);
void zc_fail(const char *s) __attribute__((noreturn));

void zc_output(const char *s) {
    if (output_cb)
        output_cb(s);
}
void zc_set_source_reader(zc_source_read_fn read) { disk_reader = read; }
void zc_set_key_reader(zc_key_event_fn read, zc_key_reset_fn reset) {
    key_reader = read;
    key_reset = reset;
}
void zc_fail(const char *s) {
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
    zc_fail("compile failed; use zreset to discard the session");
}
void DoNothing(void) { zc_fail("unresolved external call"); }

static void arena_init(unsigned i, void *p, size_t size) {
    arenas[i] = p;
    *arenas[i] = (struct block){.magic = BLOCK_MAGIC, .size = size - sizeof(struct block)};
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
    for (struct block *b = arenas[a]; b; b = b->next) {
        if (b->used || b->size < n)
            continue;
        if (b->size >= n + sizeof(*b) + 16) {
            struct block *tail = (void *)((unsigned char *)(b + 1) + n);
            *tail = (struct block){
                .magic = BLOCK_MAGIC, .size = b->size - n - sizeof(*b), .prev = b, .next = b->next};
            if (tail->next)
                tail->next->prev = tail;
            b->next = tail;
            b->size = n;
        }
        b->used = 1;
        b->requested = (size_t)count;
        b->owner = owner;
        owner->used_u8s += (int64_t)b->size;
        sync_guest_heap(owner);
        return b + 1;
    }
    zc_fail(a ? "code arena exhausted" : "data arena exhausted");
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
    owner->used_u8s -= (int64_t)b->size;
    sync_guest_heap(owner);
    b->used = 0;
    b->owner = NULL;
    b->requested = 0;
    if (b->next && !b->next->used) {
        struct block *n = b->next;
        b->size += sizeof(*n) + n->size;
        b->next = n->next;
        n->magic = 0;
        if (b->next)
            b->next->prev = b;
    }
    if (b->prev && !b->prev->used) {
        struct block *p = b->prev;
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
static char *disk_copy(const char *path, size_t *length) {
    char name[128];
    size_t n = 0, got = 0;
    if (disk_name(path, name) || !disk_reader || disk_reader(name, NULL, 0, &n) || !n ||
        n > 4u * 1024u * 1024u)
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
FILE *fopen(const char *path, const char *mode) {
    if (strcmp(mode, "rb") && strcmp(mode, "r"))
        return NULL;
    size_t n;
    char *owned = NULL;
    const unsigned char *data;
    if (!strncmp(path, "disk:", 5)) {
        owned = disk_copy(path, &n);
        data = (const unsigned char *)owned;
    } else {
        data = source_find(path, &n);
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

static int64_t host_malloc(int64_t *a) {
    /* ZealOS MAlloc(size, mem_task=NULL). Task-specific heaps are not wired
     * yet; ignore mem_task and allocate on the current fiber/data heap. */
    CHeapCtrl *owner = guest_task_bound ? &guest_fibers[guest_current].heap_owner : &data_heap;
    (void)a[1];
    return (int64_t)(uintptr_t)__AIWNIOS_MAlloc(a[0], owner);
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
static int64_t host_set(int64_t *a) {
    return (int64_t)(uintptr_t)memset((void *)(uintptr_t)a[0], (int)a[1], (size_t)a[2]);
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
                                       a[3], a[4], a[5], a[6]);
}
static int64_t host_fb_graph_rect(int64_t *a) {
    return (int64_t)zeal_fb_graph_rect(a[0], a[1], a[2], a[3],
                                       (uint32_t)a[4], a[5], a[6], a[7], a[8]);
}
static int64_t host_fb_graph_text(int64_t *a) {
    return (int64_t)zeal_fb_graph_text(a[0], a[1], (uint32_t)a[2],
                                       a[3], a[4], a[5], a[6],
                                       (const char *)(uintptr_t)a[7]);
}
static int64_t host_fb_graph_line(int64_t *a) {
    return (int64_t)zeal_fb_graph_line(a[0], a[1], a[2], a[3],
                                       (uint32_t)a[4], a[5], a[6],
                                       a[7], a[8], a[9], a[10]);
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
    return (int64_t)(uintptr_t)guest_cpu;
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
static int64_t host_guest_throw(int64_t *a) {
    struct guest_fiber *f = &guest_fibers[guest_current];
    struct guest_exception *pad = guest_task_bound ? f->except_top : NULL;
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
    *(int64_t *)((unsigned char *)f->words + guest_except_ch_off) = a[0];
    *(uint8_t *)((unsigned char *)f->words + guest_catch_except_off) = 0;
    memcpy(f->throw_context, pad->context, sizeof(f->throw_context));
    longjmp(f->throw_context, 1);
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
#define GUEST_INPUT_FILTER_TASK (1u << 5)
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
static int64_t host_rflags_get(int64_t *a);
static int64_t host_rflags_set(int64_t *a);
static int64_t host_jobs_handler(int64_t *a);
static uint64_t counter_now(void);
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
static void guest_job_delete(struct guest_job *job) {
    if (job->aux_str)
        __AIWNIOS_Free(job->aux_str);
    __AIWNIOS_Free(job);
    guest_job_count--;
}
static int64_t host_job_del(int64_t *a) {
    struct guest_job *job = (void *)(uintptr_t)a[0];
    if (!job || job->next || job->last || MSize(job) != sizeof(*job) ||
        !guest_job_count)
        zc_fail("JobDel requires a detached job");
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
    if (f->wait_kind == GUEST_WAIT_IDLE) {
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
    for (unsigned i = 1; i < GUEST_TASKS; i++)
        if (guest_fibers[i].state == GUEST_RUNNABLE &&
            guest_fibers[i].wait_kind == GUEST_WAIT_TIMER &&
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
    *(void **)(words + guest_parent_off) = parent;
    *(void **)(words + guest_input_filter_off) = words;
    *(void **)(words + guest_next_filter_off) = words;
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
     * hash. HashTableNew's CHash/CHashTable layout matches ZealOS. */
    *(void **)(words + guest_hash_table_off) = HashTableNew(1024, NULL);
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
    f->context[12] = (uintptr_t)(f->stack + sizeof(f->stack)); /* saved SP */
    __asm__ volatile("mrs %0, fpcr" : "=r"(f->context[22]));
    __asm__ volatile("mrs %0, fpsr" : "=r"(f->context[23]));
}
static struct guest_fiber *new_guest_fiber(void *entry, void *data, void *parent,
                                           const char *name, enum guest_entry kind) {
    if (!guest_task_bound ||
        !find_guest_task(parent) ||
        (kind != GUEST_ENTRY_INPUT_FILTER && !code_lives_with_parent(entry, parent)))
        return NULL;
    for (unsigned i = 1; i < GUEST_APP_TASKS; i++) {
        struct guest_fiber *f = &guest_fibers[i];
        if (f->state == GUEST_RUNNABLE)
            continue;
        memset(f, 0, sizeof(*f));
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
    guest_jobs_clear(f);
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
static void end_guest_current(int64_t result) {
    unsigned me = guest_current;
    struct guest_fiber *f = &guest_fibers[me];
    if (!me || me == GUEST_EXECUTIVE || f->state != GUEST_RUNNABLE)
        zc_fail("cannot end root or executive task");
    if (f->ending)
        longjmp(f->end_context, 1); /* Exit inside a callback resumes teardown. */
    f->ending = 1;
    f->kill_requested = 0;
    f->wait_kind = GUEST_WAIT_NONE;
    *guest_flags(f) &= ~(GUEST_KILL_TASK | GUEST_SUSPENDED | GUEST_AWAITING_MESSAGE);
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
                                            GUEST_ENTRY_NO_ARG);
    return f ? (int64_t)(f - guest_fibers) : -1;
}
static struct guest_fiber *find_guest_task(void *task) {
    for (unsigned i = 0; i < GUEST_TASKS; i++)
        if (guest_fibers[i].words == task && guest_fibers[i].state == GUEST_RUNNABLE)
            return &guest_fibers[i];
    return NULL;
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
        *(int64_t *)(task + guest_win_bottom_off), plane, a[2]);
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
    /* Sched.ZC walks next_task from the current focus, or sys_task if
     * nothing owns focus. Root stands in for sys_task in this bootstrap.
     * Window stacking (EXT_WIN_TO_TOP) is not present yet. */
    void *root = guest_fibers[0].words;
    void *start = valid_guest_task(guest_focus_task) &&
                  guest_focus_task != guest_fibers[GUEST_EXECUTIVE].words
        ? guest_focus_task : root;
    void *task = *(void **)((unsigned char *)start + guest_next_task_off);
    guest_focus_task = NULL;
    for (unsigned hops = 0; hops < GUEST_APP_TASKS + 1; hops++) {
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
    if (!guest_task_bound || a[3] != -1 || a[5] != 0 || a[6] != (1 << 8) ||
        !find_guest_task(parent))
        return 0;
    struct guest_fiber *f = new_guest_fiber(entry, (void *)(uintptr_t)a[1], parent,
                                            (const char *)(uintptr_t)a[2], GUEST_ENTRY_SPAWN);
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
    if (!f || f == &guest_fibers[0])
        return 0; /* The shell's root context must remain schedulable. */
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
    if (!f || f == &guest_fibers[0] || f == &guest_fibers[GUEST_EXECUTIVE] || a[2])
        return 0; /* Protect shell/executive; debug-break needs an exception runtime. */
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
    if (guest_task_exe_active)
        zc_fail("yield during job dispatch is unsupported");
    unsigned me = guest_current;
    struct guest_fiber *self = &guest_fibers[me];
    /* A focused child may be parked in MessageGet. The original KeyGet also
     * marks the current task AWAITING_MESSAGE before yielding, even when no
     * explicit focus exists yet. Feed that task's queue before scheduling. */
    if (guest_focus_task ||
        (guest_task_bound && (*guest_flags(self) & GUEST_AWAITING_MESSAGE)))
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
    if (next == me) {
        if (park_server)
            self->wait_kind = GUEST_WAIT_NONE;
        return 0;
    }
    if (!setjmp(self->context))
        enter_guest(next);
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
    if (a[1])
        zc_fail("DeathWait send_exit needs XTalk task-exit protocol");
    return guest_wait(GUEST_WAIT_DEATH, (void *)(uintptr_t)a[0], 0);
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
    if (guest_job_count > GUEST_JOBS_MAX - needed)
        zc_fail("guest message queue full");
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
        guest_job_count++;
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
        if (type != 2 && type != 3)
            continue;
        if (post_guest_message(task, NULL,
                               type, (int64_t)ch, (int64_t)sc, 0))
            last_code = type;
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
        filter->source_job_active++;
        guest_task_exe_active = 1;
        (void)execute_source_result("<TaskText>", job->aux_str, 1, 1);
        guest_task_exe_active = 0;
        filter->source_job_active--;
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
    guest_job_count++;
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
                                 GUEST_ENTRY_INPUT_FILTER);
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
static int64_t host_task_exe(int64_t *a) {
    struct guest_fiber *server = find_guest_task((void *)(uintptr_t)a[0]);
    void *master = (void *)(uintptr_t)a[1];
    const char *source = (const char *)(uintptr_t)a[2];
    uint64_t flags = (uint64_t)a[3];
    if (!server || !source ||
        (*(void **)((unsigned char *)server->words + guest_popup_off) &&
         !(*guest_flags(server) & GUEST_FILTER_INPUT)) ||
        (master && !valid_guest_task(master)) ||
        (flags & ~(uint64_t)(GUEST_JOBF_WAKE_MASTER |
                             GUEST_JOBF_FOCUS_MASTER |
                             GUEST_JOBF_EXIT_ON_COMPLETE |
                             GUEST_JOBF_FREE_ON_COMPLETE)) ||
        ((flags & GUEST_JOBF_WAKE_MASTER) &&
         (master != guest_fibers[guest_current].words || master == server->words)) ||
        ((flags & GUEST_JOBF_EXIT_ON_COMPLETE) && server == &guest_fibers[0]))
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
    job->flags = (int64_t)flags;
    job->aux_str = copy;
    job->master_task = master;
    guest_job_insert(guest_job_head(server), job);
    guest_job_count++;
    *guest_flags(server) &= ~(GUEST_IDLE | GUEST_AWAITING_MESSAGE);
    if (flags & GUEST_JOBF_WAKE_MASTER) {
        struct guest_fiber *self = &guest_fibers[guest_current];
        self->wait_target = job;
        (void)guest_wait(GUEST_WAIT_JOB, &self->wait_target, 0);
    }
    return (int64_t)(uintptr_t)job;
}
static int64_t host_job_queue(int64_t *a) {
    void *callback = (void *)(uintptr_t)a[0];
    uint64_t flags = (uint64_t)a[3];
    int spawn = a[4] == GUEST_JOBT_SPAWN_TASK;
    void *parent = a[6] ? (void *)(uintptr_t)a[6] : guest_fibers[0].words;
    /* Queued callbacks may outlive their submitter. Root-owned code stays
     * live for this compiler session; child-owned code would need pinning. */
    if (!guest_task_bound || code_owner(callback) != &code_heap || a[2] != 0 ||
        (spawn ? (flags != GUEST_JOBF_ADD_TO_QUE || a[7] ||
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
    }
    guest_job_insert(guest_job_head(server), job);
    guest_job_count++;
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
static int64_t host_jobs_handler(int64_t *a) {
    void *task = a[1] ? (void *)(uintptr_t)a[1] : guest_fibers[guest_current].words;
    if (task != guest_fibers[guest_current].words)
        zc_fail("JobsHandler requires the current task");
    if (guest_task_exe_active ||
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
        if (job->job_code == GUEST_JOBT_EXE_STR) {
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
                    job->aux_str ? job->aux_str : "Unnamed", GUEST_ENTRY_SPAWN)
                : NULL;
            job->spawned_task = child ? child->words : NULL;
        }
        server->active_jobs = active.previous;
        int exit_on_complete = !!(job->flags & GUEST_JOBF_EXIT_ON_COMPLETE);
        struct guest_fiber *master = find_guest_task(job->master_task);
        /* Pinned JobRunOne restores focus after dispatch, unless the master
         * has WIf_SELF_FOCUS set in win_inhibit. */
        if (master && (job->flags & GUEST_JOBF_FOCUS_MASTER) &&
            !(*(uint32_t *)((unsigned char *)master->words + guest_win_inhibit_off) & 1u))
            guest_focus_task = master->words;
        if ((job->flags & GUEST_JOBF_FREE_ON_COMPLETE) ||
            (job->master_task && !master)) {
            guest_job_delete(job);
        } else {
            struct guest_fiber *owner = master ? master : server;
            job->flags |= GUEST_JOBF_DONE;
            guest_job_insert(guest_done_head(owner), job);
        }
        count++;
        if (exit_on_complete)
            end_guest_current(0);
    }
    return count;
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
    self->wait_deadline = deadline;
    self->wait_kind = GUEST_WAIT_TIMER;
    while (!wait_ready(self)) {
        (void)host_task_yield(NULL);
        __asm__ volatile("yield");
    }
    self->wait_kind = GUEST_WAIT_NONE;
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
    CHashFun *fun = (CHashFun *)HashFind("Fs", Fs->hash_table, HTT_FUN, 1);
    if (!cls || !heap || !ctrl || !job || !counts || !fun || !fun->import_name || strcmp(fun->import_name, "IC_FS") ||
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
        !task_field(cls, "next_task", 8, -1) ||
        !task_field(cls, "last_task", 8, -1) ||
        !task_field(cls, "next_sibling_task", 8, -1) ||
        !task_field(cls, "last_sibling_task", 8, -1) ||
        !task_field(cls, "next_child_task", 8, -1) ||
        !task_field(cls, "last_child_task", 8, -1) ||
        !task_field(cls, "popup_task", 8, -1) ||
        !task_field(cls, "last_input_filter_task", 8, -1) ||
        !task_field(cls, "next_input_filter_task", 8, -1) ||
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
        !task_field(heap, "hc_signature", 4, 8) ||
        !task_field(heap, "used_u8s", 8, -1) ||
        !task_field(heap, "mem_task", 8, -1))
        zc_fail("pinned task/heap layout or Fs signature mismatch");
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
    guest_next_task_off = MemberFind("next_task", cls)->off;
    guest_last_task_off = MemberFind("last_task", cls)->off;
    guest_next_sibling_off = MemberFind("next_sibling_task", cls)->off;
    guest_last_sibling_off = MemberFind("last_sibling_task", cls)->off;
    guest_next_child_off = MemberFind("next_child_task", cls)->off;
    guest_last_child_off = MemberFind("last_child_task", cls)->off;
    if (guest_next_child_off < guest_next_sibling_off ||
        guest_last_child_off - guest_next_child_off !=
        guest_last_sibling_off - guest_next_sibling_off ||
        guest_last_sibling_off - guest_next_sibling_off != 8)
        zc_fail("pinned task child sentinel layout mismatch");
    guest_popup_off = MemberFind("popup_task", cls)->off;
    guest_input_filter_off = MemberFind("last_input_filter_task", cls)->off;
    guest_next_filter_off = MemberFind("next_input_filter_task", cls)->off;
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
    guest_heap_sig_off = MemberFind("hc_signature", heap)->off;
    guest_heap_used_off = MemberFind("used_u8s", heap)->off;
    guest_heap_task_off = MemberFind("mem_task", heap)->off;
    memset(guest_fibers, 0, sizeof(guest_fibers));
    memset(guest_progresses, 0, sizeof(guest_progresses));
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
    PrsBindCSymbol("ToF64", host_to_f64, 1);
    PrsBindCSymbol("ToI64", host_to_i64, 1);
    PrsBindCSymbol("AbsI64", host_abs_i64, 1);
    PrsBindCSymbol("ToBool", host_to_bool, 1);
    PrsBindCSymbol("ToUpper", host_to_upper, 1);
    PrsBindCSymbol("SwapI64", host_swap, 2);
    PrsBindCSymbol("sys_semas", guest_sys_semas, 0);
}

static int64_t execute_source_result(const char *path, const char *src,
                                     int return_expr, int record_answer) {
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
    lex->file->dir = __AIWNIOS_StrDup(dir, NULL);
    CCmpCtrl *cc = CmpCtrlNew(lex);
    cc->flags |= CCF_STRINGS_ON_HEAP;
    CodeCtrlPush(cc);
    Lex(lex);
    int64_t result = 0;
    while (PrsStmt(cc)) {
        CRPN *first = (CRPN *)cc->code_ctrl->ir_code->next;
        int64_t raw_type = RT_U0;
        if (record_answer && first != (CRPN *)cc->code_ctrl->ir_code) {
            if (first->type == IC_RET)
                first = (CRPN *)first->base.next;
            if (first != (CRPN *)cc->code_ctrl->ir_code && first->type != IC_NOP)
                raw_type = AssignRawTypeToNode(cc, first);
        }
        /* The Aiwnios file loader discards each top-level expression. ExePrint
         * returns its last value, so give each TaskExe statement a return.
         * Definition-only statements are emitted as IC_NOP and remain zero. */
        if (return_expr && cc->code_ctrl->ir_code->next != cc->code_ctrl->ir_code &&
            ((CRPN *)cc->code_ctrl->ir_code->next)->type != IC_NOP) {
            CRPN *ret = __AIWNIOS_CAlloc(sizeof(*ret), NULL);
            ret->type = IC_RET;
            QueIns((CQue *)ret, cc->code_ctrl->ir_code);
        }
        char *code = Compile(cc, NULL, NULL, NULL);
        if (code) {
            uint64_t start = record_answer ? counter_now() : 0;
            int source_job = guest_task_bound &&
                             guest_fibers[guest_current].source_job_active;
            if (source_job)
                guest_task_exe_active = 0;
            result = FFI_CALL_TOS_0(code);
            if (source_job)
                guest_task_exe_active = 1;
            uint64_t end = record_answer ? counter_now() : 0;
            free(code);
            if (record_answer && guest_task_bound) {
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
    if (lex->cur_tok) {
        printf("zc: unconsumed token=%ld at %s:%ld:%ld name=%s\n",
               (long)lex->cur_tok, lex->file ? lex->file->filename : "<eof>",
               lex->file ? (long)lex->file->ln + 1 : 0,
               lex->file ? (long)lex->file->col + 1 : 0, lex->string);
        zc_fail("unconsumed source token");
    }
    CodeCtrlPop(cc);
    CmpCtrlDel(cc);
    LexerDel(lex);
    return result;
}
static int64_t host_exe_print(int64_t *a) {
    const char *format = (const char *)(uintptr_t)a[0];
    const char *source = (const char *)(uintptr_t)a[1];
    if (!guest_task_bound || guest_fibers[guest_current].state != GUEST_RUNNABLE)
        zc_fail("ExePrint requires a guest task");
    if (!format || strcmp(format, "%s") || !source)
        zc_fail("ExePrint currently requires \"%s\" and a source string");
    size_t length = 0;
    while (length <= 4096 && source[length])
        length++;
    if (length > 4096)
        zc_fail("ExePrint source exceeds 4096 bytes");
    if (guest_task_exe_active)
        zc_fail("ExePrint during compilation is unsupported");
    struct guest_fiber *f = &guest_fibers[guest_current];
    f->source_job_active++;
    guest_task_exe_active = 1;
    int64_t result = execute_source_result("<ExePrint>", source, 1, 1);
    guest_task_exe_active = 0;
    f->source_job_active--;
    return result;
}
static int64_t host_print(int64_t *a);
/* Bounded text formatter shared by Print and StrPrintJoin. Supports the
 * markers and codes used by Message.ZC, Job.ZC, and upstream progress UI. */
static size_t format_text(char *output, size_t capacity, const char *format,
                          int64_t argc, const int64_t *argv) {
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
        if (format[i] == '$' && format[i + 1] == '$') {
            /* Text console has no DolDoc style state; drop $$...$$ markers. */
            i += 2;
            while (format[i] && !(format[i] == '$' && format[i + 1] == '$'))
                i++;
            if (format[i] == '$' && format[i + 1] == '$')
                i += 2;
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
        /* ZealOS formats commonly use %0.3f. Width is parsed and ignored
         * for now; precision controls the emitted fractional digits. */
        while (format[spec] >= '0' && format[spec] <= '9')
            spec++;
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
        char code = format[spec];
        if (!code)
            zc_fail("format ends with incomplete conversion");
        i = spec + 1;
        if (code == '%') {
            FMT_BYTE('%');
            continue;
        }
        if (code != 'c' && code != 'C' && code != 's' && code != 'd' &&
            code != 'f' && code != 'F')
            zc_fail("format code is unsupported");
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
        } else if (code == 's') {
            const char *s = (const char *)(uintptr_t)argv[arg++];
            if (!s) zc_fail("format %s argument is null");
            for (size_t j = 0; s[j]; j++)
                FMT_BYTE(s[j]);
        } else if (code == 'f' || code == 'F') {
            char number[64];
            char *end = number;
            hc_fmt_f64_bits(&end, (uint64_t)argv[arg++],
                            has_precision ? precision : 1);
            *end = 0;
            for (const char *p = number; *p; p++)
                FMT_BYTE(*p);
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
    size_t used = format_text(stack, sizeof(stack), format, argc, argv);
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
    format_text(output, sizeof(output), format, argc, argv);
    zc_output(output);
    guest_task_text_output(output);
    return 0;
}
static int execute_source(const char *path, const char *src) {
    (void)execute_source_result(path, src, 0, 0);
    return 0;
}
static int load_inner(const char *path) {
    size_t n;
    char *src;
    if (!strncmp(path, "disk:", 5)) {
        src = disk_copy(path, &n);
    } else {
        const unsigned char *s = source_find(path, &n);
        src = s ? malloc(n + 1) : NULL;
        if (src) {
            memcpy(src, s, n);
            src[n] = 0;
        }
    }
    if (!src) {
        printf("zc: source not found: %s\n", path);
        return -1;
    }
    execute_source(path, src);
    if (!strcmp(path, "/Kernel/KernelB.HH")) {
        bind_guest_task();
        if (load_inner("/System/TaskBridge.ZC"))
            zc_fail("task bridge source missing");
        PrsBindCSymbol("SysTry", host_sys_try, 0);
        PrsBindCSymbol("SysUntry", host_sys_untry, 0);
        PrsBindCSymbol("EndCatch", host_end_catch, 0);
        PrsBindCSymbolNaked("AIWNIOS_SetJmp", zc_setjmp, 1);
        PrsBindCSymbol("throw", host_guest_throw, 2);
        PrsBindCSymbol("ExePrint", host_exe_print, 2);
        PrsBindCSymbol("Print", host_print, 3);
        PrsBindCSymbol("StrPrintJoin", host_str_print_join, 4);
        PrsBindCSymbol("PutKey", host_put_key, 2);
        PrsBindCSymbol("StrLen", host_str_len, 1);
        PrsBindCSymbol("counts", &guest_counts, 0);
        PrsBindCSymbol("RFlagsPush", host_rflags_push, 0);
        PrsBindCSymbol("RFlagsPop", host_rflags_pop, 0);
        PrsBindCSymbol("KbdMouseHandler", host_kbd_mouse_handler, 2);
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
        PrsBindCSymbol("HashFind", host_hash_find, 4);
        PrsBindCSymbol("HashAdd", host_hash_add, 2);
    }
    free(src);
    loaded_modules++;
    printf("zc: loaded %s\n", path);
    return 0;
}
int zc_exec(const char *src) {
    if (!ready || failed) {
        zc_output("zc: session unavailable; use zreset\n");
        return -1;
    }
    guarded = 1;
    if (setjmp(guard)) {
        guarded = 0;
        return -1;
    }
    if (key_reset)
        key_reset();
    /* The bootstrap can compile before KernelA/KernelB bind a guest CTask.
     * Once bound, interactive zc input follows ExeCmdLine's per-statement
     * answer path. Module loads keep their separate load-only path. */
    (void)execute_source_result("<shell>", src, guest_task_bound, guest_task_bound);
    guarded = 0;
    return 0;
}
int zc_load(const char *path) {
    if (!ready || failed) {
        zc_output("zc: session unavailable; use zreset\n");
        return -1;
    }
    guarded = 1;
    if (setjmp(guard)) {
        guarded = 0;
        return -1;
    }
    int r = load_inner(path);
    guarded = 0;
    return r;
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
    guarded = 1;
    if (setjmp(guard)) {
        guarded = 0;
        return -1;
    }
    if (key_reset)
        key_reset();
    int64_t r = FFI_CALL_TOS_0(f->fun_ptr);
    if (out)
        *out = r;
    guarded = 0;
    return 0;
}
void zc_idle_step(void) {
    if (!ready || failed || !guest_task_bound || guest_current ||
        guest_task_exe_active ||
        (!guest_focus_task && !next_guest(0)))
        return;
    guarded = 1;
    if (setjmp(guard)) {
        guest_background_dispatch = 0;
        guarded = 0;
        return;
    }
    guest_background_dispatch = 1;
    (void)host_task_yield(NULL);
    guest_background_dispatch = 0;
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
    guest_task_bound = 0;
    guest_current = guest_completed = 0;
    guest_job_count = 0;
    guest_task_exe_active = 0;
    guest_job_call_active = 0;
    guest_background_dispatch = 0;
    guest_focus_task = NULL;
    memset(guest_fibers, 0, sizeof(guest_fibers));
    set_guest_task(NULL);
    memset(&task, 0, sizeof(task));
    memset(&data_heap, 0, sizeof(data_heap));
    memset(&code_heap, 0, sizeof(code_heap));
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
    } bindings[] = {{"MAlloc", host_malloc, 2}, {"Free", host_free, 1},  {"MSize", host_msize, 1},
                    /* MSize2 is ZealOS internal size; alias requested size for now. */
                    {"MSize2", host_msize, 1},
                    {"MemCopy", host_copy, 3},  {"MemSet", host_set, 3}, {"SwapI64", host_swap, 2},
                    {"Puts", host_puts, 1}, {"ZcTaskSpawn", host_task_spawn, 1},
                    {"ZcTaskYield", host_task_yield, 0}, {"ZcTaskRun", host_task_run, 0},
                    {"ZcTaskResult", host_task_result, 1},
                    {"ZcFbTextSpan", host_fb_text_span, 10},
                    {"ZcFbTextCellsDrawn", host_fb_text_cells_drawn, 0},
                    {"ZcFbTextPixel", host_fb_text_pixel, 2},
                    {"ZcFbTextChar", host_fb_text_char, 8},
                    {"ZcFbTextFill", host_fb_text_fill, 8},
                    {"ZcFbTextRect", host_fb_text_rect, 7},
                    {"ZcFbTextFlush", host_fb_text_flush, 3},
                    {"ZcFbTextFlushRect", host_fb_text_flush_rect, 6},
                    {"ZcFbGraphPlot", host_fb_graph_plot, 7},
                    {"ZcFbGraphRect", host_fb_graph_rect, 9},
                    {"ZcFbGraphText", host_fb_graph_text, 8},
                    {"ZcFbGraphLine", host_fb_graph_line, 11},
                    {"ZcFbScreenWidth", host_fb_screen_width, 0},
                    {"ZcFbScreenHeight", host_fb_screen_height, 0},
                    {"ZcFbTaskTextCompose", host_fb_task_text_compose, 3},
                    {"ZcFbShellTextCompose", host_fb_shell_text_compose, 2}};
    for (size_t i = 0; i < sizeof(bindings) / sizeof(*bindings); i++)
        PrsBindCSymbol((char *)bindings[i].name, (void *)bindings[i].fn, bindings[i].arity);
    __clear_cache(code_arena, code_arena + sizeof(code_arena));
    ready = 1;
    guarded = 0;
    zc_output("zc: native compiler ready (persistent session)\n");
}
void zc_status(void) {
    size_t used[2] = {0}, blocks[2] = {0};
    for (unsigned a = 0; a < 2; a++)
        for (struct block *b = arenas[a]; b; b = b->next)
            if (b->used) {
                used[a] += b->size;
                blocks[a]++;
            }
    printf("zc: ready=%d failed=%d modules=%lu data=%lu/%u code=%lu/%u allocations=%lu\n", ready,
           failed, (unsigned long)loaded_modules, (unsigned long)used[0], DATA_BYTES,
           (unsigned long)used[1], CODE_BYTES, (unsigned long)(blocks[0] + blocks[1]));
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
