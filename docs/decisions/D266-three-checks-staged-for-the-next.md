# D266 - Three checks staged for the next hardware run: the layout of a memory type, whether a short size bounds a write, and more export candidates


The console is available again, so three measurements go on the wire now rather than waiting for
the run after. Each is the confirmation half of a question a prior run left as a hypothesis, and
each earns a check only because its verdict is decidable from behaviour alone.

**`139-exports` gains six candidates.** The section confirmed two vaddrs (getpid, sceKernelWrite)
by calling `base + vaddr` and checking the function behaved (D264). The same move settles any
export with a behavioural signature a wrong address is overwhelmingly unlikely to satisfy, so the
table now carries eight: four id calls (getuid/geteuid/getgid/getppid, `STABLE_ID` - the same
non-negative value twice), the counter frequency (`sceKernelGetTscFrequency`, `TSC_FREQ` -
returns exactly `0x5f259b8e`, the number three other measurements already agree on), and a
monotonic clock (`sceKernelGetProcessTime`, `ADVANCES` - the second reading no earlier than the
first). The frequency candidate is the strong one: a function answering that precise value and
taking no argument is that call and almost nothing else, so a match confirms both the vaddr and,
a fourth time, the frequency. A candidate's vaddr source still carries no weight; only the verdict
is reported.

**`130-layout/memory-type` asks what the third field of a direct-memory record is.** The query
record is `(start, end, then a third word)`, and on hardware that word read `3` - the value of
`SCE_KERNEL_WB_GARLIC`. So the check allocates a 16 KiB span of each of the three memory types in
turn, queries it, and reads the third word: if it equals the type asked for across all three, the
field **is** the type; if it is constant, the field is state and the `3` was a coincidence of that
run; if it varies but does not track, it is neither. The verdict distinguishes the three rather
than asserting the appealing one. Provenance `assumed` - it records bytes and names which
hypothesis they support, it does not claim a layout.

**`130-layout/short-buffer-overrun` asks whether a short declared size bounds the write.** The
size ladder proved every size 1-256 is *accepted* (the size is not validated), but acceptance is
not the same as truncation: a call may ignore the size and write the whole record anyway, which
corrupts a small-buffer caller. So the check poisons the buffer past a short declared size (8, 16,
24, 32) but inside it, calls, and sees whether anything past the size changed. It is the
per-declared-size sibling of the ladder's tail guard - that catches a write past the whole 256-byte
buffer, this catches a write past the 8 bytes the caller claimed. GetModuleInfo already carries the
same guard-word test (`110-modules/info-size`); this closes the equivalent gap on the query.
Provenance `derived`, like the ladder: the platform draws the boundary, the check records where it
fell.

`120-measure/clocks-advance` also now emits each clock's **absolute** reading beside its busy
delta - a clock's origin is a fact a later run wants and the delta discarded.

All four are green under `make host` (memory-type reads a constant on the stub, so it correctly
reports "state, not the type"; short-buffer-overrun bounds at 24/32 and refuses 8/16;
139-exports skips with no libkernel base, as designed). `guards`, `caps` and `counts` pass at 171
checks across 36 sections. Status: **done, awaiting the hardware run.**

**Hardware result (2026-08-31).** 139-exports confirmed seven of eight on the payload run
(getpid, sceKernelWrite, getuid, geteuid, getgid, getppid, sceKernelGetProcessTime) and **refuted
sceKernelGetTscFrequency at `0x1cf30`** - the offset is wrong for that function. The other three
skipped: the elfldr payload reaches libkernel by `base + vaddr` (so 139-exports runs) but resolves
only a minimal import set, and memory-type / short-buffer-overrun / clocks-advance call resolved
imports. They need an **eboot** run, or a `base + vaddr` rewrite. See WORKLOG 2026-08-31.

