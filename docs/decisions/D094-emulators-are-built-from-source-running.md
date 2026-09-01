# D094 - Emulators are built from source. Running a binary that does not match the source read is a method error


Status: policy change, and a correction to findings already reported.

The toolkit ran downloaded binaries and read shallow clones of the source, on the reasoning
that the source was for *reading* and the binary for *running*. Those are the same
question asked twice, and the two answers were never checked against each other.

They do not match. The shadPS4 clone sits at `be21649` (2026-08-20); the binary reports
`v0.18.0`, revision `e3ce810`. **Every conclusion drawn by reading that source to explain
that binary's behaviour rests on an assumption that is false.**

Specifically, these were reported as facts and are now claims about a different commit:

- "shadPS4 has no barrier implementation, so the stub returns success with a null handle"
- "shadPS4 implements `sceKernelIsStack` and delegates to its memory manager"
- "shadPS4 does not implement `malloc` at all"
- "shadPS4 registers no `sceAgc*` through its export macro"

Each may still be true. None was established the way it was written up, and the
*observations* behind them - the null handle, the two identical answers, the null return -
stand regardless, because those came from running the thing.

**The rule now: if a loader's behaviour is going to be explained by its source, the binary
must be built from that source.** craziiEmu and fpPS4 already are. SharpEMU is being added
on the same basis. shadPS4 and Kyty are the outstanding cases, and until they are built the
findings above are marked as what they are.

This is the same failure the project keeps meeting from a new angle: a mechanism trusted
because it seemed reasonable, never checked. Reading the version string out of a report and
`git log` out of a clone took one command, and nobody ran it for weeks.

