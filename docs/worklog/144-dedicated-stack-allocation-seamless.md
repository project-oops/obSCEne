# 2026-09-01 (Dedicated Stack Allocation & Seamless Register Restore Trampoline) (D312)


Architecture and Execution Refinement:
1. **Diagnosis**:
   - Rather than executing directly inside the host game thread's stack space (which could have unknown depth, non-standard alignment, or limited size), the injector must ensure a pristine, SysV AMD64 ABI compliant stack frame.
2. **Remediation**:
   - **Dedicated 256KB Remote Stack (`src/injector/injector.c`)**: `procctl_remote_mmap` now allocates `0x40000` bytes (256 KB) in the target address space. Offset 0 holds `payload_args`, while the remaining space serves as a dedicated payload execution stack with guaranteed 16-byte alignment (`RSP % 16 == 8` at entry).
   - **Full Register-Restore Trampoline (`src/injector/injector.c`)**: Staged a machine-code trampoline at `stack_top + 0x80` that restores all 16 AMD64 general-purpose registers (`RAX`, `RBX`, `RCX`, `RDX`, `RSI`, `RDI`, `RBP`, `R8-R15`, `RSP`), pushes `bak_rip`, and executes `ret`. When `obscene_start` returns, it jumps into this trampoline, cleanly handing execution back to the host game thread without leaving any side effects.

Verified: `make payload injector HARDWARE=1` (`9,411,192` bytes) compiles 100% clean with zero warnings and zero errors.

