# D214 - Rendering is blocked by process context, not by the toolchain - the injected process has no display to own


The payload path works: obSCEne's code builds via selfish's primitives, resolves across
libraries, and calls whatever it likes. Non-privileged surfaces run - libc, memory, timing,
the whole report. **Display is the exception, and the wall is where PS5PCEM and a real console
differ most.**

Measured, each call returning cleanly (no crash - the process stays functional):

| step | result on hardware | meaning |
|---|---|---|
| `sceUserServiceGetInitialUser` (no init) | `0x80960002` | user service not initialised |
| `sceUserServiceInitialize(0)` | **never returns** | blocks on IPC to a daemon a background process cannot reach |
| `sceVideoOutOpen(userId, …)` for userId 0/1/-1/0x10000000 | `0x80290001` INVALID_VALUE | no valid user session |
| `sceVideoOutOpen(0xFF, …)` | `0x80290009` | accepted as a value, refused for another reason (permission/ownership) |

elfldr injects into `NPXS40112`, a **background system process**. It has no logged-in user
session and does not own the scanout - so the user service hangs and the display refuses to
open. PS5PCEM renders because an emulator hands the guest a complete foreground app context
with a user and a display; the real injected context has neither.

### This is not a resolution or a signature problem

Every address was verified correct: `sceVideoOutOpen` at the measured `libSceVideoOut` base plus
its vaddr, called with the right arity, returns a *documented error code* rather than faulting.
The crt0, the multi-library resolution, the signatures - all correct. The barrier is the
platform's, and it is exactly the foreground-context question raised in D210.

### The two ways past it, both substantial

1. **A foreground app** - run obSCEne as an eboot/pkg the system launches, which owns a user
   and the display. selfish's `container::build` (eboot) exists; this is the "proper" path and
   the one that completes the format matrix.
2. **Privilege escalation** - elfldr hands the payload kernel-R/W primitives (`rwpipe`,
   `kpipe_addr`, `kdata_base` in `payload_args`) precisely so a payload can patch its own
   process credentials and gain a session. Firmware-specific (12.40 kernel offsets), and the
   larger lift.

Neither is more probing of videoout - that road ends here, correctly. Recorded so the next
session does not re-run the sequence expecting a different userId to work.

Status: **hardware** - every code above observed on the console, 2026-08-27.

