# D102 - The command protocol is specified first, with transcripts as contract

**Status:** decided
**Date:** 2026-09-26

`docs/PROTOCOL.md` is the specification; the probe's `net.c` and `obscene-tool drive` implement
it. Captured exchanges under `docs/examples/protocol/` are part of the contract and are checked
against the grammar. An `ack` is flushed before the command runs, every acknowledged command gets
exactly one terminal reply, and the driver records `died`, `timeout` and `lost` itself, marked
`observed_by=driver`.

**Why:** another implementation builds against the protocol and has no say in it, so the grammar
must exist as a document. Transcripts let a consumer be built with no hardware. A command that did
not answer is never recorded as having answered.

**Rejected:** implement first and document after - every correction becomes a silent break.
