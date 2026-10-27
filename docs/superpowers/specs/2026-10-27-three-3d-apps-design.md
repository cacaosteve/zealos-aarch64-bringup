# Three 3D Applications on ARM64 UTM

## Intent

Bring the original ZealOS **GrModels**, **KeepAway**, and **Titanium** applications to usable launch-and-interact status in the ARM64 UTM image. The user wants several more regular apps to work together as a batch. The application source and graphics assets stay the upstream versions; the work belongs in the shared ARM64 3D sprite layer.

## Current state

The pinned application sources and resources are already included in the image. The compile-only catalog stops at these first unresolved symbols:

| App | First blocker |
|---|---|
| GrModels | `sprite_elem_base_sizes` while building generated mesh sprites |
| KeepAway | `SpriteInterpolate` for animated player sprites |
| Titanium | `Sprite3Mat4x4B` for transformed 3D sprites |

The ARM64 sprite path already serializes bitmap and mesh records and has a software sprite rasterizer. The batch should extend this path without changing the original app bodies or packed sprite formats.

## Goals

- Launch each app through its ordinary Apps menu entry in one USB-CD UTM candidate.
- Make GrModels generate and display its Man and Ball sprites through the original forms.
- Make KeepAway display and animate its original court, players, and ball, with basic input and a clean exit.
- Make Titanium display its original terrain, units, and animation, with basic input and a clean exit.
- Preserve a responsive desktop and return from each app to a terminal that evaluates `40+2;` as `42`.
- Keep existing Budget, Digital Logic, Doodle, Mass Spring, Life, editor, and install/persistence acceptance intact.

## Non-goals

- Claim support for the full ZealOS Apps or Demo catalog.
- Implement unrelated audio, dictionary, printer, Pi 4B, or hardware graphics features.
- Rewrite, simplify, or replace the three pinned applications or their artwork.
- Treat compilation or a static screenshot as full app acceptance.

## Approaches considered

1. **Shared 3D sprite layer for GrModels, KeepAway, and Titanium — selected.** Restore the small set of sprite size, interpolation, and matrix-transform services these apps share through the existing renderer. This has the best chance of enabling three visible apps with one focused runtime change.
2. **Span and Strut control widgets.** They share a button-state compile blocker, but their broader control behavior may require separate GUI work and would not exercise the 3D renderer.
3. **TimeClock and Vocabulary runtime acceptance.** Both compile, but TimeClock's saved data and Vocabulary's dictionary/audio requirements are separate subsystems, so they do not form one shared implementation batch.

## Design

Audit each missing operation against the pinned upstream implementation and the current ARM64 rasterizer. Add only the source-level adapters or bounded runtime helpers needed by the three applications; preserve upstream sprite record layouts and validate counts, sizes, and transforms before rasterizing. Keep the applications themselves byte-identical to the pinned source. If one app exposes a separate unsupported subsystem after the shared 3D work, report that boundary instead of adding a visual or interaction stub.

Build the work into a distinct ISO path so the prior live-accepted Logic image and the pending Budget menu-fix ISO remain identifiable. Boot the candidate as a USB CD with the installed VirtIO disk attached; app testing does not reinstall or erase the disk.

## Acceptance

- Each app compiles from its ordinary Apps entry and starts in the graphical desktop.
- GrModels completes its Man and Ball model-generation flows and visibly produces the expected sprite output.
- KeepAway and Titanium render their upstream scenes, continue updating over time, respond to their basic documented controls, and exit cleanly.
- No app launch, input, or exit raises an unresolved external call or freezes the desktop.
- After each app exits, `40+2;` returns `42`.
- The existing focused app, desktop/editor, ISO-install, and disk-persistence checks still pass.
- Live UTM review is tracked separately from automated QEMU acceptance; the user confirms the three visible app paths on the produced candidate ISO.

## Constraints and open implementation details

- Keep original packed sprite data and application source unchanged.
- Implement only the APIs required by this batch; do not imply broad 3D support beyond tested sprite types.
- Use a new candidate ISO filename and record its SHA-256 and test results.
- Determine the exact user input sequence for each app from its original menu/help text while implementing the checks.
- Treat persistence of high scores or other app-specific data as outside this batch unless an app cannot reach its basic usable workflow without it.
