# 174 - suspend lifecycle census (141-suspend) and button-bits edge capture

**2026-09-23**

Took temporary ownership of the two open bus requests and implemented the probe side of
both. Neither can be *closed* from this side - each needs a run this machine cannot make
alone - but the code is what makes each run answer in one pass. See D333.

## 141-suspend (REQ-20260922T2226Z-5e8c)

A new section resolving, and never calling, the surface a big-app must satisfy to be
suspended cleanly - the mechanism behind neverball's
`0xa0d0c00f CPU_FAULT_SUSPENDPOINT_TIMEOUT_IN_SUSPEND_ASYNC` on Close. Four checks:

- `declare-ready` - libSceSystemServiceSuspend's three census-callable symbols
  (`sceSystemServiceDeclareReadyForSuspend` + the Enable/Disable notification pair).
- `receive-event` - libSceSystemService's event pump (`ReceiveEvent`, `GetStatus`,
  `GetEventForDaemon`, `GetPSButtonEvent`, `IsAppSuspended`).
- `application-lifecycle` - libSceSysCore's `sceApplication` suspend/resume family.
- `agc-suspend-point` - libSceAgc's `sceAgcSuspendPoint`, the GPU-stream suspend point;
  emits the `0xa0d0c00f`/`0xa0d0c00c` fault codes beside it and a
  `gpu-drain-required / needs-live-suspend` marker for the behavioural arm an inert probe
  cannot settle.

Resolution-only and string-resolved through `obs_module_open`/`obs_module_symbol`, so it
adds nothing to the census and needs none of the five add-a-check steps (nothing is
called; `DeclareReadyForSuspend` would declare the probe itself ready to freeze). D008.
Registered at 141, beside the oracle. Host build clean under `-Werror -Wconversion`.

## button-bits edge capture (REQ-20260910T0650Z-d1c4)

`100-input/button-bits` reported only the OR of the button word, which cannot map a button
to a bit - the reason the request stayed open after the automated sweep settled its two
neighbours. It now also emits an `edge` row (the bit's index) the first sample each bit
appears in, in press order, plus an `edges` count. One manual run pressing buttons in a
documented order now maps every face button to its offset/bit. Window ~4s -> ~8s;
`button-or` unchanged, so no historical row moves.

## Follow-up: load-on-demand (added after the first run)

The 2026-09-23 run (`reports/hardware/20260923-suspend-eboot.obs.log`) settled the surface:
`receive-event` passed 5/5 (the event pump is reachable) and `agc-suspend-point` passed
(`sceAgcSuspendPoint` at `0x800594980`), but `declare-ready` and `application-lifecycle`
both skipped - libSceSystemServiceSuspend and libSceSysCore are absent from a homebrew
title's address space, though the corpus marks their symbols callable. Neither is in the
sysmodule id table, so `obs_module_open` only tried `LoadStartModule` over its prefixes and
swallowed the codes.

Added a fifth check, `141-suspend/load-on-demand`: it calls `sceKernelLoadStartModule` on
each candidate `.sprx` (built from the census library names, no guessed id) with a poisoned
result word, reports the handle and code per path so "not found" reads apart from "refused",
then re-resolves the two entry points to report whether the load made them appear. This
loads only - it still calls none of the suspend/lifecycle symbols. Host build clean; the
next hardware run says whether the declare-ready path is reachable to a homebrew title at
all.

## What is left, and what is not mine

- Both requests remain OPEN on the bus pending their runs: a controller + a human for
  d1c4, a live Close on hardware for 5e8c's behavioural arm. Not run here (no unasked
  console runs).
- `make check` is currently red on 22 `censused twice` problems - all libc/unwind/`dl*`
  C++-exception symbols (`__cxa_throw`, `_Unwind_*`, `dladdr`, `dlsym`). These are a
  half-moved-names condition in committed `libc.c`/`surface.h` (they already appear in
  `data/obscene-report.txt` at HEAD), unrelated to this session's files and left for
  whoever owns that C++-exceptions work. This session's section is not among the
  duplicates and validates clean apart from that pre-existing red.
- Working tree also carries another session's edits to `agc.c` and `scripts/pull-log.sh`;
  not touched here.
