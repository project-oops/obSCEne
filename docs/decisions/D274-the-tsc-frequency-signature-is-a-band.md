# D274 - The TSC-frequency signature is a band, not an equality; four more export candidates


**measured** - 2026-08-31

`139-exports` confirms a candidate vaddr by calling `base + vaddr` and checking the function
behaved. The frequency candidate (`sceKernelGetTscFrequency`) checked `fn() == 0x5f259b8e`, an
exact value copied from an earlier measurement. On the next hardware run it recorded the function
**refuted** - and the function was answering correctly: the console returned `0x5f259bdd`, ~79 Hz
away. The counter frequency is calibrated at boot and the calibration lands a few dozen hertz apart
between boots, so an equality against one boot's value refuses the real function on every other. It
is the exact failure §2 warns about - a constant treated as more certain than it is - turned into a
false negative instead of a false positive.

Fixed with a band: `OBS_TSC_FREQUENCY_MIN..MAX` = `0x5f259000..0x5f25a000`, a few-thousand-hertz
window around the measured value. Wide enough to survive recalibration, far too tight for anything
that is not this clock to land inside by accident, so it stays a strong signature.

Added four candidates reached the same way (by `base + vaddr`, so no declaration or import is
needed - the section's own base gate does the guarding): `getsid` (stable id), `sceKernelReadTsc`
and `sceKernelGetProcessTimeCounter` (monotonic clocks), and
`sceKernelGetProcessTimeCounterFrequency` (the same frequency band). Each is a function with a
signature a wrong address is overwhelmingly unlikely to satisfy, which is what §7 asks for. Vaddrs
taken from the confirmed table; the source carries no weight, only the behaviour on the next run
will. `make host`/`make check` green; the section skips off a payload, as designed.

