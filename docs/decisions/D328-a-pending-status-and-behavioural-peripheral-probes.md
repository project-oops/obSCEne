# D328 - a pending status, and peripheral probes that wait for their input rather than failing

**assumed** - 2026-09-08

oops-sdk needs a set of measurements only hardware can give: the controller read's record
stride, which buttons arrive on which bits, the stick and trigger raw ranges, the keyboard and
mouse record layouts, the audio format selector and blocking behaviour, and the video attribute
block. Most of these need a peripheral attached and, for some, a button pressed while the probe
watches. A probe that needs a controller and finds none has not failed and has not skipped - it
is *waiting*. This records the status added for that, and the probes built on it.

## The status

`OBS_PENDING` (`pending` on the wire) is a fifth-and-a-sixth outcome beside pass/partial/fail/
skip/crash. It means the check can run but was not given the input it needs. It is never blocking
and never fatal: the check samples its window, finds nothing, and reports that it is still
waiting; a re-run with the peripheral attached or the button pressed produces the real result.

It is distinct from `skip`, which says the check does not apply here (a library absent, a
prerequisite unmet). `pending` says it *does* apply and an input away from an answer, so it is
worth seeing apart from a skip and does not count against coverage the way a skip does. In the
severity order it sits `crash < pending < skip < fail < partial < pass` - unresolved like a skip,
but carrying an action rather than a dismissal.

The report contract gains it the way `crash` was added (D325): a trailing `pending` field on the
tally and section tally (a pre-`pending` parser reads it as zero), a `pending` status row, and the
tooling's enum, colour (cyan) and marker (`PEND`). Principle 1 is untouched - a PENDING `res`
follows its `try` normally, because the call *did* return; it just returned without the input.

## The probes

All call their functions as imports so they run in every leg (the earlier out-param probes went
through dlsym and skipped on a native eboot). Where the peripheral or the input is absent, the
result is PENDING. Four census names became callable for this - `scePadRead`,
`sceAudioOutGetPortState`, `sceAudioOutOutput`, `sceAudioOutSetVolume` - moved to
`@called-elsewhere` with declarations in `platform.h` and entries in `imports.c`.

- **Controller** (`100-input`): `read-extent` (the single-read record size), `batched-read`
  (scePadRead of four records - extent over count is the stride), `button-bits` (OR of the button
  word over a sampled window), `stick-trigger-range` (per-byte min/max over a sweep). The
  existing `controller-info` and the trigger-effect prologue dump already cover the rest.
- **Keyboard and mouse** (`101-input-ext`): `keyboard-held` (idle vs a held key, to locate the
  modifier and key-code fields), `mouse-moving` (one record for the extent, four for the stride,
  a sampled button word), `reachability` (whether the libraries come up through sysmodule and
  their read symbols resolve - the app-vs-eboot answer).
- **Audio** (`090-audio`), which needs no peripheral so never goes PENDING: `format-selector`
  (port state at selectors 0/1/2, for the channel count), `open-shapes` (which chunk sizes an
  open accepts at 48000/44100), `blocking` (timing eight 512-frame outputs), `volume-flag`.
- **Display** (`080-video`): `attribute-block` (the 256 bytes `sceVideoOutSetBufferAttribute2`
  writes at tiling 0 vs 1, to confirm which is the tiled mode) and `flip-status`. `visual-flip`
  is a PENDING that names what it needs - the display's own buffers and a watcher for tear vs
  duplicate - because that observation cannot be made from a probe drawing its own report on the
  only output.

## Reporting

A `peripherals` record is emitted once per run - pad, keyboard, mouse, audio, each opened and
closed at run start - so a peripheral probe's PENDING (or a zero extent) reads as "nothing
plugged in" rather than a defect. The SDK poll probe now dumps its raw record, not only the
button word, so a pass carries the bytes a later layout question will need. Every probe keeps all
three legs, because the app-versus-eboot split is itself a finding.

## Left for later

The event-queue flip-wait shape (32- vs 64-bit timeout timing) needs the libkernel equeue
symbols, which are not yet on the surface - a census move of their own, deferred rather than
stubbed as a permanent skip. The full visual-flip tear/duplicate observation is the PENDING
`080-video/visual-flip` until it is run in a context built to hand the probe the display.
