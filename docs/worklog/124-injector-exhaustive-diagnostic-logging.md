# 2026-09-01 (injector exhaustive diagnostic logging instrumentation) (D292)


Instrumented the entire injection and process control pipeline with exhaustive diagnostic telemetry to eliminate trial-and-error round trips during hardware verification:
1. **Target Discovery Telemetry (`src/injector/target.c`)**:
   - Every evaluated process logs PID, UID, comm name, and classification (`skip system proc` vs `>>> candidate user proc`).
   - Logs `kproc` and `ucred` kernel virtual addresses for all candidate userland processes.
   - Explicitly logs Title ID matching details and candidate selection reasoning.
2. **Credential & Prison Telemetry (`src/injector/krw.c`)**:
   - Logs `my_ucred` and `target_ucred` kernel virtual addresses alongside their respective `cr_prison` jail pointer addresses (`ucred + 0x30`).
   - Logs original and elevated auth IDs, capabilities, attribute flags, and UID triplets (`cr_uid`, `cr_ruid`, `cr_svuid`).
   - Thread scanner logs calling thread `td` pointer, resolved `td_ucred` offset (verified `0x140`), previous credential address, and patched address.
3. **Process Control & Remote Syscall Telemetry (`src/injector/procctl.c`)**:
   - `procctl_attach`: Inspects target `struct proc` in kernel memory prior to attachment, logging target `kproc`, `p_flag` (offset `0xB0`), and `p_state` (offset `0x08`). Logs return code, errno, and `wait4` stop signal details.
   - All ptrace helpers (`step`, `continue`, `getregs`, `setregs`, `copyin`, `copyout`, `detach`) log target addresses and POSIX errno on any failure.
   - `procctl_remote_syscall`: Logs syscall number, single-stepping step count, safety loop guard (2000 steps max), and returned `%rax`.
   - `procctl_remote_mmap`: Logs requested virtual address, length, protection flags, and allocated base.
4. **Remote Loader Telemetry (`src/injector/loader.c`)**:
   - Logs ELF header `e_entry`, `e_phnum`, total virtual size, and allocated `target_base`.
   - Every `PT_LOAD` segment logs its index, destination virtual address, `p_filesz`, `p_memsz`, and protection mask.
   - Logs final calculated entry point address.
5. **Injector Lifecycle Telemetry (`src/injector/injector.c`)**:
   - Logs `payload_args` addresses (`kpipe_addr`, `kdata_base_addr`).
   - Logs pre-hijack thread register state (`RIP`, `RSP`, `RBP`, `RAX`, `RFLAGS`).
   - Logs post-hijack target register state (`RIP`, `RSP`, `RDI`).
   - Reads back and verifies target thread registers via `procctl_getregs()` after `procctl_setregs()`.
   - Logs `procctl_detach()` return code.

Verified: `make payload injector HARDWARE=1` (`9,394,336` bytes) compiles 100% clean with zero warnings and zero errors.

