# D051 - Value checks use exact answers that a stub cannot give

**Status:** decided
**Date:** 2026-09-26

A value check uses inputs whose answers are exactly representable, so comparisons need no
tolerance, and at least two inputs whose answers differ, so a function returning a constant
fails. Each input aims at a plausible wrong implementation: `round(2.5)`, `trunc(-2.7)`, a
64-bit `llabs`, base-zero `strtoull`. A function with no exact answer is left out.

**Why:** an epsilon is a specification nobody wrote, and the place a wrong answer hides. A check
whose every expected value equals a stub's return value cannot tell a stub from a success.

**Rejected:** tolerances - invented expectations. Single-input checks - pass on a stub that
happens to return the right constant.
