# D013 - The entry point returns rather than exiting

**assumed** · 2026-08-19

`obscene_start` runs the suite and returns; it calls no exit function.

A loader calls it as an ordinary function and regains control. That is what makes it
possible to run the probe, collect the report, and carry on - a probe that terminates
the process cannot be run twice in one session and cannot hand a tally back to
whatever invoked it. The final tally is also left in a global so a loader can read the
outcome without parsing the stream.

