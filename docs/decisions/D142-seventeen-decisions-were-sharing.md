# D142 - Seventeen decisions were sharing thirteen numbers, and nothing was checking. The log now gates its own numbering


`D102` through `D114` each headed two or three unrelated decisions. Two numbering streams -
one from the GPU and protocol work, one from the corpus and census work - each appended from
where it had last looked, and neither noticed the other. A hundred citations across the tree
pointed at numbers that no longer identified anything.

This is a worse failure than a dangling reference, and quieter. A broken link announces
itself; a duplicate number does not. Every `see D105` still finds *an* entry, still reads as
a working citation, and is wrong roughly half the time - and the only way to notice is to
follow one and recognise that the decision you landed on is not the one being talked about.
The log looked completely intact.

**Which entry keeps a contested number is decided by citation count, not by date.** The
entry with the most references elsewhere in the tree keeps it; the others are renumbered from
D125 up. That is the assignment where the fewest existing citations become wrong - three
source references, against the thirty-odd that would have broken under first-come. All three
were repointed, along with seven in this file, ten in `WORKLOG.md` and two in
`COMPATIBILITY.md`, each resolved by reading what the citing sentence claims and matching it
to the entry that says it.

**The gate lives in `doccheck.py`.** `DECISIONS.md` is exempt from that script's other checks
because it is a dated record rather than a description of the tree - but a number is not
prose, and uniqueness is the one property the log has to hold for any citation to work. It
also resolves citations the other way: a `D<n>` naming no entry is the ordinary dangling link.

Two things had to be got right for the check to mean anything, and the first was wrong:

- **Both heading styles.** The early entries carry a title (`## D001 - Two builds from one
  source tree`), the later ones leave the heading bare. Anchoring the pattern to end-of-line
  matched only the bare form and silently skipped the first twenty-six entries - passing
  while checking eighty per cent of the file.
- **Zero-padding is spelling, not identity.** `D008` and `D8` are the same decision and both
  are in the tree, so numbers are compared as integers and reported back as written.

Shown to reject before it was believed: a deliberate duplicate heading, which it named. That
is D125's rule applied to the gate enforcing D125's file.

