# 2026-08-31 (later still) - the hardware run: 139-exports lands, and the payload/eboot split becomes concrete


The three checks from the previous entry went to the console. First pkg install of the obSCEne
toolchain ever succeeded (retiring build-pkg's "nothing built by this toolchain has ever been
installed" note) - the console fetched all 6 MB over libhttp. Launch under `OBSC00001` hit the
stuck-title refusal `0x8094000c` (D223: a prior crashed instance held the id, before this session);
rebuilding under a fresh `OBSC00002` sidestepped it, per the documented workaround. The suite then
ran as the elfldr **payload** and streamed 36,353 records to `OBS|end|sceKernelWrite`
(reports/hardware/console-report.txt).

**139-exports is the win.** Seven of eight candidates confirmed by calling `base + vaddr` and
checking behaviour: getpid `0x5b0`, sceKernelWrite `0x16e00`, getuid `0x630`, geteuid `0x650`,
getgid `0x870`, getppid `0x7d0`, sceKernelGetProcessTime `0x16160`. Five of those are newly
confirmed. The eighth, **sceKernelGetTscFrequency at `0x1cf30`, refuted** - called as a no-arg
frequency getter it did not return the measured `0x5f259b8e`, so that offset is wrong for that
function (or it is not a plain no-arg getter). This is the hypothesis-then-confirm methodology
doing exactly its job: the vaddr's source carried no weight, the behaviour settled it.

**The surprise, and it is a structural one: the other three checks skipped** - "the loader did not
resolve this symbol for this build." The payload path reaches libkernel by `base + vaddr`
arithmetic, so 139-exports thrives, but it resolves only a minimal **import** set, and
memory-type / short-buffer-overrun / clocks-advance all call resolved imports
(sceKernelAllocateDirectMemory, sceKernelDirectMemoryQuery, sceKernelGetProcessTime-as-import).
161 checks skipped for this one reason - the whole import-based surface, not just these three. The
two build paths are therefore **complementary**: the payload confirms export vaddrs and skips the
import checks; an eboot goes through the full system loader (resolves the imports) but gets no
`payload_args` base, so it runs the import checks and skips 139-exports. To get real values for the
three, they need an eboot run - or they could be rewritten to reach their functions by
`base + vaddr` too, now that sceKernelGetProcessTime's offset is confirmed and the direct-memory
offsets are in the table. (The 363 "fails" were all `900-surface` census entries for absent
symbols, expected for a payload's narrow surface.)

