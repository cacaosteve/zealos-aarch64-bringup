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
DEMO_LEDGER = ROOT / "docs/acceptance/demo-source-ledger-2026-11-10.json"
PROGRAM_CATALOG = ROOT / "docs/acceptance/program-catalog-qemu-2026-11-03.json"
SOURCE_CONTEXT_CATALOG = ROOT / "docs/acceptance/source-context-catalog-qemu-2026-11-02.json"
NAMED_APPS = ROOT / "docs/acceptance/named-apps-qemu-2026-11-02.json"
THREE_D = ROOT / "docs/acceptance/three-d-apps-utm-qemu-current-2026-11-10.json"
SPAN_STRUT_CURRENT = ROOT / "docs/acceptance/span-strut-current-qemu-2026-11-10.json"
TOTHEFRONT_CURRENT = ROOT / "docs/acceptance/tothefront-current-qemu-2026-11-10.json"
BUDGET = ROOT / "docs/acceptance/budget-current-qemu-2026-11-10.json"
LOGIC = ROOT / "docs/acceptance/digital-logic-current-qemu-2026-11-10.json"
VOCABULARY = ROOT / "docs/acceptance/vocabulary-current-qemu-sound-2026-11-10.json"
PSALMODY = ROOT / "docs/acceptance/psalmody-current-qemu-2026-11-10.json"
TIMECLOCK = ROOT / "docs/acceptance/timeclock-current-qemu-2026-11-10.json"
XCALIBER = ROOT / "docs/acceptance/scheduler-xcaliber-current-2026-11-10.json"
PALETTE_EDITOR = ROOT / "docs/acceptance/demo-wallpaper-desktop-current-qemu-2026-11-10.json"
NETWORK = ROOT / "docs/acceptance/network-programs-qemu-2026-11-03.json"
NETWORK_COMPILE = ROOT / "docs/acceptance/network-compile-qemu-2026-11-06.json"
SYSTEM_STANDALONE_COMPILE = ROOT / "docs/acceptance/system-standalone-compile-qemu-2026-11-06.json"
ROOT_BOOTSTRAP_COMPILE = ROOT / "docs/acceptance/root-bootstrap-compile-qemu-2026-11-06.json"
MISC_STANDALONE_COMPILE = ROOT / "docs/acceptance/misc-standalone-compile-qemu-2026-11-06.json"
MAKEHOME = ROOT / "docs/acceptance/demo-makehome-qemu-2026-11-05.json"
MATH = ROOT / "docs/acceptance/math-conversion-f32-qemu-2026-11-05.json"
MATH_VECTORS = ROOT / "docs/acceptance/math-vectors-matrix-qemu-2026-11-05.json"
MATH_NDARRAY = ROOT / "docs/acceptance/math-ndarray-qemu-2026-11-05.json"
MATH_TESTS = ROOT / "docs/acceptance/math-test-compile-qemu-2026-11-05.json"
TARGETED_SYSTEM = ROOT / "docs/acceptance/targeted-system-compile-qemu-2026-11-05.json"
OUTPUT = ROOT / "docs/acceptance/program-source-inventory-2026-11-10.json"


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
    catalog_compile = set(app_catalog.get("program_sources_compiled", []))
    catalog_blocked = set(app_catalog.get("program_sources_blocked", []))
    source_context_compile = set()
    source_context_blocked = set()
    if SOURCE_CONTEXT_CATALOG.exists():
        source_context = read(SOURCE_CONTEXT_CATALOG)
        source_context_compile.update(
            source["path"] for source in source_context["sources"]
            if source["compile"] == "pass"
        )
        source_context_blocked.update(
            source["path"] for source in source_context["sources"]
            if source["compile"] == "blocked"
        )
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
    # Prefer the current exact-image QEMU run for the inventory. Live UTM
    # acceptance is described separately in PROGRAMS.md.
    app_runtime["/Apps/Logic/Run.ZC"] = {
        "status": "pass",
        "evidence": str(LOGIC.relative_to(ROOT)),
    }
    app_runtime["/Apps/Vocabulary/Run.ZC"] = {
        "status": "pass",
        "evidence": str(VOCABULARY.relative_to(ROOT)),
    }
    app_runtime["/Apps/Psalmody/Run.ZC"] = {
        "status": "pass",
        "evidence": str(PSALMODY.relative_to(ROOT)),
    }
    app_runtime["/Apps/TimeClock/Load.ZC"] = {
        "status": "pass",
        "evidence": str(TIMECLOCK.relative_to(ROOT)),
    }
    app_runtime["/Apps/X-Caliber/Run.ZC"] = {
        "status": "pass",
        "evidence": str(XCALIBER.relative_to(ROOT)),
    }
    for name in ("Span", "Strut"):
        app_runtime["/Apps/" + name + "/Run.ZC"] = {
            "status": "pass",
            "evidence": str(SPAN_STRUT_CURRENT.relative_to(ROOT)),
        }
    app_runtime["/Apps/ToTheFront/Run.ZC"] = {
        "status": "pass",
        "evidence": str(TOTHEFRONT_CURRENT.relative_to(ROOT)),
    }
    # The stock wallpaper desktop launches this exact Home tool entrypoint.
    # Its acceptance covers drawing/color editing, hex entry, Save As, Open
    # listing, reload, and terminal task recovery.
    home_tool_runtime = {
        "/Home/PaletteEditor/Run.ZC": {
            "status": "pass",
            "evidence": str(PALETTE_EDITOR.relative_to(ROOT)),
        },
    }
    network = read(NETWORK)
    network_compile = set()
    network_compile_blocked = {}
    if NETWORK_COMPILE.exists():
        network_compile_evidence = read(NETWORK_COMPILE)
        network_compile.update(network_compile_evidence.get("compiled_sources", []))
        network_compile_blocked = network_compile_evidence.get("blocked_sources", {})
    system_standalone_compile = set()
    if SYSTEM_STANDALONE_COMPILE.exists():
        system_standalone_compile.update(
            read(SYSTEM_STANDALONE_COMPILE).get("compiled_sources", []))
    root_bootstrap_compile = set()
    if ROOT_BOOTSTRAP_COMPILE.exists():
        root_bootstrap_compile.update(
            read(ROOT_BOOTSTRAP_COMPILE).get("compiled_sources", []))
    misc_standalone_compile = set()
    if MISC_STANDALONE_COMPILE.exists():
        misc_standalone_compile.update(
            read(MISC_STANDALONE_COMPILE).get("compiled_sources", []))
    network_runtime = {
        path: {"status": "pass", "evidence": str(NETWORK.relative_to(ROOT))}
        for path, result in network.get("runtime_clients", {}).items()
        if result.startswith("pass:")
    }
    makehome = read(MAKEHOME)
    makehome_compile = set(makehome.get("compiled_sources", []))
    math = read(MATH)
    math_compile = set(math.get("compiled_sources", []))
    math_runtime = {"/System/Math/Conversion.ZC", "/System/Math/F32.ZC"}
    if MATH_VECTORS.exists():
        math_vectors = read(MATH_VECTORS)
        math_compile.update(math_vectors.get("compiled_sources", []))
        math_runtime.update(math_vectors.get("runtime_sources", []))
    if MATH_NDARRAY.exists():
        math_ndarray = read(MATH_NDARRAY)
        math_compile.update(math_ndarray.get("compiled_sources", []))
        math_runtime.update(math_ndarray.get("runtime_sources", []))
    if MATH_TESTS.exists():
        math_tests = read(MATH_TESTS)
        math_compile.update(math_tests.get("compiled_sources", []))
    if TARGETED_SYSTEM.exists():
        targeted_system = read(TARGETED_SYSTEM)
        math_compile.update(targeted_system.get("compiled_sources", []))
        targeted_system_blocked = set(targeted_system.get("blocked_sources", []))
    else:
        targeted_system_blocked = set()

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

    # Palette Editor is a Home tool, not an /Apps entrypoint. Its successful
    # Run.ZC probe loads Load.ZC normally, which includes these source files.
    compiled_home_sources = set()
    if "/Home/PaletteEditor/Run.ZC" in catalog_compile:
        compiled_home_sources.update(
            (PINNED / "Home/PaletteEditor" / name).resolve()
            for name in ("Load.ZC", "PaletteSelect.ZC", "PaletteGod.ZC",
                         "PaletteEdit.ZC")
        )

    records = []
    for source in sorted(PINNED.rglob("*.ZC")):
        rel = source.relative_to(PINNED).as_posix()
        guest_path = "/" + rel
        parts = source.relative_to(PINNED).parts
        top = parts[0] if len(parts) > 1 else "Root"
        record = {
            "path": guest_path,
            "area": top,
            "kind": ("home_tool_entrypoint" if guest_path in home_tool_runtime
                     else "support_or_component"),
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
        elif top == "Home" and source.resolve() in compiled_home_sources:
            record["compile"] = "pass"
            record["evidence"] = str(PROGRAM_CATALOG.relative_to(ROOT))
            record["evidence_scope"] = "transitively included by a compiled Home tool entrypoint; compile only"
        if guest_path in network_runtime:
            record["runtime"] = network_runtime[guest_path]["status"]
            record["runtime_evidence"] = network_runtime[guest_path]["evidence"]
        if guest_path in home_tool_runtime:
            record["runtime"] = home_tool_runtime[guest_path]["status"]
            record["runtime_evidence"] = home_tool_runtime[guest_path]["evidence"]
        if guest_path in network_compile and record["compile"] == "unverified":
            record["compile"] = "pass"
            record["evidence"] = str(NETWORK_COMPILE.relative_to(ROOT))
            record["evidence_scope"] = "compiled in the accepted network stack or Telnet source-load probe; compile only"
        if guest_path in network_compile_blocked and record["compile"] == "unverified":
            record["compile"] = "blocked"
            record["evidence"] = str(NETWORK_COMPILE.relative_to(ROOT))
            record["evidence_scope"] = network_compile_blocked[guest_path]
        if guest_path in system_standalone_compile and record["compile"] == "unverified":
            record["compile"] = "pass"
            record["evidence"] = str(SYSTEM_STANDALONE_COMPILE.relative_to(ROOT))
            record["evidence_scope"] = "compiled standalone in a disposable AArch64 guest task; compile only"
        if guest_path in root_bootstrap_compile and record["compile"] == "unverified":
            record["compile"] = "pass"
            record["evidence"] = str(ROOT_BOOTSTRAP_COMPILE.relative_to(ROOT))
            record["evidence_scope"] = "compiled standalone in a disposable AArch64 guest task; compile only"
        if guest_path in misc_standalone_compile and record["compile"] == "unverified":
            record["compile"] = "pass"
            record["evidence"] = str(MISC_STANDALONE_COMPILE.relative_to(ROOT))
            record["evidence_scope"] = "compiled standalone in a disposable AArch64 guest task; compile only"
        if guest_path in makehome_compile:
            record["compile"] = "pass"
            record["evidence"] = str(MAKEHOME.relative_to(ROOT))
            record["evidence_scope"] = "compiled transitively through the accepted MakeHome source-load path; compile evidence only"
        elif guest_path in math_compile:
            record["compile"] = "pass"
            vector_math_evidence = (MATH_VECTORS.exists() and guest_path in
                                    set(math_vectors.get("compiled_sources", [])))
            ndarray_evidence = (MATH_NDARRAY.exists() and guest_path in
                                set(math_ndarray.get("compiled_sources", [])))
            math_test_evidence = (MATH_TESTS.exists() and guest_path in
                                  set(math_tests.get("compiled_sources", [])))
            targeted_system_evidence = (TARGETED_SYSTEM.exists() and guest_path in
                                        set(targeted_system.get("compiled_sources", [])))
            math_evidence = (TARGETED_SYSTEM if targeted_system_evidence else
                             MATH_TESTS if math_test_evidence else
                             MATH_NDARRAY if ndarray_evidence else
                             MATH_VECTORS if vector_math_evidence else MATH)
            record["evidence"] = str(math_evidence.relative_to(ROOT))
            record["evidence_scope"] = "compiled in the accepted system ExeFile2 source probe"
            if guest_path in math_runtime:
                record["runtime"] = "pass"
                record["runtime_evidence"] = str(math_evidence.relative_to(ROOT))
        if guest_path in targeted_system_blocked:
            record["compile"] = "blocked"
            record["evidence"] = str(TARGETED_SYSTEM.relative_to(ROOT))
            record["evidence_scope"] = "compile probe faulted or reported a source compiler error; source-specific diagnostics are in the acceptance record"
        elif guest_path in source_context_compile and record["compile"] == "unverified":
            record["compile"] = "pass"
            record["evidence"] = str(SOURCE_CONTEXT_CATALOG.relative_to(ROOT))
            record["evidence_scope"] = "live-kernel namespace compile-only probe"
        elif guest_path in source_context_blocked and record["compile"] == "unverified":
            record["compile"] = "blocked"
            record["evidence"] = str(SOURCE_CONTEXT_CATALOG.relative_to(ROOT))
            record["evidence_scope"] = "compile probe failed; see source-context diagnostic"
        elif guest_path in catalog_compile and record["compile"] == "unverified":
            record["compile"] = "pass"
            record["evidence"] = str(PROGRAM_CATALOG.relative_to(ROOT))
            record["evidence_scope"] = "isolated compile-only program catalog probe"
        elif guest_path in catalog_blocked and record["compile"] == "unverified":
            record["compile"] = "blocked"
            record["evidence"] = str(PROGRAM_CATALOG.relative_to(ROOT))
            record["evidence_scope"] = "compile probe failed; see catalog diagnostic"
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
            "runtime_failed": sum(r["runtime"] == "fail" for r in subset),
            "runtime_unverified": sum(r["runtime"] == "unverified" for r in subset),
        }
    report = {
        "date": "2026-11-10",
        "upstream_commit": read(PINNED / "UPSTREAM.json")["commit"],
        "scope": "All pinned upstream .ZC files; includes runnable entries and non-runnable support modules.",
        "classification_note": "A .ZC source is not necessarily a standalone program. Apps are grouped by Run.ZC/Load.ZC entry points; Demo files remain source-or-component until individually classified.",
        "evidence_note": "App implementation sources transitively included by a compiled entrypoint inherit compile-pass evidence only; this does not imply standalone launchability or runtime acceptance. Home tool entrypoints are listed separately and require explicit runtime evidence. Additional standalone app/demo/support source compile results and compile blockers are linked individually to the 2026-11-02 program and source-context catalogs. Other unreferenced sources remain unverified.",
        "source_files": len(records),
        "app_directories": len(app_dirs),
        "app_entrypoints": len(app_entrypoints),
        "home_tool_entrypoints": len(home_tool_runtime),
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
