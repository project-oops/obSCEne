# 2026-09-07 - Two probes for the heaviest premises orbistoun is blocked on

Authored two non-invasive behavioural sections to settle the top two premises in
`docs/backlog/022`, and documented them in `docs/backlog/023` and `docs/decisions/D321`.

## What was added

- **`016-syncbounds`** (four checks): the bounds of the semaphore and event-flag primitives
  `015-sync` proves work. `sema-bad-handle` and `event-flag-bad-handle` record the code each family
  returns for an invalid handle, which answers D540's "do the two families agree" by inspection of
  the two `err` records. `sema-count` settles whether the poll's `need` argument is a count or a
  flag. `event-flag-waitmode` sweeps candidate wait-mode values to find which bits the platform
  understands, where only AND has ever been modelled.
- **`019-posixerr`** (two checks): the failure convention of the POSIX-named exports, the heaviest
  single ask in `022`. `fd-encoding` provokes `posix_read`/`posix_write` on descriptor -1;
  `pthread-encoding` provokes a write lock while a read lock is held. Each records the raw code and a
  classified encoding (`posix`/`vendor`/`other`), in two independent families so the result is a
  convention rather than a point.

Both sections were wired into `registry.c` (016 after sync, 019 after relational) and `sections.h`,
and the generated counts updated: 233 -> 239 checks, 46 -> 48 sections.

## The two calls worth knowing

- **`sceKernelWaitSema` is measured through `sceKernelPollSema`.** `022` names Wait, but Wait blocks
  and its timeout unit is the unestablished thing, so the same `need`-count and bad-handle questions
  are asked through the non-blocking Poll, and Wait is not added to `platform.h` at all (D321).
  Reviewable: it is a deliberate divergence from the literal ask, on the no-block rule.
- **The POSIX exports are resolved by name**, reusing `017-posix`'s pattern, so nothing enters the
  census. On the host they resolve to real `read`/`write` and the existing rwlock stubs, which makes
  the host the known-good POSIX baseline.

## Surprises

- **No WSL distribution is installed on this machine, and there is no native C compiler** (no
  clang/gcc/make on PATH). So `make host`/`make check` could not be run at all, and the two sections
  have not been compiled. Validation was the Rust gates instead - `guards`, `caps`, `counts`,
  `surface`, `doccheck`, `decisions`, `protocol` all pass, which covers the check tables, the
  cross-symbol guards, the capability ordering and the counts, but not the compile. A `make check` on
  a toolchain machine is still owed.
- **The Rust gates overflow the stack on Windows.** Every gate shares a parser that is fine on
  Linux's 8 MB main-thread stack and blows Windows' 1 MB one. Building the tool with
  `RUSTFLAGS="-C link-arg=/STACK:67108864"` fixes it; without that every gate prints only "has
  overflowed its stack" and looks like a failure.
- **The decision-index splitter read D321's status as "blocked"** because the word appeared in the
  title, and its status regex greps the first six lines and takes the first keyword it finds - the
  exact trap its own comments warn about. Renaming the title to avoid the word fixed it to `assumed`.
