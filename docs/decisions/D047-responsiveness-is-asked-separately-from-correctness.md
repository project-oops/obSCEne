# D047 - Responsiveness is asked separately from correctness

**Status:** decided
**Date:** 2026-09-26

`007-responsive` calls each function twice with inputs whose answers must differ and compares the
results to each other, reporting `responds` or `silent`. It runs before the behavioural sections.

**Why:** a stub returning zero and a wrong implementation both fail a value check, and need
opposite work. Comparing two answers needs no knowledge of the correct one, so the verdict holds
where an expectation could be argued with.

**Rejected:** reading value failures alone - cannot separate "write this" from "you have a bug".
