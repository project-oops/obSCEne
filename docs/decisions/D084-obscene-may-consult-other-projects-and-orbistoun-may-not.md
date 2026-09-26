# D084 - obSCEne may consult other projects; orbistoun may not

**Status:** decided
**Date:** 2026-09-26

obSCEne reads emulator source, public databases and toolchain headers for facts. It does not
recommend that orbistoun do the same: orbistoun obtains names by proposing candidates and letting
the hash confirm them. Findings sent to orbistoun are phrased as mistakes to avoid, never as how
another loader implemented something.

**Why:** orbistoun's provenance property is all-or-nothing; adopting an external table would make
every name it already found untraceable. The permissive side of the asymmetry is what makes this
project's advice about provenance unsafe to transfer.

**Rejected:** one provenance rule for both projects - either blinds the probe or poisons the
emulator.
