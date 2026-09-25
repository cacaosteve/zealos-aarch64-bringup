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
| Serial | `utmctl attach ZealosAarch64Hello` (if supported) |

There must be **one** entry. If UTM shows two or Start is greyed out: quit UTM, run `make utm`, reopen. RedSea lives past the ESP; `make utm` preserves that region.

```text
ZealOS aarch64 shell (HolyC-IR exprs)
>
```

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

**Status:** M33–M39 UTM freeze bar **accepted** (including `Notes.ZC` from PCI RedSea + `#include`). Pi remains a separate track ([PI4.md](PI4.md)).

**Re-pass 2026-09-23:** `make utm` refreshed; QEMU `run-pci` scripted checklist green (vblk, rspersist `0x4`, catalog/rsdir, `runzc` UseAdd/`0x2a`, MemSort/`0x10`, Notes on disk, compiler 4/0/8); second boot persist OK. UTM `ZealosAarch64Hello` started with synced disk — interactive FB/`Notes` + keyboard still eyes-on in the UTM app (`utmctl attach` not implemented).


## UTM eyes-on notes (M151)

- `utmctl attach` prints **not implemented**; use the UTM window **ZealosAarch64Hello (Terminal 1)** for serial, or the main FB window for graphics/shell text.
- After `make utm` + Play: expect shell banner and `>` on a **cleared** dark FB (boot smokes no longer leave bars/lines under the prompt).
- M152: startup shows short `type: help | vblk | …` hints (full list via `help`).
- M153: after `disklat` / `nearlatticelite`, FB should clear (no leftover HUD/`Width:nan` soup); `>` on clean surface.
- Freeze checklist still: `vblk`, `rspersist`, `rscatalog`, `runzc` UseAdd/Notes/MemSort (same as QEMU `run-pci`).

## Notes

- Case-insensitive names: `runzc notes.zc` is OK.
- Root dir has **8** slots; catalog is sized to fit with `.` / `Keep` / `Hi.ZC`.
- Catalog seed is **best-effort** (full root from an older UTM image no longer blocks `rsdir` / `runzc`).
- QEMU: `make run-serial` / `make run-pci`.
- Pi: `make pi-sd` → `build/pi-esp/` — do not install Pi-diag ELFs into UTM.
