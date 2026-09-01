# D187 - "The display accepted it" and "a frame reached the screen" are different facts, and the report only carried the first


`obs_display_open` reported `ready|1920x1080 framebuffer` once an output opened, a
framebuffer registered and a flip was accepted. On Kyty every one of those succeeded and the
window was black for the whole run. The report asserted a working display on a platform that
could not present - **the only claim in it with no measurement behind it**, in the program
whose entire argument is that a return code is not evidence.

### The question that produced it

*"If Kyty doesn't implement the GPU for the current generation then it is not a
current-generation emulator, so we should be able to detect that at runtime - and PS5PCEM
implements them, so we have a testbed for what does versus what doesn't."*

Correct, and the framing is the important part: not *which loader is this*, which would be the
per-loader accommodation this project refuses, but *does this platform present*. That is a
behavioural question with a behavioural answer.

### The mechanism

`sceVideoOutGetFlipStatus` reports how many frames have been shown. Submit a flip, watch the
counter. It moves or it does not, and neither answer needs to know which loader is running.

Promoted out of the census, which required a struct layout - the thing D008 forbids inventing.
Two independent implementations agree on the **first field**, which is the only one read:

    Kyty       uint64_t count;   // first member
    PS5PCEM    count: u64,       // first member, in two separate files

They disagree about most of the rest, which is left alone behind a buffer far larger than
either writes. Same standard D111 set for the buffer descriptor.

### Polled, not read once - and this is the part that mattered

The first version read the counter immediately after the flip and reported **shadPS4 as
blind**, which draws the report perfectly well and whose implementation of the call is
entirely correct. A flip is *queued*; the counter moves when a presenter picks it up. A test
that calls a working display broken is worse than no test at all.

Twenty polls at five milliseconds, once per run, then never again: the question is whether the
platform presents *at all*, and one frame settles it.

### Made to fail before being believed

Kyty's flip registration was removed, deliberately, and the module rerun:

    OBS|display|blind|flips are accepted and the frame count never moves

Restored, it reports `presenting`. A guard nobody has watched reject something is a guard
nobody knows anything about.

| loader | verdict | true? |
|---|---|---|
| shadPS4 | presenting | yes - draws the report |
| PS5PCEM | presenting | yes - draws the report |
| fpPS4 | presenting | yes - draws the report |
| Kyty (gen 4) | presenting | yes - frames reach the display, black ones |
| Kyty, flip unregistered | **blind** | yes - the failing case |

### What it still cannot answer

Whether the frame was *correct*. Kyty presents black because its renderer reads a linear
buffer through its tile path, and a guest cannot read back what was scanned out. `presenting`
means a frame arrived, not that it was the right one.

Status: **assumed** - the first field being the frame count rests on two emulators agreeing.

