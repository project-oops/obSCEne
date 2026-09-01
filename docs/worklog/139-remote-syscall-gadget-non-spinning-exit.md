# 2026-09-01 (Remote Syscall Gadget & Non-Spinning Exit Path) (D307)


Analyzed hardware test log (Run #19):
1. **Diagnosis**:
   - `target thread RIP=0x21ce77975`: In Run #18, `obscene_start` actually executed all test suites (`obs_run_all`) and reached the end of `obscene_start` (`0x5f7975`), where it was spinning in the `for (;;) { pause; }` loop.
   - When the injector attached to the target process in Run #19, the thread was stopped inside that loop instead of a `libkernel` syscall trampoline.
   - `procctl_remote_syscall` assumed whatever instruction `RIP` was pointing at would execute a syscall. Stepping `jmp 0x5f7975` simply repeated the jump 2000 times until timing out, leaving `rax=0x1dd` (477), which broke `remote_mmap`.
2. **Remediation**:
   - **Syscall Gadget Discovery (`src/injector/procctl.c`, `procctl.h`)**: Implemented `procctl_find_syscall_gadget()` which checks `libkernel_base + 0x5ba` (`getpid + 0xa`) for `0x0f 0x05` (`syscall`) or scans `libkernel` text for a `syscall` gadget.
   - **Single-Step Remote Syscall Execution (`src/injector/procctl.c`)**: `procctl_remote_syscall()` now sets `jmp_reg.r_rip = s_remote_syscall_gadget` and executes exactly 1 single-step, capturing `rax` and restoring original registers immediately without looping or timeouts.
   - **Validation in `procctl_remote_mmap`**: Rejects invalid addresses (`< 0x10000` or `> 0x00007fffffffffff`).
   - **Thread Exit Path Conformance (`src/probe/start.c`)**: Replaced `for (;;) { pause; }` at the end of `obscene_start` with `return;`, allowing the hijacked game thread to pop its saved return RIP and cleanly resume running retail game frames after the probe completes.

Verified: `make payload injector HARDWARE=1` (`9,411,064` bytes) compiles 100% clean with zero warnings and zero errors.

