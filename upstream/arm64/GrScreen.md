# ARM64 overlay: `System/Gr/GrScreen.ZC`

The pinned ZealOS source stays unchanged under `upstream/pinned/`. When the
read-only source bundle is built for this AArch64 kernel, the bundle builder
applies a guarded source overlay to `GrUpdateTextBG`'s x86 inline assembly.

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

When `GrScreen.ZC` is loaded, the bootstrap frame now binds its active graphics
globals to the shared 8-bit frame CDC and text plane, then runs the unchanged
`GrUpdateTextBG` and `GrUpdateTextFG` before drawing task windows. If those
upstream functions are not loaded yet, startup keeps the existing text
fallback. `GrScreenFinalUpdateChecks` verifies the live frame invoked the
upstream text passes and retained overlay behavior. The native AArch64 runtime
also binds `DCBlotColor4`, the packed-pixel to four-bitplane conversion used at
the end of `GrUpdateScreen`; its guest smoke checks bit ordering, cache updates,
and that an unchanged cached group leaves the destination untouched.

This is a bundle-time architecture port, not a change to the pinned upstream
file and not an x86 instruction emulator. The live frame uses the upstream
text passes but still presents through the bootstrap palette renderer; it does
not yet run upstream `GrUpdateScreen32` or the complete `GrUpdateScreen` path.
The remaining graphics integration is to initialize `text.raw_screen`,
`gr.screen_cache`, `gr.dc1`, `gr.dc_cache`, and zoom surfaces, then route the
shared frame through the ZealOS presenter. Task-window traversal also remains
the bounded bootstrap bridge rather than full upstream `GrUpdateTasks`. The x86
build continues to use the original source. The guest source archive includes
this note at `/PORTS/ARM64/GrScreen.md` so the substitution is discoverable.
