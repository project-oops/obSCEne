# D058 - A check guards every symbol it calls

**Status:** decided
**Date:** 2026-09-26

The harness skips a check whose own table-row symbol is null. A check body that calls any other
platform symbol marks it with `OBS_REQUIRE(&sym, ...)` and tests its address first.
`obscene-tool guards` compares the symbols each body calls with the ones it guards and fails on
the difference; it runs in the gate.

**Why:** every platform declaration is weak, so an unresolved second symbol is a jump to zero that
loses the rest of the run. An unguarded second call also makes the announced symbol a lie about
which call died. A rule only written down was broken in dozens of places.

**Rejected:** trusting reviewers to remember - demonstrably did not hold.
