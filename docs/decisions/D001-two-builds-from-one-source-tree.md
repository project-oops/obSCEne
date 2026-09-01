# D001 - Two builds from one source tree

**assumed** · 2026-08-19

`make target` produces the freestanding module a loader runs. `make host` produces a
native binary with every platform function stubbed.

The host build is not a convenience. Without it, the first time this program runs is
inside an emulator that does not work yet, and a bug in the harness is
indistinguishable from a bug in the thing being measured. The host build makes the
framework falsifiable on its own: the expected outcome is a full sheet of red, in the
right order, with the dependency skips in the right places. That is a claim that can
be checked, and `tools/verify.py` checks it.

It caught a real bug the same day it was written (see D011).

