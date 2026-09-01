# D102 - The command protocol is specified before it is implemented, and the captured exchanges are part of the contract


Status: assumed.

obSCEne owns the protocol. Another implementation builds against it and has no say in it -
which is only fair if the grammar exists as a document it can read. So `docs/PROTOCOL.md`
is written first, and the C and the driver are implementations of it.

The alternative was to implement first and document after. That makes obSCEne's C the
reference, and then every correction to the C is a silent break for anyone who built
against what it used to do. A specification derived from an implementation is not a
specification; it is a description, and it changes whenever the description does.

### Nine transcripts, and they are not illustrations

`docs/examples/protocol/` holds one captured exchange per scenario. They exist so **a
consumer can be built and tested with no hardware attached**, which is the difference
between a contract and an intention.

Prose is agreed to and then interpreted differently by each side. A transcript is
checkable, and `scripts/protocol.py --check` parses every one against the grammar, so an
example cannot drift away from the document without something saying so.

### The rule the whole format exists for

> A command that did not answer is never recorded as having answered.

`ack` is flushed before the command runs, so an `ack` with no result names the call that
ended the process - the same principle the report already applies to itself. `died`,
`timeout` and `lost` are three separate outcomes and none of them collapses into
`returned 0`. A corpus that blurs them is worse than no corpus: the fiction is
indistinguishable from evidence, and it is the fiction that gets trusted.

Recording a non-answer is **the driver's obligation**, because the probe is gone and cannot
do it. Those records are marked as observed by the driver, so a reader can tell a fact the
system reported from one inferred from its silence.

