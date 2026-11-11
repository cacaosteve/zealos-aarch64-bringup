# Regular ZealOS programs on ARM64

The desktop, compiler and installer milestones do not establish that every
regular ZealOS application works. Programs are imported from the same public
ZealOS commit as the system sources:
`eee210f4379ba3d00b47507f336a158eefb2c993`. Exact file hashes are recorded in
`upstream/pinned/UPSTREAM.json` and verified by `make check-upstream`.

## Broader compile inventory (2026-11-10)

A targeted PCI QEMU compile-only sweep covered 290 selected Apps and support
sources: 193 compiled and 97 did not compile in isolation. All 20 selected
`/Apps` entrypoints/components and all 16 selected `/Demo` sources compiled.
The broader `/Home` group compiled 59 of 65 entries; its six blocks are the
x86 SSE diagnostic, three legacy x86 PCI NIC drivers, the PCNet-only test, and
the JSON utility's use of a compiler-private `LexExcept` API that the ARM64
native compiler does not expose. The remaining blocked files are mostly
compiler internals and system/build modules whose dependencies or hardware
bindings are not present in the standalone compile context. They need separate
triage; this count does not mean 97 user-facing programs are missing.

This is compile evidence only. It does not establish runtime acceptance or
feature parity. Per-source output is recorded in
[`program-catalog-full-qemu-2026-11-10.json`](docs/acceptance/program-catalog-full-qemu-2026-11-10.json).

The refreshed compile sweep on the current ISO (`f17ed84e…`) reproduced the
same 193/290 result; the current-image manifest and per-file output are in
[`program-catalog-full-qemu-current-2026-11-10.json`](docs/acceptance/program-catalog-full-qemu-current-2026-11-10.json).

The dependency-aware Demo inventory is separate from that 290-source sample:
all 206 pinned Demo `.ZC` files compile, and 202 have explicit QEMU runtime
acceptance; four remain partial. Those are the two TOS account examples
(`TOSDistro` and `TOSMisc`) and `MultiCore/Lock`/`MultiCore/MPRadix`. The
multicore examples are partial because the current guest runtime exposes one
CPU; the TOS account examples still have workflow coverage gaps. The ARM64
`TimeIns` adaptation has runtime acceptance, while the pinned stock source's
optimizer `PassTrace` diagnostic is not implemented. `TOSExtern` is accepted
at its interface-module scope: its forms and external declarations load and
resolve, while the external routines and dependent workflows remain
unverified. These are source entries, not necessarily four independently
launchable apps. The installed
`WallPaperDesktop` workflows now pass QEMU coverage for all six controls,
palette save/reopen, Home and Terminal shells, NetLog, and a local Telnet
round-trip; live UTM and Pi behavior remain separate checks. See
[`demo-wallpaper-desktop-current-qemu-2026-11-10.json`](docs/acceptance/demo-wallpaper-desktop-current-qemu-2026-11-10.json).
The read-only `DiskRaw` workflow is now counted as accepted for its tested
VirtIO fixture; general block-device registration, writes, filesystem mounts,
hotplug, Pi storage and live-target behavior remain separate storage gaps.
See the [per-source ledger](docs/acceptance/demo-source-ledger-2026-11-10.json).
The 206/206 compile-only sweep was rerun against the current ISO
(`f17ed84e…`); see the [current-image compile record](docs/acceptance/demo-source-catalog-current-qemu-2026-11-10.json).

## Unchanged graphics programs

The source bundle now contains these original files, with no program edits:

| Program | Launch from a graphical terminal | Exit |
|--|--|--|
| Net of Dots | `#include "/Demo/Graphics/NetOfDots"` or Apps → Net of Dots | Press a character key at PRESS A KEY |
| Lines | `#include "/Demo/Graphics/Lines"` or Apps → Lines | Press a character key or Escape |
| Doodle | Apps → Doodle | Escape |
| Mass Spring | Apps → Mass Spring | Return resets; Escape exits |
| Game of Life | Apps → Game of Life | Escape |
| Digital Logic | Apps → Digital Logic | Press a character or Escape after the circuit is drawn |

`make check-stock-programs` runs both through the graphical terminal with
UTM's NEC USB controller model. It checks actual menu selection, visible
drawing/animation, removal of previous document text, key exit, a working
prompt afterward and signed integer intrinsic boundary cases. The alternate
VirtIO input path is checked with:

```sh
python3 scripts/check-compat.py --pci --probe-stock-programs
```

`make check-stock-interactive` exercises Doodle's original color picker and
retained thick stroke, Mass Spring's mouse placement, spring creation, moving
masses and reset, and Life's drawing and background evolution. It also checks
that Escape restores a working terminal and that one actual pointer click
delivers exactly one down and one up. These three Apps entries select a light
task surface so the original black ink remains visible. Their upstream source
files are unchanged. The same checks can use VirtIO input with:

```sh
python3 scripts/check-compat.py --pci --probe-stock-interactive
```

The fan must remain visible in its own window across compositor redraws and
clear on exit. Lines must produce changing colored pixels inside that same
window. Red text in another terminal, menu changes and cursor blinking do not
satisfy those checks. The runtime keeps the upstream foreground (`gr.dc`)
separate from the compositor target (`gr.dc2`) so immediate-mode program
drawing can persist while the desktop refreshes.

The old `netdots`/`lines` bring-up commands use separately adapted legacy
sources. They are not the unchanged programs above. This gate covers drawing,
input and normal exit, not pixel-for-pixel graphics parity or arbitrary
application compatibility. Doodle, Mass Spring and Life also pass live UTM
drawing/input, simulation, reset where applicable, and Escape-to-terminal
checks on ISO `3b59782c…`. Physical Pi acceptance remains separate.

`/Demo/Games/RainDrops.ZC` is now runtime-accepted in the QEMU ARM64 graphical
terminal. Its guarded ARM64 bundle overlay paints the canvas white, matching
the original demo's empty-space color; the probe confirms the red geometry,
moving blue drops, Escape cleanup and terminal recovery. See
[`demo-raindrops-qemu-2026-11-02.json`](docs/acceptance/demo-raindrops-qemu-2026-11-02.json).

`/Demo/Games/Digits.ZC` also passes a QEMU ARM64 interaction probe: instructions,
the first pattern, guess input, invalid-answer score/retry, Escape and terminal
recovery. See [`demo-digits-qemu-2026-11-02.json`](docs/acceptance/demo-digits-qemu-2026-11-02.json).

`/Demo/Games/ElephantWalk.ZC` now passes a QEMU ARM64 graphical probe. The
guarded bundle overlay supplies the framebuffer drawing context and explicit
symmetry context; the probe verifies the grayscale sprite, Right-key redraw,
Escape cleanup and terminal recovery. See
[`demo-elephant-walk-qemu-2026-11-02.json`](docs/acceptance/demo-elephant-walk-qemu-2026-11-02.json).

`/Demo/Games/FlapBat.ZC` now passes a QEMU ARM64 runtime probe for its game
canvas, animation, Space flap, Escape cleanup and terminal recovery. The
scripted gameplay path did not trigger a collision sound, so FlapBat's collision
effect remains unverified. A 2026-11-09 QEMU VirtIO-sound follow-up captured
nonzero audio during gameplay and verified that the song task and tone stop
after Escape; it does not establish musical fidelity or live UTM/Pi behavior.
See [`flapbat-audio-qemu-2026-11-09.json`](docs/acceptance/flapbat-audio-qemu-2026-11-09.json).
The VirtIO driver and stock `Sound()`/`Beep()` paths are tested separately; see the sound records below and
[`demo-flapbat-qemu-2026-11-02.json`](docs/acceptance/demo-flapbat-qemu-2026-11-02.json).

`/Demo/Graphics/32BitPaint.ZC` now passes a QEMU ARM64 interaction probe for
its color HUD, mouse drawing, End clear, Shift+Escape cleanup and terminal
recovery. See [`demo-32bit-paint-qemu-2026-11-02.json`](docs/acceptance/demo-32bit-paint-qemu-2026-11-02.json).

`/Demo/Graphics/Symmetry.ZC` now passes a focused PCI QEMU runtime check. The
bootstrap launches its blocking `MessageGet` loop in a separate focused task;
the probe confirms pointer strokes appear on both sides of the symmetry axis,
Escape returns to the graphical terminal, and `zstatus` reports no failed
tasks. This is QEMU evidence; live UTM and Pi behavior remain unverified. See
[`demo-symmetry-qemu-2026-11-05.json`](docs/acceptance/demo-symmetry-qemu-2026-11-05.json).

`/Demo/Graphics/Pick.ZC` now passes a focused PCI QEMU interaction check. The
probe verifies its white canvas and sprites, a pointer drag that changes pixels
in the selected sprite region, Escape recovery to a terminal where `40+2;`
returns 42, and no failed tasks. Live UTM and Pi behavior remain unverified.
See [`demo-pick-qemu-2026-11-05.json`](docs/acceptance/demo-pick-qemu-2026-11-05.json).

`/Demo/Graphics/CommonAncestor.ZC` now passes a focused PCI QEMU check. The
probe confirms the generated colored tree renders, a key exits its `PressAKey`
wait, and the terminal accepts `40+2;` with no failed tasks. Live UTM and Pi
behavior remain unverified. See
[`demo-common-ancestor-qemu-2026-11-05.json`](docs/acceptance/demo-common-ancestor-qemu-2026-11-05.json).

`/Demo/Lectures/MiniGrLib.ZC` now passes PCI QEMU scanout and recovery: its
generated framebuffer image changed 42,483 scanout pixels, then the graphical
terminal returned and evaluated `40+2;` as 42. Live UTM and Pi behavior remain
unverified. See
[`demo-mini-grlib-qemu-2026-11-07.json`](docs/acceptance/demo-mini-grlib-qemu-2026-11-07.json).

`/Demo/Disk/DataBase.ZC` now passes a focused PCI QEMU form and storage round
trip: it saves account 0, reopens the original source, reads both fields from
disk using the shared ARM64 `ClassRep` bridge, and returns to a working
terminal. A QEMU check also reopened both fields after a cold boot from the
same virtual disk. The bridge adapts the compiler metadata layout and treats
some unresolved function-pointer metadata conservatively; complete reflection
parity, live UTM and Pi behavior remain unverified. See
[`demo-database-qemu-2026-11-07.json`](docs/acceptance/demo-database-qemu-2026-11-07.json).

The ARM64 compiler now emits matching 32-bit exclusive loads and stores for
locked updates, and zero-extends unsigned 16- and 32-bit operands. A
UTM-framework QEMU probe reproduced the previous 32-bit carry and unsigned
16-bit failures, then passed the corrected pointer checks and an existing
64-bit check. A follow-up fixed the separate locked prefix/postfix update
path and passed combined 16-, 32-, and 64-bit pointer checks. The exclusive
store now uses distinct status and data registers, retries from the stable
address, and its load has the acquire encoding; generated instructions are
checked in the same regression. See [the lock-width record](docs/acceptance/lock-width-utm-qemu-2026-11-09.json).
The parser now marks the current locked statement rather than preceding
instruction-list nodes. A nested function-call argument and neighboring
braced/unbraced lock statements pass generated-code and functional checks;
expression results are published after the exclusive store. A later test found
global locked updates lacked exclusive instructions; address normalization
now covers tested globals and address-exposed stack locals as well as pointers.
This single-vCPU test does not establish SMP contention behavior.

## Digital Logic and embedded resources

`make check-logic-program` runs the original `/Apps/Logic/Run.ZC` from the Apps
menu with NEC USB input. The same test also passes with VirtIO input:

```sh
python3 scripts/check-compat.py --pci --probe-logic-program
```

The gate compares all ten original gate bitmaps against their saved pixels,
checks the original AND solver, then types through the actual application
prompts and checks the visible circuit, Escape, window restoration and a
subsequent `6*7;` answer. It then reopens Logic, solves an OR circuit and checks
that the gate changes and exits cleanly again. It also verifies visible
Backspace/forward Delete and middle-of-line edits, malformed numeric input
recovery, all three missing-value aborts and Shift+Escape. Each exit must
restore terminal geometry, remove the drawing callback and accept `40+2;`.
Live UTM confirmed the AND
circuit, but the initial `804f42c2…` image failed prompt-editing/abort checks:
Backspace left stale text and an early abort reached an unimplemented
`PutExcept`. On 2026-10-26 the user confirmed the requested corrected-image
retest works. The new screenshot confirms the expected AND circuit; prompt
editing, setup cancellation and terminal recovery are accepted by user report.
See [the live acceptance record](docs/acceptance/utm-logic-prompt-fix-2026-10-26.json).
The accepted program-test ISO is `build/zealos-aarch64-logic-prompt-fix.iso`
(`9c5e90c9…`), also promoted to `build/zealos-aarch64-hello.iso`. It can run the
program directly from its USB CD without installation.
For the small live example, enter `AND`, Return, then another Return to finish
the gate list. Enter `0x10` for table size; inputs `0xC`, `0xA`, then an empty
line; output `0x8`, then an empty line. The result is a two-input AND gate.
During setup, Shift+Escape returns an empty value. At the table-size prompt
this aborts setup; at an input/output prompt an empty value finishes that list.
Plain Escape exits the circuit view, but does not cancel a setup prompt in the
original document reader.

![Original Digital Logic AND circuit in the automated QEMU test](docs/images/qemu-logic-and.png)

Nested source includes now use the same DolDoc reader as `ExeDoc`. They retain
embedded binary constants after an include reaches EOF and close owned
documents on normal completion and compiler recovery. The common document
loader recovers the UTF-8 representation of CP437 binary tails found in 54
pinned Apps/Demo documents; it preserves native packed files and source text.
The bitmap writer, size reader and renderer use the original packed 17-byte
bitmap header and preserve transparent pixels. This does not establish support
for every other serialized sprite opcode.

`make check-doc-bytes` checks exact resource decoding and malformed inputs.
`make check-doc-includes` checks saved binary documents, relative nested
includes, file/line provenance, constant lifetime, catchable malformed data and
repeated failed compilations without managed-memory growth. Application source
should use the graphical terminal's normal `#include`/`ExeFile2` path; the
bootstrap `zload` command does not itself decode a DolDoc root file.

## Budget candidate

Budget now has the calendar, clock, string scanning and class-form services
needed by its original entry/data code. Apps → Budget opens its default
`~/Budget` accounts. Click a field to edit it; Escape accepts a form and
Shift+Escape cancels it. In the main Budget view, Escape saves and exits;
Shift+Escape exits without saving changes. The clock defaults to UTC.

`make check-budget-program` checks calendar boundaries and formatting, the
boot wall clock, class metadata and inferred class names, scanner cleanup,
and malformed-source recovery. Its visible workflow creates an entry, edits
and aborts a change, cancels a new form, saves, reopens and checks the data
after a cold disk-only reboot. The NEC USB gate passes 115 checks on
`build/zealos-aarch64-budget.iso` (`00b5cd34…`). The exact ISO also passes
USB-CD installation, full read-back and two disk-only persistence boots.
Live UTM then exposed a menu-hover freeze in that image. The replacement is
`build/zealos-aarch64-budget-menu-fix.iso` (`c92f5f3c…`), which loads missing
key-name services and parses the original File/Edit/View shortcuts.
Its expanded NEC USB gate passes 140 checks, including actual shortcut hover,
menu selection and cold disk-only entry persistence. See
[the correction report](docs/acceptance/utm-budget-menu-fix-2026-10-27.json).
The current combined Logic/Budget/3D image is at
`build/zealos-aarch64-hello.iso`; its shared `DocForm` correction is described
below. Live UTM acceptance of the corrected Budget workflow is still pending.
A later integrated-candidate check covers daily and monthly template expansion,
purge and periodic copies at the data level, including a year boundary; see
[the Budget template check](docs/acceptance/budget-integrated-template-qemu-2026-11-09.json).
A direct data-routine check also writes, reopens and expands a monthly template
after a cold guest reboot; an empty-disk negative control fails as expected.
A separate NEC USB test creates an interval template through Budget’s forms,
saves it and reopens it after a cold boot. Template modification/deletion is
covered below; account-file editing and account-filter UI remain unverified,
and live UTM acceptance remains open.
The follow-up [template edit/delete acceptance](docs/acceptance/budget-integrated-template-qemu-2026-11-09.json)
now covers changing the end date, amount and description, saving and reopening
the edited template, deleting it with Ctrl-Y, and verifying that deletion
survives a cold disk-only reboot. Account-file editing, account-filter UI and
the older reported live UTM freeze still need separate checks.

Budget records dated transfers between accounts and displays a balance.
An empty white ledger is expected before the first transaction. Choose
Edit → New Entry or press lowercase `n` to open the Date/Amount/accounts/Desc
form. A small example is amount `12.50`, credit `BANK`, debit `FOOD` and a
description; the default date `*` means the current date. Escape accepts the
form, or Shift+Escape cancels it. File → Exit saves and returns to the terminal;
File → Abort returns without saving changes.

The pinned Budget files remain byte-identical to upstream. The generated
`BgtStrs.ZC` corrects one separator comparison from a two-byte comma/space
literal to a comma, so account names do not gain a comma when reopened.
The factory image seeds the original `Accts.DD` in `Home/Budget`; this does
not implement the general directory creation or copy APIs used by upstream's
installer. Saving requires a writable installed VirtIO source volume.

![Budget entry reopened after a cold disk-only boot](docs/images/qemu-budget-entry.png)

## Current named-app status — 2026-11-02

A separate [installer candidate](docs/acceptance/candidate-installer-utm-qemu-2026-11-09.json) includes the current document and popup fixes. It passed an automated USB-CD install, full disk read-back and disk-only persistence, plus the 3D gate using UTM's QEMU framework and `virtio-ramfb`. The Pi desktop kernel builds, but this candidate has not been tested in the live UTM UI or on the Pi.

The matching [Pi desktop payload](docs/acceptance/pi-desktop-candidate-2026-11-10.json) was rebuilt from commit `0299bd32`, passed its boot-chain validator and host platform tests, and was staged, hash-verified, and safely ejected on 2026-11-10. It is awaiting a physical boot check. Keyboard and pointer input were already accepted on an earlier Pi desktop image; the refreshed desktop state needs confirmation, and HDMI audio can be checked with `Beep(60);`.

The follow-up [ToTheFront-fix candidate](docs/acceptance/tothefront-installer-utm-qemu-2026-11-09.json) includes the player-2 visibility correction in both its live bundle and installed disk. Its USB-CD installer, disk-only persistence and uninstrumented ToTheFront artillery workflow pass using UTM's QEMU engine. A matching Pi payload has been prepared, but neither candidate has been checked in live UTM UI or on physical Pi hardware.

The pinned upstream `src/Apps` and `src/Demo` mirror contains 294 files,
including source, documents and binary resources. It is complete and
hash-checked; that file count is not a count of supported programs. This
table tracks the named Apps currently in scope alongside Logic. A compile or
normal-load result is not runtime acceptance.

| Application | Current verified status |
|--|--|
| Digital Logic | The exact current ISO (`f17ed84e…`) draws an AND circuit and passes prompt editing, deletion, abort/cancel cleanup, relaunch, and clean exit in UTM's bundled QEMU profile; `zstatus` reports no failures. This is automated QEMU evidence, not a visible UTM-window retest ([exact-current named-app checks](docs/acceptance/named-apps-exact-current-f17-utm-qemu-2026-11-10.json)). |
| Budget | The exact current ISO (`5effe8cb…`) passes calendar/clock/scanner setup, File/Edit/View keyboard shortcuts, entry creation/edit/cancel/abort/save/reopen, cold disk-only persistence, and terminal recovery (`40+2` -> 42, `zstatus failed=0`) under UTM's bundled QEMU profile ([exact-current acceptance](docs/acceptance/budget-exact-current-utm-qemu-2026-11-10.json)). This is headless QEMU, not a visible UTM app-window retest; the earlier live freeze report remains open. Interval templates can be created, edited, deleted, and verified across cold disk-only boots in the [NEC USB template UI run](docs/acceptance/budget-integrated-template-qemu-2026-11-09.json). Account-file editing and account-filter UI remain open. |
| GrModels | The current ISO (`923ae07d…`) passes Man and Ball forms, mesh generation, and terminal recovery under UTM's bundled QEMU with `virtio-ramfb`; Man generated 112 vertices/168 triangles and Ball 58/128 with an inline sprite ([current-image check](docs/acceptance/three-d-apps-current-utm-qemu-2026-11-11.json)). The user reported no visible model after selecting colors in the live UTM window. Automated mesh/raster checks do not resolve that visual discrepancy. At `Do another(y or n)?`, enter `n` to finish the current model and return. |
| KeepAway | The rebuilt ISO (`36969fff…`) passes gameplay, HUD/input checks, a 186-second full-game run with 18 repeated input cycles, Escape recovery, terminal focus, and animation-task cleanup under UTM's bundled QEMU ([full-game check](docs/acceptance/three-d-apps-full-game-clean-iso-qemu-2026-11-11.json)). Earlier one/two-vCPU runs and input stress also pass in automated QEMU ([one-vCPU](docs/acceptance/keepaway-full-current-qemu-2026-11-10.json), [two-vCPU](docs/acceptance/keepaway-current-smp2-qemu-2026-11-10.json), [input stress](docs/acceptance/keepaway-smp2-input-stress-qemu-2026-11-10.json)). The user reported a complete freeze in the visible UTM session; this separate automated run did not reproduce it, so live UTM acceptance remains open. ARM64 `Beep()` emits a VirtIO tone and `SoundReset()` stops it. The pinned `Noise()` and `Sweep()` tasks produce PCM in isolated QEMU captures; effect timing and live UTM/Pi audio remain unverified; see [sound effects](docs/acceptance/sound-effects-qemu-2026-11-09.json) and [full-game audio](docs/acceptance/keepaway-audio-qemu-2026-11-09.json). |
| Psalmody | Exact current ISO (`f17ed84e…`) opens the editor, records H as C5, captures C5 in three consecutive 50 ms UTM-bundled QEMU VirtIO WAV windows, accepts Shift+Escape during playback, and returns to a working terminal with 10 audio-timer IRQs; see [current sound acceptance](docs/acceptance/psalmody-heap-fix-utm-sound-2026-11-10.json). Live UTM app audio and broader song editing remain unverified. |
| Oh Great (Demo) | Exact current ISO (`f17ed84e…`) completes the long song after a normal key under UTM's bundled QEMU profile, returns to the graphical terminal, and captures 4,168,334 nonzero WAV samples over 47.30 seconds; see [current sound acceptance](docs/acceptance/ohgreat-heap-fix-utm-sound-2026-11-10.json). The user reported the song playing through Pi 4B HDMI; the exact card image provenance was not verified. Live UTM audible playback remains unverified. |
| Span | The exact current ISO (`f17ed84e…`) opens the graphical app/help and bridge canvas, draws a frame, and recovers from Shift+Escape. Save/reopen and cold-reboot persistence passed in an earlier QEMU run ([save/reopen check](docs/acceptance/span-save-reopen-qemu-2026-11-09.json)); full bridge behavior and live UTM/Pi interaction remain unverified. The current-image check is in [the named-app record](docs/acceptance/named-apps-exact-current-f17-utm-qemu-2026-11-10.json). |
| Strut | The exact current ISO (`f17ed84e…`) opens the selection prompt, accepts input, draws graphics, and exits to a terminal where `40+2` returns 42; `zstatus` reports no failures ([current named-app check](docs/acceptance/named-apps-exact-current-f17-utm-qemu-2026-11-10.json)). Earlier QEMU evidence covers connected masses, Space play-mode, and a digit-controlled thruster ([spring/thruster checks](docs/acceptance/strut-spring-qemu-2026-11-09.json)). Full physics and live UTM/Pi interaction remain unverified; the pinned app exposes no save/open command. |
| TimeClock | The exact current ISO (`f17ed84e…`) saves and rereads punch-in/out descriptions, then returns to a working terminal; `zstatus` reports no failures ([current named-app check](docs/acceptance/named-apps-exact-current-f17-utm-qemu-2026-11-10.json)). The older graphical probe's report-state round-trip also passed. The red `IN`/`OUT` report labels are below the probe's monochrome OCR threshold, so it asserts saved descriptions instead. Live clock behavior remains open; CMOS hardware clock access is unavailable. |
| Titanium | The current ISO (`923ae07d…`) passes intro/gameplay, Space, Escape, and terminal recovery under UTM's bundled QEMU ([current-image check](docs/acceptance/three-d-apps-current-utm-qemu-2026-11-11.json)). The user confirmed ordinary Left/Right, Space, and Escape work in the live UTM window with a rebuilt ISO. The VirtIO keyboard bridge updates both merged and raw scan-code bitmaps. A later gameplay run captured PCM at pitches consistent with Titanium effects in QEMU; exact effect timing and live UTM/Pi audio remain unverified. See [the audio check](docs/acceptance/titanium-audio-qemu-2026-11-09.json). |
| Chess (Demo) | The exact current ISO (`f17ed84e…`) renders the board, accepts a dragged pawn move, verifies the AI's E7-E5 reply, and returns through Escape to a working terminal; `zstatus` reports no failures ([current named-app check](docs/acceptance/named-apps-exact-current-f17-utm-qemu-2026-11-10.json)). Other moves, full games, and live UTM/Pi interaction remain unverified. The ARM64 `MouseSet` bridge posts a guest mouse-move message; it does not warp the host pointer. |
| TicTacToe (Demo) | The exact current ISO (`f17ed84e…`) passes a five-move win, board reset/replay, mouse input, Escape, and graphical terminal recovery; `zstatus` reports no failures ([current named-app check](docs/acceptance/named-apps-exact-current-f17-utm-qemu-2026-11-10.json)). A nine-move draw and replay also passed on an earlier image. Other game outcomes and live UTM/Pi interaction remain unverified. |
| ToTheFront | The exact current ISO (`f17ed84e…`) selects both human players, enters the map, plots artillery, and returns through Escape to a working shell ([current named-app check](docs/acceptance/named-apps-exact-current-f17-utm-qemu-2026-11-10.json)). Existing ARM64 fixes for indirect-fire unit counts and player-specific direct-fire visibility have separate controlled QEMU evidence ([fix checks](docs/acceptance/tothefront-candidate-qemu-2026-11-09.json)). Broader AI gameplay and live UTM/Pi behavior remain unverified. |
| Vocabulary | The exact current ISO (`f17ed84e…`) accepts an answer, advances the quiz, and returns through Escape to a working terminal; `zstatus` reports no failures ([current named-app check](docs/acceptance/named-apps-exact-current-f17-utm-qemu-2026-11-10.json)). Incorrect-answer WAV evidence exists for an earlier ISO; live UTM/Pi app-audio output remains unverified. Run `make check-vocabulary-program`. |

| X-Caliber | The exact current ISO (`f17ed84e…`) passes both plain-arrow thrusters, a visible Space projectile, Escape, and terminal recovery (`40+7` returns 47) with UTM's bundled QEMU profile ([current named-app check](docs/acceptance/named-apps-exact-current-f17-utm-qemu-2026-11-10.json)). Broader gameplay remains unverified. |

A subsequent Vocabulary run using the graphical launcher captured a tone
consistent with its correct-answer `Sound(74)` as well; see the
[QEMU record](docs/acceptance/vocabulary-correct-sound-qemu-2026-11-09.json).

The 2026-11-08 task-message regression and a fresh run on the current ISO
(`4bc48433…`) pass in PCI QEMU. Five rounds of empty
`MessageScan`, `KeyScan`, `CharScan`, and `FlushMessages` polling do not hand off
the core; explicit `Yield` and a blocked `MessageGet` let the next ready task
run. The earlier timer-sleep handshake was removed from this focused regression
because it could hang while draining session tasks. This verifies the current
cooperative bridge behavior, but does not mean the pinned upstream `Sched.ZC` is
the scheduler currently running. See
[`current ISO acceptance`](docs/acceptance/task-message-scheduling-current-qemu-2026-11-10.json)
and the [initial regression](docs/acceptance/task-message-scheduling-qemu-2026-11-08.json).
`BreakUnlock` now delivers an already-pending break to the current task or
queues it for another task's cooperative resumption. QEMU checks cover
sleeping and message-waiting tasks and the Shift-Escape option. A focused
task also caught a real Ctrl-Alt-C after its break lock was released; when
the chord arrived during MessageScan polling, handling waited for Yield. The
workflow also passed with emulated QEMU and NEC USB controllers, including
USB/VirtIO keyboard ownership changes.
The bridge clears interrupted wait state, wake time, popup wait flags and
queued messages on Break. Native drive and block-device lock release is not
implemented; see
[the pending-break record](docs/acceptance/task-break-pending-qemu-2026-11-09.json).
The terminal output bridge now uses the pinned `DocPrintPartial`, matching the
upstream output path's partial-document behavior and its conditional yield.
The first Ctrl-Alt-C check only verified serial recovery and missed that the
bootstrap shell stopped the graphical terminal. The keyboard byte queue now
retains the Alt modifier so that Ctrl-Alt-C reaches the guest without invoking
the shell's Ctrl-C escape. Three QEMU VirtIO DebugDemo runs then preserved the
graphical terminal and executed a new command in it after each break. A further
keyboard handoff fix also preserved the separate plain Ctrl-C shell escape;
both behaviors passed with QEMU VirtIO and emulated xHCI USB keyboards. The
`DOCF_DONT_SWAP_OUT` and message-polling scheduler checks also passed. Ctrl-Alt-D,
live UTM and Pi remain unverified. This is not the pinned `Sched.ZC` scheduler.
The unchanged output path also failed the no-growth check (+96 data bytes
across two cycles); a separate diagnostic run timed out during long input before
a second heap snapshot. The cause remains unconfirmed. See
[`terminal-output-investigation-2026-11-09.json`](docs/acceptance/terminal-output-investigation-2026-11-09.json).
An exploratory repeat with per-task heap totals found that the separate
AutoComplete task's live allocation size fluctuated while global heap use and
allocation count remained constant. The aggregate test therefore cannot by
itself attribute its variation to terminal teardown.
An independent check of three UserText terminal/filter cycles found no live
allocations owned by finished tasks. It does not cover shared allocations or
allocations retained by live tasks, so the aggregate no-growth failure remains
open.
An isolated live-task test found that interrupting partial document output
retained 16–32 bytes per run. A guarded ARM64 overlay now releases its scratch
buffer and acquired document lock when document processing throws, then
rethrows the same exception. Normal completion and three interrupted QEMU
runs retained zero bytes in that focused test. A callback that throws also
previously retained 16 bytes; the extended overlay now cleans that path, and
QEMU checks passed for callbacks that handle, decline or throw on output.

The 2026-11-01 rebuilt PCI QEMU catalog reconfirmed compilation of all 12 named
app entry points. A comprehensive PCI QEMU run of the named-app probes completed with
exit status 0 for Logic, Budget, TimeClock, GrModels, KeepAway, Titanium,
Span, Strut, ToTheFront, Vocabulary, and Psalmody. The X-Caliber focus and
Escape issue recorded during an earlier trace is superseded by the current
PCI QEMU probe: sequential Left/Right, Space firing, Escape, and terminal
recovery pass on ISO `e34393b6…`; see its dated acceptance record.
Runtime probes also passed for Digital Logic, ToTheFront, Span, Strut,
TimeClock, Vocabulary and Psalmody. The dependency-aware Demo catalog covers
all 206 upstream `.ZC` files in a fresh full PCI QEMU sweep. `/Demo/NullCase.ZC` was rechecked after
the implicit-case parser fix, and the ToHtmlToTXTDemo sources were rechecked after expanding the code arena and binding `Log10`. `HtmlGen.ZC` now compiles with normal file loading (preserving its relative include), and `TXTGen.ZC` compiles after loading the stock `ToTXT` module. The refreshed dependency-aware PCI QEMU source sweep compiles all 206 Demo sources; the five failures recorded in the earlier 2026-11-02 run were subsequently resolved by correcting stock-module context and ARM64 adaptations. This is source compilation evidence, not runtime acceptance. The refresh confirms the LightTable compile follow-up and adds AArch64 SVC adaptations for InterruptDemo and Ring3; runtime probes pass for both adaptations, while Ring3 explicitly reports that EL0 tasks remain unsupported. `/Demo/Asm/PutDec.ZC` now uses an ARM64-portable right-to-left decimal conversion; `/Demo/Asm/AsmHelloWorld.ZC` preserves the ten-line output with the portable printer, and `/Demo/Asm/BuzzFizz.ZC` translates the stock 1–100 loop with its original spans; `/Demo/Asm/MulByHand.ZC` provides a shift-and-add byte multiply plus a 32-bit-limb 64×64→128 multiply; `/Demo/Asm/DivByHand.ZC` supplies restoring 64-bit division; all five compile. Focused QEMU guest checks validated the arithmetic results and edge cases (see `docs/acceptance/demo-mulbyhand-math-2026-11-01.json` and `docs/acceptance/demo-divbyhand-math-2026-11-01.json`); full demo output acceptance remains separate. These are compile results, not runtime acceptance. The inventory now uses a source-preserving loader for `Directives.ZC`; native lexer built-ins for `__DATE__`, `__TIME__`, and `__CMD_LINE__` also let `WebLogRep.ZC` compile. `ClassMeta.ZC` now passes its graphical QEMU probe: the ARM64 reflection bridge displays the `age` and `serial_num` fields, the assigned `55555`, and returns to a working terminal. The overlay renders `RankOut` inline and represents percentile metadata using supported forms; this adapts this demo and does not establish general callback-pointer metadata compatibility. This closes the shared `sys_compile_time` external-data blocker in both HomeSys examples: the runtime now binds the build `CDate`, and isolated compile tasks resolve exported host data. The lexer preserves comment markers inside quoted `#define` strings, fixing the file-mask macro in `HomeWrappers.ZC`; loading the stock `System/Utils/Find.ZC` dependency lets that source compile. Expanding the lexer literal buffer allows the long literals in `MemDemo.ZC` and `WaterFowl.ZC` to compile. `SpriteText.ZC` now compiles with the isolated probe’s compiler and sprite module setup. `PhoneNumWords.ZC` also compiles after changing its A–Z digit lookup to a 26-element explicit initializer in the ARM64 overlay. FlatTops now compiles when the isolated probe loads the stock `GrDC`, `GrMath`, `GrPrimatives`, and `GrComposites` modules; TreeCheckers compiles with its required constants and stock Math/Sound modules. TOSTheme, InSetIP, UnusedSpaceRep, UnusedDefine, and SerializeTree now compile with their stock module dependencies loaded. The `SysRep.ZC` ARM64 overlay removes x86 CPUID-specific reporting; `AndNotMod.ZC` and `FixedPoint.ZC` now compile with the stock `SysRep` and `CPURep` modules. The upstream sources are unchanged. The refreshed catalog also compiles `/Demo/MultiCore/LoadTest.ZC` with the stock compiler headers and program modules loaded, correcting a probe-context false negative. Its QEMU runtime acceptance now exercises a cooperative CPU-0 worker loop, app key exit, and terminal recovery; the ARM64 bundle substitutes JIT loads of stock `Print.ZC` for the unavailable x86 `Compiler.ZXE` worker and maps stock CPU-1 worker requests to the bridge’s CPU 0. This does not establish SMP or AOT compiler support; see [`demo-loadtest-qemu-2026-11-09.json`](docs/acceptance/demo-loadtest-qemu-2026-11-09.json). The `Mandel.ZC` ARM64 overlay now uses ZealC-supported syntax for its one-CPU worker count; its rebuilt QEMU image rendered the fractal, handled Escape, and returned to a terminal where `40+2;` yielded 42. The same pass restored the stock `Blink(F64 Hz=2.5)` API in `TaskBridge.ZC`. CharAnimation, outside the Demo tree, also compiled with its required
stock modules. The Grid probe exposed two missing ARM64
bridge services, `GridInit` and `DCFill`; both are implemented in `TaskBridge.ZC`,
and an on-guest check validates them and returns `0x2a`. Grid and MouseDemo
also needed an overlay to pass `gr.dc` explicitly to `GrPlot`, because the
isolated source loader does not retain its default first argument; both now
compile in the broad sweep. These
are source-file results, not a count of runnable programs. The refreshed source catalog records all 206 Demo `.ZC` sources as compile passes and zero compile blockers. Ring3 now compiles and its AArch64 SVC adaptation runs, but it does not implement ring-3/EL0 tasks. TOSConfig itself now compiles after the ARM64 overlay disables the legacy x86 debug-distro RAM mapping; it is compile evidence only, not acceptance of the TOS installer workflow.
The source-by-source failures are listed in the generated ledger. The Carry
demo has an ARM64 source overlay that explicitly computes the carry bits
used by its two-word count, shift, and branch examples; its runtime behavior is
accepted as described below. Consecutive `#exe` blocks now execute in source
order, with a QEMU regression and GlobalVars recheck recorded in
[`exe-order-qemu-2026-11-09.json`](docs/acceptance/exe-order-qemu-2026-11-09.json).
`StreamPrint` now injects generated source, and the unmodified `DefineStr.ZC`
passes its focused QEMU document test; see the [acceptance record](docs/acceptance/stream-print-qemu-2026-11-09.json).
Simple statements inside `#exe` now execute individually in source order;
the [regression](docs/acceptance/exe-statement-order-qemu-2026-11-09.json)
checks a definition created by one statement and consumed by the next.
A directive following an `if` inside `#exe` executes while the parser looks
for `else`, before that `if` executes. This appears consistent with the
pinned ZealOS source; the opt-in QEMU regression returns 41. See
[`exe-directive-lookahead-qemu-2026-11-09.json`](docs/acceptance/exe-directive-lookahead-qemu-2026-11-09.json).
The previous fix that re-expands a name after an `if` executes needs an
upstream x86 comparison: pinned `ParseIf` also looks ahead at identifiers
before executing the `if`, so the current behavior may diverge from ZealOS.
Full `#exe` semantics remain unverified. A direct QEMU load of the original
`TimeIns.ZC` currently fails at `PassTrace`; the bundled ARM64 timing demo is
an adaptation and does not demonstrate the original optimizer-trace behavior.
See [`demo-time-ins-stock-qemu-2026-11-09.json`](docs/acceptance/demo-time-ins-stock-qemu-2026-11-09.json).
The graphical terminal now re-anchors its document cursor while clearing or
replacing a line, fixing a QEMU long-line display mismatch; see the
[input regression](docs/acceptance/gui-long-input-qemu-2026-11-09.json).
Physical Pi and live UTM validation remain outstanding.
The missing-block-device bridge now throws `Drive` when requested, matching
the upstream missing-drive contract. A separate read-only primitive can read
absolute PCI VirtIO disk sectors, with a [QEMU GPT and bounds check](docs/acceptance/raw-disk-read-qemu-2026-11-09.json).
The legacy RedSea window can now be identified only after checking its boot
header and bounded root; [QEMU fixtures](docs/acceptance/raw-redsea-geometry-qemu-2026-11-09.json)
cover stock absolute and bootstrap relative addresses, absent media, and a
malformed root.
An explicit experimental read-only `D` drive can now expose a validated
legacy volume to `Letter2Drive` and `BlkRead`; the original DiskRaw demo
completed a [QEMU read-only GPT-sector workflow](docs/acceptance/diskraw-readonly-qemu-2026-11-09.json).
Its bitmap geometry and `RedSeaValidate` behavior have a separate
[QEMU check](docs/acceptance/redsea-guest-drive-qemu-2026-11-09.json).
The `disk:` source provider can now read files from the
stock absolute-address layout; the [QEMU probe](docs/acceptance/stock-redsea-file-qemu-2026-11-09.json)
checks source loads from multiple root sectors and a nested directory, and
rejects invalid entries. As in upstream RedSea, file extents are read directly
even if the contiguous attribute is absent. This is not a general D: filesystem.
The same read-only source bridge can enumerate reachable stock RedSea files
and directories for `FilesFind`, including nested files and empty directories.
Direct directory checks and `Cd` can also resolve empty directories.
Empty stock RedSea files also read as zero-length files rather than missing
files. The native source store can create and reopen empty files; a QEMU check
overwrote a nonempty file with an empty file and reopened it after a cold boot.
This does not establish file operations, stock drive-registry integration,
writes, or a Pi storage backend. The boot source store remains separate.
`Directives.ZC` and `WebLogRep.ZC` now compile with native built-in handling. The previous ToHtml resource blocker is
resolved; the 96 MiB executable arena is reserved as zero-filled RAM so it
does not add 96 MiB to the ISO. The enlarged compiler arenas and recovery
copies reserve about 448 MiB of RAM; this QEMU build boots with 1 GiB, while
the expanded-memory configuration still needs physical Pi validation.
The ARM64 bridge provides waveform math helpers used by stock Sound sources;
the VirtIO sound driver initializes on QEMU; and the driver probe, ASCIIOrgan,
Psalmody and the `Beep()` probe produce stereo PCM through the WAV backend.
Normal and `busy=TRUE` Beep probes now both capture a full 500 ms, approximately
784 Hz tone in QEMU; the WAV contains 0.520 seconds including the queued audio
tail. The stock 200 ms trailing wait is implemented but has not been timed
independently. These checks establish QEMU playback-path behavior, not audible
output on UTM or Pi hardware.
See [`virtio-sound-qemu-2026-11-04.json`](docs/acceptance/virtio-sound-qemu-2026-11-04.json), [`demo-ascii-organ-sound-qemu-2026-11-04.json`](docs/acceptance/demo-ascii-organ-sound-qemu-2026-11-04.json), [`psalmody-virtio-sound-qemu-2026-11-04.json`](docs/acceptance/psalmody-virtio-sound-qemu-2026-11-04.json), and [`virtio-beep-qemu-2026-11-04.json`](docs/acceptance/virtio-beep-qemu-2026-11-04.json). The generated per-file
results are in `build/demo-source-catalog.json`; see the detailed run record
in [the program inventory](docs/PROGRAM-INVENTORY-2026-10-28.md). The
source-level ledger at
[`demo-source-ledger-2026-11-10.json`](docs/acceptance/demo-source-ledger-2026-11-10.json)
records the 206 Demo `.ZC` compile results. Its current totals are 206 compile
passes, zero compile blockers, 202 runtime-accepted sources, four partial
runtime results, and zero recorded runtime failures. The four partial sources
are the two TOS account examples
(`/Demo/AcctExample/TOS/TOSDistro.ZC` and `TOSMisc.ZC`) and the two multicore
examples (`/Demo/MultiCore/Lock.ZC` and `MPRadix.ZC`). `TOSExtern.ZC` is
accepted as a declaration-only interface module; its external implementations
and dependent workflows are not thereby accepted. See its
[`interface acceptance`](docs/acceptance/demo-tos-extern-interface-qemu-2026-11-10.json).
The current ISO revalidated the Wallpaper Desktop installer, cold-boot layout,
six controls, palette editing, palette source save/list/reload, and Telnet
client/local fixture workflow; arbitrary startup customizations and live
UTM/Pi use remain open ([acceptance](docs/acceptance/demo-wallpaper-desktop-current-qemu-2026-11-10.json)).
That workflow also exercises the Home tool entrypoint
`/Home/PaletteEditor/Run.ZC` for color changes, hexadecimal RGB entry, Save As,
Open listing, reload, and task recovery. Other Palette Editor controls and the
rest of `/Home` are not covered by that result.
These are source-level counts, not a
count of standalone programs; several entries are support modules or need
feature-specific test setups. `DiskRaw` is accepted for its isolated read-only
sector workflow; full UI and storage integration remain outside that
acceptance. `DebugDemo` is accepted for its QEMU busy-loop interruption and
graphical terminal recovery, while debugger and live hardware checks remain
open. `Lock` passes the single-CPU worker path with matching expected and actual
counts, terminal recovery, and no failed tasks; CPU-1 requests map to CPU 0, so
full Lock acceptance still requires SMP contention testing. The current ISO
revalidates `TOSDistro` module initialization and terminal recovery in QEMU;
this excludes its destructive installer and distribution-regeneration workflow
([acceptance](docs/acceptance/demo-tosdistro-safe-load-current-qemu-2026-11-10.json)).
`TOSMisc` now verifies `DC2Doc` against all 94 printable ASCII glyphs and
checks that saved dollar glyphs are encoded as literal DolDoc text. Its
`SoundShift` queue cleanup and `AUWrite`/`FileRead` AU-record round trip also
pass in QEMU. Stock `GRWrite`, `GR2MV`, and `GRRead` also pass a synthetic
single-frame movie round trip. Stock `ScreenCast(ON/OFF)` now toggles recording
in the QEMU compatibility runtime, produces a nonempty GR frame that `GRRead`
reopens, and records note changes into an AU file that `AUWrite` writes and
`FileRead` verifies. Stock `MemTest` now fills the data and separate code
arenas through their expected 16 MiB reserve thresholds, then returns to a
working compiler; the earlier failure was the synthetic root task pointing at
an uninitialized per-fiber code heap ([acceptance](docs/acceptance/demo-tosmisc-memtest-qemu-2026-11-10.json)). The stock
`DiskCheck('D',FALSE,FALSE)` path also returns zero on the disposable,
explicitly mounted read-only RedSea fixture; `DiskCheckAll`, repair behavior,
and its configured-drive workflow remain unverified. On the current ISO
(`923ae07d…`), the ARM64 `JukeSongsPuppet` adapter now launches stock `.ZC`
songs directly through JukeBox's `SingleSong` path. The QEMU regression
returned from both the stock song and a looping Beep fixture; the fixture wrote
markers before and after entering `Song`, the graphical terminal recovered,
and `zstatus` reported `failed=0`. The WAV capture contains 2.52 seconds of
nonzero stereo PCM. This establishes song-task execution and tone generation
in automated QEMU; it is not a live UTM-window, Pi, or audible music acceptance
([JukePuppet direct-play acceptance](docs/acceptance/jukepuppet-direct-play-qemu-2026-11-11.json)).
Other TOS workflows remain unverified
([acceptance](docs/acceptance/demo-tosmisc-current-qemu-2026-11-10.json)).
The earlier JukePuppet run on ISO (`4ba080db…`) is superseded: its fixture
returned without proving that playback started. Keep that record as historical
evidence alongside the earlier timeout
([current check](docs/acceptance/jukepuppet-current-4ba-utm-qemu-2026-11-11.json),
[earlier timeout](docs/acceptance/demo-jukepuppet-timeout-qemu-2026-11-10.json)).
The same heap-routing fix preserves the GrModels, KeepAway and Titanium QEMU
app checks; the fresh regression log is recorded
[here](docs/acceptance/three-d-apps-heap-fix-qemu-2026-11-10.json).
The TimeIns ARM64 adaptation now has a fresh three-sample QEMU runtime check
with terminal recovery. The stock source still fails at `PassTrace`, so this
does not reproduce its optimizer traces ([runtime acceptance](docs/acceptance/demo-time-ins-arm64-runtime-qemu-2026-11-10.json),
[stock-source probe](docs/acceptance/demo-time-ins-stock-qemu-2026-11-09.json)).
The partial and accepted classifications and their evidence are recorded in the ledger.
Runtime acceptance is not proof of complete feature parity; each evidence
record states its scope and limitations. `/Demo/CompileDemo.ZC` now passes a focused QEMU run through the native guest compiler/JIT bridge, including multi-expression side effects, printed results, the final 0–9 loop, and terminal recovery; see [`demo-compiledemo-qemu-2026-11-02.json`](docs/acceptance/demo-compiledemo-qemu-2026-11-02.json). `/Demo/MultiCore/MPAdd.ZC` completes the stock serial-versus-queued benchmark, renders its performance chart, and returns to a working terminal in QEMU; see [`demo-mpadd-qemu-2026-11-07.json`](docs/acceptance/demo-mpadd-qemu-2026-11-07.json). `/Demo/MultiCore/MPPrint.ZC` visibly prints CPU0 queued-job output and returns to a working graphical terminal in one-vCPU QEMU; SMP worker execution remains unsupported and unverified. See [`demo-mpprint-qemu-2026-11-08-cpu0-color.json`](docs/acceptance/demo-mpprint-qemu-2026-11-08-cpu0-color.json). `/Demo/WordSearch.ZC` now renders its random grid and dictionary matches before returning to a working terminal; `/Demo/SuggestSpelling.ZC` returns dictionary matches for `Helo` and exits cleanly. `/Demo/LastClass.ZC` now displays CCPU and CTask class reports and returns to the terminal in QEMU after fixing task-to-compiler hash lookup; see [`demo-last-class-hash-fix-qemu-2026-11-08.json`](docs/acceptance/demo-last-class-hash-fix-qemu-2026-11-08.json). `/Demo/ParenWarn.ZC` displays source-file warnings and returns to the terminal in QEMU; see the later [`demo-paren-warn-qemu-2026-11-03.json`](docs/acceptance/demo-paren-warn-qemu-2026-11-03.json). Neither record establishes live UTM or Pi behavior. The 2026-11-02 QEMU probes add BlackDiamond, ScreenCapture, MathAudioDemo, PredatorPrey, ElephantWalk, FlapBat and 32BitPaint. BlackDiamond loads stock Math for `Cosh`, paints its light canvas, and verifies the HUD, movement, Escape cleanup, and terminal recovery. ScreenCapture writes a GR capture, reads it back into visible scanout, and returns to the graphical terminal. MathAudioDemo verifies its stock waveform plotting path, not audio output. PredatorPrey verifies queued trajectory rendering and Escape recovery. ElephantWalk verifies sprite rendering and keyboard-driven redraw. FlapBat verifies its game canvas, animation and Space input; a later QEMU sound-device run captured multiple pitches and confirmed the song task and tone stopped after exit (see `docs/acceptance/demo-flapbat-sound-qemu-2026-11-09.json`). 32BitPaint verifies mouse drawing, clear and exit. Directory-derived categories remain
provisional, and per-file runnable-versus-module classification is in progress.

`/Demo/MemDemo.ZC` and `/Demo/Carry.ZC` now have QEMU runtime acceptance.
MemDemo reaches the final system powers-of-two row. Carry reaches the `0x10`
high word in its two-word counter, shifts through the 64-bit boundary, and
prints all 64 branch bits from `0xFFCC3311` (the terminal wraps the result).
Both return to the prompt; `40+2;` returns 42 and `zstatus failed=0`. The
explicit Demo runtime acceptance count at that run was 45 sources. See the
[`demo-runtime-basics` record](docs/acceptance/demo-runtime-basics-2026-11-01.json)
and [Carry record](docs/acceptance/demo-carry-qemu-2026-11-02.json).

The portable assembly demos now have isolated PCI QEMU runtime acceptance.
AsmHelloWorld prints all ten numbered lines; BuzzFizz matches the FIZZ/BUZZ
spans for its visible output; PutDec prints `122333221` on its own line.
MulByHand prints its byte-multiply and 64-by-64-to-128-bit examples, and
DivByHand prints `123456/10=12345`. AsmAndC3 exercises inline assembly carry
handling and prints its three `0x42` results. All six return to the graphical terminal,
where `40+2;` returns 42 and `zstatus failed=0`. Explicit Demo runtime
acceptance is now 51 sources; see the [assembly output record](docs/acceptance/demo-asm-output-qemu-2026-11-02.json),
[arithmetic output record](docs/acceptance/demo-asm-arithmetic-qemu-2026-11-02.json),
and [AsmAndC3 record](docs/acceptance/demo-asmandc3-qemu-2026-11-02.json).
The interactive `/Demo/Define.ZC` also passes its Spanish prompt/localization
flow and returns cleanly; see the [Define record](docs/acceptance/demo-define-qemu-2026-11-02.json).
Explicit Demo runtime acceptance is now 52 sources.

A fresh run on the current ISO (`e34393b6…`) rechecked 13 graphical Demo
programs in PCI QEMU: Box, Hanoi, CircleTrace, Pick3D, 32BitColor, Blot,
MouseDemo, Grid, Palette, Bounce, Clock, 3DPoly, and RotateTank. Each reached
its drawing or interaction check, exited through its supported key path, and
returned to a terminal where `40+2;` produced 42. This refreshes evidence for
programs already included in the 32-source runtime tally. A separate focused
gate now verifies Slider's visible custom control, left-track click changing
the value to 9 while the right remains 0, and terminal recovery. This brings
the explicit Demo runtime acceptance count to 33 distinct sources. See the
[`Slider acceptance record`](docs/acceptance/demo-slider-qemu-2026-11-02.json) and
[`demo-graphics-qemu-2026-11-02.json`](docs/acceptance/demo-graphics-qemu-2026-11-02.json).

The system-context probe now accepts ordered `--demo-system-module` options to
load stock `/System/*.ZC` modules before compiling selected demos. The ARM64 overlay now lets the LightTable source compile in the isolated
module-aware probe. Its runtime behavior remains unverified; compile success does
not establish that the graphical demo works. The six baseline system-context demos still
compile in the fresh module-aware run.

An earlier sweep found 29 first compile failures. The later dependency-aware
source sweep resolved those catalog failures, as described above; compilation
alone does not verify original architecture-specific behavior or runtime parity.

The graphical QEMU probe `python3 scripts/check-compat.py --pci --probe-demo-basics`
runs the original Print, DateTime, RadixSort, SubIntAccess, SubSwitch and
NullCase demos through the visible terminal. It checks Print's number format
output, DateTime's clock/day/month fields, both 16-row RadixSort tables and
that the output equals the sorted input, the four SubIntAccess expressions,
all six SubSwitch branches and all six NullCase labels. It then verifies
terminal recovery with `40+2;` -> 42. This closes runtime coverage for those
six demos in PCI QEMU only; it does not
classify or accept all 206 Demo source files. The 2026-10-31 system-launch
gate also passes its `Log10` numeric checks. See
[`demo-runtime-basics-2026-10-31.json`](docs/acceptance/demo-runtime-basics-2026-10-31.json).

The current isolated catalog is `docs/acceptance/program-catalog-2026-10-27.json`;
its mode is `CCF_JUST_LOAD` plus selected normal-load checks. The sprite service fixtures
are `build/3d-apps-compile.json`. Neither report proves interactive app
behavior. The user confirmed live UTM drawing/simulation and Escape recovery
for Doodle, Mass Spring and Game of Life, and confirmed the Digital Logic
prompt fix. Their acceptance records are above. Chess and Varoom have focused
QEMU runtime checks, but those checks do not establish all their features or
live UTM/Pi behavior. Other sources also need broader acceptance. Assembly
demos require architecture-specific implementations.

The apparent Budget and GrModels form failures were traced to the generated
ARM64 `DocForm` adapter: its replacement left behind the original
`return FALSE`, causing every form to exit before document creation. The
translator now replaces the condition and its return as one block, with a
regression assertion on the generated source. The corrected PCI workflow
passes Budget form creation/edit/cancel/save/reopen and cold disk-only
persistence; the 3D live probe passes GrModels Man and Ball form and mesh
generation. KeepAway now runs through gameplay and returns cleanly in the PCI
probe. Track upstream source integrity separately from compilation, visible
behavior/input, clean exit and save/reopen requirements.

## Full Apps and Demo inventory / work plan — 2026-10-28

See [the file-derived inventory and phased plan](docs/PROGRAM-INVENTORY-2026-10-28.md).
The pinned mirror contains 70 `.ZC` files under `Apps` and 206 under `Demo`.
These are source-file counts, not runnable-program counts: both trees include
loaders, shared routines, templates, and data-driven examples. The inventory
keeps file presence, compilation, runtime, interactive behavior, and live UTM
acceptance as separate states.

The focused `/Demo/GlobalVars.ZC` probe passes its JIT half: the global and
static arrays print `Time 1` and `Time 2`, and the graphical terminal recovers
to evaluate `40+2` as 42. Its AOT-only `#ifjit` half remains unverified because
the current bootstrap does not expose the stock `Comp()` command. This is
partial runtime evidence and does not increase the fully accepted Demo count.
See [`demo-global-vars-jit-2026-11-01.json`](docs/acceptance/demo-global-vars-jit-2026-11-01.json).

`/Demo/Exceptions.ZC` now passes its nested Point1 throw/catch path in PCI QEMU.
The ARM64 formatter supports ZealOS `%P` pointer links, the demo reaches
`Caught in D2`, completes its final prompt, and returns to a terminal where
`40+2;` evaluates to 42. The Demo runtime ledger now records twelve fully
accepted programs; other exception paths and `%P` call sites remain unverified.
See [`demo-exceptions-2026-11-01.json`](docs/acceptance/demo-exceptions-2026-11-01.json).
