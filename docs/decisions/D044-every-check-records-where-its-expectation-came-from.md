# D044 - Every check records where its expectation came from

**Status:** decided
**Date:** 2026-09-26

Each check carries a provenance, appended to its `res` record, from this ladder:

```
assumed -> implementations -> spec -> documented -> derived -> hardware
```

`assumed` is this project's reasoning and the default. `implementations` needs at least two
implementations that share no codebase, named in the check's comment. `derived` means the kernel's
documented ancestor settles this specific case. Each upgrade is made per check, never by pattern.

**Why:** a failure against an ISO C expectation and a failure against this project's guess read
the same without it, and an emulator author cannot tell whose bug it is. The ladder makes the value
of each source explicit, including the caveat that implementations read each other.

**Rejected:** no provenance - every failure is equally arguable. Upgrading by rule - sweeps up
checks whose case the document does not settle.
