# D329 - a network section for Porthole, and a record for "I could not look"

**assumed** - 2026-09-08

Porthole is an elfldr payload that serves video and controller sockets over libSceNet, which it
must resolve itself because imports do not auto-bind in unsigned payload mode. Nothing had
confirmed the network is reachable that way, and the constants its accept loop turns on - the
non-blocking option value and the would-block code - were taken from public headers that
disagree. This adds the section that answers it, and a record that lets payload mode say when it
could not look at all.

## The section (`102-net`)

Six checks, each resolving libSceNet the way a payload must and reporting what it found:

- `resolve`: for each of the eight socket calls (and `connect`), three records - the bound-import
  address, the dlsym-gadget address, and the kernel export-table walk (D277/D300) address - so a
  null one is told from a call that failed, and the route that works in each leg is visible.
- `listener`: socket / bind / listen on a scratch port (9891; ports 9891-9895 are allocated per
  check to avoid EADDRINUSE collisions), each return reported, and whether the name argument to
  `sceNetSocket` matters.
- `nonblocking-option`: `setsockopt(SOL_SOCKET, 0x1200, ...)`, and `0x1100` if 0x1200 is refused,
  each raw code reported. The two values are the disagreeing public sources; this is the one that
  decides whether Porthole's accept loop blocks.
- `recv-would-block`: a `MSG_DONTWAIT` recv on an idle listener (port 9892), reporting whether it
  returned promptly and the **exact** negative code - reported raw, because Porthole matches on it.
- `accept-inherits`: a loopback self-connect (port 9893), then a send on the accepted socket, to see
  whether it inherited the listener's non-blocking mode.
- `sockaddr-bind`: bind with `sin_len` = struct size (port 9894) and with `0` (port 9895), `sin_vport`
  zero either way, so the two in-tree sockaddr shapes can be reconciled to one without port-reuse
  collisions confusing the outcome.

Two of these - the non-blocking option and the would-block code - are what block Porthole's first
hardware run; the rest can follow.

### Why the check address is the check, not a symbol

The harness skips a check whose table address will not resolve, which is right for a probe of one
symbol on a loader that lacks it. But in payload mode **every** direct import is unbound, and that
is exactly the leg this section exists to measure - the point is to reach the call through the
dlsym gadget instead. So each net check's address is its own function, always callable, and the
check does its own resolution and skips itself when nothing resolves. The pattern is the one
`100-input/dualsense-symbols` already uses.

### Two names became callable

`sceNetSetsockopt` was absent and is now declared and imported; `sceNetConnect` moved from the
census to `@called-elsewhere` for the self-connect. The other seven were already declared for the
command socket (`net_target.c`).

## The resolution record (Ask 4, and the honest-failure principle)

Separate from Porthole. In payload mode obSCEne wrote its report by calling libkernel and yet
recorded libkernel as handle `0x0` and its symbols unresolvable, because the loader hands a
payload no module list and no dlsym handle. `libSceNet` and `libScePad` get the same treatment, so
their payload-mode verdicts carry no information: a reader cannot tell a library that is absent
from one the enumeration could not see, which is the distinction Principle 2 exists to keep.

The fix is a run-level `resolution` record beside `guard` and `peripherals`: `works` or
`unavailable`, with a reason. When it says `unavailable`, every `module|...|0x0` and every
unresolvable symbol below it reads as "not seen", not "absent". One line, so the report can say "I
could not look" rather than implying it looked and found nothing - which is the more useful and
the more honest of the two.
