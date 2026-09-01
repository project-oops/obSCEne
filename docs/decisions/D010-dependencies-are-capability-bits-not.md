# D010 - Dependencies are capability bits, not check names

**assumed** · 2026-08-19

A check declares what it requires and what a pass provides, as a bitmask.

Naming a prerequisite check directly would mean string comparison in a freestanding
binary and a dependency that breaks silently when a check is renamed. A mask compare
needs no allocation and no string handling. It also expresses the right thing:
`020-memory/map` does not care *which* check established that memory works, only that
something did.

