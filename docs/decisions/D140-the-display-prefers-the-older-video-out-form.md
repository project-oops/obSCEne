# D140 - The display prefers the older video-out form when both resolve

**Status:** decided
**Date:** 2026-09-26

The display imports both generations' video-out entry points weakly and uses the plain form when it
resolves, the `2`-suffixed form only when the plain one is absent. It asks the user service for a
user first and initialises the service only if refused. Every give-up keeps the platform's code and
releases the output handle.

**Why:** a loader that installs stubs for unresolved imports makes the newer form look present, but
a platform that has the older form works with it and a current-generation platform does not offer
it. Each fallback is conditional on the first attempt failing, so no loader that already works is
disturbed.

**Rejected:** branching on the detected generation - wrong on a platform offering both. Preferring
the newer form - selects stubs.
