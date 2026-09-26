# D189 - The console transport is a shared crate, taken by path

**Status:** decided
**Date:** 2026-09-26

The client for a console's services lives in `pros-link`, in the prosperous repository, and
`obscene-tool` takes it as a path dependency. The console registry and this project's defaults stay
here. The repositories are checked out side by side, locally and in CI.

**Why:** the emulator needs the same transport, and two copies would each rediscover the same
mistakes. A path dependency avoids a release process for two adjacent consumers.

**Rejected:** a second copy of the transport. A published crate - a release process to owe.
