# D315 - A document beside a Makefile names that Makefile's rules

**Status:** decided
**Date:** 2026-09-26

`obscene-tool doccheck` resolves a `make <rule>` reference against the root Makefile and against a
Makefile in the document's own directory, if there is one.

**Why:** a README beside its own Makefile that tells the reader to `cd` there and run a rule was
reported as naming a missing rule. A negative from a gate that looked in only one place is a fact
about the gate.

**Rejected:** requiring every document to write `make -C <dir>` - makes the document worse to
satisfy the gate.
