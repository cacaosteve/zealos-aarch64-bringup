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
| Serial | UTM built-in Terminal, shown as **ZealosAarch64Hello (Terminal 1)** |

There must be **one** entry. If UTM shows two or Start is greyed out: quit UTM, run `make utm`, reopen. RedSea lives past the ESP; `make utm` preserves that region.

```text
ZealOS aarch64 shell (HolyC-IR exprs)
>
```

The boot graphical terminal is a guest window with a smaller command set.
Click inside it before typing; its `help` lists graphical commands such as
`zstatus`, `zvol`, `zls`, `zload`, `zcall`, and `edit`. Commands outside that
set report that they need the serial shell. The current installer keeps the
working UTM 5.0.7 built-in Terminal 1 serial window. This is independent of
the graphical guest terminal's keyboard and document rendering. The graphical
`zstatus` output should show
`ready=1 failed=0`, `winmgr=1 gterm=1`, a nonzero `presented_frames`, and the
`gterm_window` / `scanout` diagnostics.

If the VM is deliberately configured to use the older Pseudo-TTY fallback,
query its path after every restart, then connect the Mac Terminal session to
that path (do not reuse a path from an earlier boot):

```sh
/Applications/UTM.app/Contents/MacOS/utmctl attach ZealosAarch64Hello
screen /dev/ttysNNN 115200
```

On this UTM version, `utmctl attach` prints a warning that attachment is not
implemented, but still reports the current `PTTY: /dev/ttysNNN`. Use that
reported path in the `screen` command. For example, the 2026-10-16 live check
reported `/dev/ttys001`; the earlier `/dev/ttys002` was no longer connected.

## Graphical development gate

Run `make check-gui-workflow` on a disposable PCI guest. It checks framebuffer
glyphs before Return, a visible insertion caret, tablet-pointer pixels, and
the graphical edit/save/compile/run/reboot/reopen path. Successful command
assembly and nonzero key counters alone do not establish visible input.
Screenshots and decoded text are retained under `build/gui-*.ppm` / `.txt`.

After refreshing the live UTM VM, check the physical input path once:

1. Click the graphical prompt. Type `help` without Return: the letters must
   appear immediately, with an insertion caret. Move the mouse and confirm
   its pointer stays visible. Then press Return.
2. Run `edit /Tests/GuiAcceptance.ZC`, type
   `I64 GuiAcceptance(){return 42;}`, and press Escape to save and return.
3. Run `zload disk:Tests/GuiAcceptance.ZC`, then `zcall GuiAcceptance`.
   Expect `0x000000000000002a`.
4. Restart and run `edit /Tests/GuiAcceptance.ZC`. The saved source must be
   visible. Escape saves/exits; Shift+Escape cancels/exits.

The live UTM check is separate from QMP injection, which bypasses UTM's host
keyboard capture. Keep the display adapter and serial mode fixed during this
gate so input/rendering changes can be compared against the same VM setup.

## Application settings gate

Run `make check-task-settings` on a disposable PCI guest. It boots the
unchanged upstream TaskSettings module, checks nested window/palette/document
restoration, and verifies task-local directories and parent-owned allocations
survive child exit. Relative file writes, loads and includes must work.
Follow with `make check-gui-workflow` after changing the editor wrapper or text
state: visible typing and the save/cancel/reopen paths must still pass.

`make check-source-context` checks nested source-directory contexts, mask
lists, task isolation, project-file lookup, and relative links through the
unchanged upstream `DocLinkCheck`. The graphical editor must reject a missing
parent directory and return with its saved settings restored.

`make check-source-execute` checks the upstream `ExeFile` wrapper with native
file compilation, relative includes, nested files, task isolation, cooperative
execution, and `CCF_JUST_LOAD`. In the graphical terminal, an executable source
file can be included with `zc ExeFile("/path/Program.ZC");`. It executes in
the current task, as a command-line include does in upstream ZealOS.

2026-10-17 application-state update: the focused settings gate passed 18
checks, the full PCI suite passed 354, the ISO suite passed 277, and the NEC
two-hub/VirtIO recovery workflow passed 157, including edit/save/run/reboot/reopen, twelve keyboard
reconnects, tablet fallback/reattach and whole-hub recovery.
The live refreshed UTM guest displayed `help` and its caret before Return,
reopened `/Tests/GuiAcceptance.ZC` with its function returning 42, and cancelled
back to the same terminal with Shift+Escape. Its graphical `zstatus` reported
`ready=1 failed=0`, terminal focus, native USB keyboard/tablet owners, zero
dropped routed keys and no controller errors. Input capture was released.
USB 3.0, display and built-in serial settings matched the pre-refresh backup.

If an existing UTM graphical viewer reports “Display output is not active”
after a completed serial boot, stop only this VM, close its stopped graphical
window, and start it from UTM Library to create a fresh viewer. That restored
the display during this update without changing the adapter or other VMs.

## Native USB input gate

`make check-usb-input` uses disposable QEMU/HVF PCI guests with NEC xHCI
(UTM's default USB 3.0 controller) and QEMU xHCI. USB-only runs omit VirtIO
input devices. The gate requires enumeration of HID keyboard, relative mouse
and absolute tablet, visible typing/caret/pointer, button press/release,
wheel reports, repeated command/event/transfer ring cycling, and the same
graphical edit/save/run/reboot workflow. A separate run keeps duplicate
VirtIO input devices, checks that each key-down is delivered once, then
unplugs the USB keyboard and checks visible VirtIO recovery typing. The HID
decoder also runs under address/undefined-behavior sanitizers on the host.

In live UTM, preserve USB 3.0 and run `zstatus`. Expect `keyboard=usb`,
`pointer=usb-tablet`, a running xHCI controller and `errors=0`. Type `help`
without Return, move the released tablet cursor, click/drag windows, and
repeat the graphical development gate above. Headless QMP acceptance does
not establish physical host capture/release behavior in the UTM application.
Serial Terminal 1 remains available throughout. `usbcheck` is a serial-only
diagnostic for bounded No-Op command ring cycling.

`make check-usb-topology` adds USB2 hub trees (two tiers on NEC, five on
QEMU), twelve keyboard disconnect/reconnect cycles, held-Shift removal,
held-button tablet removal with mouse fallback, and whole-subtree detach
and reattach. It checks visible typing after recovery and constant DMA usage.

Scope: HID input on emulated NEC/QEMU xHCI, polled without MSI. USB 1.x/2.0
hubs support up to fifteen ports and five tiers within the eight-device
limit. Unsupported HID layouts and SuperSpeed hubs do not claim input.
Disconnect/failure releases USB ownership and held input. Safe slot teardown
allows record/DMA reuse; failed teardown quarantines the region. No Pi or
physical USB controller acceptance is implied.

2026-10-17 topology gates passed on NEC root ports, NEC with two nested hubs
and VirtIO recovery, and USB-only QEMU with five nested hubs (all eight device
records occupied). Each run replaced the keyboard twelve times and checked
visible typing after recovery without DMA growth. Tablet removal released a
held button and selected the remaining mouse. Both hub runs removed and
rebuilt the entire subtree, restored visible typing, and finished with zero
USB errors or quarantined regions. The NEC hub run also saved, compiled and
ran an edited function, rebooted, and reopened its persisted source.
The refreshed live UTM guest retained USB 3.0, display and serial settings,
showed `help` before Return, and executed graphical `zstatus` with native
keyboard/tablet owners, two running controllers, zero quarantined regions
and zero dropped routed keys. Capture was released. After correcting an
earlier response about the selected capture button and an unperformed drag
check, the user confirmed that the refreshed VM interaction works. The
earlier response is not counted as a separate drag acceptance result.

2026-10-17 live UTM: USB 3.0, display and built-in Terminal 1 settings were
preserved. The guest reported two running controllers (`error=0`), three
USB devices, `keyboard=usb`, `pointer=usb-tablet`, and parked VirtIO input.
Host key injection visibly entered the mixed-case
`edit /Tests/GuiAcceptance.ZC` command, reopened the saved function returning
42 and cancelled back to the prompt with Shift+Escape. A fresh `zstatus`
reported zero dropped routed keys. With capture released, the user confirmed
that the white guest arrow follows the physical mouse and the graphical
terminal can be dragged by its top border. This accepts the physical UTM
tablet path separately from QMP injection.

2026-10-17, after the native `ExeFile` refresh: the user confirmed visible
physical typing and successful graphical `help`. Their 18:19 screenshot shows
the command, output, and prompt caret with capture released. This verifies
graphical boot and physical keyboard input on the refreshed image. Their 18:21
screenshot and explicit confirmation then accepted
`zc ExeFile("/Tests/GuiAcceptance.ZC");` followed by `zcall GuiAcceptance`:
the graphical terminal reports `zc: executed`, returns `0x2a`, and displays a
fresh prompt and caret. The saved-file execution gate is accepted in live UTM.
The earlier physical pointer/drag acceptance was not repeated for this
compiler-only change.

`make check-compiler-recovery` checks failed definitions without losing an
existing implementation, class and include errors, assertion/EOF failures,
compiler re-entry rejection, continued execution in source jobs, child tasks
and on allocated stacks, and repeated
failures without managed-arena growth. Successful earlier statements in a
partially failed source file must remain usable. The graphical workflow also
injects malformed source, observes its error, and executes a corrected command
while retaining the terminal and its keyboard focus. Runtime allocator failures
remain fatal to the session and are checked separately.

2026-10-17 automated recovery validation passed 49 focused checks, 391 full
PCI checks, 309 full ISO checks and 208 NEC two-hub/VirtIO graphical workflow
checks. The UTM recovery smoke is to enter malformed source,
then define and call a corrected function in the same terminal without reset:

```text
zc I64 Broken( {
zc I64 RecoveryOK(){return 42;}
zcall RecoveryOK
```

Expect a visible compiler error and fresh prompt after the first line, then
`zc: executed` and `0x000000000000002a` from the corrected function.
The refreshed live UTM guest passed this sequence through desktop automation,
including visible source before Return. Its status remained `ready=1 failed=0`
with one recovered error, an inactive recovery boundary and terminal focus.
Physical input was accepted on the earlier native USB baseline and was not
repeated for this compiler-only update.

`make check-compiler-context` checks nested compiler queues, inherited options,
independent child-task defaults, option state across task yields, include
metadata and cleanup without managed-arena growth. It checks decimal-only and
no-floating-constant rejection, corrected input after each error, and mixed
floating-argument functions with register allocation enabled and disabled.
The graphical workflow enables decimal-only mode, rejects a hexadecimal
assignment, disables it and executes the same assignment in the same prompt.
The original Option / OptionGet bodies are checked against pinned CMisc.ZC.

2026-10-17 final automated context validation passed 407 full PCI checks,
325 full ISO checks and 225 NEC two-hub/VirtIO graphical workflow checks.

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

5. To check real window input, reset the compiler after a failed load, then
   load the upstream declarations and the visual test in this order:
   The kernel modules must use their canonical `/Kernel/...` paths so the
   guest installs the ARM64 task bindings when `KernelB.HH` loads. Use `disk:`
   for the test source stored on the UTM source partition. Run `make utm`
   once after adding a bundled source file; refresh preserves guest edits and
   deletion masks while adding files missing from the existing source bank.
   The refresh updates the repository-managed `Tests/WindowDragLive.ZC`,
   `System/GuiTerminal.ZC`, `System/TaskBridge.ZC`, and `System/WinMgr.ZC` files.
   For other existing bundled files, `make utm` keeps the older guest copy. Stop the VM and
   replace that entry explicitly, for example:

   ```sh
   python3 scripts/source-volume.py put \
     /path/to/ZealosAarch64Hello.utm/Data/Omarchy-QEMU.raw \
     tests/compat/WindowDragLive.ZC Tests/WindowDragLive.ZC
   ```

```text
zreset
zload /Kernel/KernelA.HH
zload /Kernel/KernelB.HH
zload /Kernel/SerialDev/Message.ZC
zload /Kernel/Job.ZC
zload /Kernel/KeyDev.ZC
zload disk:Tests/WindowDragLive.ZC
zcall WindowDragLiveButtonChecks
zcall WindowDragLiveStart
zcall WindowDragLiveRenderChecks
```

   Click each bordered window to raise it, drag the middle of its title bar to
   move it, drag the left frame edge to move it horizontally, and drag the
   lower-right frame corner to resize it. In Window B,
   right-click the control box; it should turn green while held. With Window B
   focused, scroll the wheel: positive deltas turn it cyan and negative deltas
   turn it purple when UTM reports `REL_WHEEL`. The
   first four
   title cells post the upstream task-menu command; the right title cells close
   a window with no document. The button regression should return `0`, and the
   render check should return `0x2a`; the
   parked demo tasks do not render a task menu. Finish with
   `zcall WindowDragLiveStop`.

   With the source manager active, hold **Ctrl** and scroll up/down over the
   framebuffer. The desktop should zoom in/out around the pointer; scrolling
   without Ctrl should continue to reach the focused window's wheel control.

`make check-window-buttons` runs the same live draw/control probe in the
headless UTM-shaped QEMU guest. It checks that the control callback draws the
expected pixel, the frame is presented, and the framebuffer reports yellow at
that pixel. It does not replace the interactive UTM check for window visibility
and pointer input.

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

## Native compiler and writable source

The full compiler has a separate session and source loader. In the UTM app,
run `zcheck` and expect `zc: upstream QuickSort + persistent modules OK`.
On the refreshed 128 MiB UTM image, try:

```text
zvol
zverify Kernel/KernelA.HH
zls
zput App/Native.ZC I64 DiskValue(){return 42;}
zload disk:App/Native.ZC
zcall DiskValue
```

Expect `zvol: source partition ready bank=A generation=0` on a new disk,
`zverify: OK bytes=113880`, and `0x2a`. An existing disk may report bank B
or a later generation after `zgc`.
Restart the VM, repeat the last two commands, and expect the
same value. `make check-compat` and `make check-compat-pci` perform this reboot
gate on disposable disks and additionally check a disk-backed relative include.
The native source partition does not use RedSea root slots, so all eight
existing UTM RedSea files can remain. The shell input still limits `zput` to
one short line. For a larger file on a running VM, use
`python3 scripts/send-zc.py /path/to/Module.ZC Home/Module.ZC --emit` and paste
the printed lines in UTM Terminal 1, waiting for each `zrecv: ack` before the
next line. A raw TTY or PTY can use `--serial DEVICE` for automatic sending.
The stopped-VM `scripts/source-volume.py put` importer also remains available.
For an in-guest edit, run `zedit App/EditorCheck.ZC`, then at `edit>` enter
`a I64 EditorCheck(){return 42;}`, `p 1 1`, and `wq`. Run
`zload disk:App/EditorCheck.ZC` and `zcall EditorCheck`; expect `0x2a`.
Reopen it with `zedit App/EditorCheck.ZC` to check existing-file edits.
To exercise source cleanup on a disposable name, run
`zput App/RemoveMe.ZC I64 RemoveMe(){return 7;}`, `zrm App/RemoveMe.ZC`,
then `zverify App/RemoveMe.ZC`; the last command must report missing source.
`zls` must omit the deleted name. After stopping UTM, run
`python3 scripts/source-volume.py compact /path/to/VM/Data/Omarchy-QEMU.raw`
to reclaim obsolete source records;
it retains deletions that mask legacy RedSea names. Start UTM again and repeat
the native load/reboot checks.

For in-guest reclamation, run `zls`, `zgc`, then `zls` again. The second
sector count should be no larger, and `zvol` should show the other bank and
one higher generation. Reboot and verify the same bank, source files and
deletion results. The host compactor remains useful for an offline copy.

The unchanged upstream declaration headers are an additional compiler check:

```text
zreset
zload /Tests/MathTable.ZC
zcall MathTableChecks
zload /Kernel/KernelA.HH
zload /Kernel/KernelB.HH
zload /Tests/HeaderCopy.ZC
zcall HeaderCopyChecks
zload /Kernel/KMathB.ZC
zload /Tests/TaskMath.ZC
zcall TaskMathChecks
zload /Tests/TaskSwitch.ZC
zcall TaskSwitchChecks
zload /Tests/SpawnBridge.ZC
zcall SpawnBridgeChecks
zload /Tests/TaskLifecycle.ZC
zcall TaskLifecycleChecks
zload /Tests/TaskWait.ZC
zcall TaskWaitChecks
zload /Tests/TaskIdle.ZC
zcall TaskIdleChecks
zload /Tests/TaskMessages.ZC
zcall TaskMessageChecks
zload /Tests/TaskJobs.ZC
zcall TaskExeChecks
zcall TaskExeLifecycleChecks
zcall TaskExeFocusChecks
zload /Tests/TaskMessageJobs.ZC
zcall TaskMessageJobsChecks
zload /Tests/TaskKeys.ZC
zcall TaskKeyChecks
zcall TaskFocusKillChecks
zload /Tests/TaskJobQueue.ZC
zcall TaskJobQueueChecks
zload /Tests/TaskSpawnQueue.ZC
zcall TaskSpawnQueueChecks
```

Expect each check to return `0x2a`. This validates declaration parsing,
`KernelA` layout checks, the compatible `MemCopy` binding, task-local math,
cooperative switching, default `Spawn`, suspend/kill lifecycle behavior,
task-pointer waits, idle/prompt readiness, cooperative message delivery, and
queued source execution through both `JobsHandler` and `MessageGet`, caller
wake, return of focus to an eligible master after a source job, one-shot
server exit, and CPU-0
function-call jobs and queued CPU-0 task creation. `KeyScan`/`KeyGet` and
key-masked `MessageScan`/`MessageGet` consume live input through task messages.
In the UTM app, `zcall TaskLiveKey` then type `k` in the framebuffer window;
expect `0x6b` in Terminal 1. Run `zcall TaskLiveMessage`, then type `m` in the
framebuffer; expect `0x6d`. The QEMU compatibility suites also check
`TaskLiveScan` with serial input. `TaskFocusKillChecks` should return `0x2a`.
Run `zcall TaskLiveFocus`, wait for `focus ready`, then type `f` in the
framebuffer; the focused child should wake and Terminal 1 should show `0x66`.
Source jobs still use the shared bootstrap
compiler.
Most x86-only `_intern` operations and unresolved `_extern`
symbols remain unavailable. Run `zreset` afterward before loading other
programs into a clean session.

**Status:** M33–M39 UTM freeze bar **accepted** (including `Notes.ZC` from PCI RedSea + `#include`). Pi remains a separate track ([PI4.md](PI4.md)).

**Live UTM check (2026-10-16):** UTM 5.0.7 reported the VM started and assigned
`/dev/ttys001` for that boot. The serial shell answered `zstatus`; the guest
reported `winmgr=1`, `gterm=1`, and a nonzero frame count. A later screenshot
confirmed physical host keyboard input in the graphical terminal: `zstatus`
was submitted, with 15 virtio events, 15 routed events, 8 graphical-terminal
key-downs, and zero dropped events. After refreshing the UTM disk, a host-input
check submitted `zls` and displayed all 164 live files, followed by
`zls: n=164 records=209 sectors=3880/65536` and a fresh prompt. GUI output now
uses a 32 KiB capture and formatter-safe chunks rather than truncating at 3.8
KiB. The QEMU QMP probe forms and submits `help`, `zls`, and an edited `zvol`,
then checks history recall. Re-query `utmctl attach` after each restart because
the pseudo-TTY path changes.

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
- M200: PL011 `RXE` enable + FIFO drain before shell (`pl011_rx_prep`); `uartrx` waits ~2s for one RX byte (Pi USB-TTL); check-serial `hc: uart RX idle OK`.
- M201: interactive shell idle uses `Sleep(1)` (peeks UART/virtio) instead of a busy-spin.
- M202: `shell_fb_ready` clears Lattice depth after demos; `uartrx` waits with `Sleep(1)` (not a hot spin).
- M203: StockLat `MESSAGE_NULL` idle case (DiskLat parity); `hc_run_src_ex` clears depth on exit; shell banner lists `uartrx`.
- M204: `PopUpColor` uses `"Mid…"` / `"Edge…"` header strings (g_hc_mem); Edge-first smoke → BLACK; Mid → YELLOW.
- M205: live `PopUpColor` paints the header first word above the Mid/Edge swatch (eyes-on `'c'` cue).
- M206: Mid/Edge header words persist across Refresh (`paint_swatch` / `paint_live`); cleared on Restart/`hc_run`.
- **Scope:** QEMU/`disklat`/`LatticePlay` exercise DiskLat-shaped compose source, **not** stock ZealOS `Demo/Graphics/Lattice.ZC` with original behavior. Live `stockplay` softens idle DrawIt (M193–M194) but still ≠ DiskLat/latticeplay. Place/aim/step/restart/colors need an eyes-on UTM pass after `make utm`.
- Freeze checklist still: `vblk`, `rspersist`, `rscatalog`, `runzc` UseAdd/Notes/MemSort (same as QEMU `run-pci`).

## Notes

- Case-insensitive names: `runzc notes.zc` is OK.
- Root dir has **8** slots; catalog is sized to fit with `.` / `Keep` / `Hi.ZC`.
- Catalog seed is **best-effort** (full root from an older UTM image no longer blocks `rsdir` / `runzc`).
- QEMU: `make run-serial` / `make run-pci`.
- Pi: `make pi-sd` → `build/pi-esp/` — do not install Pi-diag ELFs into UTM.

## Native compiler session (additional gate)

Keep the frozen checks above. After a compiler/runtime refresh, also run:

```text
zcheck
zc I64 Answer(){return 6*7;}
zcall Answer
zstatus
```

Expect `zc: upstream QuickSort + persistent modules OK`, result `0x2a`, and
`ready=1 failed=0`. The `zcheck` reset only discards the new compiler session;
it does not format RedSea. Native-session globals persist until reset/reboot,
not on disk. See [COMPATIBILITY.md](COMPATIBILITY.md).

2026-09-26: native `zcheck` passed in the refreshed UTM framebuffer window
(`ready=1 failed=0 modules=5`). Source-pin verification, ISO and PCI guest harnesses
passed. RedSea bytes were verified unchanged across the image refresh. Fast
computer-automation typing lost characters; individual keystrokes completed the
check. This entry does not renew the full mouse/Notes/graphics acceptance.

2026-10-17: the refreshed UTM graphical terminal passed the compiler-context
option/recovery smoke through desktop automation. Decimal-only mode persisted
between prompt commands and rejected a hex constant; disabling it let the same
function compile, and `zcall ContextAnswer` returned `0x000000000000002a`.
The option error and recovery message were separately visible, with
`ready=1 failed=0` and `compiler_recovery: errors=1 active=0` afterward.
USB keyboard/tablet ownership and released input capture were verified.
The final build passed 407 PCI, 325 ISO and 225 NEC two-hub/VirtIO graphical
workflow checks. The stopped-VM refresh preserved all 170 existing files
outside the four managed bridge/demo paths, deletion masks, RedSea bytes and
VM settings. See [COMPATIBILITY.md](COMPATIBILITY.md) for scope and evidence.
This does not establish full compiler/startup compatibility or Pi hardware
acceptance; physical typing/pointer/drag acceptance remains the earlier native
USB baseline.

2026-10-17: the refreshed UTM graphical terminal passed application-argument
execution through desktop automation. After `zload /Tests/RunFile.ZC`,
`zcall RunFileTaskChecks`, `zcall RunFileChecks` and
`zcall RunFileNestedChecks` each returned `0x000000000000002a`.
This covers terminal-owned child code, mixed integer/F64 register/stack
arguments, default flags, JUST_LOAD and nested/variadic execution. Typing and
the caret were visible before Return; status afterward showed
`ready=1 failed=0`, `compiler_recovery: errors=0 active=0`, terminal focus and
USB keyboard/tablet ownership, with input capture released.
The native implementation passed 419 full PCI and 337 full ISO checks;
the expanded NEC two-hub/VirtIO graphical workflow passed 250 checks and the
final focused gate passed 77. The final stopped-VM refresh preserved all 175
other files outside the managed bridge/demo paths, deletion masks, RedSea
bytes and VM settings. Only the newly installed acceptance fixture was
explicitly updated after verifying its prior checksum. See
[COMPATIBILITY.md](COMPATIBILITY.md) for scope, backups and screenshots.
Full compiler/startup compatibility, cross-owner code retention and Pi hardware
acceptance remain work; physical input acceptance is the earlier USB baseline.

2026-10-18: the user physically loaded `/Tests/SystemLaunch.ZC` and ran
`SystemLaunchPopupChecks` in the refreshed UTM graphical terminal. Typing
appeared and the result was `0x2a` (42), completing the live system/file/popup
launch acceptance described in [COMPATIBILITY.md](COMPATIBILITY.md).
This confirmation precedes the dedicated System-server update; that update
requires its own live check.
