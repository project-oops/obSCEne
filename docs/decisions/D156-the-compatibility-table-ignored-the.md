# D156 - The compatibility table ignored the census control that was in every report, and presented a void count beside a measured one


Found by finally reading the 24 consensus outliers rather than only generating them.

Ten of the 24 were `host alone says fail`, which is the host being a PC and not a finding.
Twelve more were `900-surface/*` where PS5PCEM stands alone, which is the known stub-versus-
honest split (D130) restated once per library. That left two that mattered, and one of them
was this.

**`900-surface/control` fails on shadPS4 and fpPS4.** The check probes one symbol that must
resolve and one that cannot exist; a loader that stub-resolves everything answers "present"
to the impossible one. Its failure message has always said what that means:

> a symbol that does not exist reported present; every count in this section is meaningless

And the table printed `35337 / 0` for shadPS4 and `376 / 0` for fpPS4 with nothing to
distinguish them from PS5PCEM's `3736 / 31601`, which was actually measured. A reader
comparing those three numbers is comparing two loader opinions against one measurement.

The verdict was in every report the table was generated from. The table simply did not read
it - which is a worse failure than not having the control, because the control ran, said the
number was meaningless, and was ignored by the thing that published the number.

**Marked, not hidden.** The counts are still shown, with `(void)` beside them, and the
document explains what earns the marker. Hiding them would lose the fact that the loader
resolved 35,337 symbols, which is itself the evidence of stub-everything behaviour; the
marker says only that the number measures the loader rather than the platform.

A report with no control record at all is treated as trustworthy. That case is a run that
died before the census, and it has not been contradicted - marking it void would be a claim
the run never made.

