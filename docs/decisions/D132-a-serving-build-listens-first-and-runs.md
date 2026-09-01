# D132 - A serving build listens first and runs the suite on demand. The report streams over the socket. A named Deck target exists


Status: derived - listen-first proven to reach the socket in ~3s in shadPS4 with no
exclusion list, and the full loop (hello/read/call/bye/reconnect) driven live there.

Three changes, one theme: make the interactive endpoint robust to a target whose stability
is not deterministic. shadPS4 died mid-suite on one launch and completed the next from the
same binary - unprovable as ours or theirs without hardware, and not worth chasing in
someone else's emulator. So the fix is to stop making the socket depend on the flaky part.

**Listen first (#1).** `obscene_start` for a serving build no longer runs the 146-check suite
before opening the socket - it listens immediately and runs the suite only when a driver
sends `report`. The socket is now up in ~3s regardless of suite stability, a serving build
needs no exclusion list (the crash-prone checks are on demand), and a crash during `report`
costs one session and a reconnect rather than the whole endpoint. The screen is skipped in
this mode too, which also steps around whatever the intermittent fault is. The static-report
behaviour is unchanged for the ordinary (non-serving) build, which is the build that is for.

**Report streams (#2).** `report` now copies every record down the socket as the suite runs,
via an additive `obs_set_write_tee` hook in `obs_write` - so a driver receives the full
section/try/res/sym/tally stream between its `ack` and `done`, as docs/PROTOCOL.md always
said it would, while the report still reaches stdout and the file sink. Verified the socket
stream is identical to a plain run (no duplication): both emit 5909 `sym` records, which also
corrected a stale belief - the census is ~6089 symbols now, not the few hundred assumed.

**Deck target (#4).** `make deck` builds the host-shaped binary (Linux x86-64, platform
stubbed, POSIX socket and file backends) with `GPU=1`, because the Deck's value is being the
project's only RDNA2 execution oracle, not its CPU. It serves at runtime via
`obscene-deck --serve`; there is no SERVE build flag for it because a Linux binary takes an
argv flag where a console module cannot. Named separately from `host` so "build for the
Deck" is first-class and a Deck result is not confused with a dev-machine one by habit.

Left undone deliberately: the intermittent shadPS4 mid-suite crash itself. The host build
runs all 26 sections deterministically every time, so the evidence points at shadPS4, but it
cannot be proven ours-or-theirs without a stable reference - which is the whole argument for
hardware. Listen-first makes it not matter for the socket.

