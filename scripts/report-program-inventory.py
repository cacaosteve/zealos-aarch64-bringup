#!/usr/bin/env python3
"""Combine pinned ZealOS .ZC sources with the current compile/runtime evidence.

This is an inventory, not a claim that each .ZC file is an independently
launchable program. Demo folders contain both programs and support modules;
App folders contain one launch entry point plus implementation modules.
"""
import json
import pathlib
import re


ROOT = pathlib.Path(__file__).resolve().parents[1]
PINNED = ROOT / "upstream/pinned"
DEMO_LEDGER = ROOT / "docs/acceptance/demo-source-ledger-2026-11-01.json"
PROGRAM_CATALOG = ROOT / "docs/acceptance/program-catalog-2026-11-01.json"
NAMED_APPS = ROOT / "docs/acceptance/named-apps-qemu-2026-11-01.json"
THREE_D = ROOT / "docs/acceptance/three-d-apps-qemu-2026-11-01.json"
BUDGET = ROOT / "docs/acceptance/budget-qemu-2026-11-01.json"
LOGIC = ROOT / "docs/acceptance/utm-logic-prompt-fix-2026-10-26.json"
OUTPUT = ROOT / "docs/acceptance/program-source-inventory-2026-11-01.json"


def read(path):
    return json.loads(path.read_text())


def included_app_sources(entrypoint):
    """Return app-local sources transitively included by a compiled entrypoint.

    The compile catalog compiles each app entrypoint with CCF_JUST_LOAD. Since
    ZealOS #include compiles the included source into that unit, this is valid
    compile evidence for those implementation components. It is not runtime
    evidence and does not cover unreferenced examples or other data files.
    """
    app_root = PINNED / "Apps"
    found = set()
    pending = [entrypoint]
    while pending:
        source = pending.pop()
        if source in found or not source.is_file():
            continue
        found.add(source)
        for include in re.findall(
                r'(?m)^\s*#include\s+["<]([^">]+)[">]',
                source.read_text(errors="replace")):
            include_path = pathlib.Path(include)
            candidates = ([PINNED / include_path.as_posix().lstrip("/")]
                          if include_path.is_absolute() else
                          [source.parent / include_path])
            if not include_path.suffix:
                candidates = [candidate.with_suffix(ext)
                              for candidate in candidates
                              for ext in (".ZC", ".HH")]
            resolved = next((candidate.resolve() for candidate in candidates
                             if candidate.is_file()), None)
            if resolved and resolved.is_relative_to(app_root.resolve()):
                pending.append(resolved)
    return found


def main():
    demo_ledger = read(DEMO_LEDGER)
    demo_by_path = {item["path"]: item for item in demo_ledger["sources"]}
    app_catalog = read(PROGRAM_CATALOG)
    app_compile = {path: True for path in app_catalog["apps_compiled"]}
    app_component_compile = set(app_catalog.get("app_components_compiled", []))
    named_apps = read(NAMED_APPS)
    app_runtime = {
        "/Apps/" + name + "/" + ("Run.ZC" if (PINNED / "Apps" / name / "Run.ZC").exists()
                                  else "Load.ZC"): {
            "status": "pass",
            "evidence": str(NAMED_APPS.relative_to(ROOT)),
        }
        for name, result in named_apps["checks"].items()
        if result["result"] == "pass"
    }
    three_d = read(THREE_D)
    for name in ("GrModels", "KeepAway", "Titanium"):
        app_runtime["/Apps/" + name + "/Run.ZC"] = {
            "status": "pass",
            "evidence": str(THREE_D.relative_to(ROOT)),
        }
    app_runtime["/Apps/Budget/Run.ZC"] = {
        "status": "pass",
        "evidence": str(BUDGET.relative_to(ROOT)),
    }
    # Digital Logic's live UTM acceptance is also recorded with its exact-image
    # checks; its QEMU/UTM input and rendering probes are described there.
    app_runtime["/Apps/Logic/Run.ZC"] = {
        "status": "pass",
        "evidence": str(LOGIC.relative_to(ROOT)),
    }

    app_dirs = sorted(p for p in (PINNED / "Apps").iterdir() if p.is_dir())
    app_entrypoints = {}
    for app_dir in app_dirs:
        candidate = app_dir / "Run.ZC"
        if not candidate.is_file():
            candidate = app_dir / "Load.ZC"
        app_entrypoints["/" + candidate.relative_to(PINNED).as_posix()] = True

    compiled_app_sources = set()
    for entrypoint, passed in app_compile.items():
        if not passed:
            continue
        source = PINNED / entrypoint.lstrip("/")
        compiled_app_sources.update(included_app_sources(source))

    records = []
    for source in sorted(PINNED.rglob("*.ZC")):
        rel = source.relative_to(PINNED).as_posix()
        guest_path = "/" + rel
        parts = source.relative_to(PINNED).parts
        top = parts[0] if len(parts) > 1 else "Root"
        record = {
            "path": guest_path,
            "area": top,
            "kind": "support_or_component",
            "compile": "unverified",
            "runtime": "unverified",
            "evidence": None,
        }
        if top == "Apps":
            record["kind"] = ("app_entrypoint" if guest_path in app_entrypoints
                              else "app_component")
            if guest_path in app_compile:
                record["compile"] = ("pass" if app_compile[guest_path]
                                      else "blocked")
                record["evidence"] = str(PROGRAM_CATALOG.relative_to(ROOT))
            elif guest_path in app_component_compile:
                record["compile"] = "pass"
                record["evidence"] = str(PROGRAM_CATALOG.relative_to(ROOT))
                record["evidence_scope"] = "isolated compile-only component probe"
            elif source.resolve() in compiled_app_sources:
                record["compile"] = "pass"
                record["evidence"] = str(PROGRAM_CATALOG.relative_to(ROOT))
                record["evidence_scope"] = "transitively included by a compiled App entrypoint; compile only"
            if guest_path in app_runtime:
                record["runtime"] = app_runtime[guest_path]["status"]
                record["runtime_evidence"] = app_runtime[guest_path]["evidence"]
        elif top == "Demo":
            record["kind"] = "demo_source_or_component"
            demo_record = demo_by_path.get(guest_path)
            if demo_record:
                record["compile"] = demo_record["compile"]
                record["runtime"] = demo_record["runtime"]
                record["evidence"] = demo_record["runtime_evidence"] or str(
                    DEMO_LEDGER.relative_to(ROOT))
        records.append(record)

    areas = {}
    for area in sorted({record["area"] for record in records}):
        subset = [record for record in records if record["area"] == area]
        areas[area] = {
            "source_files": len(subset),
            "compile_pass": sum(r["compile"] == "pass" for r in subset),
            "compile_blocked": sum(r["compile"] == "blocked" for r in subset),
            "compile_unverified": sum(r["compile"] == "unverified" for r in subset),
            "runtime_pass": sum(r["runtime"] == "pass" for r in subset),
            "runtime_partial": sum(r["runtime"] == "partial" for r in subset),
        }
    report = {
        "date": "2026-11-01",
        "upstream_commit": read(PINNED / "UPSTREAM.json")["commit"],
        "scope": "All pinned upstream .ZC files; includes runnable entries and non-runnable support modules.",
        "classification_note": "A .ZC source is not necessarily a standalone program. Apps are grouped by Run.ZC/Load.ZC entry points; Demo files remain source-or-component until individually classified.",
        "evidence_note": "App implementation sources transitively included by a compiled entrypoint inherit compile-pass evidence only; this does not imply standalone launchability or runtime acceptance. Unreferenced App components and non-Demo/non-App areas remain unverified.",
        "source_files": len(records),
        "app_directories": len(app_dirs),
        "app_entrypoints": len(app_entrypoints),
        "demo_sources": sum(r["area"] == "Demo" for r in records),
        "demo_compile_pass": demo_ledger["compile_pass"],
        "demo_compile_blocked": demo_ledger["compile_blocked"],
        "demo_runtime_accepted": demo_ledger["runtime_accepted"],
        "areas": areas,
        "sources": records,
    }
    OUTPUT.write_text(json.dumps(report, indent=2) + "\n")
    print(f"wrote {OUTPUT.relative_to(ROOT)}: {len(records)} pinned .ZC files; "
          f"{len(app_dirs)} Apps, {len(app_entrypoints)} App entry points, "
          f"{demo_ledger['runtime_accepted']} explicitly accepted Demo sources")


if __name__ == "__main__":
    main()
