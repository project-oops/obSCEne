# D133 - A consumer's strict decoder found a defective fixture the checker had passed. Both are fixed


Status: derived - from orbistoun's read/call consumer, built against the spec.

`docs/examples/protocol/06-read.txt` carried a `bytes` record with **65 hex digits** for a
read of `0x20` (32 bytes = 64 digits) - one nibble too many to be a whole number of bytes.
It survived because two things did not check it: the fixture was hand-written, and
`scripts/protocol.py` validated a `bytes` record's structure but never its hex run. A
consumer whose decoder refuses the impossible - rather than dropping or inventing a nibble,
both invisible - is what surfaced it.

Fixed on both sides, because a defect a downstream reader catches is a defect the gate
should have caught:

- The fixture now reads back `0x20` as **two 16-byte records** at offsets 0 and 16, each a
  valid 32-digit run - which also corrects a second error, that the single record ignored the
  server's 16-byte-per-record chunking. The fixture now matches what the live server emits.
- `protocol.py` now checks every `bytes` record's run is even-length and actually hex, and
  the self-test mutates a run to odd length and requires the checker to catch it. The
  invariant lives in the gate now, not only in the one consumer strict enough to notice.

The lesson is the same one that keeps recurring here: a parser that copes with the
impossible hides it, and a strict one that refuses it is doing the contract a favour. The
consumer was right to refuse and pin it; the fix is upstream, where the fixture is.

