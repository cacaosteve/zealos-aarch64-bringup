# UTM acceptance (interactive) — **frozen virt regression bar (M33–M39)**

Automated QEMU/HVF covers boot + compiler smokes. **You** verify screen / keyboard / mouse
and the disk path below.

**Role:** UTM (`ZealosAarch64Hello`) is the frozen **regression harness** for the virt
track. Do not extend the small-demo ladder further; next large efforts are subsystems
or Pi bring-up. Re-run this list after risky changes; QEMU first, UTM when it matters.

## VM

| Field | Value |
|--|--|
| Name in UTM (only one) | **ZealosAarch64Hello** |
| Refresh | `make utm` |
| Start | UTM → ZealosAarch64Hello → Play |
| Serial | `utmctl attach ZealosAarch64Hello` (if supported) |

There must be **one** entry. If UTM shows two or Start is greyed out: quit UTM, run `make utm`, reopen. RedSea lives past the ESP; `make utm` preserves that region.

```text
ZealOS aarch64 shell (HolyC-IR exprs)
>
```

## Checklist — interactive smoke

1. Boot until `>` stays on screen.
2. `help`, Backspace typo fix.
3. Spot-check `bars` / `paint` if touching FB/input.
4. Compiler:

```text
hc return sizeof(U32);
hc U32 x=0xFFFFFFFF;x=x+1;return x;
hc I64 f(I64 x){return x+1;}I64 a=5;return f(2)+a;
```

Expect **4**, **0**, **8**.

## Checklist — RedSea disk story (freeze)

```text
vblk
rspersist
rscatalog
rsdir
runzc
runzc UseAdd.ZC
runzc Notes.ZC
runzc MemSort.ZC
```

| Cmd | Expect |
|--|--|
| `vblk` | Disk R/W OK |
| `rspersist` | Size **`0x4`** |
| `rscatalog` / `rsdir` | AddLib, UseAdd, DocLib, Notes, MemSort listed |
| `runzc` | Hi.ZC → **`0x2a`** |
| `runzc UseAdd.ZC` | **`0x2a`** (`#include` AddLib) |
| `runzc Notes.ZC` | **`0xe`** (14) + note list on FB |
| `runzc MemSort.ZC` | **`0x10`** |

Restart VM; repeat `rspersist`, `rsdir`, `runzc Notes.ZC` (persist-safe seeds).

**Status:** M33–M39 UTM freeze bar **accepted** (including `Notes.ZC` from PCI RedSea + `#include`). Pi remains a separate track ([PI4.md](PI4.md)).

**Re-pass 2026-09-23:** `make utm` refreshed; QEMU `run-pci` scripted checklist green (vblk, rspersist `0x4`, catalog/rsdir, `runzc` UseAdd/`0x2a`, MemSort/`0x10`, Notes on disk, compiler 4/0/8); second boot persist OK. UTM `ZealosAarch64Hello` started with synced disk — interactive FB/`Notes` + keyboard still eyes-on in the UTM app (`utmctl attach` not implemented).


## UTM eyes-on notes (M151–M166)

- `utmctl attach` prints **not implemented**; use the UTM window **ZealosAarch64Hello (Terminal 1)** for serial, or the main FB window for graphics/shell text.
- After `make utm` + Play: expect shell banner and `>` on a **cleared** dark FB (boot smokes no longer leave bars/lines under the prompt).
- M152: startup shows short `type: help | vblk | …` hints (full list via `help`).
- M153: after `disklat` / `nearlatticelite`, FB should clear (no leftover HUD/`Width:nan` soup); `>` on clean surface.
- M154: Lattice idle — empty MessageGet no longer aborts the demo early; scripted ESC still ends the smoke.
- M155: `latticeplay` — live Lattice on UTM (tablet/keys); Esc to exit. `disklat` remains the scripted smoke.
- M156: compose *Lites (Demo/Frame/Live/Play/Wrap) idle like Lattice on empty MessageGet.
- M157: after `disklat` / `latticeplay` / etc., shell restores dark FB before `ok` (not pure black from DCFill).
- M158: in `latticeplay`, Enter restarts (virtio Enter maps to `\n` like Lattice Restart).
- M159: `latticeplay` prints a short Esc/Enter/Space hint on the FB after Cls.
- M160: `latticeplay` `'c'` shows Mid/Edge color swatches at the bottom of the FB.
- M161: `latticeplay` `'c'` cycles Mid/Edge color pairs (first press still YELLOW/BLACK).
- M162: `latticeplay` Enter/Restart re-prints the Esc/Enter/Space/c hint after DCFill.
- M163: DiskLat RedSea load buffer grown to 16K (M162 source exceeded 8K).
- M164: `latticeplay` Enter/`DCFill` resets the color cycle and Mid/Edge swatches to match TurtleInit.
- M165: `latticeplay` shows Mid/Edge swatches from entry (`Cls`) and keeps them on top after each `Refresh`.
- M166: `latticeplay` controls hint survives `Refresh` and mentions `+/-=w`.
- M167: `disklat` / `latticeplay` DrawIt HUD includes `di` (angle-step degrees) like NearLattice/Lattice.
- M168: `latticeplay` second hint line — L-click place / R-drag aim / `e=ends` (survives Refresh with the first line).
- M169: `latticeplay` third hint line — `arrows=di/speed` / `0-9=layer`.
- M170: `disklat` / `latticeplay` DrawIt draws HUD + head only (no per-Refresh `TurtleMove` preview) so idle frames do not flood the FB with red strokes.
- M171: DrawIt erases the previous LTRED head before redraw so place/aim do not leave ghost markers.
- M172: head erase uses BLACK GrLines (not `FillRect`) so Space strokes near the turtle are not punched out.
- M173: DrawIt skips head erase/redraw when x/y/th unchanged so idle Refresh does not thrash BLACK/LTRED over the path.
- M174: `DCFill` (Enter/Restart) resets the depth buffer so new strokes are not occluded by stale z cells.
- M175: `Cls` (latticeplay entry) also clears the depth map, matching `DCFill`.
- M176: after Space, DrawIt does not BLACK-erase the old head (the stroke already covered it), avoiding a black notch at the step start.
- M177: compose exposes stock `Lattice()` entry (wrapper around `DiskLat`); RedSea `Lattice.ZC` / smokes still expect **15**.
- M178: `help` lists `latticeplay` controls (Esc/Enter/Space/c/+/-/e, place/aim, arrows, layers).
- M179: `latticeplay` prints the same controls line on entry (serial/FB shell) before the live loop.
- M180: Lattice idle `Sleep(16)` (~60 Hz) instead of `Sleep(1)` so UTM/`latticeplay` is not Refresh-bound at ~1 kHz.
- M181: `Sleep` peeks virtio kbd/tablet (and UART RX on Pi) about every 1 ms and returns early so idle `Sleep(16)` does not add up to ~16 ms input lag.
- M182: `StockLat.ZC` — near-verbatim stock `Demo/Graphics/Lattice.ZC` (TempleOS π/θ glyphs, `U0` helpers, setup stubs) scripted SPACE+ESC → **15**. Interactive `latticeplay` remains DiskLat-shaped (idle Sleep, head erase).
- M183: DiskLat/`latticeplay` Turtle fields use stock `θ` / `dθ_idx` names (and HUD labels), matching StockLat.
- M184: FB `GrPrint` draws TempleOS `0xE3`/`0xE9` as π/θ glyphs (was `?` on the ASCII-only font).
- M185: NearLatticeLite Turtle fields/HUD also use stock `θ` / `dθ_idx` (DiskLat parity).
- M186: DemoLite/PlayLite/FrameLite/LiveLite Turtle fields/HUD use stock `θ` / `dθ_idx`.
- M187: remaining turtle *Lites (DrawIt/End/Event/Micro/Move*/AimMove/StepCount/Tick/Lattice/IdxAll/Inc/Init/Right) use stock `θ` / `dθ_idx`.
- M188: TurtleLite uses TempleOS `θ` (drops ASCII `theta` stand-in).
- M189: live `MessageGet` (`g_hc_popup_live`) interruptible `Sleep(16)` before NULL; DiskLat/compose NULL handlers only `Refresh` (smokes stay immediate-NULL).
- M190: `stockplay` — live StockLat (MsgQue stripped, `popup_live`); StockLat `Refresh` after switch; StockPlay smoke ESC-only.
- M191: `stockplay` injects the same FB controls GrPrint cues as `latticeplay`; chrome/Restart hints say `dθ/speed` (TempleOS 0xE9).
- M192: UTF-8 `θ`/`π` (U+03B8/U+03C0) folded to TempleOS `0xE9`/`0xE3` on compile; fixed UTF-8 π bytes (`CF 80`); `TosUtf8Lite` → **15**.
- M193: live `stockplay` strips DrawIt idle `TurtleMove(dc, &t2, RED, LTRED)` (Space Step kept); StockPlay smoke keeps stock DrawIt.
- M194: StockLat DrawIt uses DiskLat-style `HeadMark` + pose skip (no idle TurtleMove / LTRED ghosts); Space/Restart clear `head_on`.
- M195: StockLat Restart accepts `\r`/`\n` and re-prints controls GrPrint (stockplay Enter parity with latticeplay).
- M196: StockLat `Cls(0)` + `head_on=0` at try entry; play-src injects after `Cls` (same as DiskLat) so stockplay starts on a clean FB.
- M197: `stockplay` help/entry cues match `latticeplay` controls one-liner; StockLat `Lattice()` wrapper → LatticeSmoke.
- M198: shell `stocklat` runs scripted StockLat (SPACE+ESC → **15**); help lists it next to `stockplay`.
- M199: top-level `help` summary lists `stocklat` / `stockplay` with the other Lattice cmds.
- **Scope:** QEMU/`disklat`/`LatticePlay` exercise DiskLat-shaped compose source, **not** stock ZealOS `Demo/Graphics/Lattice.ZC` with original behavior. Live `stockplay` softens idle DrawIt (M193–M194) but still ≠ DiskLat/latticeplay. Place/aim/step/restart/colors need an eyes-on UTM pass after `make utm`.
- Freeze checklist still: `vblk`, `rspersist`, `rscatalog`, `runzc` UseAdd/Notes/MemSort (same as QEMU `run-pci`).

## Notes

- Case-insensitive names: `runzc notes.zc` is OK.
- Root dir has **8** slots; catalog is sized to fit with `.` / `Keep` / `Hi.ZC`.
- Catalog seed is **best-effort** (full root from an older UTM image no longer blocks `rsdir` / `runzc`).
- QEMU: `make run-serial` / `make run-pci`.
- Pi: `make pi-sd` → `build/pi-esp/` — do not install Pi-diag ELFs into UTM.
