# ZealOS aarch64 bring-up

Limine → ZealBooter → kernel + `demo.hcbc` → HolyC-IR JIT → FB text shell → **virtio keyboard + tablet**

**Not a full ZealOS port.** Two parallel tracks: QEMU/UTM virt (HolyC + RedSea + Lattice compose) and Pi 4B UEFI (boot/timer/IRQ/shell; UART RX still hardware-pending).

## Native source compatibility milestone

A new **persistent compiler session** runs the pinned Aiwnios ARM64 compiler
inside the freestanding guest. The first integration target is the complete,
unchanged upstream `Kernel/QuickSort.ZC`, with separate callers exercising
recursion, callbacks, shared symbols, F64 and reclaimable allocations.

```sh
make check-compat       # disposable ISO guest + old boot regressions
make check-compat-pci   # disposable UTM-shaped GPT/PCI disk guest
make utm               # refresh VM, preserving RedSea/source edits and adding new bundled files
```

The UTM refresh also updates the repository-owned
`Tests/WindowDragLive.ZC` fixture so its ABI stays in sync with the kernel;
other existing guest source files remain untouched.

At the UTM prompt, run `zcheck`. Expected: `zc: upstream QuickSort + persistent
modules OK`. Use `zc` for declarations/statements, `zload` for a source file in the
boot archive, `zcall` for a no-argument integer function, and `zreset` to discard
the native session. Existing `hc`/`runzc` commands keep their original behavior.
`zput Name.ZC <source>` saves a short source file on the native 32 MiB source
partition in the 128 MiB UTM/GPT disk (legacy ISO/MMIO uses RedSea);
`zload disk:Name.ZC` compiles it, including after a full reboot. The QEMU gates also
exercise a relative include between two disk files. `zverify` checks the
seeded large upstream files, and `scripts/source-volume.py put` can import a
larger file while the VM is stopped. For a running UTM guest, `zrecv` accepts
larger files as acknowledged serial hex chunks; `scripts/send-zc.py --emit`
produces lines to paste into Terminal 1. `zls` lists live source files and
`zrm <path>` hides one; `zgc` reclaims superseded records in the running guest
using the disk's second source bank. `zedit <path>` opens a guest
line editor with `p`, `a`, `i`, `c`, `d`, `wq` and `q!` commands; saved files
load through `zload disk:<path>`. The complete unchanged upstream
`KernelA.HH` and `KernelB.HH` load as declaration headers in that order.
The unchanged `KMathB.ZC` also loads with ARM64 bindings for task-local `Fs`,
bit operations, the counter and a powers-of-ten table. A bounded cooperative
task bridge exposes the default upstream `Spawn` callback signature, `Yield`,
`TaskValidate`, `Suspend`, `IsSuspended`, `Kill`, `BirthWait`, `DeathWait`,
idle/prompt `TaskWait`, bounded `TaskMessage`/`MessagePost`/`MessageGet`
delivery, a per-task inherited-symbol `TaskExe`/`JobsHandler` subset with `MessageScan`
job dispatch, key-aware `MessageScan`/`MessageGet` and `KeyScan`/`KeyGet` over polled
virtio/serial input, portable `CharScan`/`CharGet` behavior, and the pinned
`sys_focus_task` pointer for a focused child waiting on a key. It also exposes
CPU-0 `JobQueue` callbacks and `SpawnQueue` task creation. A reserved executive runs queued jobs; four
live app child tasks still have their own stacks and allocation owners.
The bridge also maintains the upstream-visible runnable-task and parent/child
task rings, including `TaskFocusNext` selection and child unlink on teardown.
`TaskMathChecks`,
`TaskSwitchChecks`, `SpawnBridgeChecks`, `TaskLifecycleChecks` and
`TaskWaitChecks`, `TaskIdleChecks`, `TaskMessageChecks`, `TaskExeChecks` and
`TaskExeLifecycleChecks`, `TaskExeFocusChecks`, `TaskMessageJobsChecks`, `TaskKeyChecks`,
`TaskCharQueueChecks`, `KernelBitOpsChecks`, `TaskIRQFlagsChecks`, `TaskQueueChecks`,
`TaskFocusKillChecks`, `TaskCompilerScopeChecks`, `TaskAnswerChecks`, `TaskJobQueueChecks` and
`TaskSpawnQueueChecks` exercise
those paths. Most other x86 intrinsics and unresolved externals
remain unavailable. This does not boot the upstream kernel or provide its
preemptive scheduler.

[COMPATIBILITY.md](COMPATIBILITY.md) describes the scope and remaining work.
This is a compiler/runtime foundation, not the complete ZealOS desktop or an
installer. The immediate focus is UTM; Pi diagnostics remain a separate path.

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
| QEMU/UTM virt | **Regression freeze** M33–M39 (`Notes.ZC` / DocLib `#include`) still the bar — see ACCEPTANCE.md. **Lattice compose** continued on top (M151–M199) + **M200–M206** PL011 RX/`uartrx`, shell idle, depth clear, StockLat NULL, PopUpColor Mid/Edge headers + live titles. The UTM framebuffer keyboard now passes shell input and native `KeyGet`; the full mouse/Notes graphics pass remains a separate interactive check. |
| Pi 4B UEFI | PI-DIAG-1..4 + **shell entry on FB** accepted (2026-09-23). **UART RX:** use `uartrx` on USB–TTL @ 115200 after `make pi-sd` (M200 prep). No virtio/RedSea disk on Pi yet ([PI4.md](PI4.md)). |

## Earlier platform follow-ups

1. UTM eyes-on after `make utm`: freeze checklist + `latticeplay`/`stockplay` (place/aim/step/restart/colors/Esc)
2. Pi: prove UART RX — `uartrx` then `help`/`halt` on USB–TTL @ 115200; then native storage / keyboard path
3. HCRT host stubs only after IR gap shrinks; DT-based discovery before other ARM64 boards
