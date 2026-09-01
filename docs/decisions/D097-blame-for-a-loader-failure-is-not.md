# D097 - Blame for a loader failure is not assigned without a control build


Status: assumed.

obSCEne ran under one loader out of five. Two explanations fit that equally well - the other
four are immature, or obSCEne's module is unusual - and this project spent a long exchange
arguing between them from source code and README claims. Both readings were argued
confidently and **both were wrong at least once**:

- SharpEMU's failure was attributed to the emulator, then re-attributed to obSCEne on the
  strength of its README, then shown by experiment to be the emulator after all;
- the mechanism offered for that re-attribution - obSCEne being small enough for a wrong
  read to land in mapped memory - was invented to fit, and a 1.8 MB control disproved it in
  one command.

A README says what its authors believe. A control says what happens.

**The rule: before a loader failure is attributed to either side, the same loader is given a
module built by a standard toolchain.** Three outcomes, three meanings, none of them
arguable:

| control | obSCEne | conclusion |
|---|---|---|
| runs | fails | obSCEne's module is at fault |
| fails | fails | the loader is at fault |
| fails | runs | obSCEne is ahead, and the loader's limits are the story |

All three occurred on the first run of this experiment, which is the strongest possible
argument that reasoning would not have got there. This is the same discipline the suite
already applies to itself - the census has a must-resolve and a must-not-resolve control,
`007-responsive` compares answers to each other rather than to an invented expectation - and
it had simply never been applied to the loaders.

