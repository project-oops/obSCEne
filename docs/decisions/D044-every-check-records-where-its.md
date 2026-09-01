# D044 - Every check records where its expectation came from


Status: decided.

A conformance suite is worth what its expectations are worth, and they are not all worth
the same. "`strlen` returns the number of characters" is settled by a standard anyone can
read. "closing an invalid handle returns non-zero" is a belief this program holds that
nobody has confirmed. Both produce a FAIL, and the report presented them identically -
so an emulator author reading one could not tell whether it was their bug or ours.

`obs_check` now carries `obs_provenance`, appended to the `res` record:

| | means |
|---|---|
| `spec` | ISO C or POSIX. Settled by a document. |
| `documented` | vendor documentation describes this behaviour specifically |
| `hardware` | observed on a real console and recorded |
| `assumed` | this program's own reasoning; sensible, unconfirmed |

**Nothing carries `hardware` yet, and the report says so out loud.** The current split is
51 assumed, 30 spec. `pretty` prints that summary and, while no check is hardware-backed,
adds the caveat that a failure marked `[assumed]` may be this suite's belief rather than
a bug.

**That absence is the point.** It makes the value of a hardware run concrete: such a run
does not merely check the suite, it *upgrades* it, moving checks out of `assumed` one at
a time. Only after that is a green run something an emulator can be held to.

**Done now because it gets more expensive with every check added**, and the suite is at
its smallest it will ever be. `assumed` is zero so the conservative answer is the default
- a new check that says nothing about its provenance is correctly described as an
assumption.

The initial assignment is by section: ISO C, maths and pthreads are `spec`, everything
else `assumed`. That is deliberately coarse and deliberately pessimistic. Refining it is
what hardware is for.

