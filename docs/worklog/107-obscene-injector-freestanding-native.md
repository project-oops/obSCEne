# 2026-08-31 (obscene-injector) - freestanding native process injector (Plan v2, D278)


Landed `obscene-injector`: a standalone payload ELF (`obscene-injector.elf`) that consumes the session kernel R/W established by kstuff-lite, attaches to a target native process (foreground game title or specified process), maps `obscene.elf` segments into the target address space via an in-memory ELF loader, rewires registers, and resumes execution natively (`payload/ps5-native`).

Key components:
- `src/common/freestd.c` & `src/common/freestd.h`: Factored freestanding string (strlen, strcmp, strncmp, strncpy), formatting (u64, i64, hex), and memory (memset, memcpy, memcmp) helpers out of `runtime.c`, shared across probe and injector.
- `src/common/krw.h`: Defined kernel R/W interface contract.
- `src/injector/krw.c`: Implements `krw.h` consuming `payload_args_t` (IPv6 socket pair + pipe buffers) pinned to `ps5-payload-dev-sdk`. Supports firmware versions 1.xx through 13.xx, ucred authid elevation (`0x4800000000010003`) and capability patching for unrestricted ptrace access.
- `src/injector/procctl.c` & `procctl.h`: FreeBSD x86_64 ptrace process control (attach, getregs, setregs, step, continue, remote memory copyin/copyout, remote mmap/mprotect/syscall).
- `src/injector/target.c` & `target.h`: Resolves target process by name, foreground app, or numeric PID via kernel `allproc` traversal.
- `src/injector/loader.c` & `loader.h`: `libelfldr`-shaped in-memory ELF loader (PT_LOAD segment mapping, relative relocations, 16 KiB page alignment, protection bits).
- `src/injector/injector.c`: Main entry point orchestrating R/W initialization, credential elevation, target resolution, ELF mapping, and register hijacking.
- `link/injector.ld`: Dedicated freestanding ET_DYN linker script with 16 KiB page separation.
- `docs/INJECTOR.md`: Complete architectural documentation and operational runbook.
- `Makefile`: Added `INJECTOR_SRC`, `INJECTOR_OBJ`, `injector` build rules, and added `src/common/freestd.c` to `COMMON_SRC`.
- `./bin/obscene` & `scripts/payload-run.sh`: Added `./bin/obscene inject` (alias `injector`) and `--injector` flag to build both payload and injector and run the native injection workflow on console.

Verified: `make host`, `make payload`, `make module`, `make injector`, `./bin/obscene inject --build-only`, and `make check` all green under WSL Ubuntu. `verify.sh` green.

