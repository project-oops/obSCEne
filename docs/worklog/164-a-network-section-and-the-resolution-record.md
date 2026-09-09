# 2026-09-08 - A network section for Porthole, and a record for "I could not look"

Porthole needs to know whether an elfldr payload can reach libSceNet at all, and the two
socket-option values its accept loop turns on. This session added the section that measures it and
a record that lets payload mode say when it could not enumerate. Documented in `D329`.

`102-net`, six checks: `resolve` (the import, dlsym, and kernel export-table walk addresses of each
of the eight socket calls, so a null is told from a failed call and payload resolution paths are
proven), `listener` (socket/bind/listen returns and whether the name argument matters; dedicated
scratch ports 9891-9895 avoid EADDRINUSE collisions), `nonblocking-option` (setsockopt 0x1200, then
0x1100 if refused, raw codes), `recv-would-block` (a MSG_DONTWAIT recv on an idle listener - promptness
and the exact negative code, raw), `accept-inherits` (a loopback self-connect and a send on the accepted
socket), and `sockaddr-bind` (sin_len = struct size vs 0, sin_vport zero, tested on separate ports).
The non-blocking option and the would-block code are the two that block Porthole's first run.

Each check's table address is its own function, not a libSceNet symbol, so the harness runs it
even where every direct import is unbound - which in payload mode is all of them. The check then
resolves through direct import, dlsym gadget, or kernel export-table walk (D277/D300) the way
Porthole must, or skips when nothing resolves. Two names became callable: `sceNetSetsockopt` (was
absent) and `sceNetConnect` (moved from the census).

## The resolution record

In payload mode obSCEne recorded libkernel as handle 0x0 and unresolvable while calling it, because
the loader hands a payload no module list and no dlsym handle - so libSceNet and libScePad got the
same treatment, and absent could not be told from unseen. A run-level `resolution` record beside
`guard` and `peripherals` now says `works` or `unavailable` with a reason; when unavailable, every
`module|...|0x0` below reads as "not seen", not "absent". The report can say "I could not look".

## Verified

`make check` green across all shapes, gates and 223 tooling tests. The host harness runs all six
net checks: on host, where there is no libSceNet, `resolve` emits its per-symbol import-null /
dlsym-null records and the behavioural checks skip with "did not resolve in this leg" - which is
the correct answer there. On an emulator, the eboot, or the payload the checks resolve and measure.

## Surprise worth keeping

The first build put the section at id `170-net` after `101-input-ext`, and `verify` caught it at
once: section ids must ascend in registry order, and 170 between 101 and 105 does not. Renumbered
`102-net` to match its position. The gate that reads a report's own structure paid for itself.
