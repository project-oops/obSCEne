# 1. The current generation's graphics interface - censused, and now called


Was: "no source for the names that satisfies D018". That was true when written and stopped
being true when the emulator toolkit arrived.

**87 names across `libSceAgc` and `libSceAgcDriver`**, from a current-generation emulator
that carries them as structured export attributes - identifier, name and library together
- corroborated for 81 of them by an independent NID database. Both sources public, neither
requiring anything decrypted.

**Every name is checked against this project's own hash**, and that rejected five. Two were
placeholders with the identifier embedded in the name (`sceAgcDriverUnknown_KRzWekV120`) -
a project marking a function it could not name, which would otherwise have entered the
census as a symbol that cannot exist. The other three carry a plausible name whose hash
does not match the identifier recorded beside it, so one of the two is wrong and neither is
worth censusing.

Marked `OBS_CURRENT`: absence on previous-generation hardware is correct rather than a gap.

**No longer presence-only.** This entry used to end "Nothing here is called", and that was
true until the blind prober was pointed at a current-generation loader. `910-bulk` calls every
censused symbol with nothing in its arguments, and it had only ever run against shadPS4 - a
*previous*-generation emulator, where all 118 AGC symbols that answered returned zero. That
measured a PS4 emulator stubbing a PS5 interface and nothing else.

Against PS5PCEM, 108 AGC symbols answered:

| outcome | count |
|---|---|
| `zero` | 83 |
| `rejected` | **21** |
| `value` | 2 |
| `error-shaped` | 2 |

The 21 refusals carry real errnos - `0x8002000e` EFAULT ten times, `0x80020016` EINVAL nine,
`0x80020009` EBADF twice - which is argument validation running, and is the first evidence
this project has that anything is behind an AGC name anywhere.

Still not the same as exercising the interface: a call with nothing in its arguments can only
find out whether a function objects to nothing. **Driving a command buffer needs struct
layouts, which is §2**, and that has not moved.

### The two `error-shaped` returns are worth their own paragraph

`sceAgcGetRegisterDefaults2` and `sceAgcGetRegisterDefaults2Internal` returned
`0x7ff6d0961b80` and `0x7ff6d0961bd8` - addresses inside the emulator's own Windows executable
image. Ten more do the same across `libSceFontFt`, `libSceJson`, `libSceJson2` and `libkernel`
(`getargv`, `sceKernelGetSanitizerMallocReplaceExternal`), twelve in all, and every one is a
function whose contract is to return a pointer.

**This is not necessarily a fault.** An HLE emulator running guest code in its own process has
one address space, so a host pointer is reachable by the guest and returning one is a
reasonable implementation. What it is not is a *platform* fact: the hardware would return an
address in a guest range, and no value in the `0x7ff6…` family can be read as one.

The point for this project is narrower and firmer: **a corpus recorded here carries values
that are properties of the emulator's design**, which is precisely why the origin field exists
and why `target=` is operator-asserted. It is also a clean fingerprint - twelve pointer-valued
functions all answering in the host image is a signature of HLE-in-process, not a coincidence
to be averaged away.

