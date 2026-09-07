# D321 - the encoding and bounds probes poll rather than wait, and resolve by name

**assumed** - 2026-09-07

`docs/backlog/022` is the sibling project's list of measurements it cannot make for itself,
ranked by how much a console day buys. Two premises sit at the top by weight, and this session
adds a probe section for each: `016-syncbounds` for the semaphore and event-flag bounds, and
`019-posixerr` for the failure convention of the POSIX-named exports. The premises are settled by
`docs/backlog/023`; this records the three architecture choices the sections rest on, because each
is a place the obvious approach is wrong.

## 1. Poll, never wait - and `sceKernelWaitSema` is deliberately not added

The premise names `sceKernelWaitSema`. The function blocks: on an empty semaphore it does not
return until a token arrives, and the argument that would bound the wait is a timeout whose unit
is itself one of the things orbistoun records as unestablished (its D540). Calling it is the exact
shape of hazard this suite has paid for twice - a probe that blocks loses every check behind it -
and the no-block rule (principle 8, D008-adjacent) says a blocking call is written as the `try`
form or not at all.

`sceKernelPollSema` is that `try` form. It takes the same `need` count and the same handle and
always returns, so the two questions the premise actually turns on - **is `need` a count or a
flag**, and **what does a bad handle return** - are answered by Poll with no risk. The only
behaviour `Wait` has beyond `Poll` is the timeout, and the timeout cannot be measured without a
bounded wait, which needs the very unit the probe would be trying to discover. That one
sub-question is left for a hardware session that can approach it with a known unit.

So `sceKernelWaitSema` is not added to `platform.h`. Adding it would cost the full five-step census
move (it is censused in `surface.h`, and a name cannot be callable and censused at once), a host
stub, an entry in `imports.c`, and an **assumed arity** - all to gain a measurement that is either
identical to Poll's or unsafe. The cost is real and the yield is nil. This is the reviewable call:
the premise said `WaitSema` and the probe measures the same bounds through `PollSema` instead,
which is why this decision is `assumed` rather than `decided`.

## 2. Resolve the POSIX-named exports by name, as 017-posix already does

`019-posixerr` calls `posix_read`, `posix_write` and the `posix_pthread_rwlock_*` family. These are
censused, not declared in `platform.h`, and moving them would be the same census surgery for no
reason: `017-posix` already resolves libScePosix exports by name through `obs_module_open` /
`obs_module_symbol`, and this reuses that pattern. Nothing is added to the census, so the surface
count does not move.

**It resolves libScePosix first, then falls back to libkernel, and the fallback is what makes the
premise measurable on the console.** The captured hardware runs show `libScePosix` does not load in
the PS5 app sandbox - `900-surface/..._libScePosix fail - this library could not be loaded` - which
is exactly why `017-posix` skips all five of its checks on every console capture. The same `posix_`
names are exported by `libkernel` (`data/hardware/libkernel-vaddrs.txt`), the library this program
runs on and therefore always present, so `obs_posixerr_symbol` tries the POSIX library and then
libkernel. Resolving only libScePosix, as the first cut of this section did, would have made premise
A skip on the one platform whose answer it is waiting for. The `err` records keep the `libScePosix`
label for the POSIX namespace the names belong to; where they resolve is this decision, not the
report field.

On the host build the same names resolve to the real thing - `read` and `write` from the C library,
the read/write lock from the host stubs `017-posix` uses - which makes the host the known-good POSIX
baseline the target is measured against. A check that has not passed a known-good implementation is
not evidence (principle 5), and this is how these pass one without a console.

## 3. Record the encoding; fail only on an accepted bad argument

The question is *which* failure convention the POSIX-named exports use - POSIX's `-1`-and-errno, the
bare errno a pthread call returns, or the vendor `0x8002_0000 | errno` its own twins use (the
sibling's D398). All three are legitimate answers, so the sections record the encoding rather than
grade it, the stance `140-oracle/error-codes` already takes for the vendor-named calls. The single
thing that *is* grounds to fail is the platform accepting the bad argument - a read on a closed
descriptor that returns a count, a write lock granted while a reader holds it - which POSIX settles
must be refused. That refusal is the asserted postcondition, which is why the checks carry
`OBS_FROM_DERIVED`: the refusal is POSIX's, the encoding is the open measurement.

The provoked failures are chosen to be deterministic and safe on both the target and the host
oracle: a read and a write on descriptor -1, and a write lock attempted while a read lock is held.
The literal shape the premise suggests - `pthread_mutex_lock` on an invalid handle - is avoided on
purpose: on a real lock it blocks, and on the host an invalid handle is undefined behaviour, so it
could neither run safely on hardware nor be validated under `make host`.

## What this does not do

Neither section has run on hardware, so nothing here upgrades an assumption to a measurement yet -
the sections are the instrument, not the reading. `023` records what a single run will settle, and
`016`/`019`'s checks all read `assumed` or `derived` until a console produces the numbers.

The build was validated as far as this machine allows: the Rust gates (`guards`, `caps`, `counts`,
`surface`, `doccheck`, `decisions`, `protocol`) all pass, which covers the check tables, the
cross-symbol guards, the capability ordering and the counts. The C build itself (`make host`,
`make check`) was **not** run: no WSL distribution is installed here and there is no native C
toolchain, so the two sections have not been compiled. They are written to the established idioms
and the gates that read the source agree, but a `make check` on a machine with the toolchain is the
step still owed.
