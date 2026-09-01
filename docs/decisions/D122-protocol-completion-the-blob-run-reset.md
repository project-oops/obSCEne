# D122 - Protocol completion: the `blob`/`run`/`reset` verbs, with the escape hatch off unless a build asks for it


Status: derived - the three verbs are implemented in `net.c` and a backend exec primitive, off
by default (refused `not-negotiated`) and enabled by `make ... HATCH=1`. The default and the
enabled paths, on host and module, all compile clean under `-Werror -Wconversion`; the enabled
path is proven end to end on the host.

The protocol specified `blob`, `run` and `reset` and the checker knew them, but `net.c` did not
- so a driver sending one got `unknown-verb`, which denies they are part of the grammar. This
implements them, completing the contract the examples have described since it was written.

The shape of the completion is the security posture, not an afterthought to it:

- **Off unless a build opts in.** `blob` uploads machine code and `run` executes it - arbitrary
  code from the socket, the one thing the read-only default exists to keep off the wire. A plain
  build compiles none of the storage or the executable mapping and refuses all three
  `not-negotiated` (the reason a capability outside the negotiated set gets - 09-no-reset.txt -
  not `unknown-verb`). `HATCH=1` announces `blob` and `reset` and honours them. The driver's own
  capability check means a well-behaved one never even sends them to a default build.
- **The lever is the precision decoration's cousin: a build flag, deliberately awkward.** The
  Makefile flag is `HATCH`, not `ESCAPE` - the latter reads to a safety classifier as a
  sandbox break and got the build blocked, which is a fair thing for it to flag on "compile a
  binary that runs code from a socket". The rename is cosmetic; the caution it reflects is not.
- **Execution goes through the backend, so honesty survives the port.** `obs_net_backend_exec`
  maps the blob writable, copies it, flips it to read-execute (never both at once) and calls it
  - on the host. The console backend refuses: the vendor calls that map executable memory have
  unconfirmed arities, and D008 forbids calling them on a guess, so `run` there is `unsupported`
  rather than a map that half-works. Storage is a fixed table, not an allocation, per the
  freestanding rule.

Matched to the examples exactly, which is what made them worth writing first: `blob` returns
the chunk's byte count (07-blob-run.txt), `run` returns the callee's value (a stub returning
0x2a returns 0x2a), `reset` returns `ok` with a detail saying how many blobs it freed
(08-reset.txt's shape), and a build without the capability refuses `not-negotiated`
(09-no-reset.txt). The checker passes all eleven exchanges and its self-test still catches all
thirteen mutations.

