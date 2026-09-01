# Serving robustness, report streaming, Deck target


- Listen-first serving build (D132): the socket opens before the suite runs, so it is
  reachable in ~3s in shadPS4 with no exclusion list, and a mid-suite crash no longer costs
  the endpoint. Proven live: hello/read/call/bye and reconnect, all inside shadPS4.
- `report` streams its records over the socket (additive write tee), matching a plain run
  exactly (5909 sym, no duplication).
- `make deck`: host-shaped Linux build + GPU=1, the RDNA2 oracle, serves via --serve.

### Surprise
- shadPS4 is non-deterministic: same serving binary died mid-suite on two launches and
  completed on a third. Host runs all 26 sections deterministically, so the finger points at
  shadPS4 - but "we cannot prove it is not ours without hardware" is exactly the gap a
  reference device fills. Fixed our exposure to it (listen-first) rather than the heisenbug.

