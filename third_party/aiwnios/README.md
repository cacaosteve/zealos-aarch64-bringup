# Freestanding compiler import

Pinned source and original SHA-256 hashes: `UPSTREAM.json`. License: `LICENSE`.
Only the lexer, parser, optimizer, symbol tables, ARM64 emitter and FFI bridge
are imported. No SDL, hosted filesystem, audio, threads, HCRT or task scheduler.

`PORT.patch` records every modification to imported source. `make check-upstream`
reverses it in a temporary directory and checks every original file's hash.
Normal builds use vendored files and need neither a sibling checkout nor network.

Port changes:

- Select Aiwnios' non-Darwin ARM register convention on freestanding ELF.
- Initialize keywords per symbol table so a discarded session can restart.
- Resolve absolute includes without adding the current source directory.
- Avoid `log10` when reading decimal literals; use integer digit counting.
- Bound diagnostic and include-path formatting; handle diagnostics at EOF.
- Preserve relational chains through constant folding (the backend implements
  HolyC chained comparisons; folding an inner compare changed their meaning).
- Accept UTF-8 identifier bytes used in upstream source and consume file-scope
  `public` declarations in the single global symbol table.
- Evaluate `#assert` and fail a module when its layout expression is false.
- Support class-scoped `$$` layout offsets, including member placement and
  trailing class size; reject negative or oversized offsets.
- Restore preprocessor branch state for false `#if` conditions and accept
  ZealOS's quoted `#define "Help/Category"` metadata form.
- Parse ZealOS `_intern` and named `_extern`/`_import` declarations. Preserve
  an existing host function binding only when the redeclared signature agrees;
  reject calls to unbound ARM64 intrinsics or named external symbols, and use
  of unbound external data.
- Reset ARM64 emitter instruction tracking for each top-level IR instruction;
  otherwise a stale pointer can overwrite a reused label and crash cleanup.
- Reject guest try/catch until its runtime exists.
- Reject FS/GS accesses: task segment semantics are not implemented here.
- Emit declaration-time stores for complete one-dimensional local scalar array
  brace initializers, so values can come from the current call frame.
- Do not use `ic_class-1` / `ic_class[-1]` on function-pointer types: those are
  single `CHashFun` objects, not the `CHashClass` star array. Indexing
  `U0 (**fp)(I64)` (as in KeyDev's ctrl-alt table) keeps `RT_FUNC` and uses
  the funptr's own pointer width.
- Type multi-star funptrs (`(**fp)(...)`) as `RT_PTR` so member loads are
  kept; only single-star funptrs stay `RT_FUNC`. Otherwise `fp[i]` writes at
  the member's address and smashes adjacent globals.

The freestanding C support and host API are in `src/zc/`. See
`COMPATIBILITY.md` for the tested scope and deliberate runtime limitations.
