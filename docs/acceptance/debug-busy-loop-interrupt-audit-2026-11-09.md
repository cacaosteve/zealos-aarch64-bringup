# Busy-loop interrupt audit

Status: unresolved. This is a source audit, not a working ARM64 interrupt
implementation or proof that DebugDemo can be interrupted.

The pinned ZealOS scheduler is cooperative: `Yield` advances from the
current task to the next ready task. `MessageScan` does not yield.
`Kernel/KInterrupts.ZC`'s `IRQ_TIMER` saves the interrupted task and, for a
non-idle task, jumps to `RESTORE_RSI_TASK` with that same task in RSI. That
restore path in `Kernel/Sched.ZC` first checks `SYS_CTRL_ALT_FLAGS`; the
`CTRL_ALT_C` path selects the focused task and handles its break state.
`Kernel/KeyDev.ZC` registers Ctrl-Alt-C as an IRQ callback that sets the flag.
This timer return path is distinct from advancing the ready queue in `Yield`.

The ARM64 runtime presently polls its keyboard reader from cooperative runtime
paths. The QEMU DebugDemo test demonstrates that a focused task running its
stock busy loop can continue emitting dots after Ctrl-Alt-C without recovering.
The current ARM64 timer IRQ handler in `src/kernel_stub.c` is used by diagnostic
bring-up; it does not implement the ZealOS task/keyboard interrupt path. A
print-boundary poll would not cover a silent busy loop and must not be treated
as a general fix.

A faithful implementation needs interrupt-capable input delivery, a pending
action mechanism, correct focused-task and break-lock handling, and safe
delivery at an interrupted guest context. It must respect guest IRQ masking
and preserve all guest state, including floating-point/SIMD state when the
interrupt path can use it. It must not invoke non-reentrant parser, allocator,
device, or guest scheduling code from an interrupt without a proven safe
protocol. The exact design and support for both QEMU and Pi remain open.

Acceptance must cover a focused silent infinite loop as well as the unchanged
DebugDemo, verify Ctrl-Alt-C and terminal recovery, preserve a nonfocused task,
and demonstrate that timer interrupts alone do not rotate runnable tasks.
Existing `MessageScan` polling and explicit-yield round-robin regressions must
continue to pass. Ctrl-Alt-D remains a separate debugger requirement.
