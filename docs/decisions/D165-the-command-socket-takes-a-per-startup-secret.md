# D165 - The command socket takes a secret generated per startup

**Status:** decided
**Date:** 2026-09-26

A serving probe generates a secret when it starts listening and shows it on the HUD as `KEY`. The
first `hello` must carry it as a trailing field; otherwise the reply is `refused|unauthorised` and
every later verb is `not-negotiated`. The comparison is constant-time. The target backend's entropy
is best-effort and described that way.

**Why:** on a console the module binds every interface, so any device on the network could drive
it. A secret per startup is not shared by everyone holding the module. The screen is the only
channel a console has to show it.

**Rejected:** a build-time secret - shared by every copy. SSH inside the probe - not freestanding,
and its bugs would read as platform bugs. No authentication.
