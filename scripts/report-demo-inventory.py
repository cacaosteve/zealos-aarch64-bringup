#!/usr/bin/env python3
"""Build a source-level status ledger for the pinned ZealOS Demo tree.

The directory-derived kind is intentionally provisional: a directory is not
proof that each file is a standalone program. Runtime acceptance is populated
only from explicit evidence in the checked-in acceptance record.
"""
import json
import hashlib
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "upstream/pinned/Demo"
CATALOG = ROOT / "build/demo-source-catalog.json"
OUTPUT = ROOT / "docs/acceptance/demo-source-ledger-2026-11-10.json"
SUPPLEMENTAL_COMPILE = {
    "/Demo/AcctExample/MakeHome.ZC":
        "docs/acceptance/demo-makehome-arm64-compile-2026-11-03.json",
    "/Demo/Graphics/LightTable.ZC":
        "docs/acceptance/demo-lighttable-arm64-compile-2026-11-02.json",
}

KINDS = {
    "AcctExample": "account/home-system integration examples",
    "Asm": "architecture-specific assembly examples",
    "Disk": "disk and storage examples",
    "DolDoc": "DolDoc examples and components",
    "Games": "interactive game sources",
    "Graphics": "graphics examples and components",
    "Lectures": "lecture and tutorial sources",
    "MultiCore": "multicore and interrupt examples",
    "Sound": "sound examples",
    "Templates": "templates",
}
RUNTIME_ACCEPTED = {
    "/Demo/AcctExample/TOS/TOSConfig.ZC":
        "docs/acceptance/demo-tos-config-qemu-2026-11-08.json",
    "/Demo/AcctExample/TOS/TOSExtern.ZC":
        "docs/acceptance/demo-tos-extern-interface-qemu-2026-11-10.json",
    "/Demo/AcctExample/TOS/TOSTheme.ZC":
        "docs/acceptance/demo-tos-theme-qemu-2026-11-08.json",
    "/Demo/Templates/Lex1.ZC":
        "docs/acceptance/demo-lex1-qemu-2026-11-08.json",
    "/Demo/Templates/ODE1.ZC":
        "docs/acceptance/demo-ode1-qemu-2026-11-08.json",
    "/Demo/Templates/Util1.ZC":
        "docs/acceptance/demo-util1-qemu-2026-11-08.json",
    "/Demo/Templates/Util2.ZC":
        "docs/acceptance/demo-util2-qemu-2026-11-08.json",
    "/Demo/WebLogDemo/WebLogScramble.ZC":
        "docs/acceptance/demo-weblog-scramble-qemu-2026-11-08.json",
    "/Demo/Lectures/FixedPoint.ZC":
        "docs/acceptance/demo-fixed-point-clock-qemu-2026-11-08.json",
    "/Demo/Lectures/FixedPointAdvanced.ZC":
        "docs/acceptance/demo-fixed-point-advanced-qemu-2026-11-08.json",
    "/Demo/Lectures/Optimization.ZC":
        "docs/acceptance/demo-optimization-qemu-2026-11-08.json",
    "/Demo/Templates/MultiCore1.ZC":
        "docs/acceptance/demo-multicore1-qemu-2026-11-08.json",
    "/Demo/Graphics/WallPapers/WallPaperCtrl.ZC":
        "docs/acceptance/demo-wallpaper-ctrl-qemu-2026-11-08.json",
    "/Demo/Graphics/WallPapers/WallPaperDesktop.ZC":
        "docs/acceptance/demo-wallpaper-desktop-current-qemu-2026-11-10.json",
    "/Demo/Graphics/WallPapers/WallPaperDesktop/Install.ZC":
        "docs/acceptance/demo-wallpaper-install-qemu-2026-11-08.json",
    "/Demo/Graphics/WallPapers/WallPaperStars.ZC":
        "docs/acceptance/demo-runtime-followup-qemu-2026-11-07.json",
    "/Demo/Graphics/WallPapers/WallPaperFish.ZC":
        "docs/acceptance/demo-runtime-followup-qemu-2026-11-07.json",
    "/Demo/MultiCore/MPPrint.ZC":
        "docs/acceptance/demo-mpprint-qemu-2026-11-08-cpu0-color.json",
    "/Demo/MultiCore/LoadTest.ZC":
        "docs/acceptance/demo-loadtest-qemu-2026-11-09.json",
    "/Demo/WebLogDemo/WebLogRep.ZC":
        "docs/acceptance/demo-runtime-followup-qemu-2026-11-07.json",
    "/Demo/ToHtmlToTXTDemo/HtmlDirList.ZC":
        "docs/acceptance/demo-runtime-followup-qemu-2026-11-07.json",
    "/Demo/Spy.ZC":
        "docs/acceptance/demo-spy-qemu-2026-11-07.json",
    "/Demo/Disk/DataBase.ZC":
        "docs/acceptance/demo-database-qemu-2026-11-07.json",
    "/Demo/AcctExample/HomeKeyPlugIns.ZC":
        "docs/acceptance/demo-home-key-plugins-qemu-2026-11-07.json",
    "/Demo/Lectures/MiniGrLib.ZC":
        "docs/acceptance/demo-mini-grlib-qemu-2026-11-07.json",
    "/Demo/InFile/InSetIP.ZC":
        "docs/acceptance/demo-insetip-mouseset-qemu-2026-11-07.json",
    "/Demo/Disk/UnusedSpaceRep.ZC":
        "docs/acceptance/demo-unused-space-qemu-2026-11-07.json",
    "/Demo/Templates/Game1.ZC":
        "docs/acceptance/demo-template-game1-qemu-2026-11-07.json",
    "/Demo/Lectures/NegDisp.ZC":
        "docs/acceptance/demo-negdisp-qemu-2026-11-07.json",
    "/Demo/Templates/Registry1.ZC":
        "docs/acceptance/demo-template-registry1-qemu-2026-11-07.json",
    "/Demo/SortFileDemo/F64FileGen.ZC":
        "docs/acceptance/demo-sort-file-qemu-2026-11-07.json",
    "/Demo/SortFileDemo/F64FileSort.ZC":
        "docs/acceptance/demo-sort-file-qemu-2026-11-07.json",
    "/Demo/GlobalVars.ZC":
        "docs/acceptance/demo-global-vars-jit-2026-11-07.json",
    "/Demo/StackGrow.ZC":
        "docs/acceptance/demo-stack-grow-qemu-2026-11-06.json",
    "/Demo/Lectures/AndNotMod.ZC":
        "docs/acceptance/demo-and-not-mod-qemu-2026-11-06.json",
    "/Demo/ToHtmlToTXTDemo/TXTGen.ZC":
        "docs/acceptance/demo-txt-gen-qemu-2026-11-06.json",
    "/Demo/ToHtmlToTXTDemo/HtmlGen.ZC":
        "docs/acceptance/demo-html-gen-qemu-2026-11-06.json",
    # HtmlGen includes and executes the stock ToHtml module, so this helper
    # has runtime evidence through that end-to-end converter workflow.
    "/Demo/ToHtmlToTXTDemo/ToHtml.ZC":
        "docs/acceptance/demo-html-gen-qemu-2026-11-06.json",
    "/Demo/AcctExample/MakeHome.ZC":
        "docs/acceptance/demo-makehome-qemu-2026-11-05.json",
    "/Demo/AcctExample/TOS/MakeTOS.ZC":
        "docs/acceptance/demo-maketos-qemu-2026-11-05.json",
    "/Demo/DolDoc/NumBible.ZC":
        "docs/acceptance/demo-num-bible-qemu-2026-11-05.json",
    "/Demo/DolDoc/MenuSprite.ZC":
        "docs/acceptance/demo-menu-sprite-qemu-2026-11-06.json",
    "/Demo/DolDoc/MenuBttn.ZC":
        "docs/acceptance/demo-menu-bttn-qemu-2026-11-07.json",
    "/Demo/OnceDemo.ZC":
        "docs/acceptance/demo-oncedemo-qemu-2026-11-06.json",
    "/Demo/Graphics/WinZBuf.ZC":
        "docs/acceptance/demo-winzbuf-qemu-2026-11-06.json",
    "/Demo/Graphics/SpriteText.ZC":
        "docs/acceptance/demo-sprite-text-qemu-2026-11-06.json",
    "/Demo/Graphics/GrDir.ZC":
        "docs/acceptance/demo-grdir-qemu-2026-11-06.json",
    "/Demo/Graphics/LightTable.ZC":
        "docs/acceptance/demo-lighttable-qemu-2026-11-06.json",
    "/Demo/Lectures/LexDemo.ZC":
        "docs/acceptance/demo-lex-demo-qemu-2026-11-06.json",
    "/Demo/Graphics/Elephant.ZC":
        "docs/acceptance/demo-elephant-qemu-2026-11-05.json",
    "/Demo/Graphics/FontEd.ZC":
        "docs/acceptance/demo-fonted-qemu-2026-11-05.json",
    "/Demo/Graphics/PoleZeros.ZC":
        "docs/acceptance/demo-polezeros-qemu-2026-11-05.json",
    "/Demo/Games/Stadium/StadiumGen.ZC":
        "docs/acceptance/demo-stadium-gen-qemu-2026-11-04.json",
    "/Demo/Games/Wenceslas.ZC":
        "docs/acceptance/demo-wenceslas-qemu-2026-11-04.json",
    "/Demo/Graphics/Transform.ZC":
        "docs/acceptance/demo-transform-qemu-2026-11-04.json",
    "/Demo/Graphics/Speedline.ZC":
        "docs/acceptance/demo-speedline-qemu-2026-11-04.json",
    "/Demo/Graphics/32BitColor.ZC":
        "docs/acceptance/demo-32bit-color-qemu-2026-11-04.json",
    "/Demo/Graphics/EdSprite.ZC":
        "docs/acceptance/demo-edsprite-qemu-2026-11-04.json",
    "/Demo/AcctExample/HomeLocalize.ZC":
        "docs/acceptance/demo-home-localize-qemu-2026-11-05.json",
    "/Demo/AcctExample/HomeSys.ZC":
        "docs/acceptance/demo-home-sys-qemu-2026-11-05.json",
    "/Demo/AcctExample/Registry.ZC":
        "docs/acceptance/demo-acct-registry-qemu-2026-11-05.json",
    "/Demo/AcctExample/Once.ZC":
        "docs/acceptance/demo-acct-once-qemu-2026-11-05.json",
    "/Demo/DolDoc/CallBack.ZC":
        "docs/acceptance/demo-callback-qemu-2026-11-05.json",
    "/Demo/DolDoc/ClickCallBack.ZC":
        "docs/acceptance/demo-click-callback-qemu-2026-11-05.json",
    "/Demo/RandDemo.ZC": "docs/acceptance/demo-interactive-basics-qemu-2026-11-01.json",
    "/Demo/MessageLoop.ZC": "docs/acceptance/demo-interactive-basics-qemu-2026-11-01.json",
    "/Demo/RegistryDemo.ZC": "docs/acceptance/demo-registry-qemu-2026-11-01.json",
    "/Demo/Prompt.ZC": "docs/acceptance/demo-prompt-qemu-2026-11-01.json",
    "/Demo/DateTime.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/MemDemo.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/Carry.ZC": "docs/acceptance/demo-carry-qemu-2026-11-02.json",
    "/Demo/Asm/AsmHelloWorld.ZC": "docs/acceptance/demo-asm-output-qemu-2026-11-02.json",
    "/Demo/Asm/BuzzFizz.ZC": "docs/acceptance/demo-asm-output-qemu-2026-11-02.json",
    "/Demo/Asm/PutDec.ZC": "docs/acceptance/demo-asm-output-qemu-2026-11-02.json",
    "/Demo/Asm/MulByHand.ZC": "docs/acceptance/demo-asm-arithmetic-qemu-2026-11-02.json",
    "/Demo/Asm/DivByHand.ZC": "docs/acceptance/demo-asm-arithmetic-qemu-2026-11-02.json",
    "/Demo/Asm/AsmAndC3.ZC": "docs/acceptance/demo-asmandc3-qemu-2026-11-02.json",
    "/Demo/Asm/AsmAndC1.ZC": "docs/acceptance/demo-asmandc12-qemu-2026-11-05.json",
    "/Demo/Asm/AsmAndC2.ZC": "docs/acceptance/demo-asmandc12-qemu-2026-11-05.json",
    "/Demo/MagicPairs.ZC": "docs/acceptance/demo-magicpairs-qemu-2026-11-05.json",
    "/Demo/RevFileDemo/Rev.ZC": "docs/acceptance/demo-revfile-qemu-2026-11-05.json",
    "/Demo/InFile/InPopUp.ZC": "docs/acceptance/demo-inpopup-qemu-2026-11-05.json",
    "/Demo/Disk/SerializeTree.ZC": "docs/acceptance/demo-serialize-tree-qemu-2026-11-05.json",
    "/Demo/Define.ZC": "docs/acceptance/demo-define-qemu-2026-11-02.json",
    "/Demo/PhoneNumWords.ZC": "docs/acceptance/demo-phone-num-words-qemu-2026-11-02.json",
    "/Demo/Directives.ZC": "docs/acceptance/demo-directives-qemu-2026-11-02.json",
    "/Demo/ExtChars.ZC": "docs/acceptance/demo-extchars-qemu-2026-11-02.json",
    "/Demo/ScreenCodes.ZC": "docs/acceptance/demo-screencodes-qemu-2026-11-02.json",
    "/Demo/KeyBitMap.ZC": "docs/acceptance/demo-keybitmap-qemu-2026-11-02.json",
    "/Demo/NullCase.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/Print.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/RadixSort.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/SubIntAccess.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/SubSwitch.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/Exceptions.ZC": "docs/acceptance/demo-exceptions-2026-11-01.json",
    "/Demo/Games/Chess.ZC": "docs/acceptance/demo-interactive-games-2026-11-01.json",
    "/Demo/Games/CircleTrace.ZC": "docs/acceptance/demo-circletrace-qemu-2026-11-01.json",
    "/Demo/Games/Maze.ZC": "docs/acceptance/demo-maze-qemu-2026-11-02.json",
    "/Demo/Games/BlackDiamond.ZC": "docs/acceptance/demo-blackdiamond-qemu-2026-11-02.json",
    "/Demo/Games/CharDemo.ZC": "docs/acceptance/demo-char-demo-qemu-2026-11-03.json",
    "/Demo/Games/BigGuns.ZC": "docs/acceptance/demo-big-guns-qemu-2026-11-03.json",
    "/Demo/Games/Halogen.ZC": "docs/acceptance/demo-halogen-qemu-2026-11-03.json",
    "/Demo/Games/ZoneOut.ZC": "docs/acceptance/demo-zoneout-qemu-2026-11-03.json",
    "/Demo/Graphics/Shading.ZC": "docs/acceptance/demo-shading-qemu-2026-11-03.json",
    "/Demo/Graphics/CharAnimation.ZC": "docs/acceptance/demo-char-animation-qemu-2026-11-03.json",
    "/Demo/Graphics/LowPassFilter.ZC": "docs/acceptance/demo-low-pass-filter-qemu-2026-11-03.json",
    "/Demo/Games/ElephantWalk.ZC": "docs/acceptance/demo-elephant-walk-qemu-2026-11-02.json",
    "/Demo/Games/FlapBat.ZC": "docs/acceptance/demo-flapbat-qemu-2026-11-02.json",
    "/Demo/Graphics/32BitPaint.ZC": "docs/acceptance/demo-32bit-paint-qemu-2026-11-02.json",
    "/Demo/Games/RainDrops.ZC": "docs/acceptance/demo-raindrops-qemu-2026-11-02.json",
    "/Demo/Games/Digits.ZC": "docs/acceptance/demo-digits-qemu-2026-11-02.json",
    "/Demo/Games/TicTacToe.ZC": "docs/acceptance/demo-interactive-games-2026-11-01.json",
    "/Demo/Games/MassSpring.ZC": "docs/acceptance/demo-stock-interactive-2026-11-01.json",
    "/Demo/Graphics/Lines.ZC": "docs/acceptance/demo-stock-graphics-2026-11-01.json",
    "/Demo/Graphics/3DPoly.ZC": "docs/acceptance/demo-bounce-clock-qemu-2026-11-02.json",
    "/Demo/Graphics/Bounce.ZC": "docs/acceptance/demo-bounce-clock-qemu-2026-11-02.json",
    "/Demo/Graphics/Clock.ZC": "docs/acceptance/demo-bounce-clock-qemu-2026-11-02.json",
    "/Demo/Graphics/RotateTank.ZC": "docs/acceptance/demo-bounce-clock-qemu-2026-11-02.json",
    "/Demo/Graphics/Blot.ZC": "docs/acceptance/demo-graphics-render-2026-11-01.json",
    "/Demo/Graphics/PanText.ZC": "docs/acceptance/demo-pan-text-qemu-2026-11-03.json",
    "/Demo/Graphics/SunMoon.ZC": "docs/acceptance/demo-sun-moon-qemu-2026-11-03.json",
    "/Demo/Graphics/Pick.ZC": "docs/acceptance/demo-pick-qemu-2026-11-05.json",
    "/Demo/Graphics/CommonAncestor.ZC": "docs/acceptance/demo-common-ancestor-qemu-2026-11-05.json",
    "/Demo/Graphics/Symmetry.ZC": "docs/acceptance/demo-symmetry-qemu-2026-11-05.json",
    "/Demo/Graphics/ScrollBars.ZC": "docs/acceptance/demo-scrollbars-qemu-2026-11-03.json",
    "/Demo/Graphics/Box.ZC": "docs/acceptance/demo-box-hanoi-2026-11-01.json",
    "/Demo/Graphics/Hanoi.ZC": "docs/acceptance/demo-box-hanoi-2026-11-01.json",
    "/Demo/Graphics/Mandel.ZC": "docs/acceptance/demo-mandel-qemu-2026-11-01.json",
    "/Demo/Graphics/MouseDemo.ZC": "docs/acceptance/demo-input-graphics-2026-11-01.json",
    "/Demo/Graphics/Slider.ZC": "docs/acceptance/demo-slider-qemu-2026-11-02.json",
    "/Demo/Graphics/Cartesian.ZC": "docs/acceptance/demo-cartesian-qemu-2026-11-03.json",
    "/Demo/Graphics/BSpline.ZC": "docs/acceptance/demo-bspline-qemu-2026-11-03.json",
    "/Demo/Graphics/Collision.ZC": "docs/acceptance/demo-collision-qemu-2026-11-03.json",
    "/Demo/Graphics/Shadow.ZC": "docs/acceptance/demo-shadow-qemu-2026-11-03.json",
    "/Demo/Graphics/Extents.ZC": "docs/acceptance/demo-extents-qemu-2026-11-03.json",
    "/Demo/Graphics/SpritePut.ZC": "docs/acceptance/demo-sprite-put-qemu-2026-11-03.json",
    "/Demo/Graphics/SpritePutExt.ZC": "docs/acceptance/demo-sprite-put-family-qemu-2026-11-03.json",
    "/Demo/Graphics/SpriteRaw.ZC": "docs/acceptance/demo-sprite-put-family-qemu-2026-11-03.json",
    "/Demo/Graphics/SpritePlot.ZC": "docs/acceptance/demo-sprite-put-family-qemu-2026-11-03.json",
    "/Demo/Graphics/SpritePlot3D.ZC": "docs/acceptance/demo-sprite-plot3d-qemu-2026-11-03.json",
    "/Demo/Graphics/Usage.ZC": "docs/acceptance/demo-usage-qemu-2026-11-03.json",
    "/Demo/DolDoc/TextDemo.ZC": "docs/acceptance/demo-dol-doc-qemu-2026-11-03.json",
    "/Demo/DolDoc/CursorMove.ZC": "docs/acceptance/demo-dol-doc-qemu-2026-11-03.json",
    "/Demo/DolDoc/DefineStr.ZC": "docs/acceptance/demo-dol-doc-qemu-2026-11-03.json",
    "/Demo/DolDoc/TreeDemo.ZC": "docs/acceptance/demo-dol-doc-qemu-2026-11-03.json",
    "/Demo/Disk/FPrintF.ZC": "docs/acceptance/demo-dol-doc-qemu-2026-11-03.json",
    "/Demo/DolDoc/FileRead.ZC": "docs/acceptance/demo-file-read-qemu-2026-11-03.json",
    "/Demo/DolDoc/Data.ZC": "docs/acceptance/demo-data-qemu-2026-11-03.json",
    "/Demo/DolDoc/Form.ZC": "docs/acceptance/demo-form-qemu-2026-11-03.json",
    "/Demo/PullDownMenu.ZC": "docs/acceptance/demo-pulldown-menu-qemu-2026-11-03.json",
    "/Demo/Graphics/Lattice.ZC": "docs/acceptance/demo-lattice-qemu-2026-11-03.json",
    "/Demo/Graphics/ScreenCapture.ZC": "docs/acceptance/demo-screencapture-qemu-2026-11-02.json",
    "/Demo/Graphics/Grid.ZC": "docs/acceptance/demo-input-graphics-2026-11-01.json",
    "/Demo/Graphics/Palette.ZC": "docs/acceptance/demo-input-graphics-2026-11-01.json",
    "/Demo/Graphics/NetOfDots.ZC": "docs/acceptance/demo-stock-graphics-2026-11-01.json",
    "/Demo/Graphics/Pick3D.ZC": "docs/acceptance/demo-interactive-games-2026-11-01.json",
    "/Demo/Graphics/Doodle.ZC": "docs/acceptance/demo-stock-interactive-2026-11-01.json",
    "/Demo/Graphics/Life.ZC": "docs/acceptance/demo-stock-interactive-2026-11-01.json",
    "/Demo/Graphics/MathAudioDemo.ZC": "docs/acceptance/demo-stock-interactive-2026-11-01.json",
    "/Demo/Graphics/PredatorPrey.ZC": "docs/acceptance/demo-stock-interactive-2026-11-01.json",
    "/Demo/CompileDemo.ZC": "docs/acceptance/demo-compiledemo-qemu-2026-11-02.json",
    "/Demo/WordSearch.ZC": "docs/acceptance/demo-word-search-qemu-2026-11-02.json",
    "/Demo/SuggestSpelling.ZC": "docs/acceptance/demo-suggest-spelling-qemu-2026-11-02.json",
    "/Demo/Lectures/InterruptDemo.ZC": "docs/acceptance/demo-interrupt-demo-svc-qemu-2026-11-02.json",
    "/Demo/Lectures/Ring3.ZC": "docs/acceptance/demo-ring3-svc-qemu-2026-11-02.json",
    "/Demo/Lectures/PCIInterrupts.ZC": "docs/acceptance/demo-arm64-irq-adaptations-qemu-2026-11-03.json",
    "/Demo/MultiCore/Interrupts.ZC": "docs/acceptance/demo-arm64-irq-adaptations-qemu-2026-11-03.json",
    "/Demo/ClassMeta.ZC": "docs/acceptance/demo-class-meta-qemu-2026-11-02.json",
    "/Demo/LastClass.ZC": "docs/acceptance/demo-last-class-hash-fix-qemu-2026-11-08.json",
    "/Demo/DolDoc/UnusedDefine.ZC": "docs/acceptance/demo-unused-define-qemu-2026-11-09.json",
    "/Demo/Lectures/MiniCompiler.ZC": "docs/acceptance/demo-mini-compiler-interactive-qemu-2026-11-02.json",
    "/Demo/Progress.ZC": "docs/acceptance/demo-progress-qemu-2026-11-03.json",
    "/Demo/Games/BattleLines.ZC": "docs/acceptance/demo-battle-lines-qemu-2026-11-03.json",
    "/Demo/Games/Rocket.ZC": "docs/acceptance/demo-rocket-qemu-2026-11-03.json",
    "/Demo/Games/RocketScience.ZC": "docs/acceptance/demo-rocket-science-qemu-2026-11-03.json",
    "/Demo/Games/Maze.ZC": "docs/acceptance/demo-maze-qemu-2026-11-04.json",
    "/Demo/ParenWarn.ZC": "docs/acceptance/demo-paren-warn-qemu-2026-11-03.json",
    "/Demo/Games/BomberGolf.ZC": "docs/acceptance/demo-bomber-golf-qemu-2026-11-04.json",
    "/Demo/Games/CastleFrankenstein.ZC": "docs/acceptance/demo-castle-frankenstein-qemu-2026-11-04.json",
    "/Demo/Games/Collision.ZC": "docs/acceptance/demo-collision-qemu-2026-11-04.json",
    "/Demo/Games/DunGen.ZC": "docs/acceptance/demo-dungen-qemu-2026-11-04.json",
    "/Demo/Games/FlatTops.ZC": "docs/acceptance/demo-flattops-qemu-2026-11-04.json",
    "/Demo/Games/RawHide.ZC": "docs/acceptance/demo-rawhide-qemu-2026-11-04.json",
    "/Demo/Games/Squirt.ZC": "docs/acceptance/demo-squirt-qemu-2026-11-04.json",
    "/Demo/Games/Stadium/Stadium.ZC": "docs/acceptance/demo-stadium-qemu-2026-11-04.json",
    "/Demo/Games/Talons.ZC": "docs/acceptance/demo-talons-qemu-2026-11-04.json",
    "/Demo/Games/TheDead.ZC": "docs/acceptance/demo-the-dead-qemu-2026-11-04.json",
    "/Demo/Games/TreeCheckers.ZC": "docs/acceptance/demo-tree-checkers-qemu-2026-11-04.json",
    "/Demo/Games/Varoom.ZC": "docs/acceptance/demo-varoom-qemu-2026-11-04.json",
    "/Demo/Games/Whap.ZC": "docs/acceptance/demo-whap-qemu-2026-11-04.json",
    "/Demo/Games/Zing.ZC": "docs/acceptance/demo-zing-qemu-2026-11-04.json",
    "/Demo/Sound/MorseCode.ZC": "docs/acceptance/demo-morse-code-virtio-sound-qemu-2026-11-04.json",
    "/Demo/Sound/OhGreat.ZC": "docs/acceptance/demo-ohgreat-virtio-sound-qemu-2026-11-04.json",
    "/Demo/Sound/WaterFowl.ZC": "docs/acceptance/demo-waterfowl-virtio-sound-qemu-2026-11-04.json",
    "/Demo/Sound/ASCIIOrgan.ZC": "docs/acceptance/demo-ascii-organ-virtio-sound-qemu-2026-11-07.json",
    "/Demo/AcctExample/HomeWrappers.ZC": "docs/acceptance/demo-home-wrappers-dol-doc-links-qemu-2026-11-07.json",
    "/Demo/Graphics/WallPapers/WallPaperDesktop/HomeSys.ZC": "docs/acceptance/demo-wallpaper-desktop-qemu-2026-11-08.json",
    "/Demo/MultiCore/MPAdd.ZC": "docs/acceptance/demo-mpadd-qemu-2026-11-07.json",
    "/Demo/MultiCore/Palindrome.ZC":
        "docs/acceptance/demo-palindrome-qemu-2026-11-08.json",
    "/Demo/MultiCore/Primes.ZC":
        "docs/acceptance/demo-primes-single-cpu-qemu-2026-11-08.json",
    "/Demo/DebugDemo.ZC":
        "docs/acceptance/demo-debugdemo-qemu-2026-11-09.json",
    # DiskRaw's complete program workflow is read-only: it reports the active
    # drive/device, reads and dumps a selected block, then restores the
    # partition range. Broader block-device registration and write support
    # are separate storage-integration gaps, not missing steps in this demo.
    "/Demo/Disk/DiskRaw.ZC":
        "docs/acceptance/diskraw-readonly-qemu-2026-11-09.json",
}
RUNTIME_PARTIAL = {
    "/Demo/TimeIns.ZC":
        "docs/acceptance/demo-time-ins-stock-qemu-2026-11-09.json",
    "/Demo/AcctExample/TOS/TOSMisc.ZC":
        "docs/acceptance/demo-tosmisc-qemu-2026-11-08.json",
    "/Demo/AcctExample/TOS/TOSDistro.ZC":
        "docs/acceptance/demo-tosdistro-safe-load-qemu-2026-11-09.json",
    "/Demo/MultiCore/MPRadix.ZC":
        "docs/acceptance/demo-mpradix-current-qemu-2026-11-10.json",
    "/Demo/MultiCore/Lock.ZC":
        "docs/acceptance/demo-lock-single-cpu-current-qemu-2026-11-10.json",
}
RUNTIME_FAILED = {}


def first_error(output):
    for line in output.splitlines():
        line = line.strip()
        if line.startswith("ERR"):
            return line
    return None


def blocker_group(error):
    if not error:
        return "missing compiler diagnostic"
    if "Unknown symbol 'asm'" in error:
        return "x86 inline assembly in the MakeHome boot-installer dependency"
    if "Unknown symbol 'interrupt'" in error:
        return "interrupt-handler syntax unsupported"
    if "AArch64 intrinsic 'Carry'" in error:
        return "Carry intrinsic unsupported"
    if "AArch64 external data 'mem_boot_base'" in error:
        return "TOS boot-memory binding unavailable"
    if "AArch64 external data 'dev'" in error:
        return "system device-global binding unavailable"
    if "can't find file 'TOSExt.ZC'" in error:
        return "missing TOS extension dependency"
    if "Unknown symbol 'TOS_CONFIG'" in error:
        return "TOS_CONFIG definition or include ordering"
    if "Unexpected charactor '#'" in error:
        return "compile-time directive parse/context support"
    if "Expected ':' after label candidate" in error:
        if "candidate 'BootRAM'" in error:
            return "BootRAM symbol/context unavailable"
        if "candidate 'BootHDIns'" in error:
            return "legacy TOS boot-installer label/assembly path unavailable on ARM64"
        return "assembly-like label syntax parse"
    return "other compiler or environment blocker"


def main():
    catalog = json.loads(CATALOG.read_text())
    by_path = {source["path"]: source for source in catalog["sources"]}
    records = []
    for path in sorted(SOURCE_ROOT.rglob("*.ZC")):
        guest_path = "/Demo/" + path.relative_to(SOURCE_ROOT).as_posix()
        result = by_path.get(guest_path)
        if result is None:
            raise SystemExit(f"catalog is missing {guest_path}")
        supplemental_compile = None
        evidence_path = SUPPLEMENTAL_COMPILE.get(guest_path)
        if evidence_path:
            evidence = json.loads((ROOT / evidence_path).read_text())
            source_sha256 = hashlib.sha256(path.read_bytes()).hexdigest()
            if evidence.get("source_path") != guest_path or evidence.get("source_sha256") != source_sha256:
                raise SystemExit(f"stale supplemental compile evidence for {guest_path}")
            if evidence.get("result") != "pass":
                raise SystemExit(f"supplemental compile evidence is not a pass for {guest_path}")
            supplemental_compile = evidence_path
        parts = path.relative_to(SOURCE_ROOT).parts
        directory = parts[0] if len(parts) > 1 else "."
        if directory in KINDS:
            kind = KINDS[directory]
            confidence = "provisional; based on directory only"
        else:
            kind = "root-level source; needs classification"
            confidence = "unclassified"
        record = {
            "path": guest_path,
            "directory": directory,
            "preliminary_kind": kind,
            "classification_confidence": confidence,
            "compile": "pass" if result["compiled"] or supplemental_compile else "blocked",
            "first_compile_error": (None if result["compiled"] or supplemental_compile
                                     else first_error(result.get("output", ""))),
            "runtime": ("pass" if guest_path in RUNTIME_ACCEPTED else
                        "partial" if guest_path in RUNTIME_PARTIAL else
                        "fail" if guest_path in RUNTIME_FAILED else
                        "unverified"),
            "runtime_evidence": (RUNTIME_ACCEPTED.get(guest_path) or
                                 RUNTIME_PARTIAL.get(guest_path) or
                                 RUNTIME_FAILED.get(guest_path)),
        }
        if supplemental_compile:
            record["compile_evidence"] = supplemental_compile
        records.append(record)
    blocked = [r for r in records if r["compile"] == "blocked"]
    blocker_groups = {}
    for record in blocked:
        category = blocker_group(record["first_compile_error"])
        blocker_groups.setdefault(category, []).append(record["path"])
    report = {
        "source_commit": catalog["commit"],
        "generated_from": ["build/demo-source-catalog.json",
                           *sorted(set(SUPPLEMENTAL_COMPILE.values())),
                           *sorted(set(RUNTIME_ACCEPTED.values())),
                           *sorted(set(RUNTIME_PARTIAL.values())),
                           *sorted(set(RUNTIME_FAILED.values()))],
        "source_count": len(records),
        "compile_pass": sum(r["compile"] == "pass" for r in records),
        "compile_blocked": sum(r["compile"] == "blocked" for r in records),
        "compile_blocker_groups": [
            {"category": category, "source_count": len(paths), "paths": paths}
            for category, paths in sorted(blocker_groups.items(),
                                          key=lambda item: (-len(item[1]), item[0]))
        ],
        "runtime_accepted": sum(r["runtime"] == "pass" for r in records),
        "runtime_partial": sum(r["runtime"] == "partial" for r in records),
        "runtime_failed": sum(r["runtime"] == "fail" for r in records),
        "directories": {},
        "classification_note": "Directory-based kinds are triage hints, not final runnable-program classifications.",
        "runtime_note": "Only sources with explicit per-program QEMU runtime evidence in checked-in acceptance records are marked pass. Reproduced runtime failures are marked fail with evidence. Architecture adaptations are described in their evidence and do not imply the original x86 behavior is available. QEMU acceptance does not imply live UTM or Pi hardware acceptance.",
        "sources": records,
    }
    for directory in sorted({r["directory"] for r in records}):
        subset = [r for r in records if r["directory"] == directory]
        report["directories"][directory] = {
            "source_count": len(subset),
            "compile_pass": sum(r["compile"] == "pass" for r in subset),
            "compile_blocked": sum(r["compile"] == "blocked" for r in subset),
            "preliminary_kind": KINDS.get(directory,
                "root-level source; needs classification"),
        }
    OUTPUT.write_text(json.dumps(report, indent=2) + "\n")
    print(f"wrote {OUTPUT.relative_to(ROOT)}: {len(records)} sources, "
          f"{report['compile_pass']} compile pass, "
          f"{report['compile_blocked']} blocked, "
          f"{report['runtime_accepted']} runtime accepted, "
          f"{report['runtime_partial']} partial")


if __name__ == "__main__":
    main()
