# D331 - A borrowed value is a hypothesis a check can refute, not an expectation

**Status:** decided
**Date:** 2026-09-26

Where implementations disagree on a value (the `*GetSize` command-buffer sizes), the check records
every number - the reported size, the top half of the return, the byte count, what the builder then
wrote - and judges only the invariant this project can defend: a reservation must cover the write.
The measured figures are the output; the only failure is a reservation shorter than the write. A
single reading is swept over several arguments to tell a constant from a function of its argument.

**Why:** there is no consensus to adopt, and asserting either side would write an expectation this
project cannot defend and fit the instrument to the answer. The relationship is falsifiable without
any prior number.

**Rejected:** asserting a borrowed size - launders a hypothesis into a fact and can break a working
title to match a table.
