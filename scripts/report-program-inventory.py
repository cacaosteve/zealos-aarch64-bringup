#!/usr/bin/env python3
"""Combine pinned ZealOS .ZC sources with the current compile/runtime evidence.

This is an inventory, not a claim that each .ZC file is an independently
launchable program. Demo folders contain both programs and support modules;
App folders contain one launch entry point plus implementation modules.
"""
import json
import pathlib


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


def main():
    demo_ledger = read(DEMO_LEDGER)
    demo_by_path = {item["path"]: item for item in demo_ledger["sources"]}
    app_catalog = read(PROGRAM_CATALOG)
    app_compile = {path: True for path in app_catalog["apps_compiled"]}
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
    app_runtime["/Apps/X-Caliber/Run.ZC"] = {
        "status": "partial",
        "evidence": str(NAMED_APPS.relative_to(ROOT)),
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
        "evidence_note": "Only statuses backed by the listed acceptance artifacts are marked pass/partial. Unlisted App components and non-Demo/non-App areas are unverified, not assumed broken or working.",
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
