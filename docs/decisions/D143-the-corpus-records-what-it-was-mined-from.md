# D143 - The corpus records what it was mined from

**Status:** decided
**Date:** 2026-09-26

`data/mined-names.txt` carries `mined-from:` lines (`<source>@<commit>`) and a `firmware:` line.
The census check compares them with the checkouts on disk and reports drift. A machine without the
sources says so and passes. Generated headers are gated against `data/`.

**Why:** a corpus older than its sources reports `absent` for a surface it never asked about, which
reads as a platform gap. Re-mining is too slow for a gate, so the gate checks the recorded inputs
instead.

**Rejected:** re-mining in the gate. Failing on missing sources - teaches people to skip the gate.
