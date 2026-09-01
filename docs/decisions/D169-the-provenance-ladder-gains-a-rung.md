# D169 - The provenance ladder gains a rung: `implementations`


D166 corrected a check that asserted the opposite of what `sceKernelClearEventFlag` does, and
the correction had nowhere honest to go. What supports the corrected version is shadPS4's
`Clear` (`m_bits &= bits`, C++) and PS5PCEM's `clearEventFlag` (`object.bits &= mask`, Zig,
carrying the comment "The PS5 ABI supplies the bits to retain, not the bits to remove") - two
implementations, two languages, no shared codebase.

`ASSUMED` says this project reasoned it out, which throws away that somebody's working code
says so. `DOCUMENTED` claims a citation nobody here can produce, which is the mistake that
caused D166 in the first place. The check sat at `ASSUMED` for as long as it took to conclude
the ladder was missing a rung rather than that this was a guess.

```
ASSUMED -> IMPLEMENTATIONS -> SPEC -> DOCUMENTED -> DERIVED -> HARDWARE
```

### Below `SPEC`, and the placement is the entire caveat

Stronger than this project's own guess. **Weaker than a document anyone can check, because
implementations are not independent witnesses.** These projects read each other's source -
`obscene-tool consensus` prints exactly that in its own output, "agreement is evidence, not
four witnesses" - and two sharing an ancestor agree about their ancestor.

The line to hold: at least two implementations that do not share a codebase, read directly,
and **named in the check's own comment**. One implementation is not this; `ASSUMED` describes
a single opinion accurately whoever holds it.

### This reopens something declined earlier

The sibling project proposed a value for "measured, but not on the target" and it was declined
on the argument that the origin field carries which-machine as data. That argument is sound
and it does not transfer: there is no origin field on the *documentation* axis. Declining a
second time would have been a position held for symmetry rather than for a reason.

Which is worth saying to them plainly, since the decline is on the record between the two
projects.

### Contract impact

A new value in the `provenance` field of `res` records, so a consumer matching exhaustively on
provenance needs the one line. Same shape as `unauthorised`, and it goes on the bridge for the
same reason. `counts` reports it, `OUTPUT.md` describes it with the caveat attached, and
exactly one check carries it today.

