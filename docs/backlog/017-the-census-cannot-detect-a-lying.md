# 7. The census cannot detect a lying platform - now it says so, in the report


Presence is not behaviour. A platform resolving every symbol to a stub that returns
success scores full marks on `900-surface` and is useless, and a census cannot tell the
two apart: it reads addresses and never calls anything.

**Not hypothetical.** A previous-generation emulator reports all 87 of the current
generation's graphics symbols as present, through a generic stub, for an interface it does
not implement at all.

`900-surface/presence-is-not-behaviour` reads the totals `007-responsive` gathers - the
only part of the program that knows the difference - and states the gap as the second line
of the census section, before any of the numbers it qualifies.

**Its verdict is never `pass`.** It does not test the platform, it qualifies the rest of
the section, so a green line would be one more thing to add up. It reports `partial`, and
escalates its wording: 22 of 54 probed functions responding under shadPS4 gets *"most
probed functions are stubs; read this census as an upper bound, not a coverage figure"*.
Zero responding is a `fail`.

A caveat in a section's purpose line is read once. A number in the report is read every
time somebody quotes a coverage figure, which is the moment it matters.

