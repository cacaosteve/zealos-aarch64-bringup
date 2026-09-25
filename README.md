# ZealOS aarch64 bring-up

Limine → ZealBooter → kernel + `demo.hcbc` → HolyC-IR JIT → FB text shell → **virtio keyboard + tablet**

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
| virtio keyboard + tablet | OK |

Window keystrokes (`make run-iso`) and serial both feed the shell. HHDM lacks the virtio-mmio window, so the kernel maps a 2MiB Device block at `0x0a000000` before probing.

## Shell

```
help | abs | sum | bars | stars | circles | bounce | paint | halt
hc GrCircle(200,150,40,0xFFFFFF);return 1;
hc I64 x=3;I64 *p=&x;*p=9;return x+(NULL==0);
```

## Build

```sh
make && make run-serial
make run-iso   # bars / stars / circles / bounce / paint / click / Esc
```

UTM: `make utm`.

Pi 4B UEFI (parallel track): [PI4.md](PI4.md) — `make pi-sd` stages Limine/ESP; unpack [pftf/RPi4](https://github.com/pftf/RPi4/releases) onto the SD root first.

Bridge notes: [AIWNIOS.md](AIWNIOS.md).

## Tracks

| Track | Status |
|--|--|
| QEMU/UTM virt (HolyC + RedSea + virtio) | **Frozen** through M39 `Notes.ZC` (DocLib `#include`). See ACCEPTANCE.md. |
| Pi 4B UEFI | PI-DIAG-1..4 OK; shell on FB — UART RX next ([PI4.md](PI4.md)) |

## Next

1. Pi UEFI SD: firmware zip + `make pi-sd` → serial/FB shell on hardware
2. Continue UTM compatibility demos as needed
3. HCRT host stubs only after IR gap shrinks
