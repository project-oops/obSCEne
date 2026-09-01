# D108 - A report and a corpus are different artifacts, and only the corpus carries origin


**A report and a corpus are different artifacts, and only the corpus carries machine origin
- which the operator asserts, because the probe cannot certify its own machine.**

Status: derived - from consumer feedback (orbistoun built against the spec), reconciled with
what the driver actually emits.

The feedback was right on the substance and half of it was already answered by code written
after it: the denormalised corpus PROTOCOL.md promised now exists (D106, `OBSCORPUS|`), but
`docs/OUTPUT.md` documented only the report and never named the corpus. Two documents
drifting from each other - the same failure the report table has had, one layer up.

Three things this settles:

**Report vs corpus.** A *report* (`OBS|`) is what the probe emits alone; its only origin is
`build`, the binary kind, and it has no machine provenance because there is no session to
carry one. A consumer grading a bare report should get "0 gradeable", and that is correct,
not a defect. A *corpus* (`OBSCORPUS|`) is what the driver produces from a session, machine
origin denormalised onto every line. OUTPUT.md now documents both and draws the line between
them.

**The probe cannot certify its own machine.** This is the deeper point, and it goes further
than the feedback stated. Inside an emulator, `sceKernelGetSystemSwVersion` returns the
*emulator's* chosen version; a probe stamping that as `firmware=` would be exactly the
`measured`-as-`assumed` fiction the consumer's grading rule exists to catch. So machine
identity - target, gpu, firmware, above all "is this real hardware" - is **operator-asserted
through the driver** (`drive --part target=console`), never self-reported. What the probe
observes about itself travels as ordinary records marked `observed-by=probe`, a weaker claim.

**Consequences shipped:** OUTPUT.md gains a corpus section and a report-vs-corpus section;
the drifted `sink` and `net` records are added to the table; `net.c`'s hello stops sending
the binary kind under the `target` key (it is `binary` now, freeing `target` for the
operator's machine assertion); `drive --part key=value` merges operator identity into the
corpus origin, winning over any probe self-report. The consumer's smaller note is already
satisfied - absent provenance stays absent and is never backfilled.

