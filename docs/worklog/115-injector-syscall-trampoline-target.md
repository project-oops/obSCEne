# 2026-09-01 (injector syscall trampoline & target resolver) - eliminated PPRBUG-22859 direct-syscall traps (D283)


Hardware test analysis:
- The log line `PPRBUG-22859: the process pid=191 directly issued a syscall 649 at 0x200006316` revealed that PS5 userland kernel security terminates any process that executes raw `syscall` instructions directly from payload memory rather than through `libkernel`'s text segment.
- `src/common/syscall.h` & `src/common/syscall.c`: Implemented `sys_call_init` and `sys_call` which resolve `getpid` / `sceKernelDlsym` from `payload_args_t->sys_dynlib_dlsym` and route all syscalls through `libkernel`'s `syscall; ret` trampoline (`+ 0xa`).
- `src/injector/injector.c`, `src/injector/krw.c`, `src/injector/procctl.c`, `src/injector/target.c`: Migrated all direct `__asm__("syscall")` calls to `sys_call(...)`.
- `src/injector/target.c`: Replaced naive daemon candidate list with a comprehensive system daemon filter (`is_system_daemon()`) to prevent accidentally targeting internal OS daemons (like `SceSpZeroConf`) when resolving foreground titles.

Verified: `make payload injector HARDWARE=1` builds cleanly and all gates green (`verify.sh`).

