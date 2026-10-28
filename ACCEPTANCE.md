# UTM acceptance (interactive) — **frozen virt regression bar (M33–M39)**

The UTM/QEMU ARM64 bring-up is active again. The M33–M39 table remains the
regression baseline; the desktop/startup/graphics and ISO-installer acceptance
gates below are being closed as this goal proceeds.

Automated QEMU/HVF covers boot + compiler smokes. **You** verify screen / keyboard / mouse
and the disk path below.

**Role:** UTM (`ZealosAarch64Hello`) is the live virt acceptance target. Re-run
the M33–M39 regression list after risky changes, then run the focused UTM gates
for the active bring-up.

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

The boot graphical terminal accepts ZealC directly through the original
`UserCmdLine` / `UserTaskCont` / `ExeCmdLine` path and the ARM64 compiler adapter. Click inside
it and type `40+2;`, then Return: the answer should be 42. `help` also lists
bootstrap commands such as `zstatus`, `zvol`, `zls`, `zload`, `zcall`, and `edit`.
`User;` opens another graphical terminal through the original task entry and
the default per-user `HomeSys` hook. Each terminal has its own input history,
document and compiler context. Boot also runs a guarded ARM64 translation of
`HomeSys.StartUpTasks`: it starts and tiles the second user terminal on disk
boot, queues `Home/Once.ZC`, initializes autocomplete, and opens the splash
where the boot-media path calls for it. Full native `KTask` startup and the
complete system menu/music path remain separate work.
The current installer keeps the
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

`make check-user-command` checks the original command loop, then inspects real
framebuffer glyphs for direct typing and output, a function entered on multiple
lines, persistent compiler options, Escape cancellation, and a usable prompt
after compiler and runtime errors. The input editor accepts 255 characters per
line and collects open delimiters, strings and comments into chunks up to 64 KiB.
Oversized input is discarded with a diagnostic. Use braces for multiline
control-flow bodies; this is not the complete upstream document-backed lexer.

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

## Native source-editor scanout gate

`zedit` is controlled from the built-in Terminal 1 serial shell while its
full-screen source view is drawn on the guest display. After `make utm`, enter
these commands one at a time in Terminal 1 and inspect the UTM display between
commands:

```text
zedit App/EditorScanout.ZC
a I64 EditorScanout(){return 40+2;}
wq
zload disk:App/EditorScanout.ZC
zcall EditorScanout
zedit App/EditorScanout.ZC
q
```

The first editor view should show its navigation/save controls. After `a`, the
source line and `[modified]` state should be visible; after `wq`, reopening the
file should show the same line and `[saved]`. `zcall` should return
`0x000000000000002a`. This eyes-on check complements
`make check-source-editor`, which verifies the same scanout and persistence
path in the disposable PCI guest.

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
- Live UTM check (2026-10-20/21): after restoring normal zoom, `latticeplay` responded to Space with a drawn step and Escape returned to the prompt. The user confirmed left-click placement, right-drag aiming, pointer-centered Control+wheel zoom, and that Space/Escape still work afterward. On 2026-10-21, the user also confirmed that `c` changes the color swatches and Return restarts the drawing/colors in UTM.
- Automated NEC/xHCI GUI regression (`make check-lattice-controls`, 2026-10-21): live pointer left-click places the head, right-drag changes its heading, `c` updates Mid/Edge swatches, Enter restores the initial pose/colors, widened Space leaves a visible stroke, and Escape restores the terminal. The boot menu now also provides bounded manual pan steps; QEMU verifies those actions and their panned framebuffer output, while live UTM confirmation of the new menu entries remains open.
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
- **Scope:** QEMU/`disklat`/`LatticePlay` exercise DiskLat-shaped compose source, **not** stock ZealOS `Demo/Graphics/Lattice.ZC` with original behavior. Live `stockplay` softens idle DrawIt (M193–M194) but still ≠ DiskLat/latticeplay. UTM has eyes-on place/aim, color/Return restart, Space/Escape, zoom, and plain-wheel behavior. Bounded directional pan is now in the OS menu and QEMU-tested; live UTM confirmation remains pending.
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

2026-10-18: dedicated System-server implementation `52b27ba` passed 461 PCI,
379 ISO, 251 graphics, 314 NEC two-hub/VirtIO graphical workflow, and 37 final
focused root/graphical checks. UTM was backed up, refreshed and restarted;
183 existing files outside six managed paths, deletion masks, RedSea and VM
settings were verified unchanged. The user's 08:27 screenshot confirms the
refreshed graphical terminal completed:

```text
zload /Tests/SystemServer.ZC
zcall SystemServerChecks
zcall SystemServerErrorChecks
```

`SystemServerChecks` returned `0x000000000000002a`. The user ran
`SystemServerErrorChecks` twice; each deliberately emitted one compiler
error, returned the same 42 and left a usable graphical prompt. Command text,
the insertion caret and guest pointer are visible. The screenshot is saved
as `build/system-server-utm-accepted.png`. This accepts the live System-server
and repeated error-recovery checks. Status diagnostics and pointer motion
retain their automated/earlier physical acceptance; no new `zstatus` output
or pointer-motion test was supplied here. Backup and scope details are in
[COMPATIBILITY.md](COMPATIBILITY.md).

2026-10-18: the original `UserTaskCont` / `ExeCmdLine` command-loop adapter
passed 219 PCI graphical workflow checks, 379 ISO checks and 39 native USB
command-loop checks through two hubs. Actual scanout confirms visible direct
ZealC input/output, multiline functions, float answers, compiler options shared
with legacy `zc`, Escape cancellation and recovery after errors. The editor
save/compile/run/reboot/reopen gate also passes. The user physically typed
`40+2;` without a `zc` prefix in the refreshed UTM graphical terminal and
confirmed that the characters appeared and the result was 42. This accepts
the live interactive command-loop milestone; full `HomeSys` startup
and the remaining compiler/document interfaces are still separate work.

The VM is backed up and refreshed from `440631e`; CLI reports it started.
The refresh preserved 185 existing files outside the managed paths, deletion
mask, RedSea bytes and VM settings, and verified the boot payload against the
tested image. The user's subsequent physical typing check completed the live
gate. Pointer motion and drag retain their earlier acceptance.

2026-10-18: original `User` / `UserCmdLine` entry and the per-user `HomeSys`
hook passed 42 focused PCI and 45 native USB startup checks, plus 379 ISO,
219 graphical workflow and 36 command-loop regression checks. Two graphical
terminals show independent visible typing, carets, answers and histories;
repeated creation/teardown preserves managed memory usage. This milestone also
fixes input starvation when a compositor frame misses its sleep deadline.
UTM was backed up, refreshed from `0034e51` and restarted. Its installed kernel
matches the tested build; 184 existing files outside the managed paths,
deletion masks, RedSea bytes and VM settings were preserved. Live desktop
automation opened another terminal with `User;` and visibly obtained
`ans=0x2A=42` from `40+2;`. A physical typing/release check remains pending
after later automation repeated a modifier key while arranging the window.
Full `HomeSys` desktop startup remains separate work.

2026-10-18: formatted `User` startup commands and `XTalk` input delivery passed
31 PCI and 34 native USB checks, with visible script output, subsequent device
typing and exact terminal/filter reclamation. The receiving terminal owns the
definitions and executes queued includes and commands after parser/runtime
errors. A malformed-expression compiler fault found by this gate is fixed by
the documented Aiwnios parser patch. Dedicated compiler recovery (67), ISO
compatibility (380) and graphical save/compile/run/reboot/reopen workflow (220)
checks pass. The running UTM bundle remains on `0034e51` for its pending physical
typing check; these changes have been verified in disposable guests.


2026-10-18: shared home paths now pass 66 PCI and 69 native USB checks,
including visible typing/caret, System-owned HomeSet lifetime, unsupported-path
recovery without leaks, home file execution and reopening the saved source
after reboot. The USB shape retains duplicate VirtIO devices and two hubs.
The ISO compatibility gate passed 384 checks, a focused ISO home gate passed
51, XTalk regression passed 31, and the graphical editor workflow passed 225
with save/compile/run/reboot/reopen. Pinned source hashes, HomeSet extraction
guards, bundle seed integrity, Python/shell syntax and whitespace checks pass.
Default home is `/Home`; an existing customized `Home/Once.ZC` is preserved on
refresh. Registry-backed Once execution and full HomeSys startup remain pending.
The current UTM session was left unchanged on `0034e51`; these results are from
disposable guests and do not claim a new physical UTM acceptance.

2026-10-18: the System-owned registry, original document-tree APIs and
`ExeDoc` character-reader adapter passed 47 PCI and 47 native USB logged
checks, including error recovery, killed-child cleanup, exact managed-memory
reclamation and saved Once/System + Once/User execution after reboot. The
read-only ISO gate passed 30, full ISO compatibility passed 384, and graphical
save/compile/run/reboot/reopen passed 225. Explicit key-edge tests passed 16
PCI and 19 native USB checks, including held Shift+9, release, Escape and a
subsequent visible `42;` answer with idle modifiers/repeat state.

The user's physical input report included wrong symbols and missing typing;
the live pre-refresh check subsequently entered visible `42;` and `zstatus`
successfully and restored released-pointer mode. This records recovery, not a
confirmed fix for intermittent physical input. A QMP injection failure in a
disposable guest is addressed by explicit key edges in the test harness.
The guest USB driver remains unchanged; `zstatus` gains keyboard-state
diagnostics. UTM has been backed up and refreshed with the tested kernel,
preserving 185 existing files outside managed paths, deletion masks, RedSea and
VM settings. First-stage `HomeSys.StartUpTasks`, automatic home `Once` loading,
tips, autocomplete initialization, and the boot-media splash are now part of
the tested boot path. Full native `KTask` startup and the complete system
menu/music path remain open.

The refreshed live graphical window also passed desktop checks for visible
`help`, help execution, visible `42;` before Return and `ans=0x2A=42` afterward,
with UTM capture released. The user then physically typed `help` without first
tapping Shift and confirmed normal lowercase typing and successful execution.
This completes the refreshed-session physical typing gate.

2026-10-21: the live source Window Manager had been drawing task-window
`GrPlot`/`GrRect` output straight to scanout, where staged RGBA presentation
could overwrite it. These primitives now draw into the indexed compositor
bitmap and apply current window-cell visibility. `make check-window-buttons`
passes the stable overlap-pixel assertion across repeated frames, and
`make check-lattice-controls` still passes with native NEC USB keyboard, mouse,
tablet, Space and Escape. The `ZealosAarch64Hello` UTM disk was refreshed from
this tested build; the installer verified the EFI image and preserved the
RedSea/source partitions and VM settings. The full pre-refresh backup is at
`build/utm-before-20261021-054447`. The user subsequently confirmed the window
control's plain-wheel swatch on UTM. After restoring normal zoom with Control+scroll,
`latticeplay` also drew with Space and returned to the prompt with Escape. These
close the plain-wheel and live Lattice keyboard-control checks. The user then
confirmed Control+scroll zoom and pointer-edge pan both work; zoom remains active
while the desktop recenters at the edge. The ARM64 GrScreen regression now checks
that invariant directly.

2026-10-21 UTM graphics follow-up: the OS menu's View Angles control appeared,
and the user confirmed that its X, Y, and Z tracks each respond. In the refreshed
guest, `GrBlot3Checks`, `GrSpriteTransformChecks`, and
`GrSpriteSymmetryChecks` each returned `0x2a`. `make check-grscreen-full` also
passed the QEMU source-backed graphics suite, including zoomed framebuffer
presentation and the three-axis control checks. UTM pan beyond pointer-edge
recentering, zoom-cache presentation, and the full normal desktop/menu startup
remain open.

## ARM64 ISO installer milestone — 2026-10-22

The ISO now carries a 5.2 MiB sparse representation of the normal 128 MiB
UTM/QEMU disk, rather than embedding the whole disk and exhausting the 512 MiB
guest's boot memory. The packer/round-trip check reconstructs the disk
byte-for-byte across its 25 nonzero extents. The installer accepts only one
128 MiB PCI VirtIO target, shows a no-write readiness prompt, and requires the
explicit `zinstall ERASE` command. It zeroes the full disk, writes the image,
flushes, and compares every sector on read-back.

`make check-dvd-live` passed the 512 MiB ISO boot, graphical startup, visible
installer instructions, and interactive help. `make check-iso-installer`
passed on QEMU/HVF: the blank disposable disk was installed and read back
byte-for-byte, booted by itself without the ISO, and started the graphical
desktop. A source file saved on that installed disk returned 42, then returned
42 again after a second reboot. Logs: `build/check-iso-installer-live.log`,
`build/check-iso-installer-disk-boot.log`, and
`build/check-iso-installer-reboot.log`.

For a manual UTM install, attach the ISO itself to the VM's SCSI CD/DVD drive
and put that drive above the VirtIO target in the Drives list; UTM uses list
order as boot priority. The purple highlight only selects a device for editing.
After installation, shut down and boot the VirtIO target first, with the CD/DVD
below it or detached.

2026-10-22 live UTM acceptance used the isolated
`ZealOS AArch64 ISO Installer Acceptance` clone and its blank 128 MiB VirtIO
disk. The corrected ISO booted as a read-only USB CD-ROM; the default
`zinstall` command made no writes and displayed the explicit `zinstall ERASE`
gate. The confirmed install erased, wrote, flushed and read back every sector.
The installed disk matched that build's `build/disk.img` byte-for-byte
(`sha256=7499fc5c394efd624eef892c479c6af7ede0a2aa6e61a7733f122816da83c4dc`).
With the target disk at QEMU boot index 0 and the installer CD at index 1, a
UTM cold boot started the installed desktop: `zstatus` reported
`ready=1 failed=0`, `compiler_recovery: errors=0 active=0`,
`winmgr=1 gterm=1`, two xHCI controllers running with zero errors, and
nonblack scanout pixels. `Tests/InstallerPersist.ZC`, saved to the installed
disk, returned `0x2a` before and after the cold reboot. The imported ISO
remained attached as a lower-priority fallback during that reboot; disk-only
boot is independently covered by the QEMU/HVF test above. The exact tested ISO
is `build/zealos-aarch64-hello.iso` (SHA-256
`330ab43890bd30be51376b73b86b8230f4698359c2745ce9a80fe22c4875e63e`).

The first live UTM installer milestone is closed for the ISO above. After
adding bounded Zoom In/Out, Reset View, and Pan Left/Right/Up/Down actions to
the boot OS menu, the graphics gate passed again: `make check-grscreen-full`
returned exit code 0, including the menu action bounds smoke and source
compositor zoom plus non-origin pan presentation on the Limine framebuffer.
The refreshed current ISO then passed `make check-iso-installer` on QEMU/HVF:
all 25 sparse extents round-tripped; `zinstall` refused to write without
confirmation; `zinstall ERASE` installed the image; the installed target
matched `build/disk.img` byte-for-byte (`sha256=de6fcd2f9a51a37e25571678da7abef55ae30f6942cc224cbd9d5a9d9f9030b2`);
it booted and returned 42 from `InstallerPersist` before and after another
reboot. Current artifact hashes: ISO
`2e9f64e38512a302a912ff7f84454893c6889b084415ad7d33e25439279d8ca4`, disk
`de6fcd2f9a51a37e25571678da7abef55ae30f6942cc224cbd9d5a9d9f9030b2`.

The exact current ISO was then installed in the isolated UTM acceptance clone
`ZealOS AArch64 ISO Installer Acceptance`. Its decoded USB-CD image matched
`build/zealos-aarch64-hello.iso` byte-for-byte at SHA-256
`2e9f64e38512a302a912ff7f84454893c6889b084415ad7d33e25439279d8ca4`; the
target was confirmed all-zero and 128 MiB before installation. UTM live boot
reported `ready=1 failed=0`, `compiler_recovery: errors=0 active=0`,
`winmgr=1 gterm=1`, two running xHCI controllers with zero errors, and
nonblack scanout. The default `zinstall` wrote nothing and displayed the
confirmation gate. `zinstall ERASE` installed and read back the image; before
saving test data the UTM target matched the build disk byte-for-byte at SHA-256
`de6fcd2f9a51a37e25571678da7abef55ae30f6942cc224cbd9d5a9d9f9030b2`. The
saved `Tests/InstallerPersist.ZC` function returned `0x2a`, then after a cold
UTM stop/start boot with the VirtIO target at boot index 0 and ISO at index 1,
the desktop again reported clean compiler recovery, two clean controllers and
nonblack scanout; loading the saved file from `disk:` again returned `0x2a`.
That reboot's serial transcript also shows the disk-startup `Once` command,
autocomplete initialization, splash launch, and return to the shell. In the
same refreshed UTM guest, `zcall BootstrapGuiTerminalZoomPanChecks` and
`zcall BootstrapGuiTerminalSmoke` both returned `0x2a`, covering the menu
action bridge, zoom changes, bounded pan steps, command editor, and current
terminal/window state. A following `zstatus` still reported
`compiler_recovery: errors=0 active=0`, `winmgr=1 gterm=1`, the System task
running, two clean xHCI controllers, and nonblack scanout.
The graphical UTM window could not be inspected because the Mac session was
locked; the serial status and guest framebuffer diagnostics were available.
The live installer cycle is accepted on that ISO. At that iteration, a direct
run of the framebuffer test source inside UTM first needed the `ODEsUpdateStub`
fixture and source modules loaded in the same order as the QEMU harness; it was
not counted until that setup was corrected below. The VM was cold-booted again,
and the final guest status returned to `errors=0` with the saved function still
returning 42.

2026-10-22 full-source compositor and ISO acceptance: the UTM-only
`GrScreenFinalUpdateChecks` exposed that the live full-source path called
`ZcFbShellTextCompose` without advancing the shared text-update diagnostic.
The full compositor now records that composition, and a source guard keeps the
counter tied to the real shell-text call. `make check-grscreen-full` and
`make check-iso-installer` both pass. The exact artifact hashes are ISO
`1d5feeddd699f077b499703f3ea98f91c16e2ca747f1fa9ce83ca45757c17de5` and
installer disk
`5964ea3a0a151f903a95bf26283feb9e8e1749e0e24ae021b9e6356331c4bbcb`.

The isolated UTM acceptance VM was refreshed with that exact ISO; the decoded
USB-CD image matched byte-for-byte. Its disposable 128 MiB target was backed
up, confirmed all-zero, and installed only after the no-write `zinstall` gate
showed the explicit `zinstall ERASE` confirmation. The confirmed install
read back every sector, and its pre-test target matched `build/disk.img`
byte-for-byte at the hash above. `Tests/InstallerPersist.ZC` returned `0x2a`
before and after a cold UTM stop/start with the target first in boot order.
After reboot, the same guest loaded the full graphics fixtures in the QEMU
harness order: `ODEsUpdateStubCheck`, text background/foreground, screen update,
zoom, `GrScreenFinalUpdateChecks`, and ViewAngles checks all returned `0x2a`.
`BootstrapGuiTerminalZoomPanChecks` and `BootstrapGuiTerminalSmoke` also
returned `0x2a`. Final `zstatus` reported `ready=1 failed=0`,
`compiler_recovery: errors=0 active=0`, `winmgr=1 gterm=1`, a running System
task, two xHCI controllers with zero errors, and nonblack scanout. The Mac
session is locked, so visual confirmation of the added OS-menu entries remains
pending. Normal upstream menu/task startup, wallpaper, and broader CDC/3D
graphics work also remain open; the ISO/installed-disk acceptance is complete
for this artifact, while the larger UTM bring-up goal remains active.

2026-10-22 wallpaper layer and final UTM installer retest: the source
compositor now applies a restrained three-color pattern through packed text
background attributes on uncovered desktop cells. Window-owned cells, glyphs,
and foreground colors are preserved. `BootstrapWallpaperChecks` exercises the
actual text-plane application and confirms a covered cell remains unchanged;
`make check-grscreen-full` passes with `GrScreenFinalUpdateChecks` returning
`0x2a`.

The exact rebuilt ISO (`sha256=fb940e7030def43d02b67b15139f0d2c137a90c6a8bf2ac74018442c6beb8384`)
passes `make check-iso-installer`; its 25 sparse extents round-trip, the
unconfirmed install remains write-free, the confirmed install reads back every
sector, and the disk-only QEMU reboot starts the desktop and reopens the saved
test function as 42. The corresponding build disk hash is
`37302a23fdddb8d71cb9958b232129f0ad86ae89be176c2c575752a0fe6835c0`.

The same ISO was booted in the isolated UTM VM `ZealOS AArch64 Wallpaper
Acceptance` from its USB CD drive. Serial boot output contains the compiler,
VirtIO block, source Window Manager, and graphical terminal startup markers.
`zinstall` displayed the explicit `ERASE` gate; `zinstall ERASE` completed its
full target read-back. The installed 128 MiB VirtIO target was then copied into
the separate `ZealOS AArch64 Installed Disk Acceptance` VM with no ISO drive
attached. That disk-only VM cold-booted the desktop with
`compiler_recovery: errors=0 active=0`, `winmgr=1 gterm=1`, and nonzero
presented frames. `Tests/InstallerPersist.ZC` returned `0x2a` before and after
a second cold UTM stop/start. Serial transcripts are
`build/utm-wallpaper-serial-install-final.log`,
`build/utm-wallpaper-disk-boot.log`, and
`build/utm-wallpaper-disk-reboot.log`.

The UTM graphics run was performed while the Mac session was locked, so the
wallpaper's visible appearance still needs an eyes-on check. The wallpaper
code path and installer acceptance are implemented and automated; the full
upstream menu/task startup, broader CDC operations, and complete 3D graphics
remain open bring-up work.

The same `make check-grscreen-full` run exercises the bounded transformed-mesh
projective path: homogeneous perspective division, clipping through the near
plane and each viewport edge before integer projection, plus a five-vertex
polygon clipped on multiple planes and fan-rasterized. Perspective behavior
for other sprite opcodes and pixel-identical upstream mesh parity remain open.

2026-10-22 projective-line graphics fix and current-artifact UTM retest: the
default non-affine `SPT_LINE` path now clips both homogeneous endpoints against
the near plane and four CDC viewport edges before integer projection. The
regression includes an endpoint at -5,000,000 source pixels, verifies the
clipped projected segment, and checks actual framebuffer pixels; affine and
custom transform callbacks keep their previous behavior. `make
check-grscreen-full` passes. In the live UTM guest,
`GrSpriteSymmetryChecks` also returned `0x2a` on the same line-clipping path.

The rebuilt ISO SHA-256 is
`860b7f0ceb8651edfbac794013971ce8f704461e587019433132fb15968beae3`; the build
disk hash is `596dabe04abc242fd63caa952bfcaaaadd29cb8dd6f90ee10f822c7c22588b35`.
`make check-iso-installer` passed: the ISO booted, required the explicit erase
confirmation, installed all 128 MiB, and read back every sector. In UTM, that
same ISO was copied into the USB-CD installer VM, booted with a fresh zeroed
VirtIO target, and installed. After UTM's full readback completed, the target
file compared byte-for-byte with `build/disk.img` at the hash above. The exact
target was copied into the separate disk-only VM. Its cold boot reported
`compiler_recovery: errors=0 active=0`, `winmgr=1 gterm=1`, and nonblack scanout;
`Tests/InstallerPersist.ZC` returned `0x2a` before and after another cold
stop/start. Transcripts are `build/utm-projective-line-serial-status.log`,
`build/utm-projective-line-disk-boot.log`,
`build/utm-projective-line-disk-persist-before.log`,
`build/utm-projective-line-disk-reboot.log`, and
`build/utm-projective-line-graphics-check.log`.

The Mac remained locked during this run, so the wallpaper's visual appearance
still needs eyes-on UTM confirmation. The projective line slice and current ISO
installer cycle are accepted; normal upstream menu/task startup, broader CDC
operations, other projective sprite opcodes, and pixel-identical mesh parity
remain open bring-up work.

2026-10-22 projective polyline clipping and refreshed ISO/UTM install: the
default non-affine `SPT_POLYLINE` path now clips each segment in homogeneous
space against the near plane and four CDC edges before integer projection.
`GrSpriteSymmetryChecks` sends a five-million-pixel off-screen endpoint into a
visible horizontal segment followed by a visible vertical segment, then checks
the actual scanout pixels and clipped-off area. `make check-grscreen-full`
passes, and the same test returns `0x2a` in the refreshed UTM installed-disk
guest.

The rebuilt ISO has SHA-256
`ee7620a224988b9d69d024648c0268050e1b6184ceae7c93c94a800fc07849f1`; the
build disk hash is
`2764110dbae5630c65c6bbe00048de67854c383f0787e9fede3b068484728aa1`.
`make check-iso-installer` passed the 25-extent payload round-trip, write-free
confirmation gate, complete sector readback, installed-disk cold boot, and
saved-source return value `42` before and after reboot.

The exact ISO was copied to the isolated UTM USB-CD installer VM with a fresh
zeroed 128 MiB VirtIO target. UTM boot reported `ready=1 failed=0`, clean
compiler recovery, `winmgr=1 gterm=1`, and nonblack scanout. `zinstall` showed
the explicit `ERASE` gate; `zinstall ERASE` completed its full readback. The
installer guest then created its normal `Home/Registry.ZC` record, so the
stopped target includes that post-install startup write and is not a pristine
byte-for-byte copy of `build/disk.img`. The target was copied into the separate
disk-only UTM VM with no CD attached. After a cold boot and another cold
stop/start, `Tests/InstallerPersist.ZC` returned `0x2a`; the live UTM
`GrSpriteSymmetryChecks` also returned `0x2a` for projective polyline clipping.
Serial transcripts are `build/utm-polyline-serial-install.log`,
`build/utm-polyline-disk-boot.log`, and
`build/utm-polyline-disk-reboot.log`.

The Mac session remains locked, so the wallpaper and final desktop appearance
still need eyes-on confirmation. Normal upstream menu/task startup, broader
CDC operations, projective handling for splines and other sprite opcodes, and
pixel-identical mesh parity remain open; the ISO installer, disk-only desktop
boot, and saved-data path are accepted for this artifact.

2026-10-22 projective rectangle clipping and refreshed ISO/UTM install: the
default non-affine `SPT_RECT` path now clips both triangle faces and all four
outline segments in homogeneous space against the near plane and CDC viewport
before bounded projection. Its regression sends a rectangle five million
source pixels beyond the left edge, then checks the clipped filled CDC pixel,
visible scanout outline, and off-viewport area. `make check-grscreen-full`
passes, and the refreshed disk-only UTM guest returns `0x2a` from
`GrSpriteSymmetryChecks` after a second cold stop/start.

The current ISO SHA-256 is
`d75b43fed2c2d7a0286e0fa811ef7cf3d7460cdc73b3512f4ec6392215b18087`; the
build disk hash is
`bc35220aa62f1f374e784fc4587d73e76aba3c7dedbf88af8bbf45b7e3e7a726`.
`make check-iso-installer` passed payload round-trip, the write-free
confirmation gate, complete sector readback, disk-only desktop boot, and
saved-source return value `42` before and after reboot. In UTM, this exact ISO
booted with a fresh zeroed 128 MiB VirtIO target. `zinstall` required the
explicit `ERASE` token and then read back the full target. The installer made
its normal `Home/Registry.ZC` first-startup write, so the stopped target
contains that record in addition to the ISO image. After copying it into the
separate disk-only VM, two cold starts reported `ready=1 failed=0`,
`compiler_recovery: errors=0 active=0`, `winmgr=1 gterm=1`, and nonblack
scanout; `Tests/InstallerPersist.ZC` and `GrSpriteSymmetryChecks` each returned
`0x2a` after the second restart. Transcripts are
`build/utm-rect-serial-install.log`, `build/utm-rect-disk-boot.log`, and
`build/utm-rect-disk-reboot.log`.

The Mac is still locked, so visual confirmation of the wallpaper and final
desktop appearance is pending. Projective rectangle and polygon clipping and
this ISO installer cycle are accepted; upstream menu/task startup, broader CDC
operations, projective spline and other sprite paths, and pixel-identical mesh
parity remain open.

2026-10-22 projective polygon clipping and refreshed ISO/UTM install: the
default non-affine `SPT_POLYGON` path now clips each outline edge and fill
triangle in homogeneous space before integer projection. Its regression sends
a regular polygon through both the `w=0` near plane and the CDC viewport,
checks the visible clipped edge at the center column, and verifies a
known-empty pixel. `make check-grscreen-full` passes, and the same
`GrSpriteSymmetryChecks` returns `0x2a` from a disk-only UTM guest after a
cold restart.

The refreshed ISO SHA-256 is
`e3a8915af37e92664a7efd17667150ec9dc860804f6582e0df8e938c1826d417`; the
build disk hash is
`a8639a1a6ea5753ecd385dd3bf10031878e7233cca2bf35a6ebb83c0f7da3238`.
`make check-iso-installer` passed the live ISO boot, guarded erase, complete
readback, installed-disk boot, and a second cold reboot with
`Tests/InstallerPersist.ZC` returning `0x2a`. In UTM, that exact ISO booted in
the isolated installer VM, `zinstall ERASE` completed and read back the full
128 MiB target, and the copied disk-only VM reported `ready=1 failed=0`,
`winmgr=1 gterm=1`, and nonblack scanout on two cold starts. The saved-data
check and projective `GrSpriteSymmetryChecks` both returned `0x2a` after the
second start. Transcripts are `build/utm-polygon-serial-install.log`,
`build/utm-polygon-disk-boot.log`, and
`build/utm-polygon-disk-reboot.log`.

2026-10-22 projective spline clipping and current-ISO UTM cycle: default
projective quadratic/cubic B-splines now stay in source space while each
sampled curve segment is clipped through the homogeneous near plane and CDC
viewport before rasterization. `GrSpriteSymmetryChecks` verifies the projected
curve in scanout both through direct `SpriteBSpline0` use and the public
`Sprite3` record path. `make check-grscreen-full` passes. The refreshed ISO
SHA-256 is `6e190f916007ffcd32d2aca643b6afb9dbccb737270fd820ecbe2c017716b550`;
`make check-iso-installer` passes live boot, guarded erase, full-sector
readback, disk-only reboot, and saved-data return value `42` before and after
a cold restart.

That exact ISO booted in the isolated UTM installer VM with a fresh zeroed
128 MiB VirtIO target; `zinstall ERASE` completed its full readback. The
installed target was copied to the separate disk-only UTM VM. Both cold boots
reported `ready=1 failed=0`, `compiler_recovery: errors=0 active=0`,
`winmgr=1 gterm=1`, and nonblack scanout. The saved-source function and
`GrSpriteSymmetryChecks` both returned `0x2a` on the first boot and after a
second cold stop/start. Transcripts are
`build/utm-spline-serial-install.log`, `build/utm-spline-disk-boot.log`, and
`build/utm-spline-disk-reboot.log`.

The Mac remains locked during this automated run, so visual confirmation of
the wallpaper and final desktop appearance is still pending. Projective
splines are now accepted; projective clipping for other sprite paths, broader
CDC operations, normal upstream menu/task startup, and pixel-identical mesh
parity remain open.

2026-10-22 projective ellipse/circle clipping and refreshed ISO/UTM install:
default projective `SPT_ELLIPSE` and `SPT_CIRCLE` now sample source-space
outlines and clip each chord through the homogeneous near plane and CDC
viewport before rasterization. `GrSpriteSymmetryChecks` exercises an ellipse
and a circle crossing the near plane and checks their visible scanout pixels.
`make check-grscreen-full` passes.

The rebuilt ISO SHA-256 is
`974532ae9d4f80224cfdeb7a5faf35568edc6f75f2374a55fc7837dd2f977588`; the
build-disk SHA-256 is
`6007b1f5318ce9cb53ec90476ffbd538c3a294d5fcf6eec828612b7396ceba34`.
`make check-iso-installer` passed the guarded erase, whole-disk readback,
disk-only desktop boot, and persistent return value `42` before and after a
cold reboot. The exact ISO was installed in the isolated UTM installer VM;
`zinstall ERASE` completed full target readback. The copied disk-only VM
reported `ready=1 failed=0`, clean compiler recovery, `winmgr=1 gterm=1`, and
nonblack scanout on two cold starts. Both the saved-data function and
`GrSpriteSymmetryChecks` returned `0x2a` after reboot. Transcripts are
`build/utm-ellipse-serial-install.log`, `build/utm-ellipse-disk-boot.log`, and
`build/utm-ellipse-disk-reboot.log`.

The Mac is locked, so final desktop and wallpaper appearance still needs
eyes-on confirmation. Projective line, polyline, rectangle, polygon, spline,
ellipse, and circle paths now pass; other transformed sprite paths, broader
CDC operations, normal upstream menu/task startup, and pixel-identical mesh
parity remain open.

2026-10-22 projective arrow clipping and refreshed ISO/UTM install: default
projective `SPT_ARROW` now clips its shaft against the homogeneous near plane
and viewport before drawing. It draws an arrowhead only when the true endpoint
remains visible, instead of moving the head to a clipped viewport edge. The
regression starts the shaft behind the near plane, checks the clipped endpoint,
and counts the full visible raster in scanout. `make check-grscreen-full`
passes with `GrSpriteSymmetryChecks=0x2a`.

The refreshed ISO SHA-256 is
`8a705d3df5dc9a39146e7ce3fc226b4ab6b89aa4d054ca78b63aec160dfce16a`; the
build-disk SHA-256 is
`d682f9148d4f87ac4e2875356fd855fa7978b49a2a5de32587dbbda666143693`.
`make check-iso-installer` passed the confirmation gate, full target readback,
disk-only desktop boot, and saved return value `42` before and after reboot.
The exact ISO then booted in the isolated UTM installer VM, erased and verified
the disposable 128 MiB target, and was copied byte-for-byte to the separate
disk-only VM. Both cold starts reported a healthy desktop and nonblack scanout;
the saved function and projective graphics suite returned `0x2a`, including
after a second cold restart. Transcripts are
`build/utm-arrow-serial-install.log`, `build/utm-arrow-disk-boot.log`, and
`build/utm-arrow-disk-reboot.log`.

The Mac is still locked, so wallpaper and final desktop appearance need
eyes-on confirmation. Projective line, polyline, rectangle, polygon, spline,
ellipse, circle, and arrow paths now pass; transformed rotated rectangles and
other sprite opcodes, broader CDC operations, normal upstream menu/task
startup, and pixel-identical mesh parity remain open.

2026-10-22 rotated-rectangle clipping and current UTM disk-only acceptance:
default projective `SPT_ROTATED_RECT` now transforms its four source corners,
clips the resulting fill triangles and all four outline segments against the
near plane and CDC viewport, and preflights bounded draw work. The regression
uses a non-cardinal angle with one edge outside the left viewport; direct
helper and `Sprite3` paths both draw visible body and scanout pixels. The full
`make check-grscreen-full` suite passes.

The current ISO SHA-256 is
`211cb160efbcfb9841397f0cb212401e657a6e20a9061660819cdb9d4e5d3c88`; its
build-disk SHA-256 is
`d3b2f7775ceec9820c9a4bb20002eab0954d34f3b5d6fba9e81c2fe0e35b4f5c`.
`make check-iso-installer` passed the explicit erase gate, full 128 MiB
readback, byte-for-byte image check, disk-only boot, and saved return value 42
before and after reboot. The exact ISO was also installed through the isolated
UTM installer VM into its disposable 128 MiB VirtIO target. After copying that
target to the separate disk-only UTM VM, two cold starts reported
`ready=1 failed=0`, `compiler_recovery: errors=0 active=0`, `winmgr=1 gterm=1`,
two clean xHCI controllers, and active nonblack framebuffer scanout. A
`Tests/InstallerPersist.ZC` function saved on the installed volume returned
`0x2a` before and after a cold UTM stop/start. The rotated-sprite
`GrSpriteSymmetryChecks` returned `0x2a` both before and after that restart.

Visual wallpaper acceptance is still pending: the UTM console capture in this
session showed a blank gray canvas despite the guest's active-frame and
nonblack-scanout diagnostics, so the actual wallpaper colors and desktop
layout were not visually established. The OS menu's expanded zoom/pan entries
have bootstrap/QEMU bounds checks; clicking those new entries live also
remains open. Broader CDC operations, normal upstream menu/task startup, and
pixel-identical transformed-mesh parity remain open.

2026-10-22 current ISO installed through UTM: the installer VM was saved with
the USB CD ahead of the target VirtIO disk, and its launch arguments confirmed
the ISO as the boot device. The ISO SHA-256 is
`d9e2c5d5aa64c54bdbe259f286c5c1560e141b7db0495a69a97f42ffc3da858b`; the
128 MiB installed-disk SHA-256 is
`676d51daebb9b5839188e30a24fe5c594a563a7a6b3c1c699c845cbd1ffdec86`.
`zinstall ERASE` completed the UTM full-sector read-back. The disk was copied
byte-for-byte to the disk-only acceptance VM, whose two cold starts reported
`ready=1 failed=0`, `compiler_recovery: errors=0 active=0`, `winmgr=1 gterm=1`,
and nonblack scanout. `Tests/InstallerPersist.ZC` returned `0x2a` before and
after restart; the expanded projective `GrSpriteSymmetryChecks`, including
bitmap and glyph samples, also returned `0x2a` after restart. The live UTM
display capture remains gray while serial reports nonblack scanout, so visual
wallpaper colors and desktop layout are not yet accepted.

2026-10-22 centered splash sprite and refreshed UTM installer acceptance:
`SpriteHLine0` now sends filled screen-bitmap scanlines through the task-aware
plot path, applying the task's screen origin, scroll offset, clip rectangle,
and Z order. This fixes the splash Z being drawn away from its centered logo
area. `scripts/check-compat.py` checks that blue mesh pixels appear between
the splash labels during animation. A refreshed UTM screenshot shows the
rotating Z centered between “ZealOS ARM64” and “Modern Innovation,” with both
desktop terminals visible.

The refreshed ISO SHA-256 is
`e0545f0a9d2fda8f872c90ab91c9bf46f465db2ecf0a7cc2e4294a752a9ca4e7`; the
installed build-disk SHA-256 is
`88a12f6d8dfb94d9457b874e3cb450947c0d19087f1546a94c8e71f70a24479e`.
`make check-home-autostart`, `make check-grscreen-full`, and
`make check-iso-installer` pass. The graphical startup test observed 2,006 blue
mesh pixels in the center logo area in both the boot splash and splash-window
animation, then confirmed the splash graphic cleared on dismissal. The exact
ISO booted in the isolated UTM installer, required `zinstall ERASE`, and completed
full 128 MiB read-back verification. Its byte-identical disk-only UTM copy
cold-booted with the desktop; `Tests/InstallerPersist.ZC` and
`GrSpriteSymmetryChecks` both returned `0x2a` after cold restart. The post-boot
serial transcript is `build/utm-splash-disk-reboot.log`; installer read-back
is recorded in `build/utm-splash-serial-install.log`.

The centered splash and current ISO installer cycle are accepted. The broader
bring-up remains active: normal upstream menu/task startup, remaining CDC and
transformed-sprite paths, expanded OS-menu zoom/pan interaction, and
pixel-identical transformed-mesh parity remain open.

2026-10-22 live UTM splash review: with the installed-disk acceptance VM
unlocked and visible, the UTM screenshot confirms the animated blue Z rotates
in the center of the splash between its two labels, while both graphical
terminals remain visible. The splash alignment is visually accepted. The live
OS-menu zoom/pan check remains open: the UI bridge reached the running guest,
but did not expose the dropdown for item selection. The QEMU menu-action and
bounded-pan regression remains green.

2026-10-22 current-artifact UTM disk-only cold boot: ISO SHA-256
`f81ea919301d9e57e5171b354dc5078ef15b31e19660fae7cf5fce71c9070479`; installed
128 MiB image SHA-256
`c04c141bec750201f3265bd0abd9c05d81472aa8688b84f2dd78ea26af5d010c`. The
exact ISO passed `make check-iso-installer`, including the explicit erase gate,
full read-back, and byte-identical installed-image comparison. In UTM, the
acceptance VM was restarted with only its VirtIO target attached at boot index
0. The QEMU launch record contains the target disk and no optical drive. The
cold boot reached the graphical desktop (`ready=1 failed=0`, 72 modules,
`winmgr=1 gterm=1`), displayed both terminal windows and the centered animated
splash Z, and loaded `disk:Tests/InstallerPersist.ZC`; `InstallerPersist`
returned `0x2a`. Running `zinstall` on this disk-only boot reported that the
installer image is unavailable, confirming it did not boot from the ISO. The
serial transcript is `build/utm-current-disk-coldboot.log`. The live UTM menu
zoom/pan click acceptance remains open; headless menu action and pan tests pass.

2026-10-22 live UTM zoom/pan menu acceptance: the user selected Zoom In, Pan
Right, and Reset View in the installed-disk guest and confirmed the actions
visibly worked. This closes the remaining live interaction check for the
expanded menu; zoom/pan regressions also pass in QEMU. Startup beyond the
current staged desktop, broader CDC behavior, and pixel-identical transformed
mesh parity remain open.

2026-10-22 Paint Demo menu and rebuilt ISO acceptance: the OS menu now has a
Paint Demo item, and the graphical shell exposes the existing bounded `paint`
program. `BootstrapGuiTerminalSmoke` checks the label/action wiring and runs
the program through the menu, confirming it returns a dot count.
`make check-grscreen-full` passes.

The rebuilt ISO SHA-256 is
`77d3103707f3840db33827781ef57b125ed01756091c787323d63ee40ae4fc9a`; its
128 MiB build-disk SHA-256 is
`999349b3714f055c3d40eb40e1becd64cc583298d588e9f129e4bbba3b3f33c8`.
`make check-iso-installer` passed the explicit erase gate, full readback,
byte-identical installed image, disk-only boot, and saved return value `42`
before and after reboot. This latest source revision still needs a live UTM
installer/disk-only cycle; the previous artifact's live UTM cycle remains
recorded above.

2026-10-22 current graphics and installer regression: the non-cardinal
`SPT_ROTATED_RECT` case now asserts actual scanout pixels and an untouched
off-shape pixel, in addition to the cardinal CDC bounds. `make
check-grscreen-full` passes, including rotated rectangles, transformed sprites,
view-angle controls, and the QEMU zoom/pan menu path. The current ISO SHA-256 is
`11bba223d4162ab6dd1788e66dd34dd0c7ecb9b02f35c27bf38fc07577fbf8c4`; its
build-disk SHA-256 is
`49a1fac2e761fabf5804a08d65eb2a37cb662283f37615cac6c1ff0920898ae5`.
`make check-iso-installer` passed the explicit erase gate, full 128 MiB
read-back, byte-identical installed image, disk-only boot, and saved-data
checks returning `0x2a` before and after cold reboot. A live UTM check has
confirmed Zoom In, Pan Right, and Reset View work visibly; the current ISO
revision still needs its own live install and disk-only cold-boot cycle.
`make check-home-autostart` also passes: disk startup tiles both terminals,
runs `Home/Once.ZC`, presents and dismisses the centered animated splash, and
restores graphical-terminal input.

2026-10-22 projective rotated-ellipse regression: `GrSpriteSymmetryChecks`
now covers a non-cardinal ellipse crossing the homogeneous near plane and
asserts visible scanout plus an untouched off-shape pixel. Both `make
check-grscreen` and `make check-grscreen-full` pass with this case, along with
the view-angle controls, window-manager graphics checks, and full-text frame
smoke. This
test-only update did not rebuild the ISO, which remains SHA-256
`11bba223d4162ab6dd1788e66dd34dd0c7ecb9b02f35c27bf38fc07577fbf8c4`. The
current live UTM attempt booted the disk-only guest (`zinstall` reported that
the installer image was unavailable); the ISO drive still needs to be
attached and selected for boot before the exact-artifact install cycle can
pass.

2026-10-22 projective bitmap near-plane regression: `GrSpriteSymmetryChecks`
now covers an indexed `SPT_BITMAP` whose source texels straddle `w=0`. It
checks exact projected colors for positive-`w` texels and confirms that
nonpositive-`w` samples stay clipped. `make check-grscreen-full` and
`make check-home-autostart` pass; the latter confirms both startup splash
labels, animated centered Z, dismissal, and restored terminal input. A
separate candidate ISO was built without replacing the previously attached
artifact: `build/zealos-aarch64-hello-projective-bitmap.iso`, SHA-256
`2132026a4c4f045222dcdf9f2a3a23e33db8e84a6dce2478af7788b67c872d94`.
`make ISO=build/zealos-aarch64-hello-projective-bitmap.iso
check-iso-installer` passed guarded erase, complete readback, byte-identical
install, disk-only boot, saved result `0x2a`, and the same result after cold
reboot. The original
`build/zealos-aarch64-hello.iso` remains unchanged at SHA-256
`11bba223d4162ab6dd1788e66dd34dd0c7ecb9b02f35c27bf38fc07577fbf8c4`.
The candidate still needs the live UTM cycle: boot the SCSI CD/DVD first,
install to the disposable VirtIO target, then cold-boot that target alone.

2026-10-22 projective point near-plane regression: `GrSpriteSymmetryChecks`
now checks transformed `SPT_PT` and packed `SPT_POLYPT` records at negative,
zero, and positive homogeneous `w`; invalid points remain clipped and the
positive-w samples land on their expected pixels. The QEMU graphics suite
passed with the new case. A fresh, separately named ISO,
`build/zealos-aarch64-hello-projective-points.iso`
(`sha256=4cc06c89809220abb6d67f6d6a8b945aaae9d5d26d46c8f471be3f04abc674fa`),
also passed `make ISO=build/zealos-aarch64-hello-projective-points.iso
check-iso-installer`: guarded erase, complete read-back, byte-identical
installation, disk-only boot, and saved-data result `0x2a` before and after a
cold reboot. The previous candidate files remain unchanged. This exact image
still needs the live UTM install and disk-only cold-boot cycle.

2026-10-22 live UTM installer retry: the guest reached the installed-disk
shell and `zinstall ERASE` reported that the installer image was unavailable,
so this was not a CD boot. The earlier UTM drive-list screenshot had VirtIO
above SCSI; UTM uses list order as boot order. For the next attempt, put the
SCSI CD/DVD first, confirm it points to the projective-points ISO above, and
leave the disposable VirtIO target second.

2026-10-22 graphics rerun and live boot diagnosis: the latest
`make check-grscreen-full` completed with exit status 0. It includes the
projective point, packed-point, text-box, text-diamond, bitmap, mesh, spline,
ellipse, rotated-rectangle, and compositor checks. The projective-points ISO
remains unchanged at SHA-256
`4cc06c89809220abb6d67f6d6a8b945aaae9d5d26d46c8f471be3f04abc674fa`.
The 11:05 UTM screenshot again shows the disk shell and the
“installer image is unavailable” response, so the live ISO installation and
subsequent disk-only acceptance have not happened yet.

2026-10-22 upstream pixel-operation parity and fresh installer candidate:
the bootstrap pixel path now applies the pinned `GrPlot0` rules for bounded
offscreen CDCs and screen CDCs, including depth test/update, `DCF_DONT_DRAW`,
nearest-point and extents tracking, deterministic and probability dithering,
collision counting, and XOR. Filled `GrRect` routes each clipped pixel through
that same path, preserving task-window pixel visibility and CDC operations.
`GrPlotSemanticsChecks` passes every case, and `make check-grscreen-full`
passes the source-backed graphics suite.

The separately named ISO
`build/zealos-aarch64-hello-grplot-ops.iso` has SHA-256
`a9e35867928b69a3aa1a9ea5abde21fd59d1112f5758370d28c09fd52388366e` and its
build disk has SHA-256
`68d995d7e773fbd30b6befb3c5294d2acd01c5ea5fd7fec880383ae19636c513`.
`make ISO=build/zealos-aarch64-hello-grplot-ops.iso check-iso-installer`
passed ISO boot/startup, the no-write erase gate, full target read-back,
byte-for-byte install comparison, installed-disk-only cold boot, and
`InstallerPersist` returning `0x2a` before and after another cold reboot.
This exact ISO still needs its live UTM CD-first install and installed-disk
cold-boot cycle; the previous UTM screenshot showed the CD was not selected
first.

2026-10-22 verified GrPlot installer refresh: after the upstream `GrLine`
experiment was removed because it changed the pinned text-diamond raster,
`make check-grscreen-full` passed again with `GrPlotSemanticsChecks` returning
`0x2a`, and the existing source sprite, menu, window-manager, and view-angle
checks remained green. The fresh candidate
`build/zealos-aarch64-hello-grplot-final.iso` has SHA-256
`61a3a7117893290d7fdd7f8a0ab2838754884c0dedcaf06c6531d8d0a1679496`; its
128 MiB build disk has SHA-256
`3964e6db44c60202ab35aa0e73e064c415e4ba7ae8241442aa0c1fd2be0c5cf9`.
`make ISO=build/zealos-aarch64-hello-grplot-final.iso
check-iso-installer` passed the erase guard, full read-back, byte-identical
installation, disk-only cold boot, and `InstallerPersist` returning `0x2a`
before and after another reboot. The exact ISO still needs its live UTM
CD-first install and installed-disk cold boot.

2026-10-22 bitmap-blit ROP regressions and refreshed installer: `GrBlot3Checks`
now covers the 2D `GrBlot` fallback's source-color XOR while preserving
transparent source pixels, plus collision counting for eligible destination
overlaps. `make check-grscreen-full` passed with `GrBlot3Checks` and the full
sprite/text/compositor suite returning their expected success values. The new
candidate `build/zealos-aarch64-hello-grplot-blot-rop.iso` has SHA-256
`055eaa5ab08bd82b269ca936c544ed4a35647b5a811422a548644f8d59523b61`; its
128 MiB build disk has SHA-256
`3b1cc0efce44c30bcc7c51eead85ac9a662f7dfea23aec40a838846ade18dddd`.
`make ISO=build/zealos-aarch64-hello-grplot-blot-rop.iso
check-iso-installer` passed the erase confirmation gate, full read-back,
byte-identical install, disk-only cold boot, and `InstallerPersist` returning
`0x2a` before and after another reboot. This exact candidate still needs its
live UTM CD-first install and installed-disk cold boot.

2026-10-22 bounded offscreen `GrLine` and refreshed installer: offscreen CDC
lines now rasterize with bounded clipping, step/start patterns, pen thickness,
and pixel ROP handling. `GrLineCdcChecks` and the text-diamond outline reference
pass as part of `make check-grscreen-full`; the symmetry test also restores its
screen-bitmap flag after the preceding private-CDC mesh checks. The new
candidate `build/zealos-aarch64-hello-grline-cdc.iso` has SHA-256
`14b63f995f3d7399846394788a851b5b109cfae10c1c220a64f972f29838f4df`; its
128 MiB build disk has SHA-256
`85ebcb8c48b342feeb19049e1de5ba15100272ef0eb550c0a49f0bf868067342`.
`make ISO=build/zealos-aarch64-hello-grline-cdc.iso check-iso-installer`
passed the ISO live boot, erase confirmation, full read-back, byte-for-byte
disk comparison, disk-only cold boot, and saved `InstallerPersist` returning
`0x2a` before and after another reboot. This exact ISO still needs its live
UTM CD-first install and installed-disk cold boot.

2026-10-22 CDC brush-line rasterization and refreshed installer: the bounded
offscreen `GrLine` bridge now sends brush strokes through upstream `GrPlot1`,
preserving ZealOS's bit-mask brush format. `GrLineCdcChecks` covers a transparent
mask brush on an aligned stamp and at the clipped negative edge; the full
`make check-grscreen-full` gate passed. The new candidate
`build/zealos-aarch64-hello-grline-brush.iso` has SHA-256
`09a16f80072ea67c7edd704a97708476132d1015ece68ad70b1e7bc02a981a8a`; its
128 MiB build disk has SHA-256
`0ae23cfe9578870f16fa8f3b0a3db718908960577452fdafa312f1a4c684397a`.
`make ISO=build/zealos-aarch64-hello-grline-brush.iso check-iso-installer`
passed live ISO boot, the explicit erase gate, full read-back, byte-identical
installation, disk-only cold boot, and `InstallerPersist` returning `0x2a`
before and after a second cold reboot. This exact candidate still needs its
live UTM CD-first install and installed-disk cold boot. Full brush-blit ROPs,
broader CDC parity, full upstream menu/task startup, and pixel-identical
transformed-mesh output remain open.

2026-10-22 screen-backed CDC brush lines and refreshed installer: screen
`GrLine` strokes with a ZealOS brush now stay in task-local coordinates and
use upstream `GrPlot1` for brush stamps and window clipping. The focused
`--pci --probe-grline-cdc` check passed, and `make check-grscreen-full` passed
with the fixture-aware screen-CDC assertion. The new candidate
`build/zealos-aarch64-hello-grline-screen-brush.iso` has SHA-256
`15e0aca9474719c473da00f16c2c849e2fb69abd2aa4670ea2f76c6ffa75b639`; its
128 MiB build disk has SHA-256
`e7883e6b28f0da77bd95b97d787a6c1807a5f6b20cb2de0f20402faf0735f26e`.
`make ISO=build/zealos-aarch64-hello-grline-screen-brush.iso
check-iso-installer` passed live ISO boot, explicit erase confirmation,
complete read-back, byte-identical install, disk-only cold boot, and the saved
`InstallerPersist` value `0x2a` before and after a second cold reboot. This
exact ISO still needs its live UTM CD-first install and installed-disk cold
boot. Brush-blit ROP parity, broader CDC parity, full upstream menu/task
startup, and pixel-identical transformed-mesh output remain open.

2026-10-22 unaligned packed bitmap blits and refreshed installer: the guarded
ARM64 `GrBitMap.ZC` overlay now sends unaligned `DCF_NO_TRANSPARENTS` blits
through per-pixel ROP handling. This avoids the upstream shifted-word copy
producing incorrect pixels while preserving MONO mask, EQU, XOR, collision,
screen z-buffer visibility, and `DCF_DONT_DRAW` behavior; aligned blits keep
the packed path. `GrBlot3Checks` covers unaligned MONO mask, EQU copy, XOR,
collision/background exclusions, `DCF_DONT_DRAW`, and untouched boundary
pixels; the `GrLineCdcChecks` screen/offscreen brush cases still pass.
`python3 scripts/check-arm64-source.py`,
`python3 scripts/check-compat.py --pci --probe-grblot3`,
`python3 scripts/check-compat.py --pci --probe-grline-cdc`, and
`make check-grscreen-full` all pass. The current candidate
`build/zealos-aarch64-hello-grline-screen-brush-bitmap-safe.iso` has SHA-256
`7e794907b5048de8771227557c1df86f13158ce97ab1a953c6179d0f078a2e3b`; its
128 MiB build disk has SHA-256
`d10b9f00d2365b0672eafe641f5ea0563be3dc88272ec573b5cb47eca76643e`.
`make ISO=build/zealos-aarch64-hello-grline-screen-brush-bitmap-safe.iso
check-iso-installer` passed live ISO startup, erase confirmation, full
read-back, byte-identical install, disk-only cold boot, and `InstallerPersist`
returning `0x2a` before and after a second reboot. Live UTM install and
installed-disk cold boot of this exact ISO remain open. Broader brush-blit and
CDC parity, normal upstream menu/task startup, and pixel-identical transformed
mesh output remain open.

2026-10-22 screen-bitmap collision visibility and refreshed installer:
`GrBlot3Checks` now verifies that the screen-window z-buffer excludes the
occluded half of a bitmap collision while retaining the eight visible
overlapping pixels. The test uses a synthetic task and temporary z-buffer, so
it is deterministic in headless PCI guests. The focused GrBlot probe and full
`make check-grscreen-full` suite pass. The candidate
`build/zealos-aarch64-hello-bitmap-collision.iso` has SHA-256
`4c91f5d9de71de6829dd3fe91be7c0160c76ab1f250b7e3ca6d1ee282cf527eb`; its
128 MiB build disk has SHA-256
`496c80c403874ce87592d191710135eaf85eb1eb3fe5e725fcf056784283f9e5`.
`make ISO=build/zealos-aarch64-hello-bitmap-collision.iso
check-iso-installer` passed the ISO startup and desktop markers, guarded erase,
full sector read-back, byte-for-byte install, disk-only cold boot, desktop
status, and `InstallerPersist` returning `0x2a` before and after a second
reboot. This exact candidate still needs the live UTM CD-first installation
and installed-disk cold boot. Full upstream menu/task startup, broader brush
and CDC combinations, and pixel-identical transformed-mesh output remain
open.

2026-10-22 screen-window bitmap ROP checks and refreshed installer: the
`GrBlot3Checks` fixture now covers unaligned `DCF_NO_TRANSPARENTS` blits into a
screen CDC across three z-buffer cells. It verifies that the hidden edge cells
remain untouched and the visible middle cell gets the expected MONO, EQU, and
XOR pixels; collision mode counts only the eight visible destinations. The
focused QEMU probe passed three consecutive runs with `0x2a` and no arena
growth, and `make check-grscreen-full` passed the full source, startup, task,
sprite, compositor, and graphics regression gates.

The new ISO `build/zealos-aarch64-hello-bitmap-window-rops.iso` has SHA-256
`c90828dc332be943118dd5dce4d27e8f362f74dfc44dcc52cb02f17f4dd61683`; its
build-disk SHA-256 is
`b5a41daa55da628cd4c6091fb9a70a8bd2fff0cc9819f369d98b74a65ddac04a`. The
QEMU installer acceptance passed ISO startup, the write-free confirmation
gate, full sector read-back, byte-for-byte installation, disk-only cold boot,
desktop status, and the saved `InstallerPersist` result `0x2a` both before and
after a second reboot. This exact ISO still needs the live UTM CD-first install
and installed-disk cold boot. Full upstream menu/task startup, broader
brush-blit/CDC parity, and pixel-identical transformed-mesh output remain
open.

2026-10-22 aligned packed bitmap ROP checks and refreshed installer: the
screen-window `GrBlot3Checks` fixture now exercises both the aligned packed
copy path and the unaligned per-pixel path. For each, MONO, EQU, and XOR leave
the occluded destination cell unchanged and update only the visible cell;
collision counts exactly eight visible pixels. The aligned fixture includes
nonzero task scroll offsets. The focused probe returned `0x2a` on three
consecutive runs without arena growth, and `make check-grscreen-full` passed.

The new ISO `build/zealos-aarch64-hello-bitmap-window-rops-scroll.iso` has
SHA-256
`76b67ed2070f696f4e05e64a4efaf901f9f969e7f6fd11be963cda6485035600`; its
build-disk SHA-256 is
`2d3437e4978583f3885247bd26c2f80cdb35d6bb6197b5b17aa0abdd646017ec`. The
QEMU installer gate passed live ISO boot, guarded erase, full-sector read-back,
byte-for-byte installation, disk-only desktop boot, and saved-data return
value `0x2a` before and after a second reboot. This exact ISO still needs the
live UTM CD-first install and installed-disk cold boot. Full upstream
menu/task startup, broader brush-blit/CDC parity, and pixel-identical
transformed-mesh output remain open.

2026-10-22 packed bitmap vertical window-boundary check and refreshed ISO:
`GrBlot3Checks` now sends a 16x9 no-transparency bitmap across a vertical
8-pixel window-z-buffer boundary, with alternating visibility across all four
cells. MONO, EQU, XOR, and collision each update/count only visible pixels;
hidden cells remain unchanged. The focused probe returned `0x2a` three times
without arena growth, and `make check-grscreen-full` passed the full startup,
task, compositor, and graphics gate.

The refreshed ISO `build/zealos-aarch64-hello-bitmap-window-rops-vertical.iso`
has SHA-256
`064f7bb8b7a2181b062e45f9c85dcd94eb53992ccd69593930208f7b5a19c419`; its
128 MiB build disk has SHA-256
`50e826076387d9807a64acd62e9fca7c3040b79172361f43a4927f04141caa04`.
`make ISO=build/zealos-aarch64-hello-bitmap-window-rops-vertical.iso
check-iso-installer` passed the ISO boot and desktop markers, guarded erase,
full-sector read-back, byte-identical installation, disk-only cold boot, and
saved `InstallerPersist` value `0x2a` before and after a second reboot. This
exact candidate still needs the live UTM CD-first install and disk-only cold
boot. Full upstream menu/task startup, broader brush-blit/CDC parity, and
pixel-identical transformed-mesh output remain open.

2026-10-22 transparent bitmap ROPs across the vertical window boundary: the
`GrBlot3Checks` 16x9 screen fixture now also exercises ordinary transparent
EQU and XOR copies and collision detection with an alternating transparent
source mask. Transparent source pixels and occluded cells stay unchanged;
collision counts exactly 36 visible, nontransparent intersections. The
focused probe returned `0x2a` three times without arena growth, and the full
`make check-grscreen-full` gate passed with the updated fixture.

The refreshed current ISO
`build/zealos-aarch64-hello-bitmap-window-rops-visible.iso` has SHA-256
`386fd22c76eec201cd11bacd4b94c5ee1cb2ebe2126bb764c3fd75ddece5c5b3`; its
128 MiB installed-disk SHA-256 is
`6755be1b238b7c1b61ea99ec382e55536d4b4dd9aa1c50b007db29a7671edce8`.
`make ISO=build/zealos-aarch64-hello-bitmap-window-rops-visible.iso
check-iso-installer` passed ISO startup, guarded erase, full read-back,
byte-identical installation, disk-only desktop boot, and `InstallerPersist`
returning `0x2a` before and after a second cold reboot. This exact candidate
still needs the live UTM CD-first install and disk-only cold boot. Full
upstream menu/task startup, broader brush-blit/CDC parity, and
pixel-identical transformed-mesh output remain open.

2026-10-22 CDC mask-brush ROP restoration and collision: `GrLineCdcChecks`
now verifies that `GrPlot1` renders a no-transparency brush as a MONO mask
even when the caller's ROP is XOR, then restores the original color and ROP.
A transparent brush in collision mode counts only its opaque pixels that
overlap non-background, nontransparent destination pixels. The focused
`--probe-grline-cdc` guest check and full `make check-grscreen-full` gate pass.

The current ISO `build/zealos-aarch64-hello-bitmap-window-rops-brush.iso` has
SHA-256
`876c7dee6f489140209eeebf652080c3e9cea37a0bcdf7bc1e0021d7efb6bea2`; its
128 MiB installed-disk SHA-256 is
`c90e3e9db449243cf4938d945ca654fd362e80cebc0534a0395468292299b12b`.
`make ISO=build/zealos-aarch64-hello-bitmap-window-rops-brush.iso
check-iso-installer` passed live ISO boot, the erase gate, complete read-back,
byte-identical installation, disk-only desktop boot, and saved-data result
`0x2a` before and after a second cold reboot. This exact ISO still needs the
live UTM CD-first install and disk-only cold boot. Full upstream menu/task
startup, broader CDC operations, and pixel-identical transformed-mesh output
remain open.

2026-10-22 screen-backed mesh depth isolation: screen sprite mesh draws now
use an 800x600 screen-coordinate depth plane separate from CDC-owned depth
buffers. Each source task callback resets only its visible screen rectangle;
sprite coordinates include task origin and scroll, while the window z-buffer
continues to decide cross-task visibility and `DCF_ON_TOP` bypasses scene
depth. `SpriteScreenDepthChecks` returns `0x2a` for separate task regions,
redraw reset, and overlay bypass; `make check-grscreen-full` passes.

The new ISO candidate `build/zealos-aarch64-hello-screen-depth.iso` has SHA-256
`a2acb07c4510c9358e2bb51cf3a42b9eaa83e3c5a2eb93587659be97b5f15789`; its
build disk SHA-256 is
`1f3df13b36fe5dce87f88803151b8f991a500a9ac3f3d56be829bbba1ff2a866`.
`make ISO=build/zealos-aarch64-hello-screen-depth.iso check-iso-installer`
passed ISO startup, the no-write `zinstall` gate, confirmed full-sector
installation and byte-for-byte comparison, disk-only desktop boot, and
`InstallerPersist` returning `0x2a` before and after a cold reboot. This exact
ISO still needs the live UTM CD-first installer cycle and installed-disk-only
cold boot. For that check, attach this ISO as SCSI CD/DVD above the disposable
VirtIO disk in boot order, install with `zinstall ERASE`, then detach the ISO
and cold-boot the VirtIO disk by itself. Exact upstream mesh raster parity and
damage-region redraw remain open.

2026-10-22 screen-depth fixture isolation correction: `SpriteScreenDepthChecks`
now creates and tears down a real temporary guest task, saves and restores both
task geometries, scroll offsets, window-Z values, depth regions, visibility
entries, and CDC state. This avoids relying on task globals that can be reset
when modules are loaded from the shell. The focused probe and
`make check-grscreen-full` both return `0x2a`/pass with the corrected fixture.

The refreshed candidate `build/zealos-aarch64-hello-screen-depth-validated.iso`
has SHA-256
`f3c082919bee000e4e0bf83fe2dd578df1ae4d5c3eebd8cae707eb5315b14339`; its
128 MiB build disk has SHA-256
`e64a67f36ea62959e0a73ce3447b03f80ddffd3c67533f4edcc052a585fad3ef`.
`make ISO=build/zealos-aarch64-hello-screen-depth-validated.iso
check-iso-installer` passed ISO startup, the no-write `zinstall` gate,
confirmed full-sector installation and byte-for-byte comparison, disk-only
desktop cold boot, and `InstallerPersist` returning `0x2a` before and after a
second reboot. This exact ISO still needs the live UTM CD-first install and
installed-disk-only cold boot. Broader upstream startup, CDC coverage, and
pixel-identical transformed-mesh output remain open.

2026-10-22 animated splash regression and correction: the preceding
`screen-depth-validated.iso` passed the installer gate but failed the home
startup visual regression: the title and motto appeared without the rotating
mesh. `SpriteHLine0` now retains a CDC-owned task depth buffer when present,
uses the shared screen depth plane only when the CDC has none, and stores
visible mesh pixels in the staged framebuffer surface. `make
check-home-autostart` now verifies 698 changed pixels between frames, 2,082
centered blue mesh pixels, and that dismissal clears the splash. The full
`make check-grscreen-full` suite also passes, including
`SpriteScreenDepthChecks` returning `0x2a`.

The replacement ISO `build/zealos-aarch64-hello-screen-depth-splash.iso` has
SHA-256
`e495e6e52095777d1742e63219b0f90a104096a6cb7914016a08cf5f0b2e9d19`; its
128 MiB build disk has SHA-256
`ac0a1efdcd867e4eb96f2133c2d0db92beaed400e5c61653ab75198f5e54896f`.
`make ISO=build/zealos-aarch64-hello-screen-depth-splash.iso
check-iso-installer` passed ISO startup, the no-write confirmation gate,
full-sector install read-back, byte-identical comparison, disk-only desktop
cold boot, and `InstallerPersist` returning `0x2a` before and after another
reboot. This exact replacement still needs live UTM CD-first installation and
disk-only boot; the prior `screen-depth-validated.iso` is superseded.

2026-10-22 projective rectangle triangle fill and current installer candidate:
the clipped fill now uses the pinned upstream `GrFillTri0` scan converter
(`eee210f4379ba3d00b47507f336a158eefb2c993`) with ARM64 CDC line adapters.
`DCF_ON_TOP` triangle fills skip task-window bounds because their line writer
uses CDC-local coordinates. `GrSpriteSymmetryChecks` exercises the triangle
writer and the perspective rectangle directly and through `Sprite3`. Both
`make check-grscreen-full` and `make check-home-autostart` pass; the splash
regression observes 542 changing pixels, 2,199 centered blue mesh pixels, and
clearing on dismissal.

The refreshed ISO `build/zealos-aarch64-hello-triangle-ontop-clip.iso` has
SHA-256
`e1688919f825ea8d6f1a5905db67a9c0cb3ed7d723c521bbc3e024cdee99c5b8`; its
128 MiB build disk has SHA-256
`eabe65ea6bfd404241932090982508b791aba68393e16c1cfba96cf8721a8bdf`.
`python3 scripts/check-iso-installer.py
build/zealos-aarch64-hello-triangle-ontop-clip.iso` passed with exit code 0:
ISO startup, the no-write erase-confirmation gate, full-sector install
read-back, byte-for-byte disk comparison, disk-only desktop cold boot, and
`InstallerPersist` returning `0x2a` before and after a second cold boot.
This exact ISO still needs live UTM installation from its SCSI CD/DVD with the
disposable VirtIO target second, followed by a disk-only cold boot. Full
upstream menu/task startup, broader CDC parity, exact whole-mesh raster parity,
and optimized damage-region redraw remain open.

2026-10-22 pinned standard font and refreshed ISO candidate: the graphics
bridge now loads all 256 glyphs from the hash-pinned upstream
`Kernel/FontStd.ZC` instead of a compact fallback. The generator verifies the
source hash (`4f592db78b85ba1dcb45171415493922d9fe26017637b284fa9d53cba935ecff`)
and emits the exact row-major 8x8 table. The foreground ROP and text-frame
fixtures check upstream glyph bytes and LSB-left pixel order. The viewport
frame regression suspends live tablet polling only while it checks non-origin
zoom/pan, then restores the input state. `make check-grscreen-full` and
`make check-home-autostart` pass; the latter sees 724/720 changed pixels across
splash animation, 2,067/1,991 centered blue mesh pixels, and cleared pixels
after dismissal.

The refreshed ISO `build/zealos-aarch64-hello-font-std.iso` has SHA-256
`1f53f3317ccd607492be6d0bedea97451ffc44621917d88bc295ac8d3686ce36`; its
128 MiB install image SHA-256 is
`73f215db77fb4ef8ae3571784e11c88cad8d4c1d18892835a28deefcd2fc73c0`.
`make ISO=build/zealos-aarch64-hello-font-std.iso check-iso-installer` passed
ISO startup and desktop markers, the explicit no-write erase gate, complete
read-back, byte-identical disk comparison, disk-only desktop cold boot, and
`InstallerPersist` returning `0x2a` before and after another reboot. The
previous `triangle-ontop-clip.iso` is superseded for current-artifact checks.
This exact new ISO still needs the live UTM CD-first installer cycle and
installed-disk-only cold boot in the isolated disposable acceptance VM. Use
the SCSI CD/DVD before its VirtIO target, run `zinstall ERASE`, then detach the
ISO and boot the VirtIO disk alone. Full upstream menu/task startup, broader
CDC parity, exact whole-mesh raster parity, and optimized damage-region redraw
remain open.


2026-10-22 exact-font ISO rebuild and UTM CD refresh: `make ISO=build/zealos-aarch64-hello-font-std.iso check-iso-installer` regenerated the same source candidate with ISO SHA-256 `6b051913f3f940feee39c017d908c8899714e16e70ad5891de772b4c1ca822c1` and build-disk SHA-256 `e84b771ac24f7084a31421734739423d750785dfb346943adb286f13d5b89c8f`. The QEMU gate passed live ISO startup, the no-write erase guard, full install/read-back, byte-identical target comparison, disk-only desktop boot, and `InstallerPersist` returning `0x2a` before and after reboot. The earlier live UTM launch still had the superseded `projective-bitmap.iso`; the acceptance VM is now configured with a copy of this exact rebuilt ISO as SCSI CD boot index 0 and the disposable VirtIO target at index 1. The running UTM QEMU command line confirms both device order and the new ISO filename. Live display/installer interaction and the disk-only UTM cold boot remain pending.


2026-10-22 current-font ISO live UTM install and disk-only persistence acceptance: the rebuilt ISO SHA-256 `6b051913f3f940feee39c017d908c8899714e16e70ad5891de772b4c1ca822c1` booted in the isolated installer VM from a USB CD. A harmless SCSI-CD-only probe did not reach the shell; the repository’s prior accepted UTM runs use USB CD, and switching to that interface reached the ISO shell. With the disposable 128 MiB VirtIO target attached second, `zinstall` showed the erase confirmation, and `zinstall ERASE` erased, installed, checked every sector through 100%, and reported full read-back verified. After stopping the VM, the installed target SHA-256 was `e84b771ac24f7084a31421734739423d750785dfb346943adb286f13d5b89c8f`, byte-identical to `build/disk.img`.

The same target then cold-booted in UTM with the CD detached and VirtIO as the only drive. Serial `zstatus` reported `ready=1 failed=0`, `winmgr=1 gterm=1`, `presented_frames=664`, and `scanout: nonblack_pixels=50980 of 480000`. `Tests/InstallerPersist.ZC` was saved with `zput`, loaded from `disk:`, and returned `0x2a`; after a second cold stop/start with the disk alone, `zstatus` again showed the desktop and `InstallerPersist` again returned `0x2a`. Transcripts are `build/utm-font-std-utm-install.log`, `build/utm-font-std-disk-only-boot.log`, `build/utm-font-std-persist.log`, and `build/utm-font-std-disk-reboot.log`. The current UTM window remains locked, so a screenshot of this exact rebuilt image is not recorded. Full upstream menu/task startup, broader CDC parity, exact whole-mesh raster parity, and optimized damage-region redraw remain open.

Follow-up visual inspection after raising the UTM window showed a uniformly gray guest canvas rather than the desktop. Treat the serial desktop/persistence acceptance above as valid, but keep exact-image visual acceptance open until the guest display shows the splash and desktop.

2026-10-22 startup/splash regression rerun: `make check-home-autostart` rebuilt the disposable test disk and reached `HomeAutoStartChecks = 0x2a`; `PopupWindowOpen`, `PopupWindowClose`, `SplashWindowOpen`, and `SplashWindowWaitForExit` also each returned `0x2a`. The final guest status remained `ready=1 failed=0`, `winmgr=1 gterm=1`, with nonblack scanout. Transcript: `build/check-home-autostart-pci.log`. This verifies the automated startup/splash path; it does not resolve the gray live UTM canvas or the remaining full-menu/graphics parity gaps.

2026-10-22 full graphics regression rerun: `make check-grscreen-full` exited successfully. The PCI/QEMU guest returned `0x2a` from the plot semantics, line CDC, sprite bitmap/text/spline/flood-fill/symmetry/transform, screen-depth, DC lifecycle/copy/diff/ROP, source compositor, zoom, and view-angle checks; the final screen-frame tests passed as well. This does not close the documented broader CDC/brush ROP or exact whole-mesh parity gaps.

2026-10-22 bitmap ROP coverage follow-up: `GrBlot3Checks` now exercises the
upstream transparent-image MONO mask rule (zero clears, nonzero paints) at a
clipped destination edge and across screen cells with alternating visible and
occluded regions. The focused PCI probe returned `0x2a` three times without
arena growth, and the subsequent `make check-grscreen-full` passed, including
the full startup/window/graphics checks. Broader CDC and brush ROP parity,
whole-mesh pixel parity, and optimized damage redraw remain open.

2026-10-22 installed-disk UTM visual follow-up: the user started
`ZealOS AArch64 ISO Installer Acceptance` and supplied a live screenshot of
the guest. It shows both terminal windows and the blue animated Z centered
between the splash labels. This supersedes the earlier gray-canvas observation
and closes the live visual check for the previously UTM-installed
runtime-identical `font-std.iso`; no `zinstall` command was needed on this
disk-only boot.

2026-10-22 current ISO installer rerun: `make
ISO=build/zealos-aarch64-hello-font-std.iso check-iso-installer` passed against
the current source tree. The rebuilt ISO SHA-256 is
`4f532e3f6271ca06f6788ab7fd80ceb25e4b7de2781ef6e4e8b9213fa1467d73`; the
128 MiB build disk SHA-256 is
`713eb20b0340e21b036e08a1c250f53e8f26ca92b2d642377c55619f56b99e67`. QEMU
passed ISO startup, the no-write erase guard, full install and read-back,
byte-identical comparison, disk-only boot, and `InstallerPersist` returning
`0x2a` before and after reboot. This rebuild followed the test/documentation
update and has not been separately installed in UTM; the live UTM screenshot
and installation record above apply to the prior runtime-identical image.

2026-10-22 persistent Paint Demo and installer rerun: the live UTM Paint Demo
test exposed that the previous action returned only a `paint dots=` counter
without leaving an interactive drawing surface. Paint Demo now stays active
after OS-menu selection (or the `paint` command), samples held left-button
motion each frame, interpolates pointer gaps, and clears its bounded retained
stroke on Escape. `BootstrapGuiTerminalPaintChecks` verifies pointer sampling,
line interpolation, the yellow raster pixel in the screen CDC, release, and
cleanup; `BootstrapGuiTerminalSmoke` returns `0x2a`. `make
check-grscreen-full` passes with this change.

`make check-iso-installer` then passed with the updated source: ISO startup,
explicit erase confirmation, full readback, byte-identical disk comparison,
disk-only boot, and `InstallerPersist = 0x2a` before and after a second reboot.
The current `build/zealos-aarch64-hello.iso` SHA-256 is
`f4192b2766c059b024a66e181a9878ab0988f9b53ca739f647bcbae6f8b8fff2`; its
`build/install.img` payload SHA-256 is
`9506c65c5c91c48ad686f6497ada6021375abeb2e554b1d606a2b14f443443aa`.
This exact ISO still needs a live UTM install/cold-boot and interactive Paint
Demo confirmation. Full upstream menu/task startup, broader CDC/brush ROP
parity, pixel-identical whole-mesh output, and optimized damage redraw remain
open.

The same source revision also passed `make check-home-autostart`: the disk boot
tiles both terminals, executes `Home/Once.ZC`, opens and dismisses the source
PopUp and animated ZSplash windows, then restores graphical shell input and a
visible caret. The splash scanout check found 2,199 centered blue logo pixels
and verified that dismissal clears them. This confirms the staged startup
sequence; the complete upstream menu/task startup remains separate work.

2026-10-22 live UTM installer and Paint checkpoint: the user configured the
current ARM64 ISO as a USB CD ahead of the disposable VirtIO target, booted
ZealOS, and ran `zinstall ERASE` against an exact 128 MiB target. The guest
reported erase progress, checked every sector, and completed full read-back
verification. The subsequent UTM screenshot shows the desktop and yellow
Paint Demo strokes drawn inside the graphical terminal; Paint is an inline
terminal demo, not a separate window. The screenshot does not show the VM's
drive list, so the installed-disk-only cold boot and live `InstallerPersist`
save/readback across another reboot still need confirmation. The independent
QEMU installer gate already verifies those steps for this runtime.

The focused `GrLineCdcChecks` now also covers a thick dashed opaque-mask brush
stroke and confirms the MONO/ROP restoration and transparent-brush collision
behavior. The isolated probe returned `0x2a`, followed by a passing
`make check-grscreen-full` run. This closes the tested line/brush cases while
broader CDC/brush ROP combinations, whole-mesh pixel parity, full upstream
menu/task startup, and optimized damage redraw remain open.

2026-10-22 live UTM Paint follow-up: the user confirmed Paint Demo is not a
separate window and reported that it appears to work. The screenshot shows the
OS menu popup and yellow strokes drawn directly over the left graphical
terminal; this is the intended inline drawing surface. It confirms live guest
rasterization, but does not show the drive list or establish Escape cleanup,
ISO detachment, disk-only cold boot, or saved-file persistence. Those remain
separate live acceptance points; QEMU's installer/persistence gate passes.

2026-10-22 transparent brush regression: `GrLineCdcChecks` now additionally
verifies ordinary MONO brushes preserve destination pixels under zero-valued
source cells, paint nonzero brush cells with the caller's color even when the
caller selected XOR, and restore the caller's ROP afterward. The focused QEMU
probe returned `0x2a`; `make check-grscreen-full` then passed, including this
case and the graphics/startup suite. Broader CDC/brush ROP combinations,
whole-mesh pixel parity, full upstream menu/task startup, and optimized damage
redraw remain open.

2026-10-22 line rasterization follow-up: `GrLineCdcChecks` now also verifies
the exact seven-pixel shallow Bresenham path, per-pixel alternating-color
dither, and a depth-blocked pixel that must retain both its prior color and
depth. `make check-grscreen-full` passed after rebuilding the 245-file source
bundle; `GrLineCdcChecks`, the full sprite/CDC suite, startup checks, and final
screen-frame checks returned `0x2a` or passed as applicable.

2026-10-22 live installed-disk data check: the user corrected the `zput`
destination syntax, saved `Tests/InstallerPersist.ZC`, loaded it back from
`disk:`, and called `InstallerPersist` successfully; the shell reported
`ans=0x2A=42`. The `zcall` shell command takes a bare function name, without
parentheses or a semicolon. This verifies file write/read and execution on the
current installed volume. The post-reboot disk-only result remains to be
confirmed live.

2026-10-22 latest ISO gate after line-raster test additions:
`make check-iso-installer` passed from a fresh source bundle. The ISO booted
and reached the desktop, refused installation without the explicit ERASE
confirmation, fully erased/wrote/read back the disposable target, and matched
the payload byte-for-byte (`sha256=1ef64879a7e04f8c2d4b422aa4d7a0b75e7e55dd7ca83b44268fb883d082e4c5`).
The installed disk booted by itself in QEMU and `InstallerPersist()` returned
`0x2a` before and after another reboot. Latest ISO SHA-256:
`e7fd59bfae46f56e60fbfbb77f86d605144f15f146d610bb675dea4875a48286`.
This ISO differs from the live UTM-installed runtime only by the added
source-level `GrLineCdcChecks` regression cases; live UTM confirmation of the
disk-only reboot on the user's installed target remains pending.

2026-10-22 grouped boot-menu and installer refresh: the boot graphical
terminal now presents its implemented commands under `File`, `Windows`,
`Apps`, `View`, and `OS`, preserving the existing action handlers. Each menu
is parsed from a short `MenuNew` string because the guest compiler enforces
`STR_LEN`; the roots are then linked into one menu bar. `BootstrapGuiTerminalSmoke`
passed the nested-root and action-binding assertions in the full graphics
QEMU run. `make check-home-autostart` also passed: both terminals tiled, the
animated splash moved and cleared, and terminal keyboard input recovered.
`make check-iso-installer` passed with the rebuilt image: it reached
the desktop, refused an unconfirmed erase, fully wrote and read back the
disposable target, verified the installed bytes against the source disk
(`dafd8c46ea6b2812dfb456bbf6ae95d577a928794b148cc01effc956388686c4`),
booted the target without the ISO, and returned `0x2a` from `InstallerPersist`
before and after a cold reboot. ISO SHA-256:
`24f71af50d0e0f885ac7656771985fd2cf8d316b6e0a35744772c0cc4b8ff635`.

The user then cold-booted the installed UTM disk with the CD detached, loaded
`disk:Tests/InstallerPersist.ZC`, and ran `zcall InstallerPersist`; the guest
returned `0x2a`. The parenthesized command was rejected because `zcall` is a
shell command that takes a bare name. This closes live installed-disk saved
data persistence for that installed image. The refreshed grouped-menu ISO has
passed QEMU startup and installer checks but has not yet been installed or
visually inspected in UTM. Full upstream menu/task startup, broader CDC parity,
exact whole-mesh raster parity, and optimized damage-region redraw remain
open.

2026-10-22 final grouped-menu ISO gate: after the latest menu and GUI workflow
changes, `make check-home-autostart`, `make check-grscreen-full`, and
`make check-iso-installer` all passed. The home-startup probe tiled both
terminals, animated and cleared the centered Z splash, dismissed the source
popup, and restored shell input. The full graphics workflow passed its grouped
File/Windows/Apps/View/OS menu routes, graphical editor/save/reboot/reopen
flow, live Lattice and Stock demos, sprite/screen-depth cases, and final-frame
checks. The installer gate booted the ISO, rejected an unconfirmed erase,
installed and read-back-verified the disposable target, and matched its bytes
to the source disk (`sha256=d8780ca1e1c57d7e6e583e59123d35c171844b2b1875d23030f64ded898ba41d`).
The installed disk then booted alone and returned `0x2a` from
`InstallerPersist` both before and after a second reboot. Current artifact
hashes: ISO `ea471a30ab6ee3f7004f8323285ecdf6445a25b19832c94a4596493c7870ef1f`,
installer payload `74876fb25b79046b98394edc562bebf0bbe654d9008bc8e1b0580883ecac8817`.
At this QEMU-gate checkpoint, the exact refreshed ISO still awaited its live
UTM installation and visual menu check; the subsequent live install milestone
is recorded below.

2026-10-22 live UTM install of the grouped-menu candidate: the user reports
booting from its freshly installed target. The live guest shows the File,
Windows, Apps, View, and OS menu groups. The user saved
`Tests/InstallerPersist.ZC`, loaded it with
`zload disk:Tests/InstallerPersist.ZC`, and ran `zcall InstallerPersist`; the
guest returned `0x2a`. An initial load omitted the `Tests/` directory and
reported source-not-found; the corrected path succeeded. The user then
cold-booted with the CD detached and repeated the disk-only `zload`/`zcall`;
it returned `0x2a` again. The supplied screenshot shows the grouped menus and
successful result after that cold boot. Live installed-disk persistence is
accepted for ISO
`ea471a30ab6ee3f7004f8323285ecdf6445a25b19832c94a4596493c7870ef1f`.

2026-10-22 final current-artifact installer rerun: after the startup test
regenerated the ISO, `make check-iso-installer` passed again against
`build/zealos-aarch64-hello.iso`. It rejected an unconfirmed erase, installed
and fully read-back-verified the disposable 128 MiB VirtIO disk, and confirmed
the installed image matched byte-for-byte
(`sha256=53beb35a854bb0ba01660ba0cf8f73aead36eb650bc77d87eec051f4831907d2`).
The disk-only guest reached the desktop, returned `0x2a` from
`InstallerPersist` before reboot, then loaded and returned `0x2a` again after
reboot. Current ISO SHA-256:
`03be7af84d2029cb2252b631a47c4fa383b8c5d18a71692e20cf8befc8c66fa7`.
The live UTM install above used the same tested source state with different
generated GPT identifiers; its installed disk independently passed the
user-observed second cold-boot check. Remaining broader port work—full
upstream startup/menu/task initialization, complete CDC/brush behavior,
pixel-identical whole-mesh rasterization, and damage-region redraw—is still
tracked in `COMPATIBILITY.md` and `upstream/GRAPHICS-PORT.md`.

2026-10-22 Paint Demo stop/clear regression and final ISO rebuild:
`BootstrapGuiTerminalPaintChecks` now activates the demo, samples a held-button
stroke, verifies the yellow CDC pixel, invokes the same stop action used by
Escape, and asserts the active flag, retained point count, and previous-point
state are cleared. `make check-grscreen-full` passed with the new assertion and
all source-backed graphics/window-manager checks. The rebuilt current ISO
`build/zealos-aarch64-hello.iso`
(`sha256=aa2dbf05fd2c63dd10ed67dce801bbec0a3ebeb553af74da66e0a5b52e238b6d`)
passed `make check-iso-installer`: guarded erase, complete read-back,
byte-identical disk
(`sha256=f88010e61145442a98ad0439f6917f849686ba41feba4226d087c14473cb424c`),
disk-only boot, and `InstallerPersist` returning `0x2a` before and after a
cold reboot. The immediately preceding same-source ISO remains the one
installed and visually checked by the user in UTM; this rebuild adds the
automated Escape-state assertion to its bundled source checks.

2026-10-22 formatter/startup extension and latest installer rebuild:
`MStrPrint` and the graphical formatter now support significant-digit `%g` and
`%G`, fixed/scientific notation, numeric field width, zero padding, and dynamic
`%0*tX`. `make check-user-startup` passed the formatting assertions, the
original startup and terminal workflows, independent input/history in two
graphical terminals, and the exact repeated-terminal cleanup check.
The same updated runtime passed `make check-grscreen-full`, including the
source window-manager frame path, standard-font glyphs, zoom/view controls,
CDC/sprite operations, and full-frame text scanout.
`make check-iso-installer` passed for the rebuilt
`build/zealos-aarch64-hello.iso`: it rejected unconfirmed erase, installed the
disposable 128 MiB VirtIO disk, and read every sector back. The installed disk
matched the build disk (`sha256=840f6f1a1dce85a9a72a386321f79d6d7675d9c6979a5d72ac44c41909958bcf`);
its disk-only boot reached the desktop and `InstallerPersist` returned `0x2a`
before and after a cold reboot. Current hashes: ISO
`058eada37cbf24427f8528cf31d72a240d34827bf7c5939c3ab2d7cf90cdca88`, installer
payload `c5675e34efbf6d0bbe834cf73af4a1f276482b95284f7373bd416030a8223784`.
The exact current ISO has not yet been installed in UTM; the user-observed
File/Windows/Apps/View/OS menus and second-reboot persistence remain accepted
for the earlier `ea471a30…` candidate. Full upstream startup/menu/task
initialization, the rest of formatter/font parity, broader CDC/brush behavior,
pixel-identical whole-mesh output, and damage-region redraw remain open.

2026-10-22 final QEMU desktop and installer rerun: the terminal now restores
the shell caret to the DolDoc sentinel after output and mouse focus changes.
`make check-user-startup` passed the formatted Home/Once startup, two tiled
terminals, visible typing and output in both, isolated command history, and
prompt recovery. `make check-home-autostart` passed both terminal startup, the
centered animated splash and its dismissal, popup open/close, and restored
keyboard input. `make check-grscreen-full` passed the grouped menu, source
window manager, zoom/pan, CDC, sprite and full-frame checks. The first broad
PCI rerun exposed a post-Window-Demo focus timing race in its QMP test; the
test now refocuses the shell and confirms visible `zls` before Return.
`make check-compat-pci` then exited successfully, including the graphical
keyboard/editor/window-demo recovery path and the MathODE module probe.

The same source produced ISO
`build/zealos-aarch64-hello.iso` (SHA-256
`3b5ab41723f781e0c40a6dd2218a0a48e6bf97fbc9e5fb17bf99520e64cebea3`) and
installer payload `build/install.img` (SHA-256
`4d2986f1302039073f3b559ad28759188f2a7ff74d09825b85d64de89f691b71`).
`make check-iso-installer` passed the unconfirmed-erase guard, full 128 MiB
read-back, byte-identical target comparison (disk SHA-256
`9dd15f5ee2b91dda98dbd080487193dfb7892d2de30857eeb8d4e5c7cc0499ef`),
disk-only desktop boot, and `InstallerPersist` returning `0x2a` before and
after reboot. This exact ISO still needs its live UTM install and disk-only
cold-boot check; the earlier UTM artifact acceptance above remains valid.

2026-10-23 terminal output ownership fix and current installer candidate:
`BootstrapGuiTerminalPrintOutput` now appends output to the registered GUI
terminal's document explicitly. The previous callback recalculated the active
task's `put_doc`; compiler output could therefore move the cursor in a source
document while that file was being compiled. The NEC/QEMU graphical-workflow
script passed after the fix. The rebuilt `build/zealos-aarch64-hello.iso` passed
`make check-iso-installer`: unconfirmed erase was rejected, the disposable
128 MiB VirtIO target was fully read back and matched the embedded disk image
(`sha256=7b0e55db04b21848b50df48aefe54bd3112160788a02352df5cd7084dfc9af54`),
the installed-disk-only boot reached the desktop, and `InstallerPersist`
returned `0x2a` before and after a cold reboot. Current ISO SHA-256:
`0a7063a9f4777a12b1e2d8b99f86652d760bc15ebcdd27560d3263c506899756`;
installer payload SHA-256:
`a846515fc0a0e34d396796198074e15f052eb04a0db54453b2c3371294677777`.

The user also supplied a 2026-10-22 UTM screenshot showing the grouped File,
Windows, Apps, View, and OS menus and `zcall InstallerPersist` returning
`0x2a` after a disk-only boot. The screenshot does not identify its ISO digest.
The exact `0a7063a9…` candidate still needs the live UTM install, splash/menu
visual check, and disk-only cold-boot persistence check.

2026-10-23 GUI output ownership regression and final QEMU installer run:
`BootstrapGuiTerminalOutputOwnershipChecks` creates a temporary source document
on a separate valid task, directs output to the GUI terminal while that source
task is active, then verifies that only the terminal document changes. The
test is included in `BootstrapGuiTerminalSmoke`, and the focused
`make check-gui-terminal` gate passed both before and after restarting the
graphical terminal. The broader NEC/QEMU USB, graphical editor, menu, window,
and VirtIO recovery matrix also passed with `make check-usb-input`.

The rebuilt current ISO passed `make check-iso-installer`: the unconfirmed
erase guard held, the disposable 128 MiB VirtIO disk passed full read-back and
matched the source image (`sha256=3fe6df8dbdc899f5c456d42482e9479614ab237b004a482db855c5e884593354`),
the installed disk booted alone, and `InstallerPersist` returned `0x2a`
before and after a cold reboot. Current artifact hashes: ISO
`132401a095fe5f3c828492bbf6f08f8cc814e4e8d4100198f45c1e9610f6fc16`,
installer payload
`0bf4cda4582d6ff98fc327b1eb32c1488f20dd7c27675ebecf7cdffd59fc2a73`.
The same updated source tree also passed `make check-home-autostart`, which
measured the centered splash mesh and its dismissal, and
`make check-grscreen-full`, which exercised full-frame scanout, CDC and sprite
regressions. The exact ISO file and installer payload stayed unchanged after
these source-tree checks.

The user supplied a later UTM screenshot showing File/Windows/Apps/View/OS
menus and `InstallerPersist` returning `0x2a` after loading the saved source
from the installed disk. The screenshot does not identify its ISO digest, so
it confirms the prior live UTM install but not this final rebuilt candidate.
The exact `132401a0…` ISO still needs a live UTM install, visual desktop/splash
check, and disk-only cold-boot persistence check.

2026-10-23 formatter compatibility and refreshed installer candidate:
`StrPrintJoin` now handles ZealOS uppercase packed `%C`, boolean `%o`, unsigned
`%u`, binary `%b`/`%B`, and decoded `%q` string escapes in addition to its existing
formatter subset. `UserStartUpChecks` covers these conversions; both
`make check-user-startup` and `make check-grscreen-full` passed. The refreshed
`build/zealos-aarch64-hello.iso` passed `make check-iso-installer`: guarded
erase, complete 128 MiB read-back, byte-identical target, installed-disk-only
boot, and saved data returning `0x2a` before and after a cold reboot. The
subsequent `make check-home-autostart` also passed the centered animated splash,
popup lifecycle, and restored terminal input checks. Hashes: ISO
`e81be7dae77f8d32a1c9186192585df804b26e01d5e9ecce01ea337ef17538b6`, installer
payload `adf0759771c105ae325b13e7c59e6b196e525d99ddcedf9755ecbbcfee984fd7`,
and installed disk
`a3b50b4879eda2f30ffb00b78250e50494070f578a67d81cff083e4fb006df96`. The
exact rebuilt ISO still needs live UTM install, splash/menu inspection, and
disk-only persistence confirmation. The remaining formatter conversions and
flags, complete font system, broader CDC/brush ROP parity, exact whole-mesh
raster parity, full upstream menu/task startup, and damage-region redraw remain
open.

2026-10-23 AArch64 font-table and installer refresh: the build now generates
both pinned ZealOS `FontStd.ZC` and `FontAux.ZC` tables, and the translated
`GrScreen.ZC` passes the active `text.font` pointer to the ARM64 glyph
rasterizer. `make check-grscreen-full` passed standard and auxiliary glyph
raster comparisons against the pinned tables, CDC, and sprite checks;
`make check-home-autostart` passed the centered animated splash, popup
lifecycle, and restored terminal input. The source-side standard/auxiliary
pointer swap passes; the complete upstream font hotkey/event path remains open.

The refreshed `build/zealos-aarch64-hello.iso` passed `make check-iso-installer`:
the erase guard held, all sectors of the disposable 128 MiB disk were read back,
the installed image matched byte-for-byte (`sha256=944b1088b6a2bb30797e2449392411b2fbbabe1fa6a420e9e0f7713517e78403`),
the disk booted without the ISO, and `InstallerPersist` returned `0x2a` before
and after a second reboot. Artifact hashes: ISO
`ec1d78cb70eb1853d8983e89111a78eb966177aa93af00257c2e824e14b9cb6d`, installer
payload `60d91a26319968581049029d247266d0cb586c0b804d9ada0a1e7ff778a769ac`.
The Oct 22 UTM screenshot confirms persistence on its installed candidate but
does not identify an ISO digest. This exact ISO still needs live UTM install,
visual splash/menu inspection, and disk-only cold-boot persistence confirmation.

2026-10-23 auxiliary-font raster parity: `make check-grscreen-full` now checks
the active auxiliary `A` glyph row-by-row and pixel-by-pixel against the
pinned `FontAux.ZC` table, including transparent pixels and row padding. The
full graphics suite passed. `make check-iso-installer` rebuilt and passed the
QEMU install/read-back, disk-only boot, and second-reboot persistence gate for
the ISO above. The exact ISO still needs installation and visual acceptance in
UTM; the complete font hotkey/event path, full upstream menu/task startup,
broader CDC/brush parity, exact whole-mesh raster parity, and optimized
damage-region redraw remain open.

2026-10-23 Ctrl-Alt-F message dispatch and installer refresh: the guarded
ARM64 `Message.ZC` overlay routes decoded, non-IRQ Ctrl-Alt callbacks through
the normal message consumer while preserving the pinned upstream file. The
Home startup regression passed with synthetic messages, and the live QEMU
keyboard probe pressed Ctrl-Alt-F twice and verified the auxiliary font became
active and the standard font returned. `make check-input-edges` passed for PCI
VirtIO input and NEC USB input through two hubs with duplicate VirtIO input
present. The rebuilt ISO passed `make check-iso-installer`: erase protection,
complete 128 MiB read-back, byte-identical installed disk
(`sha256=a696fc5ef912d1c008bac07f4c4ca6a233889e59d57fadedb8b177840bc22223`),
disk-only boot, and `InstallerPersist` returning `0x2a` before and after a
second cold reboot. Artifact hashes: ISO
`2ea7965e6fd34867797e00dc9a44d072247e1a361f4bc84e74edf54f1d9a7bd4`, installer
payload `5a1b8ea265579e54902be4a8723f5e96f9d1bf74e1f20d9383985218e11b83d3`.
The user supplied a disk-only UTM screenshot showing
`zload disk:Tests/InstallerPersist.ZC` followed by `zcall InstallerPersist`
returning `0x2a`. Its filename timestamp (2026-10-22 9:31 PM) predates this
candidate's build (2026-10-23 4:31 AM PDT), so it confirms persistence on the
earlier installed candidate only. This exact ISO still needs live UTM
installation, visual desktop/splash inspection, font-toggle confirmation, and
disk-only persistence confirmation.

UTM's current acceptance VM config has only a 128 MiB VirtIO disk attached;
its notes identify an earlier installer ISO (`6b051913…`). The powered-off
disk was preserved before the next erase cycle as
`build/utm-installed-disk-pre-2ea.qcow2` (SHA-256
`1b27dd69574734f6a6a96f41f075bb12bcaa8d7b5f11ed106c5189f9c87f5156`); its
`qemu-img check` reports no errors. The current ISO is ready to attach as a
USB CD at boot index 0 for the live installer acceptance.

2026-10-23 prepared a separate live UTM acceptance VM for the current ISO.
`ZealOS AArch64 Current ISO Acceptance` was cloned from the wallpaper
acceptance template; the original template and ISO Installer Acceptance VM
were left untouched. The clone uses the current `2ea7965e…` ISO in its existing
USB-CD-first slot and has a fresh zeroed 128 MiB VirtIO target second. The
template's copied target was found nonblank, so it was preserved at
`build/utm-current-acceptance-template-backup/WallpaperAcceptanceTarget.raw`
(SHA-256 `75e8d8c8c93300ffcb2836bb9d62d687eb6bc02118a16bb22613e22b780947c8`)
before replacing only the clone's target. The cloned CD file hashes exactly to
the current ISO; the VM is stopped and ready for the user to start. The current
ISO's live UTM install, desktop/splash and Ctrl-Alt-F inspection, and
disk-only persistence check remain pending.

The user's Oct 22 9:31 PM screenshot shows the earlier installed disk loading
`Tests/InstallerPersist.ZC` and returning `0x2a` after reboot. It is useful
evidence for that earlier disk only; the screenshot predates the current
`2ea7965e…` candidate.

2026-10-23 Ctrl-Alt help bridge: `KeyMapCtrlAltFamily` now prints the source
shortcut descriptions from guest `keydev`, and its QEMU regression asserts the
Ctrl-Alt-F font-toggle entry. `make check-home-autostart` passed the map check,
both queued font-toggle states, centered animated splash and dismissal,
popup lifecycle, and terminal focus/input recovery. The native bridge bounds
guest descriptions to their arena and caps output. The ISO predates this source
change and will be rebuilt before the exact-image UTM acceptance run.

2026-10-23 rebuilt UTM candidate: the source-bound shortcut-help change passed
`make check-home-autostart`; `make check-iso-installer` then passed guarded
erase, full 128 MiB read-back, byte-identical install
(`sha256=6ac5aed711f81a067a11866c906b3827a67fefb8833b480ef50ba3e1c69bdf6),
disk-only desktop boot, and `InstallerPersist` returning `0x2a` before and
after a second cold reboot. Artifact hashes: ISO
`e2bbad705ba3f60d8527af12ab3dbb7847e11f96f198639548b068d48c9b29a8`, installer
payload `2b85227620cdf1a4db196d53888d6b24d320ff18984a377284494156d9722cf6`.
`make check-grscreen-full` also passed against the same rebuilt source tree.
`make check-input-edges` passed both PCI VirtIO keyboard and NEC USB keyboard
input through two nested hubs with VirtIO also attached; `make
check-user-startup` passed formatted Home/Once startup, independent terminal
typing/output/history, late-bound hooks, prompt recovery, and exact task
cleanup. The current source regression set is green; the remaining acceptance
step is the live UTM run of this exact ISO.
The stopped `ZealOS AArch64 Current ISO Acceptance` clone now contains this
exact ISO as USB CD boot index 0 and the fresh 128 MiB VirtIO target at index
1. The prior CD image was preserved as
`build/utm-current-acceptance-pre-e2bb.iso`. This exact candidate still needs
live UTM install, visual desktop/splash and Ctrl-Alt-F confirmation, and
disk-only persistence verification.

2026-10-23 screenshot follow-up: the supplied disk-only screenshot shows
`zload disk:Tests/InstallerPersist.ZC` followed by `zcall InstallerPersist`
returning `0x2a` after reboot. Its embedded filename timestamp is Oct 22, so it
confirms the previously installed disk and does not identify the ISO digest.

2026-10-23 bootstrap keyboard help: the ordinary key-map preview now lists the
command-line controls implemented by the ARM64 graphical terminal, while the
Ctrl-Alt map continues to show the registered font toggle. `make
check-home-autostart` passed the key-map assertions, both Ctrl-Alt-F font
states, centered splash and dismissal, popup lifecycle, and terminal focus and
input recovery. This source change is newer than `e2bbad70…`; the exact UTM
candidate must be rebuilt and rechecked before the pending live acceptance.

2026-10-23 current ISO rebuild after bootstrap key-map work: the current
`build/zealos-aarch64-hello.iso` has SHA-256
`4696f713b7e1c34a7d8b65a825d41fe9c22dec497f4bfbfc1add6c052c987074` and its
embedded installer disk is `build/disk.img` (SHA-256
`05d9d62375fed8a234b13dcc40a5dac797cc536e31f37b09b3f57744a462fd63`). The
rebuilt image passed `make check-iso-installer`: the erase guard held, all
sectors of the disposable 128 MiB target were read back, the installed disk
booted alone, and `InstallerPersist` returned `0x2a` before and after a second
cold reboot. `make check-home-autostart`, `make check-grscreen-full`,
`make check-input-edges`, and `make check-user-startup` passed on the current
source tree, covering centered splash animation and dismissal, grouped menus,
keyboard help and Ctrl-Alt-F font-toggle paths, full graphics scanout, input
through nested hubs, independent terminal input/output/history, startup hooks,
and terminal recovery.

The stopped live VM `ZealOS AArch64 Current ISO Acceptance` now has this exact
ISO in its USB CD slot at boot index 0 and a fresh zeroed 128 MiB VirtIO target
at index 1. The prior CD image was preserved as
`build/utm-current-acceptance-pre-4696.iso`. The supplied Oct 22 disk-only
screenshot returns `0x2a`, but its timestamp predates this ISO and cannot
confirm this candidate. Exact-image live UTM installation, visual desktop and
splash inspection, Ctrl-Alt-F check, and disk-only persistence after cold
reboot remain pending.

2026-10-23 refreshed live candidate after the RedSea file-list bridge: current
ISO `build/zealos-aarch64-hello.iso` SHA-256 is
`3dec114946364a91eae0e5b1af69a93bf89bff2397e809aa307f35156eb45be1`. The
installer round-trip verified guarded erase, every sector of the 128 MiB
VirtIO target, byte-identical installation, disk-only desktop startup, and
`InstallerPersist` returning `0x2a` before and after a second cold reboot.
`make check-home-autostart`, `make check-grscreen-full`, `make
check-input-edges`, and `make check-user-startup` also passed on this source
tree, and the RedSea DocFile probe verified root-file listing, size, and
contents. Together these gates cover the centered animated splash, menu and
view paths, font switching/help, graphics scanout, keyboard input and terminal
recovery.

The `ZealOS AArch64 Current ISO Acceptance` clone was configured with this
exact ISO in its USB CD slot at boot index 0 and a zero-filled 128 MiB VirtIO
target at index 1. The prior CD image is preserved as
`build/utm-current-acceptance-pre-3dec.iso` (SHA-256
`4696f713b7e1c34a7d8b65a825d41fe9c22dec497f4bfbfc1add6c052c987074`). The
exact current ISO was installed in this UTM clone; the guest reported full
sector read-back verification. The ISO was then removed from the clone
configuration, leaving only the VirtIO disk. Its first disk-only boot reported
the window manager and graphical terminal started; after saving
`Tests/InstallerPersist.ZC`, the program loaded from `disk:` and returned
`0x2a`. A second cold boot with only the VirtIO disk again reported desktop
startup, loaded the saved file, and returned `0x2a`. The running VM's config
now lists only that VirtIO disk. The final disk image hash is
`919b194272fffd1e26ec3c77b66ae0239f9b12177c4f8cb33c31d43093be37b7`.

The CUA display bridge still reports the Mac locked, so the remaining exact
image acceptance is visual: confirm the animated centered Z splash, menus,
and Ctrl-Alt-F font toggle in the running disk-only guest. The Oct 22
screenshot remains useful as earlier visual/persistence evidence, but is not
being used as a substitute for this ISO's live check.

2026-10-23 final post-multi-terminal-fix candidate: `build/zealos-aarch64-hello.iso`
has SHA-256
`a2a161123ba31e3d00c20bccd6fe1810ea4f186b93035a01a9c2fa4c1a047a91`; its
embedded installer disk is `build/disk.img` with SHA-256
`db775d46f879c4a2ba23ead3cb21c89cf39385f40233578cca61e979802385b8`.
`make check-iso-installer` passed guarded erase, full 128 MiB read-back,
byte-identical installation, disk-only desktop boot, and saved-data persistence
through a second cold reboot (`InstallerPersist` returned `0x2a` both times).
The output callback now routes `Print` and diagnostics to the task that
registered the callback, fixing the second Home/Once terminal's output capture.
`make check-home-once`, `make check-home-autostart`, `make check-user-startup`,
`make check-gui-workflow`, `make check-grscreen-full`, and
`make check-input-edges` all pass on this source. These include splash and
font-toggle checks, two-terminal startup and input, graphics, and nested-hub
USB input.

The `ZealOS AArch64 Current ISO Acceptance` VM has this exact ISO in its USB CD
slot at boot index 0 and a disposable 128 MiB VirtIO disk at index 1. It booted
the ISO and its serial output reported that the window manager and graphical
terminal started. The previous installed disk is preserved at
`build/utm-3dec-final-persist.raw`. UTM installation of this exact candidate,
disk-only persistence, and visual splash/menu inspection remain pending. The
Mac display bridge reports the Mac locked; the current UTM stop request has
also timed out, so the VirtIO target will be re-zeroed after the VM is stopped
and before installation.

2026-10-23 refreshed candidate and UTM boot-source check: the latest ISO
`build/zealos-aarch64-hello.iso` has SHA-256
`7cbcc872e2c992d1c52081126efa81b5fbfddb17186779cf1152cd0bb76693c0`; its
embedded installer disk `build/disk.img` has SHA-256
`e0625c5fe7535851086aadb6d866f14b32309747dc54e7d71c8fcfa8e74c386a`.
`make check-iso-installer` passed for this exact build: guarded erase, full
128 MiB read-back, disk-only desktop boot, and `InstallerPersist` returning
`0x2a` before and after a second cold reboot.

The live `ZealOS AArch64 ISO Installer Acceptance` VM was inspected in UTM.
Its active QEMU command line contains only a VirtIO block device (boot index
0) and no CD/DVD device, so its current desktop is booted from storage. A
second launch failed while trying to bind `127.0.0.1:2222`, which is already
used by this running VM's SSH forward; this is a host-port collision, not an
ISO boot error. Do not count that earlier storage-only session as exact-image
ISO acceptance. At that point, the remaining live step was to attach the ISO
as a USB CD and verify install, disk-only startup, saved data after reboot,
and the visual desktop/splash/menu paths.

2026-10-23 refreshed ISO USB-CD boot confirmed in UTM: the
`ZealOS AArch64 ISO Installer Acceptance` VM now has
`build/zealos-aarch64-hello.iso` attached as a read-only USB CD at boot index 0
and the disposable 128 MiB VirtIO target at index 1. UTM's QEMU command line
shows `media=cdrom`, `readonly=on`, and the ISO path inside the VM package; the
UEFI screen loaded the USB device and the guest reached ISO startup graphics.
Converting UTM's imported qcow2 back to raw produced a byte-for-byte match for
the repository ISO, and the VM target passes `qemu-img check`. The target
contains a prior test install and
will be erased by the already-authorized `zinstall ERASE` acceptance step.
The remaining checks are the installer read-back report, CD-detached cold
boot, persistent `InstallerPersist` value, and final visual desktop/menu
inspection. The Mac locked again while the startup sequence was running, so
the guest prompt needs a manual unlock to continue.

2026-10-23 exact-image UTM install and disk-only desktop boot: the same
`7cbcc872…` ISO reached the guest desktop from its read-only USB CD, and
`zinstall ERASE` completed on the disposable 128 MiB VirtIO target. The guest
reported erase through 100%, checked every sector, verified through 100%, and
ended with “installed, and fully read-back verified.” After stopping the VM,
the CD image was preserved and its drive interface was set to `None
(Advanced)` in UTM. A cold boot then loaded the disk through UEFI/RedSea to the
graphical desktop, with the centered animated Z splash visible during startup.
The saved UTM configuration confirms the CD image has `Interface=None` and
the writable target is the only drive with an active storage interface
(`VirtIO`).
The remaining live checks are saving `Tests/InstallerPersist.ZC`, verifying
`0x2a` on the disk-only boot and after a second cold reboot, and completing
the current-image menu/font-toggle inspection.

2026-10-23 final ISO UTM acceptance: the current
`build/zealos-aarch64-hello.iso` (SHA-256
`52c214c29a9c23d86c94899e8ef2ba94bd4a21231c92ecdd649714708e2bc627`) was
attached as a read-only USB CD at boot index 0 in a fresh UTM installer VM,
ahead of its disposable 128 MiB VirtIO target at index 1. `zinstall ERASE`
completed erase, sector checking, and full read-back verification. The target
passed `qemu-img check` and compared byte-for-byte with the embedded
`build/disk.img` (SHA-256
`1a4516a67d42aecc7284bfc68e455c5231767360e75a9df021dddb6946d98fde`).

A separate UTM VM with only the installed VirtIO disk cold-booted through
RedSea to the graphical desktop. `zstatus` reported `winmgr=1 gterm=1` and
nonblack scanout. `Tests/InstallerPersist.ZC` returned `0x2a` on the first
disk-only boot and again after a second cold reboot. The live
`CtrlAltFontDispatchChecks` also returned `0x2a`. The captured final desktop
screen is preserved at `build/utm-52c-installed-disk.png`; it shows the two
terminal windows and centered animated Z. `make check-home-autostart` and
`make check-gui-terminal` passed on this source, covering startup, menu actions,
zoom/pan, Paint Demo drawing, and Escape cleanup. The desktop-control bridge
returned a gray live capture during the final visual session, so no claim is
made that its latest menu or Escape interaction was observed through that
bridge.

Final local gates passed: `make check-home-autostart`, `make check-gui-terminal`,
`make check-iso-installer`, and `git diff --check`. The ISO installer gate
verified guarded erase, full read-back, byte-for-byte disk comparison,
disk-only desktop boot, and persistence before and after a second cold reboot.

## Pi 4B native desktop / basic input (2026-10-26)

**Accepted for PI-DESKTOP-2**, staged native kernel SHA-256
`b12497c8e4df1998966b2a46d4f1ed7c31363090c3a1363df0a6183993d4930b`.
IMG_9182 shows coherent graphical terminal output and executed `usbstatus`,
physical framebuffer 1920×1280/pitch 7680, packed logical 800×600/pitch 3200
scaled 2×, 1632 composed frames, and USB active/fault/stopped **1/0/0**.
The user then explicitly confirms visible cursor movement, working menu
clicks and **40+2;** returning **42**. This closes the PI-DESKTOP-1 stalled
desktop/basic input follow-up. Details and prior physical USB/FAT32 evidence
are in [PI4.md](PI4.md). The accepted M6 diagnostic recovery entry is retained.

The Pi image remains read-only for storage/source writes. This gate does not
accept writable save/reboot/reopen persistence, native SD, disconnect recovery,
Pi installation, Apple multitouch/secondary click, full splash-animation
behavior or complete ZealOS parity. No boot payload changed for this evidence
update; the previously passing platform/image tests remain applicable.

## Unchanged upstream programs in UTM-shaped QEMU (2026-10-26)

The first regular-program batch imports the complete, unchanged
`Demo/Graphics/NetOfDots.ZC` and `Demo/Graphics/Lines.ZC` from the existing
public ZealOS pin. Their file hashes are in `upstream/pinned/UPSTREAM.json`;
`make check-upstream` checks those bytes and the original StrB `PressAKey`
body used by the bridge. This is distinct from the legacy adapted bring-up
copies. Both programs are in the boot archive/source partition and have Apps
menu entries. The menu returns their include command to the original User
command loop, so compilation does not run inside the input-reader callback.

Running the originals exposed four concrete gaps: unbound `PressAKey`,
unimplemented `SignI64`, `DocClear` retaining an early no-op `DocPrint`, and
one-shot graphics being erased by the next compositor refresh. The runtime
now binds signed comparison-based `SignI64` (including I64_MIN), uses the
unchanged `PressAKey` body with native input, and reloads DocRecalcLib after
the real output writer. Retained foreground `gr.dc` now has its own
transparent 800×600 indexed surface, separate from compositor target `gr.dc2`.
Foreground lines use the existing CDC rasterizer with task coordinates and
visibility; source and fallback compositors retain the foreground stage.

The program gate passes with both NEC USB input and VirtIO input. It observes
actual menu selection and graphical-terminal inclusion, clears a document
sentinel, requires the red fan inside the program's own window across later
frames, verifies DCFill clears it after key exit, and runs `40+2; → 42`.
Lines must produce changing colored pixels within that window, then exit on
Escape and leave `6*7; → 42` working with `failed=0`. The color/animation
checks exclude the other terminal, menu transitions and cursor blinking.
`StockProgramIntrinsicChecks` also returns `0x2a` for direct and indirect
SignI64 calls over negative/zero/positive values and both signed extremes.

Logs: `build/check-stock-programs-nec-run.log`,
`build/check-usb-nec-tablet-stock-programs.log`,
`build/check-stock-programs-virtio-run.log`, and
`build/check-stock-programs-pci.log`. A captured retained fan is
`build/utm-stock-netofdots.png`. These are automated QEMU/HVF guest checks,
not a new eyes-on UTM app acceptance or physical Pi program acceptance.
The broader `make check-grscreen-full`, `make check-gui-workflow`, and
`make check-iso-installer` regressions also pass. The graphics gate covers
CDC lines, sprites, screen context/foreground roles, zoom, view controls and
final text composition. The NEC GUI workflow covers menus/paint, compiler
options/error recovery, source execution and System tasks, graphical editor
save/compile/run, cold reboot/reopen, Lattice controls and demo Escape returns.
The new ISO installs onto a disposable 128 MiB VirtIO disk with full read-back
and byte-identical comparison, then cold-boots from the disk alone. Saved
`InstallerPersist` returns `0x2a` before and after a second disk-only cold boot.
Logs: `build/check-stock-programs-graphics-regression.log`,
`build/check-stock-programs-workflow-regression.log`, and
`build/check-stock-programs-iso-regression.log`. All commands exited zero.
The prior live-accepted ISO `52c214c…` is preserved as
`build/zealos-aarch64-hello-accepted-52c214c.iso`.

The wider upstream Apps/Demo collection is still not bundled or accepted;
[PROGRAMS.md](PROGRAMS.md) records the program boundary. The previously
accepted physical Pi desktop image and diagnostic recovery were not rebuilt
or staged by this UTM follow-up.

Current program ISO SHA-256:
`95cb32ad3d28926adc8845e4def9532ae42ab49b24b4dfdabdfbc9626f55126b`.
Installed template disk SHA-256:
`c1e9d008b9cb531d8d4315992d3041194893a76d0b6039421c805ace76c41a96`.
Kernel SHA-256:
`52fda0313ab8d2855de2fece5a0bc6a8f29429373c9ef9c8596382d78de9a2e5`.
The **249-file** source archive SHA-256 is
`22ce6cb2e7a1315100f50d5ca66c024b446a9d56c81b139ded57334e73fe6c29`.
Both program files in that archive match the pinned originals byte-for-byte.
Artifact sizes/hashes and gate summary are in
`build/utm-stock-programs-artifacts.json`. Source integrity/ARM64 overlays and
`git diff --check` pass. The last live UTM app installer acceptance remains
the preserved `52c214c…` image; an eyes-on UTM launch of the two new Apps
entries on `95cb32ad…` is still pending.

## Collection expansion and interactive program work (2026-10-26)

All 294 files in the pinned upstream Apps/Demo trees are now bundled unchanged,
including their document and binary resources. The original `GrMath.ZC`,
`GrPrimatives.ZC`, `Math.ZC`, `DocGr.ZC` and `KDataTypes.ZC` are also pinned.
`make check-upstream` verifies those files and the unchanged R2P, PopUpColor,
QueueDel and QueueCount excerpts used by the bootstrap. Availability is separate
from program acceptance; see [PROGRAMS.md](PROGRAMS.md) for the application
inventory and first compilation blockers.

The native compiler now expands file/directory/line macros from the originating
file, including nested includes and macro expansion. `no_warn` accepts existing
local variables and arguments without executing them. Inline-array lowering now
keeps row addresses at pointer width and uses the complete trailing-dimension
stride. Guest checks cover dynamic signed byte member arrays, wide elements,
local and global structs, row addresses, a three-dimensional array and guard
values around the storage. The unchanged thick-line primitive also draws the
expected offscreen pixels instead of faulting in `GrFillCircle`.

The runtime adds Ceil, floating/unsigned square and normal Sin/Cos/Arg bindings.
Arg uses reduced atan arguments; R2P checks cover all four quadrants. The square
check uses exact result bits because decimal literal conversion can be one ULP
off (observed for `1.5625`); correctly rounded decimal input remains separate
compiler work. Popup formatting now preserves adjacent DolDoc commands, and
the load graph binds real form navigation and document editing rather than
their early forward stubs. Popup tasks receive the real DolDoc KeyDev echo
chain. Killing a child with teardown from an executing source body is allowed
while parser/executive callback guards remain in place.

Running the original Mass Spring exposed a frozen clock: TaskBridge's later
`tS` declaration shadowed the timer-backed cell. Each bridge load now binds its
new cell to the live timer. The boot-session elapsed-time check returns 42.
Native event tracing also found mouse events arriving from both the source
WinMgr and a preceding bootstrap router retained by a forward declaration.
The current router is now defined before its poll caller. The real pointer
regression receives exactly one down and one up for one injected click.

Doodle, Mass Spring and Life now pass their combined interactive gate through
both NEC USB keyboard/tablet input and VirtIO input:

- Doodle opens the original PopUpColor chooser, selects RED by an actual menu
  click, previews and retains its thick stroke across compositor refreshes,
  clears it on Escape and returns to a working terminal.
- Mass Spring accepts two mouse-placed masses and a right-dragged spring. The
  circles visibly approach one another under the original ODE simulation.
  Return clears the masses and spring; Escape restores window geometry and
  removes the drawing callback before the next terminal command.
- Life accepts mouse-drawn green cells, changes them through its background
  simulation and tears down that child on Escape before prompt recovery.

Logs: `build/check-stock-interactive-nec-final.log` and
`build/check-stock-interactive-virtio-final.log` (both exited zero). The full
graphics regression (`build/check-stock-collection-graphics-final.log`) and
NEC graphical editor/save/reopen/workflow regression
(`build/check-stock-collection-final-workflow.log`) also exited zero on this
kernel/source pair. The isolated application inventory passed and records
Budget and Vocabulary as compile-only; the other applications' first blockers
are documented in PROGRAMS.md. No complete-collection acceptance is implied.

The rebuilt ISO also passes installation through an NEC xHCI USB CD with USB
keyboard/tablet and VirtIO recovery input present. Unconfirmed `zinstall`
leaves the target unchanged. `zinstall ERASE` completes full read-back, and the
installed 128 MiB raw disk matches the template byte-for-byte. Two subsequent
cold boots have only that VirtIO disk attached. The desktop starts on each;
saved `Tests/InstallerPersist.ZC` loads and returns `0x2a` before and after the
second boot. The log is `build/check-stock-collection-iso-usb.log` (exit zero).
`make check-iso-installer-usb` repeats that media/input configuration. The
original NetOfDots/Lines gate also passes again on NEC USB and VirtIO input
(`build/check-stock-programs-nec-final.log` and
`build/check-stock-programs-virtio-final.log`, both exit zero).

Initial Logic ISO SHA-256 (superseded after the prompt failure below):
`3b59782c36e57b058670281cb5661d8b86f31212db637020237e1d65f980f4bb`.
Installed template disk SHA-256:
`a509c2f73880b93c1ec2bd55eff5f59168b0f543df909cb7d75b4d46b20f15aa`.
Kernel SHA-256:
`20c04e1f862d4211ad9bc6322d0d19b0adaa7268aa461b1986e2dc91d284e459`.
The **556-file** source archive SHA-256 is
`e498e8030cfcd20d61ecf0372095dfa1f43c8e8e5f4bdeaa4297e93e712f3091`.
All 294 Apps/Demo files in the built archive match the pinned manifest hashes.
Artifact sizes, hashes and gate summary are recorded in
`build/utm-stock-collection-artifacts.json`, with a committed snapshot at
[docs/acceptance/utm-programs-2026-10-26.json](docs/acceptance/utm-programs-2026-10-26.json).
Live UTM program acceptance is recorded below. The prior live-accepted
`52c214c…` ISO is still preserved. Pi boot media were not rebuilt or staged.

### Live UTM program evidence — 2026-10-26

The user booted the current program ISO in `ZealOS AArch64 ISO Installer
Acceptance` and reached the original Mass Spring and Doodle programs. The
18:33:29 screenshot shows mouse-placed masses connected by red springs; the
user confirmed the right-drag interaction. The 18:33:57 screenshot shows
Doodle's retained thick black/green strokes and XOR intersections, and the
user reports that Doodle seems good. The original screenshots are preserved
without edits in [Mass Spring](docs/images/utm-mass-spring-live.png) and
[Doodle](docs/images/utm-doodle-live.png).

The 18:34:38 and 18:35:01 screenshots show the same mouse-drawn Life pattern
changing from thin green lines into a different cellular pattern, and the user
explicitly confirms that it is evolving. Both originals are preserved:
[Life drawing](docs/images/utm-life-drawn-live.png) and
[Life evolution](docs/images/utm-life-evolved-live.png).

The user subsequently confirmed that all remaining checks pass: Mass Spring's
connected circles move, Return clears them, and Escape from each of Doodle,
Mass Spring and Life returns to a terminal where `40+2;` gives 42. This closes
live UTM acceptance for these three original programs on ISO `3b59782c…`, in
addition to their passing automated NEC/VirtIO checks. It does not accept the
rest of the Apps collection or claim a new live installer/disk-persistence run;
the exact current ISO's installation/persistence run above is automated QEMU.

## Digital Logic and document-resource includes — 2026-10-26

The unchanged `/Apps/Logic/Run.ZC` is available through Apps → Digital Logic.
The common compiler adapter now reads nested includes through DolDoc, retains
compiled inline binary constants after EOF, and closes owned documents on EOF
and error cleanup. Saved source files containing NUL bytes remain readable.
The document loader restores the repository's UTF-8 representation of CP437
binary resources, validates record bounds and raises a catchable exception
before locking a document with malformed data. The formatter adds original
`ListSub`-based `%z`; progress aliases share the actual progress records. The
bitmap writer, size reader and renderer use the original packed 17-byte bitmap
header, preserving transparent pixels. The Apps/Demo files themselves remain
unchanged and hash-checked.

The NEC USB and VirtIO Logic gates pass **49** and **47** logged checks.
They compare all ten original gate bitmaps, pixel for pixel including the
surrounding background, and exercise the original AND solver. Both then select
the actual Apps menu entry, type an AND truth table, verify its visible circuit,
exit to a working prompt, reopen Logic and solve an OR circuit with a different
gate image. The checks verify callback cleanup, window restoration and working
terminal output after exit and relaunch.
The automated AND circuit is preserved in
[the QEMU capture](docs/images/qemu-logic-and.png).

The document-input gate passes **40** checks: nested relative includes,
file/line macros, saved binary documents, compiled constant lifetime, malformed
resources both directly and inside an include, assignment/syntax error recovery
and later registry/script execution. Repeated failed loads leave managed
data/code usage and allocation count unchanged. The native decoder also passes
exact recovery of all **54** encoded Apps/Demo documents, all 256 CP437 byte
values and malformed/truncated payloads.

Regression gates pass: **275** full graphics checks (including bitmap packing,
sprite transforms/symmetry and full text presentation), **272** Window Manager
and MathODE frame checks, **421** NEC graphical editor/workflow checks including
save/compile/run/reboot/reopen, **60** Doodle/Mass Spring/Life interaction checks,
and **33** Net of Dots/Lines checks. The isolated compilation inventory passes
and records the remaining individual application blockers; compilation is not
runtime acceptance.

The exact candidate ISO passes **14** installation checks using an NEC USB CD,
native USB input and VirtIO recovery input. Unconfirmed `zinstall` leaves the
blank disk unchanged; `zinstall ERASE` completes full read-back verification.
The installed 128 MiB disk matches the template byte for byte. Two subsequent
cold boots have no CD attached; saved `Tests/InstallerPersist.ZC` loads and
returns `0x2a` on each. All reported test commands exit zero.

Current ISO SHA-256:
`804f42c2850f9e93c607402f187e879d060d7df6969fb64c3a3e090f5651ef2a`.
Kernel SHA-256:
`607acd3630238056ab7103b8174d17feb093a90e71339ee07e64572f161b8fec`.
The **560-file** source archive SHA-256 is
`022d388712d7771481d4e18d716c2d0f1fe258d6adb1cf3022983886efd82f55`;
all **294** Apps/Demo files match the pinned originals. The template disk hash
is `54fc196533a3ab4e6487d8afde921bbf71a0b129f9e24d0ea0bb8e885e6b406b`.
Artifact sizes, hashes and exact log paths are in
[the Logic report](docs/acceptance/utm-logic-2026-10-26.json), and the current
[compile inventory](docs/acceptance/program-catalog-2026-10-26.json) records
the first remaining blocker for each application.

The tested ISO has replaced `build/zealos-aarch64-hello.iso` atomically. The
live-accepted `3b59782c…` program ISO remains preserved alongside the older
`52c214c…` installer image. Live UTM confirmed the AND circuit, then exposed
failed prompt editing and a reported freeze. The original perfect-input gates
above did not cover these cases. The user confirmed deletion was inside a
prompt, not circuit editing. Evidence is preserved in
[the live AND capture](docs/images/utm-logic-and-live.png) and
[the reported prompt failure](docs/images/utm-logic-input-freeze-live.png).
Full live Logic acceptance remains pending. Pi media were not rebuilt or staged.

## Logic prompt editing and setup abort correction — 2026-10-26

The live report exposed gaps in the original perfect-input acceptance. The
bootstrap `StrGet` reader sent `Backspace, Space, Backspace` to a graphical
document rather than editing its entries. It also ignored cursor keys and
Escape. The graphical service now selects the original `DocGetStr2`, retaining
the early fallback for raw/no-document callers. Separately, Logic's setup
abort reached the declared but unimplemented `PutExcept` and set runtime
`failed=1`. The bridge now implements its reporting/catch contract; the test
also verifies `PutExcept(FALSE)` propagation. This reproduces a fatal setup
abort, without claiming the exact sequence behind the user's reported freeze.

The expanded NEC USB and VirtIO gates pass **106** and **104** checks. They
visibly edit `ANX` into `AND`, change a table value in the middle, Backspace
`0x3` into `0xC`, and forward-Delete an extra digit from an input. Both malformed
quote and closing-parenthesis values raise recoverable errors; pointer input
continues and a corrected value produces the circuit. Missing table, inputs
and outputs, plus Shift+Escape at the table prompt, each return to a terminal
where `40+2;` gives 42. Normal AND/OR rendering, exit and relaunch still pass.
The [corrected prompt capture](docs/images/qemu-logic-input-edited.png) shows
the actual framebuffer after Backspace and replacement, with no stale digit.

The Doodle/Mass Spring/Life interaction regression passes **60** checks. The
exact replacement ISO passes **14** USB-CD installer checks, complete read-back,
byte-identical installation and two cold disk-only boots returning `0x2a`
from the saved source file. The full NEC desktop/editor workflow passes
**421** checks, including graphical compiler recovery, edit/save/compile/run,
cold reboot/reopen, graphics and menu controls. All five test commands exit zero.

Corrected ISO: `build/zealos-aarch64-logic-prompt-fix.iso`, SHA-256
`9c5e90c964cbd1f0badd36d20b712e27e65947b129a2ab43ec4f79473180976c`.
The source archive SHA-256 is
`eafb017fc0b34274129d80ba4a668806169d0a62cd5dc0f76693387fb12f6543`;
all 294 Apps/Demo files remain unchanged. Kernel SHA-256 stays `607acd36…`.
The new template disk hash is
`0396bdce1da20746180acaeee79e19e19a8f33c82167a2f941f267709738db34`.
Exact logs, artifact sizes and full hashes are in
[the correction report](docs/acceptance/utm-logic-prompt-fix-2026-10-26.json).
The initial `804f42c2…` Logic ISO is preserved as
`build/zealos-aarch64-hello-logic-804f42c2.iso`. At build completion, live UTM
acceptance of the correction was pending. The existing live UTM session and Pi media were
not changed during this investigation.
The corrected ISO has also replaced `build/zealos-aarch64-hello.iso` atomically,
so a running VM retains its original open image rather than seeing a truncated
or partly rewritten CD.

### Corrected-image live UTM retest — 2026-10-26

After receiving the corrected `9c5e90c9…` ISO and the prompt-editing/exit
checklist, the user reported **“okay it works”** and attached a screenshot
timestamped 21:50:02 local time. The [live screenshot](docs/images/utm-logic-prompt-fix-live.png)
shows the expected AND circuit: inputs `000A` and `000C`, output `0008`.
The requested prompt editing, Escape exit, Shift+Escape setup cancellation,
pointer response and subsequent `40+2;` checks are accepted by user report;
those interactions are not independently visible in the still image.

This closes the Digital Logic live program retest for the corrected ISO. It
does not add a new live installation/persistence run or accept other programs
in the remaining collection. The report records the screenshot digest and
separates observed pixels from the user's confirmation. No ISO or Pi media
were rebuilt or staged while recording this result.

## Budget calendar, forms and persistent entries — 2026-10-27

The next application candidate is `build/zealos-aarch64-budget.iso`, SHA-256
`00b5cd349b6da41edeba525066268df4795bb0c8df6f3818f40d04a22b2d9649`.
Apps → Budget runs its original entry and data workflow. The unchanged pinned
sources are complemented by a guarded, generated correction to Budget's comma
separator comparison and a seeded copy of its original accounts document.
The complete pinned Apps/Demo mirror still has 294 unchanged files; this does
not imply acceptance of the rest of the collection.

Calendar support loads the original source date/scanning/formatting helpers
with bounded ARM64 adaptations. The boot wall clock comes from Limine and
advances using the generic counter; the current default is UTC. Missing boot
time raises a catchable error. Original DocForm rendering uses an explicit
native class-field metadata interface instead of assuming the two compilers'
internal structures share a layout. The compiler now retains member metadata,
infers `lastclass` from the actual argument type and preserves its default
string after compilation.

Repeated Budget loading exposed a forward declaration still calling its old
implementation and freed global data. Forward aliases now follow a compatible
later definition in the same task, while native adapters retain their ABI
boundaries. A separate expression fix prevents an address argument from
turning a subsequent implicit function call into a function pointer. DolDoc's
numeric lexer also retains full hexadecimal pointer values for editable Budget
rows. Focused fixtures exercise these cases, malformed metadata recovery and
integer/date parsing without managed-memory growth.

The NEC USB Budget gate passes **115** checks. It creates a dated 12.50 entry,
saves and reopens it, edits the amount to 25.00 then aborts without altering
saved data, cancels a new form without adding an entry, and reopens the saved
entry after a cold disk-only boot. Escape accepts the form; Shift+Escape
cancels it. In the main view, Escape saves and exits and Shift+Escape aborts
changes. Each tested exit restores a working terminal returning 42.
The [filled form](docs/images/qemu-budget-form.png) and
[cold-reopened entry](docs/images/qemu-budget-entry.png) are framebuffer captures
from this passing run.

The expanded VirtIO Digital Logic regression passes **104** checks; NEC
Doodle/Mass Spring/Life interaction passes **60**. The exact Budget ISO passes
**14** USB-CD installer checks, including complete read-back, a byte-identical
installed disk and two subsequent cold disk-only boots returning `0x2a` from
saved source. The full NEC desktop/editor workflow passes **421** checks,
including compiler/task recovery, edit/save/compile/run/reboot/reopen,
interactive Lattice/Stock Play and menu/window controls. All six guest test
commands exit zero. The compile inventory now includes TimeClock as compilable;
its runtime workflows remain unaccepted. Original-source, guarded-overlay,
resource-byte and Python syntax checks also pass.

Kernel SHA-256 is
`0d150179ead170f4387e4aa06027f6eb522f9fe9669dea83eacaab6853836e0c`.
The **575-file** source archive SHA-256 is
`d71865677c623bc33e0097504ed3cb4fa8c952fbde957555db754433d3ee5cc7`.
The template disk hash is
`01027bd014dbff21b250c3fb7325f35c03109bdbb77b006fa483e40be058d671`.
Full artifact hashes, checks and limitations are in
[the Budget report](docs/acceptance/utm-budget-2026-10-27.json); the updated
[compile inventory](docs/acceptance/program-catalog-2026-10-27.json) records
individual blockers.

Live UTM Budget testing subsequently failed: the user reported frozen input
with File/Abort/Exit visible in the empty ledger. Hovering File → Abort in a
separate QEMU guest reproduces an unresolved external call from DrawMenu and
sets `failed=1`. The transaction-only gate above did not exercise this path.
The [live screenshot](docs/images/utm-budget-menu-freeze-live.png) and report
record that failure; the screenshot alone does not establish loss of input.
Periodic/template, account-file editing
and account-filter workflows have not been accepted. Saving requires an
installed writable VirtIO source volume. The standard ISO path retains the
live-accepted `9c5e90c9…` Logic image; the Budget candidate has its own path.
The existing UTM session and Pi media were not changed.

## Budget File/Edit/View menu correction — 2026-10-27

The failed live image `00b5cd34…` called unresolved keyboard helpers when
`DrawMenu` tried to label File → Abort. Its small document lexer also lacked
quoted character literals and evaluated named key constants as zero. This
discarded the Edit/View menu tree and made the surviving exit actions inert.
The QEMU hover reproduction exits nonzero with `failed=1`; its log is linked
from [the correction report](docs/acceptance/utm-budget-menu-fix-2026-10-27.json).

The corrected image loads portable original keyboard translation/name helpers
and `CatPrint`, with equivalent source byte arrays for the original assembly
tables. PS/2 I/O code is not loaded. A guarded change initializes the original
key-name buffer and bounds its printable-character lookup. The document lexer
reads quoted/escaped character literals and literal-valued compiler defines
through an explicit borrowed-text interface, including trailing whitespace.
This does not add general expression-valued define evaluation.

The expanded NEC USB Budget gate passes **140** checks. It verifies the
original key maps and key names, character escapes and named constants, then
hovers the actual File/Edit/View menu items while checking that presentation
continues with `failed=0`. Edit → New Entry opens the real form, File → Exit
saves and returns to a working terminal, and File → Abort discards changes.
Edit/abort, form cancellation, reopen and a cold disk-only persistence boot
also pass. The [menu capture](docs/images/qemu-budget-menu-exit.png) shows
File/Edit/View and the Exit shortcut over the saved entry.

VirtIO Digital Logic's expanded regression passes **104** checks. The exact
corrected ISO passes **14** USB-CD installer checks, full read-back,
byte-identical installation and two cold disk-only persistence boots.
The full NEC desktop/editor regression passes **422** checks, including task,
compiler and exception recovery, edit/save/compile/run/cold reboot/reopen,
Lattice/Stock Play and menu/window controls. All four commands exit zero.

Replacement ISO: `build/zealos-aarch64-budget-menu-fix.iso`, SHA-256
`c92f5f3cc58cd005277add43b3601b0089b8756b34e4ee4d7f134043d9bca30c`.
Kernel SHA-256:
`0e4fe8f42058211fb4ea9c545447e0b75103ea437313b9b3095e13ef2de59cfc`.
The **578-file** source archive SHA-256 is
`bfaf6708ef04b52446355290c55c5b67a0ec1e6c06362c15c6b5e4bfef672ffa`.
The template disk hash is
`1a8cfe7f77df71d88a3d1b16eac40379f28b74ebf4f3f10e32d9d1de12e49de0`.
Exact artifact sizes, hashes, logs and the scope of live evidence are recorded
in the correction report. The failed Budget ISO and prior live-accepted Logic
image remain preserved. Live UTM retesting of this replacement and advanced
Budget workflows remain pending. No running UTM session or Pi media changed.

### Follow-up PCI form check — 2026-10-27

A later fresh PCI run of `--probe-budget-program` still did not open the
transaction form. The automated workflow selected Edit → New Entry and then
sent the documented `n` shortcut; neither produced the form labels. The
blank ledger remained responsive, with terminal focus and advancing desktop
frames, so this run isolates the failure to Budget's entry-form path rather
than a whole-guest lockup. Calendar, account/data storage, class reflection,
and menu-hover fixtures still passed. This newer failure supersedes the
earlier automated form-persistence result for the current candidate; Budget
is not counted as finished. Log: `build/check-budget-pci.log`.

The 3D mesh, interpolation, formatter and service fixtures also pass. The
GrModels app workflow gate compiles the app but does not yet pass its form and
mesh-generation interaction checks. KeepAway and Titanium compile, but the
interactive games still need acceptance.

### Named-app catalog rerun — 2026-10-27

The fresh isolated catalog now compiles KeepAway and Titanium after adding
session-only high-score globals, and compiles X-Caliber after its normal
loader initializes registry globals. Span, Strut, TimeClock, Vocabulary,
Budget, GrModels and Logic also compile in their catalog paths. This is
compile/load evidence, not interactive acceptance. Psalmody now reaches the
missing `Ona2Octave` helper after the text-search/reward compatibility hooks;
ToTheFront's normal loader still aborts in the ARM64 compiler on its
runtime-sized global arrays. The full per-entry output is saved in
`docs/acceptance/program-catalog-2026-10-27.json`. Psalmody audio hooks remain
silent, and Vocabulary has no dictionary data bundled.

### Shared ARM64 DocForm correction and form acceptance — 2026-10-27

The Budget and GrModels form failures shared a translator defect. The ARM64
`DocForm` source rewrite replaced the original class lookup but left its
following `return FALSE` in place, so the translated function returned before
creating any form. The rewrite now consumes the original condition and its
failure return together. `scripts/check-arm64-source.py` asserts that the
generated guard flows directly into `doc = DocNew`.

The fresh PCI Budget workflow now passes form creation, field edits, save and
abort, cancel, reopen, cold disk-only persistence, and clean guest status. The
fresh `--probe-3d-apps` run passes the GrModels Man and Ball form workflows,
mesh generation (Ball reports 58 vertices), clean terminal recovery, and the
3D fixtures. Its first KeepAway run exposed an unresolved `Noise` import in
the animation task. The ARM64 runtime now binds `Noise` as a silent no-op after
loading `Externs.ZC`, and makes `Sound` a silent no-op because this target has
no audio device. A service-fixture regression calls both hooks. The rerun
renders the KeepAway court and players, keeps the frame active through Space
and mouse input, returns on Escape, and passes `40+2;` plus `zstatus`.

Titanium compiles and reaches its help page, but the automated PCI keypress
does not advance its `PressAKey` wait. Its gameplay and normal exit remain
unverified and need a live UTM check. These are QEMU PCI checks, not live UTM
acceptance.

The rebuilt combined ISO is `build/zealos-aarch64-hello.iso`, SHA-256
`8f0bc3f474e7fc09c0692051d4d8efbab1c4b47bfbf7363bc652dcc2295b3275`. The
3D probe details are in `build/3d-apps-compile.json` and
`build/check-3d-apps-pci.log`. The source translation regression is checked by
`python3 scripts/check-arm64-source.py`.
