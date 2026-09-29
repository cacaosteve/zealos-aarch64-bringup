/* Modal line editor for the native source partition. Included by kernel_stub.c
 * after the console and source-store helpers; not part of the Pi diagnostic. */
#pragma once

static struct {
    char name[ZSS_MAX_NAME + 1];
    uint32_t length;
    int active, dirty, existed;
    char text[ZSS_MAX_FILE + 1u];
} g_zedit;

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
    con_puts("zedit: p [line [count]] | a <text> | i <line> <text> | "
             "c <line> <text> | d <line> | w | wq | q | q!\n");
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
    con_puts(found == 1 ? "zedit: opened " : "zedit: new ");
    con_puts(name); con_puts(" lines="); zc_print_decimal(zedit_lines());
    con_puts(" bytes="); zc_print_decimal(g_zedit.length); con_puts("\n");
    zedit_help();
}

static void zedit_handle(const char *line) {
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
        zedit_print(first, count); return;
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
            else { con_puts("zedit: deleted line "); zc_print_decimal(number); con_puts("\n"); }
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
        con_puts(command == 'i' ? "zedit: inserted line " : "zedit: changed line ");
        zc_print_decimal(number); con_puts("\n"); return;
    }
    zedit_help();
}
