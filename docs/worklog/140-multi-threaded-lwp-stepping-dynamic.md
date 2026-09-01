# 2026-09-01 (Multi-Threaded LWP Stepping & Dynamic Kernel NID Syscall Resolution) (D308)


Analyzed hardware test log (Run #21):
1. **Diagnosis**:
   - `procctl_step(pid)` passed the process ID `pid` (330) to `ptrace(PT_STEP)`. In FreeBSD multi-threaded processes, `PT_STEP` requires the specific thread LWP ID (Lightweight Process ID) returned by `PT_GETLWPLIST` (`0x0F`), otherwise stepping does not advance the targeted thread and `rax` retains its input value (`sysno = 477 = 0x1dd`).
   - In addition, `procctl_find_syscall_gadget` could fail if text reading was rejected by kernel permissions.
2. **Remediation**:
   - **Kernel SPRX NID Resolver (`src/injector/krw.c`, `krw.h`)**: Implemented `krw_dynlib_resolve()` which parses the target process's kernel SPRX dispatch tables (`target_kproc + 0x3E8`) to resolve NID `"W0xkN0+ZkCE"` directly from `libkernel` exports in kernel memory, computing `s_remote_syscall_gadget = sym + 0x0A`.
   - **LWP Thread Discovery (`src/injector/procctl.c`)**: Implemented `get_target_lwp()` querying `PT_GETNUMLWPS` (`14`) and `PT_GETLWPLIST` (`15`) so `PT_STEP` steps the exact target thread.

Verified: `make payload injector HARDWARE=1` (`9,411,144` bytes) compiles 100% clean with zero warnings and zero errors.

