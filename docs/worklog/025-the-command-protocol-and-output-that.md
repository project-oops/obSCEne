# The command protocol, and output that survives the run


Deliverable one of the hardware-probe workstream, plus the first item of its order.

**The specification came first** (D102). `docs/PROTOCOL.md` defines the grammar, the record
shapes, capability negotiation, and the three ways a command can fail to answer. Ten captured
exchanges in `docs/examples/protocol/` are part of the contract rather than illustrations of
it - they are what lets a consumer be built and tested with no hardware attached.

`scripts/protocol.py` parses every transcript against the grammar, and
`scripts/protocol-selftest.py` breaks them twelve ways and requires the checker to catch each
one. Both run in `verify.sh`, self-test first, because a gate has to be shown to say no before
it is trusted to say yes (D125).

**The file sink** (D104). The report now reaches disk as well as a terminal, written a record
at a time, never buffered - a buffered file loses exactly the records a crash makes valuable.
The path is discovered from four candidates and the winning one is reported, so a run whose
sink failed is distinguishable from a build that never had one.

**The socket** (D126). The protocol is served and driven end to end on the host. The console
backend refuses deliberately: those signatures are not confirmed and D008 forbids guessing at
them.

### Surprises

**A test that could not fail, testing a gate for whether it could fail.** The first mutation
test pointed the checker at a broken copy through an environment variable the checker ignored,
so all eight runs re-checked the originals and reported every mutation caught. Fixing it
immediately found a real hole: the checker accepted a truncated transcript whenever the file
mentioned `died` anywhere, and one file mentions `03-died.txt` in a comment.

**The wire format was wrong from its first byte** and no amount of reading found it. The
record prefix went through the sanitiser that replaces separators *inside* fields, so every
reply read `OBS ack|1|hello`. One real session over a real socket made it obvious.

**The implementation was right and the specification was wrong**, once, about acknowledging a
repeated sequence number. Resolved in the document, which is the point of having one.

**Three escaping mistakes in one session**, all the same: building C or Makefile text inside a
Python heredoc, where `\n` becomes a newline the target language did not want. It produced a
Makefile continuation containing a literal `n`, and a `fprintf` broken across two lines.
Structured text gets the editor, not string replacement - a lesson this session had to learn
three times to keep.

