# D056 - A value check whose every expected answer is zero cannot tell a stub from a success


Status: bug in this project's own work, found under an emulator.

`037-math/inverse-trigonometry` asserts `tan(0) == 0`, `asin(0) == 0`, `acos(1) == 0`,
`atan(0) == 0` and `atan2(0, 1) == 0`. Those are the only exact answers those functions
have - `asin(1)` is pi/2, which no binary format holds exactly - so the check was written
entirely out of zeros, and a function returning zero to everything passes all five.

It passed under shadPS4, where the responsiveness verdict says those functions do in fact
work. The pass happened to be correct and was not evidence of anything.

This is the failure `007-responsive` exists to prevent, committed inside a value check
where that section could not see it. The generalisation is worth stating: **a check is
only as good as the difference between passing and failing it**, and a check whose
expected values are all the same as a stub's return value has no such difference.

Fixed by requiring that differing inputs produce differing answers, in the same check -
the responsiveness argument applied in place rather than in a separate section.

Twenty-seven responsiveness probes were added at the same time, covering everything
promoted in D051, so that a failure in `035-libc` or `037-math` can always be read against
whether the function answers at all. Under shadPS4 that turned nine "wrong answer"
findings into nine "not implemented" findings, which need entirely different work.

