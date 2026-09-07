# 15. Two probes for the heaviest blocked premises: POSIX error encoding, and sync bounds

Authored this session, unrun. `docs/backlog/022` is the sibling project's list of measurements it
cannot make for itself, and two premises sit at the top of it by weight. Each now has a probe
section, so one hardware run settles them. The architecture behind both is `docs/decisions/D321`;
this records what the checks do and what a run will produce.

Neither has been on hardware yet, so nothing here is answered - the sections are the instrument,
not the reading. Every check reads `assumed` or `derived` until a console produces the numbers.

## Premise A - the failure convention of the POSIX-named exports (`019-posixerr`)

This is the heaviest single ask in `022`: **which convention do the platform's POSIX-named exports
use on failure** - POSIX's own `-1`-and-errno, or the vendor `0x8002_0000 | errno` scheme its own
`sceKernel*` twins use (the sibling's D398 measured that across five families). No POSIX-named
export has ever been measured, and 24 of orbistoun's entries and roughly 101k recorded calls rest
on the answer.

`libScePosix` is resolved by name, the way `017-posix` already resolves it, so nothing is added to
the census - and it falls back to `libkernel`, which exports the same `posix_` names and always
loads. That fallback matters: the captured console runs show `libScePosix` does not load in the app
sandbox (it is why `017-posix` skips all five of its checks on hardware), so without it premise A
would skip on the console it most needs to run on. Two provoked failures, one in each of two
families, so the result is a convention rather than a single point:

| check | provoked failure | what a run settles |
|---|---|---|
| `019-posixerr/fd-encoding` | `posix_read` and `posix_write` on descriptor -1 | the file family's failure encoding, recorded as `err` and classified as `posix`/`vendor`/`other` in a `measure` |
| `019-posixerr/pthread-encoding` | `posix_pthread_rwlock_trywrlock` while a read lock is held | the pthread family's failure encoding, the same way |

A single family could be a quirk of one call; two that disagree refute "it is a convention"
outright, and two that agree make it credible for the other 147 names nobody will call by hand. The
sections record the encoding and fail only if the bad argument is *accepted* - a read on a closed
descriptor returning a count, or a write lock granted while a reader holds it - which POSIX settles
must be refused, and which is why the checks carry `OBS_FROM_DERIVED`.

The literal shape `022` suggests, `pthread_mutex_lock` on an invalid handle, is deliberately not
used: it blocks on a real lock and is undefined behaviour on the host oracle, so it could neither
run safely on hardware nor be validated under `make host`. The provoked failures chosen are
deterministic and safe in both places.

## Premise B - semaphore and event-flag bounds (`016-syncbounds`)

`015-sync` proves these primitives work; `022`/D540 asks what their edges do, and the asks are
cheap additions to any sync sweep:

| check | what it measures |
|---|---|
| `016-syncbounds/sema-bad-handle` | the code `sceKernelPollSema` returns on handles 0 and -1, recorded as `err` |
| `016-syncbounds/sema-count` | whether the poll's `need` argument is a count or a flag - signal three, take two, then two again |
| `016-syncbounds/event-flag-bad-handle` | the code `sceKernelPollEventFlag` returns on a null handle |
| `016-syncbounds/event-flag-waitmode` | which wait-mode bits the platform understands, by polling for two bits with one set under a sweep of candidate modes |

The two `bad-handle` checks answer one of D540's specific asks directly: orbistoun measured ESRCH
(`0x80020003`) for the event-flag family and carries an unmeasured placeholder (`0x7fff0003`) for
the semaphore family, so **whether the two families agree** is open. The two `err` records - one per
family - answer it by inspection, with no check having to reach into the other family's symbols.

### `sceKernelWaitSema` is measured through `sceKernelPollSema`, and that is a reviewable call

`022` names `sceKernelWaitSema`. It blocks - on an empty semaphore it does not return - and its
third argument is a timeout whose unit D540 records as unestablished. Calling it is the hazard the
no-block rule exists to prevent, so `016-syncbounds` measures the same `need` count and bad-handle
behaviour through the non-blocking `sceKernelPollSema` instead, exactly as `015-sync` polls event
flags rather than waiting on them. The one thing `Wait` has beyond `Poll` - the timeout - cannot be
measured without a bounded wait, which needs the very unit the probe would be trying to discover, so
that sub-question is left for a hardware session that can bring a known unit to it. See D321 for why
`sceKernelWaitSema` is not added to `platform.h` to get there.

## What is still owed

- A `make check` on a machine with the C toolchain. This session validated the sections with the
  Rust gates (`guards`, `caps`, `counts`, `surface`, `doccheck`, `decisions`, `protocol`), which
  read the source and all pass, but no WSL distribution or native C compiler was available here, so
  the two sections have not been compiled or run under `make host`.
- The hardware run itself, which turns the `assumed`/`derived` checks into the `err` and `measure`
  records the premises above are waiting on.
