# D060 - Which clock counts what is established by experiment, not assumed


Status: decided.

The platform offers three time sources readable without a struct and does not say what any
of them counts. Process time counts CPU consumed, so a sleeping thread does not accrue it;
a wall clock advances regardless. Guessing wrong makes every duration wrong.

It does not need guessing. Sleep a known interval and read them all: the sources that
advanced by roughly the sleep are wall clocks, the ones that barely moved are counting CPU.

**Validated where the answer is known.** The host stubs were changed from constants to real
clocks - `CLOCK_PROCESS_CPUTIME_ID` and `CLOCK_MONOTONIC` - precisely so this experiment
runs somewhere the right answer exists. Across a 20ms sleep it reported 42µs for process
time and 20.13ms for the monotonic counter, which is the correct classification.

A section whose logic has never run is not something to hand an emulator, and constants in
the host stubs would have meant exactly that.

**First finding.** Under shadPS4 0.18.0 all three sources advanced by the full 20ms, so
`sceKernelGetProcessTime` behaves there as a wall clock. The name and the toolchain
documentation both describe CPU time. Flagged as a divergence from the host reference
rather than as a proven emulator bug: what the console does is exactly what has not been
measured yet, and this is the first check written to answer that rather than assume it.

**Second finding.** `sceKernelUsleep` returns early - 217µs short of a 5ms request and
712µs short of a 20ms one, in the same direction across two runs, where the host overshoots
every time. A sleep is a lower bound. The first version of the check could not see this,
because its floor was half the request; the shortfall is now its own recorded quantity and
a `partial` verdict.

