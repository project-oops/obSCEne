# D144 - A timeout is told from a hang by doubling the budget

**Status:** decided
**Date:** 2026-09-26

When a sweep round ends on its timeout, `scripts/sweep.sh` retries the same build with twice the
budget. Only a stopping point identical under the doubled budget is a hang and excluded; otherwise
the doubled budget is kept. Retries count against `--max-rounds`. Progress is read from `checks n
of m`, not from record counts.

**Why:** a killed run leaves the same trace as a hang, so excluding on one timeout blames whichever
check was running. Two runs separate them by measurement. Record totals move opposite to progress
when exclusion lists differ.

**Rejected:** an operator flag asserting the budget is generous enough - a judgement where a
measurement will do.
