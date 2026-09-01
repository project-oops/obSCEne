# 2026-08-25 - the two emulators are opposites, and §1 stopped being presence-only


The blind prober had only ever run against shadPS4. Six rounds against PS5PCEM reached the
end of the list, and the result inverts:

| | shadPS4 | PS5PCEM |
|---|---|---|
| answered | 32,275 | 3,709 |
| `zero` | **96.4%** | 7.8% |
| `rejected` | 125 | **2,986 (80.5%)** |

One resolves everything and accepts everything; the other resolves a ninth as many and refuses
four out of five. `COMPATIBILITY.md` has argued precisely this in prose since it was written -
"a loader that resolves only what it implements scores nothing on presence while being the
most honest of the group". Now with numbers under it. (D167)

**§1 no longer says "nothing here is called."** 108 AGC symbols answered on PS5PCEM and **21
refused with real errnos** - EFAULT ten times, EINVAL nine, EBADF twice. First evidence this
project has that anything is behind an AGC name anywhere. Under shadPS4 all 118 returned zero,
which measured a PS4 emulator stubbing a PS5 interface.

Still not exercising the interface: calling with nothing can only find out whether a function
objects to nothing. Driving a command buffer needs §2's struct layouts, which have not moved.

### Twelve functions hand the guest an address inside the emulator

`sceAgcGetRegisterDefaults2` returned `0x7ff6d0961b80`; eleven others across `libSceFontFt`,
`libSceJson`, `libSceJson2` and `libkernel` answered in the same `0x7ff6…` family. Windows x64
user space, inside the PS5PCEM binary. All twelve are functions whose contract is to return a
pointer.

Recorded as an observation rather than a defect - an HLE emulator with one address space can
legitimately hand back a host pointer. What it is not is a *platform* fact, and twelve
pointer-valued functions all answering inside the host image is a clean fingerprint of
HLE-in-process rather than noise.

**One thing worth saying before someone misreads the round count.** Six rounds versus
thirty-one is not the better loader. A round ends at the first function that does not return,
and an unresolved symbol is never called, so it cannot fault. Fewer implementations means
fewer crashes means fewer rounds - a property of coverage, not quality.

