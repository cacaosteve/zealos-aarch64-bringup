# ZealOS graphics and window-manager port

The references in `pinned/` are unchanged ZealOS sources from commit
`eee210f4379ba3d00b47507f336a158eefb2c993`. Their hashes are checked by
`scripts/check-upstream.py`. The bootstrap bridge is kept separate so ARM64
implementations can be compared against the x86 source without editing it.

## Source map

| Upstream source | Role | Current ARM64 status |
| --- | --- | --- |
| `Kernel/Display.ZC` | Raw text output and task pixel/window geometry | Limine framebuffer replaces boot-time mode discovery; `WinDerivedValsUpdate` is ported in `TaskBridge.ZC` |
| `System/Gr/GrGlobals.ZC` | `gr.text_base`, screen DCs, and compositor state | Bootstrap `gr.text_base` now points to a compact guest-owned `U32[100*75]` plane; upstream DCs and compositor state are not initialized |
| `System/Gr/GrTextBase.ZC` | `TextChar`, text spans/fills, and borders; the hot primitives are x86 assembly | ARM64 bridge primitives update `gr.text_base` and the framebuffer; a QEMU smoke proves direct packed-cell writes through the pointer are presented |
| `System/Gr/GrScreen.ZC` | Per-task redraw, controls, draw callbacks, z-buffer, and final screen update | The guarded ARM64 source overlay expands the x86 eight-row background store loop into eight typed stores in line; the stride helper is tested separately. AArch64 runtime bindings cover `DCBlotColor4` and the scalar `GrRopEquU8NoClipping` foreground-glyph rasterizer, with regressions for glyph bits, stride, transparency, and underline. Once the one-CPU task rings validate, the bridge dispatches unchanged `GrUpdateScreen`, which runs `GrUpdateTasks` and the source text passes; startup loads `KMathB.ZC` for its `Clamp` call. The staged full-frame text smoke passes with framebuffer-console output visible and serial output enabled. A bounded 100x75 cell z-buffer marks windows with any visible cells; each marked window's text, task `draw_it`, and visible control `draw_it` callbacks render in task-ring order. The final screen callback runs afterward with `DCF_ON_TOP`. Source zoom scaling is now checked end to end through the compositor and Limine framebuffer at 2x. View-angle controls, other `GrAsm` routines, and complete graphics-table initialization remain unported |
| `System/Win.ZC` | Window geometry, focus, and tiling operations | `WinHorz`/`WinVert`, derived geometry, bounded click focus/raise, title-bar move, and frame resize are ported; tiling and complete focus policies remain incomplete |
| `System/WinMgr.ZC` | Refresh loop, mouse routing, move/resize, and window-manager task | The unchanged `WinMgrTask` starts on demand with `winmgr` and stops with `winmgrstop`. `windowdemo` runs two bordered fixture windows with a CDC control through the source compositor. While the source manager is active, the bridge submits raw tablet samples to `mouse_hard` and leaves click/drag/control dispatch to ZealOS, avoiding duplicate routing through both paths. Focused live-manager QEMU probes verify task/control drawing, framebuffer pixels, captured right-click down/release, and wheel delta. Source-manager focus/move/resize and tablet-event paths pass QEMU probes. Full desktop initialization and manual UTM acceptance remain open |
| `System/Gr/MakeGr.ZC` | Graphics module include order | Reference only; its complete dependency set is not loaded by the bring-up runtime |

## First source-backed slice

`TextLenStr` and `TextLenAttrStr` return `Bool` in upstream ZealOS. The bridge
now preserves that contract and reports false when clipping removes the whole
span. `WinHorz` and `WinVert` now follow upstream edge normalization, update
`CTask` bounds, and recalculate the corresponding character and pixel extents.
The `TaskOriginalDocRecalc` probe checks partially and fully clipped text spans,
per-cell attributes, and the derived bounds for an overlapping child window.

## Canonical packed-cell plane

Bootstrap owns the cell array in guest memory, so unchanged ZealC code can
address `gr.text_base` using the upstream packed-`U32` layout. The ARM64 text
helpers and retained task/shell overlays update that same array; flush reads it
back as the presentation source. `TaskOriginalDocRecalcChecks` now writes a
cell directly through `gr.text_base` and checks its framebuffer background.
The plane is currently sized to the bring-up's 800x600 text grid. This does
not load the x86 assembly implementation or initialize upstream `gr.dc2`,
fonts, zoom, pan, blink, or compositor globals.

## Task drawing callback slice

The task redraw now processes visible windows in task-ring order. For each
window it refreshes DolDoc text into the canonical cell plane, flushes only
that window's text rectangle, then runs its `draw_it(task, dc)` callback before
advancing to the next window. This makes later windows cover earlier text and
graphics in overlaps. `WinZBufUpdate` rebuilds the 100x75 cell z-buffer and a
64-window uncovered bitmap; fully covered tasks are skipped while partially
visible ones still render. The callback receives a static screen CDC alias
with its owning task and screen dimensions. The initial graphics surface
supports palette/RGB
`GrPlot`, filled `GrRect`, solid `GrLine`, and transparent 8x8 `GrPrint` text,
translated by task pixel origin and scroll and clipped to that task's pixel
rectangle. `GrPrint` uses the bootstrap formatter's current `%%`, `%c`/`%C`,
`%s`, `%d`, and `%f` subset. `MStrPrint` supports the same formats with
floating-point precision (including upstream `%fs` elapsed-time labels); the
compatibility probe runs the unchanged `DrawProgressBars` body. These cover
basic geometry and labels used by upstream progress drawing, but do not port
CDC allocation/lifetime, the full formatter/font system, general raster
operations, sprites, view-angle controls, pixel-level z-buffer composition, or
optimized damage-region redraw.

`DrawCtrls` now walks a task's circular control list in list order, invokes the
draw callback on visible controls, skips hidden controls, and temporarily clears
the task's document scroll so controls remain fixed to the window. Traversal is
bounded to 64 entries. `CtrlsUpdate`-style derived geometry and rectangle hit
testing are ported; `WinScrollsInit` creates horizontal and vertical controls,
whose thumb geometry, drawing, click, and wheel callbacks are covered by the
QEMU probe. Bounded `CtrlDispatchLeftClick` and `CtrlDispatchWheel` helpers
route explicit screen-coordinate events to those callbacks, including
border-origin translation and release to a clicked control. The tablet bridge
now provides one packed coordinate/button sample, and `GrUpdateTasks` polls it
for left-button press, held drag, and release. A new press hit-tests shown task
rectangles and visible control bounds, raises the topmost owner in the shared
task/window ring, and assigns focus when allowed; control bounds also keep
scrollbars just beyond a client rectangle from falling through to a parent.
Title-bar drags move bordered windows, the left frame edge moves them
horizontally at fixed width as upstream does, and the right, bottom, and
lower-right frame edges resize them while the pointer is captured. Drag updates
now pass through the same `WinHorz`/`WinVert` normalization as upstream, keeping
dimensions at least one cell and applying the original screen-edge rules. The
QEMU probe checks focus/raise on overlapping windows, title and left-edge
movement, corner resize, one-cell minimum sizing, and a captured scrollbar
drag. The shell's
cooperative idle pump invokes
`BootstrapWinMgrTick` every 16 idle polls after the ZealOS task layer loads; it
needs no extra guest task slot. Right-button control callbacks now follow the
same hit testing and `CTRLF_CAPTURE_RIGHT_MS` held/release behavior as pinned
`WinMgr.ZC`; the task regression exercises captured movement beyond the
control bounds. `WIf_SELF_CTRLS` and `WIf_FOCUS_TASK_CTRLS` now gate left-click,
right-click, and wheel dispatch as in upstream WinMgr. VirtIO `REL_WHEEL`
steps now flow through the packed tablet sample into the focused task's wheel
control, including signed up/down deltas. Border hit testing and title actions
honor both `WIf_SELF_BORDER` on the target task and `WIf_FOCUS_TASK_BORDER` on
the focused task, matching the corresponding `WinMgr.ZC` gates. The remaining
upstream focus policies are unported. Body clicks now use the pinned
`WinCursorPosSet` coordinate mapping, including document scroll offsets and
the `WIf_SELF_MS_L` / `WIf_FOCUS_TASK_MS_L` gates. Body clicks now capture
through button release, flash bordered entries while pressed, and send Space
or Enter for the upstream left/right clickable entry flags. Full drag
selection remains unported.
Pointer movement now also reaches the focused task as `MESSAGE_MS_MOVE`, with
coordinates translated to that task's scrolled client area. Button messages
use those same translated coordinates. Tasks that request single-click mode
receive immediate down/up messages; other tasks receive delayed single-click
or synthesized `MESSAGE_MS_*_D_*` events using ZealOS's 175 ms interval.
DolDoc body clicks wait for that interval before activating an entry; a double
click sends the upstream Escape or Shift-Escape action instead. Window controls
continue to react to the physical button transitions.
Each valid absolute-tablet sample now applies the active ZealOS mouse scale and
offset (including the transform maintained by `GrScaleZoom`), then updates the
current guest-global `mouse.pos`, `mouse.pos_text`, and timestamp fields. It
also marks the virtual pointer device installed. The unchanged `WinFinalUpdate`
coordinate overlay therefore reports the live virtual pointer location. Its
normal `DrawMouse` path reaches a bootstrap software arrow in the final screen
overlay. The cursor is an interim black-and-white shape rather than the
upstream hardware cursor sprite or grab-scroll cursor.
The first four title cells post the upstream
Ctrl-M task-menu key; the last three post Shift-Esc to a task with a document
or kill a task without one. These release-triggered actions match pinned
`WinMgr.ZC`.
View-angle controls remain unported.

The tablet bridge resolves the active `mouse` global before applying scale and
offset, so modules that redeclare the source global still share the current
zoom transform. `WinMousePointerStateCheck` verifies a deterministic scaled
sample rather than only the identity mapping.

`/Tests/WindowDragLive.ZC` provides two overlapping bordered windows for a
manual UTM check. Load `KernelA.HH`, `KernelB.HH`, `Message.ZC`, `Job.ZC`, and
`KeyDev.ZC` before the fixture. `WindowDragLiveButtonChecks` covers the packed
tablet press/release route, task-menu queueing, and no-document close in QEMU;
`WindowDragLiveStart` provides the visible focus/raise, title movement, corner
resize, right-click capture (the control box turns green while held), and
close-button check with the real tablet. Its parked demo tasks do not draw a
task menu. Call `WindowDragLiveStop` to close any remaining test windows. The
automated check cannot replace the interactive UTM check.

After window callbacks, `GrUpdateTasks` invokes the optional
`gr.fp_final_screen_update` callback with `DCF_ON_TOP`. Graphics calls through
that CDC use screen coordinates and the full framebuffer clip, matching the
final overlay stage in upstream `GrUpdateScreen`. The progress-bar probe now
uses this route rather than attaching `DrawProgressBars` to a task.

The idle WinMgr bridge resolves the newest `gr` global through a small native
hash-table binding before invoking that optional callback. HolyC creates a new
storage object for each non-`extern` global definition, so the bootstrap's
early `gr` and later `GrGlobals.ZC`'s `gr` are distinct even though they share a
name and layout. The tick also falls back to `sys_task` if `sys_winmgr_task` is
stale. `GrScreenFinalUpdateChecks` sets the later graphics global's callback,
runs `BootstrapWinMgrTick`, and verifies the callback receives the bridge CDC
and writes its screen-space pixel after composition. The same smoke renders
two staged frames through the normal idle-frame wrapper, toggles
`mouse_grid.coord`, and checks that the real `WinFinalUpdate` coordinate text
changes the pixels in its screen-space overlay region. It also verifies the
unchanged Win callback invokes the software pointer and checks the pointer
against actual Limine framebuffer pixels. Real UTM pointer appearance and
positioning remain an interactive acceptance check.

These calls also exposed an AArch64 Aiwnios host-FFI gap: arguments after x0–x7
were not copied from the caller stack into the host shim's contiguous argument
array. The ARM64 FFI bridge now gathers those stack-passed arguments for
bindings with more than eight parameters; the eleven-argument clipped line
call exercises that path in the QEMU compatibility probe.

## Foreground glyph rasterizer

`GrRopEquU8NoClipping` is now an AArch64 scalar host binding for the pinned
x86 routine. It reads the guest's 256-entry `text.font` table, maps the masked
foreground nibble to the interim palette-index surface, writes only set glyph
bits, preserves destination pixels under transparent bits, and forces the
bottom scanline for `ATTRF_UNDERLINE`. Its guest checks cover the `A` glyph,
row stride/padding, transparent spaces, and underline. The current table is
seeded from the bring-up's compact 8x8 font; it is not ZealOS's full font set.

`GrUpdateTextFGChecks` also runs the unchanged foreground loop against a
temporarily initialized 800x600 paletted CDC. It verifies the exact changed
pixel span for one text cell and returns `0x2a`. This validates source-to-CDC
foreground composition, not full-screen presentation. `GrUpdateScreen32Checks`
also runs unchanged `GrUpdateScreen32` against temporary palette, raw-screen,
cache, and 32-bit alias buffers. It confirms palette conversion, changed-pixel
copy, and unchanged-pixel preservation. KernelB now exports Limine's HHDM
framebuffer address as `sys_framebuffer_addr` and seeds `text.fb_alias` with
that address; the native source also receives the real dimensions, byte pitch,
and pixel format. `GrUpdateScreen32LimineChecks` invokes unchanged
`GrUpdateScreen32` against the actual framebuffer, checks the written pixel
through the kernel readback path, then restores that pixel. This is covered by
`make check-grscreen` on QEMU's Limine-provided ramfb.

The shell's periodic task refresh renders host-backed text and drawing
primitives into a shared
800x600 indexed CDC surface, then presents that composed frame to the Limine
framebuffer. CDC byte writes and host-backed framebuffer primitives share the
surface. The staging path quantizes host colors to the current 16-color text
palette and clears to black each frame; arbitrary 8-bit CDC color semantics are
not preserved. The full source `GrUpdateScreen` sequence now runs with
bridge-staged `dc1`, `dc2`, cache, zoom, and raw-screen surfaces; its text and
final-overlay path passes with framebuffer-console output visible and serial
output enabled. Upstream-owned CDC allocation/lifetime and the full palette and
pixel-z-buffer semantics are not implemented. Each active source `gr` global is
explicitly bound to the bridge's bounded window z-buffer and uncovered-window
bitmap before source task traversal. The unchanged 2x zoom scaler is
covered both directly and through full-frame output against Limine framebuffer
pixels. `GrScaleZoom` is also called directly and its pointer-preserving
scale/offset update is verified against the tablet transform. Each bridge frame
now calls unchanged `WinMouseUpdate` after polling the absolute tablet, so source
pointer-edge recentering and wheel-capability synchronization run against the
active ZealOS globals. The QEMU regression places the pointer at the right edge
of a 2x view and verifies source pan, velocity decay, and wheel state. Live
pointer-centered zoom controls, non-default pan, and the full zoom interaction
path still need UTM acceptance.

## Remaining graphics path

The active source `winmgr` timing pointer is initialized lazily from the bridge
before each frame, with CPU idle counters and refresh timestamps seeded once.
The source `WinCalcIdles` has a deterministic smoke against that state,
including its guarded counter snapshot and idle-factor calculation. The
unchanged upstream `WinMgrTask` can now be started and stopped on demand through
the shell bridge; the bootstrap still owns the initial shell and framebuffer
pump. The active ZealOS `sys_winmgr_task` pointer is synchronized with the
bridge when the manager starts/stops. Its uncovered-window bitmap follows the
source z-index convention (bit zero reserved); a live QEMU probe now verifies
source CDC control rendering and framebuffer presentation while the manager
keeps refreshing. Turning this into the normal desktop startup path requires
menu startup and more surrounding task services. View-angle controls,
wallpaper, and broader CDC operations also remain open.
`Kernel/Display.ZC`'s
framebuffer writes can target the Limine-provided framebuffer, but x86 assembly
in `GrTextBase.ZC` and `GrAsm.ZC` must stay behind the ARM64 bridge or be
replaced with architecture-neutral ZealC.

## Milestone 224 — zoomed source frame reaches the framebuffer

| Item | Path |
|--|--|
| Gap | `GrZoomInScreen` had a direct scaler regression, but the full source compositor/presenter had not been exercised with zoom enabled |
| Bring-up | Enable 2x on the active source graphics global, run the complete staged frame, and compare selected Limine pixels with the source zoom CDC and palette |
| Smoke | `GrScreenFinalUpdateChecks` verifies zoomed framebuffer output and restores zoom/pan state |
| Limits | Live pointer-centered zoom controls, non-default pan, zoom-cache presentation, and view-angle controls remain open |

Freeze catalog unchanged.
