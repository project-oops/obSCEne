# D252 - Two codes from one call are worth more than one code from two runs


*status: measured*

A note on method, because the sweep worked for a reason worth naming.

A single refusal code is nearly uninformative: it says a step failed. **Two codes from
neighbouring inputs bracket the cause.** `format=0` giving `0x80290003` where the baseline gives
`0x80290015` says the format field is read *and* that the baseline format is fine - two facts
from one extra call, neither available from any number of runs that only vary whether the whole
thing worked.

That is why the variations differ from the baseline in exactly one argument. A sweep where two
things change at once produces a table nobody can read.

