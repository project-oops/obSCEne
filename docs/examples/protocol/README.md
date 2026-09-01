# Captured exchanges

One transcript per scenario. These are **part of the contract**, not illustrations of it:
`docs/PROTOCOL.md` gives the grammar, and these give a consumer something to be built and
tested against with no hardware attached.

`obscene-tool protocol` parses every file here against the grammar and fails if a line
does not conform, so an example cannot quietly stop matching the specification.

## Reading them

- `CMD|` lines went from the driver to the probe.
- `OBS|` lines came back, **except** where an annotation says otherwise - the two non-answer
  outcomes are established by the driver, because a probe that has died cannot report it.
- `#` lines are annotation. They are not protocol and never appear on the wire.

## The scenarios

| file | what it pins down |
|---|---|
| `01-hello.txt` | version and capability negotiation, and the `part` records |
| `02-resolve-call.txt` | the ordinary path: name to address, address to return value |
| `03-died.txt` | **an `ack` with no `done`.** The case the format exists for |
| `04-timeout.txt` | not `died`, and deliberately not resolved into it |
| `05-refused.txt` | unknown verbs, and a capability that was never negotiated |
| `06-read.txt` | memory as `bytes` records, and an unmapped address |
| `07-blob-run.txt` | the escape hatch: chunked upload, then a call |
| `08-reset.txt` | state between commands, including a target that cannot reset |
