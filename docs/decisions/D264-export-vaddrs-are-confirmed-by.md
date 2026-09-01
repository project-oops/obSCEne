# D264 - Export vaddrs are confirmed by behaviour, not read from firmware


**hardware** - 2026-08-30

The sibling emulator needs libkernel's export vaddr table to lay the module out and run the
open-toolchain payloads. The table *could* be read out of `libkernel_sys.sprx` - pull the file,
parse its export table - which is where the two vaddrs this program already relies on came from
(D209). But scanning numbers out of a decrypted firmware image is a different act from measuring
what the console does, and it is not reproducible the way an `OBS|` record is: a hardware run
does not regenerate it, it depends on possessing that extracted file.

### Discovery is gated; confirmation is not

Enumerating the export table at runtime is blocked - the sandbox will not let libkernel be read
(D208). But **confirming** one candidate address needs no table: call `base + vaddr` and check the
function did what that function does. So a vaddr's *source* - firmware scan, published header,
guess - carries no weight, because it is never what is reported. What is reported is that the
address **behaved as the named function**, which anyone with the payload and a console reproduces.

A candidate is a hypothesis with no provenance; the behaviour is the fact. `139-exports` turns
the former into the latter, and the firmware file drops out of the chain - it was only ever a way
to know where to point the probe.

### The section

For each candidate `(name, vaddr, check)` it calls `obs_libkernel_base() + vaddr` and runs a
behavioural check: getpid returns the same non-negative value twice; sceKernelWrite accepts bytes
on the loader's open descriptor and answers the count. A confirmed candidate is reported with its
vaddr as `139-exports/confirm`; the set of confirmed ones is the reproducible table.

Two candidates today, because two is what this program has both a vaddr and a signature for - and
each is one the output path already leans on, so confirming them turns the bootstrap's own
assumptions into measurements. The table grows as candidates are found by any means; on-console
resolution, once a payload can resolve, would supply the vaddr and the confirmation at once.

### Safety, against D058 and against 136-kernel's rule

`136-kernel` reads the handoff and invokes nothing, because a kernel primitive that is not really
there can take a machine down. This section calls **libkernel exports** - getpid, a write - which
`boot.c` has called on hardware since the first run. It issues no kernel primitive and touches
nothing in the high half. An ordinary userland call is safe; a kernel-memory primitive is not,
and none is made here. The base is zero on any build not loaded by elfldr, so the host and an
eboot skip rather than call a computed address.

