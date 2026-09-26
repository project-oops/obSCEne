# D132 - A serving build listens first and runs the suite on demand

**Status:** decided
**Date:** 2026-09-26

A build made with `SERVE=1` (or `obscene-deck --serve`) opens the command socket at once and runs
the suite only when a driver sends `report`, streaming every record down the socket as well as to
the usual destinations. It draws a HUD-only screen.

**Why:** the socket should not depend on the least stable part of the program. A crash during
`report` then costs one session and a reconnect, not the endpoint, and a serving build needs no
exclusion list.

**Rejected:** running the suite before listening - an unstable check keeps the socket from ever
opening.
