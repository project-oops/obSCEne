# D278 - obscene-injector: decoupled freestanding ELF injection via session kernel R/W


Status: decided.

To measure native Prospero behavior (`payload/ps5-native`) rather than the PS4 backward-compatibility
sandbox (`payload/ps4-bc`, D276), `obSCEne` must run inside a native-category process. `obscene-injector`
implements this as a standalone payload ELF (`obscene-injector.elf`) consuming the session kernel R/W
established by kstuff-lite.

The probe and injector remain strictly decoupled:
- The probe never links the injector; the injector never links the probe checks.
- A shared freestanding layer (`src/common/freestd.c`, `src/common/krw.h`) factors string/mem helpers out of `runtime.c`.
- `src/injector/krw.c` consumes `payload_args_t` (IPv6 socket pair + pipe buffer) pinned against `ps5-payload-dev-sdk`.
- `src/injector/procctl.c` provides ptrace-style process control (attach, get/setregs, step, remote syscall/mmap).
- `src/injector/loader.c` loads `obscene.elf` into the target process address space, maps PT_LOAD segments, applies relocations, and sets memory protections.
- `src/injector/target.c` resolves the target process (foreground app or specified title).
- Linked as plain ET_DYN via `link/injector.ld`.

