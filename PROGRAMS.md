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

## Remaining collection

An inventory of the pinned upstream `src/Apps` and `src/Demo` tree contains
294 files, including source, documents and binary resources. That is a file
count, not a count of applications. The ARM64 source bundle now ships that
complete tree, unchanged and hash-checked. Availability does not establish
compilation or runtime support.

The full application directories still need individual acceptance:
Budget, GrModels, KeepAway, Logic, Psalmody, Span, Strut, TimeClock, Titanium,
ToTheFront, Vocabulary and X-Caliber. The wider demo/game collection includes
Chess, Varoom, FlapBat and many others that have not been accepted on ARM64.
Some depend on missing sound, graphics, document, task or hardware behavior;
the assembly demos also require architecture-specific implementations.

`make check-program-catalog` compiles each application entry point in an
isolated guest task with `CCF_JUST_LOAD` and records the first blocker in
`build/program-catalog.json`. This is an inventory, not execution acceptance.
The current blockers include embedded DolDoc sprites in raw source includes,
missing controls and graphics types, date/time globals and dictionary/audio
services. Budget and Vocabulary compile; their runtime behavior is untested.

| Application | First compile blocker in the inventory |
|--|--|
| Budget | Compiles after `no_warn` support; date, forms and save/reopen need runtime acceptance |
| GrModels | `sprite_elem_base_sizes` |
| KeepAway, Logic, Strut, Titanium, X-Caliber | Embedded DolDoc/sprite data in raw `#include` input |
| Psalmody | `GodWordStr` (audio also remains unimplemented) |
| Span | `CCtrlBttnState` |
| TimeClock | Unbound `local_time_offset` |
| ToTheFront | `DCOS` |
| Vocabulary | Compiles only; dictionary and audio need runtime acceptance |

For each program, track original-file integrity separately from compilation,
visible behavior/input, clean exit and any save/reboot/reopen requirements.
Merely copying an upstream file into the image does not make it supported.
Detailed passing runs and limitations belong in [ACCEPTANCE.md](ACCEPTANCE.md).
