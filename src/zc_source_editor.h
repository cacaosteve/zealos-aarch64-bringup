/* Modal line editor for the native source partition. Included by kernel_stub.c
 * after the console and source-store helpers; not part of the Pi diagnostic. */
#pragma once

static struct {
    char name[ZSS_MAX_NAME + 1];
    uint32_t length;
    uint32_t view_first, view_count, cursor_line;
    int active, dirty, existed;
    char text[ZSS_MAX_FILE + 1u];
} g_zedit;

static uint32_t zedit_lines(void);
static int zedit_bounds(uint32_t number, uint32_t *start, uint32_t *end);

/* The editor keeps the existing command-based edits, but uses the framebuffer
 * as a stable full-screen source view. fb_putc updates the retained shell text
 * plane without sending these redraw bytes to the serial console. */
static void zedit_fb_puts(const char *text) {
    while (*text) fb_putc(*text++);
}

static void zedit_fb_number(uint32_t value) {
    char digits[10];
    uint32_t count = 0;
    do {
        digits[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value && count < sizeof(digits));
    while (count) fb_putc(digits[--count]);
}

static void zedit_render(void) {
    if (!g_fb || !g_zedit.active) return;
    uint32_t rows = g_fb_rows < ZEAL_TEXT_MAX_ROWS ? g_fb_rows : ZEAL_TEXT_MAX_ROWS;
    uint32_t cols = g_fb_cols < ZEAL_TEXT_MAX_COLS ? g_fb_cols : ZEAL_TEXT_MAX_COLS;
    if (rows < 6 || !cols) return;

    uint32_t capacity = rows - 5u;
    uint32_t lines = zedit_lines();
    if (!g_zedit.view_first) g_zedit.view_first = 1;
    if (g_zedit.view_first > lines && lines) g_zedit.view_first = lines;
    if (g_zedit.view_count > capacity) g_zedit.view_count = capacity;
    if (!g_zedit.view_count) g_zedit.view_count = capacity;

    fb_clear(0x00101820u);
    zedit_fb_puts("zedit ");
    zedit_fb_puts(g_zedit.name);
    zedit_fb_puts(g_zedit.dirty ? "  [modified]" : "  [saved]");
    fb_putc('\n');

    uint32_t shown = 0;
    for (uint32_t number = g_zedit.view_first;
         number <= lines && shown < g_zedit.view_count; number++, shown++) {
        uint32_t start, end;
        if (!zedit_bounds(number, &start, &end)) break;
        zedit_fb_number(number);
        zedit_fb_puts(number == g_zedit.cursor_line ? ":> " : ":  ");
        uint32_t room = cols > 14u ? cols - 14u : 1u;
        uint32_t copied = 0;
        for (uint32_t i = start; i < end && g_zedit.text[i] != '\n'; i++) {
            if (copied == room) {
                fb_putc('.'); fb_putc('.'); fb_putc('.');
                break;
            }
            fb_putc(g_zedit.text[i]);
            copied++;
        }
        fb_putc('\n');
    }

    g_fb_cx = 0;
    g_fb_cy = rows - 3u;
    zedit_fb_puts("j/k move  e edit  n insert  x delete  p page  w save  q close");
    g_fb_cx = 0;
    g_fb_cy = rows - 2u;
    zedit_fb_puts("view "); zedit_fb_number(g_zedit.view_first);
    zedit_fb_puts("  lines "); zedit_fb_number(lines);
    zedit_fb_puts(g_zedit.dirty ? "  unsaved" : "  saved");
    /* shell_run writes the edit> prompt at this reserved final row. */
    g_fb_cx = 0;
    g_fb_cy = rows - 1u;
}

static uint32_t zedit_lines(void) {
    uint32_t lines = 0;
    for (uint32_t i = 0; i < g_zedit.length; i++)
        if (g_zedit.text[i] == '\n') lines++;
    if (g_zedit.length && g_zedit.text[g_zedit.length - 1] != '\n') lines++;
    return lines;
}

static int zedit_bounds(uint32_t number, uint32_t *start, uint32_t *end) {
    uint32_t current = 1, at = 0;
    if (!number) return 0;
    while (at < g_zedit.length) {
        uint32_t last = at;
        while (last < g_zedit.length && g_zedit.text[last] != '\n') last++;
        if (last < g_zedit.length) last++;
        if (current == number) {
            *start = at; *end = last;
            return 1;
        }
        current++;
        at = last;
    }
    return 0;
}

static uint32_t zedit_page_rows(void) {
    uint32_t rows = g_fb_rows < ZEAL_TEXT_MAX_ROWS ? g_fb_rows : ZEAL_TEXT_MAX_ROWS;
    return rows > 5u ? rows - 5u : 1u;
}

static void zedit_reveal_cursor(void) {
    uint32_t capacity = zedit_page_rows();
    uint32_t visible = g_zedit.view_count ? g_zedit.view_count : capacity;
    if (visible > capacity) visible = capacity;
    if (g_zedit.cursor_line < g_zedit.view_first)
        g_zedit.view_first = g_zedit.cursor_line;
    else if (visible && g_zedit.cursor_line >= g_zedit.view_first + visible)
        g_zedit.view_first = g_zedit.cursor_line - visible + 1u;
}

/* Replace [start,end) with one input line. At EOF, add a separator if the
 * existing last line has no newline. Move the NUL terminator with the tail. */
static int zedit_splice(uint32_t start, uint32_t end, const char *body,
                        uint32_t body_len, int append_eof) {
    uint32_t prefix = append_eof && g_zedit.length &&
                      g_zedit.text[g_zedit.length - 1] != '\n';
    uint32_t inserted = prefix + body_len + 1u;
    uint32_t removed = end - start;
    if (inserted > ZSS_MAX_FILE ||
        g_zedit.length - removed > ZSS_MAX_FILE - inserted) return -1;
    uint32_t destination = start + inserted;
    if (destination > end) {
        for (uint32_t i = g_zedit.length + 1u; i > end; i--)
            g_zedit.text[destination + i - end - 1u] = g_zedit.text[i - 1u];
    } else {
        for (uint32_t i = end; i <= g_zedit.length; i++)
            g_zedit.text[destination + i - end] = g_zedit.text[i];
    }
    uint32_t at = start;
    if (prefix) g_zedit.text[at++] = '\n';
    for (uint32_t i = 0; i < body_len; i++) g_zedit.text[at++] = body[i];
    g_zedit.text[at] = '\n';
    g_zedit.length = g_zedit.length - removed + inserted;
    g_zedit.dirty = 1;
    return 0;
}

static int zedit_delete_line(uint32_t number) {
    uint32_t start, end;
    if (!zedit_bounds(number, &start, &end)) return -1;
    for (uint32_t i = end; i <= g_zedit.length; i++)
        g_zedit.text[start + i - end] = g_zedit.text[i];
    g_zedit.length -= end - start;
    g_zedit.dirty = 1;
    return 0;
}

static int zedit_number(const char **input, uint32_t *out) {
    const char *p = *input;
    uint32_t value = 0;
    if (*p < '0' || *p > '9') return 0;
    while (*p >= '0' && *p <= '9') {
        uint32_t digit = (uint32_t)(*p++ - '0');
        if (value > (UINT32_MAX - digit) / 10u) return 0;
        value = value * 10u + digit;
    }
    if (!value) return 0;
    *input = p; *out = value;
    return 1;
}

static void zedit_print(uint32_t first, uint32_t count) {
    uint32_t lines = zedit_lines();
    if (!lines) { con_puts("zedit: empty\n"); return; }
    if (!first || first > lines) { con_puts("zedit: line out of range\n"); return; }
    if (count > 50u) count = 50u;
    for (uint32_t n = first; n <= lines && n - first < count; n++) {
        uint32_t start, end, printed = 0;
        if (!zedit_bounds(n, &start, &end)) break;
        zc_print_decimal(n); con_puts(": ");
        for (uint32_t i = start; i < end && g_zedit.text[i] != '\n'; i++) {
            if (printed++ == 160u) { con_puts("..."); break; }
            con_write(g_zedit.text[i]);
        }
        con_puts("\n");
    }
}

static int zedit_save(void) {
    if (!g_zedit.dirty && g_zedit.existed) {
        con_puts("zedit: unchanged\n"); return 0;
    }
    if (!g_zedit.length) {
        con_puts("zedit: empty source; use zrm to delete a file\n"); return -1;
    }
    int result = zss_put(g_zedit.name, g_zedit.text, g_zedit.length);
    if (result) {
        con_puts(result == -3 ? "zedit: source partition full\n" :
                 result == -2 ? "zedit: source partition unavailable\n" :
                                "zedit: save failed; editor remains open\n");
        return -1;
    }
    g_zedit.dirty = 0;
    g_zedit.existed = 1;
    con_puts("zedit: saved "); con_puts(g_zedit.name);
    con_puts(" bytes="); zc_print_decimal(g_zedit.length); con_puts("\n");
    return 0;
}

static void zedit_help(void) {
    con_puts("zedit: j/k move | [/] page | g/G top/bottom | e <text> edit | n <text> insert | x delete | "
             "p [line [count]] | a <text> | i <line> <text> | c <line> <text> | "
             "d <line> | w | wq | q | q!\n");
    con_puts("zedit: lines are numbered from 1; p defaults to the first 20\n");
}

static void zedit_start(const char *name) {
    struct zss_entry entry;
    size_t length = 0;
    if (!zss_name_ok(name, NULL)) {
        con_puts("zedit: use zedit path/to/File.ZC\n"); return;
    }
    int found = zss_find(name, &entry, NULL, NULL);
    if (found == -2) {
        con_puts("zedit: native source partition unavailable\n"); return;
    }
    if (found == -1) {
        con_puts("zedit: damaged source catalog\n"); return;
    }
    if (found == 1 && zss_read(&entry, g_zedit.text,
                                sizeof(g_zedit.text), &length)) {
        con_puts("zedit: source checksum or read failed\n"); return;
    }
    if (found == 1) {
        for (size_t i = 0; i < length; i++) {
            if (!g_zedit.text[i]) {
                con_puts("zedit: source contains a NUL byte\n"); return;
            }
        }
    }
    if (found != 1) g_zedit.text[0] = 0;
    g_zedit.length = found == 1 ? (uint32_t)length : 0;
    for (unsigned i = 0; i <= ZSS_MAX_NAME; i++) {
        g_zedit.name[i] = name[i];
        if (!name[i]) break;
    }
    g_zedit.active = 1;
    g_zedit.dirty = 0;
    g_zedit.existed = found == 1;
    g_zedit.view_first = 1;
    g_zedit.view_count = 0;
    g_zedit.cursor_line = 1;
    con_puts(found == 1 ? "zedit: opened " : "zedit: new ");
    con_puts(name); con_puts(" lines="); zc_print_decimal(zedit_lines());
    con_puts(" bytes="); zc_print_decimal(g_zedit.length); con_puts("\n");
    zedit_help();
    zedit_render();
}

static void zedit_handle_command(const char *line) {
    if (!line[0] || zss_str_eq(line, "h") || zss_str_eq(line, "help")) {
        zedit_help(); return;
    }
    if (zss_str_eq(line, "q") || zss_str_eq(line, "q!")) {
        if (g_zedit.dirty && line[1] != '!') {
            con_puts("zedit: unsaved changes; use wq or q!\n"); return;
        }
        int discarded = g_zedit.dirty;
        g_zedit.active = 0;
        con_puts(discarded ? "zedit: discarded changes\n" :
                             "zedit: closed\n"); return;
    }
    if (zss_str_eq(line, "w") || zss_str_eq(line, "wq")) {
        if (!zedit_save() && line[1] == 'q') g_zedit.active = 0;
        return;
    }
    if (line[0] == 'p' && (line[1] == 0 || line[1] == ' ')) {
        uint32_t first = 1, count = 20;
        const char *p = line + 1;
        if (*p == ' ') {
            while (*p == ' ') p++;
            if (!zedit_number(&p, &first)) { zedit_help(); return; }
            if (*p == ' ') {
                while (*p == ' ') p++;
                if (!zedit_number(&p, &count)) { zedit_help(); return; }
            }
            if (*p) { zedit_help(); return; }
        }
        g_zedit.view_first = first;
        g_zedit.view_count = count;
        if (first <= zedit_lines()) g_zedit.cursor_line = first;
        zedit_print(first, count); return;
    }
    if (zss_str_eq(line, "j") || zss_str_eq(line, "k") ||
        zss_str_eq(line, "[") || zss_str_eq(line, "]") ||
        zss_str_eq(line, "g") || zss_str_eq(line, "G")) {
        uint32_t lines = zedit_lines();
        if (zss_str_eq(line, "j")) {
            if (g_zedit.cursor_line < lines) g_zedit.cursor_line++;
        } else if (zss_str_eq(line, "k")) {
            if (g_zedit.cursor_line > 1) g_zedit.cursor_line--;
        } else if (zss_str_eq(line, "[")) {
            uint32_t step = zedit_page_rows();
            g_zedit.cursor_line = g_zedit.cursor_line > step ?
                                  g_zedit.cursor_line - step : 1u;
        } else if (zss_str_eq(line, "]")) {
            uint32_t step = zedit_page_rows();
            if (lines) {
                uint32_t remaining = lines - g_zedit.cursor_line;
                g_zedit.cursor_line += step < remaining ? step : remaining;
            }
        } else if (zss_str_eq(line, "g")) {
            g_zedit.cursor_line = 1;
        } else if (lines) {
            g_zedit.cursor_line = lines;
        }
        if (g_zedit.cursor_line > lines && lines) g_zedit.cursor_line = lines;
        if (!lines) g_zedit.cursor_line = 1;
        zedit_reveal_cursor();
        con_puts("zedit: cursor line ");
        zc_print_decimal(g_zedit.cursor_line); con_puts("\n");
        return;
    }
    if (line[0] == 'e' && line[1] == ' ') {
        uint32_t start, end, body_len = 0;
        const char *body = line + 2;
        while (body[body_len]) body_len++;
        if (!zedit_bounds(g_zedit.cursor_line, &start, &end)) {
            con_puts("zedit: cursor line out of range\n"); return;
        }
        if (zedit_splice(start, end, body, body_len, 0)) {
            con_puts("zedit: file exceeds 4 MiB\n"); return;
        }
        con_puts("zedit: edited line "); zc_print_decimal(g_zedit.cursor_line); con_puts("\n");
        return;
    }
    if (line[0] == 'n' && line[1] == ' ') {
        uint32_t start, end, body_len = 0;
        const char *body = line + 2;
        uint32_t lines = zedit_lines();
        while (body[body_len]) body_len++;
        if (g_zedit.cursor_line == lines + 1u) {
            start = end = g_zedit.length;
        } else if (!zedit_bounds(g_zedit.cursor_line, &start, &end)) {
            con_puts("zedit: cursor line out of range\n"); return;
        }
        if (zedit_splice(start, start, body, body_len,
                         g_zedit.cursor_line == lines + 1u)) {
            con_puts("zedit: file exceeds 4 MiB\n"); return;
        }
        con_puts("zedit: inserted line "); zc_print_decimal(g_zedit.cursor_line); con_puts("\n");
        return;
    }
    if (zss_str_eq(line, "x")) {
        if (zedit_delete_line(g_zedit.cursor_line)) {
            con_puts("zedit: cursor line out of range\n"); return;
        }
        uint32_t lines = zedit_lines();
        if (g_zedit.cursor_line > lines && lines) g_zedit.cursor_line = lines;
        if (!lines) g_zedit.cursor_line = 1;
        con_puts("zedit: deleted selected line\n");
        return;
    }
    if (line[0] == 'a' && (line[1] == 0 || line[1] == ' ')) {
        const char *body = line[1] ? line + 2 : line + 1;
        uint32_t length = 0;
        while (body[length]) length++;
        uint32_t next = zedit_lines() + 1u;
        if (zedit_splice(g_zedit.length, g_zedit.length, body, length, 1))
            con_puts("zedit: file exceeds 4 MiB\n");
        else {
            con_puts("zedit: added line "); zc_print_decimal(next); con_puts("\n");
        }
        return;
    }
    if ((line[0] == 'i' || line[0] == 'c' || line[0] == 'd') && line[1] == ' ') {
        char command = line[0];
        const char *p = line + 2;
        uint32_t number, start, end;
        if (!zedit_number(&p, &number)) { zedit_help(); return; }
        if (command == 'd') {
            if (*p || zedit_delete_line(number)) con_puts("zedit: line out of range\n");
            else {
                if (number < g_zedit.cursor_line) g_zedit.cursor_line--;
                uint32_t remaining = zedit_lines();
                if (!remaining) g_zedit.cursor_line = 1;
                else if (g_zedit.cursor_line > remaining) g_zedit.cursor_line = remaining;
                con_puts("zedit: deleted line "); zc_print_decimal(number); con_puts("\n");
            }
            return;
        }
        if (*p != ' ') { zedit_help(); return; }
        p++;
        uint32_t body_len = 0;
        while (p[body_len]) body_len++;
        uint32_t lines = zedit_lines();
        if (command == 'i' && number == lines + 1u) {
            start = end = g_zedit.length;
        } else if (!zedit_bounds(number, &start, &end)) {
            con_puts("zedit: line out of range\n"); return;
        }
        if (zedit_splice(start, command == 'i' ? start : end, p, body_len,
                         command == 'i' && number == lines + 1u)) {
            con_puts("zedit: file exceeds 4 MiB\n"); return;
        }
        if (command == 'i' && number <= g_zedit.cursor_line)
            g_zedit.cursor_line++;
        con_puts(command == 'i' ? "zedit: inserted line " : "zedit: changed line ");
        zc_print_decimal(number); con_puts("\n"); return;
    }
    zedit_help();
}

static void zedit_handle(const char *line) {
    zedit_handle_command(line);
    if (g_zedit.active) {
        zedit_render();
    } else if (g_fb) {
        /* Leave a clean shell canvas after closing the modal editor. */
        fb_clear(0x00101820u);
    }
}
