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
    "/Demo/Games/TicTacToe.ZC": "docs/acceptance/demo-interactive-games-2026-11-01.json",
    "/Demo/Graphics/Lines.ZC": "docs/acceptance/demo-stock-graphics-2026-11-01.json",
    "/Demo/Graphics/NetOfDots.ZC": "docs/acceptance/demo-stock-graphics-2026-11-01.json",
    "/Demo/Graphics/Pick3D.ZC": "docs/acceptance/demo-interactive-games-2026-11-01.json",
}
RUNTIME_PARTIAL = {
    "/Demo/AcctExample/HomeKeyPlugIns.ZC": "docs/acceptance/demo-global-vars-jit-2026-11-01.json",
}


def first_error(output):
    for line in output.splitlines():
        line = line.strip()
        if line.startswith("ERR"):
            return line
    return None


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
    report = {
        "source_commit": catalog["commit"],
        "generated_from": "build/demo-source-catalog.json",
        "source_count": len(records),
        "compile_pass": sum(r["compile"] == "pass" for r in records),
        "compile_blocked": sum(r["compile"] == "blocked" for r in records),
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
