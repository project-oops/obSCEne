# Recording: the encoder the console already drives


`105-record`, four checks over `libSceVideoRecording`. All of them refusals, for the reason
`media.c` gives and one more: **the arities are assumed.** The corpus names a symbol; it does
not say how many arguments it takes, and declaring too few leaves the extra registers holding
whatever was there - harmless until the callee reads one as a pointer.

So only the plausibly-integer functions are called, with obviously invalid values.
`sceVideoRecordingOpen`, `Open2`, `SetInfo` and `GetInfo` take pointers and are deliberately
absent: **a check that kills the probe takes the whole report with it**, including every check
that had already passed. Those belong on the command protocol, where `ack` is flushed before
the call and a fault is a recorded `died` rather than a lost report. When a sequence has been
established that way it can come back here, which is what this suite is for - keeping what is
known, not finding it.

Why this library is worth the attention: the console encodes video continuously for its own
share feature, in hardware, and this is the interface that drives it. It is the cheapest route
to encoded frames off a target - not a capture pipeline written from nothing, but the one
already running. `libSceVencCore` sits behind it with `sceVencCoreGetAuData`, which is where
encoded access units come out.

### Two surprises, both recorded as decisions

**The documented way to add a check does not cover a mined name** (D202). Step one says to move
the name from the census into `platform.h`. That works for the curated census, which has
`@called-elsewhere`; the mined corpus has no equivalent and `mine.rs` reads no exclusion list.
Following the step produced four redefinitions in `surface.c` - the exact breakage the step
exists to prevent, arrived at from the direction it does not describe.

**The registry will hold any order; the report contract will not** (D203). Numbered 170 and
placed by dependency, it built clean and failed `make check` with *sections are out of order*.
The numbers carry the layering, not the list position. 105 satisfies both, and it is where the
dependency argument had already put it.

### State

`make check` passes, exit 0. On the host build the section reports 3 pass, 1 partial against
stubs returning `OBS_HOST_NOT_IMPLEMENTED` - which is the known-good implementation this is
meant to be run against before believing anything it says. **It has not run on hardware.**

Prosperous gained `pros supervise` in the same session: it watches the probe's port and
re-sends the module through the loader when it stops answering. `docs/PROTOCOL.md` names the
restarter as *"a person on a console"* and that is now optional - which matters here, because
the pointer-taking half of this library is meant to be explored over the protocol, where each
fault costs a re-send.

