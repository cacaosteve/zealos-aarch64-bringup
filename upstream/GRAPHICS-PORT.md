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
| `System/Gr/GrScreen.ZC` | Per-task redraw, controls, draw callbacks, z-buffer, and final screen update | The ARM64 source overlay compiles `GrUpdateTextBG` by replacing its x86 eight-row store loop with a tested stride-aware helper. AArch64 runtime bindings cover `DCBlotColor4` and the scalar `GrRopEquU8NoClipping` foreground-glyph rasterizer. The latter has a guest regression for glyph bits, stride, transparency, and underline; full `GrUpdateTextFG` presentation still needs initialized ZealOS display state. A bounded 100x75 cell z-buffer marks windows with any visible cells; each marked window's text, task `draw_it`, and visible control `draw_it` callbacks render in task-ring order. The final screen callback runs afterward with `DCF_ON_TOP`. View-angle controls, other `GrAsm` routines, and pixel-level z-buffer composition remain unported |
| `System/Win.ZC` | Window geometry, focus, and tiling operations | `WinHorz`/`WinVert`, derived geometry, bounded click focus/raise, title-bar move, and frame resize are ported; tiling and complete focus policies remain incomplete |
| `System/WinMgr.ZC` | Refresh loop, mouse routing, move/resize, and window-manager task | The shell idle pump routes tablet clicks to the topmost shown window/control, captures title-bar move and frame-resize drags, and redraws the task ring; the full upstream WinMgr task remains unported |
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
The first four title cells post the upstream
Ctrl-M task-menu key; the last three post Shift-Esc to a task with a document
or kill a task without one. These release-triggered actions match pinned
`WinMgr.ZC`.
View-angle controls remain unported.

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
changes the pixels in its screen-space overlay region. This confirms the
fixture's mouse-grid storage is shared with the unchanged Win callback; it
does not yet exercise physical mouse motion or cursor drawing.

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

This proves the final 32-bit presenter can reach the real surface, but not the
full `GrUpdateScreen` sequence. Upstream `gr.dc1`, `gr.dc_cache`, and
zoom/pan surfaces are not initialized together. The shell's periodic task
refresh now renders host-backed text and drawing primitives into a shared
800x600 indexed CDC surface, then presents that composed frame to the Limine
framebuffer. CDC byte writes and host-backed framebuffer primitives share the
surface. The staging path quantizes host colors to the current 16-color text
palette and clears to black each frame; arbitrary 8-bit CDC color semantics are
not preserved. This is not yet the full upstream `GrUpdateScreen` compositor or
a guarantee that arbitrary graphics DCs are presented. The next integration
step is to initialize the upstream display, cache, and alias surfaces as one
pipeline and route those DC operations through it before claiming full-frame
compatibility.

## Remaining graphics path

The next substantive graphics work is remaining WinMgr input behavior,
view-angle controls, wallpaper, and broader CDC operations.
`Kernel/Display.ZC`'s
framebuffer writes can target the Limine-provided framebuffer, but x86 assembly
in `GrTextBase.ZC` and `GrAsm.ZC` must stay behind the ARM64 bridge or be
replaced with architecture-neutral ZealC.
