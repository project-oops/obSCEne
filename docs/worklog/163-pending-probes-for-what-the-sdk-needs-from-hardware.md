# 2026-09-08 - Pending probes for what oops-sdk needs from hardware

oops-sdk listed the measurements only hardware can give - controller record stride and button
bits, stick/trigger ranges, keyboard/mouse layouts, the audio format selector and blocking
behaviour, the video attribute block. Most need a peripheral attached and some a button pressed
while the probe watches. This session added the status and the probes for that. Documented in
`D328`.

## A pending status

`OBS_PENDING` (`pending`) is added beside pass/partial/fail/skip/crash: the check can run but was
not given its input, so it samples, finds nothing, and says it is still waiting - never blocking,
never fatal, and a re-run with the input answers it. Distinct from skip (which says the check does
not apply); ordered `crash < pending < skip < ...`. Wired through the C harness, the report
contract (trailing tally field, status row, ordering), and the Rust tooling (enum, parse, colour
`PEND`), the same way `crash` was.

## The probes

Four census names became callable (`scePadRead`, `sceAudioOutGetPortState`, `sceAudioOutOutput`,
`sceAudioOutSetVolume`): moved to `@called-elsewhere`, declared in `platform.h`, listed in
`imports.c`, census regenerated. Then, all calling their functions as imports so they run in every
leg and going PENDING when the input is absent:

- `100-input`: read-extent, batched-read, button-bits, stick-trigger-range.
- `101-input-ext`: keyboard-held, mouse-moving, reachability.
- `090-audio` (no peripheral, so no PENDING): format-selector, open-shapes, blocking, volume-flag.
- `080-video`: attribute-block (tiling 0 vs 1), flip-status, and visual-flip as a PENDING naming
  what it needs.

A `peripherals` record now says what was attached at run start, so a PENDING reads against it; the
SDK poll probe dumps its raw record, not only the button word. `make check` green; the host
harness registers and runs all fourteen new checks without a crash, emits the `peripherals` line,
and the tally carries its sixth field.

## Left for later

The event-queue flip-wait shape needs the libkernel equeue symbols, not yet on the surface - a
census move of its own, deferred rather than stubbed as a permanent skip. The visual tear/duplicate
observation is the PENDING `080-video/visual-flip` until it is run with the display handed over.

## Surprise worth keeping

`make host` shows every new probe as fail or skip against the stubs, which is correct and the whole
point of the host build: the stubs are not a controller, so the probes' real answers only exist on
hardware - which is exactly why they are PENDING there rather than green here.
