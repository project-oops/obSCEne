# D059 - A third kind of record: measurements, which assert nothing


Status: decided, on the user's suggestion, correcting a limitation in the earlier design.

Every record until now carried a judgement, and a judgement needs an expectation. Where a
document supplies one that is `spec`; where nobody does, this project supplies it and marks
it `assumed`.

There is a third option and this program did not have it: **record what happened and
assert nothing**. A duration is a fact. It needs no expectation and does not go stale when
a guess turns out wrong.

The earlier objection - that timing a sleep means picking a threshold, and picking one is
inventing a specification - was aimed at the wrong thing. It argues against *asserting* on
a measurement, not against *recording* one. `OBS|measure` records carry a quantity, a
value and a unit, and no verdict.

**Assertions stay.** A loose bound marked `assumed` is still worth having, because it is
correctable and because "no expectation at all" is not a stronger position than "a stated
guess". What changes is that the number sits beside the verdict, so correcting the guess
from hardware is reading a figure and editing a constant rather than re-deriving what the
expectation should have been.

**The measurement is the calibration.** Forty-seven checks are `assumed` and none is
`hardware`. That is the ratio a console visit has to move, and a measurement makes each
move cheap and unambiguous.

