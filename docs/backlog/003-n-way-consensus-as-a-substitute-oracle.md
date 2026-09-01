# 12. N-way consensus as a substitute oracle


Nothing carries `hardware`, and on current trajectory nothing will until the hardware is
available. That caps what the suite can claim: when obSCEne and an emulator disagree, the
report cannot say which is wrong, and `fail [assumed]` is a verdict the recipient can
decline without argument.

There is an oracle available that is not the hardware: **agreement between independent
implementations.** `obscene-tool consensus`, taking N reports and printing only where they
disagree, would change the message from "you failed check X" to "you are the only one of
four that fails X". That is actionable without authority and costs a subcommand.

It also gives the provenance ladder a rung that can be climbed without hardware:
`assumed` → `consensus` → `hardware`.

**Two things it must not pretend.** These emulators are *not* independent of each other -
they read each other's source, and fpPS4's NID table is a superset of ps4libdoc's. This
project made exactly that mistake once (D064), counting one source twice. So consensus
must record *which* implementations agreed, so correlation stays visible rather than
being laundered into a number.

**The prerequisite turned out to be softer than this said.** It claimed consensus needs a
second *emulator*. The host build is a real implementation of the POSIX and C library
surface, so disagreement against it is disagreement with something known to work - which
is the closest thing to an oracle available without the hardware. `consensus` runs on host
and shadPS4 today.

**And Kyty is closed as a candidate, with evidence** (D080). Its `sceKernelWrite` refuses
standard output by design, and the `puts` fallback is registered under a library name this
module does not import from. Getting a report out of it would mean importing the C library
under a loader-specific name - an accommodation in the import table itself, which is worse
than the three that already exist. It remains the only source of a named unresolved-import
list, which is what it is uniquely good at.

**Overtaken.** This said craziiEmu was "the remaining candidate for a third report" - there are
now five reports, from shadPS4, PS5PCEM, fpPS4, Kyty and prosper, and craziiEmu is not among
them. The scarce thing is no longer a third report; it is a third *current-generation* one:
PS5PCEM and prosper both complete, and two implementations produce only `SPLIT`s and never an
`OUTLIER`, so no majority can form and neither can be called wrong. An oracle needs three.
(D197)

---

