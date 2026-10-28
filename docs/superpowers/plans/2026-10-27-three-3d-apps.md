# Three 3D Applications on ARM64 UTM Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Run the original GrModels, KeepAway, and Titanium applications in the ARM64 UTM image using the existing software sprite renderer.

**Architecture:** Pin and extract only the upstream sprite helpers this batch needs, then expose them through a bounded ARM64 3D sprite bridge that reuses the current sprite record parser, renderer, and fixed-point matrix support. Add API tests and one application-level gate that launches all three original Apps entries, checks their visible behavior and exit path, and builds a distinct candidate ISO.

**Tech Stack:** C runtime and native ARM64 HolyC compiler; ZealOS packed sprite records; Python/QMP QEMU/HVF compatibility harness; USB-CD ISO packaging.

**Spec:** [2026-10-27-three-3d-apps-design.md](../specs/2026-10-27-three-3d-apps-design.md)

## Global Constraints

- Keep the original packed sprite data and application source unchanged.
- Implement only the APIs required by this batch; do not imply broad 3D support beyond tested sprite types.
- Use a new candidate ISO filename and record its SHA-256 and test results.
- Determine the exact user input sequence for each app from its original menu/help text while implementing the checks.
- Treat persistence of high scores or other app-specific data as outside this batch unless an app cannot reach its basic usable workflow without it.
- Preserve prior Budget, Digital Logic, Doodle, Mass Spring, Life, editor, install, and persistence acceptance.

## Review Focus

1. Mesh vertex/triangle counts that overflow an allocation or exceed the rasterizer's bounded limits — test zero, normal, and over-limit counts in Task 2.
2. Sprite streams with truncated variable records or mismatched frame types — test malformed and incompatible records in Task 3.
3. Interpolation at `t=0`, `t=0.5`, `t=1`, and outside the documented interval — test endpoints, midpoint, and bounded handling in Task 3.
4. Identity, rotation, scale, and translation matrices with values outside the supported transform range — test stable identity and expected transformed bounds in Task 4.
5. App exit after repeated launch or input, which can leave the terminal or desktop unresponsive — test per-app launch, action, Escape, terminal smoke, and `zstatus` in Task 5.

---

### Task 1: Add failing 3D sprite and app acceptance gates

**Files:**
- Create: `tests/compat/Sprite3DServices.ZC`
- Modify: `scripts/check-compat.py`
- Modify: `tests/compat/ProgramCatalog.ZC` only if a reusable compile helper needs an additional assertion

**Interfaces:**
- Consumes: Current `/System/Gr/SpriteBridge.ZC`, `/System/Gr/SpriteRaster.ZC`, `StockCatalogCompile`, and the test harness's existing QMP pointer, keyboard, screenshot, and text-plane helpers.
- Produces: `Sprite3DServiceChecks()` returning `0x2a` when packed-size, interpolation, and matrix-render checks pass; `python3 scripts/check-compat.py --pci --probe-3d-apps` as the focused application gate.

- [x] **Step 1: Add service assertions for the sprite end record and mesh headers.** Assert `sprite_elem_base_sizes[SPT_END] == 1`, `sprite_elem_base_sizes[SPT_MESH] == 12`, and `sprite_elem_base_sizes[SPT_SHIFTABLE_MESH] == 24`; assert a mesh with `v` vertices and `t` triangles has size `12 + 12*v + 16*t`.
- [x] **Step 2: Add interpolation assertions.** Build two same-layout line/point sprites with different coordinates, call `SpriteInterpolate` at `0`, `0.5`, and `1`, and assert the copied/interpolated coordinates and unchanged record types. Add a mismatched-type case that must raise a catchable graphics error.
- [x] **Step 3: Add matrix-render assertions.** Render a tiny mesh with identity and 90-degree rotation transforms into a test CDC; assert identity preserves its expected pixel bounds and the rotation changes those bounds without stopping presentation.
- [x] **Step 4: Add the focused gate and confirm it fails on the known blockers.** The gate must compile `/Apps/GrModels/Run.ZC`, `/Apps/KeepAway/Run.ZC`, and `/Apps/Titanium/Run.ZC` in isolated namespaces, then report each blocker instead of treating `CCF_JUST_LOAD` as runtime support. Run `python3 scripts/check-compat.py --pci --probe-3d-apps`; expected initial failures name `sprite_elem_base_sizes`, `SpriteInterpolate`, and `Sprite3Mat4x4B` (or an earlier missing dependency).

### Task 2: Restore sprite record sizes and bounded mesh construction

**Files:**
- Create: `upstream/pinned/System/Gr/SpriteNew.ZC` if the exact original table is not already pinned
- Modify: `upstream/pinned/UPSTREAM.json`
- Modify: `scripts/arm64_source.py`
- Modify: `scripts/build-source-bundle.py`
- Modify: `scripts/check-arm64-source.py`
- Modify: `upstream/bootstrap/System/Gr/SpriteBridge.ZC` or create `upstream/bootstrap/System/Gr/Sprite3DBridge.ZC`
- Modify: `src/zc/runtime.c` only if a new bridge module must be loaded during graphics startup
- Modify: `tests/compat/Sprite3DServices.ZC`

**Interfaces:**
- Consumes: `SpriteElemSize(U8 *p, I64 type)` and the stable `SPT_MESH`/`SPT_SHIFTABLE_MESH` packed records.
- Produces: `I64 sprite_elem_base_sizes[SPT_TYPES_NUM]` with upstream-compatible base sizes; the existing bounded `SpriteElemSize` walker remains the authority for variable-size mesh records.

- [x] **Step 1: Extend the failing fixture.** Assert that the declared table indexes through `SPT_TYPES_NUM-1`, check its end, mesh, and shiftable-mesh base sizes against the pinned upstream definitions, and verify `SpriteElemSize` computes a normal mesh's byte size while rejecting negative and over-limit vertex/triangle counts.
- [x] **Step 2: Run the focused fixture and confirm it fails before the table/service is present.** Use `python3 scripts/check-compat.py --pci --probe-3d-apps`; expected result is a missing table/helper or failed assertion, not a compiler crash.
- [x] **Step 3: Pin the upstream `SpriteNew.ZC` source at the existing ZealOS upstream revision.** Add its SHA-256 to `upstream/pinned/UPSTREAM.json` and keep the source byte-identical.
- [x] **Step 4: Generate the upstream-compatible base-size table.** Translate only the required declaration from the pinned source and confirm the existing `SpriteElemSize` validation prevents invalid mesh counts and arithmetic overflow during record traversal.
- [x] **Step 5: Re-run the service gate and the three-app compile gate.** GrModels must pass the `sprite_elem_base_sizes` failure point and produce its mesh sprite allocation safely.

### Task 3: Add upstream-compatible sprite interpolation

**Files:**
- Create: `upstream/pinned/System/Gr/GrSpritePlot.ZC` if the upstream source is not already pinned
- Modify: `upstream/pinned/UPSTREAM.json`
- Modify: `scripts/arm64_source.py`
- Modify: `scripts/build-source-bundle.py`
- Modify: `scripts/check-arm64-source.py`
- Modify: `upstream/bootstrap/System/Gr/SpriteBridge.ZC` or `upstream/bootstrap/System/Gr/Sprite3DBridge.ZC`
- Modify: `tests/compat/Sprite3DServices.ZC`

**Interfaces:**
- Consumes: `SpriteElemSize`, the base-size table from Task 2, and the pinned upstream interpolation behavior.
- Produces: bounded `I64 SpriteSize(U8 *elems)` for zero-terminated sprite streams and `U8 *SpriteInterpolate(F64 t, U8 *elems0, U8 *elems1)` returning an owned sprite stream or raising a catchable graphics error for incompatible/truncated streams.

- [x] **Step 1: Add assertions for copied fields and interpolated coordinates at `t=0`, `0.5`, and `1`.** Include bitmap/mesh records that must be copied byte-for-byte and a mismatched record sequence that must fail safely.
- [x] **Step 2: Run `python3 scripts/check-compat.py --pci --probe-3d-apps` and confirm the `SpriteInterpolate` fixture fails before implementation.** The desktop must remain responsive and `zstatus` must show a catchable error rather than a failed native call.
- [x] **Step 3: Pin `GrSpritePlot.ZC` and generate only `SpriteInterpolate` plus the minimal record-walk logic it needs.** Preserve upstream interpolation rules for supported record types, cap record counts and byte traversal, and return caller-owned memory.
- [x] **Step 4: Re-run the interpolation assertions.** The PCI/QEMU gate compiles and runs the line, bitmap, mesh, malformed-stream, and invalid-`t` fixture; it returns `0x2a`. KeepAway's animated app check moves to Task 5 because its ordinary entry also needs Task 4's `Sprite3YB` transform drawing API.

### Task 4: Add transformed 3D sprite drawing

**Files:**
- Modify: `upstream/bootstrap/System/Gr/SpriteBridge.ZC` or `upstream/bootstrap/System/Gr/Sprite3DBridge.ZC`
- Modify: `upstream/bootstrap/System/Gr/SpriteRaster.ZC` only if the existing `DCF_TRANSFORMATION` raster path lacks a required bounded mesh case
- Modify: `upstream/bootstrap/System/TaskBridge.ZC` only if matrix composition or translation helpers are absent
- Modify: `src/zc/runtime.c` only if the bridge module load order changes
- Modify: `tests/compat/Sprite3DServices.ZC`

**Interfaces:**
- Consumes: `Sprite3(CDC *dc, I64 x, I64 y, I64 z, U8 *sprite)`, `Mat4x4IdentEqu`, `Mat4x4RotX/Y/Z`, the existing fixed-point matrix representation, and bounded mesh rasterization.
- Produces: `U0 Sprite3Mat4x4B(CDC *dc=gr.dc, I64 x, I64 y, I64 z, U8 *elems, I64 *m)` matching the original call signature, restoring CDC transform state on normal return and catchable failure.

- [ ] **Step 1: Add an identity transform regression and assert the untransformed bounds.** Add a rotation/translation case with exact expected bounds and ensure the CDC flags/matrix are restored after drawing.
- [ ] **Step 2: Run the 3D service gate and confirm transformed drawing fails before implementation while the existing 2D sprite checks still pass.** Use `python3 scripts/check-compat.py --pci --probe-3d-apps` and `python3 scripts/check-compat.py --pci --probe-stock-interactive`.
- [ ] **Step 3: Implement `Sprite3Mat4x4B` against the existing matrix representation.** Compose matrix and origin in the same order as pinned upstream; preserve the caller's CDC flags, origin, and matrix on every exit path.
- [ ] **Step 4: Extend mesh rasterization only for the mesh record variants the three applications use.** Keep vertex, triangle, work, and transformed-coordinate bounds enforced; do not add unneeded sprite types.
- [ ] **Step 5: Re-run transform assertions and compile all three Apps entries.** The focused compile gate must report success for GrModels, KeepAway, and Titanium.

### Task 5: Exercise the three original applications and produce the candidate ISO

**Files:**
- Modify: `scripts/check-compat.py`
- Modify: `tests/compat/Sprite3DServices.ZC` if app-level assertions are shared there
- Create: `docs/acceptance/utm-3d-apps-2026-10-27.json`
- Modify: `PROGRAMS.md`
- Modify: `ACCEPTANCE.md`
- Create: `build/zealos-aarch64-3d-apps.iso`

**Interfaces:**
- Consumes: the three completed sprite APIs, the pinned original app entry points, and existing QEMU/UTM QMP/QTest input helpers.
- Produces: `python3 scripts/check-compat.py --pci --probe-3d-apps`, `python3 scripts/check-compat.py --usb-input nec --probe-3d-apps`, and `build/zealos-aarch64-3d-apps.iso` with a recorded SHA-256 and live UTM status.

- [ ] **Step 1: Add app-level checks using the now-implemented services.** For GrModels, navigate its original Man and Ball forms, finish one generation of each, and assert the produced mesh sprite is visible. For KeepAway and Titanium, capture initial frames, wait for updates, exercise one documented control, send Escape, and assert the desktop continues presenting.
- [ ] **Step 2: Run the focused PCI and NEC gates.** Require all three ordinary Apps entries to compile and launch; no unresolved external, crash, or frozen presentation; each exits to a terminal where `40+2;` returns `42`.
- [ ] **Step 3: Build the candidate to the distinct ISO path.** Use `make -j4 iso ISO=/Users/steve/github/zealos-aarch64-bringup/build/zealos-aarch64-3d-apps.iso`; verify its kernel, booter, source bundle, and ISO hashes against the acceptance report.
- [ ] **Step 4: Re-run existing acceptance gates.** Run `python3 scripts/check-compat.py --pci --probe-logic-program`, `python3 scripts/check-compat.py --usb-input nec --probe-budget-program`, `python3 scripts/check-compat.py --pci --probe-stock-interactive`, `python3 scripts/check-compat.py --usb-input nec --probe-gui-workflow`, `python3 scripts/check-iso-installer.py --usb-cd build/zealos-aarch64-3d-apps.iso`, and `python3 scripts/check-upstream.py`.
- [ ] **Step 5: Record limitations and live status.** Update `PROGRAMS.md`, `ACCEPTANCE.md`, and the JSON report with exact API scope, gate counts, artifact hashes, and whether live UTM checks are pending or user-confirmed. Do not mark an app accepted based only on compilation or QEMU.
- [ ] **Step 6: Commit and push the completed, verified source change.** Stage only source, pinned provenance, tests, and docs; do not stage generated logs or ISO files. Push the requested repo branch after every automated gate passes; leave the ISO as a local build artifact.
