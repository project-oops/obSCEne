# D015 - A census needs a control in both directions

**Status:** decided
**Date:** 2026-09-26

`900-surface/control` probes one symbol that must resolve and one that cannot exist, through the
same path the census uses, and runs first. If either answer is wrong, the section reports every
count in it as meaningless. The run-time census carries the same control for module resolution
(D229).

**Why:** a platform that implements nothing and a broken presence test produce the same all-absent
report, and a loader that stub-resolves everything produces an all-present one. Only the control
separates a measurement from an instrument failure.

**Rejected:** trusting the counts - both failure shapes are the normal case on some loader.
