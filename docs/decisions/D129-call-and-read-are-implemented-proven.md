# D129 - `call` and `read` invoke and dump without pre-validating

**Status:** decided
**Date:** 2026-09-26

The `call` verb invokes an address with up to six integer arguments; `read` dumps memory as
`bytes` records. A malformed address is refused; a valid address that faults is not pre-checked -
the `ack` is already on the wire, so the death reads as a lone `ack`. They are announced as
capabilities; the write and code-execution verbs stay off (D122).

**Why:** on this ABI the caller cleans up, so a mismatched arity cannot corrupt the stack, and
these verbs assert nothing. "Not readable" and "asking killed it" are different facts, and dying
reports the second.

**Rejected:** pre-validating the address - hides the death path a consumer must handle.
