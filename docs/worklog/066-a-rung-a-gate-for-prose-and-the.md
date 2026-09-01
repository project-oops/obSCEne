# 2026-08-25 - a rung, a gate for prose, and the concurrent-build fix


### The provenance ladder gained `implementations` (D169)

D166 left a check with nowhere honest to sit. What supports it is shadPS4's `Clear`
(`m_bits &= bits`, C++) and PS5PCEM's `clearEventFlag` (`object.bits &= mask`, Zig) - two
languages, no shared codebase. `assumed` says the project reasoned it out, which discards
somebody's working code; `documented` claims a citation nobody here can produce, which is the
mistake that caused D166.

```
ASSUMED -> IMPLEMENTATIONS -> SPEC -> DOCUMENTED -> DERIVED -> HARDWARE
```

Below `SPEC`, and the placement is the caveat: implementations are not independent witnesses -
`obscene-tool consensus` says so in its own output. Exactly one check carries it.

This reopens the `measured-elsewhere` proposal we declined. That decline rested on "the origin
field carries which-machine as data", which is right and **does not transfer**: there is no
origin field on the documentation axis. Said so on the bridge, because the decline was on the
record between the two projects.

### Prose anchored to the source (D170)

`doccheck` catches a document naming something that does not exist. Nothing caught one
*describing behaviour* - `PROTOCOL.md` stated "It binds loopback by default" for part of a day
after that was reverted from `net_posix.c`, and it was found by accident.

Verifying a paragraph is not on the table; verifying the one literal it rests on is:

```text
<!-- obscene:claim file=src/probe/net_posix.c contains=INADDR_ANY -->
```

Flipping the constant back reproduces the original failure as
`docs/PROTOCOL.md:428 says src/net_posix.c contains INADDR_ANY, and it does not`.

Four claims, all in the security posture - what the socket binds, that `write`/`blob` need
`OBS_NET_ESCAPE`, that the secret is generated at run time, that no build-time secret exists.
Opt-in, and that limit is deliberate: a gate that inferred claims would cry wolf, which is how
`obscene-tool rows` nearly died at birth reporting 368 macro-generated rows as missing.

**It read its own documentation as a claim.** D170 shows the syntax in a fenced block and the
first parser counted it - five claims where four were written. It happened to be *true*, which
is the worst way to be wrong: a documentation example that has quietly become a build
dependency breaks the moment somebody edits it to illustrate rather than to describe. Fenced
blocks are skipped now, which is `doccheck`'s "only inside code spans" rule arrived at from the
other direction.

### Two fpPS4 runs blamed on a concurrent build, and that was the wrong cause

Two reports came back with 8 results instead of the historical 742, and both times a
`sweep-build.sh` had overwritten `/tmp/obs` while the emulator was starting. That is a real
hazard and it is a real fix - `run-emulator.sh` has always honoured `VM_MODULE`, so a run that
must survive concurrent work builds into its own directory:

```sh
GEN=4 BUILD=/tmp/obs-fp sh scripts/sweep-build.sh
VM_MODULE=/tmp/obs-fp/obscene-module.elf sh scripts/run-emulator.sh --emulator …
```

**It was not the cause.** Isolated that way, with nothing else building, `crashes: 0` and
`timedout: 0`, the run still produced 8. The report says why in one line:

```text
OBS|try|007-responsive/libc|libSceLibcInternal|strlen
```

No result after it. fpPS4 dies at `strlen` in the third section, and the exclusion list in the
build directory was **shadPS4's**, copied there by hand. A loader needs its own list, which is
what `sweep.sh` exists to build - and its own documentation says the first pass must start
empty, because "a stale exclusion silently hides a check that no longer crashes".

Worth recording as a wrong diagnosis rather than quietly replacing: a plausible cause was
available, it explained the symptom, and it was confirmed by nothing. The evidence that would
have settled it - a dangling `try` naming the function - was in the report from the first run.

