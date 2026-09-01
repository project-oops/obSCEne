# D052 - `div` and `ldiv` were checked, then withdrawn to the census


Status: decided, against an earlier decision in the same session.

They return a struct by value, an ABI path nothing else here exercises: eight bytes come
back in one register, sixteen in two, and an implementation can be numerically perfect
and get the register class wrong. Worth a check on the face of it.

**ISO C does not specify the member order.** It fixes that the type holds `quot` and
`rem` and leaves the order to the implementation, so declaring the struct here would be
inventing a layout - the thing D008 exists to forbid. An order-independent check was
written instead, reading the two members as an unordered pair. It worked, and it stated
its own limit: it cannot tell a correct result from a swapped one.

**What killed it was the host build.** Reaching the real `div_t` means including
`<stdlib.h>`, and glibc defines several of this file's other declarations inline - so the
include turned one conflict into a dozen. The alternatives were to special-case the host
build further, or to skip the check there.

Skipping it there is not available. `make host` is what makes a bug in this probe
distinguishable from a bug in the thing being measured (D001), and a check that has never
run against a known-good implementation is not evidence. That is worth more than the
check, so the check went.

They stay in the census. Presence is what can be honestly claimed about them, and the
reasoning is written into `platform.h` where the next person to think this is a good idea
will find it.

