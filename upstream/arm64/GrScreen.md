# ARM64 overlay: `System/Gr/GrScreen.ZC`

The pinned ZealOS source stays unchanged under `upstream/pinned/`. When the
read-only source bundle is built for this AArch64 kernel, the bundle builder
applies guarded overlays to `GrUpdateTextBG`'s x86 inline assembly and the
optional `ODEsUpdate` call in `GrUpdateTaskODEs`.

The original loop writes the 64-bit background color for eight glyph scanlines,
advancing by `width_internal` between rows, then uses its `w2` adjustment to
return to scanline zero and advance eight pixels to the next glyph. The ARM64
overlay recognizes that exact x86 instruction sequence and emits a typed
scanline loop while preserving the original `w2` adjustment. A full-frame
canary test caught and fixed a double-advance in the first inline expansion:
it skipped every other glyph cell. `/Tests/GrScreenTextBG.ZC` exercises the
stride helper, and `/Tests/GrScreenTextFrame.ZC` runs the translated background
pass followed by the unchanged foreground glyph pass on a temporary 800x600
buffer. It checks both canary regions and every pixel, including the foreground
glyph over its expected background. Run that extended QEMU check with
`make check-grscreen-full`; the regular `make check-grscreen` remains the
shorter graphics regression.

When `GrScreen.ZC` is loaded, the bootstrap frame binds its active graphics
globals to the shared 8-bit frame CDC and text plane, then runs the unchanged
`GrUpdateTextBG` and `GrUpdateTextFG` before drawing task windows. After the
bounded bootstrap z-order walk selects a visible, uncovered task, it now calls
the upstream `GrUpdateTaskWin` when that source function is loaded; startup
falls back to the bootstrap window painter until then. The ARM64 overlay routes
the upstream `DocUpdateTaskDocs` call through the optional DolDoc bridge, so
the painter remains safe before that library is loaded and dispatches to the
real source function when available. The headless regression temporarily shows
a valid root task and requires the upstream painter counter to advance. After
the bootstrap blit, the native AArch64 frame runner also calls the unchanged
`GrUpdateScreen32` on the live 800x600x32 framebuffer. It binds a raw 32-bit
conversion buffer and the 8-bit screen cache, and uses ZealOS's standard
palette only when no palette has been selected yet. The presenter call is
guarded by framebuffer dimensions, pitch, and pixel format. The QEMU regression
requires both source text passes and the source presenter to run during a live
frame, then checks the resulting cursor pixels. If the source graphics modules
are not loaded, startup keeps the existing bootstrap renderer. The AArch64
frame now allocates ZealOS-shaped `dc1` and `dc_cache` surfaces. After the
source 32-bit presenter, its `DCBlotColor4` host implementation converts the
finished 8-bit `dc2` into four one-bit planes and updates the packed-group
cache. The graphics smoke verifies that this stage ran and that the cache
matches the composed frame; the focused converter smoke also checks bit
ordering and that unchanged groups leave the destination untouched. These
planes are currently a compatibility surface, not the firmware framebuffer's
scanout format. The frame also owns a full-size zoom surface and preserves
valid `screen_zoom`/pan state instead of forcing zoom back to 1 each frame. The
unchanged `GrUpdateScreen32` therefore runs upstream `GrZoomInScreen` when
zoomed. `/Tests/GrZoomScreen.ZC` checks every pixel of the 2x nearest-neighbor
result, and the live graphics smoke checks that the frame binds and uses this
zoom surface.

This is a bundle-time architecture port, not a change to the pinned upstream
file and not an x86 instruction emulator. The live frame still uses the bounded
bootstrap window traversal and its framebuffer blit before the source 32-bit
presenter; it does not yet run the complete upstream `GrUpdateScreen` sequence
or upstream `GrUpdateTasks`. The task runtime now provides a single-core CPU0
record whose executive task is the root task, and initializes every live
task's `next_ode`/`last_ode` as an empty self-linked list. The runtime now walks
the CPU0 task ring with a 64-task safety bound and calls upstream
`GrUpdateTaskODEs` only after `ODEsUpdate` is loaded; the ARM64 overlay routes
the integrator call through an optional host bridge. The QEMU smoke supplies a
small test integrator and verifies the wrapper visits the root task. This does
not yet load or execute ZealOS's `MathODE` subsystem or model additional CPUs.
Remaining graphics work includes connecting native input gestures to the
upstream zoom controls and deciding how legacy planar output should coexist
with the display driver's 32-bit framebuffer.
The x86 build continues to use the original source. The guest source archive
includes this note at `/PORTS/ARM64/GrScreen.md` so the substitution is
discoverable.
