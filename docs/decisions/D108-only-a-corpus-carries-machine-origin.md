# D108 - A report and a corpus differ, and only the corpus carries machine origin

**Status:** decided
**Date:** 2026-09-26

A report (`OBS|`) is what the probe emits alone; its only origin is `build`. A corpus
(`OBSCORPUS|`) is what the driver produces from a session, with machine identity - target, GPU,
firmware, whether it is real hardware - asserted by the operator through `drive --part` and
denormalised onto every line. What the probe observes about itself travels as ordinary records.

**Why:** the probe cannot certify its own machine; inside an emulator a version query returns the
emulator's choice. Self-reported identity would be an assumption presented as a measurement.

**Rejected:** the probe stamping its own firmware and target.
