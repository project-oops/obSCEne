# D122 - Code-execution verbs are off unless a build asks for them

**Status:** decided
**Date:** 2026-09-26

The protocol verbs `blob`, `run` and `reset` exist only in a build made with `HATCH=1`. A default
build compiles none of their storage or executable mapping and refuses them `not-negotiated`.
Where the platform cannot map executable memory with a confirmed signature, `run` reports
`unsupported`.

**Why:** these verbs run arbitrary code from a socket, which the read-only default exists to keep
off the wire. Refusing them as un-negotiated keeps them part of the grammar.

**Rejected:** always on - arbitrary code execution on every build. `unknown-verb` - denies they
exist in the protocol.
