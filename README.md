# ZealOS aarch64 bring-up

Limine → ZealBooter → kernel + `demo.hcbc` → HolyC-IR JIT → graphical terminal → **native USB or VirtIO input**

**Not a full ZealOS port.** Two parallel tracks: QEMU/UTM virt (HolyC + RedSea + Lattice compose) and Pi 4B UEFI (boot/timer/IRQ/shell plus guarded read-only PCIe config discovery; physical verification is next, with USB still disabled).

## Native source compatibility milestone

A new **persistent compiler session** runs the pinned Aiwnios ARM64 compiler
inside the freestanding guest. The first integration target is the complete,
unchanged upstream `Kernel/QuickSort.ZC`, with separate callers exercising
recursion, callbacks, shared symbols, F64 and reclaimable allocations.

```sh
make check-compat       # disposable ISO guest + old boot regressions
make check-compat-pci   # disposable UTM-shaped GPT/PCI disk guest
make check-gui-workflow # visible typing/cursors + graphical edit/save/run/reboot
make check-usb-input    # native NEC/QEMU xHCI keyboard, mouse/tablet + recovery
make check-usb-topology # nested USB2 hubs, repeated reconnects + subtree recovery
make check-pi4-platform # firmware FDT parsing + bounded Pi USB DMA reservation
make check-task-settings # upstream application state + task-local directories
make check-source-context # directory masks + relative document/project files
make check-source-execute # upstream ExeFile + native task-local file execution
make check-source-editor # full-screen editor scanout + save/reopen/run
make check-compiler-recovery # malformed source, statement rollback and task/stack recovery
make check-compiler-context # compiler queues, option inheritance and task isolation
make check-run-file     # task-local LastFun + upstream RunFile with application arguments
make check-source-exceptions # catchable source/file errors + prompt, job and task cleanup
make check-system-launch # upstream Sys/SysFile + popup application arguments
make check-system-server # dedicated source System loop + namespace/job ownership
make check-user-command # original ExeCmdLine loop, options and error recovery
make check-user-startup # per-user HomeSys setup and independent graphical terminals
make check-home-once    # original Once include, boot-media branch, tips and prompt recovery
make check-home-autostart # disk boot launches the saved Home/Once.ZC in the GUI terminal
make check-autocomplete # boot indexing and visible GUI fill-in on a disposable disk
make check-user-text    # User startup commands + XTalk input delivery and recovery
make check-home-path    # shared home paths, includes and saved source after reboot
make check-registry     # DolDoc execution, shared registry and Once across reboot
make check-input-edges  # visible modifier typing, held keys and explicit release
make utm               # refresh VM, preserving RedSea/source edits and adding new bundled files
```

The UTM refresh also updates the repository-managed
`Tests/WindowDragLive.ZC`, `System/GuiTerminal.ZC`, `System/TaskBridge.ZC`,
`System/UserTask.ZC`, `System/HomeStartUp.ZC`, `System/SourceDir.ZC`, `System/SourceHome.ZC`,
`System/VersionScalars.ZC`, `System/Registry.ZC`, `System/SourceRegistry.ZC`,
`System/SourcePrompt.ZC`, `System/SourceTraining.ZC`,
`System/DolDoc/DocCompiler.ZC`, `System/DolDoc/DocTree.ZC`,
`System/DolDoc/DocClipBoard.ZC`, `System/DolDoc/DocFile.ZC`,
`System/DolDoc/DocTerm.ZC`, `System/DolDoc/DocEd.ZC`, `System/WinMgr.ZC`,
`System/Gr/GrDC.ZC`, and `System/Gr/SpriteRaster.ZC` files
so they stay in sync with the kernel; other existing guest source files remain
untouched.

The kernel now initializes native xHCI input on QEMU virt (UTM's NEC model
and QEMU xHCI). Keep UTM's normal USB 3.0 setting. A successfully enumerated
USB keyboard and pointer take ownership; VirtIO devices remain configured
as recovery input, with their transports parked while USB is active and
reinitialized on disconnect/failure. Terminal 1
serial input stays independent. `zstatus` reports the selected owners, raw
USB events, controller state, errors, and each device's route including USB2
transaction-translator slot/port; serial `usbcheck` exercises command and
event ring wraparound. This USB backend supports boot keyboards and
unnumbered HID mouse/tablet reports on root ports or behind USB 1.x/2.0 hubs.
Port-change polling debounces new connections, removes hub subtrees, and
reuses reserved per-device DMA only after successful slot teardown. Limits
are eight devices, fifteen ports per hub and five hub tiers. A SuperSpeed hub
control path is present, but stock QEMU cannot exercise it and physical SS-hub
validation remains open. The QEMU hub stress gate uses generic full-speed hub
devices; physical high-speed TT routing is also pending. USB storage and Pi's
PCIe/VL805 backend remain future work.

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
using the disk's second source bank. `zedit <path>` opens a guest line editor;
`j`/`k` move the selected line, `[`/`]` page through the file, and `g`/`G`
jump to its first or last line. It also supports `p`, `a`, `i`, `c`, `d`, `wq`
and `q!` commands, as well as numbered-line edits. Saved files load through
`zload disk:<path>`. The graphical editor reads absolute `/...` paths from the
writable source volume before falling back to bundled files, and saves those
absolute paths to the same persistent volume. `FilesFind` returns a merged
view of bundled and native source files: saved replacements supply their native
size, saved-only files appear in listings, and tombstoned files are hidden.
Nested native-only paths also synthesize their parent directories in recursive
directory searches, and deleting the last native file removes those synthesized
directories from the merged view. Serial uploads refresh the same listing cache
as ordinary saves and deletes.
While `zedit` is active, the full-screen source view is composited over the
desktop every frame; enter its line-edit commands in UTM's built-in Terminal 1.
`make check-source-editor` checks visible controls and edits, modified/saved
state, disk reopen, and execution of the saved file.
The complete unchanged upstream
`KernelA.HH` and `KernelB.HH` load as declaration headers in that order.
The boot terminal's OS menu includes a Window Demo entry that launches the
two-window move/resize fixture and restores the terminal when the demo stops.
The unchanged `KMathB.ZC` also loads with ARM64 bindings for task-local `Fs`,
bit operations, the counter and a powers-of-ten table. A bounded cooperative
task bridge exposes the default upstream `Spawn` callback signature, `Yield`,
`TaskValidate`, `Suspend`, `IsSuspended`, `Kill`, `BirthWait`, `DeathWait`,
idle/prompt `TaskWait`, bounded `TaskMessage`/`MessagePost`/`MessageGet`
delivery, a per-task inherited-symbol `TaskExe`/`JobsHandler` subset with `MessageScan`
job dispatch, key-aware `MessageScan`/`MessageGet` and `KeyScan`/`KeyGet` over polled
USB/VirtIO/serial input, portable `CharScan`/`CharGet` behavior, and the pinned
`sys_focus_task` pointer for a focused child waiting on a key. It also exposes
CPU-0 `JobQueue` callbacks and `SpawnQueue` task creation. A reserved executive runs queued jobs; four
live app child tasks still have their own stacks and allocation owners.
Desktop boot also starts a separate System task in a reserved slot. Its
`ServerTaskCont` control flow comes from pinned `KTask.ZC`, with job execution
routed through the native compiler/unwind dispatcher. `Sys` and `SysFile`
execute there; the serial shell retains its own root stack and input. System
shares the persistent kernel namespace, has its own guest heap, directory,
queues and compiler-context stack, and drains UI messages as upstream does.
The graphical terminal now runs the original `UserCmdLine`, `UserTaskCont` and
`ExeCmdLine` control flow through a private ARM64 compiler ABI. Type ZealC
directly, for example `40+2;`. `User;` creates another terminal with its own
document, compiler context and history. The default per-user hook retains the
pinned `HomeSys.ZC` welcome-document/window setup, then installs the accepted
graphical input adapter. `User("40+2;\n")` supplies an initial command;
`XTalk` and `XTalkWait` send further input to that terminal's compiler through
the source KeyDev input filter. `ExeDoc` now reads real DolDoc entries through
the native lexer, including quoted formatting, macros and includes. The original
registry tree APIs share a System-owned cache backed by `~/Registry.ZC`.
`Once`, `SysOnce` and explicit `OnceExe` execute and flush saved User/System
branches. On disk, RAM, and DVD boots, the first graphical terminal now
receives the original `Cd; #include "Once";` startup command through `XTalk`.
DVD boot opens the live graphical session and says that the legacy disk
installer is not yet ported to ARM64; disk/RAM startup also opens a second
terminal and tiles it beside the first, excluding bridge/System tasks from the
window layout. Startup queues autocomplete indexing after `Once`; the
autocomplete corpus is indexed by its background task, and the first prompt is
checked for a visible caret and usable input after startup output. The bundled
ARM64 `ZSplash.ZC` adapter restores the pinned CP437 mesh asset, runs the
upstream rotating splash with the native pointer enabled, and restores terminal
focus when dismissed. Automated scanout checks cover the two-terminal boot,
splash animation and dismissal, popup windows, terminal input and autocomplete
fill-in.
The original `Cd; #include "Once";` path now runs with the compiler's `.ZC`
default extension, source-backed `YorN` and `TipOfDay`, and the original tips and
customization documents. `sys_boot_src` comes from the actual Limine module
medium: disk, optical, or TFTP download. The splash asset is restored from its
UTF-8 CP437 source bytes, and the renderer normalizes the 0x100 transform flag
before using its 8-bit Boolean state. The DVD live path is covered by
`make check-dvd-live`; disk/RAM HomeSys startup preserves its formatted boot-time
banner, two-terminal layout, splash and popup flow. `make check-home-once` also
covers the disk and ISO/DVD noninstall branches, including include/tip/error
cleanup. The ARM64 compiler preserves superscript, subscript and normal-script
DolDoc markers inside identifiers and executes inline binary DolDoc tokens,
including payloads with embedded NUL bytes. Startup uses a guarded translation
of the pinned `HomeSys.StartUpTasks` body; the original YDE disk installer
remains unavailable because its boot and storage paths are not ported to
ARM64. `zstatus`
also reports `keyboard_state` modifier and repeat values to diagnose held host
keys separately from guest decoding.
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
| native USB keyboard + mouse/tablet | NEC and QEMU xHCI matrices pass USB-only, VirtIO fallback, nested hubs/hotplug; live UTM typing and released-pointer drag accepted |
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
| QEMU/UTM virt | **Regression freeze** M33–M39 (`Notes.ZC` / DocLib `#include`) still the bar — see ACCEPTANCE.md. **Lattice compose** continued on top (M151–M199) + **M200–M206** PL011 RX/`uartrx`, shell idle, depth clear, StockLat NULL, PopUpColor Mid/Edge headers + live titles. Automated QMP tests verify printable keys reach the graphical terminal and native `KeyGet`; the 2026-10-16 UTM check also confirmed the physical keyboard entered and submitted `zstatus` in the graphical terminal. |
| Pi 4B UEFI | PI-DIAG-1..4 + **shell entry on FB** accepted (2026-09-23). HDMI report includes FDT PCIe windows, reserved DMA arena, and a guarded read-only bus 1/device 0/function 0 ID/class/header probe when link-up; synthetic bounds tests pass. **Physical Pi verification and UART RX** (`uartrx` on USB–TTL @ 115200) remain. No virtio/RedSea disk on Pi yet ([PI4.md](PI4.md)). |

## Earlier platform follow-ups

1. UTM eyes-on after `make utm`: freeze checklist + `latticeplay`/`stockplay` (place/aim/step/restart/colors/Esc)
2. Pi: prove UART RX — `uartrx` then `help`/`halt` on USB–TTL @ 115200; then native storage / keyboard path
3. HCRT host stubs only after IR gap shrinks; DT-based discovery before other ARM64 boards
