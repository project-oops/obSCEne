# D126 - The command socket, and why the console backend is deliberately empty


Status: derived - the protocol is served and driven end to end on the host.

Same split as the file sink (D104). `net.c` holds the protocol - parsing, sequencing,
dispatch, and the rule that an acknowledgement precedes the work. The five socket calls live
in one file per target and the build compiles exactly one.

`net_posix.c` covers the host **and a general-purpose handheld**, which is one file serving
two targets and the reason the networking can be built and proven long before either console
exists. `net_target.c` refuses.

### The console backend refuses on purpose

The vendor's networking symbols are in the census by name. Their **arities and structure
layouts are not confirmed**, and D008 forbids calling a function whose signature is
uncertain. The reasoning bites unusually hard here: a wrong arity corrupts the stack and
surfaces somewhere unrelated, so guessing would produce a probe that fails mysteriously in a
*different* subsystem, on a target that took effort to reach.

So it announces nothing it cannot honour, and a driver learns the limitation from the
protocol rather than from a connection that never works. Implementing it needs confirmed
signatures from documentation or an open-source toolchain - the same provenance rule every
other declaration follows. Until then the file sink is how a console run leaves a record
behind, which is exactly why the sink was built first.

### Two bugs that only a real session could find

Both were invisible in review and obvious the moment bytes went over a socket.

**Every record was malformed from the first byte.** The prefix `OBS|` was written through
the field sanitiser, which exists to replace a separator appearing *inside* a field - so it
duly replaced the one in the prefix, and every reply went out as `OBS ack|1|hello`. The code
reads exactly as though it writes what it means to. Structure and contents are now written
by different functions, which is the distinction that was missing.

**The session identifier was the build identifier**, so every session looked identical. That
defeats the one thing it is for: a driver detects a restart by seeing an identifier it did
not expect, and a constant can never be unexpected. It is now derived from a clock where the
platform has one, and from a counter where it does not - with a one-character prefix saying
which, because an identifier whose guarantees are unknowable is worse than none.

### The implementation corrected the specification, once

A repeated sequence number was refused **without** an acknowledgement, while the document
said every request is acknowledged first. The implementation was right and the document was
incomplete: an `ack` is keyed by sequence, so acknowledging a repeat puts two
acknowledgements bearing the same key on the wire and produces a reply that cannot be
matched to a request.

The specification now states that one exception, `10-bad-sequence.txt` captures it, and two
mutations guard it. This is the contract working in the direction it was built for - the
document is what both sides agree to, so a disagreement gets resolved *in the document*
rather than in one implementation's behaviour.

