#!/usr/bin/env python3
"""Build a source-level status ledger for the pinned ZealOS Demo tree.

The directory-derived kind is intentionally provisional: a directory is not
proof that each file is a standalone program. Runtime acceptance is populated
only from explicit evidence in the checked-in acceptance record.
"""
import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "upstream/pinned/Demo"
CATALOG = ROOT / "build/demo-source-catalog.json"
OUTPUT = ROOT / "docs/acceptance/demo-source-ledger-2026-11-01.json"

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
    "/Demo/DateTime.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/NullCase.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/Print.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/RadixSort.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/SubIntAccess.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/SubSwitch.ZC": "docs/acceptance/demo-runtime-basics-2026-11-01.json",
    "/Demo/Exceptions.ZC": "docs/acceptance/demo-exceptions-2026-11-01.json",
    "/Demo/Games/Chess.ZC": "docs/acceptance/demo-interactive-games-2026-11-01.json",
    "/Demo/Games/CircleTrace.ZC": "docs/acceptance/demo-circletrace-qemu-2026-11-01.json",
    "/Demo/Games/TicTacToe.ZC": "docs/acceptance/demo-interactive-games-2026-11-01.json",
    "/Demo/Games/MassSpring.ZC": "docs/acceptance/demo-stock-interactive-2026-11-01.json",
    "/Demo/Graphics/Lines.ZC": "docs/acceptance/demo-stock-graphics-2026-11-01.json",
    "/Demo/Graphics/3DPoly.ZC": "docs/acceptance/demo-bounce-clock-3dpoly-2026-11-01.json",
    "/Demo/Graphics/Bounce.ZC": "docs/acceptance/demo-bounce-clock-3dpoly-2026-11-01.json",
    "/Demo/Graphics/Clock.ZC": "docs/acceptance/demo-bounce-clock-3dpoly-2026-11-01.json",
    "/Demo/Graphics/RotateTank.ZC": "docs/acceptance/demo-bounce-clock-3dpoly-2026-11-01.json",
    "/Demo/Graphics/32BitColor.ZC": "docs/acceptance/demo-graphics-render-2026-11-01.json",
    "/Demo/Graphics/Blot.ZC": "docs/acceptance/demo-graphics-render-2026-11-01.json",
    "/Demo/Graphics/Box.ZC": "docs/acceptance/demo-box-hanoi-2026-11-01.json",
    "/Demo/Graphics/Hanoi.ZC": "docs/acceptance/demo-box-hanoi-2026-11-01.json",
    "/Demo/Graphics/Mandel.ZC": "docs/acceptance/demo-mandel-qemu-2026-11-01.json",
    "/Demo/Graphics/MouseDemo.ZC": "docs/acceptance/demo-input-graphics-2026-11-01.json",
    "/Demo/Graphics/Grid.ZC": "docs/acceptance/demo-input-graphics-2026-11-01.json",
    "/Demo/Graphics/Palette.ZC": "docs/acceptance/demo-input-graphics-2026-11-01.json",
    "/Demo/Graphics/NetOfDots.ZC": "docs/acceptance/demo-stock-graphics-2026-11-01.json",
    "/Demo/Graphics/Pick3D.ZC": "docs/acceptance/demo-interactive-games-2026-11-01.json",
    "/Demo/Graphics/Doodle.ZC": "docs/acceptance/demo-stock-interactive-2026-11-01.json",
    "/Demo/Graphics/Life.ZC": "docs/acceptance/demo-stock-interactive-2026-11-01.json",
}
RUNTIME_PARTIAL = {
    "/Demo/AcctExample/HomeKeyPlugIns.ZC": "docs/acceptance/demo-global-vars-jit-2026-11-01.json",
    "/Demo/Graphics/Slider.ZC": "docs/acceptance/demo-input-graphics-2026-11-01.json",
}


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
        return "inline assembly unsupported or requires an ARM64 equivalent"
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
        parts = path.relative_to(SOURCE_ROOT).parts
        directory = parts[0] if len(parts) > 1 else "."
        if directory in KINDS:
            kind = KINDS[directory]
            confidence = "provisional; based on directory only"
        else:
            kind = "root-level source; needs classification"
            confidence = "unclassified"
        records.append({
            "path": guest_path,
            "directory": directory,
            "preliminary_kind": kind,
            "classification_confidence": confidence,
            "compile": "pass" if result["compiled"] else "blocked",
            "first_compile_error": first_error(result.get("output", "")),
            "runtime": ("pass" if guest_path in RUNTIME_ACCEPTED else
                        "partial" if guest_path in RUNTIME_PARTIAL else
                        "unverified"),
            "runtime_evidence": (RUNTIME_ACCEPTED.get(guest_path) or
                                 RUNTIME_PARTIAL.get(guest_path)),
        })
    blocked = [r for r in records if r["compile"] == "blocked"]
    blocker_groups = {}
    for record in blocked:
        category = blocker_group(record["first_compile_error"])
        blocker_groups.setdefault(category, []).append(record["path"])
    report = {
        "source_commit": catalog["commit"],
        "generated_from": "build/demo-source-catalog.json",
        "source_count": len(records),
        "compile_pass": sum(r["compile"] == "pass" for r in records),
        "compile_blocked": sum(r["compile"] == "blocked" for r in records),
        "compile_blocker_groups": [
            {"category": category, "source_count": len(paths), "paths": paths}
            for category, paths in sorted(blocker_groups.items(),
                                          key=lambda item: (-len(item[1]), item[0]))
        ],
        "runtime_accepted": sum(r["runtime"] == "pass" for r in records),
        "directories": {},
        "classification_note": "Directory-based kinds are triage hints, not final runnable-program classifications.",
        "runtime_note": "Only sources with explicit per-program QEMU runtime evidence in the checked-in 2026-10-31 and 2026-11-01 acceptance records are marked pass. QEMU acceptance does not imply live UTM or Pi hardware acceptance.",
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
          f"{report['runtime_accepted']} runtime accepted")


if __name__ == "__main__":
    main()
