# D019 - The diff is the point; the report format is downstream of it

**decided** · 2026-08-19

`tools/diff.py` compares two reports and exits non-zero on regression. `make diff`
runs it against a saved baseline.

D003 said the format is machine-readable because the intended reader is an agent
diffing runs to decide whether a change helped. That was the justification for every
awkward choice on the C side - stable identifiers, one record per line, values as raw
numbers rather than prose - and the tool it justified did not exist. The format was
paying a cost for a capability nobody had built.

**The definition of regression is the load-bearing part.** A check that got *worse*,
not a check that is failing. A run where everything fails and nothing changed exits
zero, because nothing regressed. Conflating the two would make the tool useless in
exactly the situation it exists for: an emulator where almost everything fails and the
question is whether today is better than yesterday.

Statuses order `skip < fail < partial < pass`, with **skip below fail deliberately**.
A check that stopped running tells you less than one that ran and failed, so losing
coverage is a regression even though nothing went red. A check vanishing from the
report entirely counts the same way - otherwise deleting an inconvenient check would
read as an improvement.

The `build` record exists for the same reason: without it a diff cannot separate "the
probe changed" from "the platform changed", and those demand opposite responses.

