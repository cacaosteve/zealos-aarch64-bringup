# Aiwnios reuse vs freestanding runtime

Two **independent** decisions:

1. **Compiler reuse** — Can we adapt Aiwnios (`../Aiwnios`) compiler/JIT machinery
   (IC → aarch64 emit, RPN, frames) into this tree?
2. **Runtime posture** — Remain freestanding (Limine → ZealBooter → kernel), or grow
   toward a hosted HCRT/task world like Aiwnios userspace.

Finishing tasks, HCRT, or a full HolyC stdlib is **not** a prerequisite for (1).
Evaluating (1) must not redefine “ZealOS compatible” as “whatever our tiny frontend
accepts.”

## Current stance (bring-up)

| Decision | Choice | Why |
|--|--|--|
| Runtime | **Freestanding** | Goal is native ZealOS on virt (UTM acceptance; QEMU harness). |
| Compiler | **Bounded investigation** | Keep our stack IR → `a64_emit` path; probe Aiwnios for emit/IC pieces that drop into freestanding without pulling SDL/HCRT. |
| Compatibility metric | **Upstream ZealC / OS APIs** | Frontend grows only where upstream code requires it. |

## What Aiwnios could supply

- `c/arm64_asm.c` encodings (already mirrored in `a64_emit.h`)
- IC opcode vocabulary and frame/local lowering ideas
- Possibly a sliced `arm_backend` without libc/SDL — **only after** dependency audit

## What must stay ours

- Limine handoff, memory ownership (USABLE-only kernel place; reclaimable protected)
- GIC/timer, virtio, FB shell
- ZealC semantics validation (U32 layout, call frames) against upstream programs

## Explicit non-goals for this phase

- Linking full Aiwnios `arm_backend.c` (~5k LOC) or HCRT
- Growing demo shell features that do not unblock an upstream program
- Treating subset sizeof/ops as “compatible” without behavioral tests

## Compiler-reuse spike (2026-09-23) — done

### 1. Files for IC→a64 without HCRT/SDL

| Layer | Aiwnios path | Size | Host / HCRT? |
|--|--|--|--|
| Encodings | `c/arm64_asm.c` + `c/aiwn_arm.h` | ~1.5k LOC | Mostly pure; Apple cache flush only |
| IC → a64 | `c/arm_backend.c` | **~5.3k LOC** | Needs `CCmpCtrl` / `CRPN` from parser |
| Parser / IC | `c/parser.c` + `c/lex.c` + `c/optpass.c` + `aiwn_lexparser.h` | **~8k+ LOC** | `malloc`/`stdio`, hash, mem, except |
| Mem / hash | `aiwn_mem.h`, `aiwn_hash.h`, `aiwn_except.h` | small | Host alloc / exceptions |

`arm_backend.c` is **not** separable: it lowers Aiwnios RPN (`CRPN`, `IC_*`) and pulls the whole compiler control block. SDL is not in the backend, but the IC stack is. Importing “just emit” = `arm64_asm` / `aiwn_arm.h` only.

Our side today: `a64_emit.h` (~200 LOC, ~43 helpers) already mirrors the useful encodings; `hc_ir.h` + `hc_front.h` (~3.4k) is a stack IR, not Aiwnios IC.

### 2. ZealC vs Aiwnios HolyC (bring-up deltas)

| Topic | This tree | Aiwnios |
|--|--|--|
| Types | `I64`/`U32`/`U8`/`Bool`/`U0`/`F64` (M42 subset); classes | Full HolyC incl. `F64`, classes, AOT |
| ABI | AAPCS64-ish calls via `HC_CALL*`; fixed local slots | Frame + wiggle room + tmp I/F regs |
| `#include` | RedSea text include (M38+) | Host VFs / temple tree |
| Metric | Upstream `.ZC` smokes + UTM freeze | HCRT task world |

Gaps that still matter for upstream growth are **frontend / runtime APIs** (richer classes, Doc/Gr; F64 subset is M42), not missing Aiwnios IC opcodes.

### 3. Decision

**Stay custom** (freestanding frontend + `hc_ir` → `a64_emit`).

- Do **not** import `arm_backend.c` or the IC/parser stack.
- When a new encoding is needed, **cherry-pick** from `arm64_asm.c` / `aiwn_arm.h` into `a64_emit.h` (same pattern as today).
- Grow the frontend only where an upstream ZealC program / subsystem requires it.

Default remains: **freestanding runtime + local frontend**, measured by UTM boot + upstream program/subsystem milestones.
