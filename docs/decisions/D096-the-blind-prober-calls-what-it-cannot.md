# D096 - The blind prober calls what it cannot describe, and that is not a breach of D008


Status: assumed.

`910-bulk` calls every censused symbol with zero in all six argument registers and records
what comes back. Every one of those functions has an unknown signature, which is exactly
what D008 exists to forbid.

The rule survives because of what it is for. Its two stated costs are a wrong arity
corrupting the stack, and a wrong constant making a call succeed and do the wrong thing
silently. Neither is reachable here:

- **The stack is safe.** On this ABI the first six integer arguments travel in registers and
  the *caller* cleans up, so handing six registers to a function that reads two leaves the
  stack exactly as found. This is the same property that makes `printf` possible.
- **Nothing can be silently wrong**, because nothing here has an expectation to be wrong
  about. `130-layout` and `140-oracle` already route around D008 this way: a record is not a
  claim.

So D008 governs *expectations*, not *calls* - and this is now the third section to rely on
that reading, which makes it worth stating as a rule rather than re-deriving.

### What it buys

The one thing the suite could not measure: **whether there is an implementation behind a
resolved address.** The census counts addresses, and an emulator resolving all 383 imports
to a shared do-nothing stub scores full marks on it.

A returned vendor error code cannot be produced by that stub. Argument validation ran, so
somebody wrote the function. `zero` proves nothing either way and is labelled as proving
nothing.

### What it costs, and the two hazards found by running it

Some functions do not return, so a run reaches the first one and stops - the resume harness
turns that into "rounds equal to the number of fatal functions" rather than a wall. The
first host run demonstrated the mechanism at index 159: `div(0, 0)`, SIGFPE, announcement
intact, resume clean.

Two things the design did not anticipate, both found on the first sweep:

1. **Arithmetic faults, not just null dereferences.** `div`, `ldiv` and `lldiv` take their
   divisor by value, so a zeroed register is a division by zero. Listed as a hazard class
   in its own right because no amount of pointer checking would have predicted it.
2. **A round can fail to end.** The first sweep sat for fifteen minutes on one index before
   anyone looked. Every round is now bounded, inside the VM so the guest process is what is
   bounded rather than the call to it. The project already knew this - "a probe that hangs
   loses every check behind it, which has happened twice" - and this was the third.

