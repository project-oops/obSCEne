# D007 - Check from the failure side where layouts are unknown

**Status:** decided
**Date:** 2026-09-26

Where a function's structure layouts are not established, the check passes an invalid argument
and expects a refusal. Wherever a signature is confirmed, a positive check - a memory round trip,
a thread whose body must have run - is added, and the report distinguishes the two kinds.

**Why:** a guessed layout corrupts the stack and crashes somewhere unrelated. A negative check
needs no layout and still proves the function exists, is reachable and validates its arguments.
It proves nothing more: an implementation that fails everything passes every negative check,
so positive checks are preferred as soon as they are possible.

**Rejected:** calling with guessed structures - the failure this program exists to find, not to
cause. Negative checks only - cannot tell a working function from one that refuses everything.
