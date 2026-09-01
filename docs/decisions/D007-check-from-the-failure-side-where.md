# D007 - Check from the failure side where layouts are unknown

**assumed** · 2026-08-19

Much of the suite passes deliberately invalid arguments and expects an error:
`sceKernelClose(-1)`, `sceVideoOutClose(-1)`, opening a path nothing exists at.

Calling these properly would mean passing structures whose layouts this project is
not confident about, and guessing at one corrupts the stack - the resulting crash
lands nowhere near the mistake, which is the exact failure this program exists to
find rather than to contain. Negative checks need no layout and still prove the
function exists, is reachable, validates its arguments, and returns something
plausible.

**The limitation, stated honestly:** an implementation that fails *everything* passes
every negative check. Negative checks prove argument validation and nothing more.
Only the positive checks - the memory round trip, the thread whose body must actually
have run - prove a function does its job. Both kinds are present and the report
distinguishes them, but the ratio should shift toward positive checks as signatures
become known.

