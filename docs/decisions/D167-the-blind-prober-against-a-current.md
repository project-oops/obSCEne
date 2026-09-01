# D167 - The blind prober against a current-generation loader: the two emulators are opposites, and twelve functions hand the guest a host address


`910-bulk` had only ever run against shadPS4. Pointing it at PS5PCEM took six rounds to reach
the end of the list, and the result inverts:

| | shadPS4 | PS5PCEM |
|---|---|---|
| answered | 32,275 | 3,709 |
| `zero` | **31,111 (96.4%)** | 290 (7.8%) |
| `rejected` | 125 | **2,986 (80.5%)** |
| `error-shaped` | 961 | 349 |
| `value` | 78 | 84 |

**Neither number is a quality score and the pair is the finding.** shadPS4 resolves everything
and accepts everything: 32,275 answers, almost all zero. PS5PCEM resolves 3,709 - a ninth as
many - and *refuses four out of five*. `COMPATIBILITY.md` has argued exactly this in prose
since it was written ("a loader that resolves only what it implements scores nothing on
presence while being the most honest of the group"). This is that sentence with numbers under
it.

### What it settled for BACKLOG §1

That entry ended "Nothing here is called", and it stayed true through every previous sweep
because the only loader swept was previous-generation. All 118 AGC symbols that answered under
shadPS4 returned zero - a PS4 emulator stubbing a PS5 interface, which measures the emulator.

On PS5PCEM, 108 answered and **21 refused with real errnos**: EFAULT ten times, EINVAL nine,
EBADF twice. That is argument validation running, and it is the first evidence this project
has that anything is behind an AGC name anywhere.

It is still not exercising the interface. A call with nothing in its arguments can only learn
whether a function objects to nothing; driving a command buffer needs the struct layouts of
§2, which has not moved.

### Twelve functions return an address in the emulator's own image

`sceAgcGetRegisterDefaults2` returned `0x7ff6d0961b80`, and eleven others across
`libSceFontFt`, `libSceJson`, `libSceJson2` and `libkernel` (`getargv`,
`sceKernelGetSanitizerMallocReplaceExternal`) answered in the same `0x7ff6…` family - Windows
x64 user space, inside the emulator binary. Every one of the twelve is a function whose
contract is to return a pointer.

**Recorded as an observation, not a defect.** An HLE emulator running guest code in its own
process has a single address space, so a host pointer is reachable by the guest and returning
one is a reasonable implementation. Calling it a bug would be asserting a design constraint
this project has no standing to impose.

What it *is*, firmly: **not a platform fact.** A console returns an address in a guest range,
and no value in that family can be read as one. A corpus recorded here carries properties of
the emulator's design, which is the whole reason the origin field exists and why `target=` is
operator-asserted rather than probe-claimed (D108). It is also a clean fingerprint - twelve
pointer-valued functions all answering inside the host image is a signature of HLE-in-process,
not noise to average away.

### Why this run was cheap and the other was not

Six rounds against thirty-one for shadPS4's first pass. A round ends at the first function
that does not return, and PS5PCEM resolves a ninth as many symbols - an unresolved symbol is
never called, so it cannot fault. **Fewer implementations means fewer crashes means fewer
rounds**, which is a property of coverage rather than of quality, and worth stating before
somebody reads six rounds as the better loader.

