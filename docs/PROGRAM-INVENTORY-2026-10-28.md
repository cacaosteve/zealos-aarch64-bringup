# ZealOS ARM64 program inventory and completion plan

**Purpose:** Track what is present in the pinned ZealOS source bundle and plan
the work to make regular Apps and Demos usable on ARM64 under QEMU/UTM and,
where the hardware path applies, Raspberry Pi 4B.

**Source baseline:** pinned upstream source commit
`eee210f4379ba3d00b47507f336a158eefb2c993`.

## Scope and counting rules

The pinned source mirror now contains all 778 files under upstream `src` at
the tracked commit, including all 615 `.ZC` files: 70 under `Apps`, 206 under
`Demo`, and 339 across `Compiler`, `HomeSys.ZC`, `Kernel`, `Once.ZC`, `System`
and other roots. This includes the 262 `.ZC` files that were previously
missing, plus their source-tree documents, data, and examples. The ISO source
bundle now packages 991 entries, including ARM64 overlays and generated
support files; its expanded source partition uses 13,763 of 65,536 sectors.
The `Apps` tree has 12 top-level application directories; `Demo` has 16 source
directories. These are file counts, not counts of independently launchable
programs: `Load.ZC`, `Run.ZC`, shared services, helpers and examples all
contribute source files. Many files outside `Apps` and `Demo` are core
OS/compiler code rather than standalone programs.

The Demo names below are an exhaustive file-derived inventory of `.ZC` sources.
They still need to be classified as standalone programs, reusable modules,
examples or data generators before we claim an exact runnable-program count.
Architecture-specific assembly and direct hardware demos also need an explicit
target label rather than a false “works everywhere” result.

## Acceptance levels

Record a separate result for each program at each level:

1. **Bundled:** required source and data are present in the ISO.
2. **Compiles:** unchanged source compiles, with any guarded adapter recorded.
3. **Runs:** the program reaches its intended screen or operation.
4. **Interactive:** relevant keyboard, pointer, timer, graphics and audio
   behavior works; Escape or the documented exit path restores a usable shell.
5. **Persistence:** save/reopen and cold reboot work when the program stores
   data.
6. **Live target:** tested in UTM or on Pi hardware; a QEMU/QMP result alone
   does not prove physical host input, UTM presentation, or board behavior.

## Present status

On 2026-11-01, the original `/Demo/Games/Chess.ZC` passed a PCI QEMU runtime
probe: both player setup prompts were accepted, the board rendered, Escape
cleaned up the game window, and the graphical terminal returned `40+2;` = 42.
The ARM64 `MouseSet` bridge posts a guest mouse-move message for the active
task; it does not warp the host pointer. UTM pointer interaction for Chess is
still unverified. Reproduce with `make check-demo-chess`. A standard
`check-demo-tictactoe` target also passes its board, two mouse turns, Escape
exit and terminal-recovery probe. TicTacToe advertises Ctrl-Alt-C upstream,
but that chord is not verified through the emulated key-event path. These game
checks add runtime coverage; they do not complete the 206-file Demo corpus.

On 2026-11-01, the task-message scheduling regression passed in PCI QEMU:
empty `MessageScan`, `KeyScan`, `CharScan`, and `FlushMessages` polling kept
the core, while explicit `Yield`, `Sleep`, and a blocked `MessageGet` handed
off to the next ready task. This validates the current cooperative bridge; it
does not mean the pinned upstream `Sched.ZC` is the scheduler currently
running. The latest X-Caliber probe corrected a host/guest clock mismatch in
the MathODE timeout guard. Each arrow now visibly moves the ship when tested
first in a fresh gameplay state, Escape returns to the terminal, and `40+2;`
returns 42. The second arrow tested in the same session still has no visible
effect despite its key state being set; see the app row for remaining work.

On 2026-10-29, the isolated compile catalog compiled all 12 app entry points
and five shared/demo checks; the ToTheFront and X-Caliber loaders also passed
normal-load checks. TimeClock's app compile and focused date/time formatter
gate pass. This remains compile/load evidence only except where noted in the
table. Vocabulary's new PCI runtime probe launches the quiz, displays its
dictionary content, accepts a numbered answer and advances, then exits with
Escape to a terminal whose `40+2;` expression returns 42. This does not yet
visually validate the short-lived answer feedback or dictionary popup. Focused
graphical QEMU probes verified Psalmody's editor, note entry, playback call,
Shift+Escape and terminal recovery; Span's bridge frame, editing,
Shift+Escape cleanup and terminal recovery; and Strut's help, mass-placement
frame change, play-mode switch, Escape, and terminal recovery (`40+2;` -> 42).
A ToTheFront GUI launch displays and accepts text in the seed popup after
popup-server stdout is routed into its DolDoc. The PCI runtime probe now
selects both player types, dismisses the help popup, verifies the generated
map, selects a visible artillery unit, confirms its range indicator, plots an
order into an adjacent hex, advances the phase with Space, and exits cleanly
to a terminal where `40+2;` returns 42. The phase gate checks visible scanout
and map pixels rather than comparing OCR strings for graphics text. Unit
movement/direct fire, broader AI play, and live UTM behavior remain unverified.

On 2026-11-01, a rebuilt PCI QEMU source catalog compiled all 12 named app
entry points. The dependency-aware Demo catalog covers all 206 upstream `.ZC`
sources in a fresh full PCI QEMU sweep. `/Demo/NullCase.ZC` and all three ToHtmlToTXTDemo generator sources were
rechecked after the parser/resource fixes. HtmlGen compiles via normal file
loading so its relative include resolves; TXTGen compiles after loading the
stock ToTXT module. The 2026-11-01 full source catalog now compiles 179 of 206 Demo sources, leaving 27 compile-blocked. The count includes `CompileDemo.ZC`, compiled with stock compiler headers and modules, plus the targeted follow-ups for `Pick3D.ZC` and `ClassMeta.ZC`, and `WallPaperDesktop.ZC`, which compiles through a source-preserving loader with the stock compiler modules. `ClassMeta.ZC` remains runtime-broken or unresolved: its graphical launch and a follow-up explicit `DumpStruct` call produced no visible class output in QEMU. The guarded ARM64 overlay retains the rank callback and percentile display logic with supported metadata forms, but compile success does not establish runtime behavior. A shared host-data export resolution fix and build `CDate` binding removed the `sys_compile_time` blocker from both HomeSys sources. The lexer now preserves comment markers in quoted `#define` strings, fixing the stock `HomeWrappers.ZC` file-mask macro; loading `/System/Utils/Find.ZC` supplies its required `Find` routine. Loading stock dependencies also lets TOSTheme, InSetIP, UnusedSpaceRep, UnusedDefine, and SerializeTree compile in the isolated probe. Expanding the lexer literal buffer allows the long literals in `MemDemo.ZC` and `WaterFowl.ZC` to compile. `SpriteText.ZC` now compiles with the isolated probe’s compiler and sprite module setup. `PhoneNumWords.ZC` also compiles after changing its A–Z digit lookup to a 26-element explicit initializer in the ARM64 overlay. The `SysRep.ZC` ARM64 overlay removes x86 CPUID-specific reporting; `AndNotMod.ZC` and `FixedPoint.ZC` now compile with the stock `SysRep` and `CPURep` modules.
FlatTops now compiles when its isolated source context loads the stock `GrDC`,
`GrMath`, `GrPrimatives`, and `GrComposites` modules; its upstream source is
unchanged. TreeCheckers also compiles in an isolated context with its required
constants and stock Math/Sound modules. The ARM64 bridge now provides the stock
`Blink(F64 Hz=2.5)` API. CharAnimation,
outside the Demo tree, also compiled with its required stock `Memory.ZC` and
`Utils/TaskRep.ZC` modules. The 2026-10-31 system-launch runtime gate also
passes numeric checks for the original `Log10` import. ToHtml's large generated
code now fits the 96 MiB executable arena; the linker reserves that zeroed
arena in RAM, keeping the ISO small. The enlarged data/code arenas and
recovery copies reserve about 448 MiB of RAM; the QEMU image boots with 1 GiB,
but physical Pi validation is still needed for this memory footprint.
The graphics probes found missing ARM64 bridge services: `GridInit`, `DCFill`,
and `TextPrint`. Grid and MouseDemo also needed ARM64 overlays to pass `gr.dc`
explicitly to `GrPlot`; the default first argument is not carried into the
isolated source loader. Both compile in the broad sweep.
`TaskBridge.ZC` now provides these with upstream defaults or
equivalent rendering behavior, and `StockGraphicsBridgeChecks` validates
`GridInit` and `DCFill` on the rebuilt ISO, returning
`0x2a`. WebLogRep
remains blocked when its `#if __CMD_LINE__` expands the unsupported `#exe{}`
form. Blocked does not always mean a language or backend defect: some files
need additional module context, hardware services, or architecture-specific
assembly. The ARM64 bridge now supplies the pure `Saw`, `FullSaw`, `Caw`,
`FullCaw`, `Tri`, and `FullTri` math helpers from the stock Sound module; their
numeric checks pass, while audio output remains unsupported.
The source compiler now accepts the stock HolyC
`I0` zero-width return type using the same no-value ABI as `U0`; this lets the
unchanged `/Demo/Graphics/32BitColor.ZC` and `32BitPaint.ZC` sources compile.
Many other compile failures are missing source dependencies or
architecture-specific symbols, while some are genuine language/backend gaps. Runtime
probes passed for Digital Logic, ToTheFront, Span, Strut, TimeClock, Vocabulary
and Psalmody. Two fresh X-Caliber PCI probes reached gameplay but then timed
out waiting for guest command responses. Their Left and Right scanout checks
showed no reliable response, and Escape did not restore the terminal. An
earlier probe saw a Left response (`idle_diff=2`, `left_diff=39`), so input
behavior remains inconsistent and unaccepted. Space firing remains
unverified. The current catalog is
`build/demo-source-catalog.json`; 206 counts source files, including modules
and architecture-specific samples, not standalone apps.

A system-context probe on 2026-10-30 compiled the original Box, Mandel, Hanoi,
Doodle, BattleLines and DataBase sources in the booted ZealC environment. The
pinned upstream `System/Debug.ZC` now supplies its stock `ClassRep` and
`ClassRepD` services during graphics startup. Its ARM64 source overlay renames
the reserved `offset` parameter and limits boot loading to the portable
structure-report functions; the later x86 disassembly/register helpers remain
unported. The AArch64 compiler now allows the stock `MinI64` intrinsic, whose
compare/select implementation was already present in the backend. This is
compile evidence for DataBase, not acceptance of its on-disk database workflow.
The probe is recorded in the ignored build artifact
`build/demo-system-context-catalog.json`; this small sample is not a complete
Demo compile census. Graphics startup reaches the desktop with these services
loaded. The broader demo-basics run passed Print, DateTime and RadixSort, then
hit a graphical input mismatch while entering SubIntAccess; that gate needs
repair before it can be counted as passing on this build.

On 2026-10-31, the strict PCI program catalog reproduced an unresolved call in
`StockCatalogMathChecks`. Temporary stage markers localized it to the first
`MinI64` assertion; the reported `Arm64ClampU64` function name was misleading.
The bridge already bound `MaxI64`, but not `MinI64`, `MinU64`, or `MaxU64`, so
the source-backed compiler treated those stock `_intern` functions as external
calls. The runtime now binds all four signed/unsigned integer min/max functions
to width-correct host implementations. A rebuilt ISO now passes the complete
`python3 scripts/check-compat.py --pci --probe-program-catalog` gate, including
the Math and Psalmody runtime checks and compilation of all 12 named app entry
points. The temporary stage markers were removed.

The 2026-10-30 multicore compatibility pass adds both stock HolyC lock forms
(`lock { ... }` and `lock expression;`) to the AArch64 parser/backend path. A
focused compile scan now passes the original `Demo/MultiCore/Lock.ZC`,
`MPAdd.ZC`, `Palindrome.ZC`, and `Primes.ZC`; `Primes` uses the cache-line
width read from `CTR_EL0`. `LockBlockRuntime()` also returns `0x2a` in the
guest. This verifies syntax and the single-thread atomic operation, not true
cross-core exclusion: the current ARM64 guest still reports one CPU, so
`LockBlockConcurrentRuntime()` and the stock jobs targeting CPUs 0 and 1 are
not accepted as multicore runtime behavior.

On 2026-10-31, `python3 scripts/check-compat.py --pci --probe-demo-basics`
passed in PCI QEMU. It launched the original Print, DateTime, RadixSort,
SubIntAccess, SubSwitch and NullCase demos through the graphical terminal,
checked number formatting and date fields, verified all 16 RadixSort output
values equal the sorted input table, checked all four subinteger examples,
all six SubSwitch branches, all six NullCase labels, and confirmed a subsequent
`40+2;` returns 42. See
[`acceptance/demo-runtime-basics-2026-10-31.json`](acceptance/demo-runtime-basics-2026-10-31.json).

| Program/group | Evidence now available | Next acceptance gap |
|--|--|--|
| Doodle, Mass Spring, Game of Life | User confirmed live UTM drawing/simulation and Escape recovery. | Retest only if shared graphics/input code changes. |
| Digital Logic | User confirmed corrected AND workflow in live UTM; PCI workflow covers editing, setup aborts, drawing and recovery. | Retest only if shared prompt/document code changes. |
| Budget | PCI workflow covers form editing, save/reopen and cold persistence; menu-fix image has automated coverage. | Live UTM retest of the corrected menu/form image. |
| GrModels | The 2026-10-31 PCI rerun passed Man/Ball forms and mesh generation. QEMU diagnostics confirm an embedded `DOCT_SPRITE` entry with binary data; the fresh scanout assertion found 643 purple pixels for the generated Ball inline in the document. Live UTM reaches `Vertices:58` and the repeat prompt, but the user did not see a sprite; visual UTM acceptance remains open. The screenshot does not establish the selected swatch, and UTM uses `virtio-ramfb` while the gate uses plain `ramfb`. | Reproduce with the gate's bright purple swatch under UTM's display path, confirm visible output, then choose `n` and verify Escape recovery. |
| KeepAway | PCI workflow covers gameplay, Space/mouse input, Escape and shell recovery. A fresh 183-second run on the rebuilt ISO keeps scanout active through game-over and Escape recovery, then returns to a working terminal. Live UTM gameplay initially advances and the player follows the pointer, but the user reports a later guest freeze with input unresponsive. | Retest the live UTM freeze against the rebuilt ISO; ARM64 speaker effects are silent. |
| KeepAway | PCI workflow covers gameplay, Space/mouse input, Escape and shell recovery. Live UTM gameplay initially advances and the player follows the pointer, but the user reports a later guest freeze with input unresponsive. | Reproduce and diagnose the live UTM freeze; ARM64 speaker effects are silent. |
| Titanium | Live UTM confirms gameplay, Space firing, Escape recovery, and ordinary Left/Right movement on ISO `219d751c…`. The VirtIO keyboard bridge updates merged and raw scan-code bitmaps; QEMU confirms arrow movement and Escape recovery to a terminal where `40+2;` returns 42. | ARM64 speaker effects are silent. |
| Psalmody | PCI graphical probe enters the editor, accepts a note, exercises playback, and returns with Shift+Escape to a working terminal. | ARM64 audio output is silent; audible playback remains unsupported. Live UTM editor acceptance remains open. |
| Span | QEMU graphical entry/help/frame, bridge editing, Shift+Escape cleanup, worker cleanup, and shell recovery pass; rerun on 2026-10-30. | Live UTM bridge editing and save workflow. |
| Strut | QEMU help, mass placement/frame change, Space play-mode switch, Escape, and shell recovery pass; rerun on 2026-10-30. | Live UTM editing/play behavior; ship save/reopen workflow. |
| TimeClock | App compile, date/time formatter gates, and automated punch-in/out record write/read plus report-state round-trip pass. The graphical QEMU probe loads the stock app, submits both descriptions through `StrGet`, reads them back with `TimeRep;`, and recovers to a terminal where `40+2;` returns 42. The red `IN`/`OUT` report labels are below the probe's monochrome OCR threshold, so the probe asserts the saved descriptions instead. CMOS register access stays unavailable. | Verify report labels and clock behavior in live UTM; CMOS hardware clock access remains unavailable. |
| ToTheFront | ARM64 source overlay fixes the Human player's indirect-fire unit-count loop; the stock `i > UNITS_NUM` condition skipped the artillery order phase. PCI runtime probe selects both player types, displays/dismisses help, renders the generated map, selects artillery and sees its range indicator, plots an order into an adjacent hex, advances a phase with Space, and returns through Escape to a working terminal (`40+2;` -> 42). | Exercise unit movement/direct fire, view controls and broader AI behavior in QEMU, then verify setup and turn flow live in UTM. |
| Vocabulary | The PCI runtime probe launches `/Apps/Vocabulary/Run.ZC`, displays dictionary question content, accepts a numbered answer and advances, then Escape restores a terminal where `40+2;` returns 42. The autocomplete gate covers the bundled `ACDefs.DATA`/`ACWords.DATA`, boot indexing (`COMPUTER`), `ACDDefGet("Computer")`, completion popup startup and fixture completion. | Visually verify answer feedback and definition popup; check sound separately (ARM64 audio remains silent). |
| X-Caliber | QEMU reaches gameplay. Fixed a MathODE timeout comparison that mixed guest uptime with host monotonic time; ODE integration now advances. In separate fresh-state probes, Left and Right each visibly move the ship when tested first (45 and 43 changed ship pixels); the opposite direction tested second in the same session sets its bitmap and thrust value but produces no visible change. Escape returns to the terminal and `40+2;` returns 42. Space firing and reliable sequential left/right control remain unverified. Do not add a `Yield` to `MessageScan` or an X-Caliber-specific `Yield`; the cooperative scheduler regression passes. | Isolate the second-direction state and verify Space firing, then repeat live UTM gameplay/Escape. |

The primary current named-app tracker is [PROGRAMS.md](../PROGRAMS.md). The
latest Titanium automated result is stronger than the earlier status recorded
in the 2026-10-27 catalog report; update the report when the live UTM check is
accepted.

## Work plan

### 1. Close the near-complete apps in live UTM

Use the current ISO without reinstalling for non-persistent checks. Keep the
VirtIO disk attached for Budget. Verify Budget's corrected menu and entry
workflow, GrModels' two generation paths, KeepAway input/exit, and Titanium's
key-to-gameplay, Space, Escape and shell recovery. Record exact ISO hash and
screenshots/results. This closes the gap between PCI automation and the user's
actual UTM environment.

### 2. Finish remaining named Apps by shared dependency

1. **Forms and storage:** Span and Strut pass their QEMU edit/play and exit
   gates; next verify their save workflows in UTM, then focus TimeClock and
   X-Caliber (clock/registry/persistence). Reuse the Budget form and cold-boot
   gates where applicable.
2. **Compiler and bundled data:** ToTheFront's setup/map/turn runtime probe and
   Vocabulary's quiz/index checks pass in QEMU; verify UTM behavior and expand
   ToTheFront to unit selection/movement, firing, view controls and AI.
   Keep compiler support separate from app-data availability so failures are easy to attribute.
3. **Audio and music:** add a bounded ARM64 audio service, then complete
   Psalmody note conversion/playback and the Sound demos. A silent no-op hook
   is compile compatibility, not audio acceptance.

### 3. Triage the Demo source inventory

Classify each `.ZC` source as runnable, helper/module, template, generator or
hardware-specific. Then work by shared systems:

1. **Graphics and visualization:** remaining `Demo/Graphics` programs, using
   the current sprite/CDC/input coverage; test actual drawing, animation,
   resize/exit and framebuffer output.
2. **Games:** `Demo/Games`, reusing keyboard, pointer, timer, graphics and sound
   gates; prioritize games with no x86-specific code and simple exit paths.
3. **Documents, menus, forms and files:** `Demo/DolDoc`, `Demo/InFile`,
   `Demo/Disk`, `Demo/ToHtmlToTXTDemo` and account examples; focus on editor,
   serialization, directory, file-filter and form services.
4. **Sound:** `Demo/Sound` after the audio service lands.
5. **Compiler, multicore and hardware examples:** `Demo/Asm`, `Demo/Lectures`,
   and `Demo/MultiCore`. Separate portable language examples from x86
   assembly, PCI-specific and multicore assumptions; port or mark target-only
   behavior explicitly.
6. **Small utilities and templates:** root-level demos, sorting/file demos,
   WebLog demos and `Demo/Templates`; verify only those intended to be run
   directly, not include-only examples.

### 4. Keep evidence honest and reusable

For each runnable program, add a catalog entry with source path, data
dependencies, architecture assumptions, launch command, expected interaction,
exit behavior and acceptance level. Compile-only catalog success must never be
reported as a working app. Use focused QEMU automation for repeatable checks,
then live UTM/Pi checks for host/device-specific behavior. Keep one program's
failure from being hidden by a batch's aggregate success.

## Exhaustive `.ZC` source-name inventory

This list is generated from the pinned tree's `.ZC` filenames. Names within a
directory are file stems. It is a source inventory, not an assertion that
every file is a standalone application.

### Apps (70 files)

| Directory | Files |
|--|--|
| Budget (10) | BgtAccts, BgtEntry, BgtFile, BgtMain, BgtStrs, BgtTemplate, Budget, Install, Load, Run |
| GrModels (5) | BallGen, Load, ManGen, Models, Run |
| KeepAway (3) | KeepAway, Load, Run |
| Logic (3) | Load, Logic, Run |
| Psalmody (13) | childish, night, prosper, Install, JukeBox, JukePuppet, Load, Psalmody, PsalmodyCtrls, PsalmodyDraw, PsalmodyFile, PsalmodyMain, Run |
| Span (7) | Install, Load, Run, SpanBridge, SpanDerive, SpanMain, SpanNew |
| Strut (3) | Load, Run, Strut |
| TimeClock (3) | Install, Load, TimeClk |
| Titanium (3) | Load, Run, Titanium |
| ToTheFront (13) | Human, SimpleAI, Install, Load, Run, TTFDraw, TTFFire, TTFGlobals, TTFInit, TTFLib, TTFMove, TTFVis, ToTheFront |
| Vocabulary (3) | Load, Run, VocabQuiz |
| X-Caliber (4) | Load, Run, X-Caliber, XCCtrls |

### Demo (206 files)

| Directory | Files |
|--|--|
| AcctExample (13) | HomeKeyPlugIns, HomeLocalize, HomeSys, HomeWrappers, MakeHome, Once, Registry, MakeTOS, TOSConfig, TOSDistro, TOSExtern, TOSMisc, TOSTheme |
| Asm (8) | AsmAndC1, AsmAndC2, AsmAndC3, AsmHelloWorld, BuzzFizz, DivByHand, MulByHand, PutDec |
| root (34) | Carry, ClassMeta, CompileDemo, DateTime, DebugDemo, Define, Directives, Exceptions, ExtChars, GlobalVars, KeyBitMap, LastClass, MagicPairs, MemDemo, MessageLoop, NullCase, OnceDemo, ParenWarn, PhoneNumWords, Print, Progress, Prompt, PullDownMenu, RadixSort, RandDemo, RegistryDemo, ScreenCodes, Spy, StackGrow, SubIntAccess, SubSwitch, SuggestSpelling, TimeIns, WordSearch |
| Disk (5) | DataBase, DiskRaw, FPrintF, SerializeTree, UnusedSpaceRep |
| DolDoc (13) | CallBack, ClickCallBack, CursorMove, Data, DefineStr, FileRead, Form, MenuBttn, MenuSprite, NumBible, TextDemo, TreeDemo, UnusedDefine |
| Games (33) | BattleLines, BigGuns, BlackDiamond, BomberGolf, CastleFrankenstein, CharDemo, CircleTrace, Collision, Digits, DunGen, ElephantWalk, FlapBat, FlatTops, Halogen, MassSpring, Maze, RainDrops, RawHide, Rocket, RocketScience, Squirt, Stadium, StadiumGen, Talons, TheDead, TicTacToe, TreeCheckers, Varoom, Wenceslas, Whap, Zing, ZoneOut |
| Graphics (59) | 32BitColor, 32BitPaint, 3DPoly, BSpline, Blot, Bounce, Box, Cartesian, CharAnimation, Clock, Collision, CommonAncestor, Doodle, EdSprite, Elephant, Extents, FontEd, GrDir, Grid, Hanoi, Lattice, Life, LightTable, Lines, LowPassFilter, Mandel, MathAudioDemo, MouseDemo, NetOfDots, Palette, PanText, Pick, Pick3D, PoleZeros, PredatorPrey, RotateTank, ScreenCapture, ScrollBars, Shading, Shadow, Slider, Speedline, SpritePlot, SpritePlot3D, SpritePut, SpritePutExt, SpriteRaw, SpriteText, SunMoon, Symmetry, Transform, Usage, WallPaperCtrl, HomeSys, Install, WallPaperDesktop, WallPaperFish, WallPaperStars, WinZBuf |
| InFile (2) | InPopUp, InSetIP |
| Lectures (11) | AndNotMod, FixedPoint, FixedPointAdvanced, InterruptDemo, LexDemo, MiniCompiler, MiniGrLib, NegDisp, Optimization, PCIInterrupts, Ring3 |
| MultiCore (8) | Interrupts, LoadTest, Lock, MPAdd, MPPrint, MPRadix, Palindrome, Primes |
| RevFileDemo (1) | Rev |
| SortFileDemo (2) | F64FileGen, F64FileSort |
| Sound (4) | ASCIIOrgan, MorseCode, OhGreat, WaterFowl |
| Templates (7) | Game1, Lex1, MultiCore1, ODE1, Registry1, Util1, Util2 |
| ToHtmlToTXTDemo (4) | HtmlDirList, HtmlGen, TXTGen, ToHtml |
| WebLogDemo (2) | WebLogRep, WebLogScramble |

## 2026-11-01 follow-up

The full PCI QEMU source sweep now compiles 179 of 206 Demo `.ZC` sources;
27 remain blocked. One blocker was a compiler bookkeeping bug: inferred-size
static local arrays kept an unknown total size after their initializer had
computed the bound, so `sizeof(array)` failed. The parser now publishes the
inferred total, and `StockCatalogStaticArraySizeChecks` verifies both the
array size and initialized class-array contents. The original
`/Demo/GlobalVars.ZC` now compiles. The focused regression and complete sweep
are recorded in
[`acceptance/demo-globalvars-array-size-2026-11-01.json`](acceptance/demo-globalvars-array-size-2026-11-01.json).
The count is compile evidence only. Separate QEMU checks now bring the Demo
runtime accepted count to twenty-eight: unchanged TicTacToe and Chess both render,
accept input where applicable, exit cleanly, and return to a terminal where
`40+2;` evaluates to 42. MouseDemo, Grid, and Palette also passed pointer/palette interaction and terminal recovery in QEMU. Slider launches but its control was not visible in the captured guest screen and its value did not change; it is tracked as partial pending control rendering and hit-testing work. The original NetOfDots and Lines programs also render, accept exit input, and recover the terminal; Lines animation was verified by changing scanout. See
[`acceptance/demo-stock-graphics-2026-11-01.json`](acceptance/demo-stock-graphics-2026-11-01.json). Pick3D now passes its QEMU interactive graphics probe, including visible graphics, Escape, and terminal recovery. The failure was a missing bootstrap implementation of the stock task-local `HashGenericAdd` and `FramePtr` helpers. See
[`acceptance/demo-interactive-games-2026-11-01.json`](acceptance/demo-interactive-games-2026-11-01.json).
The added pointer/palette batch is recorded in
[`acceptance/demo-input-graphics-2026-11-01.json`](acceptance/demo-input-graphics-2026-11-01.json).
UTM-specific app freezes still require separate live checks. `CompileDemo.ZC` now compiles when its isolated catalog context loads the stock compiler headers and program modules; its dynamic compiler output still needs runtime acceptance.

The refreshed 2026-11-01 sweep also fixed a source-catalog false negative for `/Demo/Graphics/WallPapers/WallPaperDesktop.ZC`: the stock compiler modules define `cmp`, and the source-preserving loader keeps the class-layout `$$` token intact. `StockCatalogCompileRawWithProgramModules` now verifies this path as part of the complete sweep. The original source remains unchanged; this is compile acceptance only, not runtime acceptance.

The latest full sweep updates the source ledger to 179/206 compiling. The
isolated catalog now loads the stock compiler headers and program modules for
`/Demo/MultiCore/LoadTest.ZC`, which was previously a probe-context false
negative. The `Mandel.ZC` ARM64 overlay replaces its unsupported conditional
operator with equivalent ZealC syntax for the single-CPU case. The rebuilt
QEMU image rendered Mandel, handled Escape, and returned to a terminal where
`40+2;` evaluated to 42. See
[`acceptance/demo-mandel-qemu-2026-11-01.json`](acceptance/demo-mandel-qemu-2026-11-01.json).

The focused `/Demo/GlobalVars.ZC` run also verifies its JIT dynamic-initialization
half (`Time 1`, `Time 2`) and terminal recovery. Its AOT-only `#ifjit` section
remains unverified because the bootstrap does not expose the stock `Comp()`
command. This is partial evidence and is not included in the fully
accepted Demo program count; see
[`acceptance/demo-global-vars-jit-2026-11-01.json`](acceptance/demo-global-vars-jit-2026-11-01.json).

The 2026-11-01 `/Demo/Exceptions.ZC` PCI QEMU probe now passes the nested Point1 catch path, including ARM64 `%P` pointer-link formatting, final prompt completion, and recovery to a terminal where `40+2;` returns 42. See [`acceptance/demo-exceptions-2026-11-01.json`](acceptance/demo-exceptions-2026-11-01.json). This is one explicit runtime acceptance, not full coverage of every exception path or `%P` call site.

The original `/Demo/Graphics/Mandel.ZC` now renders on the single-CPU ARM64 QEMU guest. Its guarded source-bundle overlay runs the stock frame worker synchronously on CPU 0 when there are no worker CPUs, and preserves the upstream `JobQueue` path for multi-CPU guests. Escape restores the terminal and `40+2;` returns 42. See [`acceptance/demo-mandel-qemu-2026-11-01.json`](acceptance/demo-mandel-qemu-2026-11-01.json).

The original `/Demo/Graphics/Bounce.ZC`, `Clock.ZC`, `3DPoly.ZC`, and `RotateTank.ZC` also pass QEMU graphical rendering and exit recovery. Bounce, 3DPoly, and RotateTank return on Escape; Clock uses Shift+Escape. Each returned to a terminal where `40+2;` evaluated to 42. See [`acceptance/demo-bounce-clock-3dpoly-2026-11-01.json`](acceptance/demo-bounce-clock-3dpoly-2026-11-01.json).

The original `/Demo/Graphics/32BitColor.ZC` and `Blot.ZC` also pass PCI QEMU
framebuffer rendering and terminal recovery: they changed 38,486 and 56,555
scanout pixels respectively, and each returned to a terminal where `40+2;`
evaluated to 42. See
[`acceptance/demo-graphics-render-2026-11-01.json`](acceptance/demo-graphics-render-2026-11-01.json).
