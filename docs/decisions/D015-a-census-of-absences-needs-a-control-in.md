# D015 - A census of absences needs a control in both directions

**decided** · 2026-08-19

`900-surface/control` probes one symbol that must resolve and one that must not,
through exactly the path real entries use. It runs first in the section.

**Written because the first run made the problem obvious.** All 246 symbols reported
absent, every library red - which is the correct answer on a host that implements
none of the platform, and also precisely what a completely broken presence test
would produce. The two are indistinguishable from the output, and the case where they
are indistinguishable is the normal case for an emulator early in its life.

The control symbol is defined in a *different* translation unit from the census, so it
exercises real weak resolution rather than short-circuiting within one file. If the
control fails, the section says outright that every other number in it is meaningless.

