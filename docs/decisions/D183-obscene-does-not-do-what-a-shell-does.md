# D183 - obSCEne does not do what a shell does

**Status:** decided
**Date:** 2026-09-26

obSCEne answers questions only a loaded guest can answer: what a platform function returns in a
process loaded the way a title is. Browsing files, listing processes and launching payloads are left
to the shell services and stay out of scope.

**Why:** a shell operates the machine from outside a guest process; this probe interrogates the ABI
from inside one. The overlap is a few system facts.

**Rejected:** growing file or process browsing into the probe.
