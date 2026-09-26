# ZealOS aarch64 bring-up

Limine → ZealBooter → kernel + `demo.hcbc` → HolyC-IR JIT → FB text shell → **virtio keyboard + tablet**

**Not a full ZealOS port.** Two parallel tracks: QEMU/UTM virt (HolyC + RedSea + Lattice compose) and Pi 4B UEFI (boot/timer/IRQ/shell; UART RX still hardware-pending).

## Status

```
virtio-kbd: OK @ 0xa003e00
virtio-tablet: OK @ 0xa003c00
hc IR OK (front+host)
bc: module => 0x2d OK
> bars
bars ok
> stars
stars n=0x78
> circles
circles ok
> bounce
bounce frames=0x24
> paint
paint dots=0x0
```

| Piece | Result |
|--|--|
| Boot + GIC/CNTP + module JIT | OK |
| HolyC (`enum`/`union`, `I64*`/`U8*`, `U32`/`Bool`, `true`/`NULL`, ≤8 fns/≤5 args) | OK |
| Host: strings/`Mem*`/`HashStr`, math, `Cnt`, mouse deltas, keys, gfx | OK |
| Fast `Cls`/`FillRect` + `GrLine`/`GrHLine`/`GrVLine`/`GrCircle`/`GrFillCircle` | OK |
| Shell `bars` / `stars` / `circles` / `bounce` / `paint` | OK |
| virtio keyboard + tablet | OK (QEMU/UTM) |
| RedSea + freeze `runzc` UseAdd/Notes/MemSort | OK (`make check-serial` / `run-pci`; UTM eyes-on in ACCEPTANCE.md) |
| Lattice compose (`disklat` / `latticeplay`, through M166) | QEMU scripted green; live UTM controls are eyes-on |

Window keystrokes (`make run-iso`) and serial both feed the shell. HHDM lacks the virtio-mmio window, so the kernel maps a 2MiB Device block at `0x0a000000` before probing.

`make check-serial` uses QEMU virt + MMIO virtio disk. It does **not** exercise the UTM app window or Pi UART RX. Lattice smokes run a reduced DiskLat-shaped source ([upstream/DiskLat.ZC](upstream/DiskLat.ZC)), not proof that stock ZealOS `Demo/Graphics/Lattice.ZC` runs unchanged.

## Shell

```
help | abs | sum | bars | stars | circles | bounce | paint | halt
hc GrCircle(200,150,40,0xFFFFFF);return 1;
hc I64 x=3;I64 *p=&x;*p=9;return x+(NULL==0);
disklat | latticeplay   # Lattice compose; Esc exits live play
```

## Build

```sh
make && make run-serial
make run-iso   # bars / stars / circles / bounce / paint / click / Esc
make check-serial   # QEMU regression (must print hc IR OK)
```

UTM: `make utm` → only **ZealosAarch64Hello** ([ACCEPTANCE.md](ACCEPTANCE.md)).

Pi 4B UEFI (parallel track): [PI4.md](PI4.md) — `make pi-sd` stages Limine/ESP; unpack [pftf/RPi4](https://github.com/pftf/RPi4/releases) onto the SD root first.

Bridge notes: [AIWNIOS.md](AIWNIOS.md). Milestone ladder: [upstream/README.md](upstream/README.md).

## Tracks

| Track | Status |
|--|--|
| QEMU/UTM virt | **Regression freeze** M33–M39 (`Notes.ZC` / DocLib `#include`) still the bar — see ACCEPTANCE.md. **Lattice compose** continued on top (M151–M185: FB teardown, `latticeplay`, controls cues, DrawIt head, depth reset, `Lattice()` entry, ~60 Hz idle, interruptible Sleep, stock `StockLat` smoke, stock `θ`/`dθ_idx` names, FB π/θ glyphs). Scripted QEMU ≠ live UTM keyboard/tablet proof. |
| Pi 4B UEFI | PI-DIAG-1..4 + **shell entry on FB** accepted (2026-09-23). **UART RX not yet proven** on hardware (`help`/`halt` via USB–TTL). No virtio/RedSea disk on Pi yet ([PI4.md](PI4.md)). |

## Next

1. UTM eyes-on after `make utm`: freeze checklist + `latticeplay` (place/aim/step/restart/colors/Esc)
2. Pi: prove UART RX on USB–TTL @ 115200, then native storage / keyboard path
3. HCRT host stubs only after IR gap shrinks; DT-based discovery before other ARM64 boards
