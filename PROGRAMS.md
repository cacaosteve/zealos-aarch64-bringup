# Regular ZealOS programs on ARM64

The desktop, compiler and installer milestones do not establish that every
regular ZealOS application works. Programs are imported from the same public
ZealOS commit as the system sources:
`eee210f4379ba3d00b47507f336a158eefb2c993`. Exact file hashes are recorded in
`upstream/pinned/UPSTREAM.json` and verified by `make check-upstream`.

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
Periodic/template, account-file editing and account-filter UI workflows have
not been accepted; live UTM acceptance of this candidate also remains pending.

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

## Current named-app status — 2026-10-30

The pinned upstream `src/Apps` and `src/Demo` mirror contains 294 files,
including source, documents and binary resources. It is complete and
hash-checked; that file count is not a count of supported programs. This
table tracks the named Apps currently in scope alongside Logic. A compile or
normal-load result is not runtime acceptance.

| Application | Current verified status |
|--|--|
| Budget | Graphical entry form, field editing, save/abort, reopen and cold disk-only persistence pass in the PCI workflow; live UTM retest of the corrected image is pending. |
| GrModels | The 2026-10-31 PCI rerun passed both Man and Ball forms and mesh generation; the Ball path created a 58-vertex mesh, inserted a CSprite, and rendered 643 purple pixels inline under QEMU `ramfb`. The user's live UTM screenshot showed the repeat prompt without an obvious sprite, so visual UTM acceptance remains open. The UTM screenshot does not establish which swatch was chosen; repeat with the same bright purple swatch used by the gate before attributing the difference to rendering. UTM launches `virtio-ramfb`, while the automated PCI gate uses plain `ramfb`, so the gate does not reproduce UTM's display path yet. A local QEMU attempt with `virtio-gpu-device` also cannot serve as a comparator: firmware boot reaches the kernel, but the framebuffer self-check fails at `GrPeek` before the shell starts. At `Do another(y or n)?`, enter `n` to finish the current model and return. |
| KeepAway | PCI gameplay frame renders; Space and mouse input leave the court active, Escape returns to the terminal, and the `40+2;` smoke check passes. A fresh 183-second run on the rebuilt ISO keeps scanout active through game-over and Escape recovery, with `40+2;` working afterward. The source overlay guards nearest-player selection, and ARM64 `Beep`/`SoundReset` bindings are silent no-ops. The earlier live UTM freeze still needs retest against this ISO. |
| Psalmody | PCI graphical probe enters the editor, accepts a note, exercises playback, and returns with Shift+Escape to a working terminal. ARM64 audio output is silent, so audible playback remains unsupported. |
| Span | QEMU graphical entry/help/frame, bridge editing, Shift+Escape worker cleanup, and return to a working terminal pass; `40+2;` returns 42. Live UTM bridge editing/save still pending. |
| Strut | QEMU help, mass placement/frame change, Space play-mode switch, Escape recovery, and terminal `40+2;` -> 42 pass. Live UTM editing/play and save/reopen still pending. |
| TimeClock | App compile, date/time formatter gates, and automated punch-in/out record write/read plus report-state round-trip pass. The graphical QEMU probe loads the stock app, submits both descriptions through `StrGet`, reads them back with `TimeRep;`, and recovers to a terminal where `40+2;` returns 42. The red `IN`/`OUT` report labels are below the probe's monochrome OCR threshold, so it asserts the saved descriptions instead. Verify clock behavior in live UTM; CMOS hardware clock access is unavailable. |
| Titanium | Live UTM confirms gameplay, Space firing and Escape recovery; the user now confirms ordinary Left/Right move the ship on ISO `219d751c…`. The VirtIO keyboard bridge updates both merged and raw scan-code bitmaps; QEMU confirms arrow movement and Escape returns to a terminal where `40+2;` returns 42. Speaker effects are silent on ARM64. |
| Chess (Demo) | PCI QEMU runtime probe now renders the board, handles Escape cleanup, and returns to a terminal where `40+2;` returns 42. Live UTM pointer interaction remains unverified. The ARM64 `MouseSet` bridge posts a guest mouse-move message; it does not warp the host pointer. Run `make check-demo-chess`. |
| TicTacToe (Demo) | PCI QEMU runtime probe checks the board, two mouse turns, Escape exit, and terminal recovery. Live UTM interaction remains unverified. The source advertises Ctrl-Alt-C, but that chord is not yet verified through the emulated input path. Run `make check-demo-tictactoe`. |
| ToTheFront | ARM64 source overlay fixes the Human player's indirect-fire unit-count loop; the stock `i > UNITS_NUM` condition skipped the artillery order phase. PCI runtime probe accepts both player-type selections, displays/dismisses help, renders the generated map, selects a visible artillery unit, confirms its range indicator, plots an order into an adjacent hex, advances a phase with Space, and returns through Escape to a working terminal (`40+2;` -> 42). Unit movement/direct fire, broader AI play, and live UTM behavior remain unverified. |
| Vocabulary | QEMU PCI runtime probe launches the quiz, displays dictionary content, accepts a numbered answer and advances, then Escape returns to a terminal where `40+2;` returns 42. Dictionary indexing and lookup are covered by the autocomplete gate. The brief answer-feedback/definition popup is not yet visually verified; sound effects are silent. Run `make check-vocabulary-program`. |
| X-Caliber | QEMU reaches gameplay. Fixed a MathODE timeout comparison that mixed guest uptime with host monotonic time; ODE integration now advances. In separate fresh-state probes, Left and Right each visibly move the ship when tested first (45 and 43 changed ship pixels); the opposite direction tested second in the same session sets its bitmap and thrust value but produces no visible change. Escape returns to the terminal and `40+2;` returns 42. Space firing and reliable sequential left/right control remain unverified. Do not add a `Yield` to `MessageScan` or an X-Caliber-specific `Yield`; the cooperative scheduler regression passes. |

The 2026-11-01 task-message regression passes in PCI QEMU. Empty polling through
`MessageScan`, `KeyScan`, `CharScan`, and `FlushMessages` does not hand off the
core; explicit `Yield`, `Sleep`, and a blocked `MessageGet` let the next ready
task run. This verifies the current cooperative bridge behavior, but does not
mean the pinned upstream `Sched.ZC` is the scheduler currently running.

The 2026-11-01 rebuilt PCI QEMU catalog reconfirmed compilation of all 12 named
app entry points. A comprehensive PCI QEMU run of the named-app probes completed with
exit status 0 for Logic, Budget, TimeClock, GrModels, KeepAway, Titanium,
Span, Strut, ToTheFront, Vocabulary, and Psalmody. The separate X-Caliber
probe still fails: although focus status reports X-Caliber at the end of the
probe, a fresh-image trace shows a temporary Server focus and `JobRunOne`
restoration; later input transitions route to the parent Terminal, and Escape
does not return from gameplay.
Runtime probes also passed for Digital Logic, ToTheFront, Span, Strut,
TimeClock, Vocabulary and Psalmody. The dependency-aware Demo catalog covers
all 206 upstream `.ZC` files in a fresh full PCI QEMU sweep. `/Demo/NullCase.ZC` was rechecked after
the implicit-case parser fix, and the ToHtmlToTXTDemo sources were rechecked after expanding the code arena and binding `Log10`. `HtmlGen.ZC` now compiles with normal file loading (preserving its relative include), and `TXTGen.ZC` compiles after loading the stock `ToTXT` module. The 2026-11-01 full PCI QEMU source sweep and targeted follow-up compile 188 of 206 Demo sources, leaving 18 blocked. `/Demo/Asm/PutDec.ZC` now uses an ARM64-portable right-to-left decimal conversion; `/Demo/Asm/AsmHelloWorld.ZC` preserves the ten-line output with the portable printer, and `/Demo/Asm/BuzzFizz.ZC` translates the stock 1–100 loop with its original spans; `/Demo/Asm/MulByHand.ZC` provides a shift-and-add byte multiply plus a 32-bit-limb 64×64→128 multiply; `/Demo/Asm/DivByHand.ZC` supplies restoring 64-bit division; all five compile. Focused QEMU guest checks validated the arithmetic results and edge cases (see `docs/acceptance/demo-mulbyhand-math-2026-11-01.json` and `docs/acceptance/demo-divbyhand-math-2026-11-01.json`); full demo output acceptance remains separate. These are compile results, not runtime acceptance. The inventory now uses a source-preserving loader for `Directives.ZC`; native lexer built-ins for `__DATE__`, `__TIME__`, and `__CMD_LINE__` also let `WebLogRep.ZC` compile. `ClassMeta.ZC` remains runtime-broken or unresolved: its graphical launch and a follow-up explicit `DumpStruct` call produced no visible class output in QEMU. The guarded ARM64 overlay retains the rank callback and percentile display logic using supported metadata forms, but compile success does not establish runtime behavior. This closes the shared `sys_compile_time` external-data blocker in both HomeSys examples: the runtime now binds the build `CDate`, and isolated compile tasks resolve exported host data. The lexer preserves comment markers inside quoted `#define` strings, fixing the file-mask macro in `HomeWrappers.ZC`; loading the stock `System/Utils/Find.ZC` dependency lets that source compile. Expanding the lexer literal buffer allows the long literals in `MemDemo.ZC` and `WaterFowl.ZC` to compile. `SpriteText.ZC` now compiles with the isolated probe’s compiler and sprite module setup. `PhoneNumWords.ZC` also compiles after changing its A–Z digit lookup to a 26-element explicit initializer in the ARM64 overlay. FlatTops now compiles when the isolated probe loads the stock `GrDC`, `GrMath`, `GrPrimatives`, and `GrComposites` modules; TreeCheckers compiles with its required constants and stock Math/Sound modules. TOSTheme, InSetIP, UnusedSpaceRep, UnusedDefine, and SerializeTree now compile with their stock module dependencies loaded. The `SysRep.ZC` ARM64 overlay removes x86 CPUID-specific reporting; `AndNotMod.ZC` and `FixedPoint.ZC` now compile with the stock `SysRep` and `CPURep` modules. The upstream sources are unchanged. The refreshed catalog also compiles `/Demo/MultiCore/LoadTest.ZC` with the stock compiler headers and program modules loaded, correcting a probe-context false negative. The `Mandel.ZC` ARM64 overlay now uses ZealC-supported syntax for its one-CPU worker count; its rebuilt QEMU image rendered the fractal, handled Escape, and returned to a terminal where `40+2;` yielded 42. The same pass restored the stock `Blink(F64 Hz=2.5)` API in `TaskBridge.ZC`. CharAnimation, outside the Demo tree, also compiled with its required
stock modules. The Grid probe exposed two missing ARM64
bridge services, `GridInit` and `DCFill`; both are implemented in `TaskBridge.ZC`,
and an on-guest check validates them and returns `0x2a`. Grid and MouseDemo
also needed an overlay to pass `gr.dc` explicitly to `GrPlot`, because the
isolated source loader does not retain its default first argument; both now
compile in the broad sweep. These
are source-file results, not a count of runnable programs. The 18 blocked files
currently comprise 7 inline-assembly examples, 2 assembly-like parser cases, 2
compile-time directive cases, 2 interrupt-handler syntax cases, and 5 distinct
boot, TOS, or device-context blockers. The Carry
demo now has an ARM64 source overlay that explicitly computes the carry bits
used by its two-word count, shift, and branch examples; it compiles, but is not
yet runtime-accepted. `DefineStr.ZC` and `TimeIns.ZC` still need real `#exe` execution;
`Directives.ZC` and `WebLogRep.ZC` now compile with native built-in handling. The previous ToHtml resource blocker is
resolved; the 96 MiB executable arena is reserved as zero-filled RAM so it
does not add 96 MiB to the ISO. The enlarged compiler arenas and recovery
copies reserve about 448 MiB of RAM; this QEMU build boots with 1 GiB, while
the expanded-memory configuration still needs physical Pi validation.
The ARM64 bridge now provides the pure waveform math helpers used by stock
Sound sources; audio output is still unsupported. The generated per-file
results are in `build/demo-source-catalog.json`; see the detailed run record
in [the program inventory](docs/PROGRAM-INVENTORY-2026-10-28.md). The
source-level ledger at
[`demo-source-ledger-2026-11-01.json`](docs/acceptance/demo-source-ledger-2026-11-01.json)
records the 206 Demo `.ZC` compile results and first failures. It records 28 Demo sources with prior explicit runtime acceptance; this source
sweep adds compile evidence, not runtime acceptance. Directory-derived categories remain
provisional, and per-file runnable-versus-module classification is in progress.

The system-context probe now accepts ordered `--demo-system-module` options to
load stock `/System/*.ZC` modules before compiling selected demos. A
LightTable probe with stock Math loaded reaches its next dependency, but
`/System/DevInfo.ZC` cannot currently load because the host compiler has no
bound AArch64 external `CDevGlobals dev`. LightTable therefore remains
unverified; this is a concrete guest device-global/runtime gap, not evidence
that the demo is unsupported. The six baseline system-context demos still
compile in the fresh module-aware run.

The 29 current first failures include x86 assembly and interrupt-specific
syntax, missing dependencies and platform globals, preprocessor/parser gaps,
and ARM64 backend work such as the `Carry` intrinsic.
They are source-file failures, not a count of standalone programs; some need
the right stock module, while others require ARM64 behavior or a platform
specific disposition.

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
prompt fix. Their acceptance records are above. The wider demo/game collection
still includes Chess, Varoom, FlapBat and many other programs that have not
been accepted on ARM64. Assembly demos require architecture-specific
implementations.

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
