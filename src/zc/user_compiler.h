/* Private ABI for the pinned ExeCmdLine/UserTaskCont loops. Included after
 * the runtime's compiler checkpoint helpers. Never expose Aiwnios objects as
 * CCompCtrl: the public mirror has the original ZealOS layout. */
struct user_compiler {
    struct user_compiler *next;
    unsigned char *mirror;
    CLexer *lex;
    CCmpCtrl *native;
    char *input, *code;
    void *doc_input, *doc_reader, *doc_delete;
    char *doc_filename;
    int64_t doc_position, doc_last;
    int primed;
};
static void **user_doc_state(CLexFile *file) {
    return ((struct user_compiler *)file->port_data)->doc_input;
}
static struct user_compiler *user_compiler_find(void *mirror) {
    for (struct user_compiler *c = guest_fibers[guest_current].user_compilers;
         c; c = c->next)
        if (c->mirror == mirror) return c;
    guest_raise(GUEST_EXCEPT_COMPILER);
    return NULL;
}
static void user_compiler_delete(struct guest_fiber *f, struct user_compiler *c) {
    struct user_compiler **link = &f->user_compilers;
    while (*link && *link != c) link = &(*link)->next;
    if (!*link) return;
    *link = c->next;
    free(c->code);
    free(c->input);
    if (c->doc_delete) FFI_CALL_TOS_1(c->doc_delete, (int64_t)(uintptr_t)c->doc_input);
    free(c->doc_filename);
    if (c->native) compiler_source_cleanup(c->native, c->lex);
    guest_context_files_clear(c->mirror);
    /* Self-linked before insertion; QueueRemove clears both links. Abort
     * cleanup also accepts a controller still linked in the task's queue. */
    unsigned char *next = *(unsigned char **)c->mirror;
    unsigned char *last = *(unsigned char **)(c->mirror + 8);
    if (next && last && next != c->mirror && *(void **)last == c->mirror &&
        *(void **)(next + 8) == c->mirror) {
        *(void **)last = next;
        *(void **)(next + 8) = last;
    }
    free(c->mirror);
    free(c);
}
static void user_compilers_clear(struct guest_fiber *f) {
    while (f->user_compilers) user_compiler_delete(f, f->user_compilers);
}
static int64_t host_user_compiler_new(int64_t *a) {
    if (compiler_recovery_active) AIWNIOS_throw(0);
    struct guest_fiber *f = &guest_fibers[guest_current];
    unsigned depth = 0;
    for (struct user_compiler *c = f->user_compilers; c; c = c->next) depth++;
    if (depth >= 8) guest_raise(GUEST_EXCEPT_COMPILER);
    struct user_compiler *c = __AIWNIOS_CAlloc(sizeof(*c), &f->heap_owner);
    c->mirror = __AIWNIOS_CAlloc(guest_cc_size, &f->heap_owner);
    *(void **)c->mirror = *(void **)(c->mirror + 8) = c->mirror;
    *(int64_t *)(c->mirror + guest_cc_flags_off) = a[1] | 0x008;
    unsigned char *head = (unsigned char *)f->words + guest_next_cc_off;
    unsigned char *tail = *(unsigned char **)(head + 8);
    *(int64_t *)(c->mirror + guest_cc_opts_off) = tail == head ? 0x24 :
        *(int64_t *)(tail + guest_cc_opts_off);
    if (a[0]) c->input = __AIWNIOS_StrDup((char *)(uintptr_t)a[0], &f->heap_owner);
    c->next = f->user_compilers;
    f->user_compilers = c;
    return (int64_t)(uintptr_t)c->mirror;
}
static int64_t host_user_compiler_del(int64_t *a) {
    if (compiler_recovery_active) AIWNIOS_throw(0);
    user_compiler_delete(&guest_fibers[guest_current],
                         user_compiler_find((void *)(uintptr_t)a[0]));
    return 0;
}
/* CArmDocInput begins with doc, current entry and one-based source line.
 * The source adapter owns traversal/quoting; native objects never alias it. */
static int64_t user_doc_read(CLexer *lex, CLexFile *file) {
    struct user_compiler *c = file->port_data;
    if (file->pos + 1 == c->doc_position) return c->doc_last;
    if (file->pos != c->doc_position) AIWNIOS_throw(0);
    int64_t ch = FFI_CALL_TOS_2(c->doc_reader, (int64_t)(uintptr_t)c->doc_input,
                              lex->port_in_quotes);
    if (ch < 0 || ch > 255) AIWNIOS_throw(0);
    file->ln = ((int64_t *)c->doc_input)[2] - 1;
    c->doc_position++;
    c->doc_last = ch;
    return ch;
}
static int64_t host_user_doc_attach(int64_t *a) {
    if (compiler_recovery_active) AIWNIOS_throw(0);
    struct user_compiler *c = user_compiler_find((void *)(uintptr_t)a[0]);
    if (c->native || c->input || c->doc_input || !a[1] || !a[2] || !a[3])
        guest_raise(GUEST_EXCEPT_COMPILER);
    c->doc_input = (void *)(uintptr_t)a[1];
    c->doc_reader = (void *)(uintptr_t)a[2];
    c->doc_delete = (void *)(uintptr_t)a[3];
    c->doc_filename = __AIWNIOS_StrDup(a[4] ? (char *)(uintptr_t)a[4] : "<document>",
                                     &guest_fibers[guest_current].heap_owner);
    return 0;
}
/* Collect a logical chunk before parsing: the parser cannot Yield for more
 * input. This is only a delimiter/comment/quote completeness check, not a
 * second ZealC parser. Mismatched closing delimiters go straight to the real
 * compiler for its diagnostic. A chunk is limited to 64 KiB, never truncated. */
static int user_input_complete(const char *s) {
    char stack[256];
    unsigned depth = 0, comment = 0;
    int quote = 0, escaped = 0;
    for (size_t i = 0; s[i]; i++) {
        int ch = s[i], next = s[i + 1];
        if (comment) {
            if (ch == '/' && next == '*') { comment++; i++; }
            else if (ch == '*' && next == '/') { comment--; i++; }
        } else if (quote) {
            if (escaped) escaped = 0;
            else if (ch == '\\') escaped = 1;
            else if (ch == quote) quote = 0;
        } else if (ch == '/' && next == '/') {
            while (s[i] && s[i] != '\n') i++;
            if (!s[i]) break;
        } else if (ch == '/' && next == '*') { comment = 1; i++; }
        else if (ch == '"' || ch == '\'') quote = ch;
        else if (ch == '(' || ch == '[' || ch == '{') {
            if (depth == sizeof(stack)) return 1;
            stack[depth++] = ch;
        } else if (ch == ')' || ch == ']' || ch == '}') {
            if (!depth) return 1;
            int open = stack[--depth];
            if ((ch == ')' && open != '(') || (ch == ']' && open != '[') ||
                (ch == '}' && open != '{')) return 1;
        }
    }
    return !depth && !quote && !comment;
}
static int64_t host_user_input_complete(int64_t *a) {
    return a[0] && user_input_complete((const char *)(uintptr_t)a[0]);
}
struct user_parse_scope {
    struct guest_unwind unwind;
    struct source_context context;
    int previous_compiling;
};
static void user_parse_cleanup(struct guest_unwind *scope) {
    struct user_parse_scope *parse = (struct user_parse_scope *)scope;
    guest_task_exe_active = parse->previous_compiling;
    guest_fibers[guest_current].source_context = parse->context.previous;
    guest_fibers[guest_current].user_diagnostic_active = 0;
}
static void user_diagnostic_flush(struct guest_fiber *f) {
    if (f->user_diagnostic_length && f->user_output_writer)
        guest_task_text_output(f->user_diagnostic);
    f->user_diagnostic_length = 0;
}
static int64_t user_compiler_step(struct user_compiler *c, int prime, int64_t *type) {
    if (compiler_recovery_active) AIWNIOS_throw(0);
    struct guest_fiber *f = &guest_fibers[guest_current];
    if (!c->native) guest_raise(GUEST_EXCEPT_COMPILER);
    struct user_parse_scope parse = {.previous_compiling = guest_task_exe_active};
    parse.context.previous = f->source_context;
    parse.context.mirror = c->mirror;
    f->source_context = &parse.context;
    c->lex->port_options = (int64_t *)(c->mirror + guest_cc_opts_off);
    c->lex->port_sync = guest_context_sync;
    c->lex->port_fun_complete = guest_function_complete;
    f->user_diagnostic_length = 0;
    f->user_diagnostic_active = 1;
    jmp_buf landing;
    guest_unwind_enter(&parse.unwind, user_parse_cleanup, &landing);
    if (setjmp(landing)) {
        int64_t exception = parse.unwind.exception;
        guest_unwind_leave(&parse.unwind);
        guest_unwind_flags(&parse.unwind);
        user_diagnostic_flush(f);
        guest_raise(exception);
    }
    compiler_checkpoint_save();
    if (setjmp(compiler_recovery_guard)) {
        compiler_recovery_active = 0;
        compiler_checkpoint_restore();
        (*(int64_t *)(c->mirror + guest_cc_errors_off))++;
        compiler_recovery_errors++;
        int64_t exception = compiler_recovery_exception;
        guest_unwind_leave(&parse.unwind);
        guest_unwind_flags(&parse.unwind);
        zc_output("zc: compile failed; current statement discarded\n");
        user_diagnostic_flush(f);
        guest_raise(exception);
    }
    compiler_recovery_active = 1;
    guest_task_exe_active = 1;
    int64_t result = INVALID_PTR;
    if (prime) {
        Lex(c->lex);
        c->primed = 1;
        result = c->lex->cur_tok;
    } else {
        if (!c->primed || c->code) AIWNIOS_throw(0);
        if (!PrsStmt(c->native)) {
            if (c->lex->cur_tok) AIWNIOS_throw(0);
        } else {
            CRPN *head = (CRPN *)c->native->code_ctrl->ir_code;
            CRPN *first = (CRPN *)head->base.next;
            int explicit_return = first != head && first->type == IC_RET;
            if (explicit_return) first = (CRPN *)first->base.next;
            int64_t raw = first != head && first->type != IC_NOP ?
                AssignRawTypeToNode(c->native, first) : RT_U0;
            if (!explicit_return && raw > RT_UNDEF) {
                CRPN *ret = __AIWNIOS_CAlloc(sizeof(*ret), NULL);
                ret->type = IC_RET;
                QueIns((CQue *)ret, c->native->code_ctrl->ir_code);
            }
            c->code = Compile(c->native, NULL, NULL, NULL);
            if (type) *type = zeal_answer_type(raw);
            result = c->code ? (int64_t)(uintptr_t)c->code : INVALID_PTR;
            CodeCtrlPop(c->native);
            CodeCtrlPush(c->native);
            f->user_statements++;
        }
    }
    /* Native EOF ends a buffered chunk, not an interactive compiler session.
     * Keep the original ExeCmdLine loop alive; its next Statement2Bin call
     * obtains input outside the parser. Options then persist across prompts. */
    *(int64_t *)(c->mirror + guest_cc_token_off) = c->lex->cur_tok ? c->lex->cur_tok :
        ((*(int64_t *)(c->mirror + guest_cc_flags_off) & 0x002) && f->user_prompt_reader ? 1 : 0);
    guest_context_sync(c->lex);
    compiler_recovery_active = 0;
    guest_unwind_leave(&parse.unwind);
    user_diagnostic_flush(f);
    return result;
}
static int64_t host_user_lex(int64_t *a) {
    if (compiler_recovery_active) AIWNIOS_throw(0);
    struct user_compiler *c = user_compiler_find((void *)(uintptr_t)a[0]);
    struct guest_fiber *f = &guest_fibers[guest_current];
    if (c->native) guest_raise(GUEST_EXCEPT_COMPILER);
    /* Input may Yield; it must complete before entering the shared parser or
     * taking a checkpoint. The reader returns a task-owned, NUL-ended chunk. */
    if (!c->input && (*(int64_t *)(c->mirror + guest_cc_flags_off) & 0x002) &&
        f->user_prompt_reader) {
        int continuation = 0;
        do {
            char *line = (char *)(uintptr_t)FFI_CALL_TOS_1(f->user_prompt_reader, continuation);
            if (!line) { /* Escape cancels the pending chunk, not the loop. */
                free(c->input);
                c->input = __AIWNIOS_StrDup("", &f->heap_owner);
                if (!f->user_prompt_reader)
                    *(int64_t *)(c->mirror + guest_cc_flags_off) &= ~INT64_C(0x002);
                break;
            }
            size_t old = c->input ? strlen(c->input) : 0, len = strlen(line);
            if (len > 65534 || old > 65534 - len) {
                free(line);
                guest_raise(GUEST_EXCEPT_COMPILER);
            }
            char *joined = __AIWNIOS_CAlloc(old + len + 2, &f->heap_owner);
            if (old) memcpy(joined, c->input, old);
            memcpy(joined + old, line, len);
            joined[old + len] = '\n';
            free(line);
            free(c->input);
            c->input = joined;
            continuation = !user_input_complete(joined);
        } while (continuation);
    }
    if (!c->input && !c->doc_input) {
        *(int64_t *)(c->mirror + guest_cc_flags_off) &= ~INT64_C(0x002);
        *(int64_t *)(c->mirror + guest_cc_token_off) = 0;
        return 0;
    }
    c->lex = LexerNew(c->doc_input ? c->doc_filename : "<command>", c->input ? c->input : "");
    c->lex->file->is_file = 1;
    c->lex->file->dir = __AIWNIOS_StrDup((char *)guest_current_dir(), NULL);
    if (c->doc_input) {
        c->lex->file->port_read = user_doc_read;
        c->lex->file->port_data = c;
    }
    c->native = CmpCtrlNew(c->lex);
    c->native->flags |= CCF_STRINGS_ON_HEAP;
    CodeCtrlPush(c->native);
    free(c->input);
    c->input = NULL;
    return user_compiler_step(c, 1, NULL);
}
static int64_t host_user_statement(int64_t *a) {
    if (compiler_recovery_active) AIWNIOS_throw(0);
    struct user_compiler *c = user_compiler_find((void *)(uintptr_t)a[0]);
    if (c->native && !c->lex->cur_tok &&
        (*(int64_t *)(c->mirror + guest_cc_flags_off) & 0x002)) {
        compiler_source_cleanup(c->native, c->lex);
        c->native = NULL;
        c->lex = NULL;
        c->primed = 0;
        host_user_lex(a);
        if (!c->native || !c->lex->cur_tok) return INVALID_PTR;
    }
    return user_compiler_step(c, 0, (int64_t *)(uintptr_t)a[1]);
}
static int64_t host_user_code_free(int64_t *a) {
    if (compiler_recovery_active) AIWNIOS_throw(0);
    struct user_compiler *c = user_compiler_find((void *)(uintptr_t)a[0]);
    if (c->code != (char *)(uintptr_t)a[1]) guest_raise(GUEST_EXCEPT_COMPILER);
    free(c->code);
    c->code = NULL;
    return 0;
}
static int64_t host_user_prompt_set(int64_t *a) {
    if (compiler_recovery_active) AIWNIOS_throw(0);
    void *old = guest_fibers[guest_current].user_prompt_reader;
    guest_fibers[guest_current].user_prompt_reader = (void *)(uintptr_t)a[0];
    return (int64_t)(uintptr_t)old;
}
static int64_t host_user_output_set(int64_t *a) {
    if (compiler_recovery_active) AIWNIOS_throw(0);
    guest_fibers[guest_current].user_output_writer = (void *)(uintptr_t)a[0];
    return 0;
}
struct user_output_scope {
    struct guest_unwind unwind;
    int suppressed;
};
static void user_output_restore(struct guest_unwind *scope) {
    struct user_output_scope *output = (struct user_output_scope *)scope;
    guest_fibers[guest_current].user_output_suppressed = output->suppressed;
}
static int64_t host_gui_shell_exec(int64_t *a) {
    if (!guest_task_bound) return zc_gui_shell_exec(a);
    struct guest_fiber *f = &guest_fibers[guest_current];
    struct user_output_scope scope = {.suppressed = f->user_output_suppressed};
    guest_unwind_enter(&scope.unwind, user_output_restore, NULL);
    f->user_output_suppressed = 1;
    int64_t result = zc_gui_shell_exec(a);
    guest_unwind_leave(&scope.unwind);
    return result;
}
static int64_t host_user_put_except(int64_t *a) {
    (void)a;
    unsigned char *words = (unsigned char *)guest_fibers[guest_current].words;
    int64_t code = *(int64_t *)(words + guest_except_ch_off);
    char output[80];
    snprintf(output, sizeof(output), "Except: 0x%016lx\n", (unsigned long)code);
    zc_output(output);
    guest_task_text_output(output);
    words[guest_catch_except_off] = 1;
    return 0;
}
