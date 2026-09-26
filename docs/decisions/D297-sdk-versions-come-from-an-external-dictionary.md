# D297 - SDK versions come from an external dictionary and are validated per generation

**Status:** decided
**Date:** 2026-09-26

The SDK version an eboot or module declares is named by alias (`SDK`, e.g. `prospero`, `orbis`)
and resolved through `selfish/data/sdk-versions.toml`, which selfish stamps into the process and
module parameter segments and rejects when it does not match the target generation.

**Why:** the loader enforces a minimum SDK version, and a zero or wrong value aborts startup. A
named table is legible and updatable without recompiling, and the generation check refuses an
invalid combination at build time.

**Rejected:** hardcoded magic version numbers - opaque and unvalidated.
