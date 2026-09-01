# D078 - The census now states its own limit, in the report, where a coverage figure gets quoted


Status: decided, closing BACKLOG §7.

Presence is not behaviour. A platform resolving every symbol to a stub that returns success
scores full marks on the census and is useless, and the census cannot tell the difference -
it reads addresses and never calls anything.

That stopped being hypothetical when the current generation's graphics interface was
censused: a previous-generation emulator reports all 87 symbols present, through a generic
stub, for an interface it has no implementation of.

`900-surface/presence-is-not-behaviour` reads the totals `007-responsive` gathers - the only
part of this program that knows the difference - and states the gap as the second line of
the census section, ahead of the numbers it qualifies.

**Its verdict is never `pass`.** It does not test the platform, it qualifies a section, so
a green line would be one more thing to add up and one more reason to stop reading. It
reports `partial` and escalates its wording with the evidence: 22 of 54 probed functions
responding gets "most probed functions are stubs; read this census as an upper bound, not a
coverage figure". Zero responding is a `fail`.

The backlog item said the report "should probably say so more loudly than a line in the
section purpose". The reason that matters: a caveat in prose is read once, and a number in
the report is read every time somebody quotes a coverage figure - which is exactly the
moment the caveat is needed.

