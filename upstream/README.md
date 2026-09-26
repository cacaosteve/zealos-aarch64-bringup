# Upstream compatibility track

Progress is measured by running **ZealOS / ZealC sources** with minimal rewrite,
not by growing bring-up-only demos.

## Milestone 1 — application (done)

| Item | Path |
|--|--|
| Upstream original | `templeos/ZealOS/src/Demo/Graphics/NetOfDots.ZC` |
| Bring-up copy | `upstream/NetOfDots.ZC` |
| Smoke | `hc: Upstream NetOfDots` in `kernel_stub.c` |

Deltas are listed at the top of `NetOfDots.ZC` (CDC / DocClear / PressAKey only).

## Milestone 2 — animated Gr + input loop (this tree)

| Item | Path |
|--|--|
| Upstream original | `templeos/ZealOS/src/Demo/Graphics/Lines.ZC` |
| Bring-up copy | `upstream/Lines.ZC` |
| Smoke | `hc: Upstream Lines` in `kernel_stub.c` |
| Shell | `lines` |

Exercises Clamp/Sign/Rand/FbW/FbH/KeyHit/Sleep + GrLine beyond NetOfDots.
Deltas listed at the top of `Lines.ZC`.

## Milestone 3 — MiniGrLib / Gr primitives (this tree)

| Item | Path |
|--|--|
| Upstream original | `templeos/ZealOS/src/Demo/Lectures/MiniGrLib.ZC` |
| Bring-up copy | `upstream/MiniGr.ZC` |
| Smoke | `hc: Upstream MiniGr` in `kernel_stub.c` |
| Shell | `minigr` |

GrHLine + GrLine + GrCircle/GrFillCircle composition (no F64 flood/CDC).
Deltas listed at the top of `MiniGr.ZC`.

## Milestone 4 — mem helpers / counting sort (this tree)

| Item | Path |
|--|--|
| Upstream inspiration | `templeos/ZealOS/src/Demo/RadixSort.ZC` |
| Bring-up copy | `upstream/MemSort.ZC` |
| Smoke | `hc: Upstream MemSort` (expect 16) |
| Shell | `memsort` |

MemSet + U8 arrays + counting sort + FillRect histogram.
Raised `HC_IR_MAX_LABELS` for multi-loop bodies.

## Milestone 5 — GrPeek / GrPlot (this tree)

| Item | Path |
|--|--|
| Upstream inspiration | `System/Gr` GrPeek/GrPlot + `Demo/Graphics/Life.ZC` neighbor count |
| Bring-up copy | `upstream/PeekPlot.ZC` |
| Smoke | `hc: Front GrPeek` + `hc: Upstream PeekPlot` |
| Shell | `peekplot` |

Readback FB pixels; Life-style 3×3 peek count.

## Milestone 6 — MAlloc + offscreen bitmap (this tree)

| Item | Path |
|--|--|
| Upstream shape | `Kernel/Memory/MAllocFree.ZC` + CDC body / GrBlot blit |
| Bring-up copy | `upstream/OffBmp.ZC` |
| Smoke | `hc: Front MAlloc` + `hc: Upstream OffBmp` |
| Shell | `offbmp` |

Bump `MAlloc`/`Free`, `U8*` indexing, offscreen U8 buffer → GrPlot cells.

## Milestone 7 — heap strings (this tree)

| Item | Path |
|--|--|
| Upstream shape | `MAllocFree.ZC` `StrNew` + StrCpy/StrCat joins |
| Bring-up copy | `upstream/HeapStr.ZC` |
| Smoke | `hc: Front StrNew` + `hc: Upstream HeapStr` (expect 6) |
| Shell | `heapstr` |

`StrNew` / `StrCpy` / `StrCat` on the bump heap; char-height bars for UTM.

## Milestone 8 — CatPrint / MStrPrint (this tree)

| Item | Path |
|--|--|
| Upstream | `Kernel/StrPrint.ZC` CatPrint / StrPrint / MStrPrint |
| Bring-up copy | `upstream/CatFmt.ZC` |
| Smoke | `hc: Front CatPrint` + `hc: Upstream CatFmt` (expect 12) |
| Shell | `catfmt` |

Fixed-arity format join (`%d` `%s` `%c` `%X` `%%`); `MStrPrint` allocates.

## Milestone 9 — heap queue (this tree)

| Item | Path |
|--|--|
| Upstream | `KernelA.HH` `CQueue` + `QueueInit` / `QueueInsert` |
| Bring-up copy | `upstream/HeapQue.ZC` |
| Smoke | `hc: Upstream HeapQue` (expect 150) |
| Shell | `heapque` |

Circular doubly-linked queue on `MAlloc`; HolyC `QueueInit`/`QueueInsert`/`QueueCount`.

## Milestone 10 — QueueRemove + jobs (this tree)

| Item | Path |
|--|--|
| Upstream | `QueueRemove` + `Job.ZC` waiting-queue pattern |
| Bring-up copy | `upstream/JobQue.ZC` |
| Smoke | `hc: Upstream JobQue` (expect 120) |
| Shell | `jobque` |

Insert five jobs, cancel `val==30`, sum/draw the remaining four.

## Milestone 11 — JobRun drain (this tree)

| Item | Path |
|--|--|
| Upstream | `Job.ZC` `JobRunOne` (pop `next_waiting`) |
| Bring-up copy | `upstream/JobRun.ZC` |
| Smoke | `hc: Upstream JobRun` (expect 15) |
| Shell | `jobrun` |

Drain `head.next` until empty; sum vals 1..5.

## Milestone 12 — CTask-lite spawn (this tree)

| Item | Path |
|--|--|
| Upstream | `KTask.ZC` `Spawn` + run-one (no stacks) |
| Bring-up copy | `upstream/TaskSpawn.ZC` |
| Smoke | `hc: Upstream TaskSpawn` (expect 42) |
| Shell | `spawn` |

`Spawn` onto ready queue; `TaskExe` by kind; drain and sum results.

## Milestone 13 — PopUp Call1 (this tree)

| Item | Path |
|--|--|
| Upstream | `Spawn(fp,…)` / `PopUp` deferred fn call |
| Bring-up copy | `upstream/PopUp.ZC` |
| Smoke | `hc: Front Call1` + `hc: Upstream PopUp` (expect 27) |
| Shell | `popup` |

`&Fn` + `Call1(fp,arg)`; queue workers and run later.

## Milestone 14 — DolDoc-lite (this tree)

| Item | Path |
|--|--|
| Upstream | DolDoc `CDoc` / `CDocEntry` + `DocNew` / `DocPrint` |
| Bring-up copy | `upstream/DocLite.ZC` |
| Smoke | `hc: Upstream DocLite` (expect 7) |
| Shell | `doclite` |

Heap text-entry queue; walk/print tags; no `$$` cmds or files.

## Milestone 15 — RAM Blk stub (this tree)

| Item | Path |
|--|--|
| Upstream | `BlkDev` `BlkRead` / `BlkWrite` (512-byte blocks) |
| Bring-up copy | `upstream/RamBlk.ZC` |
| Smoke | `hc: Front Blk` + `hc: Upstream RamBlk` (expect 414) |
| Shell | `ramblk` |

16×512 RAM disk; no `CDrive` / RedSea yet.

## Milestone 16 — name table on RAM disk (this tree)

| Item | Path |
|--|--|
| Upstream | `Demo/Disk/DataBase.ZC` FBlk-shaped dir + payload |
| Bring-up copy | `upstream/NameFile.ZC` |
| Smoke | `hc: Upstream NameFile` (expect 5) |
| Shell | `namefile` |

Blk0 directory record (name/blk/len) + Blk1 payload round-trip.

## Milestone 17 — multi-file dir lookup (this tree)

| Item | Path |
|--|--|
| Upstream | RedSea/dir walk + open-by-name |
| Bring-up copy | `upstream/DirLook.ZC` |
| Smoke | `hc: Upstream DirLook` (expect 4) |
| Shell | `dirlook` |

`DirPut` / `DirFind` over 32-byte entries; lookup `"Zeal"` and read payload.

## Milestone 18 — DirDel + slot reuse (this tree)

| Item | Path |
|--|--|
| Upstream | RedSea dir delete / free slot |
| Bring-up copy | `upstream/DirDel.ZC` |
| Smoke | `hc: Upstream DirDel` (expect 2) |
| Shell | `dirdel` |

Delete `"Hi"`, reuse slot 0 for `"OS"`, lookup and read.

## Milestone 19 — FOpen / FClose (this tree)

| Item | Path |
|--|--|
| Upstream | `DiskCFile.ZC` FOpen/FClose/FBlkRead |
| Bring-up copy | `upstream/FOpen.ZC` |
| Smoke | `hc: Upstream FOpen` (expect 4) |
| Shell | `fopen` |

`CFile`-lite over RAM dir; single-blk contiguous read.

## Milestone 20 — FCreate / FBlkWrite (this tree)

| Item | Path |
|--|--|
| Upstream | FOpen `"w"` + FBlkWrite |
| Bring-up copy | `upstream/FWrite.ZC` |
| Smoke | `hc: Upstream FWrite` (expect 3) |
| Shell | `fwrite` |

Create `"New"`, write payload, reopen and read back.

## Milestone 21 — multi-blk contiguous file (this tree)

| Item | Path |
|--|--|
| Upstream | FBlkRead/FBlkWrite with advancing `pos` |
| Bring-up copy | `upstream/MultiBlk.ZC` |
| Smoke | `hc: Upstream MultiBlk` (expect 2) |
| Shell | `multiblk` |

Create `"Big"`, write two consecutive blocks, reopen and read both.

## Milestone 22 — RedSea boot-sector stub (this tree)

| Item | Path |
|--|--|
| Upstream | `CRedSeaBoot` + `RedSeaValidate` / `RedSeaFormat` |
| Bring-up copy | `upstream/RedSea.ZC` |
| Smoke | `hc: Upstream RedSea` (expect 7) |
| Shell | `redsea` |

Write/validate blk0 signatures `0x88` + `0xAA55`; return `unique_id`.

## Milestone 23 — RedSea root dir entries (this tree)

| Item | Path |
|--|--|
| Upstream | `RedSeaFormat` `.` / `..` `CDirEntry` at `root_clus` |
| Bring-up copy | `upstream/RSRoot.ZC` |
| Smoke | `hc: Upstream RSRoot` (expect 2) |
| Shell | `rsroot` |

Format boot, write `.`/`..`, find `.` → return its `clus`.

## Milestone 24 — RedSea file entry + payload (this tree)

| Item | Path |
|--|--|
| Upstream | `RedSeaFileFind` / file `CDirEntry` → `BlkRead` |
| Bring-up copy | `upstream/RSFile.ZC` |
| Smoke | `hc: Upstream RSFile` (expect 4) |
| Shell | `rsfile` |

Root dir + `"Zeal"` entry at clus 3; find, read, verify payload.

## Milestone 25 — RedSea bitmap alloc (this tree)

| Item | Path |
|--|--|
| Upstream | bitmap @ fat1 + `ClusAlloc` / `LBts` |
| Bring-up copy | `upstream/RSAlloc.ZC` |
| Smoke | `hc: Upstream RSAlloc` (expect 3) |
| Shell | `rsalloc` |

Boot+bitmap; `RSAlloc` yields root=2 then file=3; write `"OS"`.

## Milestone 26 — RedSea free + delete reuse (this tree)

| Item | Path |
|--|--|
| Upstream | `RedSeaFreeClus` / deleted dir slot |
| Bring-up copy | `upstream/RSFree.ZC` |
| Smoke | `hc: Upstream RSFree` (expect 3) |
| Shell | `rsfree` |

Alloc file clus 3; `RSDel`; `RSAlloc` reclaims 3.

## Milestone 27 — multi-clus RSAlloc file (this tree)

| Item | Path |
|--|--|
| Upstream | contiguous `ClusAlloc(cnt)` + spanned `BlkWrite` |
| Bring-up copy | `upstream/RSMulti.ZC` |
| Smoke | `hc: Upstream RSMulti` (expect 2) |
| Shell | `rsmulti` |

`RSAlloc(16,2)` → clus 3; write/read `"Hi"`/`"OS"` across two blks.

## Milestone 28 — CFile over RedSea (this tree)

| Item | Path |
|--|--|
| Upstream | `DiskCFile` FOpen/FBlkRead on RedSea clus |
| Bring-up copy | `upstream/RSCFile.ZC` |
| Smoke | `hc: Upstream RSCFile` (expect 4) |
| Shell | `rscfile` |

RedSea vol + `"Zeal"`; `FOpen`/`FBlkRead` via `CDirEntry` clus.

## Milestone 29 — FCreate / FBlkWrite on RedSea (this tree)

| Item | Path |
|--|--|
| Upstream | FOpen `"w"` + FBlkWrite on RedSea dir |
| Bring-up copy | `upstream/RSCWrite.ZC` |
| Smoke | `hc: Upstream RSCWrite` (expect 3) |
| Shell | `rscwrite` |

`FCreate "New"`; write; persist len; reopen `FBlkRead`.

## Milestone 30 — FSize / FSeek on RedSea CFile (this tree)

| Item | Path |
|--|--|
| Upstream | `DiskCFile` FSize / FSeek |
| Bring-up copy | `upstream/RSCSeek.ZC` |
| Smoke | `hc: Upstream RSCSeek` (expect 4) |
| Shell | `rscseek` |

Write `"Seek"`; `FSeek(0)`; re-read; `FSize` → 4.

## Milestone 31 — shared RedSea/CFile lib (this tree)

| Item | Path |
|--|--|
| Shared helpers | `upstream/lib/RedSeaCFile.ZC` |
| Demo | `upstream/RSCLib.ZC` (concat with lib) |
| Smoke | `hc: Upstream RSCLib` (expect 4) |
| Shell | `rsclib` |

One shared `CFile`/`RSFmt`/`RSAlloc` copy; 2-clus write + seek read. `HC_MAX_FNS` → 16.

## Milestone 32 — persist RedSea across shell cmds (this tree)

| Item | Path |
|--|--|
| Shared helpers | `RSValid` / `RSRoot` in `upstream/lib/RedSeaCFile.ZC` |
| Demo | `upstream/RSCPersist.ZC` (concat with lib) |
| Smoke | `hc: Upstream RSCPersist` then `RSCPersist2` (expect 4 each) |
| Shell | `rspersist` (run twice — second must not wipe) |

RAM disk only: format/create once, second open reads `"Keep"` without `RSFmt`. Reboot persist (virtio-blk) is later.

## Milestone 33 — virtio-blk RedSea image (this tree)

| Item | Path |
|--|--|
| Driver | `src/virtio_blk.h` (MMIO `virtio-blk-device`, PCI `virtio-blk-pci` fallback) |
| Layout | `src/disk_layout.h` — RedSea @ LBA 130911 (128 sects), before GPT backup |
| QEMU | `build/redsea.img` via MMIO; `make run-pci` for UTM-shaped PCI |
| UTM | PCI boot-disk RedSea window; `make utm` preserves LBA 130911.. |
| Boot log | `virtio-blk: OK` + `rw OK` (`mmio` or `pci`) — **rw must pass** |
| Shell | `vblk`; `rspersist` |

UTM strips `-drive` / `virtio-blk-device` from AdditionalArguments. Boot smokes use RAM (virtio inits after JIT).

**Validated:** QEMU MMIO + `make run-pci` R/W; UTM `pci` + `rw OK` + `rspersist` Keep across restart; `make utm` preserves RedSea.

## Milestone 34 — load + JIT a `.ZC` from RedSea (this tree)

| Item | Path |
|--|--|
| Seed | `upstream/RunZC.ZC` (+ `lib/RedSeaCFile.ZC`) writes `Hi.ZC` (`return 42;`) |
| Load | C `rs_load_file` reads RedSea root file into a buffer |
| Shell | `runzc` — seed if needed, load `Hi.ZC`, compile+run via HolyC front |
| Smoke | `hc: Upstream RunZC seed` then `RunZC Hi.ZC => 0x2a` |

## Milestone 35 — run real upstream `.ZC` from RedSea (this tree)

| Item | Path |
|--|--|
| File | `upstream/NetOfDots.ZC` (~572 B, multi-clus) |
| Seed | C `rs_put_file` writes contiguous clusters from embedded source |
| Load | `rs_load_file` reads across sectors (no 512-byte cap) |
| Shell | `runzc NetOfDots.ZC` — load from PCI RedSea, JIT, fan on FB |
| Smoke | `hc: Upstream RunZC NetOfDots.ZC ok n=0x23c` (or similar) |

## Milestone 36 — RedSea catalog + second program from disk (this tree)

| Item | Path |
|--|--|
| Dir | C `rs_dir_print` / shell `rsdir` lists root name+size+blk |
| Persist | `rs_put_file_if_absent` — never overwrite existing files |
| File | `upstream/Lines.ZC` (~1497 B, 3 clus) via `runzc Lines.ZC` |
| Smoke | `RunZC Lines.ZC ok` + `rsdir ok` (n≥3) |

## Milestone 37 — RedSea usability (this tree)

| Item | Path |
|--|--|
| Case | `str_ieq` on find/load/del/rename — `runzc lines.zc` works |
| Catalog | `g_rs_catalog[]` + `rs_seed_catalog` / shell `rscatalog` |
| Delete | `rs_del_file` / shell `rsdel <name>` (frees multi-clus bits) |
| Rename | `rs_rename_file` / shell `rsrename old new` |
| Smoke | lowercase NetOfDots load; MemSort from disk; rename+del Tmp |

## Milestone 38 — multi-file HolyC `#include` from RedSea (this tree)

| Item | Path |
|--|--|
| Preprocess | `hc_expand_includes` loads `#include "File.ZC"` from RedSea root |
| Lib | `upstream/AddLib.ZC` → `Add(a,b)` |
| Main | `upstream/UseAdd.ZC` → `#include "AddLib.ZC"`; `return Add(20,22);` |
| Shell | `runzc UseAdd.ZC` → **`0x2a`** |
| Smoke | `hc: Upstream UseAdd.ZC => 0x2a (#include)` |

## Milestone 39 — Notes (DocLib + program from RedSea) (this tree)

| Item | Path |
|--|--|
| Lib | `upstream/DocLib.ZC` — CDoc / DocNew / DocPrint |
| Prog | `upstream/Notes.ZC` — `#include "DocLib.ZC"`; note list on FB |
| Return | tag length sum **14** (`0xe`) |
| Catalog | AddLib, UseAdd, DocLib, Notes, MemSort (8-slot safe) |
| Freeze | [ACCEPTANCE.md](../ACCEPTANCE.md) M33–M39 regression bar |

## Milestone 40 — file-scope globals (frontend growth)

| Item | Path |
|--|--|
| Gap | File-scope `I64` / `U8[]` shared across functions (`HC_LD_ABS` / `HC_ST_ABS` / `HC_ABS_ADDR`) |
| Upstream need | `Cartesian.ZC` / `Life.ZC` (“x must be global”) |
| Bring-up | `upstream/GlobShare.ZC` — `gen`/`live`/`marks[]`; helpers mutate; return **8** |
| Smoke | `hc: Front Glob` (expect 3) + `hc: Upstream GlobShare` (expect 8) |
| Shell | `globshare` |

Does **not** extend the M33–M39 UTM freeze catalog (8-slot root stays as-is).

## Milestone 41 — Life step on global grids (frontend growth)

| Item | Path |
|--|--|
| Upstream | `Demo/Graphics/Life.ZC` neighbor / next-buffer shape |
| Bring-up | `upstream/Life.ZC` — global `cur`/`nxt`/`W`; still-life block; return **4** |
| Smoke | `hc: Upstream Life` (expect 4) |
| Shell | `life` |

## Milestone 42 — F64 + CartLite (frontend growth)

| Item | Path |
|--|--|
| Gap | `F64` arith/cmp + `ToI64`/`ToF64`; CPACR FPEN; literals via `I2F`; loads set `expr_f64` |
| Upstream need | `Cartesian.ZC` global `F64 x,y` |
| Bring-up | `upstream/CartLite.ZC` — `y=2*x` sample; return **14** |
| Smoke | `Front F64`/`Neg`/`Cmp`/`Step`/`While`(9) + `Upstream CartLite`(14) |
| Shell | `cartlite` |

CDC / DocClear / StrGet / ExePrint still deferred. Kernel C stays `+nofp`; JIT emits FP ops.

## Milestone 43 — F64 class members + Vec2Lite (frontend growth)

| Item | Path |
|--|--|
| Gap | `class` members may be `F64`; member load/store/`+=`; `F64 !=` (`HC_FNE`) |
| Upstream need | `Lattice.ZC` / `Turtle` — `class { F64 x,y,… }` |
| Bring-up | `upstream/Vec2Lite.ZC` — `3²+4²`; return **25** |
| Smoke | `Front F64Ne`(2) + `Upstream Vec2Lite`(25) |
| Shell | `vec2lite` |

Does **not** extend the M33–M39 UTM freeze catalog.

## Milestone 44 — F64 array brace init + AnglesLite (frontend growth)

| Item | Path |
|--|--|
| Gap | `Type a[N] = { e0, e1, … };` local + file-scope (remainder zeroed) |
| Upstream need | `Lattice.ZC` — `F64 angles[N] = { … }` |
| Bring-up | `upstream/AnglesLite.ZC` — global+local tables; return **12** |
| Smoke | `hc: Upstream AnglesLite` (expect 12) |
| Shell | `angleslite` |

Does **not** extend the M33–M39 UTM freeze catalog. Cos/Sin/π deferred (softfloat under `+nofp`).

## Milestone 45 — Cos/Sin + pi + CosLite (frontend growth)

| Item | Path |
|--|--|
| Gap | `Cos`/`Sin` builtins (F64); `pi` constant; `hc_f64math.c` FP TU (Taylor, no libm) |
| Notes | Fold to `[-π/2,π/2]`; finite `|x|<2^20` else qNaN; neg-quadrant Sin fixed |
| Upstream need | `Lattice.ZC` — Cos/Sin/ã |
| Bring-up | `upstream/CosLite.ZC` — Cos/Sin(0/pi/±3pi/4); return **63** |
| Smoke | `hc: Upstream CosLite` (expect 63) |
| Shell | `coslite` |

Kernel stub stays `+nofp`; only `hc_f64math.c` is built with FP. Freeze catalog unchanged.

## Milestone 46 — Sqrt + Wrap + SqrtLite (frontend growth)

| Item | Path |
|--|--|
| Gap | `Sqrt(x)`; `Wrap(θ)` / `Wrap(θ, base)` (ZealOS `[base, base+2π)`) |
| Upstream need | `Lattice.ZC` — `Sqrt(y2)`; `Wrap(θ + …)` |
| Bring-up | `upstream/SqrtLite.ZC` — Sqrt + Wrap cases; return **31** |
| Smoke | `hc: Upstream SqrtLite` (expect 31) |
| Shell | `sqrtlite` |

Freeze catalog unchanged.

## Milestone 47 — Arg + AbsI64/MemCopy + ArgLite (frontend growth)

| Item | Path |
|--|--|
| Gap | `Arg(x,y)` atan2; aliases `AbsI64`→`Abs`, `MemCopy`→`MemCpy` |
| Upstream need | `Lattice.ZC` — `Arg(dx,dy)`; `AbsI64`; `MemCopy` |
| Bring-up | `upstream/ArgLite.ZC` — quadrants + AbsI64 + MemCopy; return **63** |
| Smoke | `hc: Upstream ArgLite` (expect 63) |
| Shell | `arglite` |

Freeze catalog unchanged.

## Milestone 48 — class member commas + CommaLite (frontend growth)

| Item | Path |
|--|--|
| Gap | `class { F64 x, y, z; }` comma member lists; `HC_MAX_MEMBERS` 8→16 |
| Upstream need | `Lattice.ZC` — `class Turtle { F64 x, y, z, speed, θ, w; ... }` |
| Bring-up | `upstream/CommaLite.ZC` — Vec3/Pair/TurtleLite commas; return **63** |
| Smoke | `hc: Upstream CommaLite` (expect 63) |
| Shell | `commalite` |

Freeze catalog unchanged.

## Milestone 49 — GrPlot3/GrLine3 + Plot3Lite (frontend growth)

| Item | Path |
|--|--|
| Gap | `GrPlot3(dc,x,y,z)` / `GrLine3(...)` 2D project; `HC_CALL7`; F64→I64 at call; `HC_IR_MAX_DEPTH` 8→16 |
| Upstream need | `Lattice.ZC` — TurtlePlot uses GrPlot3/GrLine3 |
| Bring-up | `upstream/Plot3Lite.ZC` — peek yellow pixels; return **15** |
| Smoke | `hc: Upstream Plot3Lite` (expect 15) |
| Shell | `plot3lite` |

Freeze catalog unchanged.

## Milestone 50 — TempleOS pi/theta charset + TosPiLite (frontend growth)

| Item | Path |
|--|--|
| Gap | TempleOS `0xE3` → pi; `0xE9` in idents; UTF-8 U+03C0 → pi |
| Upstream need | `Lattice.ZC` — pi glyph; `Turtle.θ` / `dθ_idx` |
| Bring-up | `upstream/TosPiLite.ZC` — charset smoke; return **15** |
| Smoke | `hc: Upstream TosPiLite` (expect 15) |
| Shell | `tospilite` |

Freeze catalog unchanged.

## Milestone 51 — TRUE/FALSE, palette, FAbs, DCAlias + ColorLite

| Item | Path |
|--|--|
| Gap | `TRUE`/`FALSE`; ZealOS colors 0..15; `FAbs`; `DCAlias()`→0 |
| Upstream need | `Lattice.ZC` — `while(TRUE)`, `YELLOW`, `DCAlias` |
| Bring-up | `upstream/ColorLite.ZC` — return **63** |
| Smoke | `hc: Upstream ColorLite` (expect 63) |
| Shell | `colorlite` |

Freeze catalog unchanged.

## Milestone 52 — F64/Class* fn params + bare DCAlias + TurtleLite

| Item | Path |
|--|--|
| Gap | Fn params `Class*` + `F64` (Lattice); bare `DCAlias`; nargs≤7 |
| Upstream need | `Lattice.ZC` — `TurtleMicroMove(Turtle *t, F64 dt)`; `dc = DCAlias` |
| Bring-up | `upstream/TurtleLite.ZC` — MicroMove + bare DCAlias; return **15** |
| Smoke | `hc: Upstream TurtleLite` (expect 15) |
| Shell | `turtlelite` |

Freeze catalog unchanged.

## Milestone 53 — F64-only params + DCFill/DCDel + FillLite

| Item | Path |
|--|--|
| Gap | Clear `expr_f64` after user calls; `DCFill`/`DCDel` stubs |
| Upstream need | `Lattice.ZC` — `F(dt)==…`; `DCFill(dc)`; `DCDel(dc)` |
| Bring-up | `upstream/FillLite.ZC` — return **15** |
| Smoke | `hc: Upstream FillLite` (expect 15) |
| Shell | `filllite` |

Freeze catalog unchanged.

## Milestone 54 — Fs->pix_* + I64→F64 assign + InitLite

| Item | Path |
|--|--|
| Gap | `Fs->pix_width` / `pix_height`; I64 RHS → F64 store via `I2F` |
| Upstream need | `Lattice.ZC` TurtleInit centering |
| Bring-up | `upstream/InitLite.ZC` — return **15** |
| Smoke | `hc: Upstream InitLite` (expect 15) |
| Shell | `initlite` |

Freeze catalog unchanged.

## Milestone 55 — #define + mixed I64/F64 arith + DefLite

| Item | Path |
|--|--|
| Gap | Object-like `#define`; I64↔F64 `*`/`/`/`+`/`-` promote |
| Upstream need | `Lattice.ZC` — `TURTLE_SIZE`, `ANGLES`, `TURTLE_SIZE * Cos` |
| Bring-up | `upstream/DefLite.ZC` — return **15** |
| Smoke | `hc: Upstream DefLite` (expect 15) |
| Shell | `deflite` |

Freeze catalog unchanged.

## Milestone 56 — GrPrint + PrintLite

| Item | Path |
|--|--|
| Gap | `GrPrint(dc,x,y,fmt[,…])` (literal fmt; %f deferred) |
| Upstream need | `Lattice.ZC` HUD `GrPrint(dc, 0, 0, "Layer:…")` |
| Bring-up | `upstream/PrintLite.ZC` — return **15** |
| Smoke | `hc: Upstream PrintLite` (expect 15) |
| Shell | `printlite` |

Freeze catalog unchanged.

## Milestone 57 — DCDepthBufAlloc / PopUpColor / MessageGet + MsgLite

| Item | Path |
|--|--|
| Gap | Depth-buf / color-popup / message stubs + `MESSAGE_*` |
| Upstream need | `Lattice.ZC` event/color/depth setup |
| Bring-up | `upstream/MsgLite.ZC` — return **15** |
| Smoke | `hc: Upstream MsgLite` (expect 15) |
| Shell | `msglite` |
| Deferred | Real event queue |

Freeze catalog unchanged.

## Milestone 58 — omitted call args + MenuPush/MenuPop + MenuLite

| Item | Path |
|--|--|
| Gap | Empty call slots → 0; `MenuPush`/`MenuPop`; bare call stmts |
| Upstream need | `Lattice.ZC` — `MessageGet(,,mask)`; `MenuPush`/`MenuPop` |
| Bring-up | `upstream/MenuLite.ZC` — return **15** |
| Smoke | `hc: Upstream MenuLite` (expect 15) |
| Shell | `menulite` |

Freeze catalog unchanged.

## Milestone 59 — assign-expr / string concat / MenuEntryFind + FindLite

| Item | Path |
|--|--|
| Gap | `x=expr` as value; `"a" "b"` concat; `MenuEntryFind`; `Fs->cur_menu`; WIF/WIG |
| Upstream need | `Lattice.ZC` SetMenu / MenuPush / win_inhibit |
| Bring-up | `upstream/FindLite.ZC` — return **15** |
| Smoke | `hc: Upstream FindLite` (expect 15) |
| Shell | `findlite` |

Freeze catalog unchanged.

## Milestone 60 — Fs->win_inhibit/draw_it + anon params + CH_/SC_ + FsLite

| Item | Path |
|--|--|
| Gap | `Fs->win_inhibit`/`draw_it` r/w; anonymous `CTask *`; `CH_SHIFT_ESC`/`SC_CURSOR_*` |
| Upstream need | `Lattice.ZC` DrawIt hook + win_inhibit + key constants |
| Bring-up | `upstream/FsLite.ZC` — return **15** |
| Smoke | `hc: Upstream FsLite` (expect 15) |
| Shell | `fslite` |

Freeze catalog unchanged.

## Milestone 61 — case char/range + setup stubs + SetupLite

| Item | Path |
|--|--|
| Gap | `case 'c'` / `CH_*` / `'0'...'9'`; Settings/Win/Doc/try/catch stubs |
| Upstream need | `Lattice.ZC` Lattice() shell setup + key switch |
| Bring-up | `upstream/SetupLite.ZC` — return **15** |
| Smoke | `hc: Upstream SetupLite` (expect 15) |
| Shell | `setuplite` |

Freeze catalog unchanged.

## Milestone 62 — global class instance + CColorROPU16 + TtLite

| Item | Path |
|--|--|
| Gap | `} tt;` after class; `CColorROPU16` members/params |
| Upstream need | `Lattice.ZC` Turtle global `tt` + color fields |
| Bring-up | `upstream/TtLite.ZC` — return **15** |
| Smoke | `hc: Upstream TtLite` (expect 15) |
| Shell | `ttlite` |

Freeze catalog unchanged.

## Milestone 63 — STR_LEN + StrPrint 3-arg + I64.u8[] + BufLite

| Item | Path |
|--|--|
| Gap | `STR_LEN`; `StrPrint(buf,fmt,a)`; `arg2.u8[0]` |
| Upstream need | `Lattice.ZC` SetMenu StrPrint + scan-code byte view |
| Bring-up | `upstream/BufLite.ZC` — return **15** |
| Smoke | `hc: Upstream BufLite` (expect 15) |
| Shell | `buflite` |

Freeze catalog unchanged.

## Milestone 64 — member ++/-- + arrow/dot assign-expr + IncLite

| Item | Path |
|--|--|
| Gap | `obj.m++` / `ptr->m--`; `if (p->m = e)` as value |
| Upstream need | `Lattice.ZC` turtle width/speed/idx updates |
| Bring-up | `upstream/IncLite.ZC` — return **15** |
| Smoke | `hc: Upstream IncLite` (expect 15) |
| Shell | `inclite` |

Freeze catalog unchanged.

## Milestone 65 — builtin CDC + dc->color + DcLite

| Item | Path |
|--|--|
| Gap | `CDC *dc`; `dc->color = …`; GrPlot3/GrLine3 use color |
| Upstream need | `Lattice.ZC` TurtlePlot / DrawIt CDC |
| Bring-up | `upstream/DcLite.ZC` — return **15** |
| Smoke | `hc: Upstream DcLite` (expect 15) |
| Shell | `dclite` |

Freeze catalog unchanged.

## Milestone 66 — GrLine(dc,…) + &fn draw_it + LineDcLite

| Item | Path |
|--|--|
| Gap | `GrLine(dc,x1,y1,x2,y2)`; `Fs->draw_it = &DrawIt` |
| Upstream need | `Lattice.ZC` DrawIt HUD turtle marker |
| Bring-up | `upstream/LineDcLite.ZC` — return **15** |
| Smoke | `hc: Upstream LineDcLite` (expect 15) |
| Shell | `linedclite` |

Freeze catalog unchanged.

## Milestone 67 — GrPrint %f + 5 fmt args + GrfLite

| Item | Path |
|--|--|
| Gap | `GrPrint(…, "%f" / "%5.1f", …)` with up to 5 format args |
| Upstream need | `Lattice.ZC` DrawIt HUD |
| Bring-up | `upstream/GrfLite.ZC` — return **15** |
| Smoke | `hc: Upstream GrfLite` (expect 15) |
| Shell | `grflite` |

Freeze catalog unchanged.

## Milestone 68 — MemCopy class + !bool + angles[idx] + goto + MoveLite

| Item | Path |
|--|--|
| Gap | `MemCopy(&t2,&tt,sizeof)`; `t.ends=!t.ends`; `angles[t->idx]`; `goto` |
| Upstream need | `Lattice.ZC` TurtleEnd/MicroMove/Ends toggle |
| Bring-up | `upstream/MoveLite.ZC` — return **15** |
| Smoke | `hc: Upstream MoveLite` (expect 15) |
| Shell | `movelite` |

Freeze catalog unchanged.

## Milestone 69 — builtin CMenuEntry + MenuEntryFind stub + CheckedLite

| Item | Path |
|--|--|
| Gap | `CMenuEntry *`; `MenuEntryFind` → stub; `tmpse->checked` |
| Upstream need | `Lattice.ZC` SetMenu checked flags |
| Bring-up | `upstream/CheckedLite.ZC` — return **15** |
| Smoke | `hc: Upstream CheckedLite` (expect 15) |
| Shell | `checkedlite` |

Freeze catalog unchanged.

## Milestone 70 — F64/I64 compare promote + CmpLite

| Item | Path |
|--|--|
| Gap | `if (w < 0)` / `if (y2 >= 0)` mixed F64/I64 cmp |
| Upstream need | `Lattice.ZC` TurtlePlot / TurtleEnd |
| Bring-up | `upstream/CmpLite.ZC` — return **15** |
| Smoke | `hc: Upstream CmpLite` (expect 15) |
| Shell | `cmplite` |

Freeze catalog unchanged.

## Milestone 71 — for-step i++/i-- + ForIncLite

| Item | Path |
|--|--|
| Gap | `for (i=0; i<n; i++)` / `i--` as step |
| Upstream need | `Lattice.ZC` TurtleMove / SetMenu loops |
| Bring-up | `upstream/ForIncLite.ZC` — return **15** |
| Smoke | `hc: Upstream ForIncLite` (expect 15) |
| Shell | `forinclite` |

Freeze catalog unchanged.

## Milestone 72 — TurtleMicroMove-shaped MicroLite

| Item | Path |
|--|--|
| Gap | Integrated `TurtleMicroMove` (Cos/Sin/Wrap/angles); fn names ≤23 |
| Upstream need | `Lattice.ZC` TurtleMicroMove |
| Bring-up | `upstream/MicroLite.ZC` — return **15** |
| Smoke | `hc: Upstream MicroLite` (expect 15) |
| Shell | `microlite` |

Freeze catalog unchanged.

## Milestone 73 — TurtlePlot+MicroMove+TurtleMove compose + MoveStackLite

| Item | Path |
|--|--|
| Gap | Compose Plot/MicroMove/Move with CDC color + Gr*3 |
| Upstream need | `Lattice.ZC` TurtleMove / DrawIt Step |
| Bring-up | `upstream/MoveStackLite.ZC` — return **15** |
| Smoke | `hc: Upstream MoveStackLite` (expect 15) |
| Shell | `movestacklite` |

Freeze catalog unchanged.

## Milestone 74 — TurtleEnd + AbsI64(F64) + EndLite

| Item | Path |
|--|--|
| Gap | `AbsI64(F64)` via F2I; `TurtleEnd` while/Sqrt/MemCopy; `ends` path |
| Upstream need | `Lattice.ZC` TurtleEnd / TurtleMove AbsI64 step count |
| Bring-up | `upstream/EndLite.ZC` — return **15** |
| Smoke | `hc: Upstream EndLite` (expect 15) |
| Shell | `endlite` |

Freeze catalog unchanged.

## Milestone 75 — DrawIt compose + DrawItLite

| Item | Path |
|--|--|
| Gap | Full DrawIt: MemCopy tt, GrPrint HUD, TurtleMove, LTRED head GrLines; `GrLine` F64→I64 |
| Upstream need | `Lattice.ZC` DrawIt |
| Bring-up | `upstream/DrawItLite.ZC` — return **15** |
| Smoke | `hc: Upstream DrawItLite` (expect 15) |
| Shell | `drawitlite` |

Freeze catalog unchanged.

## Milestone 76 — Lattice setup+Step+cleanup + LatticeLite

| Item | Path |
|--|--|
| Gap | Compose TurtleInit/SetMenu/Fs hooks/Step/cleanup; bare 0-arg user fn (`SetMenu;`) |
| Upstream need | `Lattice.ZC` Lattice() entry scaffolding + CH_SPACE Step |
| Bring-up | `upstream/LatticeLite.ZC` — return **15** |
| Smoke | `hc: Upstream LatticeLite` (expect 15) |
| Shell | `latticelite` |

Freeze catalog unchanged.

## Milestone 77 — MessageGet queue + LoopLite

| Item | Path |
|--|--|
| Gap | Scripted `MsgQuePush` + `MessageGet` pop; Lattice `while/switch/goto` ESC exit |
| Upstream need | `Lattice.ZC` event loop |
| Bring-up | `upstream/LoopLite.ZC` — return **15** |
| Smoke | `hc: Upstream LoopLite` (expect 15) |
| Shell | `looplite` |

Freeze catalog unchanged.

## Milestone 78 — Lattice setup+loop stitch + DemoLite

| Item | Path |
|--|--|
| Gap | Compose Init/SetMenu/Fs + scripted SPACE Step + ESC exit + cleanup |
| Upstream need | `Lattice.ZC` Lattice() end-to-end (scripted, not interactive) |
| Bring-up | `upstream/DemoLite.ZC` — return **15** |
| Smoke | `hc: Upstream DemoLite` (expect 15) |
| Shell | `demolite` |

Freeze catalog unchanged.

## Milestone 79 — Lattice mouse/key handlers + EventLite

| Item | Path |
|--|--|
| Gap | MS_L/R + Arg aim; KEY `arg1==0` scancodes; `'e'`/`'0'...'9'` |
| Upstream need | `Lattice.ZC` event switch body |
| Bring-up | `upstream/EventLite.ZC` — return **15** |
| Smoke | `hc: Upstream EventLite` (expect 15) |
| Shell | `eventlite` |

Freeze catalog unchanged.

## Milestone 80 — Demo+Event play path + PlayLite

| Item | Path |
|--|--|
| Gap | Place → aim → SPACE Step → ESC over Lattice scaffolding |
| Upstream need | `Lattice.ZC` play interaction path |
| Bring-up | `upstream/PlayLite.ZC` — return **15** |
| Smoke | `hc: Upstream PlayLite` (expect 15) |
| Shell | `playlite` |

Freeze catalog unchanged.

## Milestone 81 — MessageGet mask + virtio fallback + InputLite

| Item | Path |
|--|--|
| Gap | Mask-filtered queue; empty → virtio kbd/tablet; `<<` above `\|` precedence |
| Upstream need | `Lattice.ZC` interactive `MessageGet` |
| Bring-up | `upstream/InputLite.ZC` — return **31**; Front `MsgMask` == 36 |
| Smoke | `hc: Upstream InputLite` (expect 31); `hc: Front MsgMask` (expect 36) |
| Shell | `inputlite` |

Freeze catalog unchanged.

## Milestone 82 — MessageGet UART when !virtio_kbd (Pi)

| Item | Path |
|--|--|
| Gap | Pi HolyC keys via UART when virtio keyboard is absent |
| Upstream need | `Lattice.ZC` interactive `MessageGet` on board (no virtio-input) |
| Bring-up | `hc_builtin_messageget`: KEY_DOWN → `virtio_kbd_getc_nb` if ready, else `uart_getc_nb`; never both |
| Smoke | Same as M81 — QEMU keeps virtio path (`InputLite` 31 / `MsgMask` 36); serial stays MESSAGE_NULL with empty queue |
| Shell | `inputlite` |

Freeze catalog unchanged. Hands-on Pi UART board still separate.

## Milestone 83 — MessageGet MS_R from virtio tablet + RightLite

| Item | Path |
|--|--|
| Gap | Lattice aim via right-button `MS_R_DOWN`/`MS_R_UP` from tablet |
| Upstream need | `Lattice.ZC` `MS_R_*` → `Arg(dx,dy)` |
| Bring-up | `virtio_tablet` `BTN_RIGHT` bit2; `MessageGet` edge → 7/8; `upstream/RightLite.ZC` — return **15** |
| Smoke | `hc: Upstream RightLite` (expect 15); mask `1<<7\|1<<8` == 384 |
| Shell | `rightlite` |

Freeze catalog unchanged.

## Milestone 84 — MessageGet cursor scancodes + CursorLite

| Item | Path |
|--|--|
| Gap | Lattice layer keys: `KEY_DOWN` with `arg1==0`, `arg2.u8[0]==SC_CURSOR_*` |
| Upstream need | `Lattice.ZC` cursor left/right → `tt.idx` |
| Bring-up | `virtio_kbd` arrows → msg `(0, SC_*)`; ASCII still `(ch,0)`; UART ASCII-only fallback |
| Smoke | `hc: Upstream CursorLite` (expect 15) |
| Shell | `cursorlite` |

Freeze catalog unchanged.

## Milestone 85 — MessageGet KEY_UP + UpLite

| Item | Path |
|--|--|
| Gap | Lattice ESC drain: `MessageGet(,, 1<<MESSAGE_KEY_UP)` |
| Upstream need | `Lattice.ZC` key-release after exit |
| Bring-up | Virtio release → typed `MESSAGE_KEY_UP` (3); mask-filtered like scripted queue; `upstream/UpLite.ZC` — **15** |
| Smoke | `hc: Upstream UpLite` (expect 15); mask `1<<2\|1<<3` == 12 |
| Shell | `uplite` |

Freeze catalog unchanged.

## Milestone 86 — Refresh → Fs->draw_it + TickLite

| Item | Path |
|--|--|
| Gap | WinMgr-shaped paint: `Refresh` calls `Fs->draw_it` |
| Upstream need | `Lattice.ZC` `Fs->draw_it = &DrawIt` frame redraw |
| Bring-up | `hc_builtin_refresh` → JIT `draw_it(0, DCAlias)`; `upstream/TickLite.ZC` — **15** |
| Smoke | `hc: Upstream TickLite` (expect 15) |
| Shell | `ticklite` |

Freeze catalog unchanged.

## Milestone 87 — MessageGet loop + Refresh + FrameLite

| Item | Path |
|--|--|
| Gap | Lattice frame: handle event then `Refresh` |
| Upstream need | `Lattice.ZC` interactive loop paint |
| Bring-up | `upstream/FrameLite.ZC` — place/aim/Step/ESC with `Refresh` after each; return **15** |
| Smoke | `hc: Upstream FrameLite` (expect 15) |
| Shell | `framelite` |

Freeze catalog unchanged.

## Milestone 88 — Lattice GrPlot(dc,x,y) / GrPeek(dc,…) + PlotDcLite

| Item | Path |
|--|--|
| Gap | ZealOS `GrPlot(dc,x,y)` / `GrPeek(dc,x,y)` vs bring-up `(x,y,c)` / `(x,y)` |
| Upstream need | `Lattice.ZC` CDC-colored plots |
| Bring-up | `hc_builtin_grplot` dual ABI; `GrPeek` 2- or 3-arg; `upstream/PlotDcLite.ZC` — **15** |
| Smoke | `hc: Upstream PlotDcLite` (expect 15) |
| Shell | `plotdclite` |

Freeze catalog unchanged.

## Milestone 89 — CH_SHIFT_ESC Abort + AbortLite

| Item | Path |
|--|--|
| Gap | Lattice Abort: `CH_SHIFT_ESC` (0x1C) distinct from `CH_ESC` |
| Upstream need | `Lattice.ZC` `Abort(,CH_SHIFT_ESC)` / `case CH_SHIFT_ESC` |
| Bring-up | Virtio Shift+Esc → `0x1C`; `upstream/AbortLite.ZC` — **15** |
| Smoke | `hc: Upstream AbortLite` (expect 15) |
| Shell | `abortlite` |

Freeze catalog unchanged.

## Milestone 90 — MS_MOVE while aiming + AimMoveLite

| Item | Path |
|--|--|
| Gap | Lattice aim-drag: `MS_MOVE` updates `Arg` while right button down |
| Upstream need | `Lattice.ZC` `aim` + `MESSAGE_MS_MOVE` |
| Bring-up | `upstream/AimMoveLite.ZC` — R_DOWN → MOVE → R_UP; post-aim MOVE ignored; return **15** |
| Smoke | `hc: Upstream AimMoveLite` (expect 15); mask `1<<4\|1<<7\|1<<8` == 400 |
| Shell | `aimmovelite` |

Freeze catalog unchanged.

## Milestone 91 — MESSAGE_NULL idle → Refresh + IdleLite

| Item | Path |
|--|--|
| Gap | Interactive idle: empty `MessageGet` → `Refresh` + `Sleep` |
| Upstream need | `Lattice.ZC` WinMgr-shaped frame when no events |
| Bring-up | `upstream/IdleLite.ZC` — 3 idle Refresh frames then ESC; return **15** |
| Smoke | `hc: Upstream IdleLite` (expect 15) |
| Shell | `idlelite` |

Freeze catalog unchanged.

## Milestone 92 — layer digit keys + SetMenu + LayerLite

| Item | Path |
|--|--|
| Gap | Lattice layer: `case '0'...'9'` → `tt.z` + `SetMenu` |
| Upstream need | `Lattice.ZC` digit layer + `Settings/Layer%d` checked |
| Bring-up | `upstream/LayerLite.ZC` — digits via MessageGet + SetMenu; return **15** |
| Smoke | `hc: Upstream LayerLite` (expect 15) |
| Shell | `layerlite` |

Freeze catalog unchanged.

## Milestone 93 — 'e' ends toggle + SetMenu + EndsLite

| Item | Path |
|--|--|
| Gap | Lattice ends marker: `case 'e'` → `tt.ends=!tt.ends` + `SetMenu` |
| Upstream need | `Lattice.ZC` `Settings/Ends` checked |
| Bring-up | `upstream/EndsLite.ZC` — three `'e'` then clear; return **15** |
| Smoke | `hc: Upstream EndsLite` (expect 15) |
| Shell | `endslite` |

Freeze catalog unchanged.

## Milestone 94 — speed/width keys + SpeedLite

| Item | Path |
|--|--|
| Gap | Lattice turtle tweaks: `+`/`=`/`-` speed; `]`/`[` width |
| Upstream need | `Lattice.ZC` interactive speed/width keys |
| Bring-up | `upstream/SpeedLite.ZC` — MessageGet key adjust; return **15** |
| Smoke | `hc: Upstream SpeedLite` (expect 15) |
| Shell | `speedlite` |

Freeze catalog unchanged.

## Milestone 95 — PopUpColor keys + MidLite

| Item | Path |
|--|--|
| Gap | Lattice color pick: `'c'` mid / `'C'` edge → `PopUpColor` |
| Upstream need | `Lattice.ZC` `tt.middle` / `tt.edge` from picker |
| Bring-up | `upstream/MidLite.ZC` — MessageGet `'c'`/`'C'`; PopUpColor → YELLOW; return **15** |
| Smoke | `hc: Upstream MidLite` (expect 15) |
| Shell | `midlite` |

Freeze catalog unchanged.

## Milestone 96 — Lattice event-loop stitch + LiveLite

| Item | Path |
|--|--|
| Gap | Compose place/aim/layer/ends/color/speed/Step/Refresh/ESC |
| Upstream need | `Lattice.ZC` interactive loop (scripted) |
| Bring-up | `upstream/LiveLite.ZC` — M87–M95 handlers in one loop; return **15** |
| Smoke | `hc: Upstream LiveLite` (expect 15) |
| Shell | `livelite` |

Freeze catalog unchanged.

## Milestone 97 — Cursor UP/DOWN speed + AccelLite

| Item | Path |
|--|--|
| Gap | Lattice Accelerator/Break: `SC_CURSOR_UP`/`DOWN` → `tt.speed` |
| Upstream need | `Lattice.ZC` `case SC_CURSOR_UP` / `SC_CURSOR_DOWN` |
| Bring-up | `upstream/AccelLite.ZC` — MessageGet(0, SC_*); ±`TURTLE_SPEED_STEP`; return **15** |
| Smoke | `hc: Upstream AccelLite` (expect 15) |
| Shell | `accellite` |

Freeze catalog unchanged.

## Milestone 98 — Restart key + RestartLite

| Item | Path |
|--|--|
| Gap | Lattice Restart: `'\n'` → `DCFill` + `TurtleInit` + `SetMenu` |
| Upstream need | `Lattice.ZC` `case '\n'` |
| Bring-up | `upstream/RestartLite.ZC` — mutate then KEY `'\n'` resets; return **15** |
| Smoke | `hc: Upstream RestartLite` (expect 15) |
| Shell | `restartlite` |

Freeze catalog unchanged.

## Milestone 99 — Lattice width keys + WidthLite

| Item | Path |
|--|--|
| Gap | Lattice Wider/Narrower: `'+'`/`'-'` → `tt.w++` / `tt.w--` |
| Upstream need | `Lattice.ZC` `case '+'` / `case '-'` (width, not speed) |
| Bring-up | `upstream/WidthLite.ZC` — MessageGet `+`/`-`; floor at 0; return **15** |
| Smoke | `hc: Upstream WidthLite` (expect 15) |
| Shell | `widthlite` |

Freeze catalog unchanged.

## Milestone 100 — Lattice 'c' dual color + BothColorLite

| Item | Path |
|--|--|
| Gap | Lattice `'c'` sets **both** `tt.middle` and `tt.edge` via `PopUpColor` |
| Upstream need | `Lattice.ZC` `case 'c'` (two Pickers, no `'C'`) |
| Bring-up | `upstream/BothColorLite.ZC` — one KEY `'c'` → mid+edge YELLOW; return **15** |
| Smoke | `hc: Upstream BothColorLite` (expect 15) |
| Shell | `bothcolorlite` |

Freeze catalog unchanged.

## Milestone 101 — Lattice multi-section MenuPush + MenuFullLite

| Item | Path |
|--|--|
| Gap | Lattice `MenuPush` File/Play/Settings via adjacent string concat |
| Upstream need | `Lattice.ZC` menu table + `SetMenu` Layer0–9 / Ends |
| Bring-up | `upstream/MenuFullLite.ZC` — full menu text + SetMenu; return **15** |
| Smoke | `hc: Upstream MenuFullLite` (expect 15) |
| Shell | `menufulllite` |

Freeze catalog unchanged.

## Milestone 102 — Lattice MenuPush capacity + MenuBigLite

| Item | Path |
|--|--|
| Gap | Full Lattice `MenuPush` (~414 chars) overflowed concat buf 240 + IR 2K |
| Upstream need | `Lattice.ZC` File/Play/Settings (all Layer0–9 entries) |
| Bring-up | concat `buf[241]`→`[512]`; `bc[2048]`→`[4096]`; `MenuBigLite.ZC` → **15** |
| Smoke | `hc: Upstream MenuBigLite` (expect 15) |
| Shell | `menubiglite` |

Freeze catalog unchanged.

## Milestone 103 — Lattice try/catch + TryLite

| Item | Path |
|--|--|
| Gap | Lattice `try { … } catch PutExcept;` |
| Upstream need | `Lattice.ZC` outer try/catch around MessageGet loop |
| Bring-up | `upstream/TryLite.ZC` — try/catch as stmts; return **15** |
| Smoke | `hc: Upstream TryLite` (expect 15) |
| Shell | `trylite` |

Freeze catalog unchanged.

## Milestone 104 — Lattice TurtleMove step count + StepCountLite

| Item | Path |
|--|--|
| Gap | `l = 16 * AbsI64(t->w + 1) * AbsI64(t->speed + 1)` |
| Upstream need | `Lattice.ZC` `TurtleMove` microstep count |
| Bring-up | `upstream/StepCountLite.ZC` — real 16× formula + short move; return **15** |
| Smoke | `hc: Upstream StepCountLite` (expect 15) |
| Shell | `stepcountlite` |

Freeze catalog unchanged.

## Milestone 105 — Lattice angles[35] + AnglesFullLite

| Item | Path |
|--|--|
| Gap | `F64 angles[35]` + Lattice ±2π/n table; long negative F64 brace inits |
| Upstream need | `Lattice.ZC` `ANGLES` / `angles[]` curvature table |
| Bring-up | `HC_IR_MAX_GLOBS` 32→48; skip F64 frac>9; fill via `AnglesFill` π exprs → **15** |
| Smoke | `hc: Upstream AnglesFullLite` (expect 15) |
| Shell | `anglesfulllite` |

Freeze catalog unchanged.

## Milestone 106 — angles[] brace init + BraceAnglesLite

| Item | Path |
|--|--|
| Gap | Full `F64 angles[35] = { -…, 0, +… }` hit `HC_IR_ERR_SPACE` (`g_jit_buf` 1536) |
| Upstream need | `Lattice.ZC` brace-initialized curvature table |
| Bring-up | `g_jit_buf` 1536→8192; `upstream/BraceAnglesLite.ZC` → **15** |
| Smoke | `hc: Upstream BraceAnglesLite` (expect 15) |
| Shell | `braceangleslite` |

Freeze catalog unchanged.

## Milestone 107 — angles[] brace π exprs + BracePiLite

| Item | Path |
|--|--|
| Gap | Lattice `F64 angles[35] = { -2*π/n … }` (ASCII `pi`) |
| Upstream need | `Lattice.ZC` curvature table as expressions |
| Bring-up | `upstream/BracePiLite.ZC` — brace init with `pi`; return **15** |
| Smoke | `hc: Upstream BracePiLite` (expect 15) |
| Shell | `bracepilite` |

Freeze catalog unchanged.

## Milestone 108 — per-path MenuEntryFind + SetMenuLite

| Item | Path |
|--|--|
| Gap | Shared `CMenuEntry` stub: SetMenu could not clear sibling layers |
| Upstream need | `Lattice.ZC` SetMenu Layer0–9 / Ends independently |
| Bring-up | `MenuEntryFind` path table; `upstream/SetMenuLite.ZC` → **15** |
| Smoke | `hc: Upstream SetMenuLite` (expect 15) |
| Shell | `setmenulite` |

Freeze catalog unchanged.

## Milestone 109 — Lattice-accurate keys + NearLatticeLite

| Item | Path |
|--|--|
| Gap | Compose Lattice key map: ±width, cursor speed, `'c'` dual color |
| Upstream need | `Lattice.ZC` interactive switch (scripted) |
| Bring-up | `NearLatticeLite.ZC`; switch cases 16→32; labels 64→128; return **15** |
| Smoke | `hc: Upstream NearLatticeLite` (expect 15) |
| Shell | `nearlatticelite` |

Freeze catalog unchanged.

## Milestone 110 — F64 if-cond + width floor + F64IfLite

| Item | Path |
|--|--|
| Gap | `if (tt.w)` in NearLattice: JIT `patches[64]` overflow; F64 cond needed `F2I` |
| Upstream need | `Lattice.ZC` `case '-': if (tt.w) tt.w--;` |
| Bring-up | fn IR 8K; JIT patches 64→256; `HC_F2I` before `JZ`; loops 4→8; `F64IfLite`; **15** |
| Smoke | `hc: Upstream F64IfLite` (expect 15) |
| Shell | `f64iflite` |

Freeze catalog unchanged.

## Milestone 111 — try/catch + newline restart + WrapLatticeLite

| Item | Path |
|--|--|
| Gap | Compose Lattice `try/catch` with `'\n'` TurtleInit restart |
| Upstream need | `Lattice.ZC` try wrap + newline restart in one loop |
| Bring-up | `upstream/WrapLatticeLite.ZC` — mutate → `'\n'` → init; return **15** |
| Smoke | `hc: Upstream WrapLatticeLite` (expect 15) |
| Shell | `wraplatticelite` |

Freeze catalog unchanged.

## Milestone 112 — SetMenu clear in key loop + MenuLoopLite

| Item | Path |
|--|--|
| Gap | Full SetMenu sibling-clear only stood alone; not called from digit keys |
| Upstream need | `Lattice.ZC` `case '0'...'9': … SetMenu;` clears Layer0–9 |
| Bring-up | `upstream/MenuLoopLite.ZC` — MessageGet `'5'`→`'2'`; Layer2 on / Layer5 off; **15** |
| Smoke | `hc: Upstream MenuLoopLite` (expect 15) |
| Shell | `menulooplite` |

Freeze catalog unchanged.

## Milestone 113 — all SC_CURSOR keys + IdxAllLite

| Item | Path |
|--|--|
| Gap | NearLattice/AccelLite covered UP/DOWN; LEFT/RIGHT idx lived in separate smokes |
| Upstream need | `Lattice.ZC` one `case 0` switch for all four `SC_CURSOR_*` |
| Bring-up | `upstream/IdxAllLite.ZC` — idx clamp + speed ±step; return **15** |
| Smoke | `hc: Upstream IdxAllLite` (expect 15) |
| Shell | `idxalllite` |

Freeze catalog unchanged.

## Milestone 114 — NearLattice full SetMenu + LEFT/RIGHT

| Item | Path |
|--|--|
| Gap | NearLattice used slim SetMenu + UP/DOWN only |
| Upstream need | `Lattice.ZC` SetMenu Layer0–9 clear + all `SC_CURSOR_*` in compose |
| Bring-up | `NearLatticeLite.ZC` — full SetMenu for-loop; LEFT/RIGHT idx; still **15** |
| Smoke | `hc: Upstream NearLatticeLite` (expect 15) |
| Shell | `nearlatticelite` |

Freeze catalog unchanged.

## Milestone 115 — Lattice compose from RedSea + DiskLat

| Item | Path |
|--|--|
| Gap | No `runzc`-shaped Lattice path (freeze catalog stays 8-slot) |
| Upstream need | Load Lattice-like `.ZC` from RedSea and JIT |
| Bring-up | `upstream/DiskLat.ZC` via `rs_put_file`→`rs_load_file`→`hc_run_src` → **15** |
| Smoke | `hc: Upstream DiskLat` (expect 15) |
| Shell | `disklat` |

Freeze catalog unchanged (ephemeral `DiskLat.ZC`).

## Milestone 116 — 16× TurtleMove in NearLattice compose

| Item | Path |
|--|--|
| Gap | NearLattice Step used `l=4`; Lattice uses `16*AbsI64(w+1)*AbsI64(speed+1)` |
| Upstream need | `Lattice.ZC` TurtleMove step count in the interactive loop |
| Bring-up | `NearLatticeLite.ZC` — real formula (scripted w=1,speed=2 → 96 micros); **15** |
| Smoke | `hc: Upstream NearLatticeLite` (expect 15) |
| Shell | `nearlatticelite` |

Freeze catalog unchanged.

## Milestone 117 — try/catch + newline in NearLattice compose

| Item | Path |
|--|--|
| Gap | NearLattice lacked Lattice `try/catch` and `'\n'` restart in-compose |
| Upstream need | `Lattice.ZC` try wrap + newline restart alongside the key map |
| Bring-up | `NearLatticeLite.ZC` — try/catch + `case '\n'`; still **15** |
| Smoke | `hc: Upstream NearLatticeLite` (expect 15) |
| Shell | `nearlatticelite` |

Freeze catalog unchanged.

## Milestone 118 — multi-section MenuPush in NearLattice/DiskLat

| Item | Path |
|--|--|
| Gap | NearLattice/DiskLat used slim `MenuPush("File…")`; Lattice uses File/Play/Settings |
| Upstream need | `Lattice.ZC` `MenuPush` File/Play/Settings adjacent-string concat |
| Bring-up | `NearLatticeLite.ZC` + `DiskLat.ZC` — multi-section MenuPush; still **15** |
| Smoke | `hc: Upstream NearLatticeLite` / `DiskLat` (expect 15) |
| Shell | `nearlatticelite` / `disklat` |

Freeze catalog unchanged.


## Milestone 119 — DCDepthBufAlloc → CDC.depth_buf + DepthBufLite

| Item | Path |
|--|--|
| Gap | `DCDepthBufAlloc` was a no-op; Lattice needs `dc->depth_buf` for GrPlot3 |
| Upstream need | `Lattice.ZC` / `GrDC.ZC` — `DCDepthBufAlloc(dc)` returns `I32 *` |
| Bring-up | Builtin CDC `depth_buf` @8; stub ptr; `DepthBufLite.ZC` → **15** |
| Smoke | `hc: Upstream DepthBufLite` (expect 15) |
| Shell | `depthbuflite` |

Freeze catalog unchanged.


## Milestone 120 — DCDepthBufReset + DepthRstLite

| Item | Path |
|--|--|
| Gap | TempleOS `DCDepthBufAlloc` returns `DCDepthBufReset` (I32_MAX fill); Reset was missing |
| Upstream need | `GrDC.ZC` — `DCDepthBufReset(dc)` |
| Bring-up | `hc_builtin_dcdepthbufreset`; Alloc→Reset; `DepthRstLite.ZC` → **15** |
| Smoke | `hc: Upstream DepthRstLite` (expect 15) |
| Shell | `depthrstlite` |

Freeze catalog unchanged.

## Milestone 121 — full Lattice MenuPush in NearLattice/DiskLat

| Item | Path |
|--|--|
| Gap | NearLattice/DiskLat MenuPush omitted Accelerator/Break/cursors + Wider/Layers |
| Upstream need | `Lattice.ZC` full File/Play/Settings `MenuPush` (~414 chars) |
| Bring-up | `NearLatticeLite.ZC` + `DiskLat.ZC` — MenuBigLite-sized MenuPush; still **15** |
| Smoke | `hc: Upstream NearLatticeLite` / `DiskLat` (expect 15) |
| Shell | `nearlatticelite` / `disklat` |

Freeze catalog unchanged.

## Milestone 122 — Lattice setup sequence in NearLattice/DiskLat

| Item | Path |
|--|--|
| Gap | NearLattice skipped `AutoComplete`/`WinBorder`/`WinMax`/`Doc*`/`win_inhibit` |
| Upstream need | `Lattice.ZC` setup between `SettingsPush` and the event loop |
| Bring-up | `NearLatticeLite.ZC` + `DiskLat.ZC` — full setup; DiskLat load buf 4K; still **15** |
| Smoke | `hc: Upstream NearLatticeLite` / `DiskLat` (expect 15) |
| Shell | `nearlatticelite` / `disklat` |

Freeze catalog unchanged.

## Milestone 123 — TurtleEnd in NearLattice TurtleMove

| Item | Path |
|--|--|
| Gap | NearLattice `TurtleMove` skipped Lattice `TurtleEnd` before/after the micro loop |
| Upstream need | `Lattice.ZC` — `if (ends && first) TurtleEnd(…+π)`; post-loop `TurtleEnd` |
| Bring-up | `NearLatticeLite.ZC` — EndLite-shaped `TurtleEnd` + Lattice `TurtleMove`; still **15** |
| Smoke | `hc: Upstream NearLatticeLite` (expect 15) |
| Shell | `nearlatticelite` |

Freeze catalog unchanged.

## Milestone 124 — Lattice DrawIt in NearLattice

| Item | Path |
|--|--|
| Gap | NearLattice `DrawIt` was a single `GrPlot`; Lattice MemCopy + HUD + head lines |
| Upstream need | `Lattice.ZC` `DrawIt` — MemCopy, GrPrint, TurtleMove preview, 3× GrLine head |
| Bring-up | `NearLatticeLite.ZC` — DrawItLite-shaped `DrawIt` + `TURTLE_SIZE`; still **15** |
| Smoke | `hc: Upstream NearLatticeLite` (expect 15) |
| Shell | `nearlatticelite` |

Freeze catalog unchanged.


## Milestone 125 — RedSea Lattice.ZC alias (DiskLat body)

| Item | Path |
|--|--|
| Gap | Compose lived as `DiskLat.ZC` only; Lattice demos are named `Lattice.ZC` |
| Upstream need | runzc-shaped load of `Lattice.ZC` (ephemeral; not freeze catalog) |
| Bring-up | Smoke put/load/run `Lattice.ZC` = DiskLat body → **15**; shell `lattice` |
| Smoke | `hc: Upstream Lattice.ZC` (expect 15) |
| Shell | `lattice` |

Freeze catalog unchanged.

## Milestone 126 — ANGLES=35 BracePi in NearLattice

| Item | Path |
|--|--|
| Gap | NearLattice used `ANGLES 3` zeros; Lattice uses 35 π-brace angles |
| Upstream need | `Lattice.ZC` — `F64 angles[35] = { -2*π/1, …, 0, …, 2*π/1 }` |
| Bring-up | `NearLatticeLite.ZC` — BracePi table; `TurtleInit` mid idx 17; still **15** |
| Smoke | `hc: Upstream NearLatticeLite` (expect 15) |
| Shell | `nearlatticelite` |

Freeze catalog unchanged.

## Milestone 127 — Lattice TurtlePlot half formula in NearLattice

| Item | Path |
|--|--|
| Gap | NearLattice mid/edge both used `w/2`; Lattice mid is `w/2-1`, edge `w/2` |
| Upstream need | `Lattice.ZC` `TurtlePlot` — inset mid GrLine3, full-radius edge GrPlot3 |
| Bring-up | `NearLatticeLite.ZC` — Lattice half formula; still **15** |
| Smoke | `hc: Upstream NearLatticeLite` (expect 15) |
| Shell | `nearlatticelite` |

Freeze catalog unchanged.


## Milestone 128 — GrPlot3 depth_buf z-test + DepthPlotLite

| Item | Path |
|--|--|
| Gap | `GrPlot3` ignored z; Lattice peeks need closer-z wins via `depth_buf` |
| Upstream need | `GrPrimatives.ZC` — plot only when z is closer than depth cell |
| Bring-up | Sparse 2048-slot z-map (tiny mcmodel); `DepthPlotLite.ZC` → **15** |
| Smoke | `hc: Upstream DepthPlotLite` (expect 15) |
| Shell | `depthplotlite` |

Freeze catalog unchanged.

## Milestone 129 — inline Lattice MessageGet mask in NearLattice

| Item | Path |
|--|--|
| Gap | NearLattice used a `mask` local; Lattice inlines the `1<<MESSAGE_*` or-expr |
| Upstream need | `Lattice.ZC` — `MessageGet(&arg1,&arg2, 1<<KEY\|1<<MS_…)` |
| Bring-up | `NearLatticeLite.ZC` — inline mask; still **15** |
| Smoke | `hc: Upstream NearLatticeLite` (expect 15) |
| Shell | `nearlatticelite` |

Freeze catalog unchanged.

## Milestone 130 — Lattice setup order (init outside try)

| Item | Path |
|--|--|
| Gap | NearLattice ran `TurtleInit`/`SetMenu`/`win_inhibit`/`draw_it` inside `try` |
| Upstream need | `Lattice.ZC` — setup before `try`; `try` wraps only the event loop |
| Bring-up | `NearLatticeLite.ZC` — Lattice setup order; still **15** |
| Smoke | `hc: Upstream NearLatticeLite` (expect 15) |
| Shell | `nearlatticelite` |

Freeze catalog unchanged.


## Milestone 131 — GrLine3 depth_buf z-test + DepthLineLite

| Item | Path |
|--|--|
| Gap | `GrLine3` ignored z; Lattice mid body uses depth-aware `GrLine3` |
| Upstream need | `GrPrimatives` line via `GrPlot3` / depth along the segment |
| Bring-up | Bresenham + z interp + sparse map; `DepthLineLite.ZC` → **15** |
| Smoke | `hc: Upstream DepthLineLite` (expect 15) |
| Shell | `depthlinelite` |

Freeze catalog unchanged.

## Milestone 132 — Lattice TurtlePlot half formula across *Lite smokes

| Item | Path |
|--|--|
| Gap | Many *Lite smokes still used one `half=w/2` for mid and edge |
| Upstream need | `Lattice.ZC` `TurtlePlot` — mid `w/2-1`, edge `w/2` |
| Bring-up | LatticeLite/Live/Frame/Play/Demo/DrawIt/Tick/End/MoveStack/StepCount |
| Smoke | existing *Lite expects unchanged (**15**) |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 133 — STR_LEN SetMenu buffers

| Item | Path |
|--|--|
| Gap | NearLattice/DiskLat/Live/Play/SetMenu still used `U8 buf[32]` |
| Upstream need | `Lattice.ZC` SetMenu — `U8 buf[STR_LEN]` |
| Bring-up | NearLatticeLite + DiskLat + LiveLite + PlayLite + SetMenuLite |
| Smoke | existing expects unchanged (**15**) |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 134 — DiskLat full SetMenu + setup order

| Item | Path |
|--|--|
| Gap | DiskLat still used slim SetMenu + init inside `try` |
| Upstream need | `Lattice.ZC` — sibling-clear SetMenu; init outside `try` |
| Bring-up | `DiskLat.ZC` (+ RedSea `Lattice.ZC` alias); still **15** |
| Smoke | `hc: Upstream DiskLat` / `Lattice.ZC` (expect 15) |
| Shell | `disklat` / `lattice` |

Freeze catalog unchanged.

## Milestone 135 — STR_LEN across remaining SetMenu *Lites

| Item | Path |
|--|--|
| Gap | Wrap/MenuLoop/Restart/Layer/Ends/Checked/MenuFull still used `buf[32]` |
| Upstream need | `Lattice.ZC` SetMenu — `U8 buf[STR_LEN]` |
| Bring-up | those *Lite SetMenu buffers → `STR_LEN` |
| Smoke | existing expects unchanged (**15**) |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 136 — DiskLat depth_buf + dual PopUpColor

| Item | Path |
|--|--|
| Gap | DiskLat skipped `DCDepthBufAlloc` and Lattice `'c'` dual color |
| Upstream need | `Lattice.ZC` — depth_buf for GrPlot3; `'c'` → mid+edge PopUpColor |
| Bring-up | `DiskLat.ZC` (+ `Lattice.ZC` alias); scripted `'c'`; still **15** |
| Smoke | `hc: Upstream DiskLat` / `Lattice.ZC` (expect 15) |
| Shell | `disklat` / `lattice` |

Freeze catalog unchanged.

## Milestone 137 — WrapLattice full SetMenu + setup order

| Item | Path |
|--|--|
| Gap | WrapLatticeLite still used slim MenuPush/SetMenu + init inside `try` |
| Upstream need | `Lattice.ZC` — full menu + sibling-clear SetMenu; init outside `try` |
| Bring-up | `WrapLatticeLite.ZC`; still **15** |
| Smoke | `hc: Upstream WrapLatticeLite` (expect 15) |
| Shell | `wraplatticelite` |

Freeze catalog unchanged.

## Milestone 138 — Live/Play full SetMenu sibling-clear

| Item | Path |
|--|--|
| Gap | LiveLite/PlayLite SetMenu still only checked the current layer |
| Upstream need | `Lattice.ZC` SetMenu — clear sibling Layer0–9 checked flags |
| Bring-up | `LiveLite.ZC` + `PlayLite.ZC` full for-loop SetMenu; still **15** |
| Smoke | existing Live/Play expects unchanged (**15**) |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 139 — Frame/Restart/Ends/Layer full SetMenu

| Item | Path |
|--|--|
| Gap | Frame/Restart/Ends/Layer still used slim single-layer SetMenu |
| Upstream need | `Lattice.ZC` SetMenu — clear sibling Layer0–9 checked flags |
| Bring-up | those *Lites → full for-loop SetMenu; expects unchanged (**15**) |
| Smoke | existing expects unchanged |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 140 — full Lattice MenuPush on compose *Lites

| Item | Path |
|--|--|
| Gap | Live/Play/Frame/Demo/LatticeLite still used slim File-only MenuPush |
| Upstream need | `Lattice.ZC` File/Play/Settings MenuPush |
| Bring-up | those *Lites → MenuBig-sized MenuPush; expects unchanged (**15**) |
| Smoke | existing expects unchanged |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 141 — fix MenuPush Restart `\n` escape in *Lites

| Item | Path |
|--|--|
| Gap | M140 MenuPush wrote a real newline inside `Restart(,'…')` strings |
| Upstream need | `Lattice.ZC` — `Restart(,'\n')` as backslash-n in source |
| Bring-up | Live/Play/Frame/Demo/LatticeLite Restart strings fixed |
| Smoke | existing expects unchanged (**15**) |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 142 — Lattice Win/Doc setup on Live/Play/Frame

| Item | Path |
|--|--|
| Gap | Live/Play/Frame skipped `AutoComplete`/`Win*`/`Doc*`/`win_inhibit` |
| Upstream need | `Lattice.ZC` setup before the event loop |
| Bring-up | those *Lites — Lattice setup sequence; still **15** |
| Smoke | existing expects unchanged |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 143 — DiskLat load 8K + Narrower `-`

| Item | Path |
|--|--|
| Gap | RedSea DiskLat/Lattice load capped at 4K; DiskLat lacked `'/'` — lacked `'-'` |
| Upstream need | headroom toward NearLattice; Lattice Wider/Narrower `'+'`/`'-'` |
| Bring-up | load buf **8192**; DiskLat `case '-'`; still **15** |
| Smoke | `hc: Upstream DiskLat` / `Lattice.ZC` (expect 15) |
| Shell | `disklat` / `lattice` |

Freeze catalog unchanged.

## Milestone 144 — DiskLat TurtleMove + SPACE (slim angles)

| Item | Path |
|--|--|
| Gap | DiskLat MenuPush listed Step but had no TurtleMove / CH_SPACE |
| Upstream need | `Lattice.ZC` SPACE → TurtleMove (16× formula + plot) |
| Bring-up | DiskLat TurtlePlot/MicroMove/Move; SPACE before `
`; ANGLES=3 (RedSea free-clus cap) |
| Smoke | `hc: Upstream DiskLat` / `Lattice.ZC` (expect 15) |
| Shell | `disklat` / `lattice` |

Freeze catalog unchanged.


## Milestone 145 — RedSea 128-clus fmt + DiskLat BracePi TurtleEnd

| Item | Path |
|--|--|
| Gap | 16-clus smoke volume + `nclus≤14` blocked BracePi DiskLat put |
| Upstream need | room for NearLattice-sized RedSea Lattice.ZC |
| Bring-up | RAM disk 128; `rs_fmt_host(128)` before DiskLat/Lattice put; nclus≤120; BracePi+End |
| Smoke | `hc: Upstream DiskLat` / `Lattice.ZC` (expect 15) |
| Shell | `disklat` / `lattice` |

Freeze catalog unchanged.

## Milestone 146 — DiskLat SC_CURSOR speed/idx

| Item | Path |
|--|--|
| Gap | DiskLat MenuPush listed cursors but ignored `arg1==0` scan codes |
| Upstream need | `Lattice.ZC` LEFT/RIGHT idx; UP/DOWN speed ±TURTLE_SPEED_STEP |
| Bring-up | DiskLat cursor switch + scripted CURSOR_UP before SPACE; still **15** |
| Smoke | `hc: Upstream DiskLat` / `Lattice.ZC` (expect 15) |
| Shell | `disklat` / `lattice` |

Freeze catalog unchanged.

## Milestone 147 — DiskLat DrawIt + aim/MS_* 

| Item | Path |
|--|--|
| Gap | DiskLat skipped DrawIt HUD and mouse place/aim |
| Upstream need | `Lattice.ZC` `Fs->draw_it` + MS_L/R place/aim |
| Bring-up | DiskLat DrawIt MemCopy HUD; MS_* + Refresh; still **15** |
| Smoke | `hc: Upstream DiskLat` / `Lattice.ZC` (expect 15) |
| Shell | `disklat` / `lattice` |

Freeze catalog unchanged.

## Milestone 148 — header-aware PopUpColor (Mid/Edge)

| Item | Path |
|--|--|
| Gap | `PopUpColor` always returned YELLOW; Lattice Mid vs Edge headers ignored |
| Upstream need | distinct mid/edge picks from PopUpColor headers |
| Bring-up | Mid→YELLOW, Edge→BLACK (header or pair order; reset per `hc_run_src`) |
| Smoke | MidLite / BothColorLite / NearLatticeLite (expect 15) |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 149 — DiskLat NearLattice script + expect parity

| Item | Path |
|--|--|
| Gap | DiskLat scripted `\n` wipe; expect bits lagged NearLattice |
| Upstream need | same key path + checks as NearLatticeLite compose |
| Bring-up | DiskLat NearLattice MsgQue + expect; DCFill/DCDel teardown; **15** |
| Smoke | `hc: Upstream DiskLat` / `Lattice.ZC` (expect 15) |
| Shell | `disklat` / `lattice` |

Freeze catalog unchanged.

## Milestone 150 — UART RX RSR clear + PopUpColor pair reset

| Item | Path |
|--|--|
| Gap | PL011 RX left RSR sticky; PopUpColor pair index leaked across `hc_run_src` |
| Upstream need | clean UART RX for Pi USB-TTL; stable Mid/Edge PopUpColor pairs |
| Bring-up | `uart_getc_nb` clears RSR/ECR; `g_hc_popup_i` reset per run (pair Mid/Edge) |
| Smoke | Mid/Both/NearLattice/DiskLat (**15**); check-serial green |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 151 — UTM shell FB clear after boot smokes

| Item | Path |
|--|--|
| Gap | UTM FB kept bars/lines from jit_smoke under the interactive `>` |
| Upstream need | clean interactive console surface on virt FB |
| Bring-up | `shell_run` clears FB + resets text cursor before banner |
| Smoke | check-serial / run-pci unchanged; UTM eyes-on clean prompt |
| Shell | unchanged commands |

Freeze catalog unchanged.

## Milestone 152 — UTM-friendly shell banner

| Item | Path |
|--|--|
| Gap | Shell printed one mega `cmds:` line that wrapped into unreadable FB soup |
| Upstream need | readable interactive console on 800×600 UTM FB |
| Bring-up | short startup hints; `help` leads with UTM freeze + Lattice cmds |
| Smoke | check-serial / run-pci unchanged |
| Shell | banner + `help` text only |

Freeze catalog unchanged.

## Milestone 153 — Lattice FB teardown + IR mem for DrawIt HUD

| Item | Path |
|--|--|
| Gap | UTM post-`disklat`: HUD soup + `Width:nan`; plot left under `>`; stale `Fs->draw_it` |
| Upstream need | DCFill clears surface; DrawIt Refresh HUD stable; no cross-demo draw_it |
| Bring-up | `HC_IR_MEM_WORDS` 128→256 (MenuPush strings+angles crushed DrawIt frames); `DCFill`→`fb_clear`; GrPrint wipes HUD band; `g_hc_fs_draw_it=0` around `hc_run_src` |
| Smoke | check-serial / run-pci; `disklat`/`nearlatticelite` **15**; UTM eyes-on clean FB after demo |
| Shell | unchanged commands |

Freeze catalog unchanged.

## Milestone 154 — Lattice MESSAGE_NULL idle (Sleep+Refresh)

| Item | Path |
|--|--|
| Gap | NearLattice/DiskLat treated empty MessageGet as exit (`goto lt_done`) |
| Upstream need | `Lattice.ZC` idle — NULL → keep looping + Refresh until ESC |
| Bring-up | `MESSAGE_NULL` → `Sleep(1)` then Refresh; scripted `CH_ESC` still ends smoke (**15**) |
| Smoke | NearLatticeLite / DiskLat / Lattice.ZC expect **15** |
| Shell | `nearlatticelite` / `disklat` (ESC still auto-pushed for smoke) |

Freeze catalog unchanged.

## Milestone 155 — latticeplay interactive shell (no scripted MsgQue)

| Item | Path |
|--|--|
| Gap | `disklat` always auto-drives MsgQue + ESC — no live UTM Lattice play |
| Upstream need | Lattice.ZC-shaped loop driven by virtio kbd/tablet until ESC |
| Bring-up | `hc_lattice_play_src` strips `MsgQuePush`; shell `latticeplay`; smoke injects ESC-only |
| Smoke | `hc: Upstream LatticePlay` (ESC-only terminate) |
| Shell | `latticeplay` (ESC to exit) |

Freeze catalog unchanged.

## Milestone 156 — MESSAGE_NULL idle in remaining compose *Lites

| Item | Path |
|--|--|
| Gap | Demo/Frame/Live/Play/WrapLattice *Lites still exited on empty MessageGet |
| Upstream need | same Lattice idle as NearLattice/DiskLat (M154) |
| Bring-up | `Sleep(1)` + continue; Demo/Play also Refresh each turn; scripted CH_ESC ends smoke |
| Smoke | DemoLite / FrameLite / LiveLite / PlayLite / WrapLatticeLite expect **15** |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 157 — shell FB restore after Lattice demos

| Item | Path |
|--|--|
| Gap | `DCFill` leaves pure black; UTM `>` after `disklat`/`latticeplay` sat on black |
| Upstream need | handoff back to interactive console surface |
| Bring-up | `shell_fb_ready` → same dark clear as `shell_run` after nearlatticelite/disklat/lattice/latticeplay |
| Smoke | check-serial unchanged |
| Shell | Lattice cmds restore FB before `ok` |

Freeze catalog unchanged.

## Milestone 158 — Enter → Lattice Restart (`\n`)

| Item | Path |
|--|--|
| Gap | virtio Enter emitted `\r`; Lattice `Restart(,'\n')` / `case '\n'` never matched on UTM |
| Upstream need | Enter restarts turtle like Lattice.ZC |
| Bring-up | virtio KEY_ENTER → `\n`; UART MessageGet CR→LF; DiskLat/NearLattice/RestartLite accept `\r`/`\n` |
| Smoke | RestartLite / DiskLat / NearLattice **15**; check-serial green |
| Shell | `latticeplay` Enter = Restart |

Freeze catalog unchanged.

## Milestone 159 — latticeplay on-screen controls hint

| Item | Path |
|--|--|
| Gap | Live `latticeplay` gave no FB reminder of Esc/Enter/Space |
| Upstream need | readable bring-up cue on virt FB before first input |
| Bring-up | `hc_lattice_play_src` injects `GrPrint` hint after `Cls` when not smoke |
| Smoke | LatticePlay ESC-only path unchanged |
| Shell | `latticeplay` shows hint under HUD line |

Freeze catalog unchanged.

## Milestone 160 — PopUpColor FB swatch + glob headroom

| Item | Path |
|--|--|
| Gap | latticeplay `'c'` picked Mid/Edge with no FB feedback; `HC_IR_MAX_GLOBS` tight at 48 |
| Upstream need | visible color pick cue; room for more file-scope words |
| Bring-up | `PopUpColor` draws Mid/Edge swatches at FB bottom; `HC_IR_MAX_GLOBS` 48→64 |
| Smoke | Mid/Both/NearLattice/DiskLat (**15**); check-serial green |
| Shell | `latticeplay` `'c'` shows swatches |

Freeze catalog unchanged.

## Milestone 161 — latticeplay PopUpColor pair cycle

| Item | Path |
|--|--|
| Gap | every `'c'` always Mid=YELLOW Edge=BLACK — no live color variety |
| Upstream need | Lattice color picks can change across presses |
| Bring-up | `g_hc_popup_live` via `hc_run_src_ex`; cycle Mid/Edge pairs (pair0 still Y/B); smokes stay locked |
| Smoke | Mid/Both/NearLattice/DiskLat (**15**) |
| Shell | `latticeplay` `'c'` cycles colors; hint mentions `c=color` |

Freeze catalog unchanged.

## Milestone 162 — Restart restores latticeplay controls hint

| Item | Path |
|--|--|
| Gap | Enter/`Restart` `DCFill` wiped the Esc/Enter/Space hint |
| Upstream need | controls cue survives Lattice Restart |
| Bring-up | DiskLat/NearLattice/Wrap Restart `GrPrint` hint after TurtleInit |
| Smoke | DiskLat / NearLattice / WrapLatticeLite **15** |
| Shell | `latticeplay` Enter keeps the hint visible |

Freeze catalog unchanged.

## Milestone 162 — Restart restores latticeplay controls hint

| Item | Path |
|--|--|
| Gap | Enter/`Restart` `DCFill` wiped the Esc/Enter/Space hint |
| Upstream need | controls cue survives Lattice Restart |
| Bring-up | DiskLat/NearLattice/Wrap Restart `GrPrint` hint after TurtleInit |
| Smoke | DiskLat / NearLattice / WrapLatticeLite **15** |
| Shell | `latticeplay` Enter keeps the hint visible |

Freeze catalog unchanged.

## Milestone 163 — DiskLat source load buffer 8K→16K

| Item | Path |
|--|--|
| Gap | M162 grew `DiskLat.ZC` past 8192 bytes → RedSea load FAIL |
| Upstream need | room for Lattice compose source on disk path |
| Bring-up | `g_hc_zc_src[16384]` for disklat/lattice/latticeplay/smoke loads |
| Smoke | DiskLat / Lattice.ZC / LatticePlay green again |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 164 — latticeplay Restart syncs PopUpColor + swatches

| Item | Path |
|--|--|
| Gap | Enter/`DCFill` left color cycle mid-palette while TurtleInit reset to YELLOW/BLACK |
| Upstream need | Restart restores default Mid/Edge visually and in the picker |
| Bring-up | live `DCFill` resets `g_hc_popup_i` and paints Mid/Edge swatches; shared `hc_popup_paint_swatch` |
| Smoke | Mid/Both/DiskLat unchanged (popup_live=0) |
| Shell | `latticeplay` Enter resets colors + swatches |

Freeze catalog unchanged.

## Milestone 165 — latticeplay Mid/Edge swatches from entry + Refresh

| Item | Path |
|--|--|
| Gap | Swatches only appeared after first `'c'` or Enter; idle `Refresh`/`DrawIt` could cover them |
| Upstream need | Mid/Edge visible for the whole live session |
| Bring-up | track Mid/Edge; live `Cls` + `Refresh` re-paint via `hc_popup_paint_live` |
| Smoke | Mid/Both/DiskLat/LatticePlay unchanged (popup_live=0 on smokes) |
| Shell | `latticeplay` shows YELLOW/BLACK from start; colors survive idle frames |

Freeze catalog unchanged.

## Milestone 166 — latticeplay controls hint on Refresh + width cue

| Item | Path |
|--|--|
| Gap | Esc/Enter/Space hint could vanish under idle `DrawIt`; width keys undocumented on FB |
| Upstream need | controls cue stays readable; `+`/`-` width called out |
| Bring-up | live `hc_popup_paint_live` re-prints hint; DiskLat/Near/Wrap/`hc_lattice_play_src` add `+/-=w` |
| Smoke | DiskLat / NearLattice / WrapLatticeLite / LatticePlay still **15** |
| Shell | `latticeplay` hint survives Refresh; shows `+/-=w` |

Freeze catalog unchanged.

## Milestone 167 — DiskLat DrawIt HUD `di` (Lattice parity)

| Item | Path |
|--|--|
| Gap | DiskLat HUD omitted `di` while NearLattice/Lattice print `angles[idx]` degrees |
| Upstream need | `Lattice.ZC` `GrPrint(… th … di … Width …)` |
| Bring-up | DiskLat DrawIt → five-field HUD with `angles[tt.idx] * 180 / pi` |
| Smoke | DiskLat / Lattice.ZC / LatticePlay still **15** |
| Shell | `latticeplay` HUD shows Layer/Speed/th/di/Width |

Freeze catalog unchanged.

## Milestone 168 — latticeplay place/aim/ends hint line

| Item | Path |
|--|--|
| Gap | tablet place/aim and `'e'` ends were on-menu only; UTM eyes-on had no FB cue |
| Upstream need | discoverable MS_L / MS_R / Ends controls |
| Bring-up | second hint at y=24 in live paint + DiskLat/Near/Wrap/`hc_lattice_play_src` |
| Smoke | DiskLat / NearLattice / WrapLatticeLite / LatticePlay still **15** |
| Shell | `latticeplay` shows L-click/R-drag/`e=ends` under the Esc line |

Freeze catalog unchanged.

## Milestone 169 — latticeplay arrows/layer hint line

| Item | Path |
|--|--|
| Gap | cursor di/speed and digit layers lived only in MenuPush |
| Upstream need | discoverable SC_CURSOR_* / `'0'...'9'` on FB |
| Bring-up | third hint at y=32: `arrows=di/speed 0-9=layer` |
| Smoke | DiskLat / NearLattice / WrapLatticeLite / LatticePlay still **15** |
| Shell | `latticeplay` shows arrows/layer cue under place/aim line |

Freeze catalog unchanged.

## Milestone 170 — DrawIt without idle TurtleMove flood

| Item | Path |
|--|--|
| Gap | DiskLat/NearLattice `DrawIt` called `TurtleMove` every `Refresh`; idle `MESSAGE_NULL` flooded FB with RED preview strokes (no WinMgr back-buffer) |
| Upstream need | usable live Lattice view; Space still steps via event path |
| Bring-up | compose DrawIt → HUD + LTRED head only; `DrawItLite`/`TickLite` keep TurtleMove for peeks |
| Smoke | DiskLat / NearLattice / Lattice.ZC / LatticePlay still **15** |
| Shell | `latticeplay` idle no longer paints accumulating red paths |

Freeze catalog unchanged.

## Milestone 171 — DrawIt erases prior LTRED head

| Item | Path |
|--|--|
| Gap | place/aim moved `tt` while prior head GrLines stayed on FB |
| Upstream need | single live head marker |
| Bring-up | `head_x`/`head_y`/`head_on` + `FillRect` erase before redraw; clear on Cls/Restart |
| Smoke | DiskLat / NearLattice / Lattice.ZC / LatticePlay still **15** |
| Shell | `latticeplay` L-click / R-drag leave one head, not a trail of ghosts |

Freeze catalog unchanged.

## Milestone 172 — head erase via BLACK GrLines

| Item | Path |
|--|--|
| Gap | M171 `FillRect` head erase punched holes in nearby Space strokes |
| Upstream need | move head without damaging the lattice path |
| Bring-up | `HeadMark` helper; erase with BLACK GrLines at saved `head_th`; DiskLat/NearLattice |
| Smoke | DiskLat / NearLattice / Lattice.ZC / LatticePlay still **15** |
| Shell | `latticeplay` place/aim/step keeps strokes under the moving head |

Freeze catalog unchanged.

## Milestone 173 — skip idle head redraw when pose unchanged

| Item | Path |
|--|--|
| Gap | every idle `Refresh` erased+redrew the head even when `tt` pose was still |
| Upstream need | quieter live view; less BLACK thrash on the path |
| Bring-up | compare `ToI64` x/y/`th*1000`; only `HeadMark` when pose changes; HUD always updates |
| Smoke | DiskLat / NearLattice / Lattice.ZC / LatticePlay still **15** |
| Shell | `latticeplay` idle updates HUD without re-stroking the head |

Freeze catalog unchanged.

## Milestone 174 — DCFill resets depth buffer

| Item | Path |
|--|--|
| Gap | Enter/`DCFill` cleared pixels but left sparse `depth_buf` cells from prior plots |
| Upstream need | Restart draws new strokes without stale z occlusion |
| Bring-up | `hc_builtin_dcfill` → `DCDepthBufReset` when depth is active |
| Smoke | DiskLat / Depth* / Lattice.ZC / LatticePlay still **15** |
| Shell | `latticeplay` Enter then Space paints cleanly over the cleared surface |

Freeze catalog unchanged.

## Milestone 175 — Cls resets depth buffer

| Item | Path |
|--|--|
| Gap | latticeplay entry `Cls` cleared FB but not the sparse depth map |
| Upstream need | first strokes after entry match Restart cleanliness |
| Bring-up | shared `hc_depth_map_clear`; `Cls` + `DCFill` both clear when depth is on |
| Smoke | DiskLat / Depth* / Lattice.ZC / LatticePlay still **15** |
| Shell | `latticeplay` first Space after entry not occluded by leftover z |

Freeze catalog unchanged.

## Milestone 176 — Space skips BLACK erase of old head

| Item | Path |
|--|--|
| Gap | after `TurtleMove`, DrawIt BLACK-erased the prior head and notched the new stroke |
| Upstream need | step path stays intact under the moving marker |
| Bring-up | `head_on = FALSE` after `CH_SPACE`; next DrawIt only paints the new LTRED head |
| Smoke | DiskLat / NearLattice / Lattice.ZC / LatticePlay still **15** |
| Shell | `latticeplay` Space no longer leaves a black bite at the step origin |

Freeze catalog unchanged.

## Milestone 177 — stock `Lattice()` entry name

| Item | Path |
|--|--|
| Gap | compose file only exposed `DiskLat()` while upstream ends with `Lattice;` |
| Upstream need | `Demo/Graphics/Lattice.ZC` entry symbol |
| Bring-up | `I64 Lattice(){return DiskLat();}` + `return Lattice();` (I64 expect retained) |
| Smoke | DiskLat / Lattice.ZC / LatticePlay still **15** |
| Shell | `lattice` / `disklat` / `latticeplay` unchanged |

Freeze catalog unchanged.

## Milestone 178 — help lists latticeplay controls

| Item | Path |
|--|--|
| Gap | `help` named `latticeplay` but not the Esc/tablet/key map for eyes-on |
| Upstream need | discoverable live Lattice controls from the shell |
| Bring-up | `help` one-liner for Esc/Enter/Space/c/+/-/e, L-click, R-drag, arrows, 0-9 |
| Smoke | unchanged (`help` text only) |
| Shell | `help` then `latticeplay` for UTM acceptance |

Freeze catalog unchanged.

## Milestone 179 — latticeplay prints controls on entry

| Item | Path |
|--|--|
| Gap | controls lived in `help` / on-FB hints; serial window saw only `latticeplay ok` after exit |
| Upstream need | cue before the live loop for UTM Terminal 1 |
| Bring-up | `con_puts` controls line immediately before `hc_run_src_ex(..., popup_live=1)` |
| Smoke | LatticePlay smoke unchanged (no shell cmd path) |
| Shell | `latticeplay` shows controls, then runs until Esc |

Freeze catalog unchanged.

## Milestone 180 — Lattice idle ~60 Hz Sleep

| Item | Path |
|--|--|
| Gap | `MESSAGE_NULL` used `Sleep(1)` → ~1000 Refresh/s on live `latticeplay` |
| Upstream need | usable live session without burning the UTM host |
| Bring-up | DiskLat/Near/Wrap/Demo/Play/Frame/Live → `Sleep(16)`; Sleep wakes early on pending virtio/UART/tablet (M181) |
| Smoke | compose *Lites / DiskLat / LatticePlay still **15** |
| Shell | `latticeplay` idle quieter; aim/step still event-driven |

Freeze catalog unchanged.

## Milestone 182 — stock Lattice.ZC smoke (StockLat)

| Item | Path |
|--|--|
| Gap | DiskLat-shaped compose never proved near-verbatim stock `Demo/Graphics/Lattice.ZC` |
| Upstream need | TempleOS `0xE3`/`0xE9` glyphs, `U0` turtle helpers, setup stubs, stock `DrawIt`/`Lattice` body |
| Bring-up | `upstream/StockLat.ZC` from YDE/ZealOS Lattice; `LatticeSmoke` + MsgQue SPACE/ESC → **15** |
| Smoke | `hc: Upstream StockLat` (expect 15) |
| Shell | interactive live path remains `latticeplay` (DiskLat); StockLat is compile/script proof only |

Freeze catalog unchanged.

## Milestone 183 — DiskLat stock θ / dθ_idx names

| Item | Path |
|--|--|
| Gap | DiskLat/`latticeplay` still used ASCII `th` / `idx` while stock Lattice uses TempleOS `θ` / `dθ_idx` |
| Upstream need | same Turtle field names as `Demo/Graphics/Lattice.ZC` |
| Bring-up | DiskLat members + HUD labels → `θ` / `dθ_idx` (0xE9); HeadMark/TurtleEnd params stay local `th` |
| Smoke | DiskLat / Lattice.ZC / LatticePlay / StockLat still **15** |
| Shell | `latticeplay` HUD shows stock-shaped θ/dθ labels |

Freeze catalog unchanged.

## Milestone 184 — FB π/θ glyphs for Lattice HUD

| Item | Path |
|--|--|
| Gap | ASCII-only `fb_draw_char` mapped TempleOS `0xE3`/`0xE9` to `?`, so DiskLat HUD after M183 showed `?:` / `d?:` |
| Upstream need | readable stock-shaped Layer/Speed/θ/dθ HUD on UTM FB |
| Bring-up | `fb_draw_char` draws 8×8 π/θ bitmaps for `0xE3`/`0xE9` |
| Smoke | unchanged (DrawIt HUD is eyes-on) |
| Shell | `latticeplay` HUD labels render as π/θ-ish glyphs |

Freeze catalog unchanged.

## Milestone 185 — NearLatticeLite stock θ / dθ_idx

| Item | Path |
|--|--|
| Gap | NearLatticeLite still used ASCII `th` / `idx` after DiskLat M183 |
| Upstream need | compose *Lite field names match stock Lattice / DiskLat |
| Bring-up | NearLatticeLite members + HUD → `θ` / `dθ_idx` |
| Smoke | NearLatticeLite still **15** |
| Shell | `nearlatticelite` HUD matches `latticeplay` labels |

Freeze catalog unchanged.

## Milestone 186 — Demo/Play/Frame/Live stock θ / dθ_idx

| Item | Path |
|--|--|
| Gap | idle compose *Lites still used ASCII `th` / `idx` after DiskLat/NearLattice |
| Upstream need | same Turtle field names across Lattice compose ladder |
| Bring-up | DemoLite, PlayLite, FrameLite, LiveLite → `θ` / `dθ_idx` + HUD |
| Smoke | those *Lites still **15** |
| Shell | demolite/playlite/… HUD matches stock labels |

Freeze catalog unchanged.

## Milestone 187 — remaining turtle *Lites stock θ / dθ_idx

| Item | Path |
|--|--|
| Gap | DrawIt/End/Event/Micro/Move*/AimMove/StepCount/Tick/Lattice/IdxAll/Inc/Init/Right still used ASCII `th` / `idx` |
| Upstream need | TempleOS `θ` / `dθ_idx` across the Lattice compose ladder |
| Bring-up | those *Lites renamed; RightLite local angle uses `θ`; CursorLite keeps local `idx` |
| Smoke | affected *Lites still **15** |
| Shell | unchanged |

Freeze catalog unchanged.

## Milestone 188 — TurtleLite TempleOS θ

| Item | Path |
|--|--|
| Gap | TurtleLite still used ASCII `theta` while the rest of the ladder uses TempleOS `0xE9` |
| Upstream need | `Turtle.θ` from the earliest MicroMove smoke |
| Bring-up | TurtleLite `theta` → `θ` |
| Smoke | TurtleLite still **15** |
| Shell | `turtlelite` unchanged |

Freeze catalog unchanged.

## Milestone 189 — MessageGet yields before MESSAGE_NULL

| Item | Path |
|--|--|
| Gap | `latticeplay` idle slept in HolyC *and* could busy-spin on stock-shaped loops |
| Upstream need | WinMgr-shaped wait (~60 Hz) inside live `MessageGet` |
| Bring-up | `g_hc_popup_live` → interruptible `Sleep(16)` then NULL; DiskLat/Near/Demo/Play/Frame/Live/Wrap NULL cases Refresh only |
| Smoke | empty-queue NULL still immediate (no live flag); **15** |
| Shell | `latticeplay` idle paced in `MessageGet`; stock interactive needs live flag too |

Freeze catalog unchanged.

## Acceptance

- QEMU `make check-serial` must print `hc IR OK` (fails make on `hc IR FAIL`).
- QEMU `make run-serial` (interactive shell) after green check-serial.
- QEMU `make run-pci` boots with virtio-blk-pci (same ESP).
- UTM (acceptance) boots the same virt HW config (`scripts/install-utm.sh` / Makefile `QEMU_VIRT`).
