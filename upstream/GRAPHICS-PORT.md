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
| `System/Gr/GrScreen.ZC` | Per-task redraw, controls, draw callbacks, z-buffer, and final screen update | Only bounded task-ring redraw, DolDoc refresh, and retained text output are implemented |
| `System/Win.ZC` | Window geometry, focus, and tiling operations | `WinHorz`/`WinVert` normalization and derived pixel geometry are ported; focus, tiling, and z-order callbacks are incomplete |
| `System/WinMgr.ZC` | Refresh loop, mouse routing, move/resize, and window-manager task | Pinned for reference; the ARM64 build has no upstream WinMgr task yet |
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

## Remaining graphics path

The next substantive port is the `GrScreen.ZC` draw path: wallpaper,
`draw_it` callbacks through `CDC`, and controls. After that, port the
uncovered-window/z-buffer update and connect `WinMgr.ZC` to the cooperative task
runtime. `Kernel/Display.ZC`'s framebuffer writes can target the Limine-provided
framebuffer, but x86 assembly in `GrTextBase.ZC` and `GrAsm.ZC` must stay behind
the ARM64 bridge or be replaced with architecture-neutral ZealC.
