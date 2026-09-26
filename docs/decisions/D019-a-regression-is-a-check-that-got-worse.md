# D019 - A regression is a check that got worse

**Status:** decided
**Date:** 2026-09-26

`obscene-tool diff` compares two reports and exits non-zero only when a check got worse. Statuses
are ordered with `skip` below `fail`, and a check that vanished counts as a regression. The
`build` record lets a diff tell a probe change from a platform change.

**Why:** the intended use is an emulator where most things fail and the question is whether today
is better than yesterday. A tool that failed on every failing check would be useless there.
Losing coverage is a regression even when nothing turns red.

**Rejected:** failing on any failing check - always red. Ignoring vanished checks - deleting an
inconvenient check would read as an improvement.
