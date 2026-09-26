# D170 - A document may anchor a claim to the source

**Status:** decided
**Date:** 2026-09-26

A document states `<!-- obscene:claim file=<path> contains=<token> -->` (or `absent=`) above a
passage, and `obscene-tool claims` fails when the token no longer holds. Markers inside fenced
blocks and markers without `file=` are skipped. Claims are written by hand for the passages a reader
acts on.

**Why:** prose describing behaviour cannot be diffed, but the literal it rests on can. Inferred
claims would produce false failures.

**Rejected:** checking prose automatically.
