# ARM64 overlay: `System/Gr/GrScreen.ZC`

The pinned ZealOS source stays unchanged under `upstream/pinned/`. When the
read-only source bundle is built for this AArch64 kernel, the bundle builder
applies a guarded source overlay to `GrUpdateTextBG`'s x86 inline assembly.

The original loop writes the 64-bit background color for eight glyph scanlines,
advancing by `width_internal` between rows, then advances to the next glyph by
eight bytes. The ARM64 overlay expands that exact instruction sequence into
eight typed U64 stores and seven row-stride advances, then applies the original
`w2` cursor adjustment. Keeping the stores inline avoids a guest function call
for each of the 7,500 text cells. The overlay checks for exactly the expected
eight `MOV U64 [RSI], R13` stores and seven `ADD RSI, R12` instructions. If
upstream changes that sequence, bundle creation fails instead of silently
applying an incorrect translation. `/Tests/GrScreenTextBG.ZC` independently
exercises the stride-aware helper's row stride, color bytes, untouched padding,
and final next-glyph pointer.

The native AArch64 runtime also binds `DCBlotColor4`, the packed-pixel to
four-bitplane conversion used at the end of `GrUpdateScreen`. Its guest smoke
checks bit ordering, cache updates, and that an unchanged cached group leaves
the destination untouched.

This is a bundle-time architecture port, not a change to the pinned upstream
file and not an x86 instruction emulator. The full `GrUpdateTextBG` and
`GrUpdateTextFG` pass is still not called by the live frame loop: an opt-in
synchronous frame experiment failed to return in the QEMU regression and was
removed. The next step is to isolate and profile that call path, then finish the
required ZealOS display-global initialization before enabling it. The x86 build
continues to use the original source. The guest source archive includes this note at
`/PORTS/ARM64/GrScreen.md` so the substitution is discoverable.
