# ZealOS ARM64 program inventory and completion plan

**Purpose:** Track what is present in the pinned ZealOS source bundle and plan
the work to make regular Apps and Demos usable on ARM64 under QEMU/UTM and,
where the hardware path applies, Raspberry Pi 4B.

**Source baseline:** pinned upstream source commit
`eee210f4379ba3d00b47507f336a158eefb2c993`.

## Scope and counting rules

The pinned `Apps` tree has 12 top-level application directories and 70 `.ZC`
files. The pinned `Demo` tree has 206 `.ZC` files in 16 source directories.
The mirror also contains documents, templates, data and binary resources.
These are file counts, not counts of independently launchable programs:
`Load.ZC`, `Run.ZC`, helpers and examples all contribute source files.

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

On 2026-10-29, the isolated compile catalog compiled all 12 app entry points
and five shared/demo checks; the ToTheFront and X-Caliber loaders also passed
normal-load checks. TimeClock's app compile and focused date/time formatter
gate pass. This remains compile/load evidence only except where noted in the
table. Vocabulary's new PCI runtime probe launches the quiz, displays its
dictionary content, accepts a numbered answer and advances, then exits with
Escape to a terminal whose `40+2;` expression returns 42. This does not yet
visually validate the short-lived answer feedback or dictionary popup. Focused
graphical QEMU probes verified Span's bridge frame,
Shift+Escape cleanup, and terminal recovery, plus Strut's help, mass-placement
frame change, play-mode switch, Escape, and terminal recovery (`40+2;` -> 42).
A ToTheFront GUI launch now displays and accepts text in the seed popup after
popup-server stdout is routed into its DolDoc. The subsequent player-type
picker shows its title and AI-directory guidance, but no choices in scanout;
gameplay acceptance is still blocked on that selector.

| Program/group | Evidence now available | Next acceptance gap |
|--|--|--|
| Doodle, Mass Spring, Game of Life | User confirmed live UTM drawing/simulation and Escape recovery. | Retest only if shared graphics/input code changes. |
| Digital Logic | User confirmed corrected AND workflow in live UTM; PCI workflow covers editing, setup aborts, drawing and recovery. | Retest only if shared prompt/document code changes. |
| Budget | PCI workflow covers form editing, save/reopen and cold persistence; menu-fix image has automated coverage. | Live UTM retest of the corrected menu/form image. |
| GrModels | PCI workflow covers Man/Ball forms and mesh generation. Live UTM reaches `Vertices:58` and the repeat prompt after choosing a color. QEMU diagnostics confirm an embedded `DOCT_SPRITE` entry with binary data, and a QEMU capture rendered a small magenta Ball sprite. The user reports no sprite visible in UTM after color selection; do not count the app as visually accepted yet. | Reproduce the missing UTM sprite; determine whether it is hidden/offscreen or a renderer/presentation difference, then verify a clearly visible sprite and Escape/recovery. At the repeat prompt choose `n` to return. |
| KeepAway | PCI workflow covers gameplay, Space/mouse input, Escape and shell recovery. Live UTM gameplay initially advances and the player follows the pointer, but the user reports a later guest freeze with input unresponsive. | Reproduce and diagnose the live UTM freeze; ARM64 speaker effects are silent. |
| Titanium | Live UTM confirms gameplay, Space firing, Escape recovery, and ordinary Left/Right movement on ISO `219d751c…`. The VirtIO keyboard bridge updates merged and raw scan-code bitmaps; QEMU confirms arrow movement and Escape recovery to a terminal where `40+2;` returns 42. | ARM64 speaker effects are silent. |
| Psalmody | Normal loader and app compile pass; catalog regression checks note/octave conversion helpers and default shared music state. | ARM64 audio path and interactive song editing/playback. |
| Span | QEMU graphical entry/help/frame, Shift+Escape cleanup, worker cleanup, and shell recovery pass. | Live UTM bridge editing and save workflow. |
| Strut | QEMU help, mass placement/frame change, Space play-mode switch, Escape, and shell recovery pass. | Live UTM editing/play behavior; ship save/reopen workflow. |
| TimeClock | App compile and focused `ModU64`, Gregorian date conversion, date/time text, and timezone formatter gates pass. CMOS register access stays unavailable. | Exercise punch-in/out, persistence, and reports in QEMU/UTM. |
| ToTheFront | Compile and normal load pass after ARM64 pointer-add address generation and guarded source overlays for its sentinel, pure timing helpers, and runtime-sized map clears. Popup text now displays in its own window. The player-type picker renders its title and AI-directory guidance, but no selectable entries are visible. | Diagnose AI picker choices/data, then exercise map initialization, turns, drawing, input, and AI in QEMU/UTM. |
| Vocabulary | The PCI runtime probe launches `/Apps/Vocabulary/Run.ZC`, displays dictionary question content, accepts a numbered answer and advances, then Escape restores a terminal where `40+2;` returns 42. The autocomplete gate covers the bundled `ACDefs.DATA`/`ACWords.DATA`, boot indexing (`COMPUTER`), `ACDDefGet("Computer")`, completion popup startup and fixture completion. | Visually verify answer feedback and definition popup; check sound separately (ARM64 audio remains silent). |
| X-Caliber | Normal loader and app entry compile. | Runtime behavior, controls and data persistence. |

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

1. **Forms and storage:** Span and Strut first (shared control-button/form
   work), then TimeClock and X-Caliber (clock/registry/persistence). Reuse the
   Budget form and cold-boot gates where applicable.
2. **Compiler and bundled data:** run ToTheFront's newly compiling game and resolve any runtime blockers;
   initialize Vocabulary's already-bundled dictionary/index and exercise its quiz.
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
