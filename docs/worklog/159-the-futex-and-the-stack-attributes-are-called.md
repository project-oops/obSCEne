# 2026-09-07 - The futex and the stack attributes stop being census entries

Two behavioural sections for the sibling emulator's open threading premises, both promoted out
of the census into real calls. Documented in `docs/decisions/D322` and `docs/backlog/024`.

## What was added

- **`031-stackattr`** (three checks): what a thread attribute set says about a running thread's
  stack. `self-describes` runs the sequence three retail titles run - `scePthreadSelf`, then
  `scePthreadAttrGet` on the answer, then the address and size - and records both.
  `address-is-the-base` settles **which end of the stack the reported address is**, by placing
  the address of one of its own locals against the reported region. `fresh-attr-names-no-stack`
  records what a set nothing configured reports.
- **`032-syncaddr`** (four checks): the platform's futex, which nothing has ever called.
  `wait-returns-on-mismatch`, `wake-releases-a-waiter`, `compare-width` and
  `wake-with-no-waiter`. Every wait runs on a worker nobody joins; the main thread only sleeps,
  reads a state counter and calls the wake.

Five names moved from the census to `platform.h`, `imports.c` and the `@called-elsewhere` block:
`scePthreadAttrGet`, `scePthreadAttrGetstackaddr`, `scePthreadAttrGetstacksize`,
`sceKernelSyncOnAddressWait` and `sceKernelSyncOnAddressWake`. The
`OBS_SURFACE_SYNC_ON_ADDRESS` group held exactly those last two and is gone; the sized variants
in `..._ADDRESS2` stay censused. 239 -> 246 checks, 48 -> 50 sections, 39532 -> 39527 symbols.

## Every check was made to fail before being believed

`make host` passes all seven. That is the weaker half of the evidence; each was then broken on
purpose:

| break | result |
|---|---|
| the host wake does nothing | `wake-releases-a-waiter` **failed** naming the state the worker stopped in; `compare-width` **skipped**; the run finished |
| the host compares 32 bits | `compare-width` reported **32** and recorded the worker's release |
| the address is the stack's top | `address-is-the-base` reported a **top** |
| the region contains no frame | `address-is-the-base` **failed**, naming the address |

The first of those is the one that matters most: it proves a platform whose futex never returns
costs one worker thread and not the run, which is the entire safety argument for calling a
blocking function at all (D322).

## Surprises

- **A C toolchain was reachable after all, through Docker.** Worklog 158 recorded that no WSL
  distribution and no native compiler existed here, so the two sections it added were never
  compiled. The only WSL distribution is docker-desktop, and a `gcc:13` container with
  `/oops` mounted builds and runs the host suite (`make host CC=gcc`, since the Makefile pins
  clang). **`016-syncbounds` and `019-posixerr` have now been compiled and run for the first
  time** as a side effect, and both pass. That is worth knowing before the next session assumes
  it cannot build.
- **Adding two sections broke the build, and the right thing caught it.** `OBS_SCREEN_MAX` was
  48 against a registry of 50, and D259's compile-time assertion in `registry.c` failed the
  build naming itself. Raised to 56 - what two columns hold by `screen.c`'s own note, not
  arbitrary headroom.
- **The attribute family had no host stubs at all**, so `010-kernel/thread-attributes` has been
  skipping since it was written. It now runs and passes. A skip on the host build is not
  necessarily inert: it can be a check nobody has ever validated.
- **The guards gate caught a real violation**, exactly the D058 shape: `compare-width` announces
  the wait and calls the wake to release a worker parked by a 32-bit comparison. Guarded.
- **The hardware reports name `scePthreadAttrGet` as present, and the current census does not
  contain it.** Those `OBS|sym` records come from an older generation - `ps5-full-run.txt` says
  `OBS|meta|1|28|521` against today's 50 sections. A presence record in a report is a fact about
  the census that produced it, not about the census now.

## What is owed

The hardware run. Both sections are `assumed`/`derived` until a console produces the records
`docs/backlog/024` lists, and the futex's third register - a timeout in an unestablished unit -
is deliberately not declared and not probed.
