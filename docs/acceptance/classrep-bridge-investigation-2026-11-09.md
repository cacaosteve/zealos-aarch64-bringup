# ClassRep field bridge investigation

Status: partially addressed. The initial experiment was removed after it
failed the complete LastClass regression; a later candidate passed the
focused checks described below. Full reflection parity remains unverified.

The earlier portable `ClassRep2` read the pinned ZealOS compiler's
`CHashClass` and `CMemberList` layouts, while the ARM64 compiler stores
different structures. The database demo consequently required a scoped
field-display overlay until the follow-up described below.

An experimental host metadata bridge supplied field name, type name, offset,
array count, primitive type and pointer depth to the pinned report renderer.
In PCI QEMU, an isolated fixture rendered a nested record's numeric value and
an adjacent string. Two significant problems remain:

* The generated DolDoc did not preserve the parent nested-field tree label;
  its saved text contained `$ER$` where that tree item was expected. The
  cause has not been isolated to the tree parser or the formatting path.
* The first full LastClass run faulted while reading compiler-only
  function-pointer metadata. Validating the metadata pointer prevented that
  fault on a follow-up, but after the large system-record reports the
  graphical terminal did not accept the recovery command. The run reported
  `failed=0`; this does not establish why the terminal stopped accepting it.

The field loop also skipped its first field when the index increment appeared
inside the bridge call argument. A later isolated test reproduced the wrong
third-argument value and identified an ARM64 argument-register scratch clash.
The tested backend correction is recorded in
`compiler-postfix-argument-qemu-2026-11-09.json`; this does not by itself fix
the tree label or terminal recovery failures above.

Local investigation artifacts are in `build/classrep-fields-meta.log`,
`build/classrep-fields-final.log`, `build/classrep-fields-safe.log`, and
`build/classrep-fields-verified.log`. The non-shipping experiment is captured
in `build/classrep-experiment-unverified.patch`. These build artifacts are
local and are not portable acceptance records.

After removing the experiment and rebuilding, `--pci --quick
--probe-demo-last-class` passed again, including terminal recovery and
`zstatus failed=0`. The rebuilt kernel SHA-256 was
`28a7a62f75771fd4c394cb4fc4f4578aa2802a5b3fd729edce996c104d802b83`;
the local `build/classrep-rollback-lastclass.log` SHA-256 was
`fe3255b6e091ea6e415ab85443268fa0968889c298eb7004d6a6b6c53fb192b7`.

Full acceptance requires coverage of tree labels, static and dynamic values,
inheritance, arrays, pointer and function-pointer metadata, invalid pointers,
large system classes, and working terminal recovery, along with testing the
original database demo without its overlay.

## Follow-up

Loading the portable debugger again after the full DolDoc writer corrected the
missing nested tree label in the focused fixture. The updated LastClass probe
checked the generated document rather than screen text that had scrolled away,
waited for the second key prompt, and passed through CCPU and CTask reports to
a working terminal. The byte-for-byte original database source then passed its
form, stock ClassRep output and cold QEMU disk-reopen checks; Budget's form and
cold-reopen checks also passed on that candidate.

The bridge flattens inherited compiler fields and falls back to an opaque
representation when a compiler-only pointer type cannot be safely resolved.
Those details, interactive dynamic editing, exact inheritance presentation,
and real UTM/Pi behavior still require separate verification. Earlier failed
candidate observations remain above as historical diagnostics.

The final rebuilt kernel (`77893e1ff4740bebaaf0bea09b8ec85d2b73158852337cc1c080bc4c50d4e4fb`)
passed the focused nested-record, tree-marker, postfix and full LastClass
QEMU checks, including terminal recovery. Its local log is
`build/classrep-final-qemu.log` (SHA-256
`3a3332c9faa5657831fd57ff3ca5585b4d0ebd4863e1dbc4c39e2c1f8b635b4d`).
The Budget and stock database cold-boot checks used the immediately preceding
candidate kernel; see their individual logs for that scope.

An inherited-record follow-up found that `CheckPtr` rejected a valid pointer
to the root shell's boot stack. The runtime now checks otherwise unknown
addresses with AArch64 `AT S1E1R` and checks the translation result in
`PAR_EL1`, preserving DAIF around the query. On kernel
`fcca41f9d3c66a15ab3afdf643be83cd75d7b04ed6764ddb6db9930b38e22ff5`,
PCI QEMU verified inherited and derived values, a string pointer, a nested
root-stack pointer, null and one unmapped address, and restoration of the IRQ
mask. LastClass and terminal recovery passed in the same run
(`build/classrep-flags-qemu.log`, SHA-256
`8108520ae356b3321265fc34d96d47006fc0157e0c94209dc29d545ea485a3d5`).
The separate normal-boot task-settings check passed
(`build/classrep-checkptr-tasksettings.log`, SHA-256
`f483bbe8446728f3516464114b24dff84a0f3eccc17eb5981db07e8fa39b63da`).

The broader `--probe-task-jobs` run did not reach its intended task checks:
after `zreset`, loading TaskBridge failed to resolve `CTextGlobals`. The cause
and whether it predates this change are not established. Its local log is
`build/classrep-checkptr-task-qemu.log` (SHA-256
`6dc3b7c75e57f7e4c1a664fddca49c40739e636f9b25bcf837da2fea8a2e2b01`).
The translation query verifies readable mapping, not allocation lifetime or
the safety of dereferencing a device mapping. Physical Pi behavior remains
unverified.
