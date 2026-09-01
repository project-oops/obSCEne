# Network handover for orbistoun


Wrote docs/HANDOVER-ORBISTOUN-NET.md - a self-contained guide for the consumer thread to
build its client and GUI against, copyable as data (no cross-repo dependency).

Grounded in what the server actually does, not just the spec. Two accuracy points verified
against net.c and surfaced prominently, because building against the spec alone would get
them wrong:
- `report` returns only a fail-count summary over the socket; the individual records go to
  the probe's own stdout/file sink, not down the connection. Socket-streaming is a listed
  TODO.
- Only hello/report/bye are implemented; call/read/write/blob/reset are reserved and refused
  today. call/read are the next priority - they are what make it a CPU/NID oracle.

Carries forward D108's rule: machine origin is operator-asserted, never probe-claimed, so
the GUI must collect it. And the CI boundary: replay transcripts, never open a socket.

