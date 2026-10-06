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
stride helper, and `/Tests/GrScreenTextFrame.ZC` runs the translated
`GrUpdateTextBG` on a temporary 800x600 buffer, checks both canary regions, and
verifies every output pixel. Run that extended QEMU check with
`make check-grscreen-full`; the regular `make check-grscreen` remains the
shorter graphics regression.

The native AArch64 runtime also binds `DCBlotColor4`, the packed-pixel to
four-bitplane conversion used at the end of `GrUpdateScreen`. Its guest smoke
checks bit ordering, cache updates, and that an unchanged cached group leaves
the destination untouched.

This is a bundle-time architecture port, not a change to the pinned upstream
file and not an x86 instruction emulator. Although the background pass is now
verified end-to-end on a temporary CDC, `GrUpdateTextBG` and `GrUpdateTextFG`
are still not called by the live frame loop. Upstream `gr.dc1`, `gr.dc_cache`,
zoom/pan surfaces, and direct task drawing must be reconciled before enabling
the compositor refresh path. The x86 build continues to use the original
source. The guest source archive includes this note at
`/PORTS/ARM64/GrScreen.md` so the substitution is discoverable.
