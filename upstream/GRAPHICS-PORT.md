# ZealOS graphics and window-manager port

The references in `pinned/` are unchanged ZealOS sources from commit
`eee210f4379ba3d00b47507f336a158eefb2c993`. Their hashes are checked by
`scripts/check-upstream.py`. The bootstrap bridge is kept separate so ARM64
implementations can be compared against the x86 source without editing it.

## Source map

| Upstream source | Role | Current ARM64 status |
| --- | --- | --- |
| `Kernel/Display.ZC` | Raw text output and task pixel/window geometry | Limine framebuffer replaces boot-time mode discovery; `WinDerivedValsUpdate` is ported in `TaskBridge.ZC` |
| `System/Gr/GrGlobals.ZC` | `gr.text_base`, screen DCs, and compositor state | A bounded packed-cell plane exists in `kernel_stub.c`; there is not yet a `gr` global or upstream DC set |
| `System/Gr/GrTextBase.ZC` | `TextChar`, text spans/fills, and borders; the hot primitives are x86 assembly | Equivalent ARM64 bridge primitives are in `TaskBridge.ZC` and the framebuffer cell plane; span return values and negative-coordinate clipping are covered by the DocRecalc smoke |
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

## Remaining graphics path

The next substantive port is to give ZealOS callers a canonical `gr.text_base`
backed by the current cell plane, then implement the `GrScreen.ZC` draw path:
wallpaper, `draw_it` callbacks through `CDC`, and controls. After that, port the
uncovered-window/z-buffer update and connect `WinMgr.ZC` to the cooperative task
runtime. `Kernel/Display.ZC`'s framebuffer writes can target the Limine-provided
framebuffer, but x86 assembly in `GrTextBase.ZC` and `GrAsm.ZC` must stay behind
the ARM64 bridge or be replaced with architecture-neutral ZealC.
