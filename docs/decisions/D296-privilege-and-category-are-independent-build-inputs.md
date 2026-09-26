# D296 - Privilege tier and application category are independent build inputs

**Status:** decided
**Date:** 2026-09-26

A build declares a privilege tier (`app`, `sysmodule`, `system`, `root` - the authority id in the
SELF header, `PRIVILEGE`) and an application category (`bigapp`, `systemapp`, `miniapp` -
`applicationCategoryType`) separately. Direct memory and display ownership are governed by
category, not privilege: a `root` build gets full resources as a big app and zero direct memory as
a system app. A build that needs the screen declares category `bigapp`. `obs_module_open_tier`
records which path provided each library.

**Why:** the two axes were measured on hardware to be orthogonal, and conflating them left a root
build with a black screen and no direct memory. A root eboot can be a big app; no runtime
escalation is needed for display output.

**Rejected:** registering privileged builds as system apps by default - loses display and direct
memory.
