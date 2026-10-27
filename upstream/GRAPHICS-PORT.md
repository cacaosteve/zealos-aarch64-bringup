# ZealOS graphics and window-manager port

The references in `pinned/` are unchanged ZealOS sources from commit
`eee210f4379ba3d00b47507f336a158eefb2c993`. Their hashes are checked by
`scripts/check-upstream.py`. The bootstrap bridge is kept separate so ARM64
implementations can be compared against the x86 source without editing it.

## Source map

| Upstream source | Role | Current ARM64 status |
| --- | --- | --- |
| `Kernel/Display.ZC` | Raw text output and task pixel/window geometry | Limine framebuffer replaces boot-time mode discovery; `WinDerivedValsUpdate` is ported in `TaskBridge.ZC` |
| `System/Gr/GrGlobals.ZC` | `gr.text_base`, screen DCs, and compositor state | Before boot-time `WinMgrTask` startup, the bridge binds the active source `gr` to the framebuffer, planar/cache/zoom CDC storage, text plane, screen cache, packed color tables, bounded visibility tables, generated zoom/brush families, and the source sprite opcode hash, then selects the unchanged standard palette. Normal boot-time graphics initialization remains incomplete |
| `System/Gr/GrTextBase.ZC` | `TextChar`, text spans/fills, and borders; the hot primitives are x86 assembly | ARM64 bridge primitives update `gr.text_base` and the framebuffer; a QEMU smoke proves direct packed-cell writes through the pointer are presented |
| `System/Gr/GrScreen.ZC` | Per-task redraw, controls, draw callbacks, z-buffer, and final screen update | The guarded ARM64 source overlay expands the x86 eight-row background store loop into eight typed stores in line; the stride helper is tested separately. AArch64 runtime bindings cover `DCBlotColor4` and the scalar `GrRopEquU8NoClipping` foreground-glyph rasterizer, with regressions for glyph bits, stride, transparency, and underline. Once the one-CPU task rings validate, the bridge stages the source graphics globals and dispatches unchanged `GrUpdateScreen`; for the boot full-frame path it draws shown tasks through unchanged `GrUpdateTaskWin` in task-ring order, preserving per-cell text clipping, because `GrUpdateTasks`' uncovered-bitmap gate can be empty while the terminal is valid. Startup loads `KMathB.ZC` for the source screen pass. The staged full-frame text smoke passes with framebuffer-console output visible and serial output enabled. A bounded 100x75 cell z-buffer supports text and border clipping. The final screen callback runs afterward with `DCF_ON_TOP`. Source zoom scaling is checked end to end through the compositor and Limine framebuffer at 2x; the live full-source path records its shell-text composition as well as screen/planar updates. Fixed-point identity and X/Y/Z matrix rotations are implemented and smoke-tested. The pinned three-axis view-angle control is ported and exposed in the boot OS menu; QEMU checks bounds, wrap/clamp, radians, and X/Y/Z click routing, and UTM confirms the control draws and all three angle readouts respond to clicks. The boot desktop now tints uncovered cells through the packed text-background plane; QEMU checks the manager callback, preserved glyph/foreground, and window-cell clipping. Other `GrAsm` routines and normal graphics initialization remain incomplete |
| `System/Win.ZC` | Window geometry, focus, and tiling operations | `WinHorz`/`WinVert`, derived geometry, bounded click focus/raise, title-bar move, frame resize, and the upstream horizontal/vertical/grid tilers are ported. QEMU checks all three arrangements against the two live terminal tasks; broader focus policies remain incomplete |
| `System/WinMgr.ZC` | Refresh loop, mouse routing, move/resize, and window-manager task | The unchanged `WinMgrTask` starts during boot and can be stopped/restarted with `winmgrstop`/`winmgr`. `windowdemo` runs two bordered fixture windows with a CDC control through the source compositor. While the source manager is active, the bridge submits raw tablet samples to `mouse_hard` and leaves click/drag/control dispatch to ZealOS, avoiding duplicate routing through both paths. Focused live-manager QEMU probes verify task/control drawing, framebuffer pixels, the final-overlay software cursor, focus/raise, title drag, resize, captured-control movement, right-click down/release, and wheel delta through that same submission path. The guarded ARM64 source overlay routes `WinMgrSleep` refreshes through the staged framebuffer transaction; the native manager callback restores both scheduler identity and ZealOS task TLS when rendering from its task. Graphics initialization now repairs a stale `sys_task` root before traversal. QEMU confirms the manager reaches this frame path, and its render regression checks the overlap control pixel across eight consecutive frames. UTM confirms move/resize and the user reports that overlap flashing is resolved. The yellow Window B rectangle is its intentional CDC demo control. The supported boot desktop starts two terminals and exposes grouped File/Windows/Apps/View/OS menus; the complete upstream desktop and task graph are not ported |
| `System/Gr/MakeGr.ZC` | Graphics module include order | Reference only; its complete dependency set is not loaded by the bring-up runtime |

## First source-backed slice

`TextLenStr` and `TextLenAttrStr` return `Bool` in upstream ZealOS. The bridge
now preserves that contract and reports false when clipping removes the whole
span. `WinHorz` and `WinVert` now follow upstream edge normalization, update
`CTask` bounds, and recalculate the corresponding character and pixel extents.
The `TaskOriginalDocRecalc` probe checks partially and fully clipped text spans,
per-cell attributes, and the derived bounds for an overlapping child window.
Text span and attributed-fill runs now consult the cell z-buffer as well, so
draw callbacks cannot repaint glyphs from a rear window through a front one.
Retained task text replay applies the same visibility test before updating the
framebuffer or canonical text plane. CDC rectangle primitives now use the same
cell z-buffer when publishing a staged frame. `WindowDragLiveRenderChecks`
places the control in the A/B overlap and checks both label and control
visibility against the current front-window z value.

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
rectangle. `GrPlot` and `GrRect` also operate on bounded offscreen CDCs and
implement the upstream pixel ROPs, deterministic/probability dithering,
depth-buffer test/update, collision count, nearest-point tracking, extents,
and `DCF_DONT_DRAW`. `GrPrint` uses the shared bootstrap formatter, which now
handles `%c`/`%C`, `%s`, `%d`, `%x`/`%X`, `%f`/`%F`, `%g`/`%G`, string
widths/truncation, and numeric width with zero padding, including dynamic
`%0*tX`. Significant-digit `%g` output covers both fixed and scientific
notation. `MStrPrint` retains upstream `%fs` elapsed-time support; the
compatibility probe runs the unchanged `DrawProgressBars` body. Focused CDC
lifecycle, copy, save/load, palette, transform, blit, and pixel-operation
checks pass. Screen-backed sprite meshes now use a separate 800x600
screen-coordinate depth plane. Each task callback clears only its visible
screen rectangle before drawing, and sprite-local coordinates map through the
task's pixel origin and scroll. The task window z-buffer remains authoritative
for cross-window visibility, while `DCF_ON_TOP` overlays bypass scene depth.
The regression checks independent task regions, depth reset on redraw, and
overlay bypass. Remaining gaps include the rest of the upstream formatter
conversions and flags plus the complete font system,
broader line/thick-brush and blit ROP parity, exact upstream sprite/mesh raster
parity, and optimized damage-region redraw.

`DrawCtrls` now walks a task's circular control list in list order, invokes the
draw callback on visible controls, skips hidden controls, and temporarily clears
the task's document scroll so controls remain fixed to the window. Traversal is
bounded to 64 entries. `CtrlsUpdate`-style derived geometry and rectangle hit
testing are ported; `WinScrollsInit` creates horizontal and vertical controls,
whose thumb geometry, drawing, click, and wheel callbacks are covered by the
QEMU probe. The source-backed three-axis view-angle control uses the same
control list and left-click routing; its QEMU checks cover derived bounds,
angle normalization, radian conversion, and independent X/Y/Z updates.
Bounded `CtrlDispatchLeftClick` and `CtrlDispatchWheel` helpers
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
The three-axis view-angle control is now a source-backed bootstrap port and is
exposed through the boot terminal's OS menu. The surrounding desktop and full
3D graphics stack remain incomplete.

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
task menu. The UTM shell accepts `Ctrl+C` as an escape while this demo has
keyboard focus; `windowdemostop` also stops it when the shell has focus. The
automated check cannot replace the interactive UTM check. If startup fails,
the shell reports whether it failed to stop the manager, select the bridge
frame path, load/start the fixture, or restart the source manager.

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

The pinned standard and auxiliary 256-entry ZealOS font tables are generated
into the ARM64 host build. The translated `GrScreen.ZC` passes its active
`text.font` pointer to the scalar glyph rasterizer, which maps the masked
foreground nibble to the interim palette-index surface, writes set glyph bits,
preserves destination pixels under transparent bits, and forces the bottom
scanline for `ATTRF_UNDERLINE`. Guest checks compare standard and auxiliary
`A` glyphs pixel-for-pixel against their pinned font rows, along with row
stride/padding, transparent spaces, underline, both pinned table values, and
the source-side pointer swap. The bounded ARM64 message overlay now routes
queued Ctrl-Alt-F through the registered source callback, and AutoComplete's
Ctrl-Alt help lists the registered shortcuts from `keydev`, including the
font toggle. `make check-home-autostart` exercises the displayed map and sends
Ctrl-Alt-F through the QEMU keyboard queue in both directions. The full
upstream key-device stack and remaining font behavior still need porting.

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
surface. Rectangle fills, individual plots, lines, and `GrPrint` check the
owning task against an 800x600 per-pixel window-visibility mask, whether
drawing to the staged frame or the direct framebuffer; text and borders use a
separate 8x8-cell visibility map. The per-pixel mask is regenerated from the
visible task rectangles, so it provides window occlusion rather than a global
3D depth compositor. The staging path quantizes host colors to the current
16-color text palette and clears to black each frame; arbitrary 8-bit CDC
color semantics are not preserved. The full source
`GrUpdateScreen` sequence now runs with
bridge-staged `dc1`, `dc2`, cache, zoom, and raw-screen surfaces; its text and
final-overlay path passes with framebuffer-console output visible and serial
output enabled. CDC object lifecycle and focused copy/save/load paths are
covered, but full palette/ROP semantics, global 3D depth composition, and
optimized damage-region redraw are not implemented. Each active source `gr`
global is explicitly bound to the bridge's bounded window z-buffer and
uncovered-window bitmap before source task traversal. The unchanged 2x zoom
scaler is covered both directly and through full-frame output against Limine framebuffer
pixels. `GrScaleZoom` is also called directly and its pointer-preserving
scale/offset update is verified against the tablet transform. Each bridge frame
now calls unchanged `WinMouseUpdate` after polling the absolute tablet, so source
pointer-edge recentering and wheel-capability synchronization run against the
active ZealOS globals. The QEMU regression places the pointer at the right edge
of a 2x view and verifies source pan, velocity decay, and wheel state. Ctrl+wheel
dispatches to unchanged `GrScaleZoom` after updating the pointer transform, while
plain wheel remains available to controls. Live UTM confirmed pointer-centered
Ctrl+wheel zoom, a plain-wheel control swatch, pointer-edge recentering while
zoom remains active, and the Lattice keyboard controls. The boot OS menu now
exposes bounded Zoom In/Out, Reset View, and four 40-source-pixel pan steps.
The bootstrap smoke checks the zoom and pan bounds, and the QEMU full-frame
graphics test verifies non-origin `sx`/`sy` through the zoom CDC and presented
Limine pixels. The exact current UTM guest also returns 42 from
`BootstrapGuiTerminalZoomPanChecks`, which exercises the same action bridge.
The user confirmed in the live UTM disk-only guest that the expanded menu
appears and Zoom In, Pan Right, and Reset View respond visibly. The boot menu
also exposes Paint Demo as a persistent pointer-driven drawing mode. While
active, held left-button motion is sampled each frame, gaps between samples
are filled with a bounded Bresenham stroke, and Escape clears the retained
stroke. `BootstrapGuiTerminalSmoke` verifies menu wiring, pointer samples,
interpolated line endpoints, pixel output through the screen CDC, release
handling, and clean exit. In live UTM, the user selected Paint Demo and
confirmed that it works; the screenshot shows yellow strokes drawn directly
over the left terminal, which is the intended inline surface rather than a
separate window. Escape cleanup has not been independently confirmed live.

## Remaining graphics path

The graphical shell output callback now always targets the registered GUI
terminal's `put_doc`. This matters when native compilation emits a diagnostic
while the active task owns a different DolDoc: cursor recalculation must not
mutate the source document being compiled. `make check-gui-terminal` exercises
that ownership rule directly, including after restarting the terminal;
`make check-usb-input` separately covers USB and GUI workflows. The installer
gate covers packaging, read-back, disk-only boot, and persistence. The output
fix closes the observed shell/document corruption during source compilation;
it does not replace the separately listed full upstream startup or graphics
parity work.

The UTM/QEMU boot route runs the supported desktop startup path: HomeSys
creates the primary and secondary terminals, the source `WinMgrTask` starts,
the centered splash is displayed and cleared, and grouped menus launch tested
file, window, app, view, and OS actions. `make check-home-autostart`,
`make check-grscreen-full`, and the graphical workflow gate exercise that
startup. This closes the earlier automatic boot-desktop gap for the supported
image; loading the full upstream application/task/menu graph remains separate.

The active source `winmgr` timing pointer is initialized lazily from the bridge
before each frame, with CPU idle counters and refresh timestamps seeded once.
The source `WinCalcIdles` has a deterministic smoke against that state,
including its guarded counter snapshot and idle-factor calculation. The
unchanged upstream `WinMgrTask` now starts as part of boot and can be
stopped/restarted through the shell bridge; the bootstrap retains the serial
shell and framebuffer pump as recovery paths. MathODE and its source frame
hooks are loaded and checked before the manager starts. The active ZealOS
`sys_winmgr_task` pointer is synchronized with the
bridge when the manager starts/stops. Its uncovered-window bitmap follows the
source z-index convention (bit zero reserved); a live QEMU probe now verifies
source CDC control rendering and framebuffer presentation while the manager
keeps refreshing. Full upstream menu entries and task services behind
unloaded applications remain unported. The view-angle control
is ported and its drawing plus X/Y/Z click response are UTM-confirmed.
The packed-cell wallpaper hook passes its compositor regression, though its
appearance still needs an eyes-on check on the current ISO in UTM. Broader CDC
operations and full upstream menu/task startup remain open.
The transformed indexed-mesh path now also performs homogeneous perspective
division and clips against the near plane and four CDC viewport edges before
triangulating; regressions cover a near-plane crossing, an out-of-range source
vertex clipped before integer conversion, and a five-vertex clipped polygon.
Transformed `SPT_LINE`, `SPT_POLYLINE`, `SPT_RECT`, `SPT_POLYGON`,
`SPT_ELLIPSE`, `SPT_CIRCLE`, and `SPT_ARROW` geometry uses homogeneous
near/viewport clipping before integer projection. Projective arrows draw a
head only when the actual endpoint remains visible. Projective quadratic/cubic
B-splines sample source-space Bezier spans and clip each projected segment
before rasterization.
The ellipse and circle paths preflight their bounded segment work. Rectangle fill
triangles and all four outline edges are preflighted as a single bounded
operation; a rectangle five million source pixels offscreen still draws its
visible clipped fill and border. The full QEMU graphics suite passes, and the
refreshed UTM installed-disk `GrSpriteSymmetryChecks` returns `0x2a` after a
cold restart. The current regression also covers projective polygon fill and
outline clipping through the near plane and CDC viewport; the refreshed ISO's
installed-disk graphics check returns `0x2a` after a cold restart. Projective
The full QEMU regression and refreshed UTM disk-only graphics check pass for
the projective point, packed-point, line, polyline, rectangle,
rotated-rectangle, polygon, spline, ellipse, circle, arrow, indexed-mesh,
bitmap, and text paths. Point and packed-point regressions now cross negative,
zero, and positive homogeneous `w`; only positive-`w` samples may reach the
CDC. The rotated-rectangle path clips fill and outline after homogeneous
projection and is exercised by a non-cardinal regression crossing the left
viewport edge. Remaining gaps include broader CDC operations, the full
upstream menu/task graph, and pixel-identical upstream mesh parity.
`Kernel/Display.ZC`'s
framebuffer writes can target the Limine-provided framebuffer, but x86 assembly
in `GrTextBase.ZC` and `GrAsm.ZC` must stay behind the ARM64 bridge or be
replaced with architecture-neutral ZealC.

The bounded `GrLine` bridge now also rasterizes offscreen CDCs directly, with
clipping, step/start patterns, pen thickness, and the CDC's pixel ROP rules.
`GrLineCdcChecks` covers horizontal and thick lines, dashed output, negative
edge clipping, XOR, exact shallow-diagonal Bresenham pixels, alternating-color
dither, and per-pixel depth rejection; the text-diamond reference test checks
the computed 11-pixel one-character outline. Unbrushed source-window strokes use the
framebuffer rasterizer. Offscreen and screen-backed CDC brush strokes dispatch
through upstream `GrPlot1`; regressions cover a ZealOS mask brush at an aligned
position and clipped against the negative edge and owning task window. Full
`make check-grscreen-full` passes with these checks. The refreshed installer
has also passed the QEMU live-CD install, byte-for-byte disk readback,
disk-only cold boot, and saved-data persistence check; the exact ISO still
needs its live UTM install cycle.

The ARM64 `GrBitMap.ZC` overlay also keeps unaligned
`DCF_NO_TRANSPARENTS` blits off the upstream shifted-word copy path. That path
produced incorrect pixels in the ARM64 guest; the clipped per-pixel fallback
preserves the MONO mask, EQU, XOR, collision, screen z-buffer, and no-draw
semantics while aligned copies retain the packed fast path. The screen-bitmap
fixture spans z-buffer cells with both unaligned per-pixel and aligned packed
no-transparency blits. Hidden cells remain unchanged, visible cells exercise
MONO/EQU/XOR output, and collision mode counts only visible destinations. The
aligned fixture uses nonzero task scrolling and a 16x9 image that crosses a
vertical 8-pixel z-buffer boundary; its four-cell visibility pattern verifies
that the packed cursor advances by the correct window row. The same four-cell
fixture now covers ordinary transparent MONO/EQU/XOR blits and collision
counting: transparent source pixels and occluded cells are preserved, with
exactly 36 visible nontransparent intersections counted. The MONO case uses
the upstream zero/nonzero mask convention and checks both clipped offscreen
and screen-z-buffer paths. The focused probe passes three times without arena
growth; `GrLineCdcChecks`, the full graphics gate, and
refreshed ISO installer gate pass. `GrLineCdcChecks` also exercises upstream
brush semantics: a mask brush uses MONO despite the caller selecting XOR,
restores the caller's color/ROP, and collision mode counts only opaque brush
pixels that overlap non-background destinations. Both no-transparency masks
(where zero cells overwrite earlier stamps) and ordinary MONO brushes (where
zero cells leave the destination untouched and nonzero cells use the caller's
color) are checked. It also checks a thick, dashed offscreen brush stroke;
opaque zero-mask pixels may overwrite earlier parallel stamps, matching the
source brush behavior. The focused probe and `make check-grscreen-full` both
return `0x2a`. Broader brush and CDC ROP combinations remain open.

The ARM64 `GrBlot` collision operation now checks the screen window z-buffer
for aligned as well as unaligned screen-backed bitmaps, so overlaps hidden by
a front task are not counted. Its fixture spans two text cells with one hidden
and one visible; exactly the visible eight pixels contribute. This closes the
upstream collision branch's explicit `win_z_buf` TODO for the tested screen
bitmap path. The test allocates a synthetic task and temporary z-buffer rather
than depending on boot-terminal focus, and passes in both the focused PCI probe
and `make check-grscreen-full`. The refreshed ISO's QEMU installer,
byte-for-byte read-back, disk-only cold boot, and saved-data reboot checks also
pass; its live UTM install is still pending. Other broader ROP/CDC combinations
remain open.

`SpriteScreenDepthChecks` now creates a real temporary guest task instead of
assuming shell-loaded modules retain `sys_task` and `sys_winmgr_task`. It
restores task rectangles, scroll offsets, window-Z values, depth regions,
visibility entries, and CDC state after exercising screen-coordinate depth,
redraw reset, and the on-top overlay. The focused probe and full graphics gate
pass. The separately named `screen-depth-validated.iso` passes the QEMU
install/read-back, disk-only boot, and saved-data reboot gate; live UTM
acceptance of this exact image remains pending.

The first screen-depth ISO exposed a splash regression: its `DrawIt` callback
owns a task-local depth buffer, and `SpriteHLine0` had replaced it with the
shared screen plane while also plotting only to host scanout. The rasterizer
now preserves task-owned depth, uses shared screen depth when no task buffer is
present, and writes visible screen mesh pixels to the staged CDC that the
source presenter consumes. `make check-home-autostart` confirms the animated
mesh changes across frames, stays centered between labels, and clears on
dismissal; `make check-grscreen-full` passes. Replacement image
`screen-depth-splash.iso` passes the QEMU installer/read-back and persistence
reboot gate. Live UTM acceptance remains pending for this replacement image.

The projective rectangle fill now feeds its clipped vertices through the
pinned upstream `GrFillTri0` scan converter (source revision
`eee210f4379ba3d00b47507f336a158eefb2c993`) via ARM64 CDC line adapters. The
on-top CDC path skips task-window clipping inside that routine because its
line writer uses CDC-local coordinates; ordinary screen CDCs retain the task
visibility clip. `GrSpriteSymmetryChecks` covers the triangle and perspective
rectangle through separate direct and dispatched paths, and the full graphics
and startup-splash regressions pass. This closes the triangle span-filling
algorithm gap for this path, while exact whole-mesh pixel parity and optimized
damage-region redraw remain open.

The transformed `GrBlot3` path now reports whether any sampled source pixel
lands within the destination CDC instead of claiming success for a wholly
off-screen image. QEMU coverage checks both a fully clipped transform and a
partially clipped right-edge blit. The refreshed UTM guest has now passed the
expanded source regression live: the user confirmed `GrBlot3Checks`,
`GrSpriteTransformChecks`, and `GrSpriteSymmetryChecks` each returned `0x2a`.

## Milestone 227 — upstream `GrPlot0` pixel semantics

| Item | Path |
|--|--|
| Gap | The bootstrap `GrPlot` only wrote screen pixels with the first color and a partial checkerboard dither; offscreen CDCs, depth, ROPs, nearest/extents, collision, and probability dither were missing |
| Bring-up | Route screen and offscreen plotting through the same clipped pixel operation, preserve per-pixel window visibility, and apply the pinned `GrPlot0` flags and ROP cases |
| Smoke | `GrPlotSemanticsChecks` returns `0x2a` for offscreen edge clipping, filled-rectangle clipping, XOR/collision, transparent/background collision exclusions, deterministic/probability dither, depth acceptance/rejection, `DCF_DONT_DRAW`, nearest point, and extents; `make check-grscreen-full` passes |
| Limits | This closes single-pixel, filled-rectangle, and bounded offscreen `GrLine` semantics, including offscreen and screen-backed CDC mask-brush strokes. Unaligned no-transparency bitmap ROPs have a clipped per-pixel ARM64 path; broader brush-blit ROP combinations, transformed mesh pixel parity, and full upstream desktop startup remain separate work |

## Milestone 224 — zoomed source frame reaches the framebuffer

| Item | Path |
|--|--|
| Gap | `GrZoomInScreen` had a direct scaler regression, but the full source compositor/presenter had not been exercised with zoom enabled |
| Bring-up | Enable 2x on the active source graphics global, run the complete staged frame, and compare selected Limine pixels with the source zoom CDC and palette |
| Smoke | `GrScreenFinalUpdateChecks` verifies zoomed framebuffer output and restores zoom/pan state |
| Limits | Pointer-centered Ctrl+wheel zoom, plain-wheel control response, pointer-edge recentering, view-angle drawing/X/Y/Z click response, and the expanded OS menu's Zoom In, Pan Right, and Reset View actions are live-UTM confirmed. Zoomed and panned source-frame presentation, including the initialized full source compositor, passes in QEMU and the isolated live UTM guest |

## Milestone 225 — CDC drawing uses the window cell z-buffer

| Item | Path |
|--|--|
| Gap | Only filled CDC rectangles honored the window z-buffer; plots, lines, and graphics text could paint through a higher window |
| Bring-up | Pass the owning task's z value through the `GrPlot`, `GrLine`, and `GrPrint` bridges and apply the same 8x8-cell visibility test to each emitted pixel |
| Smoke | Full `make check-compat-pci` passed, including source window-frame and graphics regressions. A window-frame-only rerun passed after removing exploratory test instrumentation |
| Limits | Per-pixel window occlusion and the bounded screen-CDC depth test are now exercised by the graphics gate. Broader CDC/brush ROP combinations and whole-mesh pixel parity remain open; staged refresh also resolved the reported overlap flashing in the user's UTM run |

## Milestone 226 — skip unchanged staged framebuffer pixels

| Item | Path |
|--|--|
| Gap | The staged refresh rewrote all 480,000 scanout pixels, even when most pixels were unchanged |
| Bring-up | Compare staged RGBA or palette-expanded output with the current framebuffer and store only changed pixels |
| Smoke | Full `make check-compat-pci` passed, including source frame and live window-manager probes |
| Limits | This reduces writes on the single-buffer UTM scanout but cannot provide atomic frame flips or vblank synchronization |

Freeze catalog unchanged.

## Retained foreground for unchanged source programs (2026-10-26)

`gr.dc` now owns a separate transparent 800×600 foreground surface; `gr.dc2`
remains the source compositor's frame target. Previously they shared a body,
and the text/window refresh erased graphics drawn once before waiting for
input. The unchanged NetOfDots program exposed this even though callback-based
demos redrew on each frame. Foreground lines use the bounded CDC rasterizer
and task-window visibility; the full source screen path's original
DCBlotColor8 stage and the fallback compositor both overlay the retained body.
Original DCFill clears it on exit. The foreground does not inherit ON_TOP: its
normal alias retains task-local origin and clipping.

The program gate requires the actual red fan inside its own window across
redraws and verifies that exit clears it, then checks colored Lines animation
and prompt recovery through NEC USB and VirtIO input. A whole-screen red-pixel
check was insufficient because the second terminal contains red welcome text;
that check was replaced with the program-window and persistence checks.
This does not establish complete CDC/ROP, persistent text-raster or whole-mesh
parity for the entire regular application collection. See
[PROGRAMS.md](../PROGRAMS.md) and [ACCEPTANCE.md](../ACCEPTANCE.md) for scope,
completed runs and current-image status.

## Original Doodle, Mass Spring and Life (2026-10-26)

Graphics startup now loads the complete unchanged GrMath and GrPrimatives
modules. The compiler preserves inline-array row addresses and full strides,
fixing the original brush-circle table access used by thick lines. Native
floating square, Ceil and trigonometric bindings and the exact upstream R2P
body support the original circle and polar geometry paths.

The interactive gate uses real menu and pointer input. It checks Doodle's
original color chooser and retained thick stroke, Mass Spring's circles,
spring, visible ODE motion and reset, and Life's drawn cells and evolving
background buffer. Every program must exit to a usable terminal. Both NEC USB
and VirtIO input pass. A frozen tS redeclaration and duplicate old/new mouse
routers found by these programs were fixed in the shared runtime/load order.

The Apps wrapper selects a light task surface for their original black ink;
the three program files remain byte-identical to the pinned upstream commit.
Live UTM checks also confirm Doodle drawing/colors, Mass Spring motion/reset,
Life evolution and working terminal recovery after all three programs.
This does not accept the rest of the application collection or establish full
graphics parity. Embedded sprite source includes and several application
services still block other programs; PROGRAMS.md records the inventory.
