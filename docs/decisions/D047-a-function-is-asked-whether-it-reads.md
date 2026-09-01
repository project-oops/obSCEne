# D047 - A function is asked whether it reads its arguments, separately from whether it is right


Status: decided, and it changed the reading of an existing run.

A platform that has not implemented a function resolves it to a stub returning zero. A
platform that has implemented it badly returns something wrong. Both produce the same
record - `wcslen` returned 0, `atol` returned 0 - and they need opposite work: one is
"write this", the other is "you have a bug". Twenty-five failures in one run were
indistinguishable for this reason, and the best that could honestly be said about them
was "probably stubs".

`007-responsive` removes the "probably". It calls each function twice with inputs whose
answers *must* differ, and compares the two results **to each other** rather than to an
expected value:

    strlen("a") and strlen("abcdef")   must differ
    toupper('a') and toupper('z')      must differ
    strcmp("a","b") and strcmp("b","a")  must have opposite signs

A stub returns the same value both times, whatever it is. An implementation that is
merely wrong still varies with its input. The verdict therefore needs no knowledge of the
correct answer, which is what makes it hold where an expectation could be argued with.

**First run said:** `strlen` and `strcmp` respond; the other ten are silent and every one
returns zero. So that emulator implements two of twelve and stubs the rest - and the
twenty-five failures elsewhere are mostly absence, not incorrectness.

**It runs before every behavioural section**, because it changes how their failures read.

**What it does not claim.** Responding is not correct; a function that varies with its
input has only proven it is looking. And a sufficiently clever stub could vary and still
be a stub - nothing here can see that, and pretending otherwise would be the same sin
this section exists to correct.

