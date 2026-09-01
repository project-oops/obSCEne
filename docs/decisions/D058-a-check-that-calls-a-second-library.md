# D058 - A check that calls a second library must test that symbol's address itself


Status: bug, found on the host build, generalised.

The harness skips a check whose declared symbol resolved to null, because jumping to zero
takes the process down and loses everything behind it. That guard covers **the one symbol
in the check's table row**, and nothing else the body reaches for.

`017-posix/spellings-agree` calls two libraries by design. On the host build the vendor
spelling has no stub, so `scePthreadRwlockInit` was null, and the check walked straight
into it. Segmentation fault, process gone.

Two things worked exactly as intended and are worth naming:

**The `try` record with no `res` identified it precisely.** Not "somewhere in this run" -
the exact check. That invariant is the most important property in the program and this is
the third time it has paid for itself.

**It happened on the host.** A null vendor symbol is the normal state on an ordinary
machine and an unusual one under an emulator, so the host build met this first - which is
the entire argument for having it (D001). Under an emulator it would have appeared as an
intermittent crash in a section that usually works.

Fixed by testing every symbol the body calls, since all platform declarations are weak and
the address is therefore the test. **The general rule:** a check that reaches outside its
own table row is responsible for the addresses it reaches for. Adding this to the
five-step list in CLAUDE.md rather than trusting anyone to remember it.

